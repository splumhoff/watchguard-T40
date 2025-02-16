/******************************************************************************
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or 
 *   redistributing this file, you may do so under either license.
 * 
 *   GPL LICENSE SUMMARY
 * 
 *   Copyright(c) 2007-2012 Intel Corporation. All rights reserved.
 * 
 *   This program is free software; you can redistribute it and/or modify 
 *   it under the terms of version 2 of the GNU General Public License as
 *   published by the Free Software Foundation.
 * 
 *   This program is distributed in the hope that it will be useful, but 
 *   WITHOUT ANY WARRANTY; without even the implied warranty of 
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 *   General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License 
 *   along with this program; if not, write to the Free Software 
 *   Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *   The full GNU General Public License is included in this distribution 
 *   in the file called LICENSE.GPL.
 * 
 *   Contact Information:
 *   Intel Corporation
 * 
 *   BSD LICENSE 
 * 
 *   Copyright(c) 2007-2012 Intel Corporation. All rights reserved.
 *   All rights reserved.
 * 
 *   Redistribution and use in source and binary forms, with or without 
 *   modification, are permitted provided that the following conditions 
 *   are met:
 * 
 *     * Redistributions of source code must retain the above copyright 
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright 
 *       notice, this list of conditions and the following disclaimer in 
 *       the documentation and/or other materials provided with the 
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its 
 *       contributors may be used to endorse or promote products derived 
 *       from this software without specific prior written permission.
 * 
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * 
 *  version: SXXXX.L.0.5.0-46
 *
 *****************************************************************************/

/**
 ***************************************************************************
 * @file lac_sym_api.c      Implementation of the symmetric API
 *
 * @ingroup LacSym
 *
 ***************************************************************************/

/*
*******************************************************************************
* Include public/global header files
*******************************************************************************
*/

#include "cpa.h"
#include "cpa_cy_sym.h"

#include "Osal.h"

#include "icp_adf_init.h"
#include "icp_adf_transport.h"
#include "icp_adf_transport_dp.h"
#include "icp_accel_devices.h"
#include "icp_adf_debug.h"
#include "icp_qat_fw_la.h"

/*
 ******************************************************************************
 * Include private header files
 ******************************************************************************
 */
#include "lac_common.h"
#include "lac_log.h"
#include "lac_mem.h"
#include "lac_mem_pools.h"
#include "lac_list.h"
#include "lac_sym.h"
#include "lac_sym_qat.h"
#include "lac_sal.h"
#include "lac_sal_ctrl.h"
#include "lac_session.h"
#include "lac_sym_cipher.h"
#include "lac_sym_hash.h"
#include "lac_sym_alg_chain.h"
#include "lac_sym_stats.h"
#include "lac_sym_partial.h"
#include "lac_sym_qat_hash_defs_lookup.h"
#include "lac_sym_cb.h"
#include "lac_buffer_desc.h"
#include "lac_sync.h"
#include "lac_hooks.h"
#include "lac_sal_types_crypto.h"
#include "sal_service_state.h"

/* Macro for checking if partial packet are supported for a given
 * symmetric operation */
#define IS_PARTIAL_ON_SYM_OP_SUPPORTED(symOp, cipherAlgorithm,           \
                                       hashAlgorithm, hashMode )         \
            (((CPA_CY_SYM_OP_CIPHER == symOp) &&                         \
              (CPA_CY_SYM_CIPHER_KASUMI_F8 != cipherAlgorithm) &&        \
              (CPA_CY_SYM_CIPHER_AES_F8 != cipherAlgorithm) &&           \
              (CPA_CY_SYM_CIPHER_SNOW3G_UEA2 != cipherAlgorithm)) ||     \
             ((CPA_CY_SYM_OP_HASH == symOp) &&                           \
              (CPA_CY_SYM_HASH_KASUMI_F9 != hashAlgorithm) &&            \
              (CPA_CY_SYM_HASH_SNOW3G_UIA2 != hashAlgorithm)) ||         \
             ((CPA_CY_SYM_OP_ALGORITHM_CHAINING  == symOp) &&            \
              (CPA_CY_SYM_CIPHER_KASUMI_F8 != cipherAlgorithm) &&        \
              (CPA_CY_SYM_CIPHER_AES_F8 != cipherAlgorithm) &&           \
              (CPA_CY_SYM_CIPHER_SNOW3G_UEA2 != cipherAlgorithm) &&      \
              (CPA_CY_SYM_HASH_KASUMI_F9 != hashAlgorithm) &&            \
              (CPA_CY_SYM_HASH_SNOW3G_UIA2 != hashAlgorithm)))

/*** Local functions definitions ***/
#ifdef ICP_PARAM_CHECK
    STATIC CpaStatus LacSymPerform_BufferParamCheck(
        const CpaBufferList * const pSrcBuffer,
        const CpaBufferList * const pDstBuffer,
        const lac_session_desc_t * const pSessionDesc,
        const CpaCySymOpData * const pOpData);
#endif

/**
 *****************************************************************************
 * @ingroup LacSym
 *      Generic bufferList callback function.
 * @description
 *      This function is used when the API is called in synchronous mode.
 *      It's assumed the callbackTag holds a lac_sync_op_data_t type
 *      and when the callback is received, this callback shall set the
 *      status and opResult element of that cookie structure and
 *      kick the sid.
 *      This function may be used directly as a callback function.
 *
 * @param[in]  callbackTag       Callback Tag
 * @param[in]  status            Status of callback
 * @param[in]  operationType     Operation Type
 * @param[in]  pOpData           Pointer to the Op Data
 * @param[out] pDstBuffer        Pointer to destination buffer list
 * @param[out] opResult          Boolean to indicate the result of the operation
 *
 * @return void
 *
 *****************************************************************************/
void LacSync_GenBufListVerifyCb(void *pCallbackTag,CpaStatus status,
                CpaCySymOp operationType, void *pOpData,
                CpaBufferList *pDstBuffer, CpaBoolean opResult)
{
    LacSync_GenVerifyWakeupSyncCaller(pCallbackTag,
                status, opResult);
}

/*
*******************************************************************************
* Define static function definitions
*******************************************************************************
*/
#ifdef ICP_PARAM_CHECK
/**
 * @ingroup LacSym
 * Function which perform parameter checks on session setup data
 *
 * @param[in] pSessionSetupData     Pointer to session setup data
 *
 * @retval CPA_STATUS_SUCCESS        The operation succeeded
 * @retval CPA_STATUS_INVALID_PARAM  An invalid parameter value was found
 */
STATIC CpaStatus
LacSymSession_ParamCheck(const CpaCySymSessionSetupData *pSessionSetupData)
{
    /* initialize convenient pointers to cipher and hash contexts */
    const CpaCySymCipherSetupData * const pCipherSetupData =
        &pSessionSetupData->cipherSetupData;
    const CpaCySymHashSetupData * const pHashSetupData =
        &pSessionSetupData->hashSetupData;

    /* ensure CCM, GCM, Kasumi and Snow3G cipher and hash algorithms are
     * selected together for Algorithm Chaining */
    if (CPA_CY_SYM_OP_ALGORITHM_CHAINING == pSessionSetupData->symOperation)
    {
        /* ensure both hash and cipher algorithms are CCM */
        if (((CPA_CY_SYM_CIPHER_AES_CCM == pCipherSetupData->cipherAlgorithm) &&
                (CPA_CY_SYM_HASH_AES_CCM != pHashSetupData->hashAlgorithm)) ||
                ((CPA_CY_SYM_HASH_AES_CCM == pHashSetupData->hashAlgorithm) &&
              (CPA_CY_SYM_CIPHER_AES_CCM != pCipherSetupData->cipherAlgorithm)))
        {
            LAC_INVALID_PARAM_LOG(
                "Invalid combination of Cipher/Hash Algorithms for CCM");
            return CPA_STATUS_INVALID_PARAM;
        }

        /* ensure both hash and cipher algorithms are GCM/GMAC */
        if ((CPA_CY_SYM_CIPHER_AES_GCM == pCipherSetupData->cipherAlgorithm &&
             (CPA_CY_SYM_HASH_AES_GCM != pHashSetupData->hashAlgorithm &&
              CPA_CY_SYM_HASH_AES_GMAC != pHashSetupData->hashAlgorithm)) ||
            ((CPA_CY_SYM_HASH_AES_GCM == pHashSetupData->hashAlgorithm ||
              CPA_CY_SYM_HASH_AES_GMAC == pHashSetupData->hashAlgorithm) &&
              CPA_CY_SYM_CIPHER_AES_GCM != pCipherSetupData->cipherAlgorithm))
        {
            LAC_INVALID_PARAM_LOG(
                "Invalid combination of Cipher/Hash Algorithms for GCM");
            return CPA_STATUS_INVALID_PARAM;
        }

        /* ensure both hash and cipher algorithms are Kasumi */
        if (
         ((CPA_CY_SYM_CIPHER_KASUMI_F8 == pCipherSetupData->cipherAlgorithm) &&
            (CPA_CY_SYM_HASH_KASUMI_F9 != pHashSetupData->hashAlgorithm)) ||
           ((CPA_CY_SYM_HASH_KASUMI_F9 == pHashSetupData->hashAlgorithm) &&
            (CPA_CY_SYM_CIPHER_KASUMI_F8 != pCipherSetupData->cipherAlgorithm)))
        {
            LAC_INVALID_PARAM_LOG(
                "Invalid combination of Cipher/Hash Algorithms for Kasumi");
            return CPA_STATUS_INVALID_PARAM;
        }

        /* ensure both hash and cipher algorithms are Snow3G */
        if (
       ((CPA_CY_SYM_CIPHER_SNOW3G_UEA2 == pCipherSetupData->cipherAlgorithm) &&
          (CPA_CY_SYM_HASH_SNOW3G_UIA2 != pHashSetupData->hashAlgorithm)) ||
         ((CPA_CY_SYM_HASH_SNOW3G_UIA2 == pHashSetupData->hashAlgorithm) &&
          (CPA_CY_SYM_CIPHER_SNOW3G_UEA2 != pCipherSetupData->cipherAlgorithm)))
        {
            LAC_INVALID_PARAM_LOG(
                "Invalid combination of Cipher/Hash Algorithms for Snow3G");
            return CPA_STATUS_INVALID_PARAM;
        }

    }
    /* not Algorithm Chaining so prevent CCM/GCM being selected */
    else if (CPA_CY_SYM_OP_CIPHER == pSessionSetupData->symOperation)
    {
        /* ensure cipher algorithm is not CCM or GCM */
        if ((CPA_CY_SYM_CIPHER_AES_CCM == pCipherSetupData->cipherAlgorithm) ||
               (CPA_CY_SYM_CIPHER_AES_GCM == pCipherSetupData->cipherAlgorithm))
        {
            LAC_INVALID_PARAM_LOG(
              "Invalid Cipher Algorithm for non-Algorithm Chaining operation");
            return CPA_STATUS_INVALID_PARAM;
        }
    }
    else if (CPA_CY_SYM_OP_HASH == pSessionSetupData->symOperation)
    {
        /* For hash, check if hash algorithm is correct*/
        if( (pHashSetupData->hashAlgorithm < CPA_CY_HASH_ALG_START) ||
                (pHashSetupData->hashAlgorithm > CPA_CY_HASH_ALG_END) )
        {
            LAC_INVALID_PARAM_LOG("hashAlgorithm");
            return CPA_STATUS_INVALID_PARAM;
        }

        /* ensure hash algorithm is not CCM or GCM/GMAC */
        if ((CPA_CY_SYM_HASH_AES_CCM == pHashSetupData->hashAlgorithm) ||
            (CPA_CY_SYM_HASH_AES_GCM == pHashSetupData->hashAlgorithm) ||
            (CPA_CY_SYM_HASH_AES_GMAC == pHashSetupData->hashAlgorithm))
        {
            LAC_INVALID_PARAM_LOG(
                "Invalid Hash Algorithm for non-Algorithm Chaining operation");
            return CPA_STATUS_INVALID_PARAM;
        }
    }
    /* Unsupported operation. Return error */
    else
    {
        LAC_INVALID_PARAM_LOG("symOperation");
        return CPA_STATUS_INVALID_PARAM;
    }

    /* ensure that cipher direction param is
     * valid for cipher and algchain ops */
    if(CPA_CY_SYM_OP_HASH != pSessionSetupData->symOperation)
    {
        if((pCipherSetupData->cipherDirection !=
                                CPA_CY_SYM_CIPHER_DIRECTION_ENCRYPT) &&
            (pCipherSetupData->cipherDirection !=
                                CPA_CY_SYM_CIPHER_DIRECTION_DECRYPT))
        {
            LAC_INVALID_PARAM_LOG("Invalid Cipher Direction");
            return CPA_STATUS_INVALID_PARAM;

        }
    }

    return CPA_STATUS_SUCCESS;
}
#endif /*ICP_PARAM_CHECK*/

#ifdef ICP_PARAM_CHECK

/**
 * @ingroup LacSym
 * Function which perform parameter checks on data buffers for symmetric
 * crypto operations
 *
 * @param[in] pSrcBuffer          Pointer to source buffer list
 * @param[in] pDstBuffer          Pointer to destination buffer list
 * @param[in] pSessionDesc        Pointer to session descriptor
 * @param[in] pOpData             Pointer to CryptoSymOpData.
 *
 * @retval CPA_STATUS_SUCCESS        The operation succeeded
 * @retval CPA_STATUS_INVALID_PARAM  An invalid parameter value was found
 */

STATIC CpaStatus LacSymPerform_BufferParamCheck(
    const CpaBufferList * const pSrcBuffer,
    const CpaBufferList * const pDstBuffer,
    const lac_session_desc_t * const pSessionDesc,
    const CpaCySymOpData * const pOpData)
{
    Cpa64U srcBufferLen = 0, dstBufferLen = 0;

    /* verify packet type is in correct range */
    switch (pOpData->packetType)
    {
        case CPA_CY_SYM_PACKET_TYPE_FULL:
        case CPA_CY_SYM_PACKET_TYPE_PARTIAL:
        case CPA_CY_SYM_PACKET_TYPE_LAST_PARTIAL:
            break;
        default:
        {
            LAC_INVALID_PARAM_LOG("packetType");
            return CPA_STATUS_INVALID_PARAM;
        }
    }

    if (!((CPA_CY_SYM_OP_CIPHER != pSessionDesc->symOperation &&
                   CPA_CY_SYM_HASH_MODE_PLAIN == pSessionDesc->hashMode) &&
                   (0 == pOpData->messageLenToHashInBytes)))
    {
        if (CPA_STATUS_SUCCESS !=
                LacBuffDesc_BufferListVerify(pSrcBuffer, &srcBufferLen,
                    LAC_NO_ALIGNMENT_SHIFT))
        {
            LAC_INVALID_PARAM_LOG("Source buffer invalid");
            return CPA_STATUS_INVALID_PARAM;
        }
    }
    else
    {
       /* check MetaData !NULL */
        if (NULL == pSrcBuffer->pPrivateMetaData)
        {
            LAC_INVALID_PARAM_LOG("Source buffer MetaData cannot be NULL");
            return CPA_STATUS_INVALID_PARAM;
        }
    }

    /* out of place checks */
    if (pSrcBuffer != pDstBuffer)
    {
        /* exception for this check is zero length hash requests to allow */
        /* for srcBufflen=DstBufferLen=0 */
        if (!((CPA_CY_SYM_OP_CIPHER != pSessionDesc->symOperation &&
                    CPA_CY_SYM_HASH_MODE_PLAIN == pSessionDesc->hashMode) &&
                    (0 == pOpData->messageLenToHashInBytes)))
        {
            /* Verify buffer(s) for dest packet & return packet length */
            if (CPA_STATUS_SUCCESS !=  LacBuffDesc_BufferListVerify(pDstBuffer,
                                       &dstBufferLen, LAC_NO_ALIGNMENT_SHIFT))
            {
                LAC_INVALID_PARAM_LOG(
                    "Destination buffer invalid");
                return CPA_STATUS_INVALID_PARAM;
            }
        }
        else
        {
            /* check MetaData !NULL */
            if (NULL == pDstBuffer->pPrivateMetaData)
            {
                LAC_INVALID_PARAM_LOG("Dest buffer MetaData cannot be NULL");
                return CPA_STATUS_INVALID_PARAM;
            }
        }
        /* Check that src Buffer and dst Buffer Lengths are equal */
        if (srcBufferLen != dstBufferLen)
        {
            LAC_INVALID_PARAM_LOG(
                "Source and Dest buffer lengths need to be equal ");
            return CPA_STATUS_INVALID_PARAM;
        }
    }


    /* check for partial packet suport for the session operation */
    if (CPA_CY_SYM_PACKET_TYPE_FULL != pOpData->packetType)
    {
        if (!(IS_PARTIAL_ON_SYM_OP_SUPPORTED(pSessionDesc->symOperation,
                                             pSessionDesc->cipherAlgorithm,
                                             pSessionDesc->hashAlgorithm,
                                             pSessionDesc->hashMode)))

        {
            /* return out here to simplify cleanup */
            LAC_INVALID_PARAM_LOG(
                "Partial packets not supported for operation");
            return CPA_STATUS_INVALID_PARAM;
        }
        else
        {
            /* This function checks to see if the partial packet sequence
             * is correct */
            if (CPA_STATUS_SUCCESS != LacSym_PartialPacketStateCheck(
                               pOpData->packetType, pSessionDesc->partialState))
            {
                LAC_INVALID_PARAM_LOG("Partial packet Type");
                return CPA_STATUS_INVALID_PARAM;
            }
        }
    }

    return CPA_STATUS_SUCCESS;
}
#endif

/** @ingroup LacSym */
CpaStatus
cpaCySymInitSession (const CpaInstanceHandle instanceHandle_in,
                     const CpaCySymCbFunc pSymCb,
                     const CpaCySymSessionSetupData *pSessionSetupData,
                     CpaCySymSessionCtx pSessionCtx)
{
    CpaStatus status = CPA_STATUS_SUCCESS;
    CpaInstanceHandle instanceHandle = NULL;
    sal_service_t * pService = NULL;

    if (CPA_INSTANCE_HANDLE_SINGLE == instanceHandle_in)
    {
         instanceHandle = Lac_GetFirstHandle();
    }
    else
    {
         instanceHandle = instanceHandle_in;
    }

#ifdef ICP_PARAM_CHECK
    LAC_CHECK_INSTANCE_HANDLE(instanceHandle);
    SAL_CHECK_ADDR_TRANS_SETUP(instanceHandle);
    SAL_CHECK_INSTANCE_TYPE(instanceHandle, SAL_SERVICE_TYPE_CRYPTO);
#endif /*ICP_PARAM_CHECK*/

    pService = (sal_service_t *) instanceHandle;

    /* check crypto service is running otherwise return an error */
    SAL_RUNNING_CHECK(pService);

    status = LacSym_InitSession(instanceHandle, pSymCb, pSessionSetupData,
            CPA_FALSE, /* isDPSession */
            pSessionCtx);


    if (CPA_STATUS_SUCCESS == status)
    {
        /* Increment the stats for a session registered successfully */
        LAC_SYM_STAT_INC(numSessionsInitialized, instanceHandle);

    }
    else /* if there was an error */
    {
        LAC_SYM_STAT_INC(numSessionErrors, instanceHandle);
    }
    return status;
}

CpaStatus
LacSym_InitSession(const CpaInstanceHandle instanceHandle,
                     const CpaCySymCbFunc pSymCb,
                     const CpaCySymSessionSetupData *pSessionSetupData,
                     const CpaBoolean isDPSession,
                     CpaCySymSessionCtx pSessionCtx)
{
    CpaStatus status = CPA_STATUS_SUCCESS;
    lac_session_desc_t *pSessionDesc = NULL;
    CpaPhysicalAddr physAddress = 0;
    CpaPhysicalAddr physAddressAligned = 0;
    sal_service_t * pService = NULL;

    /* Instance param checking done by calling function */

#ifdef ICP_PARAM_CHECK
    LAC_CHECK_NULL_PARAM(pSessionSetupData);
    LAC_CHECK_NULL_PARAM(pSessionCtx);
    status = LacSymSession_ParamCheck(pSessionSetupData);
    LAC_CHECK_STATUS(status);
#endif /*ICP_PARAM_CHECK*/

    pService = (sal_service_t *) instanceHandle;

    /* Re-align the session structure to 64 byte alignment */
    physAddress = LAC_OS_VIRT_TO_PHYS_EXTERNAL(
                      (*pService), (Cpa8U *)pSessionCtx + sizeof(void *));

    if (0 == physAddress)
    {
        LAC_LOG_ERROR("Unable to get the physical address of the session\n");
        return CPA_STATUS_FAIL;
    }

    physAddressAligned = LAC_ALIGN_POW2_ROUNDUP(
                             physAddress, LAC_64BYTE_ALIGNMENT);

    pSessionDesc = (lac_session_desc_t *)
                    /* Move the session pointer by the physical offset
                    between aligned and unaligned memory */
                    ((Cpa8U *) pSessionCtx + sizeof(void *)
                    + (physAddressAligned-physAddress));

    /* save the aligned pointer in the first bytes (size of unsigned long)
     * of the session memory */
    *((LAC_ARCH_UINT *)pSessionCtx) = (LAC_ARCH_UINT)pSessionDesc;

    /* Setup content descriptor info structure
     * assumption that content descriptor is the first field in
     * in the session descriptor */
    pSessionDesc->contentDescInfo.pData = (Cpa8U *)pSessionDesc;
    pSessionDesc->contentDescInfo.dataPhys = physAddressAligned;

    /* Set the Common Session Information */
    pSessionDesc->symOperation = pSessionSetupData->symOperation;

    if (CPA_FALSE == isDPSession)
    {
        /* For asynchronous - use the user supplied callback
         * for synchronous - use the internal synchronous callback */
        pSessionDesc->pSymCb =
                ((void*) NULL != (void *) pSymCb) ?
                        pSymCb : LacSync_GenBufListVerifyCb;
    }

    pSessionDesc->isDPSession = isDPSession;

    /* set the session priority for QAT AL*/
    if (CPA_CY_PRIORITY_HIGH == pSessionSetupData->sessionPriority)
    {
        pSessionDesc->qatSessionPriority = QAT_COMMS_PRIORITY_HIGH;
    }
    else if (CPA_CY_PRIORITY_NORMAL == pSessionSetupData->sessionPriority)
    {
        pSessionDesc->qatSessionPriority = QAT_COMMS_PRIORITY_NORMAL;
    }
    else
    {
        LAC_INVALID_PARAM_LOG("sessionPriority");
        status = CPA_STATUS_INVALID_PARAM;
    }

    if (CPA_STATUS_SUCCESS == status)
    {
        /* initialize convenient pointers to cipher setup data */
        const CpaCySymCipherSetupData * const pCipherSetupData =
            (const CpaCySymCipherSetupData * const)
            &pSessionSetupData->cipherSetupData;

        if ((CPA_CY_SYM_CIPHER_AES_CCM == pCipherSetupData->cipherAlgorithm)||
            (CPA_CY_SYM_CIPHER_AES_GCM == pCipherSetupData->cipherAlgorithm))
        {
            pSessionDesc->isAuthEncryptOp = CPA_TRUE;
        }
        else
        {
            pSessionDesc->isAuthEncryptOp = CPA_FALSE;
        }

        /* Session set up via API call (not internal one) */
        /* Services such as DRBG call the crypto api as part of their service
         * hence the need to for the flag, it is needed to distinguish between
         * an internal and external session.
         */
        pSessionDesc->internalSession = CPA_FALSE;

        pSessionDesc->digestVerify = pSessionSetupData->verifyDigest;

        status = LacAlgChain_SessionInit(instanceHandle, pSessionSetupData,
                                            pSessionDesc);
    }
    return status;
}

/** @ingroup LacSym */
CpaStatus
cpaCySymRemoveSession (const CpaInstanceHandle instanceHandle_in,
                       CpaCySymSessionCtx pSessionCtx)
{
    lac_session_desc_t *pSessionDesc = NULL;
    CpaStatus status = CPA_STATUS_SUCCESS;
    CpaInstanceHandle instanceHandle = NULL;
    Cpa64U numPendingRequests = 0;

    if (CPA_INSTANCE_HANDLE_SINGLE == instanceHandle_in)
    {
         instanceHandle = Lac_GetFirstHandle();
    }
    else
    {
         instanceHandle = instanceHandle_in;
    }

#ifdef ICP_PARAM_CHECK
    LAC_CHECK_INSTANCE_HANDLE(instanceHandle);
    SAL_CHECK_INSTANCE_TYPE(instanceHandle, SAL_SERVICE_TYPE_CRYPTO);
    LAC_CHECK_NULL_PARAM(pSessionCtx);
#endif /*ICP_PARAM_CHECK*/

    /* check crypto service is running otherwise return an error */
    SAL_RUNNING_CHECK(instanceHandle);
    pSessionDesc = LAC_SYM_SESSION_DESC_FROM_CTX_GET(pSessionCtx);

#ifdef ICP_PARAM_CHECK
    LAC_CHECK_NULL_PARAM(pSessionDesc);
#endif /*ICP_PARAM_CHECK*/

    if (CPA_TRUE == pSessionDesc->isDPSession)
    {
        /* Need to update tail if messages queue on tx hi ring for
         data plane api */
        icp_comms_trans_handle trans_handle =
           ((sal_crypto_service_t*)instanceHandle)->trans_handle_sym_tx_hi;

        if (CPA_TRUE == icp_adf_queueDataToSend(trans_handle))
        {
            /* process the remaining messages in the ring */
            LAC_LOG("Submitting enqueued requests");
            icp_adf_updateQueueTail(trans_handle);
            return CPA_STATUS_RETRY;
        }
        numPendingRequests = pSessionDesc->pendingDpCbCount;
    }
    else
    {
       numPendingRequests = osalAtomicGet(&(pSessionDesc->pendingCbCount));
    }

    /* If there are pending requests */
    if (0 != numPendingRequests)
    {
        LAC_LOG1("There are %d requests pending", numPendingRequests);
        status = CPA_STATUS_RETRY;
    }

    if (CPA_STATUS_SUCCESS == status)
    {
        LAC_SPINLOCK_DESTROY(&pSessionDesc->requestQueueLock);
        if(CPA_FALSE == pSessionDesc->isDPSession)
        {
            LAC_SYM_STAT_INC(numSessionsRemoved, instanceHandle);
        }
    }
    else if (CPA_FALSE == pSessionDesc->isDPSession)
    {
        LAC_SYM_STAT_INC(numSessionErrors, instanceHandle);
    }
    return status;
}

/** @ingroup LacSym */
STATIC CpaStatus LacSym_Perform (const CpaInstanceHandle instanceHandle,
                void *callbackTag,
                const CpaCySymOpData *pOpData,
                const CpaBufferList *pSrcBuffer,
                CpaBufferList *pDstBuffer,
                CpaBoolean *pVerifyResult,
                CpaBoolean isAsyncMode)
{
    lac_session_desc_t *pSessionDesc = NULL;
    CpaStatus status = CPA_STATUS_SUCCESS;
    CpaFlatBuffer     *pCurrFlatBuffer = NULL;
    Cpa32U             numBuffers = 0;
    Cpa32U         tempHashOffset = 0;
    Cpa32U         tempMessageLen = 0;

#ifdef ICP_PARAM_CHECK
    LAC_CHECK_INSTANCE_HANDLE(instanceHandle);
    SAL_CHECK_ADDR_TRANS_SETUP(instanceHandle);
    SAL_CHECK_INSTANCE_TYPE(instanceHandle, SAL_SERVICE_TYPE_CRYPTO);
#endif
    /* check crypto service is running otherwise return an error */
    SAL_RUNNING_CHECK(instanceHandle);
#ifdef ICP_PARAM_CHECK
    LAC_CHECK_NULL_PARAM(pOpData);
    LAC_CHECK_NULL_PARAM(pOpData->sessionCtx);
    LAC_CHECK_NULL_PARAM(pSrcBuffer);
    LAC_CHECK_NULL_PARAM(pDstBuffer);
#endif /*ICP_PARAM_CHECK*/

    pSessionDesc = LAC_SYM_SESSION_DESC_FROM_CTX_GET(pOpData->sessionCtx);
#ifdef ICP_PARAM_CHECK
    LAC_CHECK_NULL_PARAM(pSessionDesc);
#endif /*ICP_PARAM_CHECK*/

    /* If synchronous Operation - Callback function stored in the session
     * descriptor so a flag is set in the perform to indicate that
     * the perform is being re-called for the synchronous operation */
    if ((LacSync_GenBufListVerifyCb == pSessionDesc->pSymCb)
            && isAsyncMode == CPA_TRUE)
    {
        CpaBoolean opResult = CPA_FALSE;
        lac_sync_op_data_t* pSyncCallbackData= NULL;

        status = LacSync_CreateSyncCookie(&pSyncCallbackData);

        if (CPA_STATUS_SUCCESS == status)
        {
            status = LacSym_Perform(instanceHandle,
                        pSyncCallbackData,
                        pOpData,
                        pSrcBuffer,
                        pDstBuffer,
                        pVerifyResult,
                        CPA_FALSE);
        }
        else
        {
            /* Failure allocating sync cookie */
            LAC_SYM_STAT_INC(numSymOpRequestErrors, instanceHandle);
        }

        if (CPA_STATUS_SUCCESS == status)
        {
            CpaStatus syncStatus = CPA_STATUS_SUCCESS;
            syncStatus = LacSync_WaitForCallback(pSyncCallbackData,
                            LAC_SYM_SYNC_CALLBACK_TIMEOUT,
                            &status,
                            &opResult);
            /* If callback doesn't come back */
            if (CPA_STATUS_SUCCESS != syncStatus)
            {
                LAC_SYM_STAT_INC(numSymOpCompletedErrors, instanceHandle);
                LAC_LOG_ERROR("Callback timed out");
                status = syncStatus;
            }
        }

        if (CPA_STATUS_SUCCESS == status)
        {
            if (NULL != pVerifyResult)
            {
                *pVerifyResult = opResult;
            }
        }

        LacSync_DestroySyncCookie(&pSyncCallbackData);
        return status;
    }

#ifdef ICP_PARAM_CHECK
    status = LacSymPerform_BufferParamCheck(
                (const CpaBufferList * const) pSrcBuffer,
                pDstBuffer,
                pSessionDesc,
                pOpData);
    LAC_CHECK_STATUS(status);

    if ((!pSessionDesc->digestIsAppended) &&
       (CPA_CY_SYM_OP_ALGORITHM_CHAINING == pSessionDesc->symOperation))
    {
        int i;
        tempHashOffset = pOpData->hashStartSrcOffsetInBytes;
        tempMessageLen = pOpData->messageLenToHashInBytes;
        pCurrFlatBuffer = pDstBuffer->pBuffers;
        numBuffers = pDstBuffer->numBuffers;

        /* Check that pDigestResult is not NULL */
        LAC_CHECK_NULL_PARAM(pOpData->pDigestResult);

        /* Check that pDigestResult does not point to end of hash region */
        for (i=0 ; i<numBuffers ; i++ )
        {
            if (tempHashOffset > pCurrFlatBuffer->dataLenInBytes)
            {
               tempHashOffset -= pCurrFlatBuffer->dataLenInBytes;
               pCurrFlatBuffer++;
            }
            else
            {
                /* Hashoffset is in current buffer so */
                break;
            }
        }
        /* check for end of hash region in remaining buffers */
        do{
            if(tempHashOffset + tempMessageLen <
               pCurrFlatBuffer->dataLenInBytes)
            {
                /*check if pDigest points to end of hashLen */
                if((pOpData->pDigestResult ==
                    (Cpa8U*)pCurrFlatBuffer +
                             tempHashOffset +
                             tempMessageLen))
                {
                    LAC_INVALID_PARAM_LOG("To append the Digest Result, "
                                  "digestIsAppended flag must be set ");
                    return CPA_STATUS_INVALID_PARAM;
                }
            }
            i++;
            tempHashOffset=0;
            tempMessageLen -= pCurrFlatBuffer->dataLenInBytes;
            pCurrFlatBuffer++;
        } while (i<numBuffers);
    }

#endif /*ICP_PARAM_CHECK*/
    status = LacAlgChain_Perform(instanceHandle,
                     pSessionDesc,
                     callbackTag,
                     pOpData,
                     pSrcBuffer,
                     pDstBuffer,
                     pVerifyResult);

    if (CPA_STATUS_SUCCESS == status)
    {
        /* check for partial packet suport for the session operation */
        if (CPA_CY_SYM_PACKET_TYPE_FULL != pOpData->packetType)
        {
            LacSym_PartialPacketStateUpdate(pOpData->packetType,
                                            &pSessionDesc->partialState);
        }
        /* increment #requests stat */
        LAC_SYM_STAT_INC(numSymOpRequests, instanceHandle);
    }
    /* Retry also results in the errors stat been incremented */
    else
    {
        /* increment #errors stat */
        LAC_SYM_STAT_INC(numSymOpRequestErrors, instanceHandle);
    }
    return status;
}

/** @ingroup LacSym */
CpaStatus
cpaCySymPerformOp (const CpaInstanceHandle instanceHandle_in,
                   void *callbackTag,
                   const CpaCySymOpData *pOpData,
                   const CpaBufferList *pSrcBuffer,
                   CpaBufferList *pDstBuffer,
                   CpaBoolean *pVerifyResult)
{
   CpaInstanceHandle instanceHandle = NULL;

    if (CPA_INSTANCE_HANDLE_SINGLE == instanceHandle_in) {
         instanceHandle = Lac_GetFirstHandle();
    }
    else
    {
         instanceHandle = instanceHandle_in;
    }
    return LacSym_Perform (instanceHandle, callbackTag, pOpData,
                           pSrcBuffer, pDstBuffer, pVerifyResult, CPA_TRUE);
}


/** @ingroup LacSym */
CpaStatus
cpaCySymQueryStats(const CpaInstanceHandle instanceHandle_in,
                   struct _CpaCySymStats *pSymStats)
{

   CpaInstanceHandle instanceHandle = NULL;

    if (CPA_INSTANCE_HANDLE_SINGLE == instanceHandle_in) {
         instanceHandle = Lac_GetFirstHandle();
    }
    else
    {
         instanceHandle = instanceHandle_in;
    }

#ifdef ICP_PARAM_CHECK
    LAC_CHECK_INSTANCE_HANDLE(instanceHandle);
    SAL_CHECK_INSTANCE_TYPE(instanceHandle, SAL_SERVICE_TYPE_CRYPTO);
    LAC_CHECK_NULL_PARAM(pSymStats);
#endif /*ICP_PARAM_CHECK*/

    /* check if crypto service is running
     * otherwise return an error */
    SAL_RUNNING_CHECK(instanceHandle);

    /* copy the fields from the internal structure into the api defined
     * structure */
    LacSym_Stats32CopyGet(instanceHandle, pSymStats);
    return CPA_STATUS_SUCCESS;
}

/** @ingroup LacSym */
CpaStatus
cpaCySymQueryStats64(const CpaInstanceHandle instanceHandle_in,
                     CpaCySymStats64 *pSymStats)
{

   CpaInstanceHandle instanceHandle = NULL;

    if (CPA_INSTANCE_HANDLE_SINGLE == instanceHandle_in) {
         instanceHandle = Lac_GetFirstHandle();
    }
    else
    {
         instanceHandle = instanceHandle_in;
    }

#ifdef ICP_PARAM_CHECK
    LAC_CHECK_INSTANCE_HANDLE(instanceHandle);
    SAL_CHECK_INSTANCE_TYPE(instanceHandle, SAL_SERVICE_TYPE_CRYPTO);
    LAC_CHECK_NULL_PARAM(pSymStats);
#endif /*ICP_PARAM_CHECK*/

    /* check if crypto service is running
     * otherwise return an error */
    SAL_RUNNING_CHECK(instanceHandle);

    /* copy the fields from the internal structure into the api defined
     * structure */
    LacSym_Stats64CopyGet(instanceHandle, pSymStats);

    return CPA_STATUS_SUCCESS;
}

/** @ingroup LacSym */
CpaStatus
cpaCySymSessionCtxGetSize(const CpaInstanceHandle instanceHandle_in,
                          const CpaCySymSessionSetupData *pSessionSetupData,
                          Cpa32U *pSessionCtxSizeInBytes)
{
    CpaInstanceHandle instanceHandle = NULL;

    if (CPA_INSTANCE_HANDLE_SINGLE == instanceHandle_in) {
         instanceHandle = Lac_GetFirstHandle();
    }
    else
    {
         instanceHandle = instanceHandle_in;
    }


#ifdef ICP_PARAM_CHECK
    LAC_CHECK_INSTANCE_HANDLE(instanceHandle);
    SAL_CHECK_INSTANCE_TYPE(instanceHandle, SAL_SERVICE_TYPE_CRYPTO);
    LAC_CHECK_NULL_PARAM(pSessionSetupData);
    LAC_CHECK_NULL_PARAM(pSessionCtxSizeInBytes);
#endif /*ICP_PARAM_CHECK*/

    /* check crypto service is running otherwise return an error */
    SAL_RUNNING_CHECK(instanceHandle);
    *pSessionCtxSizeInBytes = LAC_SYM_SESSION_SIZE;
    return CPA_STATUS_SUCCESS;
}

/**
 ******************************************************************************
 * @ingroup LacSym
 *****************************************************************************/
CpaStatus
cpaCyBufferListGetMetaSize(const CpaInstanceHandle instanceHandle_in,
        Cpa32U numBuffers,
        Cpa32U *pSizeInBytes)
{

#ifdef ICP_PARAM_CHECK
    CpaInstanceHandle instanceHandle = NULL;

    if(CPA_INSTANCE_HANDLE_SINGLE == instanceHandle_in)
    {
         instanceHandle = Lac_GetFirstHandle();
    }
    else
    {
         instanceHandle = instanceHandle_in;
    }
    LAC_CHECK_INSTANCE_HANDLE(instanceHandle);
    SAL_CHECK_INSTANCE_TYPE(instanceHandle, SAL_SERVICE_TYPE_CRYPTO);
    LAC_CHECK_NULL_PARAM(pSizeInBytes);

#endif
    /* In the case of zero buffers we still need to allocate one
     * descriptor to pass to the firmware */
    if(0 == numBuffers)
    {
        numBuffers = 1;
    }

    /* Note: icp_buffer_list_desc_t is 8 bytes in size and
     * icp_flat_buffer_desc_t is 16 bytes in size. Therefore if
     * icp_buffer_list_desc_t is aligned
     * so will each icp_flat_buffer_desc_t structure */

    *pSizeInBytes =  sizeof(icp_buffer_list_desc_t) +
                     (sizeof(icp_flat_buffer_desc_t) * numBuffers) +
                     ICP_DESCRIPTOR_ALIGNMENT_BYTES;

    return CPA_STATUS_SUCCESS;
}
