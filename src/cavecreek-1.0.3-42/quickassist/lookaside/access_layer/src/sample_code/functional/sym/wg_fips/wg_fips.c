#include "cpa.h"
#include "cpa_cy_im.h"
#include "cpa_cy_sym.h"
#include "cpa_sample_utils.h"

#include <stdio.h>
#include <unistd.h>

extern	int gDebugParam;

#define TIMEOUT_MS    5000 /* 5 seconds*/

/* For IPSec outbound direction we encrypt the payload and then
   generate the ICV. For IPSec inbound direction we compare the
   ICV and decrypt the payload
*/
#define IPSEC_OUTBOUND_DIR 0
#define IPSEC_INBOUND_DIR  1

int	DIGEST_TYPE = CPA_CY_SYM_HASH_SHA1;
int	DIGEST_SIZE = 20;

int	ICV_LENGTH  = 12;

int ParseHex(uint8_t* dst, uint8_t* src)
{
  int     n,  v;
  uint8_t hex[4] = {0};

  for (n = 0; (v = -1); n++) {

    hex[0] = hex[1] = 0;

    if (src) {
      if (*src) hex[0] = *src++;
      if (*src) hex[1] = *src++;
    } else {
      if (read(0, &hex[0], 2) != 2) break;
    }

    if (hex[0] < '0') break;
    if (hex[1] < '0') break;

    if (sscanf((char*)hex, "%x", &v) != 1) break;

    if (v < 0) break;

    *dst++ = v;
  }

  return n;
}

/*
 * Callback function
 *
 * This function is "called back" (invoked by the implementation of
 * the API) when the asynchronous operation has completed.  The
 * context in which it is invoked depends on the implementation, but
 * as described in the API it should not sleep (since it may be called
 * in a context which does not permit sleeping, e.g. a Linux bottom
 * half).
 *
 * This function can perform whatever processing is appropriate to the
 * application.  For example, it may free memory, continue processing
 * of a decrypted packet, etc.  In this example, the function checks
 * verifyResult returned and sets the complete variable to indicate it
 * has been called.
 */
static void
symCallback(void *pCallbackTag,
        CpaStatus status,
        const CpaCySymOp operationType,
        void *pOpData,
        CpaBufferList *pDstBuffer,
        CpaBoolean verifyResult)
{
    PRINT_DBG("Callback called with status = %d.\n", status);

    /* For this implementation verifyResult is true by default. In
       the digest generate case verifyDigest will never be false. In
       the digest verify case verifyDigest can be false if digest
       verification fails */
    if(CPA_FALSE == verifyResult)
    {
       PRINT_ERR("Callback verify result error\n");
    }

    if (NULL != pCallbackTag)
    {
        /** indicate that the function has been called */
        COMPLETE((struct COMPLETION_STRUCT *)pCallbackTag);
    }
}

static Cpa8U sampleCipherKey[] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55
};

static Cpa8U sampleCipherIv[] = {
        0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad,
        0xde, 0xca, 0xf8, 0x88, 0x3d, 0x11, 0x59, 0x04
};

static Cpa8U sampleAuthKey[] = {
        0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,
        0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,
        0xDE, 0xAD, 0xBE, 0xEF
};

static Cpa8U sampleEspHdrData[] = {
        0x00, 0x00, 0x01, 0x2c, 0x00, 0x00, 0x00, 0x05
};

/* Payload padded to a multiple of the cipher block size
   (16), 2nd last byte gives pad length, last byte gives
   next header info */
static Cpa8U samplePayload[] = {
        0xd9, 0x31, 0x32, 0x25, 0xf8, 0x84, 0x06, 0xe5,
        0xa5, 0x59, 0x09, 0xc5, 0xaf, 0xf5, 0x26, 0x9a,
        0x86, 0xa7, 0xa9, 0x53, 0x15, 0x34, 0xf7, 0xda,
        0x2e, 0x4c, 0x30, 0x3d, 0x8a, 0x31, 0x8a, 0x72,
        0x1c, 0x3c, 0x0c, 0x95, 0x95, 0x68, 0x09, 0x53,
        0x2f, 0xcf, 0x0e, 0x24, 0x49, 0xa6, 0xb5, 0x25,
        0xb1, 0x6a, 0xed, 0xf5, 0xaa, 0x0d, 0xe6, 0x57,
        0xba, 0x63, 0x7b, 0x39, 0x01, 0x02, 0x02, 0x61
};


static Cpa8U expectedOutput[] = {
        /* ESP header unmodified */
        0x00, 0x00, 0x01, 0x2c, 0x00, 0x00, 0x00, 0x05,
        /* IV unmodified */
        0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad,
        0xde, 0xca, 0xf8, 0x88, 0x3d, 0x11, 0x59, 0x04,
        /* Ciphertext */
        0x39, 0x8E, 0x4C, 0x1B, 0x7B, 0x28, 0x94, 0x52,
        0x97, 0xAD, 0x95, 0x97, 0xD7, 0xF9, 0xB9, 0x4A,
        0x49, 0x03, 0x51, 0x47, 0x45, 0xC7, 0x58, 0x6A,
        0x9A, 0x48, 0xB6, 0x38, 0xB4, 0xD5, 0xEE, 0x42,
        0x4F, 0x39, 0x09, 0x3D, 0xAB, 0x1E, 0xB3, 0x6A,
        0x71, 0x0B, 0xFC, 0x80, 0xAD, 0x2E, 0x4C, 0xA5,
        0xAB, 0x78, 0xB8, 0xAB, 0x87, 0xCC, 0x37, 0xF0,
        0xB9, 0x61, 0xDC, 0xB1, 0xA7, 0x24, 0x26, 0x23,
        /* ICV */
        0xE6, 0x55, 0xBD, 0x90, 0x33, 0x2D, 0x04, 0x8C,
        0x34, 0x06, 0xE3, 0x2D
};

/*
 * Perform an algorithm chaining operation
 */
static CpaStatus
algChainPerformOp(CpaInstanceHandle cyInstHandle, CpaCySymSessionCtx sessionCtx, int dir)
{
    CpaStatus status = CPA_STATUS_SUCCESS;
    Cpa8U  *pBufferMeta = NULL;
    Cpa32U bufferMetaSize = 0;
    CpaBufferList *pBufferList = NULL;
    CpaFlatBuffer *pFlatBuffer = NULL;
    CpaCySymOpData *pOpData = NULL;
    /* buffer size includes space for hdr, iv, payload and icv */
    Cpa32U bufferSize = sizeof(sampleEspHdrData) + sizeof(sampleCipherIv)
                            + sizeof(samplePayload) + ICV_LENGTH;
    Cpa32U numBuffers = 1;  /* only using 1 buffer in this case */
    /* allocate memory for bufferlist and array of flat buffers in a contiguous
     * area and carve it up to reduce number of memory allocations required. */
    Cpa32U bufferListMemSize = sizeof(CpaBufferList) +
        (numBuffers * sizeof(CpaFlatBuffer));
    Cpa8U  *pSrcBuffer = NULL;
    Cpa8U  *pIvBuffer = NULL;

    /* The following variables are allocated on the stack because we block
     * until the callback comes back. If a non-blocking approach was to be
     * used then these variables should be dynamically allocated */
    struct COMPLETION_STRUCT complete;

    /* get meta information size */
    status = cpaCyBufferListGetMetaSize( cyInstHandle,
                numBuffers, &bufferMetaSize);

    if (CPA_STATUS_SUCCESS == status)
    {
        status = PHYS_CONTIG_ALLOC(&pBufferMeta, bufferMetaSize);
    }

    if (CPA_STATUS_SUCCESS == status)
    {
        status = OS_MALLOC(&pBufferList, bufferListMemSize);
    }

    if (CPA_STATUS_SUCCESS == status)
    {
        status = PHYS_CONTIG_ALLOC(&pSrcBuffer, bufferSize);
    }

    if (CPA_STATUS_SUCCESS == status)
    {
        /* increment by sizeof(CpaBufferList) to get at the
         * array of flatbuffers */
        pFlatBuffer = (CpaFlatBuffer *) (pBufferList + 1);

        pBufferList->pBuffers = pFlatBuffer;
        pBufferList->numBuffers = 1;
        pBufferList->pPrivateMetaData = pBufferMeta;

        pFlatBuffer->dataLenInBytes = bufferSize;
        pFlatBuffer->pData = pSrcBuffer;

        /* copy source into buffer */
        if(IPSEC_OUTBOUND_DIR == dir)
        {
           memcpy(pSrcBuffer, sampleEspHdrData, sizeof(sampleEspHdrData));
           memcpy(pSrcBuffer+sizeof(sampleEspHdrData), sampleCipherIv, sizeof(sampleCipherIv));
           memcpy(pSrcBuffer+(sizeof(sampleEspHdrData)+sizeof(sampleCipherIv)),
                                  samplePayload, sizeof(samplePayload));
        }
        else
        {
           memcpy(pSrcBuffer, expectedOutput, sizeof(expectedOutput));
        }

        pIvBuffer = pSrcBuffer + sizeof(sampleEspHdrData);

        status = OS_MALLOC(&pOpData, sizeof(CpaCySymOpData));
    }

    if (CPA_STATUS_SUCCESS == status)
    {
        if(IPSEC_OUTBOUND_DIR == dir)
        {
//<snippet name="opDataIPSecOut">
           /** Populate the structure containing the operational data that is
            * needed to run the algorithm in outbound direction */
           pOpData->sessionCtx = sessionCtx;
           pOpData->packetType = CPA_CY_SYM_PACKET_TYPE_FULL;
           pOpData->pIv = pIvBuffer;
           pOpData->ivLenInBytes = sizeof(sampleCipherIv);
           pOpData->cryptoStartSrcOffsetInBytes = sizeof(sampleEspHdrData)+sizeof(sampleCipherIv);
           pOpData->messageLenToCipherInBytes = sizeof(samplePayload);
           pOpData->hashStartSrcOffsetInBytes = 0;
           pOpData->messageLenToHashInBytes = sizeof(sampleEspHdrData)
                                   +sizeof(sampleCipherIv)+sizeof(samplePayload);
           /* Even though ICV follows immediately after the region to hash
           digestIsAppended is set to false in this case to workaround
           errata number IXA00378322 */
           pOpData->pDigestResult = pSrcBuffer+
                 (sizeof(sampleEspHdrData)+sizeof(sampleCipherIv)+sizeof(samplePayload));
//</snippet>
       }
       else
       {
//<snippet name="opDataIPSecIn">
           /** Populate the structure containing the operational data that is
            * needed to run the algorithm in inbound direction */
           pOpData->sessionCtx = sessionCtx;
           pOpData->packetType = CPA_CY_SYM_PACKET_TYPE_FULL;
           pOpData->pIv = pIvBuffer;
           pOpData->ivLenInBytes = sizeof(sampleCipherIv);
           pOpData->cryptoStartSrcOffsetInBytes = sizeof(sampleEspHdrData)+sizeof(sampleCipherIv);
           pOpData->messageLenToCipherInBytes = bufferSize -
                                    (sizeof(sampleEspHdrData)+sizeof(sampleCipherIv)+ICV_LENGTH);
           pOpData->hashStartSrcOffsetInBytes = 0;
           pOpData->messageLenToHashInBytes = bufferSize - ICV_LENGTH;
//</snippet>
       }
    }

    if (CPA_STATUS_SUCCESS == status)
    {
        /** initialisation for callback; the "complete" variable is used by the
         * callback function to indicate it has been called*/
        COMPLETION_INIT(&complete);

        PRINT_DBG("cpaCySymPerformOp\n");

        /** Perform symmetric operation */
        status = cpaCySymPerformOp(cyInstHandle,
                (void *)&complete, /* data sent as is to the callback function*/
                pOpData,           /* operational data struct */
                pBufferList,       /* source buffer list */
                pBufferList,       /* same src & dst for an in-place operation*/
                NULL);             /* pVerifyResult not required in async mode */

        if (CPA_STATUS_SUCCESS != status)
        {
            PRINT_ERR("cpaCySymPerformOp failed. (status = %d)\n", status);
        }

        if (CPA_STATUS_SUCCESS == status)
        {
            /** wait until the completion of the operation*/
            if (!COMPLETION_WAIT(&complete, TIMEOUT_MS))
            {
                PRINT_ERR("timeout or interruption in cpaCySymPerformOp\n");
                status = CPA_STATUS_FAIL;
            }
        }

        if (CPA_STATUS_SUCCESS == status)
        {
            if(IPSEC_OUTBOUND_DIR == dir)
            {
               if (0 == memcmp(pSrcBuffer, expectedOutput, bufferSize))
               {
                   PRINT_DBG("Output matches expected output encrypt generate\n");
               }
               else
               {
                   PRINT_DBG("Output does not match expected output encrypt generate\n");
                   status = CPA_STATUS_FAIL;
               }
            }
            else
            {
               if (0 == memcmp(pSrcBuffer+(sizeof(sampleEspHdrData)+sizeof(sampleCipherIv)),
                              samplePayload, sizeof(samplePayload)))
               {
                   PRINT_DBG("Output matches expected output decrypt verify\n");
               }
               else
               {
                   PRINT_DBG("Output does not match expected output decrypt verify\n");
                   status = CPA_STATUS_FAIL;
                }
            }
        }
    }


    /* at this stage, the callback function has returned, so it is sure that
     * the structures won't be needed any more*/
    PHYS_CONTIG_FREE(pSrcBuffer);
    OS_FREE(pBufferList);
    PHYS_CONTIG_FREE(pBufferMeta);
    OS_FREE(pOpData);

    COMPLETION_DESTROY(&complete);

    return status;
}

CpaStatus
algChainSample(void)
{
    CpaStatus status = CPA_STATUS_FAIL;
    CpaCySymSessionCtx sessionCtx = NULL;
    Cpa32U sessionCtxSize = 0;
    CpaInstanceHandle cyInstHandle = NULL;
    CpaCySymSessionSetupData sessionSetupData = {0};
    CpaCySymStats64 symStats = {0};
    /*
     * In this simplified version of instance discovery, we discover
     * exactly one instance of a crypto service.
     */
    sampleCyGetInstance(&cyInstHandle);
    if (cyInstHandle == NULL)
    {
        PRINT_DBG("No crypto instances available\n");
        return CPA_STATUS_FAIL;
    }

    /* Start Cryptographic component */
    PRINT_DBG("cpaCyStartInstance\n");
    status = cpaCyStartInstance(cyInstHandle);

    if(CPA_STATUS_SUCCESS == status)
    {
      /*
       * Set the address translation function for the instance
       */
       status = cpaCySetAddressTranslation(cyInstHandle, sampleVirtToPhys);
    }

    if (CPA_STATUS_SUCCESS == status)
    {
       /*
        * If the instance is polled start the polling thread. Note that
        * how the polling is done is implementation-dependant.
        */
        sampleCyStartPolling(cyInstHandle);

        PRINT_DBG("Encrypt-Generate ICV\n");

        /* populate symmetric session data structure */
        sessionSetupData.sessionPriority =  CPA_CY_PRIORITY_HIGH;
//<snippet name="initSessionIPSecEnc">
        sessionSetupData.symOperation = CPA_CY_SYM_OP_ALGORITHM_CHAINING;
        sessionSetupData.algChainOrder =
                                    CPA_CY_SYM_ALG_CHAIN_ORDER_CIPHER_THEN_HASH;

        sessionSetupData.cipherSetupData.cipherAlgorithm =
                                    CPA_CY_SYM_CIPHER_AES_CBC;
        sessionSetupData.cipherSetupData.pCipherKey = sampleCipherKey;
        sessionSetupData.cipherSetupData.cipherKeyLenInBytes =
                                    sizeof(sampleCipherKey);
        sessionSetupData.cipherSetupData.cipherDirection =
                                    CPA_CY_SYM_CIPHER_DIRECTION_ENCRYPT;

        sessionSetupData.hashSetupData.hashAlgorithm =  CPA_CY_SYM_HASH_SHA1;
        sessionSetupData.hashSetupData.hashMode = CPA_CY_SYM_HASH_MODE_AUTH;
        sessionSetupData.hashSetupData.digestResultLenInBytes = ICV_LENGTH;
        sessionSetupData.hashSetupData.authModeSetupData.authKey = sampleAuthKey;
        sessionSetupData.hashSetupData.authModeSetupData.authKeyLenInBytes =
                                            sizeof(sampleAuthKey);

        /* Even though ICV follows immediately after the region to hash
           digestIsAppended is set to false in this case to workaround
           errata number IXA00378322 */
        sessionSetupData.digestIsAppended = CPA_FALSE;
        /* Generate the ICV in outbound direction */
        sessionSetupData.verifyDigest = CPA_FALSE;
//</snippet>

        /* Determine size of session context to allocate */
        status = cpaCySymSessionCtxGetSize(cyInstHandle,
                    &sessionSetupData, &sessionCtxSize);
    }

    if (CPA_STATUS_SUCCESS == status)
    {
        /* Allocate session context */
        status = PHYS_CONTIG_ALLOC(&sessionCtx, sessionCtxSize);
    }

    if (CPA_STATUS_SUCCESS == status)
    {
        /* Initialize the session */
        status = cpaCySymInitSession(cyInstHandle,
                    symCallback, &sessionSetupData, sessionCtx);
    }

    if (CPA_STATUS_SUCCESS == status)
    {
        CpaStatus sessionStatus = CPA_STATUS_SUCCESS;

        /* Perform algchaining operation */
        status = algChainPerformOp(cyInstHandle, sessionCtx,
                                               IPSEC_OUTBOUND_DIR);

        /* Remove the session - session init has already succeeded */
        sessionStatus = cpaCySymRemoveSession(
                            cyInstHandle, sessionCtx);

        /* maintain status of remove session only when status of all operations
         * before it are successful. */
        if (CPA_STATUS_SUCCESS == status)
        {
            status = sessionStatus;
        }
    }

    if(CPA_STATUS_SUCCESS == status)
    {
        PRINT_DBG("Decrypt-Verify ICV\n");

        /* populate symmetric session data structure */
        sessionSetupData.sessionPriority =  CPA_CY_PRIORITY_HIGH;
//<snippet name="initSessionIPSecDec">
        sessionSetupData.symOperation = CPA_CY_SYM_OP_ALGORITHM_CHAINING;
        sessionSetupData.algChainOrder =
                                    CPA_CY_SYM_ALG_CHAIN_ORDER_HASH_THEN_CIPHER;

        sessionSetupData.cipherSetupData.cipherAlgorithm =
                                    CPA_CY_SYM_CIPHER_AES_CBC;
        sessionSetupData.cipherSetupData.pCipherKey = sampleCipherKey;
        sessionSetupData.cipherSetupData.cipherKeyLenInBytes =
                                    sizeof(sampleCipherKey);
        sessionSetupData.cipherSetupData.cipherDirection =
                                    CPA_CY_SYM_CIPHER_DIRECTION_DECRYPT;

        sessionSetupData.hashSetupData.hashAlgorithm =  CPA_CY_SYM_HASH_SHA1;
        sessionSetupData.hashSetupData.hashMode = CPA_CY_SYM_HASH_MODE_AUTH;
        sessionSetupData.hashSetupData.digestResultLenInBytes = ICV_LENGTH;
        sessionSetupData.hashSetupData.authModeSetupData.authKey = sampleAuthKey;
        sessionSetupData.hashSetupData.authModeSetupData.authKeyLenInBytes
                                                         = sizeof(sampleAuthKey);

        /* ICV follows immediately after the region to hash */
        sessionSetupData.digestIsAppended = CPA_TRUE;
        /* Verify the ICV in the inbound direction */
        sessionSetupData.verifyDigest = CPA_TRUE;
//</snippet>

    }
    if (CPA_STATUS_SUCCESS == status)
    {
        /* Initialize the session */
        status = cpaCySymInitSession(cyInstHandle,
                    symCallback, &sessionSetupData, sessionCtx);
    }

    if (CPA_STATUS_SUCCESS == status)
    {
        CpaStatus sessionStatus = CPA_STATUS_SUCCESS;

        /* Perform algchaining operation */
        status = algChainPerformOp(cyInstHandle, sessionCtx,
                                                      IPSEC_INBOUND_DIR);

        /* Remove the session - session init has already succeeded */
        sessionStatus = cpaCySymRemoveSession(
                            cyInstHandle, sessionCtx);

        /* maintain status of remove session only when status of all operations
         * before it are successful. */
        if (CPA_STATUS_SUCCESS == status)
        {
            status = sessionStatus;
        }
    }

    if (CPA_STATUS_SUCCESS == status)
    {
        /* Query symmetric statistics */
        status = cpaCySymQueryStats64(cyInstHandle, &symStats);

        if (CPA_STATUS_SUCCESS != status)
        {
            PRINT_ERR("cpaCySymQueryStats failed, status = %d\n", status);
        }
        else
        {
            PRINT_DBG("Number of symmetric operations completed: %llu\n",
                    (unsigned long long)symStats.numSymOpCompleted);
        }
    }

    /* Clean up */

    /* Free session Context */
    PHYS_CONTIG_FREE(sessionCtx);

    /* Stop the polling thread */
    sampleCyStopPolling();


    PRINT_DBG("cpaCyStopInstance\n");
    cpaCyStopInstance(cyInstHandle);

    if (CPA_STATUS_SUCCESS == status)
    {
        PRINT_DBG("Sample code ran successfully\n");
    }
    else
    {
        PRINT_DBG("Sample code failed with status of %d\n", status);
    }

    return status;
}

/* source data to hash */
static Cpa8U vectorData[] = {
        0x37
};

/*
 * Perform a hash operation
 */
static CpaStatus
hashPerformOp(CpaInstanceHandle cyInstHandle, CpaCySymSessionCtx sessionCtx)
{
    CpaStatus status = CPA_STATUS_SUCCESS;
    Cpa8U  *pBufferMeta = NULL;
    Cpa32U bufferMetaSize = 0;
    CpaBufferList *pBufferList = NULL;
    CpaFlatBuffer *pFlatBuffer = NULL;
    CpaCySymOpData *pOpData = NULL;
    Cpa32U bufferSize = sizeof(vectorData) + DIGEST_SIZE;
    Cpa32U numBuffers = 1;  /* only using 1 buffer in this case */
    /* allocate memory for bufferlist and array of flat buffers in a contiguous
     * area and carve it up to reduce number of memory allocations required. */
    Cpa32U bufferListMemSize = sizeof(CpaBufferList) +
        (numBuffers * sizeof(CpaFlatBuffer));
    Cpa8U  *pSrcBuffer = NULL;
    Cpa8U  *pDigestBuffer = NULL;

    /* The following variables are allocated on the stack because we block
     * until the callback comes back. If a non-blocking approach was to be
     * used then these variables should be dynamically allocated */
    struct COMPLETION_STRUCT complete;

    /* get meta information size */
    PRINT_DBG("cpaCyBufferListGetMetaSize\n");
    status = cpaCyBufferListGetMetaSize( cyInstHandle,
                numBuffers, &bufferMetaSize);

    if (CPA_STATUS_SUCCESS == status)
    {
        status = PHYS_CONTIG_ALLOC(&pBufferMeta, bufferMetaSize);
    }

    if (CPA_STATUS_SUCCESS == status)
    {
        status = OS_MALLOC(&pBufferList, bufferListMemSize);
    }

    if (CPA_STATUS_SUCCESS == status)
    {
        status = PHYS_CONTIG_ALLOC(&pSrcBuffer, bufferSize);
    }

    if (CPA_STATUS_SUCCESS == status)
    {
        /* copy vector into buffer */
        memcpy(pSrcBuffer, vectorData, sizeof(vectorData));

        pDigestBuffer = pSrcBuffer + sizeof(vectorData);
        /* increment by sizeof(CpaBufferList) to get at the
         * array of flatbuffers */
        pFlatBuffer = (CpaFlatBuffer *) (pBufferList + 1);

        pBufferList->pBuffers = pFlatBuffer;
        pBufferList->numBuffers = 1;
        pBufferList->pPrivateMetaData = pBufferMeta;

        pFlatBuffer->dataLenInBytes = bufferSize;
        pFlatBuffer->pData = pSrcBuffer;

        status = OS_MALLOC(&pOpData, sizeof(CpaCySymOpData));
    }

    if (CPA_STATUS_SUCCESS == status)
    {
//<snippet name="opData">
        pOpData->sessionCtx = sessionCtx;
        pOpData->packetType = CPA_CY_SYM_PACKET_TYPE_FULL;
        pOpData->hashStartSrcOffsetInBytes = 0;
        pOpData->messageLenToHashInBytes = sizeof(vectorData);
        pOpData->pDigestResult = pDigestBuffer;
//</snippet>
    }

    if (CPA_STATUS_SUCCESS == status)
    {
        /** initialisation for callback; the "complete" variable is used by the
         * callback function to indicate it has been called*/
        COMPLETION_INIT((&complete));

        PRINT_DBG("cpaCySymPerformOp\n");

        /** Perform symmetric operation */
        status = cpaCySymPerformOp(cyInstHandle,
                (void *)&complete, /* data sent as is to the callback function*/
                pOpData,           /* operational data struct */
                pBufferList,       /* source buffer list */
                pBufferList,       /* same src & dst for an in-place operation*/
                NULL);

        if (CPA_STATUS_SUCCESS != status)
        {
            PRINT_ERR("cpaCySymPerformOp failed. (status = %d)\n", status);
        }

        if (CPA_STATUS_SUCCESS == status)
        {
            /** wait until the completion of the operation*/
            if (!COMPLETION_WAIT((&complete), TIMEOUT_MS))
            {
                PRINT_ERR("timeout or interruption in cpaCySymPerformOp\n");
                status = CPA_STATUS_FAIL;
            }
        }

        if (CPA_STATUS_SUCCESS == status)
        {
            int j;
            for (j = 0; j < DIGEST_SIZE; j++)
            printf("%02X", pOpData->pDigestResult[j]);
            printf("\n");
        }
    }

    /* At this stage, the callback function should have returned,
     * so it is safe to free the memory */
    PHYS_CONTIG_FREE(pSrcBuffer);
    OS_FREE(pBufferList);
    PHYS_CONTIG_FREE(pBufferMeta);
    OS_FREE(pOpData);

    COMPLETION_DESTROY(&complete);

    return status;
}

/*
 * This is the main entry point for the sample cipher code.  It
 * demonstrates the sequence of calls to be made to the API in order
 * to create a session, perform one or more hash operations, and
 * then tear down the session.
 */
CpaStatus
hashSample(void)
{
    CpaStatus status = CPA_STATUS_SUCCESS;
    Cpa32U sessionCtxSize = 0;
    CpaInstanceHandle cyInstHandle = NULL;
    CpaCySymSessionCtx sessionCtx = NULL;
    CpaCySymSessionSetupData sessionSetupData = {0};
    CpaCySymStats64 symStats = {0};

    switch (DIGEST_SIZE) {
    case 20: DIGEST_TYPE = CPA_CY_SYM_HASH_SHA1;   break;
    case 32: DIGEST_TYPE = CPA_CY_SYM_HASH_SHA256; break;
    case 48: DIGEST_TYPE = CPA_CY_SYM_HASH_SHA384; break;
    case 64: DIGEST_TYPE = CPA_CY_SYM_HASH_SHA512; break;
    default: return        CPA_STATUS_FAIL;
    }

    /*
     * In this simplified version of instance discovery, we discover
     * exactly one instance of a crypto service.
     */
    sampleCyGetInstance(&cyInstHandle);
    if (cyInstHandle == NULL)
    {
        return CPA_STATUS_FAIL;
    }

    /* Start Cryptographic component */
    PRINT_DBG("cpaCyStartInstance\n");
    status = cpaCyStartInstance(cyInstHandle);

    if(CPA_STATUS_SUCCESS == status)
    {
       /*
        * Set the address translation function for the instance
        */
        status = cpaCySetAddressTranslation(cyInstHandle, sampleVirtToPhys);
    }

    if (CPA_STATUS_SUCCESS == status)
    {
       /*
        * If the instance is polled start the polling thread. Note that
        * how the polling is done is implementation-dependant.
        */
        sampleCyStartPolling(cyInstHandle);

       /*
        * We now populate the fields of the session operational data and create
        * the session.  Note that the size required to store a session is
        * implementation-dependent, so we query the API first to determine how
        * much memory to allocate, and then allocate that memory.
        */
//<snippet name="initSession">
        /* populate symmetric session data structure
         * for a plain hash operation */
        sessionSetupData.sessionPriority = CPA_CY_PRIORITY_NORMAL;
        sessionSetupData.symOperation = CPA_CY_SYM_OP_HASH;
        sessionSetupData.hashSetupData.hashAlgorithm = DIGEST_TYPE;
        sessionSetupData.hashSetupData.hashMode = CPA_CY_SYM_HASH_MODE_PLAIN;
        sessionSetupData.hashSetupData.digestResultLenInBytes = DIGEST_SIZE;
        /* Place the digest result in a buffer unrelated to srcBuffer */
        sessionSetupData.digestIsAppended = CPA_FALSE;
        /* Generate the digest */
        sessionSetupData.verifyDigest = CPA_FALSE;
//</snippet>

        /* Determine size of session context to allocate */
        PRINT_DBG("cpaCySymSessionCtxGetSize\n");
        status = cpaCySymSessionCtxGetSize(cyInstHandle,
                    &sessionSetupData, &sessionCtxSize);
    }

    if (CPA_STATUS_SUCCESS == status)
    {
        /* Allocate session context */
        status = PHYS_CONTIG_ALLOC(&sessionCtx, sessionCtxSize);
    }

    if (CPA_STATUS_SUCCESS == status)
    {
        /* Initialize the Hash session */
        PRINT_DBG("cpaCySymInitSession\n");
        status = cpaCySymInitSession(cyInstHandle,
                    symCallback, &sessionSetupData, sessionCtx);
    }

    if (CPA_STATUS_SUCCESS == status)
    {
        CpaStatus sessionStatus = CPA_STATUS_SUCCESS;

        /* Perform Hash operation */
        status = hashPerformOp(cyInstHandle, sessionCtx);

        /* Remove the session - session init has already succeeded */
        PRINT_DBG("cpaCySymRemoveSession\n");
        sessionStatus = cpaCySymRemoveSession(
                            cyInstHandle, sessionCtx);

        /* maintain status of remove session only when status of all operations
         * before it are successful. */
        if (CPA_STATUS_SUCCESS == status)
        {
            status = sessionStatus;
        }
    }

    if (CPA_STATUS_SUCCESS == status)
    {
        /* Query symmetric statistics */
        status = cpaCySymQueryStats64(cyInstHandle, &symStats);

        if (CPA_STATUS_SUCCESS != status)
        {
            PRINT_ERR("cpaCySymQueryStats failed, status = %d\n", status);
        }
        else
        {
            PRINT_DBG("Number of symmetric operation completed: %llu\n",
                    (unsigned long long)symStats.numSymOpCompleted);
        }
    }

    /* Clean up */

    /* Free session Context */
    PHYS_CONTIG_FREE(sessionCtx);

    /* Stop the polling thread */
    sampleCyStopPolling();

    PRINT_DBG("cpaCyStopInstance\n");
    cpaCyStopInstance(cyInstHandle);

    if (CPA_STATUS_SUCCESS == status)
    {
        PRINT_DBG("Sample code ran successfully\n");
    }
    else
    {
        PRINT_DBG("Sample code failed with status of %d\n", status);
    }

    return status;
}

/* Source data to encrypt */
static Cpa8U sampleCipherSrc[] = {
        0xD7,0x1B,0xA4,0xCA,0xEC,0xBD,0x15,0xE2,0x52,0x6A,0x21,0x0B,
        0x81,0x77,0x0C,0x90,0x68,0xF6,0x86,0x50,0xC6,0x2C,0x6E,0xED,
        0x2F,0x68,0x39,0x71,0x75,0x1D,0x94,0xF9,0x0B,0x21,0x39,0x06,
        0xBE,0x20,0x94,0xC3,0x43,0x4F,0x92,0xC9,0x07,0xAA,0xFE,0x7F,
        0xCF,0x05,0x28,0x6B,0x82,0xC4,0xD7,0x5E,0xF3,0xC7,0x74,0x68,
        0xCF,0x05,0x28,0x6B,0x82,0xC4,0xD7,0x5E,0xF3,0xC7,0x74,0x68,
        0x80,0x8B,0x28,0x8D,0xCD,0xCA,0x94,0xB8,0xF5,0x66,0x0C,0x00,
        0x5C,0x69,0xFC,0xE8,0x7F,0x0D,0x81,0x97,0x48,0xC3,0x6D,0x24
};

/*
 * This function performs a cipher operation.
 */
static CpaStatus
cipherPerformOp(CpaInstanceHandle cyInstHandle, CpaCySymSessionCtx sessionCtx)
{
    CpaStatus status = CPA_STATUS_SUCCESS;
    Cpa8U  *pBufferMeta = NULL;
    Cpa32U bufferMetaSize = 0;
    CpaBufferList *pBufferList = NULL;
    CpaFlatBuffer *pFlatBuffer = NULL;
    CpaCySymOpData *pOpData = NULL;
    Cpa32U bufferSize = sizeof(sampleCipherSrc);
    Cpa32U numBuffers = 1;  /* only using 1 buffer in this case */
    /* allocate memory for bufferlist and array of flat buffers in a contiguous
     * area and carve it up to reduce number of memory allocations required. */
    Cpa32U bufferListMemSize = sizeof(CpaBufferList) +
        (numBuffers * sizeof(CpaFlatBuffer));
    Cpa8U *pSrcBuffer = NULL;
    Cpa8U  *pIvBuffer = NULL;

    /* The following variables are allocated on the stack because we block
     * until the callback comes back. If a non-blocking approach was to be
     * used then these variables should be dynamically allocated */
    struct COMPLETION_STRUCT complete;

    PRINT_DBG("cpaCyBufferListGetMetaSize\n");

    /*
     * Different implementations of the API require different
     * amounts of space to store meta-data associated with buffer
     * lists.  We query the API to find out how much space the current
     * implementation needs, and then allocate space for the buffer
     * meta data, the buffer list, and for the buffer itself.  We also
     * allocate memory for the initialization vector.  We then
     * populate this memory with the required data.
     */
//<snippet name="memAlloc">
    status = cpaCyBufferListGetMetaSize( cyInstHandle,
                numBuffers, &bufferMetaSize);

    if (CPA_STATUS_SUCCESS == status)
    {
        status = PHYS_CONTIG_ALLOC(&pBufferMeta, bufferMetaSize);
    }

    if (CPA_STATUS_SUCCESS == status)
    {
        status = OS_MALLOC(&pBufferList, bufferListMemSize);
    }

    if (CPA_STATUS_SUCCESS == status)
    {
        status = PHYS_CONTIG_ALLOC(&pSrcBuffer, bufferSize);
    }

    if (CPA_STATUS_SUCCESS == status)
    {
        status = PHYS_CONTIG_ALLOC(&pIvBuffer, sizeof(sampleCipherIv));
    }
//</snippet>

    if (CPA_STATUS_SUCCESS == status)
    {
        /* copy source into buffer */
        memcpy(pSrcBuffer, sampleCipherSrc, sizeof(sampleCipherSrc));

        /* copy IV into buffer */
        memcpy(pIvBuffer, sampleCipherIv, sizeof(sampleCipherIv));

        /* increment by sizeof(CpaBufferList) to get at the
         * array of flatbuffers */
        pFlatBuffer = (CpaFlatBuffer *) (pBufferList + 1);

        pBufferList->pBuffers = pFlatBuffer;
        pBufferList->numBuffers = 1;
        pBufferList->pPrivateMetaData = pBufferMeta;

        pFlatBuffer->dataLenInBytes = bufferSize;
        pFlatBuffer->pData = pSrcBuffer;

        status = OS_MALLOC(&pOpData, sizeof(CpaCySymOpData));
    }

    if (CPA_STATUS_SUCCESS == status)
    {
        /*
         * Populate the structure containing the operational data needed
         * to run the algorithm:
         * - packet type information (the algorithm can operate on a full
         *   packet, perform a partial operation and maintain the state or
         *   complete the last part of a multi-part operation)
         * - the initialization vector and its length
         * - the offset in the source buffer
         * - the length of the source message
         */
//<snippet name="opData">
        pOpData->sessionCtx = sessionCtx;
        pOpData->packetType = CPA_CY_SYM_PACKET_TYPE_FULL;
        pOpData->pIv = pIvBuffer;
        pOpData->ivLenInBytes = sizeof(sampleCipherIv);
        pOpData->cryptoStartSrcOffsetInBytes = 0;
        pOpData->messageLenToCipherInBytes = sizeof(sampleCipherSrc);
//</snippet>
    }

    /*
     * Now, we initialize the completion variable which is used by the callback function
     * to indicate that the operation is complete.  We then perform the operation.
     */
    if (CPA_STATUS_SUCCESS == status)
    {
        PRINT_DBG("cpaCySymPerformOp\n");

//<snippet name="perfOp">
        COMPLETION_INIT(&complete);

        status = cpaCySymPerformOp(cyInstHandle,
                (void *)&complete, /* data sent as is to the callback function*/
                pOpData,           /* operational data struct */
                pBufferList,       /* source buffer list */
                pBufferList,       /* same src & dst for an in-place operation*/
                NULL);
//</snippet>

        if (CPA_STATUS_SUCCESS != status)
        {
            PRINT_ERR("cpaCySymPerformOp failed. (status = %d)\n", status);
        }

        /*
         * We now wait until the completion of the operation.  This uses a macro
         * which can be defined differently for different OSes.
         */
        if (CPA_STATUS_SUCCESS == status)
        {
//<snippet name="completion">
            if (!COMPLETION_WAIT(&complete, TIMEOUT_MS))
            {
                PRINT_ERR("timeout or interruption in cpaCySymPerformOp\n");
                status = CPA_STATUS_FAIL;
            }
//</snippet>
        }

        /*
         * We now check that the output matches the expected output.
         */
        if (CPA_STATUS_SUCCESS == status)
        {
            if (0 == memcmp(pSrcBuffer, expectedOutput, bufferSize))
            {
                PRINT_DBG("Output matches expected output\n");
            }
            else
            {
                PRINT_DBG("Output does not match expected output\n");
                status = CPA_STATUS_FAIL;
            }
        }
    }

    /*
     * At this stage, the callback function has returned, so it is
     * sure that the structures won't be needed any more.  Free the
     * memory!
     */
    PHYS_CONTIG_FREE(pSrcBuffer);
    PHYS_CONTIG_FREE(pIvBuffer);
    OS_FREE(pBufferList);
    PHYS_CONTIG_FREE(pBufferMeta);
    OS_FREE(pOpData);

    COMPLETION_DESTROY(&complete);

    return status;
}

/*
 * This is the main entry point for the sample cipher code.  It
 * demonstrates the sequence of calls to be made to the API in order
 * to create a session, perform one or more cipher operations, and
 * then tear down the session.
 */
CpaStatus
cipherSample(void)
{
    CpaStatus status = CPA_STATUS_SUCCESS;
    Cpa32U sessionCtxSize = 0;
    CpaCySymSessionCtx sessionCtx = NULL;
    CpaInstanceHandle cyInstHandle = NULL;
    CpaCySymSessionSetupData sessionSetupData = {0};
    CpaCySymStats64 symStats = {0};

    /*
     * In this simplified version of instance discovery, we discover
     * exactly one instance of a crypto service.
     */
    sampleCyGetInstance(&cyInstHandle);
    if (cyInstHandle == NULL)
    {
        return CPA_STATUS_FAIL;
    }

    /* Start Cryptographic component */
    PRINT_DBG("cpaCyStartInstance\n");
//<snippet name="startup">
    status = cpaCyStartInstance(cyInstHandle);
//</snippet>

    if(CPA_STATUS_SUCCESS == status)
    {
       /*
        * Set the address translation function for the instance
        */
//<snippet name="virt2phys">
        status = cpaCySetAddressTranslation(cyInstHandle, sampleVirtToPhys);
//</snippet>
    }

    if (CPA_STATUS_SUCCESS == status)
    {
       /*
        * If the instance is polled start the polling thread. Note that
        * how the polling is done is implementation-dependant.
        */
       sampleCyStartPolling(cyInstHandle);

       /*
        * We now populate the fields of the session operational data and create
        * the session.  Note that the size required to store a session is
        * implementation-dependent, so we query the API first to determine how
        * much memory to allocate, and then allocate that memory.
        */
//<snippet name="initSession">
        /* Populate the session setup structure for the operation required */
        sessionSetupData.sessionPriority =  CPA_CY_PRIORITY_NORMAL;
        sessionSetupData.symOperation =     CPA_CY_SYM_OP_CIPHER;
        sessionSetupData.cipherSetupData.cipherAlgorithm =
                                            CPA_CY_SYM_CIPHER_3DES_CBC;
        sessionSetupData.cipherSetupData.pCipherKey =
                                            sampleCipherKey;
        sessionSetupData.cipherSetupData.cipherKeyLenInBytes =
                                            sizeof(sampleCipherKey);
        sessionSetupData.cipherSetupData.cipherDirection =
                                            CPA_CY_SYM_CIPHER_DIRECTION_ENCRYPT;

        /* Determine size of session context to allocate */
        PRINT_DBG("cpaCySymSessionCtxGetSize\n");
        status = cpaCySymSessionCtxGetSize(cyInstHandle,
                    &sessionSetupData, &sessionCtxSize);
    }

    if (CPA_STATUS_SUCCESS == status)
    {
        /* Allocate session context */
        status = PHYS_CONTIG_ALLOC(&sessionCtx, sessionCtxSize);
    }


    /* Initialize the Cipher session */
    if (CPA_STATUS_SUCCESS == status)
    {
        PRINT_DBG("cpaCySymInitSession\n");
        status = cpaCySymInitSession(cyInstHandle,
                            symCallback,        /* callback function */
                            &sessionSetupData,  /* session setup data */
                            sessionCtx);       /* output of the function*/
    }
//</snippet>

    if (CPA_STATUS_SUCCESS == status)
    {
        CpaStatus sessionStatus = CPA_STATUS_SUCCESS;

        /* Perform Cipher operation */
        status = cipherPerformOp(cyInstHandle, sessionCtx);

        /*
         * In a typical usage, the session might be used to encipher
         * or decipher multiple buffers.  In this example however, we
         * can now tear down the session.
         */
        PRINT_DBG("cpaCySymRemoveSession\n");
//<snippet name="removeSession">
        sessionStatus = cpaCySymRemoveSession(
                cyInstHandle, sessionCtx);
//</snippet>

        /* Maintain status of remove session only when status of all operations
         * before it are successful. */
        if (CPA_STATUS_SUCCESS == status)
        {
            status = sessionStatus;
        }
    }

    if (CPA_STATUS_SUCCESS == status)
    {
        /*
         * We can now query the statistics on the instance.
         *
         * Note that some implementations may also make the stats
         * available through other mechanisms, e.g. in the /proc
         * virtual filesystem.
         */
        status = cpaCySymQueryStats64(cyInstHandle, &symStats);

        if (CPA_STATUS_SUCCESS != status)
        {
            PRINT_ERR("cpaCySymQueryStats64 failed, status = %d\n", status);
        }
        else
        {
            PRINT_DBG("Number of symmetric operation completed: %llu\n",
                    (unsigned long long)symStats.numSymOpCompleted);
        }
    }

    /*
     * Free up memory, stop the instance, etc.
     */

    /* Free session Context */
    PHYS_CONTIG_FREE(sessionCtx);

    /* Stop the polling thread */
    sampleCyStopPolling();

    PRINT_DBG("cpaCyStopInstance\n");
    cpaCyStopInstance(cyInstHandle);

    if (CPA_STATUS_SUCCESS == status)
    {
        PRINT_DBG("Sample code ran successfully\n");
    }
    else
    {
        PRINT_DBG("Sample code failed with status of %d\n", status);
    }

    return status;
}
