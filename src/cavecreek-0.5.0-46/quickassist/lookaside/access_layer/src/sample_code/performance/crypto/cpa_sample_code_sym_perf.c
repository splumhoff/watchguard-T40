/***************************************************************************
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
 ***************************************************************************/

/**
 *****************************************************************************
 * @file cpa_sample_code_sym_perf.c
 *
 * @defgroup sampleSymmetricPerf  Symmetric Performance code
 *
 * @ingroup sampleSymmetricPerf
 *
 * @description
 *      This file contains the main symmetric performance sample code. It is
 *      capable of performing all ciphers, all hashes, authenticated hashes
 *      and algorithm chaining. Nested Hashes are not supported
 *
 *      This code pre-allocates a number of bufferLists as defined by
 *      setup->numBuffLists, each bufferlist includes several flat buffers which
 *      its size is equal to buffer size. The pre-allocated buffers are then
 *      continuously looped until the numLoops is met.
 *      Time stamping is started prior to the
 *      Operation and is stopped when all callbacks have returned.
 *      The packet size and algorithm to be tested is setup using the
 *      setupSymmetricTest function. The framework is used to create the threads
 *      which calls functions here to execute symmetric performance
 *
 *****************************************************************************/

#include "cpa_sample_code_crypto_utils.h"
#include "cpa_sample_code_framework.h"
#include "icp_sal_poll.h"

#define EVEN_NUMBER (2)

extern int signOfLife;
extern Cpa32U symPollingInterval_g;




/**
 *****************************************************************************
 * @ingroup sampleSymmetricPerf
 *
 * @description
 *      Callback function for result of perform operation
 *
 *****************************************************************************/
void symPerformCallback(
        void *pCallbackTag,
        CpaStatus status,
        const CpaCySymOp operationType,
        void *pOpData,
        CpaBufferList *pDstBuffer,
        CpaBoolean verifyResult)
{
    /*we declare the callback as per the API requirements, but we only use
     * the pCallbackTag parameter*/
    processCallback(pCallbackTag);
}

/**
 *****************************************************************************
 * @ingroup sampleSymmetricPerf
 *
 * @description
 *      calculate the pointer of digest result in the buffer list
 *      digest result should be located in the end of Plaintext and
 *      digest result should be align with block size of cipher.
 *      Please see example as the following:
@verbatim
+--------+----------------------------------------------------+--------+----+
|        |                   Ciphertext                       | Digest |Pad +
+--------+----------------------------------------------------+--------+----+
         <-FlatBuffer[0]-><-FlatBuffer[1]-><-FlatBuffer[2]-><-FlatBuffer[3]->
         <-                               Buffer List                      ->
@endverbatim
 * @param[in] packetSize       Data packet size
 * @param[in] blockSizeInBytes block length of the cipher
 * @param[in] bufferSizeInByte buffer size in the flatbuffer of bufferlist
 * @param[in] pBufferList      the pointer of Buffer list which store data and
 *                             comprised of flatbuffers.
 *
 *****************************************************************************/
static Cpa8U * symCalDigestAddress(
        Cpa32U packetSize,
        Cpa32U blockSizeInBytes,
        Cpa32U bufferSizeInByte,
        CpaBufferList *pBufferList)
{
      Cpa32U packsetSizePad = 0;
      Cpa32U digestOffset = 0;
      Cpa8U * pDigestResult=0;
      Cpa32U indexBuffer = 0;
      /* check if  packetSize is 0  */
      if (bufferSizeInByte == 0)
      {
         pDigestResult = pBufferList->pBuffers[0].pData + packetSize;
      }
      else
      {
        /* since Digest address (pDigestResult) need to align with
        * blockSizeInBytes, we will check if packetSize is align with
        * blockSizeInBytes,
        * if not, padding will added after message */
        if(packetSize%blockSizeInBytes != 0)
        {
           packsetSizePad = blockSizeInBytes - (packetSize%blockSizeInBytes);
        }
        /* calculate actual offset of digest result in flatbuffer*/
        digestOffset = (packetSize + packsetSizePad)% bufferSizeInByte;

        /* calculate the which flat buffer store pDigestResult
           * pDigestResult will appended in the end of pData */
        indexBuffer = (packetSize + packsetSizePad )/bufferSizeInByte;
        pDigestResult = pBufferList->pBuffers[indexBuffer].pData
                      +digestOffset;
      }
      return pDigestResult;
}
/**
 *****************************************************************************
 * @ingroup sampleSymmetricPerf
 *
 * @description
 *      initialize the digest within pBufferList by value.
 *
 * @param[in] messageLenToCipherInBytes  Cipher Message Length
 * @param[in] digestLengthInBytes        Digest Length
 * @param[in] value                      Initial value for Digest Buffer
 * @param[in] pBufferList                the pointer of Buffer list which store
 *                                       data and comprised of flatbuffers.
 *****************************************************************************/
static void symSetDigestBuffer(
        Cpa32U messageLenToCipherInBytes,
        Cpa32U digestLengthInBytes,
        Cpa8U value,
        CpaBufferList *pBufferList)
{
      Cpa8U * pDigestResult = 0;
      Cpa32U indexBuffer = 0;
      Cpa32U i = 0;
      /*  all the rest of data including padding will initialized ,
      * so ivLenInBytes is 1.*/
      pDigestResult = symCalDigestAddress(
         messageLenToCipherInBytes,
         1,
         pBufferList->pBuffers[0].dataLenInBytes,
         pBufferList);
      indexBuffer =  messageLenToCipherInBytes/
                     pBufferList->pBuffers[0].dataLenInBytes;
       /* reset the digest memory to 0 */
       memset(pDigestResult,
           value,
           (pBufferList->pBuffers[0].dataLenInBytes -
            messageLenToCipherInBytes % pBufferList->pBuffers[0].dataLenInBytes)
           );
      indexBuffer ++;
      for (i = indexBuffer ; i< pBufferList->numBuffers; i++)
      {
           memset(pBufferList->pBuffers[i].pData,
              value,
              pBufferList->pBuffers[i].dataLenInBytes);
      }
}
/**
 *****************************************************************************
 * @ingroup sampleSymmetricPerf
 *
 * @description
 * Create a symmetric session
 */
static CpaStatus symmetricSetupSession(
        CpaCySymCbFunc pSymCb,
        Cpa8U *pCipherKey,
        Cpa8U *pAuthKey,
        CpaCySymSessionCtx *pSession,
        symmetric_test_params_t *setup
)
{
    Cpa32U sessionCtxSizeInBytes = 0;
    CpaStatus status = CPA_STATUS_SUCCESS;
    CpaCySymSessionCtx pLocalSession = NULL;
    Cpa32U cipherKeyLen = 0;
    Cpa32U authKeyLen = 0;
    Cpa32U node = 0;

    status = sampleCodeCyGetNode(setup->cyInstanceHandle, &node);
    if(CPA_STATUS_SUCCESS != status)
    {
        PRINT_ERR("sampleCodeCyGetNode error, status: %d\n", status);
        return status;
    }
    /*set the cipher and authentication key len*/
    cipherKeyLen = setup->setupData.cipherSetupData.cipherKeyLenInBytes;
    authKeyLen =
        setup->setupData.hashSetupData.authModeSetupData.authKeyLenInBytes;
    /*generate a random cipher and authentication key*/
    generateRandomData(pCipherKey,cipherKeyLen);
    generateRandomData(pAuthKey,authKeyLen);
    /*cipher setup only needs to be set for alg chaining, cipher, AES-GCM
     * and AES-CCM*/
    setup->setupData.cipherSetupData.pCipherKey = pCipherKey;
    /*hash setup only needs to be set for hash, AES-GCM
     * and AES-CCM*/
    setup->setupData.hashSetupData.authModeSetupData.authKey = pAuthKey;
    if(CPA_CY_SYM_HASH_AES_GMAC == setup->setupData.hashSetupData.hashAlgorithm)
    {
        setup->setupData.hashSetupData.authModeSetupData.authKey = NULL;
        setup->setupData.hashSetupData.authModeSetupData.authKeyLenInBytes = 0;
    }
    else if(CPA_CY_SYM_HASH_SNOW3G_UIA2 ==
        setup->setupData.hashSetupData.hashAlgorithm) {
        setup->setupData.hashSetupData.authModeSetupData.aadLenInBytes =
            KEY_SIZE_128_IN_BYTES;
    }
    else
    {
        setup->setupData.hashSetupData.authModeSetupData.aadLenInBytes = 0;
    }
    /* will not verify digest by default*/
    setup->setupData.verifyDigest = CPA_FALSE;

    /*get size for mem allocation*/
    status = cpaCySymSessionCtxGetSize( setup->cyInstanceHandle,
            &setup->setupData, &sessionCtxSizeInBytes);
    if(status != CPA_STATUS_SUCCESS)
    {
        PRINT_ERR("cpaCySymSessionCtxGetSize error, status: %d", status);
        return status;
    }
    /*
     * allocate session memory
     */
    pLocalSession = qaeMemAllocNUMA(sessionCtxSizeInBytes,
            node, BYTE_ALIGNMENT_64);
    if(NULL == pLocalSession)
    {
        PRINT_ERR("Could not allocate pLocalSession memory\n");
        return CPA_STATUS_FAIL;
    }
    /*zero session memory*/
    memset(pLocalSession, 0, sessionCtxSizeInBytes);
    /*
     * init session with asynchronous callback- pLocalSession will contain
     * the session context
     */
    status = cpaCySymInitSession(setup->cyInstanceHandle, pSymCb,
            &setup->setupData, pLocalSession);
    if(CPA_STATUS_SUCCESS != status)
    {
        PRINT_ERR("cpaCySymInitSession error, status: %d\n", status);
        qaeMemFreeNUMA((void**)&pLocalSession);
        return status;
    }
    *pSession = pLocalSession;
    return status;
}

/*****************************************************************************
 * @ingroup sampleSymmetricPerf
 *
 * @description
 * Free memory allocated in the symmetricPerformOpDataSetup function
 * ****************************************************************************/
void opDataMemFree(CpaCySymOpData *pOpdata[], Cpa32U numBuffers,
                        CpaBoolean digestAppend)
{
    Cpa32U k = 0;

    for(k = 0; k < numBuffers; k++)
    {
        if(NULL != pOpdata[k])
        {
            qaeMemFreeNUMA((void**)&pOpdata[k]->pIv);
            if(NULL != pOpdata[k]->pAdditionalAuthData) {
                qaeMemFreeNUMA((void**)&pOpdata[k]->pAdditionalAuthData);
            }
            if(CPA_FALSE == digestAppend)
            {
                if(NULL != pOpdata[k]->pDigestResult)
                {
                    qaeMemFreeNUMA((void**)&pOpdata[k]->pDigestResult);
                }
            }
            qaeMemFreeNUMA((void**)&pOpdata[k]);
        }
    }
}

/*****************************************************************************
 * @ingroup sampleSymmetricPerf
 *
 * @description
 * Setup symmetric operation data
 * ****************************************************************************/
static CpaStatus symmetricPerformOpDataSetup(
        CpaCySymSessionCtx pSessionCtx,
        Cpa32U *pPacketSize,
        CpaCySymOpData *pOpdata[],
        symmetric_test_params_t *setup,
        CpaBufferList *pBuffListArray[])
{
    CpaStatus status = CPA_STATUS_SUCCESS;
    Cpa32U createCount = 0;
    Cpa32U node = 0;

    /*get the node we are running on for local memory allocation*/
    status = sampleCodeCyGetNode(setup->cyInstanceHandle, &node);
    if(CPA_STATUS_SUCCESS != status)
    {
        PRINT_ERR("sampleCodeCyGetNode error, status: %d\n", status);
        return status;
    }
    /*for each bufferList set the symmetric operation data*/
    for (createCount = 0; createCount < setup->numBuffLists; createCount++)
    {
        pOpdata[createCount]=qaeMemAllocNUMA(sizeof(CpaCySymOpData),
                node,BYTE_ALIGNMENT_64);
        if(pOpdata[createCount] == NULL)
        {
            PRINT_ERR("Could not allocte Opdata memory at index %u\n",
                    createCount);
            opDataMemFree(pOpdata, setup->numBuffLists,CPA_FALSE);
            return CPA_STATUS_FAIL;
        }
        memset(pOpdata[createCount], 0, sizeof(CpaCySymOpData));
        pOpdata[createCount]->sessionCtx = pSessionCtx;
        pOpdata[createCount]->packetType = CPA_CY_SYM_PACKET_TYPE_FULL;
        /*these only need to be set for cipher and alg chaining */
        pOpdata[createCount]->cryptoStartSrcOffsetInBytes =
            setup->cryptoSrcOffset;
        /* messageLenToCipherInBytes and messageLenToHashInBytes do not have
         * to be the same. In this code we want to either hash the entire buffer
         * or encrypt the entire buffer, depending on the SymOperation.
         * For Alg Chaining, depending on the chain order, for HashThenCipher,
         * the digest will be the hash of the unencrypted buffer and then we
         * cipher  the buffer. OR for CipherThenHash, we cipher the buffer, then
         * the perform the hash on the encrypted buffer, so that the digest is
         * the digest of the encrypted data*/
        pOpdata[createCount]->messageLenToCipherInBytes =
            pPacketSize[createCount]-setup->cryptoSrcOffset;
        /*these only need to be set for hash and alg chaining*/
        pOpdata[createCount]->hashStartSrcOffsetInBytes = HASH_OFFSET_BYTES;
        pOpdata[createCount]->messageLenToHashInBytes =
            pPacketSize[createCount];

        pOpdata[createCount]->pAdditionalAuthData = NULL;

        /* In GMAC mode, there is no message to Cipher */
        if(CPA_CY_SYM_HASH_AES_GMAC ==
            setup->setupData.hashSetupData.hashAlgorithm)
        {
            pOpdata[createCount]->cryptoStartSrcOffsetInBytes = 0;
            pOpdata[createCount]->messageLenToCipherInBytes = 0;
        }

        if(CPA_CY_SYM_HASH_SNOW3G_UIA2 ==
            setup->setupData.hashSetupData.hashAlgorithm) {

            pOpdata[createCount]->pAdditionalAuthData =
                qaeMemAllocNUMA(KEY_SIZE_128_IN_BYTES ,node,BYTE_ALIGNMENT_64);
            if(NULL== pOpdata[createCount]->pAdditionalAuthData)
            {
                PRINT_ERR("Could not allocate additional auth data index %u\n",
                        createCount);
                opDataMemFree(pOpdata, setup->numBuffLists,CPA_FALSE);
                return CPA_STATUS_FAIL;
            }
            memset(pOpdata[createCount]->pAdditionalAuthData,
                    0xAA, KEY_SIZE_128_IN_BYTES);
        }
        else if(((setup->setupData.cipherSetupData.cipherAlgorithm ==
                    CPA_CY_SYM_CIPHER_AES_CCM) ||
                (setup->setupData.cipherSetupData.cipherAlgorithm ==
                    CPA_CY_SYM_CIPHER_AES_GCM)) &&
                (setup->setupData.hashSetupData.hashAlgorithm !=
                    CPA_CY_SYM_HASH_AES_GMAC))
        {
            /*must allocate to the nearest block size required
              (above 18 bytes)*/
            pOpdata[createCount]->pAdditionalAuthData =
                qaeMemAllocNUMA(AES_CCM_MIN_AAD_ALLOC_LENGTH,
                                node,BYTE_ALIGNMENT_64);
            if(NULL== pOpdata[createCount]->pAdditionalAuthData)
            {
                PRINT_ERR("Could not allocate additional auth data index %u\n",
                        createCount);
                opDataMemFree(pOpdata, setup->numBuffLists,CPA_FALSE);
                return CPA_STATUS_FAIL;
            }
            memset(pOpdata[createCount]->pAdditionalAuthData,
                    0, AES_CCM_MIN_AAD_ALLOC_LENGTH);
        }
        /*set IV len depending on what we are testing*/
        switch(setup->setupData.cipherSetupData.cipherAlgorithm)
        {
            case CPA_CY_SYM_CIPHER_AES_CBC:
            case CPA_CY_SYM_CIPHER_AES_CTR:
            case CPA_CY_SYM_CIPHER_AES_CCM:
            case CPA_CY_SYM_CIPHER_SNOW3G_UEA2:
            case CPA_CY_SYM_CIPHER_AES_F8:
                pOpdata[createCount]->ivLenInBytes =
                        IV_LEN_FOR_16_BYTE_BLOCK_CIPHER;
                break;

            case CPA_CY_SYM_CIPHER_DES_CBC:
            case CPA_CY_SYM_CIPHER_3DES_CBC:
            case CPA_CY_SYM_CIPHER_3DES_CTR:
            case CPA_CY_SYM_CIPHER_KASUMI_F8:
                pOpdata[createCount]->ivLenInBytes =
                        IV_LEN_FOR_8_BYTE_BLOCK_CIPHER;
                break;
            case CPA_CY_SYM_CIPHER_AES_GCM:
                pOpdata[createCount]->ivLenInBytes = IV_LEN_FOR_12_BYTE_GCM;
                break;
            default:
                pOpdata[createCount]->ivLenInBytes =
                        IV_LEN_FOR_8_BYTE_BLOCK_CIPHER;
                break;
        }

        /*allocate NUMA aware aligned memory for IV*/
        pOpdata[createCount]->pIv = qaeMemAllocNUMA(
                pOpdata[createCount]->ivLenInBytes, node, BYTE_ALIGNMENT_64);
        if ( NULL == pOpdata[createCount]->pIv )
        {
            PRINT_ERR("IV is null\n");
            opDataMemFree(pOpdata, setup->numBuffLists,CPA_FALSE);
            return CPA_STATUS_FAIL;
        }
        memset(pOpdata[createCount]->pIv, 0,
                pOpdata[createCount]->ivLenInBytes);
        if(setup->setupData.cipherSetupData.cipherAlgorithm ==
                    CPA_CY_SYM_CIPHER_AES_CCM)
        {
            /*Although the IV data length for CCM must be 16 bytes,
              The nonce length must be between 7 and 13 inclusive*/
            pOpdata[createCount]->ivLenInBytes = AES_CCM_DEFAULT_NONCE_LENGTH;
        }

        /*if we are testing HASH or Alg Chaining, set the location to place
         * the digest result, this space was allocated in sampleSymmetricPerform
         * function*/
        if(setup->setupData.symOperation==CPA_CY_SYM_OP_HASH ||
                setup->setupData.symOperation==CPA_CY_SYM_OP_ALGORITHM_CHAINING)
        {
            if(CPA_TRUE == setup->digestAppend)
            {
            /* calculate digest offset */
            pOpdata[createCount]->pDigestResult = symCalDigestAddress(
                  pPacketSize[createCount],
                  IV_LEN_FOR_16_BYTE_BLOCK_CIPHER,
                  setup->flatBufferSizeInBytes,
                  pBuffListArray[createCount]);
            }
            else
            {
                pOpdata[createCount]->pDigestResult =
                        qaeMemAllocNUMA(
                          setup->setupData.hashSetupData.digestResultLenInBytes,
                          node,BYTE_ALIGNMENT_64);
                if(NULL == pOpdata[createCount]->pDigestResult)
                {
                    PRINT_ERR("Cannot allocate memory for pDigestResult\n");
                    return CPA_STATUS_FAIL;
                }
            }
        }

        if(setup->setupData.cipherSetupData.cipherAlgorithm ==
                    CPA_CY_SYM_CIPHER_AES_CCM)
        {
            /*generate a random IV*/
            generateRandomData(&(pOpdata[createCount]->pIv[1]),
                    pOpdata[createCount]->ivLenInBytes);

            memcpy(&(pOpdata[createCount]->pAdditionalAuthData[1]),
                    &(pOpdata[createCount]->pIv[1]),
                    pOpdata[createCount]->ivLenInBytes);

        }
        else
        {
            /*generate a random IV*/
            generateRandomData(pOpdata[createCount]->pIv,
                    pOpdata[createCount]->ivLenInBytes);
        }
    }
    return CPA_STATUS_SUCCESS;
}


/*****************************************************************************
 * @ingroup sampleSymmetricPerf
 *
 * @description
 * measures the performance of symmetric encryption operations
 * ****************************************************************************/
CpaStatus symPerform(symmetric_test_params_t* setup,
        perf_data_t *pSymData,
        Cpa32U numOfLoops,
        CpaCySymOpData **ppOpData,
        CpaBufferList **ppSrcBuffListArray,
        CpaCySymCipherDirection cipherDirection
)
{
    CpaBoolean verifyResult = CPA_FALSE;
    CpaStatus status = CPA_STATUS_SUCCESS;
    Cpa32U outsideLoopCount = 0;
    Cpa32U insideLoopCount = 0;
    CpaInstanceInfo2 instanceInfo2 = {0};

#ifdef LATENCY_CODE
    Cpa32U submissions = 0;
    Cpa32U i =0;
    perf_cycles_t request_submit_start[100] = {0};
    perf_cycles_t request_respnse_time[100] = {0};
#endif


    memset(pSymData, 0, sizeof(perf_data_t));
    status = cpaCyInstanceGetInfo2(setup->cyInstanceHandle, &instanceInfo2);
    if(CPA_STATUS_SUCCESS != status)
    {
        PRINT_ERR("cpaCyInstanceGetInfo2 error, status: %d\n", status);
        return CPA_STATUS_FAIL;
    }

#ifdef LATENCY_CODE
    if(pSymData->numOperations > LATENCY_SUBMISSION_LIMIT)
    {
        PRINT_ERR("Error max submissions for latency  must be <= %d\n",
                LATENCY_SUBMISSION_LIMIT);
        return CPA_STATUS_FAIL;
    }
    pSymData->nextCount = (setup->numBuffLists*setup->numLoops)/100;
    pSymData->countIncrement = (setup->numBuffLists*setup->numLoops)/100;

    pSymData->response_times=request_respnse_time;
#endif
    /*preset the number of ops we plan to submit*/
    pSymData->numOperations = (Cpa64U)setup->numBuffLists*setup->numLoops;

    pSymData->retries = 0;

    /* Init the semaphore used in the callback */
    sampleCodeSemaphoreInit(&pSymData->comp, 0);

    /*this barrier will wait until all threads get to this point*/
    sampleCodeBarrier();
    /* Get the time, collect this only for the first
     * request, the callback collects it for the last */
    pSymData->startCyclesTimestamp=sampleCodeTimestamp();

    /* Need to reset the digest memory to 0 if it's AES GCM Cipher Algorithm */
    if(setup->setupData.cipherSetupData.cipherAlgorithm ==
                    CPA_CY_SYM_CIPHER_AES_CCM)
    {
        for( outsideLoopCount = 0;
        outsideLoopCount < setup->numBuffLists;
        outsideLoopCount++)
        {

            if(cipherDirection != CPA_CY_SYM_CIPHER_DIRECTION_DECRYPT)
            {
                if(CPA_FALSE == setup->digestAppend)
                {
                    memset(ppOpData[outsideLoopCount]->pDigestResult,
                            0,
                            AES_CCM_DIGEST_LENGTH_IN_BYTES);
                }
                else
                {
                    /* reset the digest memory to 0 */
                    symSetDigestBuffer(ppOpData[outsideLoopCount]->
                            messageLenToCipherInBytes,
                            AES_CCM_DIGEST_LENGTH_IN_BYTES,
                            0,
                            ppSrcBuffListArray[outsideLoopCount]);
                }
            }
            AVOID_SOFTLOCKUP;
        }
    }
    /* The outside for-loop will loop around the preallocated buffer list
     * array the number of times necessary to satisfy:
     * NUM_OPERATIONS / setup->numBuffLists*/
    for( outsideLoopCount = 0;
    outsideLoopCount < numOfLoops;
    outsideLoopCount++)
    {
        /* This inner for-loop loops around the number of Buffer Lists
         * that have been preallocated.  Once the array has completed-
         * exit to the outer loop to move on the next iteration of the
         * preallocated loop. */
        for (insideLoopCount = 0;
        insideLoopCount < setup->numBuffLists;
        insideLoopCount++)
        {
            /* When the callback returns it will increment the responses
             * counter and test if its equal to NUM_OPERATIONS, in that
             * case all responses have been successfully received. */
            do {
#ifdef LATENCY_CODE
                if(submissions+1 == pSymData->nextCount)
                {
                        request_submit_start[pSymData->latencyCount] =
                            sampleCodeTimestamp();
                }
#endif
                status = cpaCySymPerformOp (
                        setup->cyInstanceHandle,
                        pSymData,
                        ppOpData[insideLoopCount],
                        ppSrcBuffListArray[insideLoopCount],
                        ppSrcBuffListArray[insideLoopCount],
                        /*in-place operation*/
                        &verifyResult);
                if(status == CPA_STATUS_RETRY)
                {
                    setup->performanceStats->retries++;
                    AVOID_SOFTLOCKUP;
                }
            } while (CPA_STATUS_RETRY == status);
            if (CPA_STATUS_SUCCESS != status)
            {
                break;
            }
#ifdef LATENCY_CODE
           submissions++;
#endif
        } /*end of inner loop */
        if (CPA_STATUS_SUCCESS != status)
        {
            PRINT_ERR("cpaCySymPerformOp Error %d\n", status);
            break;
        }
    } /* end of outer loop */

    if (CPA_STATUS_SUCCESS == status)
    {
        status = waitForResponses(pSymData, setup->syncMode,setup->numBuffLists,
                numOfLoops);
    }

#ifdef LATENCY_CODE
    for(i=0; i<pSymData->latencyCount;i++)
    {
        pSymData->aveLatency +=
            pSymData->response_times[i] - request_submit_start[i];
    }
    /*we are finished with the response time so set to null before exit*/
    pSymData->response_times = NULL;
    if(pSymData->latencyCount>0)
    {
        do_div(pSymData->aveLatency,pSymData->latencyCount);
    }
#endif

    /*clean up the callback semaphore*/
    sampleCodeSemaphoreDestroy(&pSymData->comp);
    return status;
}

/*****************************************************************************
 * @ingroup sampleSymmetricPerf
 *
 * @description
 * Free memory allocated in the sampleSymmetricPerform function
 * ****************************************************************************/
void symPerformMemFree(symmetric_test_params_t* setup,
        CpaFlatBuffer **ppSrcBuffPtrArray,
        CpaBufferList **ppSrcBuffListArray,
        CpaCySymOpData **ppOpData,
        CpaCySymSessionCtx* pSessionCtx)
{
    /*free bufferLists, flatBuffers and data*/
    sampleFreeBuffers( ppSrcBuffPtrArray, ppSrcBuffListArray, setup);
    if(NULL != ppOpData)
    {
        opDataMemFree(ppOpData, setup->numBuffLists, setup->digestAppend);
    }
    /* free the session memory - calling code is responsible for
     * removing the session first*/
    if(NULL != *pSessionCtx)
    {
        qaeMemFreeNUMA((void**)pSessionCtx);
    }
    qaeMemFreeNUMA((void**)&ppOpData);
    qaeMemFreeNUMA((void**)&ppSrcBuffPtrArray);
    qaeMemFreeNUMA((void**)&ppSrcBuffListArray);

}


/**
 *****************************************************************************
 * @ingroup sampleSymmetricPerf
 *
 * @description
 *  Main executing function
 ******************************************************************************/
CpaStatus sampleSymmetricPerform( symmetric_test_params_t* setup)
{
    /* start of local variable declarations */
    CpaCySymSessionCtx pEncryptSessionCtx = NULL;
    CpaCySymOpData **ppOpData = NULL;
    CpaFlatBuffer **ppSrcBuffPtrArray = NULL;
    CpaBufferList **ppSrcBuffListArray = NULL;
    Cpa32U *totalSizeInBytes = NULL;
    perf_data_t *pSymPerfData = NULL;
    CpaStatus status = CPA_STATUS_SUCCESS;
    Cpa32U numOfLoops = setup->numLoops;
    Cpa32U insideLoopCount = 0;
    Cpa8U cipherKey[setup->setupData.cipherSetupData.cipherKeyLenInBytes];
    Cpa8U authKey[setup->setupData.hashSetupData.authModeSetupData.
                  authKeyLenInBytes];
    CpaCySymCbFunc pSymCb = NULL;
    CpaInstanceInfo2 instanceInfo2 = {0};
    Cpa32U node = 0;
    CpaCySymCipherDirection cipherDirection =
                                CPA_CY_SYM_CIPHER_DIRECTION_ENCRYPT;

    /*get the node we are running on for local memory allocation*/
    status = sampleCodeCyGetNode(setup->cyInstanceHandle, &node);
    if(CPA_STATUS_SUCCESS != status)
    {
        PRINT_ERR("sampleCodeCyGetNode error, status: %d\n", status);
        return CPA_STATUS_FAIL;
    }
    totalSizeInBytes = qaeMemAlloc(setup->numBuffLists * sizeof(Cpa32U));
    if(NULL == totalSizeInBytes)
    {
        PRINT_ERR("sampleCodeCyGetNode memory allocation error\n");
        return CPA_STATUS_FAIL;
    }


    /*allocate memory for an array of bufferList pointers, flatBuffer pointers
     * and operation data, the bufferLists and Flat buffers are created in
     * sampleCreateBuffers of cpa_sample_code_crypto_utils.c*/
    status = allocArrayOfPointers(
            setup->cyInstanceHandle,
            (void**)&ppOpData,
            setup->numBuffLists);
    if(CPA_STATUS_SUCCESS != status)
    {
        symPerformMemFree(setup, ppSrcBuffPtrArray, ppSrcBuffListArray,
                ppOpData, &pEncryptSessionCtx);
        qaeMemFree((void**)&totalSizeInBytes);
        return CPA_STATUS_FAIL;
    }

    status = allocArrayOfPointers(
            setup->cyInstanceHandle,
            (void**)&ppSrcBuffPtrArray,
            setup->numBuffLists);
    if(CPA_STATUS_SUCCESS != status)
    {
        symPerformMemFree(setup, ppSrcBuffPtrArray, ppSrcBuffListArray,
                ppOpData, &pEncryptSessionCtx);
        qaeMemFree((void**)&totalSizeInBytes);
        return CPA_STATUS_FAIL;
    }
    status = allocArrayOfPointers(
            setup->cyInstanceHandle,
            (void**)&ppSrcBuffListArray,
            setup->numBuffLists);
    if(CPA_STATUS_SUCCESS != status)
    {
        symPerformMemFree(setup, ppSrcBuffPtrArray, ppSrcBuffListArray,
                ppOpData, &pEncryptSessionCtx);
        qaeMemFree((void**)&totalSizeInBytes);
        return CPA_STATUS_FAIL;
    }
/*use the preallocated performance stats to store performance data, this
 * points to an element in perfStats array in the framework, each thread
 * points to a unique element of perfStats array*/


    pSymPerfData = setup->performanceStats;
    if(NULL == pSymPerfData)
    {
        PRINT_ERR("perf data pointer is NULL\n");
        symPerformMemFree(setup, ppSrcBuffPtrArray, ppSrcBuffListArray,
                ppOpData, &pEncryptSessionCtx);
        qaeMemFree((void**)&totalSizeInBytes);
        return CPA_STATUS_FAIL;
    }
    memset(pSymPerfData, 0, sizeof(perf_data_t));
    if(setup->setupData.symOperation == CPA_CY_SYM_OP_HASH &&
            (setup->setupData.hashSetupData.hashAlgorithm ==
                CPA_CY_SYM_HASH_SNOW3G_UIA2 ||
                setup->setupData.hashSetupData.hashAlgorithm ==
                    CPA_CY_SYM_HASH_KASUMI_F9))
    {
        setup->setupData.hashSetupData.digestResultLenInBytes =
            DIGEST_RESULT_4BYTES;

    }

    /*if we are testing hash or alg chain, get the hash size that needs to be
     *  allocated for the digest result. sampleCreateBuffers uses the hash size
     *  to allocate the appropriate memory*/
    for (insideLoopCount = 0;
    insideLoopCount < setup->numBuffLists;
    insideLoopCount++)
    {
        if(CPA_FALSE == setup->digestAppend)
        {
            totalSizeInBytes[insideLoopCount]=
                                setup->packetSizeInBytesArray[insideLoopCount];

        }
        else
        {
        /* needs to be allocated for the digest result. */
        totalSizeInBytes[insideLoopCount]=
            setup->packetSizeInBytesArray[insideLoopCount] +
            setup->setupData.hashSetupData.digestResultLenInBytes;
        }

    }
    status = cpaCyInstanceGetInfo2(setup->cyInstanceHandle, &instanceInfo2);
    if(status != CPA_STATUS_SUCCESS)
    {
        PRINT_ERR("cpaCyInstanceGetInfo2 error, status %d\n", status);
        symPerformMemFree(setup, ppSrcBuffPtrArray, ppSrcBuffListArray,
                ppOpData, &pEncryptSessionCtx);
        qaeMemFree((void**)&totalSizeInBytes);
        return status;
    }

    /*init the symmetric session*/
    /*if the mode is asynchronous then set the callback function*/
    if(ASYNC == setup->syncMode)
    {
        pSymCb = symPerformCallback;
    }
    status = symmetricSetupSession(pSymCb,
            cipherKey,
            authKey,
            &pEncryptSessionCtx,
            setup
    );

    if (CPA_STATUS_SUCCESS != status)
    {
        PRINT_ERR("symmetricSetupSession error, status %d\n", status);
        symPerformMemFree(setup, ppSrcBuffPtrArray, ppSrcBuffListArray,
                ppOpData, &pEncryptSessionCtx);
        qaeMemFree((void**)&totalSizeInBytes);
        return CPA_STATUS_FAIL;
    }



    /* we create sample buffers with space for digest result if testing hash or
     * alg chain , otherwise we just create sample buffers
     * based on the bufferSize we are testing*/
    status = sampleCreateBuffers( setup->cyInstanceHandle,
            totalSizeInBytes, ppSrcBuffPtrArray, ppSrcBuffListArray,
            setup);
    if (CPA_STATUS_SUCCESS != status)
    {
        PRINT_ERR("sampleCreateBuffers error, status %d\n", status);
        symPerformMemFree(setup, ppSrcBuffPtrArray, ppSrcBuffListArray,
                ppOpData, &pEncryptSessionCtx);
    }


    /*setup the symmetric operation data*/
    status = symmetricPerformOpDataSetup(
            pEncryptSessionCtx,
            setup->packetSizeInBytesArray,
            ppOpData,
            setup,
            ppSrcBuffListArray);

    if (CPA_STATUS_SUCCESS != status)
    {
        PRINT_ERR("symmetricPerformOpDataSetup error, status %d\n", status);
        symPerformMemFree(setup, ppSrcBuffPtrArray, ppSrcBuffListArray,
                ppOpData, &pEncryptSessionCtx);
        qaeMemFree((void**)&totalSizeInBytes);
        return status;
    }


        status = symPerform(setup, pSymPerfData, numOfLoops, ppOpData,
                 ppSrcBuffListArray, cipherDirection);
        if (CPA_STATUS_SUCCESS != status)
        {
            PRINT_ERR("symPerform error, status %d\n", status);
            symPerformMemFree(setup, ppSrcBuffPtrArray, ppSrcBuffListArray,
                    ppOpData, &pEncryptSessionCtx);
            qaeMemFree((void**)&totalSizeInBytes);
            return status;
        }

    /* Free up resources allocated */
    if (CPA_STATUS_SUCCESS != cpaCySymRemoveSession(
            setup->cyInstanceHandle, pEncryptSessionCtx))
    {
        PRINT_ERR("Deregister session failed\n");
        status = CPA_STATUS_FAIL;
    }
    symPerformMemFree(setup, ppSrcBuffPtrArray, ppSrcBuffListArray,
            ppOpData, &pEncryptSessionCtx);

    qaeMemFree((void**)&totalSizeInBytes);
    if(CPA_STATUS_SUCCESS != setup->performanceStats->threadReturnStatus)
    {
        status = CPA_STATUS_FAIL;
    }
    return status;
}

/**
 *****************************************************************************
 * @ingroup sampleSymmetricPerf
 *
 * @description
 *  Setup a symmetric crypto thread for a given packet size or mix
 ******************************************************************************/
void sampleSymmetricPerformance(single_thread_test_data_t* testSetup)
{
    symmetric_test_params_t symTestSetup;
    symmetric_test_params_t* pSetup =
        ((symmetric_test_params_t*)testSetup->setupPtr);
    Cpa32U loopIteration = 0;
    CpaStatus status = CPA_STATUS_SUCCESS;
    /*define the distribution of the packet mix
     * here we defined 2 lots of 10 sizes
     * later it is replicated into 100 buffers*/
    Cpa32U packetMix[NUM_PACKETS_IMIX] =
    {BUFFER_SIZE_64,  BUFFER_SIZE_752,
            BUFFER_SIZE_1504, BUFFER_SIZE_64,
            BUFFER_SIZE_752, BUFFER_SIZE_1504,
            BUFFER_SIZE_64, BUFFER_SIZE_64,
            BUFFER_SIZE_1504, BUFFER_SIZE_1504,
            BUFFER_SIZE_752, BUFFER_SIZE_64,
            BUFFER_SIZE_752, BUFFER_SIZE_64,
            BUFFER_SIZE_1504, BUFFER_SIZE_1504,
            BUFFER_SIZE_64, BUFFER_SIZE_8992,
            BUFFER_SIZE_64, BUFFER_SIZE_1504};
    Cpa32U *pPacketSize;
    Cpa16U numInstances = 0;
    CpaInstanceHandle *cyInstances = NULL;

    memset(&symTestSetup, 0, sizeof(symmetric_test_params_t));

    /*this barrier is to halt this thread when run in user space context, the
     * startThreads function releases this barrier, in kernel space it does
     * nothing, but kernel space threads do not start until we call startThreads
     * anyway*/
    startBarrier();
    /*give our thread a unique memory location to store performance stats*/
    symTestSetup.performanceStats = testSetup->performanceStats;
    /*get the instance handles so that we can start our thread on the selected
     * instance*/
    status = cpaCyGetNumInstances(&numInstances);
    if( CPA_STATUS_SUCCESS != status  || numInstances == 0)
    {
        PRINT_ERR("cpaCyGetNumInstances error, status:%d, numInstanaces:%d\n",
                status, numInstances);
        symTestSetup.performanceStats->threadReturnStatus = CPA_STATUS_FAIL;
        sampleCodeThreadExit();
    }
    cyInstances = qaeMemAlloc(sizeof(CpaInstanceHandle)*numInstances);
    if(cyInstances == NULL)
    {
        PRINT_ERR("Error allocating memory for instance handles\n");
        symTestSetup.performanceStats->threadReturnStatus = CPA_STATUS_FAIL;
        sampleCodeThreadExit();
    }
    if(cpaCyGetInstances(numInstances, cyInstances) != CPA_STATUS_SUCCESS)
    {
        PRINT_ERR("Failed to get instances\n");
        symTestSetup.performanceStats->threadReturnStatus = CPA_STATUS_FAIL;
        qaeMemFree((void**)&cyInstances);
        sampleCodeThreadExit();
    }
    if(testSetup->logicalQaInstance > numInstances)
    {
        PRINT_ERR("%u is Invalid Logical QA Instance, max is: %u\n",
                testSetup->logicalQaInstance, numInstances);
        symTestSetup.performanceStats->threadReturnStatus = CPA_STATUS_FAIL;
        qaeMemFree((void**)&cyInstances);
        sampleCodeThreadExit();
    }

    /* give our thread a logical crypto instance to use*/
    symTestSetup.cyInstanceHandle=cyInstances[testSetup->logicalQaInstance];
    pPacketSize = qaeMemAlloc(sizeof(Cpa32U)*pSetup->numBuffLists);

    if(NULL == pPacketSize)
    {
        PRINT_ERR("Could not allocate memory for pPacketSize\n");
        symTestSetup.performanceStats->threadReturnStatus = CPA_STATUS_FAIL;
        qaeMemFree((void**)&cyInstances);
        sampleCodeThreadExit();
    }

    if(testSetup->packetSize == PACKET_IMIX)
    {
        /*we are testing IMIX so we copy buffer sizes from preallocated
         * array into symTestSetup.numBuffLists*/
        Cpa32U indexer = sizeof(packetMix)/sizeof(Cpa32U);
        for( loopIteration = 0; loopIteration < pSetup->numBuffLists;
        loopIteration++ )
        {
            pPacketSize[loopIteration] =
                packetMix[loopIteration % indexer];
        }
    }
    else
    {
        /*we are testing a uniform bufferSize, so we set the bufferSize array
         * accordingly*/
        for( loopIteration = 0; loopIteration < pSetup->numBuffLists;
        loopIteration++)
        {
            pPacketSize[loopIteration]=testSetup->packetSize;
        }
    }
    /*cast the setup to a known structure so that we can populate our local
     * test setup*/
    symTestSetup.setupData = pSetup->setupData;
    /*intialize digestIsAppended with input parameter */
    symTestSetup.setupData.digestIsAppended = pSetup->digestAppend;

    symTestSetup.numBuffLists = pSetup->numBuffLists;
    symTestSetup.flatBufferSizeInBytes = pSetup->flatBufferSizeInBytes;
    symTestSetup.numLoops = pSetup->numLoops;
    /*reset the stats print function to NULL, we set it to the proper function
     * if the test passes at the end of this function*/
    testSetup->statsPrintFunc=NULL;
    /*assign the array of buffer sizes we are testing to the symmetric test
     * setup*/
    symTestSetup.packetSizeInBytesArray = pPacketSize;
    /*assign our thread a unique memory location to store performance stats*/
    symTestSetup.performanceStats = testSetup->performanceStats;
    symTestSetup.performanceStats->averagePacketSizeInBytes =
        testSetup->packetSize;
    /* give our thread a logical crypto instance to use*/
    symTestSetup.cyInstanceHandle = cyInstances[testSetup->logicalQaInstance];
    symTestSetup.syncMode = pSetup->syncMode;
    /*store core affinity, this assumes logical cpu core number is the same
     * logicalQaInstace */
    symTestSetup.performanceStats->logicalCoreAffinity =
        testSetup->logicalQaInstance;
    symTestSetup.threadID = testSetup->threadID;
    symTestSetup.isDpApi = pSetup->isDpApi;
    symTestSetup.cryptoSrcOffset = pSetup->cryptoSrcOffset;
    symTestSetup.digestAppend = pSetup->digestAppend;
    /*launch function that does all the work*/
    status = sampleSymmetricPerform(&symTestSetup);
    if(CPA_STATUS_SUCCESS != status)
    {
        printSymTestType(&symTestSetup);
        PRINT("Test %u FAILED\n", testSetup->logicalQaInstance);
        symTestSetup.performanceStats->threadReturnStatus = CPA_STATUS_FAIL;
    }
    else
    {
        /*set the print function that can be used to print stats at the end of
         * the test*/
        testSetup->statsPrintFunc=
            (stats_print_func_t)printSymmetricPerfDataAndStopCyService;
    }
    /*free memory and exit*/
    qaeMemFree((void**)&pPacketSize);
    qaeMemFree((void**)&cyInstances);
    sampleCodeThreadExit();
}

/******************************************************************************
 * @ingroup sampleSymmetricTest
 *
 * @description
 * setup a symmetric test
 * This function needs to be called from main to setup a symmetric test.
 * then the framework createThreads function is used to propagate this setup
 * across cores using different crypto logical instances
 ******************************************************************************/
CpaStatus setupSymmetricTest
(
        CpaCySymOp opType,
        CpaCySymCipherAlgorithm cipherAlg,
        Cpa32U cipherKeyLengthInBytes,
        Cpa32U cipherOffset,
        CpaCyPriority priority,
        CpaCySymHashAlgorithm hashAlg,
        CpaCySymHashMode hashMode,
        Cpa32U authKeyLengthInBytes,
        CpaCySymAlgChainOrder chainOrder,
        sync_mode_t syncMode,
        CpaCySymHashNestedModeSetupData *nestedModeSetupDataPtr,
        Cpa32U packetSize,
        Cpa32U bufferSizeInBytes,
        Cpa32U numBuffLists,
        Cpa32U numLoops,
        Cpa32U digestAppend
)
{
    /*thread_setup_g is a multidimensional global array that stores the setup
     * for all thread variations in an array of characters. We store our test
     * setup at the start of the second array ie index 0. There maybe multiple
     * thread types(setups) running as counted by testTypeCount_g*/
    symmetric_test_params_t* symmetricSetup = NULL;
    Cpa8S name[] = {'S','Y','M','\0'};
    if(testTypeCount_g >= MAX_THREAD_VARIATION)
    {
        PRINT_ERR("Maximum Supported Thread Variation has been exceeded\n");
        PRINT_ERR("Number of Thread Variations created: %d",
                testTypeCount_g);
        PRINT_ERR(" Max is %d\n", MAX_THREAD_VARIATION);
        return CPA_STATUS_FAIL;
    }

    /* Return an error if the number of packets is not modulus zero of the
     * number of packets to cover IMIX packet mix.
     */
    if(packetSize == PACKET_IMIX && (numBuffLists % NUM_PACKETS_IMIX) != 0)
    {
        PRINT_ERR("To ensure that the weighting of IMIX packets is correct "
                ", the number of buffers (%d) should be a multiple of %d\n",
                numBuffLists, NUM_PACKETS_IMIX);
        return CPA_STATUS_FAIL;
    }

    /*start crypto service if not already started*/
    if(CPA_STATUS_SUCCESS != startCyServices())
    {
        PRINT_ERR("Failed to start Crypto services\n");
        return CPA_STATUS_FAIL;
    }
    /* start polling threads if polling is enabled in the configuration file */
    if(CPA_STATUS_SUCCESS != cyCreatePollingThreadsIfPollingIsEnabled())
    {
        PRINT_ERR("Error creating polling threads\n");
        return CPA_STATUS_FAIL;
    }
    /*as setup is a multidimensional char array we need to cast it to the
     * symmetric structure*/
    memcpy(&thread_name_g[testTypeCount_g][0],name,THREAD_NAME_LEN);
    symmetricSetup =
        (symmetric_test_params_t*)&thread_setup_g[testTypeCount_g][0];
	memset(symmetricSetup, 0 , sizeof(symmetric_test_params_t));
    testSetupData_g[testTypeCount_g].performance_function =
        (performance_func_t)sampleSymmetricPerformance;
    testSetupData_g[testTypeCount_g].packetSize=packetSize;
    /*then we store the test setup in the above location*/
    //symmetricSetup->setupData.sessionPriority=CPA_CY_PRIORITY_HIGH;
    symmetricSetup->setupData.sessionPriority=priority;
    symmetricSetup->setupData.symOperation=opType;
    symmetricSetup->setupData.cipherSetupData.cipherAlgorithm = cipherAlg;
    symmetricSetup->setupData.cipherSetupData.cipherDirection =
        CPA_CY_SYM_CIPHER_DIRECTION_ENCRYPT;
    symmetricSetup->setupData.cipherSetupData.cipherKeyLenInBytes
    = cipherKeyLengthInBytes;
    symmetricSetup->setupData.hashSetupData.hashAlgorithm = hashAlg;
    symmetricSetup->setupData.hashSetupData.hashMode=hashMode;
    symmetricSetup->isDpApi = CPA_FALSE;
    symmetricSetup->cryptoSrcOffset=cipherOffset;
    /* in this code we limit the digest result len to be the same as the the
     * authentication key len*/
    symmetricSetup->setupData.hashSetupData.digestResultLenInBytes =
        authKeyLengthInBytes;
    // check which kind of hash mode is selected
    if(CPA_CY_SYM_HASH_MODE_NESTED == hashMode){//nested mode
        //set the struct for nested hash mode
        if(NULL == nestedModeSetupDataPtr){
            // set a default nested mode setup data
            symmetricSetup->setupData.hashSetupData.nestedModeSetupData.
            outerHashAlgorithm = hashAlg;
            symmetricSetup->setupData.hashSetupData.nestedModeSetupData.
            pInnerPrefixData = NULL;
            symmetricSetup->setupData.hashSetupData.nestedModeSetupData.
            innerPrefixLenInBytes = 0;
            symmetricSetup->setupData.hashSetupData.nestedModeSetupData.
            pOuterPrefixData = NULL;
            symmetricSetup->setupData.hashSetupData.nestedModeSetupData.
            outerPrefixLenInBytes = 0;
        }
        else
        {
            symmetricSetup->setupData.hashSetupData.nestedModeSetupData
            = *nestedModeSetupDataPtr;
        }
    }

    if((CPA_CY_SYM_HASH_AES_XCBC == hashAlg )&&
            (AES_XCBC_DIGEST_LENGTH_IN_BYTES != authKeyLengthInBytes))
    {
        symmetricSetup->setupData.hashSetupData.authModeSetupData.
        authKeyLenInBytes = AES_XCBC_DIGEST_LENGTH_IN_BYTES;
    }
    else
    {
        symmetricSetup->setupData.hashSetupData.authModeSetupData.
        authKeyLenInBytes = authKeyLengthInBytes;
    }

    symmetricSetup->setupData.algChainOrder = chainOrder;
    symmetricSetup->syncMode = syncMode;
    symmetricSetup->flatBufferSizeInBytes=bufferSizeInBytes;
    symmetricSetup->numLoops=numLoops;
    symmetricSetup->numBuffLists=numBuffLists;
    if(((bufferSizeInBytes != 0 ) && (packetSize == PACKET_IMIX))
	  || (bufferSizeInBytes % IV_LEN_FOR_16_BYTE_BLOCK_CIPHER != 0 ))
    {
        PRINT_ERR("Doesn't support PACKET_IMIX  "
                  "when the flat buffer size is not 0 or "
				  " it's not align with block size (%d): ",
                   bufferSizeInBytes);
        return CPA_STATUS_FAIL;
    }
    symmetricSetup->digestAppend = digestAppend;
    return CPA_STATUS_SUCCESS;
}

/******************************************************************************
 * @ingroup sampleSymmetricPerf
 *
 * @description
 * setup a cipher test
 * This function needs to be called from main to setup a cipher test.
 * then the framework createThreads function is used to propagate this setup
 * across cores using different crypto logical instances
 ******************************************************************************/
CpaStatus setupCipherTest
(
        CpaCySymCipherAlgorithm cipherAlg,
        Cpa32U cipherKeyLengthInBytes,
        CpaCyPriority priority,
        sync_mode_t syncMode,
        Cpa32U packetSize,
        Cpa32U bufferSizeInBytes,
        Cpa32U numLists,
        Cpa32U numLoops
)
{
    return setupSymmetricTest
    (
            CPA_CY_SYM_OP_CIPHER,
            cipherAlg,
            cipherKeyLengthInBytes,
            NOT_USED,
            priority,
            NOT_USED /* hash alg not needed in cipher test*/,
            NOT_USED /* hash mode not needed in cipher test*/,
            NOT_USED /* auth key len not needed in cipher test*/,
            NOT_USED /* chain mode not needed in cipher test*/,
            syncMode,
            NULL, /* nested hash data not needed in cipher test*/
            packetSize,
            bufferSizeInBytes,
            numLists,
            numLoops,
            CPA_FALSE
    );
}

/******************************************************************************
 * @ingroup sampleSymmetricPerf
 *
 * @description
 * setup a hash test
 * This function needs to be called from main to setup a hash test.
 * then the framework createThreads function is used to propagate this setup
 * across cores using different crypto logical instances
 ******************************************************************************/
CpaStatus setupHashTest
(
        CpaCySymHashAlgorithm hashAlg,
        CpaCySymHashMode hashMode,
        Cpa32U authKeyLengthInBytes,
        CpaCyPriority priority,
        sync_mode_t syncMode,
        Cpa32U packetSize,
        Cpa32U numBufferLists,
        Cpa32U numLoops
)
{
    return setupSymmetricTest
    (
            CPA_CY_SYM_OP_HASH,
            NOT_USED /* cipher alg not needed in cipher test*/,
            NOT_USED /* cipher key len not needed in cipher test*/,
            NOT_USED,
            priority,
            hashAlg,
            hashMode,
            authKeyLengthInBytes,
            NOT_USED /* chain mode not needed in cipher test*/,
            syncMode,
            NULL, /* nested hash data not needed in cipher test*/
            packetSize,
            BUFFER_SIZE_0,
            numBufferLists,
            numLoops,
            CPA_TRUE
    );
}

/******************************************************************************
 * @ingroup sampleSymmetricPerf
 *
 * @description
 * setup a alg chain test (default High Priority)
 * This function needs to be called from main to setup an alg chain test.
 * then the framework createThreads function is used to propagate this setup
 * across IA cores using different crypto logical instances
 ******************************************************************************/
CpaStatus setupAlgChainTest
(
        CpaCySymCipherAlgorithm cipherAlg,
        Cpa32U cipherKeyLengthInBytes,
        CpaCySymHashAlgorithm hashAlg,
        CpaCySymHashMode hashMode,
        Cpa32U authKeyLengthInBytes,
        CpaCySymAlgChainOrder chainOrder,
        CpaCyPriority priority,
        sync_mode_t syncMode,
        Cpa32U packetSize,
        Cpa32U bufferSizeInBytes,
        Cpa32U numBufferLists,
        Cpa32U numLoops
)
{
    return setupSymmetricTest
    (
            CPA_CY_SYM_OP_ALGORITHM_CHAINING,
            cipherAlg,
            cipherKeyLengthInBytes,
            NOT_USED,
            priority,
            hashAlg,
            hashMode,
            authKeyLengthInBytes,
            chainOrder,
            syncMode,
            NULL,
            packetSize,
            bufferSizeInBytes,
            numBufferLists,
            numLoops,
            CPA_TRUE
    );
}

/******************************************************************************
 * @ingroup sampleSymmetricPerf
 *
 * @description
 * setup an IPsec scenario where payload = IP packet, the IP header is not
 * encrypted thus requires an offset into the buffer to test.
 *
 * This function needs to be called from main to setup an alg chain test.
 * then the framework createThreads function is used to propagate this setup
 * across IA cores using different crypto logical instances
 ******************************************************************************/
CpaStatus setupIpSecTest
(
        CpaCySymCipherAlgorithm cipherAlg,
        Cpa32U cipherKeyLengthInBytes,
        Cpa32U cipherOffset,
        CpaCySymHashAlgorithm hashAlg,
        CpaCySymHashMode hashMode,
        Cpa32U authKeyLengthInBytes,
        CpaCySymAlgChainOrder chainOrder,
        Cpa32U packetSize,
        Cpa32U numBufferLists,
        Cpa32U numLoops
)
{
    return setupSymmetricTest
    (
            CPA_CY_SYM_OP_ALGORITHM_CHAINING,
            cipherAlg,
            cipherKeyLengthInBytes,
            cipherOffset,
            CPA_CY_PRIORITY_HIGH,
            hashAlg,
            hashMode,
            authKeyLengthInBytes,
            chainOrder,
            ASYNC,
            NULL,
            packetSize,
            BUFFER_SIZE_0,
            numBufferLists,
            numLoops,
            CPA_TRUE
    );
}
EXPORT_SYMBOL(setupIpSecTest);

/******************************************************************************
 * @ingroup sampleSymmetricPerf
 *
 * @description
 * setup a alg chain test with High Priority
 * This function needs to be called from main to setup an alg chain test.
 * then the framework createThreads function is used to propagate this setup
 * across IA cores using different crypto logical instances
 ******************************************************************************/
CpaStatus setupAlgChainTestHP
(
        CpaCySymCipherAlgorithm cipherAlg,
        Cpa32U cipherKeyLengthInBytes,
        CpaCySymHashAlgorithm hashAlg,
        CpaCySymHashMode hashMode,
        Cpa32U authKeyLengthInBytes,
        CpaCySymAlgChainOrder chainOrder,
        sync_mode_t syncMode,
        Cpa32U packetSize,
        Cpa32U numBufferLists,
        Cpa32U numLoops
)
{
    return setupAlgChainTest
    (
            cipherAlg,
            cipherKeyLengthInBytes,
            hashAlg,
            hashMode,
            authKeyLengthInBytes,
            chainOrder,
            CPA_CY_PRIORITY_HIGH,
            syncMode,
            packetSize,
            DEFAULT_CPA_FLAT_BUFFERS_PER_LIST,
            numBufferLists,
            numLoops
    );
}

/******************************************************************************
 * @ingroup sampleSymmetricPerf
 *
 * @description
 * setup a alg chain test with Normal Priority
 * This function needs to be called from main to setup an alg chain test.
 * then the framework createThreads function is used to propagate this setup
 * across IA cores using different crypto logical instances
 ******************************************************************************/
CpaStatus setupAlgChainTestNP
(
        CpaCySymCipherAlgorithm cipherAlg,
        Cpa32U cipherKeyLengthInBytes,
        CpaCySymHashAlgorithm hashAlg,
        CpaCySymHashMode hashMode,
        Cpa32U authKeyLengthInBytes,
        CpaCySymAlgChainOrder chainOrder,
        sync_mode_t syncMode,
        Cpa32U packetSize,
        Cpa32U numBufferLists,
        Cpa32U numLoops
)
{
    return setupAlgChainTest
    (
            cipherAlg,
            cipherKeyLengthInBytes,
            hashAlg,
            hashMode,
            authKeyLengthInBytes,
            chainOrder,
            CPA_CY_PRIORITY_NORMAL,
            syncMode,
            packetSize,
            DEFAULT_CPA_FLAT_BUFFERS_PER_LIST,
            numBufferLists,
            numLoops
    );
}

/******************************************************************************
 * @ingroup sampleSymmetricPerf
 *
 * @description
 * setup a alg chain test fixing High priority and async mode
 * This function needs to be called from main to setup an alg chain test.
 * then the framework createThreads function is used to propagate this setup
 * across IA cores using different crypto logical instances
 ******************************************************************************/
CpaStatus setupAlgChainTestHPAsync
(
        CpaCySymCipherAlgorithm cipherAlg,
        Cpa32U cipherKeyLengthInBytes,
        CpaCySymHashAlgorithm hashAlg,
        CpaCySymHashMode hashMode,
        Cpa32U authKeyLengthInBytes,
        CpaCySymAlgChainOrder chainOrder,
        Cpa32U packetSize,
        Cpa32U numBufferLists,
        Cpa32U numLoops
)
{
    return setupAlgChainTest
    (
            cipherAlg,
            cipherKeyLengthInBytes,
            hashAlg,
            hashMode,
            authKeyLengthInBytes,
            chainOrder,
            CPA_CY_PRIORITY_HIGH,
            ASYNC,
            packetSize,
            DEFAULT_CPA_FLAT_BUFFERS_PER_LIST,
            numBufferLists,
            numLoops
    );
}

/******************************************************************************
 * @ingroup sampleSymmetricPerf
 *
 * @description
 * setup a alg chain test
 * This function needs to be called from main to setup an alg chain test.
 * then the framework createThreads function is used to propagate this setup
 * across IA cores using different crypto logical instances
 ******************************************************************************/
CpaStatus setupAlgChainTestNestedMode
(
        CpaCySymCipherAlgorithm cipherAlg,
        Cpa32U cipherKeyLengthInBytes,
        CpaCySymHashAlgorithm hashAlg,
        Cpa32U authKeyLengthInBytes,
        CpaCySymAlgChainOrder chainOrder,
        CpaCyPriority priority,
        sync_mode_t syncMode,
        CpaCySymHashNestedModeSetupData *nestedModeSetupData,
        Cpa32U packetSize,
        Cpa32U numBufferLists,
        Cpa32U numLoops
)
{
    return setupSymmetricTest
    (
            CPA_CY_SYM_OP_ALGORITHM_CHAINING,
            cipherAlg,
            cipherKeyLengthInBytes,
            NOT_USED,
            priority,
            hashAlg,
            CPA_CY_SYM_HASH_MODE_NESTED,
            authKeyLengthInBytes,
            chainOrder,
            syncMode,
            nestedModeSetupData,
            packetSize,
            BUFFER_SIZE_0,
            numBufferLists,
            numLoops,
            CPA_TRUE
    );
}


