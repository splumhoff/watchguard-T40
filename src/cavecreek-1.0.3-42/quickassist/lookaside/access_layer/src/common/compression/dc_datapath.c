/****************************************************************************
 *
 *   BSD LICENSE
 * 
 *   Copyright(c) 2007-2016 Intel Corporation. All rights reserved.
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
 *  version: QAT1.7.Upstream.L.1.0.3-42
 *
 ***************************************************************************/

/**
 *****************************************************************************
 * @file dc_datapath.c
 *
 * @defgroup Dc_DataCompression DC Data Compression
 *
 * @ingroup Dc_DataCompression
 *
 * @description
 *      Implementation of the Data Compression datapath operations.
 *
 *****************************************************************************/

/*
*******************************************************************************
* Include public/global header files
*******************************************************************************
*/
#include "cpa.h"
#include "cpa_dc.h"
#include "cpa_dc_bp.h"
#include "cpa_dc_dp.h"

/*
*******************************************************************************
* Include private header files
*******************************************************************************
*/
#include "dc_session.h"
#include "dc_datapath.h"
#include "sal_statistics.h"
#include "lac_common.h"
#include "lac_mem.h"
#include "lac_mem_pools.h"
#include "lac_log.h"
#include "sal_types_compression.h"
#include "dc_stats.h"
#include "lac_buffer_desc.h"
#include "lac_sal.h"
#include "lac_sync.h"
#include "sal_service_state.h"
#include "sal_qat_cmn_msg.h"


STATIC void
processBatchAndPackResults (dc_compression_cookie_t *pCookie,
                            dc_session_desc_t *pSessionDesc)
{
    CpaStatus status = CPA_STATUS_SUCCESS;
    sal_compression_service_t* pService = NULL;
    void *callbackTag = NULL;
    Cpa32U numJobsInBatch = 0;
    Cpa32U i = 0;
    CpaDcRqResults *pResults = NULL;
    CpaDcBatchOpData *pBatchOpData = NULL;
    CpaDcCallbackFn pCbFunc = NULL;

    pService = (sal_compression_service_t*) (pCookie->dcInstance);
    numJobsInBatch = pCookie->numJobsInBatch;
    pResults = pCookie->pResults;
    pBatchOpData = pCookie->pBatchOpData;
    status = LacBuffDesc_BnpRespResultsRead(pBatchOpData,
                                            numJobsInBatch,
                                            pResults,
                                            pSessionDesc->checksumType,
                                            &(pService->generic_service_info));

    /* Increment statistics */
    COMPRESSION_STAT_INC(numCompCompleted, pService);

    for (i=0; i<numJobsInBatch; i++)
    {
        if(CPA_DC_OK != pResults[i].status)
        {
            COMPRESSION_STAT_INC(numCompCompletedErrors, pService);
        }
    }

    /* Retrieve Cookie properties */
    callbackTag = pCookie->callbackTag;
    pCbFunc = pCookie->pSessionDesc->pCompressionCb;

    /* Decrement number of pending callbacks for session */
    osalAtomicDec(&(pCookie->pSessionDesc->pendingStatelessCbCount));

    /* Free the memory pool */
    if (NULL != pCookie)
    {
        Lac_MemPoolEntryFree(pCookie);
        pCookie = NULL;
    }
    if(NULL != pCbFunc)
    {
        pCbFunc(callbackTag, status);
    }

    return;
}

void
dcCompression_ProcessCallback(void *pRespMsg)
{
    CpaStatus status = CPA_STATUS_SUCCESS;
    icp_qat_fw_comp_resp_t *pCompRespMsg = NULL;
    void *callbackTag = NULL;
    Cpa64U *pReqData = NULL;
    CpaDcDpOpData *pResponse = NULL;
    CpaDcRqResults *pResults = NULL;
    CpaDcCallbackFn pCbFunc = NULL;
    dc_session_desc_t *pSessionDesc = NULL;

    sal_compression_service_t* pService = NULL;
    dc_compression_cookie_t* pCookie = NULL;
    CpaBoolean cmpPass = CPA_TRUE, xlatPass = CPA_TRUE;
#ifndef ICP_DC_DYN_NOT_SUPPORTED
    Cpa8U cmpErr = ERR_CODE_NO_ERROR, xlatErr = ERR_CODE_NO_ERROR;
#else
    Cpa8U cmpErr = ERR_CODE_NO_ERROR;
#endif
    dc_request_dir_t compDecomp = DC_COMPRESSION_REQUEST;
    Cpa8U opStatus = ICP_QAT_FW_COMN_STATUS_FLAG_OK;

    /* Cast response message to compression response message type */
    pCompRespMsg = (icp_qat_fw_comp_resp_t *)pRespMsg;
#ifdef ICP_PARAM_CHECK
    LAC_ASSERT_NOT_NULL(pCompRespMsg);
#endif

    /* Extract request data pointer from the opaque data */
    LAC_MEM_SHARED_READ_TO_PTR(pCompRespMsg->opaque_data, pReqData);
#ifdef ICP_PARAM_CHECK
    LAC_ASSERT_NOT_NULL(pReqData);
#endif

    /* Extract fields from the request data structure */
    pCookie = (dc_compression_cookie_t*) pReqData;
    pSessionDesc = DC_SESSION_DESC_FROM_CTX_GET(pCookie->pSessionHandle);

    if(CPA_TRUE == pSessionDesc->isDcDp)
    {
        pResponse = (CpaDcDpOpData *) pReqData;
        pResults = &(pResponse->results);

        if(CPA_DC_DIR_DECOMPRESS == pSessionDesc->sessDirection)
        {
            compDecomp = DC_DECOMPRESSION_REQUEST;
        }
    }
    else
    {
        if (CPA_TRUE == pCookie->isBnp)
        {
            processBatchAndPackResults (pCookie, pSessionDesc);
            /* Exit callback */
            return;
        }
        pSessionDesc = pCookie->pSessionDesc;
        pResults = pCookie->pResults;
        callbackTag = pCookie->callbackTag;
        pCbFunc = pCookie->pSessionDesc->pCompressionCb;
        compDecomp = pCookie->compDecomp;
    }

    pService = (sal_compression_service_t*) (pCookie->dcInstance);

    opStatus = pCompRespMsg->comn_resp.comn_status;

    /* Check compression response status */
    cmpPass = (CpaBoolean)(ICP_QAT_FW_COMN_STATUS_FLAG_OK ==
            ICP_QAT_FW_COMN_RESP_CMP_STAT_GET(opStatus));

    /* Get the cmp error code */
    cmpErr = pCompRespMsg->comn_resp.comn_error.s1.cmp_err_code;

    if(CPA_DC_INCOMPLETE_FILE_ERR == (Cpa8S)cmpErr)
    {
        cmpPass = CPA_TRUE;
        cmpErr = ERR_CODE_NO_ERROR;
    }

    /* log the slice hang and endpoint push/pull error inside the response */
    if (ERR_CODE_SSM_ERROR == (Cpa8S)cmpErr)
    {
        LAC_LOG_ERROR("The slice hang is detected on the compression slice");
    }
    else if (ERR_CODE_ENDPOINT_ERROR == (Cpa8S)cmpErr)
         {
             LAC_LOG_ERROR("The PCIe End Point Push/Pull or TI/RI Parity error detected.");
         }

    /* We return the compression error code for now. We would need to update
     * the API if we decide to return both error codes */
    pResults->status = (Cpa8S)cmpErr;

#ifndef ICP_DC_DYN_NOT_SUPPORTED
    /* Check the translator status */
    if((DC_COMPRESSION_REQUEST == compDecomp)
     &&(CPA_DC_HT_FULL_DYNAMIC == pSessionDesc->huffType))
    {
        /* Check translator response status */
        xlatPass = (CpaBoolean)(ICP_QAT_FW_COMN_STATUS_FLAG_OK ==
                ICP_QAT_FW_COMN_RESP_XLAT_STAT_GET(opStatus));

        /* Get the translator error code */
        xlatErr = pCompRespMsg->comn_resp.comn_error.s1.xlat_err_code;

        /* Return a fatal error or a potential error in the translator slice
         * if the compression slice did not return any error */
        if((CPA_DC_OK == pResults->status) ||
           (CPA_DC_FATALERR == (Cpa8S)xlatErr))
        {
            pResults->status = (Cpa8S)xlatErr;
        }
    }
#endif

    if(CPA_FALSE == pSessionDesc->isDcDp)
    {
        /* In case of any error for an end of packet request, we need to update
         * the request type for the following request */
        if((CPA_DC_STATEFUL == pSessionDesc->sessState) &&
          ((CPA_DC_FLUSH_FINAL == pCookie->flushFlag) ||
           (CPA_DC_FLUSH_FULL == pCookie->flushFlag)) &&
           ((CPA_TRUE != cmpPass)||(CPA_TRUE != xlatPass)))
        {
            pSessionDesc->requestType = DC_REQUEST_SUBSEQUENT;
        }
        else if((CPA_DC_STATELESS == pSessionDesc->sessState) &&
             ((CPA_DC_FLUSH_FINAL == pCookie->flushFlag)))
        {
            pSessionDesc->requestType = DC_REQUEST_FIRST;
        }
        else if((CPA_DC_STATELESS == pSessionDesc->sessState) &&
                ((CPA_DC_FLUSH_FULL == pCookie->flushFlag)))
        {
            pSessionDesc->requestType = DC_REQUEST_SUBSEQUENT;
        }
    }

    /* Stateful overflow is a valid use case. The previous
     * update on the request type is still required */
    if(CPA_DC_STATEFUL == pSessionDesc->sessState)
    {
        if(CPA_DC_OVERFLOW == (Cpa8S)cmpErr)
            cmpPass = CPA_TRUE;

#ifndef ICP_DC_DYN_NOT_SUPPORTED
        if(CPA_DC_OVERFLOW == (Cpa8S)xlatErr)
            xlatPass = CPA_TRUE;
#endif
    }

    if((CPA_TRUE == cmpPass) && (CPA_TRUE == xlatPass))
    {
        /* Extract the response from the firmware */
        pResults->consumed = pCompRespMsg->comp_resp_pars.input_byte_counter;
        pResults->produced = pCompRespMsg->comp_resp_pars.output_byte_counter;

        if(CPA_DC_STATEFUL == pSessionDesc->sessState)
        {
            pSessionDesc->cumulativeConsumedBytes += pResults->consumed;
        }
        else
        {
            /* In the stateless case all requests have both SOP and EOP set */
            pSessionDesc->cumulativeConsumedBytes = pResults->consumed;
        }

        if(CPA_DC_CRC32 == pSessionDesc->checksumType)
        {
            pResults->checksum = pCompRespMsg->comp_resp_pars.curr_crc32;
        }
        else if(CPA_DC_ADLER32 == pSessionDesc->checksumType)
        {
            pResults->checksum = pCompRespMsg->comp_resp_pars.curr_adler_32;
        }

        if(DC_DECOMPRESSION_REQUEST == compDecomp)
        {
            pResults->endOfLastBlock = (ICP_QAT_FW_COMN_STATUS_CMP_END_OF_LAST_BLK_FLAG_SET ==
                    ICP_QAT_FW_COMN_RESP_CMP_END_OF_LAST_BLK_FLAG_GET(opStatus));
        }

        /* Save the checksum for the next request */
        pSessionDesc->previousChecksum = pResults->checksum;

        if(CPA_TRUE == pSessionDesc->isDcDp)
        {
            pResponse->responseStatus = CPA_STATUS_SUCCESS;
        }
        else
        {
            if(DC_COMPRESSION_REQUEST == compDecomp)
            {
                COMPRESSION_STAT_INC(numCompCompleted, pService);
            }
            else
            {
                COMPRESSION_STAT_INC(numDecompCompleted, pService);
            }
        }
    }
    else
    {
#ifdef ICP_DC_RETURN_COUNTERS_ON_ERROR
        /* Extract the response from the firmware */
        pResults->consumed = pCompRespMsg->comp_resp_pars.input_byte_counter;
        pResults->produced = pCompRespMsg->comp_resp_pars.output_byte_counter;

        if(CPA_DC_STATEFUL == pSessionDesc->sessState)
        {
            pSessionDesc->cumulativeConsumedBytes += pResults->consumed;
        }
        else
        {
            /* In the stateless case all requests have both SOP and EOP set */
            pSessionDesc->cumulativeConsumedBytes = pResults->consumed;
        }
#else
        pResults->consumed = 0;
        pResults->produced = 0;
#endif
        if(CPA_DC_OVERFLOW == pResults->status &&
                CPA_DC_STATELESS == pSessionDesc->sessState)
        {
            LAC_LOG_ERROR("Unrecoverable error: stateless overflow. You may "
                "need to increase the size of your destination buffer");
        }

        if(CPA_TRUE == pSessionDesc->isDcDp)
        {
            pResponse->responseStatus = CPA_STATUS_FAIL;
        }
        else
        {
            if(CPA_DC_OK != pResults->status &&
                CPA_DC_INCOMPLETE_FILE_ERR != pResults->status)
            {
                status = CPA_STATUS_FAIL;
            }

            if(DC_COMPRESSION_REQUEST == compDecomp)
            {
                COMPRESSION_STAT_INC(numCompCompletedErrors, pService);
            }
            else
            {
                COMPRESSION_STAT_INC(numDecompCompletedErrors, pService);
            }
        }
    }

    if(CPA_TRUE == pSessionDesc->isDcDp)
    {
        /* Decrement number of stateless pending callbacks for session */
        pSessionDesc->pendingDpStatelessCbCount--;
        (pService->pDcDpCb)(pResponse);
    }
    else
    {
        /* Decrement number of pending callbacks for session */
        if(CPA_DC_STATELESS == pSessionDesc->sessState)
        {
            osalAtomicDec(&(pCookie->pSessionDesc->pendingStatelessCbCount));
        }
        else if (0 != osalAtomicGet(&pCookie->pSessionDesc->pendingStatefulCbCount))
        {
            osalAtomicDec(&(pCookie->pSessionDesc->pendingStatefulCbCount));
        }

        /* Free the memory pool */
        if (NULL != pCookie)
        {
            Lac_MemPoolEntryFree(pCookie);
            pCookie = NULL;
        }

        if(NULL != pCbFunc)
        {
            pCbFunc(callbackTag, status);
        }
    }
}

#ifdef ICP_PARAM_CHECK
/**
 *****************************************************************************
 * @ingroup Dc_DataCompression
 *      Check the buffer size depending on the skip mode.
 *
 * @description
 *      Checks that for whatever skip mode option is selected, the buffer list
 *      is big enough to support the kip data modes.
 *
 * @param[in]   bufferSize            Total Buffer list size
 * @param[in]   skipData              Pointer to CpaDcSkipData skipMode setting
 *
 * @retval CPA_STATUS_SUCCESS         Function executed successfully
 * @retval CPA_STATUS_INVALID_PARAM   Invalid parameter passed in
 *
 *****************************************************************************/
STATIC CpaStatus
dcCheckSglSizeWithSkipData(const Cpa32U bufferSize, CpaDcSkipData *skipData)
{
    Cpa32U totalSkippedSize = 0;

    switch (skipData->skipMode)
    {
    case CPA_DC_SKIP_AT_START:
    case CPA_DC_SKIP_AT_END:
        totalSkippedSize = skipData->skipLength;
        break;
    case CPA_DC_SKIP_STRIDE:
        if (0 == skipData->strideLength)
        {
            LAC_INVALID_PARAM_LOG("strideLength parameter cannot "
                                  "be 0 in SKIP and STRIDE mode");
            return CPA_STATUS_INVALID_PARAM;
        }
        if (0 == skipData->skipLength)
        {
            LAC_INVALID_PARAM_LOG("skipLength parameter cannot "
                                  "be 0 in SKIP and STRIDE mode");
            return CPA_STATUS_INVALID_PARAM;
        }
        totalSkippedSize = bufferSize;
        totalSkippedSize /= (skipData->skipLength + skipData->strideLength);
        totalSkippedSize *= skipData->skipLength;
        break;
    case CPA_DC_SKIP_DISABLED:
    default:
        return CPA_STATUS_SUCCESS;
    }

    if (bufferSize < totalSkippedSize)
    {
        LAC_INVALID_PARAM_LOG("Input SGL size cannot hold skip region");
        return CPA_STATUS_INVALID_PARAM;
    }

    return CPA_STATUS_SUCCESS;
}

/**
 *****************************************************************************
 * @ingroup Dc_DataCompression
 *      Check the compression source buffer for Batch and Pack API.
 *
 * @description
 *      Check that all the parameters used for a Batch and Pack compression
 *      request are valid. This function essentially checks the source buffer
 *      parameters and results structure parameters.
 *
 * @param[in]   pSessionHandle        Session handle
 * @param[in]   pSrcBuff              Pointer to data buffer for compression
 * @param[in]   pDestBuff             Pointer to buffer space allocated for
 *                                    output data
 * @param[in]   pResults              Pointer to results structure
 * @param[in]   flushFlag             Indicates the type of flush to be
 *                                    performed
 * @param[in]   srcBuffSize           Size of the source buffer
 *
 * @retval CPA_STATUS_SUCCESS         Function executed successfully
 * @retval CPA_STATUS_INVALID_PARAM   Invalid parameter passed in
 *
 *****************************************************************************/
STATIC CpaStatus
dcCheckSourceData(CpaDcSessionHandle pSessionHandle,
        CpaBufferList *pSrcBuff,
        CpaBufferList *pDestBuff,
        CpaDcRqResults *pResults,
        CpaDcFlush flushFlag,
        Cpa64U srcBuffSize,
        CpaDcSkipData *skipData)
{
    dc_session_desc_t *pSessionDesc = NULL;

    LAC_CHECK_NULL_PARAM(pSessionHandle);
    LAC_CHECK_NULL_PARAM(pSrcBuff);
    LAC_CHECK_NULL_PARAM(pDestBuff);
    LAC_CHECK_NULL_PARAM(pResults);

    pSessionDesc = DC_SESSION_DESC_FROM_CTX_GET(pSessionHandle);
    if(NULL == pSessionDesc)
    {
        LAC_INVALID_PARAM_LOG("Session handle not as expected");
        return CPA_STATUS_INVALID_PARAM;
    }

    if((flushFlag < CPA_DC_FLUSH_NONE) ||
       (flushFlag > CPA_DC_FLUSH_FULL))
    {
        LAC_INVALID_PARAM_LOG("Invalid flushFlag value");
        return CPA_STATUS_INVALID_PARAM;
    }

    if(pSrcBuff == pDestBuff)
    {
        LAC_INVALID_PARAM_LOG("In place operation not supported");
        return CPA_STATUS_INVALID_PARAM;
    }

    /* Compressing zero bytes is not supported for stateless sessions
     * for non Batch and Pack requests */
    if((CPA_DC_STATELESS == pSessionDesc->sessState) && (0 == srcBuffSize) &&
       (NULL == skipData))
    {
        LAC_INVALID_PARAM_LOG("The source buffer size needs to be greater than "
            "zero bytes for stateless sessions");
        return CPA_STATUS_INVALID_PARAM;
    }

    if(srcBuffSize > DC_BUFFER_MAX_SIZE)
    {
        LAC_INVALID_PARAM_LOG("The source buffer size needs to be less than or "
            "equal to 2^32-1 bytes");
        return CPA_STATUS_INVALID_PARAM;
    }

    /* Ensure that total source buffers size is
     * large enough to cover the total size of all the skips
     */
    if (NULL != skipData)
    {
        return dcCheckSglSizeWithSkipData(srcBuffSize, skipData);
    }
    return CPA_STATUS_SUCCESS;
}

/**
 *****************************************************************************
 * @ingroup Dc_DataCompression
 *      Check the compression or decompression function parameters.
 *
 * @description
 *      Check that all the parameters used for a Batch and Pack compression
 *      request are valid. This function essentially checks the destination
 *      buffer parameters and intermediate buffer parameters.
 *
 * @param[in]   pService              Pointer to the compression service
 * @param[in]   pSessionHandle        Session handle
 * @param[in]   pDestBuff             Pointer to buffer space allocated for
 *                                    output data
 * @param[in]   compDecomp            Direction of the operation
 *
 * @retval CPA_STATUS_SUCCESS         Function executed successfully
 * @retval CPA_STATUS_INVALID_PARAM   Invalid parameter passed in
 *
 *****************************************************************************/
STATIC CpaStatus
dcCheckDestinationData(sal_compression_service_t *pService,
        CpaDcSessionHandle pSessionHandle,
        CpaBufferList *pDestBuff,
        CpaDcSkipData *skipData,
        dc_request_dir_t compDecomp)
{
    dc_session_desc_t *pSessionDesc = NULL;
    Cpa64U destBuffSize = 0;

    LAC_CHECK_NULL_PARAM(pSessionHandle);
    LAC_CHECK_NULL_PARAM(pDestBuff);

    pSessionDesc = DC_SESSION_DESC_FROM_CTX_GET(pSessionHandle);
    if(NULL == pSessionDesc)
    {
        LAC_INVALID_PARAM_LOG("Session handle not as expected");
        return CPA_STATUS_INVALID_PARAM;
    }

    if(LacBuffDesc_BufferListVerify(pDestBuff, &destBuffSize,
            LAC_NO_ALIGNMENT_SHIFT)
                != CPA_STATUS_SUCCESS)
    {
        LAC_INVALID_PARAM_LOG("Invalid destination buffer list parameter");
        return CPA_STATUS_INVALID_PARAM;
    }

    if(destBuffSize > DC_BUFFER_MAX_SIZE)
    {
        LAC_INVALID_PARAM_LOG("The destination buffer size needs to be less "
            "than or equal to 2^32-1 bytes");
        return CPA_STATUS_INVALID_PARAM;
    }

    if(CPA_TRUE == pSessionDesc->isDcDp)
    {
        LAC_INVALID_PARAM_LOG("The session type should not be data plane");
        return CPA_STATUS_INVALID_PARAM;
    }

    if(DC_COMPRESSION_REQUEST == compDecomp)
    {
#ifndef ICP_DC_DYN_NOT_SUPPORTED
        if(CPA_DC_HT_FULL_DYNAMIC == pSessionDesc->huffType)
        {

            /* Check if intermediate buffers are supported */
            if((0 == pService->pInterBuffPtrsArrayPhyAddr) ||
               (NULL == pService->pInterBuffPtrsArray))
            {
                LAC_LOG_ERROR("No intermediate buffer defined for this instance "
                    "- see cpaDcStartInstance");
                return CPA_STATUS_INVALID_PARAM;
            }

            /* Ensure that the destination buffer size is greater or equal
             * to 128B */
            if(destBuffSize < DC_DEST_BUFFER_DYN_MIN_SIZE)
            {
                LAC_INVALID_PARAM_LOG("Destination buffer size should be "
                    "greater or equal to 128B");
                return CPA_STATUS_INVALID_PARAM;
            }
        }
        else
#else
        if(CPA_DC_HT_FULL_DYNAMIC == pSessionDesc->huffType)
        {
            LAC_INVALID_PARAM_LOG("Invalid huffType value, dynamic sessions "
                "not supported");
            return CPA_STATUS_INVALID_PARAM;
        }
        else
#endif
        {
            /* Ensure that the destination buffer size is greater or equal
             * to devices min output buff size */
            if (destBuffSize < pService->comp_device_data.minOutputBuffSize)
            {
                LAC_INVALID_PARAM_LOG1("Destination buffer size should be "
                    "greater or equal to %d bytes",
                    pService->comp_device_data.minOutputBuffSize);
                return CPA_STATUS_INVALID_PARAM;
            }
        }

		/* Ensure that total destination buffers size is
         * large enough to cover the total size of all the skips
         */
        if (NULL != skipData)
        {
            return dcCheckSglSizeWithSkipData(destBuffSize, skipData);
        }
    }
    else
    {
        /* Ensure that the destination buffer size is greater than
         * 0 bytes */
        if(destBuffSize < DC_DEST_BUFFER_DEC_MIN_SIZE)
        {
            LAC_INVALID_PARAM_LOG("Destination buffer size should be "
                "greater than 0 bytes");
            return CPA_STATUS_INVALID_PARAM;
        }
    }

    return CPA_STATUS_SUCCESS;
}
#endif

/**
 *****************************************************************************
 * @ingroup Dc_DataCompression
 *      Populate the compression request parameters
 *
 * @description
 *      This function will populate the compression request parameters
 *
 * @param[out]  pCompReqParams   Pointer to the compression request parameters
 * @param[in]   pCookie          Pointer to the compression cookie
 *
 *****************************************************************************/
STATIC void
dcCompRequestParamsPopulate(
    icp_qat_fw_comp_req_params_t *pCompReqParams,
    dc_compression_cookie_t *pCookie)
{
    LAC_ENSURE_NOT_NULL(pCompReqParams);
    LAC_ENSURE_NOT_NULL(pCookie);

    pCompReqParams->comp_len = pCookie->srcTotalDataLenInBytes;
    pCompReqParams->out_buffer_sz = pCookie->dstTotalDataLenInBytes;

}

/**
 *****************************************************************************
 * @ingroup Dc_DataCompression
 *      Create the requests for compression or decompression
 *
 * @description
 *      Create the requests for compression or decompression. This function
 *      will update the cookie will all required information.
 *
 * @param{out]  pCookie             Pointer to the compression cookie
 * @param[in]   pService            Pointer to the compression service
 * @param[in]   pSessionDesc        Pointer to the session descriptor
 * @param[in    pSessionHandle      Session handle
 * @param[in]   pSrcBuff            Pointer to data buffer for compression
 * @param[in]   pDestBuff           Pointer to buffer space for data after
 *                                  compression
 * @param[in]   pResults            Pointer to results structure
 * @param[in]   flushFlag           Indicates the type of flush to be
 *                                  performed
 * @param[in]   callbackTag         Pointer to the callback tag
 * @param[in]   compDecomp          Direction of the operation
 * @param[in]   compressAndVerify   Compress and Verify 
 *
 * @retval CPA_STATUS_SUCCESS       Function executed successfully
 * @retval CPA_STATUS_INVALID_PARAM Invalid parameter passed in
 *
 *****************************************************************************/
STATIC CpaStatus
dcCreateRequest(
    dc_compression_cookie_t *pCookie,
    sal_compression_service_t *pService,
    dc_session_desc_t *pSessionDesc,
    CpaDcSessionHandle pSessionHandle,
    CpaBufferList *pSrcBuff,
    CpaBufferList *pDestBuff,
    CpaDcRqResults *pResults,
    CpaDcFlush flushFlag,
    void *callbackTag,
    dc_request_dir_t compDecomp,
    CpaBoolean compressAndVerify)
{
    icp_qat_fw_comp_req_t* pMsg = NULL;
    icp_qat_fw_comp_req_params_t *pCompReqParams = NULL;
    Cpa64U srcAddrPhys = 0, dstAddrPhys = 0;
    Cpa64U srcTotalDataLenInBytes = 0, dstTotalDataLenInBytes = 0;

    Cpa32U rpCmdFlags = 0;
    Cpa8U sop = ICP_QAT_FW_COMP_SOP;
    Cpa8U eop = ICP_QAT_FW_COMP_EOP;
    Cpa8U bFinal = ICP_QAT_FW_COMP_NOT_BFINAL;
    Cpa8U cnvDecompReq = ICP_QAT_FW_COMP_NO_CNV;
    // Cpa8U cnvDmaCheck = ICP_QAT_FW_COMP_CNV_NO_DMA_CHECK;
    // Cpa8U cnvCksumType = ICP_QAT_FW_COMP_CNV_CKSUM_TYPE_CRC32;

    CpaStatus status = CPA_STATUS_SUCCESS;

    icp_qat_fw_comp_req_t *pReqCache = NULL;

    /* Write the buffer descriptors */
    status = LacBuffDesc_BufferListDescWriteAndGetSize(pSrcBuff, &srcAddrPhys,
            CPA_FALSE, &srcTotalDataLenInBytes,
            &(pService->generic_service_info));
    if (status != CPA_STATUS_SUCCESS)
    {
        return status;
    }

    status = LacBuffDesc_BufferListDescWriteAndGetSize(pDestBuff, &dstAddrPhys,
            CPA_FALSE, &dstTotalDataLenInBytes,
            &(pService->generic_service_info));
    if (status != CPA_STATUS_SUCCESS)
    {
        return status;
    }

    /* Populate the compression cookie */
    pCookie->dcInstance = pService;
    pCookie->pSessionHandle = pSessionHandle;
    pCookie->callbackTag = callbackTag;
    pCookie->pSessionDesc = pSessionDesc;
    pCookie->flushFlag = flushFlag;
    pCookie->pResults = pResults;
    pCookie->compDecomp = compDecomp;
    pCookie->isBnp = CPA_FALSE;

    /* The firmware expects the length in bytes for source and destination to be
     * Cpa32U parameters. However the total data length could be bigger as
     * allocated by the user. We ensure that this is not the case in
     * dcCheckSourceData and cast the values to Cpa32U here */
    pCookie->srcTotalDataLenInBytes = (Cpa32U)srcTotalDataLenInBytes;
#ifndef ICP_DC_DYN_NOT_SUPPORTED
    if((DC_COMPRESSION_REQUEST == compDecomp) &&
       (CPA_DC_HT_FULL_DYNAMIC == pSessionDesc->huffType))
    {
            if(pService->minInterBuffSizeInBytes <
                (Cpa32U)dstTotalDataLenInBytes)
            {
                pCookie->dstTotalDataLenInBytes =
                    (Cpa32U)(pService->minInterBuffSizeInBytes);
            }
            else
            {
                pCookie->dstTotalDataLenInBytes =
                    (Cpa32U)dstTotalDataLenInBytes;
            }
        }
    else
#endif
    {
        pCookie->dstTotalDataLenInBytes = (Cpa32U)dstTotalDataLenInBytes;
    }

    /* Device can not decompress an odd byte decompression request
     * if bFinal is not set
     */
    if (CPA_TRUE != pService->comp_device_data.oddByteDecompNobFinal)
    {
        if((CPA_DC_STATEFUL == pSessionDesc->sessState)
           &&(CPA_DC_FLUSH_FINAL != flushFlag)
           &&(DC_DECOMPRESSION_REQUEST == compDecomp)
           &&(pCookie->srcTotalDataLenInBytes & 0x1))
        {
            pCookie->srcTotalDataLenInBytes--;
        }
    }
    /* Device can not decompress odd byte interim requests */
    if (CPA_TRUE != pService->comp_device_data.oddByteDecompInterim)
    {
        if((CPA_DC_STATEFUL == pSessionDesc->sessState)
            &&(CPA_DC_FLUSH_FINAL != flushFlag)
            &&(CPA_DC_FLUSH_FULL != flushFlag)
            &&(DC_DECOMPRESSION_REQUEST == compDecomp)
            &&(pCookie->srcTotalDataLenInBytes & 0x1))
        {
            pCookie->srcTotalDataLenInBytes--;
        }
    }

    pMsg = (icp_qat_fw_comp_req_t*) &pCookie->request;

    if(DC_COMPRESSION_REQUEST == compDecomp)
    {
        pReqCache = &(pSessionDesc->reqCacheComp);
    }
    else
    {
        pReqCache = &(pSessionDesc->reqCacheDecomp);
    }

    /* Fills the msg from the template cached in the session descriptor */
    osalMemCopy((void*)pMsg, (void*)(pReqCache),
        LAC_QAT_DC_REQ_SZ_LW*LAC_LONG_WORD_IN_BYTES);

    if(DC_REQUEST_FIRST == pSessionDesc->requestType)
    {
        pMsg->comp_pars.initial_adler = 1;
        pMsg->comp_pars.initial_crc32 = 0;

        if(CPA_DC_ADLER32 == pSessionDesc->checksumType)
        {
            pSessionDesc->previousChecksum = 1;
        }
        else
        {
            pSessionDesc->previousChecksum = 0;
        }
    }
    else if (CPA_DC_STATELESS == pSessionDesc->sessState)
    {
        pSessionDesc->previousChecksum = pResults->checksum;

        if(CPA_DC_ADLER32 == pSessionDesc->checksumType)
        {
            pMsg->comp_pars.initial_adler = pSessionDesc->previousChecksum;
        }
        else
        {
            pMsg->comp_pars.initial_crc32 = pSessionDesc->previousChecksum;
        }
    }

    /* Populate the cmdFlags */
    if(CPA_DC_STATEFUL == pSessionDesc->sessState)
    {
        pSessionDesc->previousRequestType = pSessionDesc->requestType;

        if(DC_REQUEST_FIRST == pSessionDesc->requestType)
        {
            /* Update the request type for following requests */
            pSessionDesc->requestType = DC_REQUEST_SUBSEQUENT;

            /* Reinitialise the cumulative amount of consumed bytes */
            pSessionDesc->cumulativeConsumedBytes = 0;

            if(DC_COMPRESSION_REQUEST == compDecomp)
            {
               pSessionDesc->isSopForCompressionProcessed = CPA_TRUE;
            }
            else if(DC_DECOMPRESSION_REQUEST == compDecomp) 
            {
               pSessionDesc->isSopForDecompressionProcessed = CPA_TRUE;
            }
        }
        else
        {
            if (DC_COMPRESSION_REQUEST == compDecomp)
            {
               if(CPA_TRUE  == pSessionDesc->isSopForCompressionProcessed)
               {
                  sop = ICP_QAT_FW_COMP_NOT_SOP;
               }
               else
               {
                  pSessionDesc->isSopForCompressionProcessed = CPA_TRUE;
               }
            }
            else if (DC_DECOMPRESSION_REQUEST == compDecomp)
            {
               if(CPA_TRUE  == pSessionDesc->isSopForDecompressionProcessed)
               {
                  sop = ICP_QAT_FW_COMP_NOT_SOP;
               }
               else
               {
                  pSessionDesc->isSopForDecompressionProcessed = CPA_TRUE;
               }
            }
        }

        if((CPA_DC_FLUSH_FINAL == flushFlag)||(CPA_DC_FLUSH_FULL == flushFlag))
        {
            /* Update the request type for following requests */
            pSessionDesc->requestType = DC_REQUEST_FIRST;
        }
        else
        {
            eop = ICP_QAT_FW_COMP_NOT_EOP;

        }
    }

    pCompReqParams = &(pMsg->comp_pars);
    /* Comp requests param populate
     * (LW 14 - 15)*/
    dcCompRequestParamsPopulate(
            pCompReqParams,
            pCookie);

    if(CPA_DC_FLUSH_FINAL == flushFlag)
    {
        bFinal = ICP_QAT_FW_COMP_BFINAL;
    }

    if(CPA_TRUE == compressAndVerify)
    {
	cnvDecompReq = ICP_QAT_FW_COMP_CNV;
    }

    rpCmdFlags = ICP_QAT_FW_COMP_REQ_PARAM_FLAGS_BUILD(sop,
                                                       eop,
                                                       bFinal,
                                                       cnvDecompReq);

    pMsg->comp_pars.req_par_flags = rpCmdFlags;

    /* Populates the QAT common request middle part of the message
     * (LW 6 to 11) */
    SalQatMsg_CmnMidWrite((icp_qat_fw_la_bulk_req_t *)pMsg,
                     pCookie,
                     DC_DEFAULT_QAT_PTR_TYPE,
                     srcAddrPhys,
                     dstAddrPhys,
                     0,
                     0);

    return CPA_STATUS_SUCCESS;
}

/**
 *****************************************************************************
 * @ingroup Dc_DataCompression
 *      Create the requests for compression or decompression
 *
 * @description
 *      Create the requests for compression or decompression. This function
 *      will update the cookie will all required information.
 *
 * @param{out]  pCookie             Pointer to the compression cookie
 * @param[in]   pService            Pointer to the compression service
 * @param[in]   pSessionDesc        Pointer to the session descriptor
 * @param[in    pSessionHandle      Session handle
 * @param[in]   numRequests         Number of operations in the batch request
 * @param[in]   pBatchOpData        Address of the list of jobs to be processed
 * @param[in]   pDestBuff           Pointer to buffer space for data after
 *                                  compression
 * @param[in]   pResults            Pointer to results structure
 * @param[in]   flushFlag           Indicates the type of flush to be
 *                                  performed
 * @param[in]   callbackTag         Pointer to the callback tag
 * @param[in]   compDecomp          Direction of the operation
 *
 * @retval CPA_STATUS_SUCCESS       Function executed successfully
 * @retval CPA_STATUS_INVALID_PARAM Invalid parameter passed in
 *
 *****************************************************************************/
STATIC CpaStatus
dcCreateBatchRequest(
    dc_compression_cookie_t   *pCookie,
    sal_compression_service_t *pService,
    dc_session_desc_t         *pSessionDesc,
    CpaDcSessionHandle        pSessionHandle,
    const Cpa32U              numRequests,
    CpaDcBatchOpData          *pBatchOpData,
    CpaBufferList             *pDestBuff,
    CpaDcRqResults            *pResults,
    void                      *callbackTag,
    dc_request_dir_t          compDecomp)
{
    CpaStatus status = CPA_STATUS_SUCCESS;

    icp_qat_fw_comp_req_t* pMsg = NULL;
    icp_qat_fw_comp_req_params_t *pCompReqParams = NULL;
    Cpa64U bnpAddrPhys = 0, dstAddrPhys = 0;
    Cpa64U dstTotalDataLenInBytes = 0;
    icp_qat_fw_comp_req_t *pReqCache = NULL;

    status = LacBuffDesc_BnpOpListDescWrite(pBatchOpData, numRequests,
            &bnpAddrPhys, pResults, pSessionDesc->checksumType,
            &(pService->generic_service_info));
    if (status != CPA_STATUS_SUCCESS)
    {
        return status;
    }

    status = LacBuffDesc_BufferListDescWriteAndGetSize(pDestBuff,
            &dstAddrPhys, CPA_FALSE, &dstTotalDataLenInBytes,
            &(pService->generic_service_info));
    if (status != CPA_STATUS_SUCCESS)
    {
        return status;
    }

    /* Populate the compression cookie */
    pCookie->dcInstance = pService;
    pCookie->pSessionHandle = pSessionHandle;
    pCookie->callbackTag = callbackTag;
    pCookie->pSessionDesc = pSessionDesc;
    pCookie->flushFlag = CPA_DC_FLUSH_NONE;
    pCookie->pResults = &pResults[0];
    pCookie->compDecomp = compDecomp;
    pCookie->pBatchOpData = pBatchOpData;
    pCookie->numJobsInBatch = numRequests;
    pCookie->isBnp = CPA_TRUE;

    /* SGL sizes are set in the descriptors for Batch and Pack */
    pCookie->srcTotalDataLenInBytes = (Cpa32U)0;
#ifndef ICP_DC_DYN_NOT_SUPPORTED
    if (CPA_DC_HT_FULL_DYNAMIC == pSessionDesc->huffType)
    {
        if (pService->minInterBuffSizeInBytes <
            (Cpa32U)dstTotalDataLenInBytes)
        {
            pCookie->dstTotalDataLenInBytes =
                (Cpa32U)(pService->minInterBuffSizeInBytes);
        }
        else
        {
            pCookie->dstTotalDataLenInBytes =
                (Cpa32U)dstTotalDataLenInBytes;
        }
    }
    else
#endif
    {
        pCookie->dstTotalDataLenInBytes = (Cpa32U)dstTotalDataLenInBytes;
    }

    pMsg = (icp_qat_fw_comp_req_t*) &pCookie->request;
    pReqCache = &(pSessionDesc->reqCacheComp);

    /* Fills the msg from the template cached in the session descriptor */
    osalMemCopy((void*)pMsg, (void*)(pReqCache),
        LAC_QAT_DC_REQ_SZ_LW*LAC_LONG_WORD_IN_BYTES);

    pCompReqParams = &(pMsg->comp_pars);
    /* Comp requests param populate
     * (LW 14 - 15)*/
    pCompReqParams->comp_len = 0;
    pCompReqParams->out_buffer_sz = pCookie->dstTotalDataLenInBytes;


    pMsg->comp_pars.req_par_flags = 0;

    /* Populates the QAT common request middle part of the message
     * (LW 6 to 11) */
   SalQatMsg_CmnMidWrite((icp_qat_fw_la_bulk_req_t *)pMsg,
                    pCookie,
                    QAT_COMN_PTR_TYPE_BATCH,
                    bnpAddrPhys,
                    dstAddrPhys,
                    0,
                    0);

    return CPA_STATUS_SUCCESS;
}

/**
 *****************************************************************************
 * @ingroup Dc_DataCompression
 *      Send a compression request to QAT
 *
 * @description
 *      Send the requests for compression or decompression to QAT
 *
 * @param{in]   pCookie               Pointer to the compression cookie
 * @param[in]   pService              Pointer to the compression service
 * @param[in]   pSessionDesc          Pointer to the session descriptor
 * @param[in]   compDecomp            Direction of the operation
 *
 * @retval CPA_STATUS_SUCCESS         Function executed successfully
 * @retval CPA_STATUS_INVALID_PARAM   Invalid parameter passed in
 *
 *****************************************************************************/
STATIC CpaStatus
dcSendRequest(dc_compression_cookie_t* pCookie,
              sal_compression_service_t* pService,
              dc_session_desc_t* pSessionDesc,
              dc_request_dir_t compDecomp)
{
    CpaStatus status = CPA_STATUS_SUCCESS;

    /* Send to QAT */
    status = SalQatMsg_transPutMsg(pService->trans_handle_compression_tx,
                            (void*)&(pCookie->request),
                        LAC_QAT_DC_REQ_SZ_LW,
                        LAC_LOG_MSG_DC);

    if((CPA_DC_STATEFUL == pSessionDesc->sessState) &&
            (CPA_STATUS_RETRY == status))
    {
        /* reset requestType after receiving an retry on
         * the stateful request */
        pSessionDesc->requestType = pSessionDesc->previousRequestType;
    }

    return status;
}

/**
 *****************************************************************************
 * @ingroup Dc_DataCompression
 *      Process the synchronous and asynchronous case for compression or
 *      decompression
 *
 * @description
 *      Process the synchronous and asynchronous case for compression or
 *      decompression. This function will then create and send the request to
 *      the firmware.
 *
 * @param[in]   pService            Pointer to the compression service
 * @param[in]   pSessionDesc        Pointer to the session descriptor
 * @param[in]   dcInstance          Instance handle derived from discovery
 *                                  functions
 * @param[in]   pSessionHandle      Session handle
 * @param[in]   numRequests         Number of operations in the batch request
 * @param[in]   pBatchOpData        Address of the list of jobs to be processed
 * @param[in]   pSrcBuff            Pointer to data buffer for compression
 * @param[in]   pDestBuff           Pointer to buffer space for data after
 *                                  compression
 * @param[in]   pResults            Pointer to results structure
 * @param[in]   flushFlag           Indicates the type of flush to be
 *                                  performed
 * @param[in]   callbackTag         Pointer to the callback tag
 * @param[in]   compDecomp          Direction of the operation
 * @param[in]   isAsyncMode         Used to know if synchronous or asynchronous
 *                                  mode
 * @param[in]   compressAndVerify   Compress and Verify mode 
 *
 * @retval CPA_STATUS_SUCCESS       Function executed successfully
 * @retval CPA_STATUS_FAIL          Function failed
 * @retval CPA_STATUS_RESOURCE      Resource error
 *
 *****************************************************************************/
STATIC CpaStatus
dcCompDecompData(sal_compression_service_t* pService,
        dc_session_desc_t* pSessionDesc,
        CpaInstanceHandle dcInstance,
        CpaDcSessionHandle pSessionHandle,
        const Cpa32U       numRequests,
        CpaDcBatchOpData   *pBatchOpData,
        CpaBufferList *pSrcBuff,
        CpaBufferList *pDestBuff,
        CpaDcRqResults *pResults,
        CpaDcFlush flushFlag,
        void *callbackTag,
        dc_request_dir_t compDecomp,
        CpaBoolean isAsyncMode,
        CpaBoolean compressAndVerify)
{
    CpaStatus status = CPA_STATUS_SUCCESS;
    dc_compression_cookie_t *pCookie = NULL;

    if((LacSync_GenWakeupSyncCaller == pSessionDesc->pCompressionCb)
        && isAsyncMode == CPA_TRUE)
    {
        lac_sync_op_data_t *pSyncCallbackData = NULL;

        status = LacSync_CreateSyncCookie(&pSyncCallbackData);

        if(CPA_STATUS_SUCCESS == status)
        {
            status = dcCompDecompData(pService, pSessionDesc, dcInstance,
                         pSessionHandle, numRequests, pBatchOpData,
                         pSrcBuff, pDestBuff, pResults,
                         flushFlag, pSyncCallbackData, compDecomp, CPA_FALSE,
                         compressAndVerify);
        }
        else
        {
            return status;
        }

        if(CPA_STATUS_SUCCESS == status)
        {
            CpaStatus syncStatus = CPA_STATUS_SUCCESS;

            syncStatus = LacSync_WaitForCallback(pSyncCallbackData,
                            DC_SYNC_CALLBACK_TIMEOUT,
                            &status, NULL);

            /* If callback doesn't come back */
            if(CPA_STATUS_SUCCESS != syncStatus)
            {
                if(DC_COMPRESSION_REQUEST == compDecomp)
                {
                    COMPRESSION_STAT_INC(numCompCompletedErrors, pService);
                }
                else
                {
                    COMPRESSION_STAT_INC(numDecompCompletedErrors, pService);
                }
                LAC_LOG_ERROR("Callback timed out");
                status = syncStatus;
            }
        }
        else
        {
            /* As the Request was not sent the Callback will never
             * be called, so need to indicate that we're finished
             * with cookie so it can be destroyed. */
            LacSync_SetSyncCookieComplete(pSyncCallbackData);
        }

        LacSync_DestroySyncCookie(&pSyncCallbackData);
        return status;
    }

    /* Allocate the compression cookie
     * The memory is freed in callback or in sendRequest if an error occurs
     */
    do {
        pCookie = (dc_compression_cookie_t*)
                                  Lac_MemPoolEntryAlloc(pService->compression_mem_pool);
        if (NULL == pCookie)
        {
            LAC_LOG_ERROR("Cannot get mem pool entry for compression");
            status = CPA_STATUS_RESOURCE;
        }
        else if ((void*)CPA_STATUS_RETRY == pCookie)
        {
            /* Give back the control to the OS */
            osalYield();
        }
    } while ((void*)CPA_STATUS_RETRY == pCookie);

    if(CPA_STATUS_SUCCESS == status)
    {
        if (NULL == pBatchOpData)
        {
            status = dcCreateRequest(pCookie, pService, pSessionDesc,
                    pSessionHandle, pSrcBuff, pDestBuff,
                    pResults, flushFlag, callbackTag, compDecomp,
                    compressAndVerify);
        }
        else
        {
            status = dcCreateBatchRequest(pCookie, pService, pSessionDesc,
                    pSessionHandle, numRequests, pBatchOpData,
                    pDestBuff, pResults, callbackTag,
                    DC_COMPRESSION_REQUEST);
        }
    }

    if(CPA_STATUS_SUCCESS == status)
    {
        status = dcSendRequest(pCookie, pService, pSessionDesc, compDecomp);
    }

    if(CPA_STATUS_SUCCESS == status)
    {
        if(DC_COMPRESSION_REQUEST == compDecomp)
        {
            COMPRESSION_STAT_INC(numCompRequests, pService);
        }
        else
        {
            COMPRESSION_STAT_INC(numDecompRequests, pService);
        }
    }
    else if (NULL == pBatchOpData)
    {
        /* Failed to create cookie and not in Batch and Pack mode */
        if(DC_COMPRESSION_REQUEST == compDecomp)
        {
            COMPRESSION_STAT_INC(numCompRequestsErrors, pService);
        }
        else
        {
            COMPRESSION_STAT_INC(numDecompRequestsErrors, pService);
        }

        /* Decrement number of pending callbacks for session */
        if(CPA_DC_STATELESS == pSessionDesc->sessState)
        {
            osalAtomicDec(&(pSessionDesc->pendingStatelessCbCount));
        }
        else
        {
            osalAtomicDec(&(pSessionDesc->pendingStatefulCbCount));
        }
        /* Free the memory pool */
        if (NULL != pCookie)
        {
            Lac_MemPoolEntryFree(pCookie);
            pCookie = NULL;
        }
    }
    else
    {
        /* Failed to create cookie in Batch and Pack mode */
        COMPRESSION_STAT_INC(numCompRequestsErrors, pService);

        osalAtomicDec(&(pSessionDesc->pendingStatelessCbCount));
        /* Free the memory pool */
        if (NULL != pCookie)
        {
            Lac_MemPoolEntryFree(pCookie);
            pCookie = NULL;
        }
    }

    return status;
}

/**
 *****************************************************************************
 * @ingroup Dc_DataCompression
 *      Handle zero length compression or decompression requests
 *
 * @description
 *      Handle zero length compression or decompression requests
 *
 * @param[in]   pService              Pointer to the compression service
 * @param[in]   pSessionDesc          Pointer to the session descriptor
 * @param[in]   pResults              Pointer to results structure
 * @param[in]   flushFlag             Indicates the type of flush to be
 *                                    performed
 * @param[in]   callbackTag           User supplied value to help correlate
 *                                    the callback with its associated request
 * @param[in]   compDecomp            Direction of the operation
 *
 * @retval CPA_TRUE                   Zero length SOP or MOP processed
 * @retval CPA_FALSE                  Zero length EOP
 *
 *****************************************************************************/
STATIC CpaBoolean
dcZeroLengthRequests(sal_compression_service_t *pService,
        dc_session_desc_t *pSessionDesc,
        CpaDcRqResults *pResults,
        CpaDcFlush flushFlag,
        void *callbackTag,
        dc_request_dir_t compDecomp)
{
    CpaBoolean status = CPA_FALSE;
    CpaDcCallbackFn pCbFunc = pSessionDesc->pCompressionCb;

    if(DC_REQUEST_FIRST == pSessionDesc->requestType)
    {
        /* Reinitialise the cumulative amount of consumed bytes */
        pSessionDesc->cumulativeConsumedBytes = 0;

        /* Zero length SOP */
        if(CPA_DC_ADLER32 == pSessionDesc->checksumType)
        {
            pResults->checksum = 1;
        }
        else
        {
            pResults->checksum = 0;
        }

        status = CPA_TRUE;
    }
    else if((CPA_DC_FLUSH_NONE == flushFlag) ||
            (CPA_DC_FLUSH_SYNC == flushFlag))
    {
        /* Zero length MOP */
        pResults->checksum = pSessionDesc->previousChecksum;
        status = CPA_TRUE;
    }

    if(CPA_TRUE == status)
    {
        pResults->status = CPA_DC_OK;
        pResults->produced = 0;
        pResults->consumed = 0;

        /* Increment statistics */
        if(DC_COMPRESSION_REQUEST == compDecomp)
        {
            COMPRESSION_STAT_INC(numCompRequests, pService);
            COMPRESSION_STAT_INC(numCompCompleted, pService);
        }
        else
        {
            COMPRESSION_STAT_INC(numDecompRequests, pService);
            COMPRESSION_STAT_INC(numDecompCompleted, pService);
        }

        LAC_SPINUNLOCK(&(pSessionDesc->sessionLock));

        if((NULL != pCbFunc) && (LacSync_GenWakeupSyncCaller != pCbFunc))
        {
            pCbFunc(callbackTag, CPA_STATUS_SUCCESS);
        }

        return CPA_TRUE;
    }

    return CPA_FALSE;
}

/**
 *****************************************************************************
 * @ingroup Dc_DataCompression
 *      Handle zero length compression batch requests
 *
 * @description
 *      Handle zero length compression batch requests
 *
 * @param[in]   pService              Pointer to the compression service
 * @param[in]   pSessionDesc          Pointer to the session descriptor
 * @param[in]   callbackTag           User supplied value to help correlate
 *                                    the callback with its associated request
 * @retval CPA_TRUE                   Zero length SOP or MOP processed
 *
 *****************************************************************************/
STATIC void
dcZeroLengthBatchRequests(sal_compression_service_t *pService,
        dc_session_desc_t  *pSessionDesc,
        const Cpa32U       numRequests,
        CpaDcRqResults     *pResults,
        void               *callbackTag)
{
    Cpa32U i = 0;
    CpaDcCallbackFn pCbFunc = pSessionDesc->pCompressionCb;

    /* Re-initialize the cumulative amount of consumed bytes */
    pSessionDesc->cumulativeConsumedBytes = 0;
    for (i=0; i<numRequests; i++)
    {
        pResults[i].status = CPA_DC_OK;
        pResults[i].consumed = 0;
        pResults[i].produced = 0;
        /* Checksum remains unchanged for zero length request */
    }

    /* Increment statistics */
    COMPRESSION_STAT_INC(numCompRequests, pService);
    COMPRESSION_STAT_INC(numCompCompleted, pService);

    /* Call application callback function */
    if((NULL != pCbFunc) && (LacSync_GenWakeupSyncCaller != pCbFunc))
    {
        pCbFunc(callbackTag, CPA_STATUS_SUCCESS);
    }
    return;
}

CpaStatus
cpaDcBPCompressData(CpaInstanceHandle dcInstance,
        CpaDcSessionHandle  pSessionHandle,
        const Cpa32U        numRequests,
        CpaDcBatchOpData    *pBatchOpData,
        CpaBufferList       *pDestBuff,
        CpaDcRqResults      *pResults,
        void                *callbackTag)
{
    sal_compression_service_t* pService = NULL;
    dc_session_desc_t* pSessionDesc = NULL;
    CpaInstanceHandle insHandle = NULL;
    Cpa64U srcBuffSize = 0;
    Cpa64U batchTotalSize = 0;
    CpaBufferList *pSrcBuff = NULL;
    CpaDcFlush flushFlag = CPA_DC_FLUSH_NONE;
    Cpa32U i = 0;
#ifdef ICP_PARAM_CHECK
    CpaDcSkipData *skipData = NULL;
    CpaDcSkipMode firstInputSkipMode = CPA_DC_SKIP_DISABLED;
    CpaDcSkipMode inputSkipMode = CPA_DC_SKIP_DISABLED;
    CpaDcSkipMode firstOutputSkipMode = CPA_DC_SKIP_DISABLED;
    CpaDcSkipMode outputSkipMode = CPA_DC_SKIP_DISABLED;
#endif


#ifdef ICP_TRACE
    LAC_LOG7("Called with params (0x%lx, 0x%lx, %d, 0x%lx, 0x%lx, 0x%lx, "
            "0x%lx)\n",
            (LAC_ARCH_UINT)dcInstance,
            (LAC_ARCH_UINT)pSessionHandle,
            numRequests,
            (LAC_ARCH_UINT)pBatchOpData,
            (LAC_ARCH_UINT)pDestBuff,
            (LAC_ARCH_UINT)pResults,
            (Cpa64U)callbackTag);
#endif

    if(CPA_INSTANCE_HANDLE_SINGLE == dcInstance)
    {
         insHandle = dcGetFirstHandle();
    }
    else
    {
         insHandle = dcInstance;
    }

    pService = (sal_compression_service_t*) insHandle;

#ifdef ICP_PARAM_CHECK
    LAC_CHECK_NULL_PARAM(insHandle);
    SAL_CHECK_ADDR_TRANS_SETUP(insHandle);
    LAC_CHECK_NULL_PARAM(pBatchOpData);

    /* Ensure there is at least one job in the batch to perform */
    if (0 == numRequests)
    {
        LAC_INVALID_PARAM_LOG("Invalid numRequests value, a minimum "
                              "of 1 request is required in the batch.");
        return CPA_STATUS_INVALID_PARAM;
    }
#endif

    /* Check if SAL is initialized otherwise return an error */
    SAL_RUNNING_CHECK(insHandle);

#ifdef ICP_PARAM_CHECK
    /* Ensure this is a compression instance */
    SAL_CHECK_INSTANCE_TYPE(insHandle, SAL_SERVICE_TYPE_COMPRESSION);

    skipData = &pBatchOpData[i].opData.outputSkipData;
    /* Ensure the skip mode has not been set to CPA_DC_SKIP_STRIDE
     * for the output skip mode
     * this feature is currently not supported by the driver */
    if(CPA_DC_SKIP_STRIDE == skipData->skipMode)
    {
        LAC_INVALID_PARAM_LOG("CPA_DC_SKIP_STRIDE is not supported for "
                               "output skipMode");
        return CPA_STATUS_UNSUPPORTED;
    }

    if (dcCheckDestinationData(pService, pSessionHandle, pDestBuff,
            skipData, DC_COMPRESSION_REQUEST) != CPA_STATUS_SUCCESS)
    {
        return CPA_STATUS_INVALID_PARAM;
    }
#endif

    pSessionDesc = DC_SESSION_DESC_FROM_CTX_GET(pSessionHandle);

#ifdef ICP_PARAM_CHECK
    /* Ensure this is a compression instance */
    if(CPA_DC_DIR_DECOMPRESS == pSessionDesc->sessDirection)
    {
        LAC_INVALID_PARAM_LOG("Invalid sessDirection value");
        return CPA_STATUS_INVALID_PARAM;
    }
    /* Ensure Data Plane API is not used. */
    if(CPA_TRUE == pSessionDesc->isDcDp)
    {
        LAC_INVALID_PARAM_LOG("Data Plane API not supported "
                              "for batch operations");
        return CPA_STATUS_INVALID_PARAM;
    }
#endif

#ifdef ICP_DC_DYN_NOT_SUPPORTED
    if (pSessionDesc->huffType == CPA_DC_HT_FULL_DYNAMIC)
    {
        LAC_INVALID_PARAM_LOG("Invalid huffType value, dynamic sessions "
                              "not supported");
        return CPA_STATUS_INVALID_PARAM;
    }
#endif

#ifdef ICP_PARAM_CHECK
    firstInputSkipMode = pBatchOpData[0].opData.inputSkipData.skipMode;
    firstOutputSkipMode = pBatchOpData[0].opData.outputSkipData.skipMode;
#endif
    for(i=0; i<numRequests; i++)
    {
        pSrcBuff = pBatchOpData[i].pSrcBuff;
        LAC_CHECK_NULL_PARAM(pSrcBuff);

        if(LacBuffDesc_BufferListVerifyNull(pSrcBuff, &srcBuffSize,
            LAC_NO_ALIGNMENT_SHIFT)
               != CPA_STATUS_SUCCESS)
        {
            LAC_INVALID_PARAM_LOG("Invalid source buffer list parameter");
            return CPA_STATUS_INVALID_PARAM;
        }
#ifdef ICP_PARAM_CHECK
        flushFlag = pBatchOpData[i].opData.flushFlag;
        skipData = &pBatchOpData[i].opData.inputSkipData;

        /* Ensure the skip mode has not been set to CPA_DC_SKIP_STRIDE
         * for the input skip mode
         * this feature is currently not supported by the driver */
        if(CPA_DC_SKIP_STRIDE == skipData->skipMode)
        {
            LAC_INVALID_PARAM_LOG("CPA_DC_SKIP_STRIDE is not supported for "
                               "input skipMode");
            return CPA_STATUS_UNSUPPORTED;
        }

        if (CPA_STATUS_SUCCESS != dcCheckSourceData(pSessionHandle, pSrcBuff,
                pDestBuff, &pResults[i], flushFlag, srcBuffSize, skipData))
        {
            return CPA_STATUS_INVALID_PARAM;
        }

        /* Ensure that input skip modes are identical
         * for all entries of the pBatchOpData[]. */
        inputSkipMode = pBatchOpData[i].opData.inputSkipData.skipMode;
        if (firstInputSkipMode != inputSkipMode)
        {
            LAC_INVALID_PARAM_LOG("Input skip modes need to be identical.");
            return CPA_STATUS_INVALID_PARAM;
        }

        /* Ensure that output skip modes are identical
         * for all entries of the pBatchOpData[]. */
        outputSkipMode = pBatchOpData[i].opData.outputSkipData.skipMode;
        if (firstOutputSkipMode != outputSkipMode)
        {
            LAC_INVALID_PARAM_LOG("Output skip modes need to be identical.");
            return CPA_STATUS_INVALID_PARAM;
        }
#endif
        batchTotalSize += srcBuffSize;
    }

    /* If the entire job is zero length, then update
     * CpaDcRqResults structures and trigger the callback. */
    if(0 == batchTotalSize)
    {
        dcZeroLengthBatchRequests(pService, pSessionDesc,
                numRequests, pResults, callbackTag);
        return CPA_STATUS_SUCCESS;
    }
    osalAtomicInc(&(pSessionDesc->pendingStatelessCbCount));

    return dcCompDecompData(pService, pSessionDesc, dcInstance,
               pSessionHandle, numRequests, pBatchOpData,
               pSrcBuff, pDestBuff, pResults,
               flushFlag, callbackTag, DC_COMPRESSION_REQUEST,
               CPA_TRUE, CPA_FALSE);
}

CpaStatus
cpaDcCompressData(CpaInstanceHandle dcInstance,
        CpaDcSessionHandle pSessionHandle,
        CpaBufferList *pSrcBuff,
        CpaBufferList *pDestBuff,
        CpaDcRqResults *pResults,
        CpaDcFlush flushFlag,
        void *callbackTag)
{
    sal_compression_service_t* pService = NULL;
    dc_session_desc_t* pSessionDesc = NULL;
    CpaInstanceHandle insHandle = NULL;
    Cpa64U srcBuffSize = 0;

#ifdef ICP_TRACE
    LAC_LOG7("Called with params (0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, "
            "0x%x, 0x%lx)\n",
            (LAC_ARCH_UINT)dcInstance,
            (LAC_ARCH_UINT)pSessionHandle,
            (LAC_ARCH_UINT)pSrcBuff,
            (LAC_ARCH_UINT)pDestBuff,
            (LAC_ARCH_UINT)pResults,
            flushFlag,
            (Cpa64U)callbackTag);
#endif

    if(CPA_INSTANCE_HANDLE_SINGLE == dcInstance)
    {
         insHandle = dcGetFirstHandle();
    }
    else
    {
         insHandle = dcInstance;
    }

    pService = (sal_compression_service_t*) insHandle;

#ifdef ICP_PARAM_CHECK
    LAC_CHECK_NULL_PARAM(insHandle);
    SAL_CHECK_ADDR_TRANS_SETUP(insHandle);
#endif

    /* Check if SAL is initialised otherwise return an error */
    SAL_RUNNING_CHECK(insHandle);

    /* This check is outside the parameter checking as it is needed to manage
     * zero length requests */
    if(LacBuffDesc_BufferListVerifyNull(pSrcBuff, &srcBuffSize,
            LAC_NO_ALIGNMENT_SHIFT)
               != CPA_STATUS_SUCCESS)
    {
        LAC_INVALID_PARAM_LOG("Invalid source buffer list parameter");
        return CPA_STATUS_INVALID_PARAM;
    }

#ifdef ICP_PARAM_CHECK
    /* Ensure this is a compression instance */
    SAL_CHECK_INSTANCE_TYPE(insHandle, SAL_SERVICE_TYPE_COMPRESSION);

    if (dcCheckSourceData(pSessionHandle, pSrcBuff, pDestBuff,
            pResults, flushFlag, srcBuffSize, NULL) != CPA_STATUS_SUCCESS)
    {
        return CPA_STATUS_INVALID_PARAM;
    }
    if (dcCheckDestinationData(pService, pSessionHandle, pDestBuff,
            NULL, DC_COMPRESSION_REQUEST) != CPA_STATUS_SUCCESS)
    {
        return CPA_STATUS_INVALID_PARAM;
    }
#endif

    pSessionDesc = DC_SESSION_DESC_FROM_CTX_GET(pSessionHandle);

#ifdef ICP_PARAM_CHECK
    if(CPA_DC_DIR_DECOMPRESS == pSessionDesc->sessDirection)
    {
        LAC_INVALID_PARAM_LOG("Invalid sessDirection value");
        return CPA_STATUS_INVALID_PARAM;
    }
#endif

#ifdef ICP_DC_DYN_NOT_SUPPORTED
    if (CPA_DC_HT_FULL_DYNAMIC == pSessionDesc->huffType)
    {
        LAC_INVALID_PARAM_LOG("Invalid huffType value, dynamic sessions "
                              "not supported");
        return CPA_STATUS_INVALID_PARAM;
    }
#endif

    if(CPA_DC_STATEFUL == pSessionDesc->sessState)
    {
        /* Lock the session to check if there are in-flight stateful requests */
        LAC_SPINLOCK(&(pSessionDesc->sessionLock));

        /* Check if there is already one in-flight stateful request */
        if(0 != osalAtomicGet(&(pSessionDesc->pendingStatefulCbCount)))
        {
            LAC_LOG_ERROR("Only one in-flight stateful request supported");
            LAC_SPINUNLOCK(&(pSessionDesc->sessionLock));
            return CPA_STATUS_RETRY;
        }

        if(0 == srcBuffSize)
        {
            if(CPA_TRUE == dcZeroLengthRequests(pService, pSessionDesc,
                    pResults, flushFlag, callbackTag, DC_COMPRESSION_REQUEST))
            {
                return CPA_STATUS_SUCCESS;
            }
        }

        osalAtomicInc(&(pSessionDesc->pendingStatefulCbCount));
        LAC_SPINUNLOCK(&(pSessionDesc->sessionLock));
    }
    else
    {
        osalAtomicInc(&(pSessionDesc->pendingStatelessCbCount));
    }

    return dcCompDecompData(pService, pSessionDesc,  dcInstance,
               pSessionHandle, 0, NULL,
               pSrcBuff, pDestBuff, pResults,
               flushFlag, callbackTag, DC_COMPRESSION_REQUEST, CPA_TRUE,
               CPA_FALSE);
}

#ifdef ICP_PARAM_CHECK
STATIC CpaStatus
cnvParamCheck ( const CpaInstanceHandle dcInstance,
                const CpaDcSessionHandle pSessionHandle,
                const CpaBufferList      *pSrcBuff,
                const CpaBufferList      *pDestBuff,
                const CpaDcOpData        *pOpData,
                const CpaDcRqResults     *pResults)
{
    sal_compression_service_t* pService = NULL;
    dc_session_desc_t* pSessionDesc = NULL;
    CpaInstanceHandle insHandle = NULL;
    Cpa64U srcBuffSize = 0;

    if ((CPA_TRUE != pOpData->compressAndVerify)
     && (CPA_FALSE != pOpData->compressAndVerify))
    {
        return CPA_STATUS_INVALID_PARAM;
    }
    if(CPA_INSTANCE_HANDLE_SINGLE == dcInstance)
    {
         insHandle = dcGetFirstHandle();
    }
    else
    {
         insHandle = dcInstance;
    }

    pService = (sal_compression_service_t*) insHandle;

    LAC_CHECK_NULL_PARAM(insHandle);
    SAL_CHECK_ADDR_TRANS_SETUP(insHandle);

    if(LacBuffDesc_BufferListVerifyNull(pSrcBuff, &srcBuffSize,
          LAC_NO_ALIGNMENT_SHIFT)
             != CPA_STATUS_SUCCESS)
  {
      LAC_INVALID_PARAM_LOG("Invalid source buffer list parameter");
      return CPA_STATUS_INVALID_PARAM;
  }

    /* Ensure this is a compression instance */
    SAL_CHECK_INSTANCE_TYPE(insHandle, SAL_SERVICE_TYPE_COMPRESSION);
    if (dcCheckSourceData(pSessionHandle, (CpaBufferList*)pSrcBuff, (CpaBufferList*)pDestBuff,
            (CpaDcRqResults*)pResults, pOpData->flushFlag, srcBuffSize, NULL) != CPA_STATUS_SUCCESS)
    {
        return CPA_STATUS_INVALID_PARAM;
    }
    if (dcCheckDestinationData(pService, pSessionHandle, (CpaBufferList*)pDestBuff,
            NULL, DC_COMPRESSION_REQUEST) != CPA_STATUS_SUCCESS)
    {
        return CPA_STATUS_INVALID_PARAM;
    }
    pSessionDesc = DC_SESSION_DESC_FROM_CTX_GET(pSessionHandle);

    if(CPA_DC_DIR_DECOMPRESS == pSessionDesc->sessDirection)
    {
        LAC_INVALID_PARAM_LOG("Invalid sessDirection value");
        return CPA_STATUS_INVALID_PARAM;
    }

    if (!(pService->generic_service_info.dcExtendedFeatures & DC_CNV_EXTENDED_CAPABILITY))
    {
        LAC_INVALID_PARAM_LOG("Invalid compressAndVerify, no CNV capability");
        return CPA_STATUS_UNSUPPORTED;
    }
    if(CPA_DC_STATEFUL == pSessionDesc->sessState)
    {
        LAC_INVALID_PARAM_LOG("Invalid CNV, stateful sessState not supported");
        return CPA_STATUS_UNSUPPORTED;
    }
    return CPA_STATUS_SUCCESS;
}
#endif

CpaStatus
cpaDcCompressData2( CpaInstanceHandle dcInstance,
        CpaDcSessionHandle  pSessionHandle,
        CpaBufferList       *pSrcBuff,
        CpaBufferList       *pDestBuff,
        CpaDcOpData         *pOpData,
        CpaDcRqResults      *pResults,
        void                 *callbackTag )
{
    sal_compression_service_t* pService = NULL;
    dc_session_desc_t* pSessionDesc = NULL;
    CpaInstanceHandle insHandle = NULL;
    Cpa64U srcBuffSize = 0;

#ifdef ICP_PARAM_CHECK
    CpaStatus status = CPA_STATUS_SUCCESS;
    LAC_CHECK_NULL_PARAM(pOpData);
#endif

    if(CPA_FALSE == pOpData->compressAndVerify)
    {
        return cpaDcCompressData(dcInstance,
                                 pSessionHandle,
                                 pSrcBuff,
                                 pDestBuff,
                                 pResults,
                                 pOpData->flushFlag,
                                 callbackTag);
    }

#ifdef ICP_TRACE
    LAC_LOG7("Called with params (0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, "
            "0x%lx, 0x%lx)\n",
            (LAC_ARCH_UINT)dcInstance,
            (LAC_ARCH_UINT)pSessionHandle,
            (LAC_ARCH_UINT)pSrcBuff,
            (LAC_ARCH_UINT)pDestBuff,
            (LAC_ARCH_UINT)pOpData,
            (LAC_ARCH_UINT)pResults,
            (Cpa64U)callbackTag);
#endif

#ifdef ICP_PARAM_CHECK
    status = cnvParamCheck(dcInstance,
                           pSessionHandle,
                           pSrcBuff,
                           pDestBuff,
                           pOpData,
                           pResults);

    if(CPA_STATUS_SUCCESS != status)
    {
        return status;
    }
#endif

    if(CPA_INSTANCE_HANDLE_SINGLE == dcInstance)
    {
         insHandle = dcGetFirstHandle();
    }
    else
    {
         insHandle = dcInstance;
    }

    pService = (sal_compression_service_t*) insHandle;

#ifdef ICP_PARAM_CHECK
    LAC_CHECK_NULL_PARAM(insHandle);
    SAL_CHECK_ADDR_TRANS_SETUP(insHandle);
#endif

    /* Check if SAL is initialised otherwise return an error */
    SAL_RUNNING_CHECK(insHandle);

    /* This check is outside the parameter checking as it is needed to manage
     * zero length requests */
    if(LacBuffDesc_BufferListVerifyNull(pSrcBuff, &srcBuffSize,
            LAC_NO_ALIGNMENT_SHIFT)
               != CPA_STATUS_SUCCESS)
    {
        LAC_INVALID_PARAM_LOG("Invalid source buffer list parameter");
        return CPA_STATUS_INVALID_PARAM;
    }


    pSessionDesc = DC_SESSION_DESC_FROM_CTX_GET(pSessionHandle);

#ifdef ICP_DC_DYN_NOT_SUPPORTED
    if (pSessionDesc->huffType == CPA_DC_HT_FULL_DYNAMIC)
    {
        LAC_INVALID_PARAM_LOG("Invalid huffType value, dynamic sessions "
                              "not supported");
        return CPA_STATUS_INVALID_PARAM;
    }
#endif

    osalAtomicInc(&(pSessionDesc->pendingStatelessCbCount));

    return dcCompDecompData(pService, pSessionDesc, dcInstance,
               pSessionHandle, 0, NULL,
               pSrcBuff, pDestBuff, pResults,
               pOpData->flushFlag, callbackTag, DC_COMPRESSION_REQUEST, CPA_TRUE,
               pOpData->compressAndVerify);
}

CpaStatus
cpaDcDecompressData(CpaInstanceHandle dcInstance,
        CpaDcSessionHandle pSessionHandle,
        CpaBufferList *pSrcBuff,
        CpaBufferList *pDestBuff,
        CpaDcRqResults *pResults,
        CpaDcFlush flushFlag,
        void *callbackTag)
{
    sal_compression_service_t* pService = NULL;
    dc_session_desc_t* pSessionDesc = NULL;
    CpaInstanceHandle insHandle = NULL;
    Cpa64U srcBuffSize = 0;

#ifdef ICP_TRACE
    LAC_LOG7("Called with params (0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, "
                "0x%x, 0x%lx)\n",
                (LAC_ARCH_UINT)dcInstance,
                (LAC_ARCH_UINT)pSessionHandle,
                (LAC_ARCH_UINT)pSrcBuff,
                (LAC_ARCH_UINT)pDestBuff,
                (LAC_ARCH_UINT)pResults,
                flushFlag,
                (Cpa64U)callbackTag);
#endif

    if(CPA_INSTANCE_HANDLE_SINGLE == dcInstance)
    {
         insHandle = dcGetFirstHandle();
    }
    else
    {
         insHandle = dcInstance;
    }

    pService = (sal_compression_service_t*) insHandle;

#ifdef ICP_PARAM_CHECK
    LAC_CHECK_NULL_PARAM(insHandle);
    SAL_CHECK_ADDR_TRANS_SETUP(insHandle);
#endif

    /* Check if SAL is initialised otherwise return an error */
    SAL_RUNNING_CHECK(insHandle);

    /* This check is outside the parameter checking as it is needed to manage
     * zero length requests */
    if(LacBuffDesc_BufferListVerifyNull(pSrcBuff, &srcBuffSize,
            LAC_NO_ALIGNMENT_SHIFT)
               != CPA_STATUS_SUCCESS)
    {
        LAC_INVALID_PARAM_LOG("Invalid source buffer list parameter");
        return CPA_STATUS_INVALID_PARAM;
    }

#ifdef ICP_PARAM_CHECK
    /* Ensure this is a compression instance */
    SAL_CHECK_INSTANCE_TYPE(insHandle, SAL_SERVICE_TYPE_COMPRESSION);

    if (dcCheckSourceData(pSessionHandle, pSrcBuff, pDestBuff,
            pResults, flushFlag, srcBuffSize, NULL) != CPA_STATUS_SUCCESS)
    {
        return CPA_STATUS_INVALID_PARAM;
    }
    if (dcCheckDestinationData(pService, pSessionHandle, pDestBuff,
            NULL, DC_DECOMPRESSION_REQUEST) != CPA_STATUS_SUCCESS)
    {
        return CPA_STATUS_INVALID_PARAM;
    }
#endif

    pSessionDesc = DC_SESSION_DESC_FROM_CTX_GET(pSessionHandle);

#ifdef ICP_PARAM_CHECK
    if(CPA_DC_DIR_COMPRESS == pSessionDesc->sessDirection)
    {
        LAC_INVALID_PARAM_LOG("Invalid sessDirection value");
        return CPA_STATUS_INVALID_PARAM;
    }
#endif

#ifdef ICP_DC_DYN_NOT_SUPPORTED
    if (CPA_DC_HT_FULL_DYNAMIC == pSessionDesc->huffType)
    {
        LAC_INVALID_PARAM_LOG("Invalid huffType value, dynamic sessions "
                              "not supported");
        return CPA_STATUS_INVALID_PARAM;
    }
#endif

    if(CPA_DC_STATEFUL == pSessionDesc->sessState)
    {
        /* Lock the session to check if there are in-flight stateful requests */
         LAC_SPINLOCK(&(pSessionDesc->sessionLock));

        /* Check if there is already one in-flight stateful request */
        if(0 != osalAtomicGet(&(pSessionDesc->pendingStatefulCbCount)))
        {
            LAC_LOG_ERROR("Only one in-flight stateful request supported");
            LAC_SPINUNLOCK(&(pSessionDesc->sessionLock));
            return CPA_STATUS_RETRY;
        }

        if((0 == srcBuffSize) ||
                ((1 == srcBuffSize) &&
                        (CPA_DC_FLUSH_FINAL != flushFlag) &&
                        (CPA_DC_FLUSH_FULL != flushFlag)))
        {
            if(CPA_TRUE == dcZeroLengthRequests(pService, pSessionDesc,
                    pResults, flushFlag, callbackTag, DC_DECOMPRESSION_REQUEST))
            {
                return CPA_STATUS_SUCCESS;
            }
        }

        osalAtomicInc(&(pSessionDesc->pendingStatefulCbCount));
        LAC_SPINUNLOCK(&(pSessionDesc->sessionLock));
    }
    else
    {
        osalAtomicInc(&(pSessionDesc->pendingStatelessCbCount));
    }
    return dcCompDecompData(pService, pSessionDesc, dcInstance,
               pSessionHandle,  0, NULL,
               pSrcBuff, pDestBuff, pResults,
               flushFlag, callbackTag, DC_DECOMPRESSION_REQUEST,
               CPA_TRUE, CPA_FALSE);
}

CpaStatus
cpaDcDecompressData2( CpaInstanceHandle dcInstance,
        CpaDcSessionHandle  pSessionHandle,
        CpaBufferList       *pSrcBuff,
        CpaBufferList       *pDestBuff,
        CpaDcOpData         *pOpData,
        CpaDcRqResults      *pResults,
        void                *callbackTag )
{

#ifdef ICP_PARAM_CHECK
    LAC_CHECK_NULL_PARAM(pOpData);
#endif

    if(CPA_FALSE == pOpData->compressAndVerify)
    {
        return cpaDcDecompressData(dcInstance,
                                   pSessionHandle,
                                   pSrcBuff,
                                   pDestBuff,
                                   pResults,
                                   pOpData->flushFlag,
                                   callbackTag);
    }
    else
    {
        return CPA_STATUS_FAIL;
    }
}

CpaStatus
cpaDcBufferListGetMetaSize(const CpaInstanceHandle instanceHandle,
        Cpa32U numBuffers,
        Cpa32U *pSizeInBytes)
{
#ifdef ICP_PARAM_CHECK
    CpaInstanceHandle insHandle = NULL;

    if(CPA_INSTANCE_HANDLE_SINGLE == instanceHandle)
    {
         insHandle = dcGetFirstHandle();
    }
    else
    {
         insHandle = instanceHandle;
    }

    LAC_CHECK_INSTANCE_HANDLE(insHandle);
    LAC_CHECK_NULL_PARAM(pSizeInBytes);

    /* Ensure this is a compression instance */
    SAL_CHECK_INSTANCE_TYPE(insHandle, SAL_SERVICE_TYPE_COMPRESSION);

    if(0 == numBuffers)
    {
        LAC_INVALID_PARAM_LOG("Number of Buffers");
        return CPA_STATUS_INVALID_PARAM;
    }
#endif

    *pSizeInBytes =  (sizeof(icp_buffer_list_desc_t) +
                     (sizeof(icp_flat_buffer_desc_t) * (numBuffers + 1))
                     + ICP_DESCRIPTOR_ALIGNMENT_BYTES);

#ifdef ICP_TRACE
    LAC_LOG4("Called with params (0x%lx, %d, 0x%lx[%d])\n",
            (LAC_ARCH_UINT)instanceHandle,
            numBuffers,
            (LAC_ARCH_UINT)pSizeInBytes, *pSizeInBytes);
#endif

    return CPA_STATUS_SUCCESS;
}

CpaStatus
cpaDcBnpBufferListGetMetaSize(const CpaInstanceHandle instanceHandle,
        Cpa32U numJobs,
        Cpa32U *pSizeInBytes)
{
#ifdef ICP_PARAM_CHECK
    CpaInstanceHandle insHandle = NULL;

    if(CPA_INSTANCE_HANDLE_SINGLE == instanceHandle)
    {
         insHandle = dcGetFirstHandle();
    }
    else
    {
         insHandle = instanceHandle;
    }

    LAC_CHECK_INSTANCE_HANDLE(insHandle);
    LAC_CHECK_NULL_PARAM(pSizeInBytes);

    /* Ensure this is a compression instance */
    SAL_CHECK_INSTANCE_TYPE(insHandle, SAL_SERVICE_TYPE_COMPRESSION);

    if(0 == numJobs)
    {
        LAC_INVALID_PARAM_LOG("Number of Buffers");
        return CPA_STATUS_INVALID_PARAM;
    }
#endif

    *pSizeInBytes = sizeof(icp_qat_fw_comp_bnp_op_data_t) +
                    sizeof(icp_qat_fw_comp_bnp_out_tbl_entry_t) +
                    sizeof(icp_buffer_list_desc_t) +
                    (sizeof(icp_flat_buffer_desc_t) * (numJobs + 1))
                     + ICP_DESCRIPTOR_ALIGNMENT_BYTES;

#ifdef ICP_TRACE
    LAC_LOG4("Called with params (0x%lx, %d, 0x%lx[%d])\n",
            (LAC_ARCH_UINT)instanceHandle,
            numJobs,
            (LAC_ARCH_UINT)pSizeInBytes, *pSizeInBytes);
#endif

    return CPA_STATUS_SUCCESS;
}
