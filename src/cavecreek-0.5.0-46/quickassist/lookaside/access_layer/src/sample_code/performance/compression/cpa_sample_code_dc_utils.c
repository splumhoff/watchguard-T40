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
 * @file cpa_sample_code_dc_utils.c
 *
 * @defgroup compressionThreads
 *
 * @ingroup compressionThreads
 *
 * @description
 * Contains function prototypes and #defines used throughout code
 * and macros
 *
 ***************************************************************************/

#include "cpa_sample_code_utils_common.h"
#include "cpa_sample_code_dc_perf.h"
#include "cpa_sample_code_crypto_utils.h"
#include "cpa_sample_code_dc_utils.h"

#include "icp_sal_poll.h"


/* Global array of polling threads */
sample_code_thread_t* dcPollingThread_g = NULL;

/* Number of Compression instances enabled for polling */
Cpa32U numDcPolledInstances_g = 0;

/* Global array of instance handles */
CpaInstanceHandle *dcInstances_g = NULL;

/* Number of Compression instances available */
Cpa16U numDcInstances_g = 0;

/* Flag to indicate if the DC services are started */
CpaBoolean dc_service_started_g = CPA_FALSE;

/* Flag to indicate if the DC polling threads have been created */
volatile CpaBoolean dc_polling_started_g = CPA_FALSE;

/* corpus Data structure */
corpus_data_t corpus_g = {0};

/* flag to define weather to use zlib to compress data before decompression*/
CpaBoolean useZlib_g = CPA_FALSE;

/* corpus File structure*/
static corpus_file_t *pCorpusFile_g =NULL;

/* Flag to indicate if the corpus is read into corpus data structure */
static CpaBoolean corpusRead_g = CPA_FALSE;

/* Dynamic Buffer List buffer list used to start DC Services */
CpaBufferList **pBuffList_g = NULL;


CpaStatus populateCorpus(Cpa32U buffSize, corpus_type_t corpusType)
{
    CpaStatus status = CPA_STATUS_SUCCESS;
    Cpa32U numFiles = 0, i = 0;
    char **pCorpusFileNamesArray = NULL;

    char *canterburyFileNames [] =
    {
            /* Single Cantebury corpus file is a cocatenation of the following
             * files:
             "alice29.txt", "asyoulik.txt", "cp.html",
            "fields.c","grammar.lsp", "kennedy.xls", "lcet10.txt" ,
            "plrabn12.txt", "ptt5"
            */
            "canterbury"
     };
    char *calgaryFileNames [] =
    {
            /* Single Calgary corpus file is a cocatenation of the following
             * files:
             *  "bib", "book1", "book2", "geo" , "news", "obj1",
            "obj2", "paper1", "paper2", "paper3", "paper4",
            "paper5", "paper6", "pic", "progc",
            "progl" , "progp" ,"trans"
            */
            "calgary"

    };

    if(corpusRead_g == CPA_FALSE)
    {
        switch(corpusType)
        {
        case CANTERBURY_CORPUS:
            pCorpusFileNamesArray = canterburyFileNames;
            numFiles = sizeof(canterburyFileNames)/sizeof(char *);
            break;
        case CALGARY_CORPUS:
            pCorpusFileNamesArray = calgaryFileNames;
            numFiles = sizeof(calgaryFileNames)/sizeof(char *);
            break;
        default:
            PRINT("Unknown Corpus Specified\n");
            return CPA_STATUS_FAIL;
        }
        /* allocate the memory for the corpus file structure */
        pCorpusFile_g = qaeMemAlloc(numFiles*sizeof(corpus_file_t));
        if(NULL == pCorpusFile_g)
        {
            PRINT_ERR(" Unable to allocate Memory for "
                    "corpus structure\n");
            return CPA_STATUS_FAIL;
        }
        for(i=0; i<numFiles; i++)
        {
            status = getCorpusFile(&pCorpusFile_g[i].corpusBinaryData,
                    pCorpusFileNamesArray[i],
                    &pCorpusFile_g[i].corpusBinaryDataLen);
            if(CPA_STATUS_SUCCESS != status )
            {
                PRINT_ERR("Get \"%s\" Corpus File Failed\n",
                                                    pCorpusFileNamesArray[i]);
                qaeMemFree((void**)&pCorpusFile_g);
                return CPA_STATUS_FAIL;
            }
        }
        corpus_g.fileArray = pCorpusFile_g;
        corpus_g.numFilesInCorpus = numFiles;
        corpusRead_g = CPA_TRUE;
    }
    return status;
}


CpaStatus startDcServices( Cpa32U buffSize, Cpa32U numBuffs)

{
    CpaStatus status = CPA_STATUS_SUCCESS;
    Cpa32U size= 0;
    Cpa32U i = 0, j = 0;
    Cpa32U nodeId = 0;
    Cpa32U nProcessorsOnline = 0;


    /*if the service started flag is false*/
    if(dc_service_started_g == CPA_FALSE)
    {
        /* Get the number of DC Instances */
        status = cpaDcGetNumInstances(&numDcInstances_g);
        /* Check the status */
        if(CPA_STATUS_SUCCESS != status)
        {
            PRINT_ERR("Unable to Get Number of DC instances\n");
            return CPA_STATUS_FAIL;
        }
        /* Check if at least one DC instance are present */
        if( 0 == numDcInstances_g )
        {
            PRINT_ERR(" DC Instances are not present\n");
            return CPA_STATUS_FAIL;
        }
        /* Allocate memory for all the instances */
        dcInstances_g = qaeMemAlloc(sizeof(CpaInstanceHandle)*numDcInstances_g);
        /* Check For NULL */
        if( NULL == dcInstances_g)
        {
            PRINT_ERR(" Unable to allocate memory for Instances \n");
            return CPA_STATUS_FAIL;
        }
        /* Get DC Instances */
        status = cpaDcGetInstances(numDcInstances_g, dcInstances_g);
        /* Check Status */
        if(CPA_STATUS_SUCCESS != status)
        {
            PRINT_ERR("Unable to Get DC instances\n");
            qaeMemFree((void**)&dcInstances_g);
            return CPA_STATUS_FAIL;
        }

        /* Allocate the buffer list pointers to the number of Instances
         * this buffer list list is used only in case of dynamic
         * compression
         */
        pBuffList_g = qaeMemAlloc(numDcInstances_g * sizeof(CpaBufferList *));
        /* Check For NULL */
        if(NULL == pBuffList_g)
        {
            PRINT_ERR("Unable to allocate dynamic buffer List\n");
            qaeMemFree((void**)&dcInstances_g);
            return CPA_STATUS_FAIL;
        }
        /* Start the Loop to create Buffer List for each instance*/
        for(i = 0; i < numDcInstances_g; i++)
        {
            /* get the Node ID for each instance Handle */
            status = sampleCodeDcGetNode(dcInstances_g[i], &nodeId);
            if(CPA_STATUS_SUCCESS != status)
            {
                PRINT_ERR("Unable to get NodeId\n");
                qaeMemFree((void**)&dcInstances_g);
                qaeMemFree((void**)&pBuffList_g);
                return CPA_STATUS_FAIL;
            }
            /* allocate the buffer list memory for the dynamic Buffers */
            pBuffList_g[i] =
                qaeMemAllocNUMA(sizeof(CpaBufferList),
                        nodeId, BYTE_ALIGNMENT_64);
            if( NULL == pBuffList_g[i])
            {
                PRINT_ERR("Unable to allocate Memory for Dynamic Buffer\n");
                qaeMemFree((void**)&dcInstances_g);
                qaeMemFree((void**)&pBuffList_g);
                return CPA_STATUS_FAIL;
            }
            /* get the size of the Private meta data
             * needed to create Buffer List
             */
            status = cpaDcBufferListGetMetaSize(dcInstances_g[i],
                    numBuffs, &size);
            if(CPA_STATUS_SUCCESS != status )
            {
                PRINT_ERR("Get Meta Size Data Failed\n");
                qaeMemFree((void**)&dcInstances_g);
                qaeMemFree((void**)&pBuffList_g);
                return CPA_STATUS_FAIL;
            }
            pBuffList_g[i]->pPrivateMetaData =
                qaeMemAllocNUMA(size, nodeId, BYTE_ALIGNMENT_64);
            if(NULL == pBuffList_g[i]->pPrivateMetaData)
            {
                PRINT(" %s:: Unable to allocate memory for "
                        "pPrivateMetaData\n", __FUNCTION__);
                qaeMemFree((void**)&dcInstances_g);
                qaeMemFree((void**)&pBuffList_g);
                return CPA_STATUS_FAIL;
            }
            pBuffList_g[i]->numBuffers = numBuffs;
            /* allocate flat buffers */
            pBuffList_g[i]->pBuffers =
                qaeMemAllocNUMA((numBuffs * sizeof(CpaFlatBuffer)),
                        nodeId, BYTE_ALIGNMENT_64);
            if(NULL == pBuffList_g[i]->pBuffers )
            {
                PRINT_ERR("Unable to allocate memory for pBuffers\n");
                qaeMemFree((void**)&dcInstances_g);
                qaeMemFree((void**)&pBuffList_g);
                return CPA_STATUS_FAIL;
            }
            for(j = 0; j < numBuffs; j++)
            {
                pBuffList_g[i]->pBuffers[j].pData =
                    qaeMemAllocNUMA((buffSize*EXTRA_BUFFER),
                            nodeId, BYTE_ALIGNMENT_64);
                if(NULL == pBuffList_g[i]->pBuffers[j].pData)
                {
                    PRINT_ERR("Unable to allocate Flat Buffer for "
                            "dynamic buffers\n");
                    qaeMemFree((void**)&dcInstances_g);
                    qaeMemFreeNUMA((void**)&pBuffList_g[i]->pBuffers);
                    qaeMemFree((void**)&pBuffList_g);
                    return CPA_STATUS_FAIL;
                }
                /* When starting the DC Instance, the API expects that the
                 * private meta data should be greater than the dataLength
                 */
                pBuffList_g[i]->pBuffers[j].dataLenInBytes =
                    (buffSize*EXTRA_BUFFER);
            }
            /* Configure memory Configuration Function */
            status = cpaDcSetAddressTranslation(dcInstances_g[i],
                    (CpaVirtualToPhysical)qaeVirtToPhysNUMA);
            if(CPA_STATUS_SUCCESS != status )
            {
                PRINT_ERR("Error setting memory config for instance\n");
                qaeMemFree((void**)&dcInstances_g);
                qaeMemFreeNUMA((void**)&pBuffList_g[i]->pBuffers);
                qaeMemFree((void**)&pBuffList_g);
                return CPA_STATUS_FAIL;
            }
            /* Start DC Instance */
            status = cpaDcStartInstance(dcInstances_g[i], pBuffList_g[i]);
            if(CPA_STATUS_SUCCESS != status)
            {
                PRINT_ERR("Unable to start DC Instance\n");
                qaeMemFree((void**)&dcInstances_g);
                qaeMemFreeNUMA((void**)&pBuffList_g[i]->pBuffers);
                qaeMemFree((void**)&pBuffList_g);
                return CPA_STATUS_FAIL;
            }
        }
        /*set the started flag to true*/
        dc_service_started_g = CPA_TRUE;
    }

    /*determine number of cores on system and limit the number of cores to be
     * used to be the smaller of the numberOf Instances or the number of cores*/
    nProcessorsOnline  = sampleCodeGetNumberOfCpus();
    if(nProcessorsOnline > numDcInstances_g)
    {
        setCoreLimit(numDcInstances_g);
    }
    return status;
}

/*stop all acceleration services*/
CpaStatus stopDcServices(compression_test_params_t *dcSetup)
{
    Cpa32U i = 0, j = 0;
    CpaStatus status = CPA_STATUS_SUCCESS;


    /* set polling flag to default */
    dc_polling_started_g = CPA_FALSE;
    /*stop only if the services is in a started state*/
    if(dc_service_started_g ==CPA_TRUE)
    {
        for(i = 0; i < numDcInstances_g; i++)
        {
            /* Free the Dynamic Buffers allocated
             * while starting DC Services
             */
            for(j = 0; j < pBuffList_g[i]->numBuffers; j++)
            {
                qaeMemFreeNUMA((void**)&pBuffList_g[i]->pBuffers[j].pData);
            }
            qaeMemFreeNUMA((void**)&pBuffList_g[i]->pPrivateMetaData);
            qaeMemFreeNUMA((void**)&pBuffList_g[i]->pBuffers);
           /* free the buffer List*/
           qaeMemFreeNUMA((void**)&pBuffList_g[i]);
                /*stop all instances*/
            cpaDcStopInstance(dcInstances_g[i]);
        }
        qaeMemFree((void**)&pBuffList_g);
        /*set the service started flag to false*/
        dc_service_started_g = CPA_FALSE;
    }
    /* Free the corpus Data */
    if (corpusRead_g == CPA_TRUE)
    {
        if(corpus_g.fileArray !=NULL)
        {
            for(i=0; i<corpus_g.numFilesInCorpus; i++)
            {
                if(NULL != pCorpusFile_g[i].corpusBinaryData)
                {
                    qaeMemFree((void**)&pCorpusFile_g[i].corpusBinaryData);
                    pCorpusFile_g[i].corpusBinaryData = NULL;
                }
            }
            /* Free corpus File Structure */
            qaeMemFree((void**)&pCorpusFile_g);
            corpus_g.fileArray = NULL;
        }
        corpusRead_g = CPA_FALSE;
    }
    /* Wait for all threads_g to complete */
    for (i=0; i < numDcPolledInstances_g; i++)
    {
        sampleCodeThreadJoin(&dcPollingThread_g[i]);

    }
    if(numDcPolledInstances_g > 0)
    {
        qaeMemFree((void**)&dcPollingThread_g);
        numDcPolledInstances_g = 0;
    }
	if ( dcInstances_g != NULL )
	{
        qaeMemFree((void**)&dcInstances_g);
		dcInstances_g = NULL;
	}
    return status;
}

CpaStatus calculateRequireBuffers(compression_test_params_t * dcSetup)
{
    Cpa32U numberOfBuffers = 0, i = 0;

    dcSetup->numberOfBuffers =
        qaeMemAlloc(corpus_g.numFilesInCorpus*sizeof(Cpa32U));
    if(NULL == dcSetup->numberOfBuffers)
    {
        PRINT("Could not allocate memory for dcSetup numberOfBuffers array");
        return CPA_STATUS_FAIL;
    }
    for(i = 0; i < corpus_g.numFilesInCorpus; i++)
    {
        numberOfBuffers =
            pCorpusFile_g[i].corpusBinaryDataLen/dcSetup->bufferSize;
        dcSetup->numberOfBuffers[i] = numberOfBuffers;
    }
    return CPA_STATUS_SUCCESS;
}


CpaStatus dcCreatePollingThreadsIfPollingIsEnabled(void)
{
    CpaInstanceInfo2 *instanceInfo2 = NULL;
    Cpa16U i = 0, j = 0, numCreatedPollingThreads = 0;
    Cpa32U coreAffinity = 0;
    CpaStatus status = CPA_STATUS_SUCCESS;
    if ( CPA_FALSE == dc_polling_started_g )
    {
        instanceInfo2 = qaeMemAlloc(numDcInstances_g *
                sizeof(CpaInstanceInfo2));
        if(NULL == instanceInfo2)
        {
            PRINT_ERR("Failed to allocate memory for pInstanceInfo2");
            return CPA_STATUS_FAIL;
        }
        for(i = 0;i < numDcInstances_g; i++)
        {
            status = cpaDcInstanceGetInfo2(dcInstances_g[i], &instanceInfo2[i]);
            if(CPA_STATUS_SUCCESS != status)
            {
                qaeMemFree((void**)&instanceInfo2);
                return CPA_STATUS_FAIL;
            }
            if(CPA_TRUE == instanceInfo2[i].isPolled)
            {
                numDcPolledInstances_g++;
            }
        }
        if (0 == numDcPolledInstances_g)
        {
            qaeMemFree((void**)&instanceInfo2);
            return CPA_STATUS_SUCCESS;
        }
        dcPollingThread_g =
            qaeMemAlloc(numDcPolledInstances_g * sizeof(sample_code_thread_t));
        if(NULL == dcPollingThread_g)
        {
            PRINT_ERR("Failed to allocate memory for polling threads\n");
            qaeMemFree((void**)&instanceInfo2);
            return CPA_STATUS_FAIL;
        }
        for(i = 0;i < numDcInstances_g; i++)
        {
            if(CPA_TRUE == instanceInfo2[i].isPolled)
            {
                status = sampleCodeThreadCreate(
                        &dcPollingThread_g[numCreatedPollingThreads],
                        NULL,
                        (performance_func_t)sampleCodeDcPoll,
                        dcInstances_g[i]);
                if(status != CPA_STATUS_SUCCESS)
                {
                    PRINT_ERR("Error starting polling thread %d\n", status);
                    /*attempt to stop any started service, we dont check status
                     * as some instances may not have been started and
                     * this might return fail*/
                    qaeMemFree((void**)&instanceInfo2);
                    return CPA_STATUS_FAIL;
                }
                /*loop of the instanceInfo coreAffinity bitmask to find
                 * the core affinity*/
                for(j = 0; j< CPA_MAX_CORES; j++)
                {
                    if(CPA_BITMAP_BIT_TEST(instanceInfo2[i].coreAffinity,j))
                    {
                        coreAffinity = j;
                        break;
                    }
                }
                sampleCodeThreadBind(
                        &dcPollingThread_g[numCreatedPollingThreads],
                        coreAffinity);
                sampleCodeThreadStart(
                        &dcPollingThread_g[numCreatedPollingThreads]);
                numCreatedPollingThreads++;
            }
        }
        qaeMemFree((void**)&instanceInfo2);
        dc_polling_started_g = CPA_TRUE;
    }
    return CPA_STATUS_SUCCESS;
}

CpaStatus dcDpPollNumOperations(perf_data_t *pPerfData,
                CpaInstanceHandle instanceHandle, Cpa64U numOperations)
{
    CpaStatus status = CPA_STATUS_FAIL;

    perf_cycles_t startCycles = 0, totalCycles = 0;
    Cpa32U freq = sampleCodeGetCpuFreq();
    startCycles = sampleCodeTimestamp();

    while( pPerfData->responses != numOperations )
    {
        status = icp_sal_DcPollDpInstance(instanceHandle,0);
        if(CPA_STATUS_FAIL == status)
        {
            PRINT_ERR("Error polling instance\n");
            return CPA_STATUS_FAIL;
        }
        if(CPA_STATUS_RETRY == status)
        {
            AVOID_SOFTLOCKUP;
        }
        totalCycles = (sampleCodeTimestamp() - startCycles);
        if(totalCycles > 0)
        {
            do_div(totalCycles,freq);
        }

        if(totalCycles > SAMPLE_CODE_WAIT_THIRTY_SEC)
        {
            PRINT_ERR("Timeout on polling remaining Operations\n");
            PRINT("Expected %llu responses, revieved %llu\n",
                    (unsigned long long )numOperations,
                    (unsigned long long )pPerfData->responses);
            return CPA_STATUS_FAIL;
        }
    }
    return CPA_STATUS_SUCCESS;
}

CpaStatus waitForSemaphore(perf_data_t *perfData)
{
    Cpa64S responsesReceived = INITIAL_RESPONSE_COUNT;
    CpaStatus status = CPA_STATUS_SUCCESS;

    /*wait for the callback to receive all responses and free the
     * semaphore, or if in sync mode, the semaphore should already be free*/

    while(sampleCodeSemaphoreWait(&perfData->comp,
            SAMPLE_CODE_WAIT_THIRTY_SEC)
            != CPA_STATUS_SUCCESS)
    {
        if(INITIAL_RESPONSE_COUNT != responsesReceived &&
                responsesReceived != perfData->numOperations &&
                responsesReceived == perfData->responses)
        {
            PRINT_ERR("System is not responding\n");
            PRINT("Responses expected/received: %llu/%llu\n",
                    (unsigned long long)perfData->numOperations,
                    (unsigned long long)perfData->responses);
            status = CPA_STATUS_FAIL;
            break;
        }
        else
        {
            responsesReceived = perfData->responses;
        }
    }

    return status;
}

CpaStatus  sampleCodeDcGetNode(CpaInstanceHandle instanceHandle, Cpa32U *node)
{
    CpaStatus status = CPA_STATUS_FAIL;
    CpaInstanceInfo2 pInstanceInfo2;

    status = cpaDcInstanceGetInfo2(instanceHandle, &pInstanceInfo2);
    if(CPA_STATUS_SUCCESS != status)
    {
        PRINT_ERR("Unable to get Node affinity\n");
        return status;
    }
    *node = pInstanceInfo2.nodeAffinity;

    return status;
}

/* Change to a compression callback tag with parameter for poll interval */
void sampleCodeDcPoll(CpaInstanceHandle instanceHandle_in)
{
    CpaStatus status = CPA_STATUS_FAIL;
#ifdef USER_SPACE
    struct timespec reqTime,remTime;
    reqTime.tv_sec = 0;
    reqTime.tv_nsec = DEFAULT_POLL_INTERVAL_NSEC;
#endif
    while(dc_service_started_g == CPA_TRUE)
    {
        /*poll for 0 means process all packets on the ET ring */
        status = icp_sal_DcPollInstance(instanceHandle_in, 0);
        if(CPA_STATUS_SUCCESS == status || CPA_STATUS_RETRY == status)
        {
            /* do nothing */
        }
        else
        {
            PRINT_ERR("WARNING icp_sal_DcPollInstance returned status %d\n",
                    status);
        }
#ifdef USER_SPACE
        nanosleep(&reqTime,&remTime);
#else
        sampleCodeSleepMilliSec(DEFAULT_POLL_INTERVAL_MSEC);
#endif

    }
    sampleCodeThreadExit();
}
#ifndef NEWDISPLAY
CpaStatus dcPrintStats(thread_creation_data_t* data)
{
    perf_cycles_t numOfCycles = {0};
    perf_data_t stats = {0};
    CpaStatus status = CPA_STATUS_SUCCESS;
    Cpa32U i = 0;
    Cpa32U throughput = 0;
    Cpa32U bytesConsumed = 0, bytesProduced = 0;
    compression_test_params_t *dcSetup = (compression_test_params_t*)
                                                            data->setupPtr;


    /* stop DC Services */
    status = stopDcServices(dcSetup);
    if(CPA_STATUS_SUCCESS != status)
    {
        PRINT_ERR("Unable to stop DC services\n");
        return status;
    }

    memset(&stats,0,sizeof(perf_data_t));
    stats.averagePacketSizeInBytes = data->packetSize;

    /* get the longest start time and longest End time
     * from the performance stats structure
     */
    getLongestCycleCount(&stats,data->performanceStats,data->numberOfThreads);

    /* Get the total number of responses, bytes consumed and bytes produced
     * for all the threads */
    for (i=0; i<data->numberOfThreads; i++)
    {
        if(CPA_STATUS_FAIL == data->performanceStats[i]->threadReturnStatus)
        {
            return CPA_STATUS_FAIL;
        }
        stats.retries += data->performanceStats[i]->retries;
        stats.responses += data->performanceStats[i]->responses;
        bytesConsumed += data->performanceStats[i]->bytesConsumedPerLoop;
        bytesProduced += data->performanceStats[i]->bytesProducedPerLoop;;
        clearPerfStats(data->performanceStats[i]);
    }
    /* get the maximum number of cycles Required */
    numOfCycles = (stats.endCyclesTimestamp - stats.startCyclesTimestamp);

    /* Print Statistics */
    dcPrintTestData(dcSetup);
    PRINT("Number of threads      %d\n", data->numberOfThreads);
    PRINT("Total Responses        %llu\n",(unsigned long long)stats.responses);
    PRINT("Total Retries          %u\n",stats.retries);
    PRINT("Clock Cycles Start     %llu\n",stats.startCyclesTimestamp);
    PRINT("Clock Cycles End       %llu\n",stats.endCyclesTimestamp);
    PRINT("Total Cycles           %llu\n", numOfCycles);
    PRINT("CPU Frequency(kHz)     %u\n",sampleCodeGetCpuFreq());

    throughput = getDcThroughput(bytesConsumed,
            numOfCycles, dcSetup->numLoops);
    PRINT("Throughput(Mbps)       %u\n", throughput);

    dcCalculateAndPrintCompressionRatio(bytesConsumed,bytesProduced);

    return status;
}
#endif

#ifndef NEWDISPLAY
void dcPrintTestData(compression_test_params_t* dcSetup)
{
    PRINT("API                    ");
    if(dcSetup->isDpApi)
    {
        PRINT("Data_Plane\n");
    }
    else
    {
        PRINT("Traditional\n");
    }
    PRINT("Session State          ");
    switch(dcSetup->setupData.sessState)
    {
    case(CPA_DC_STATEFUL):
        PRINT("STATEFUL\n");
    break;
    case(CPA_DC_STATELESS):
        PRINT("STATELESS\n");
    break;
    default:
        PRINT("Unsupported        %d\n",dcSetup->setupData.sessState);
        break;
    }

    PRINT("Algorithm              ");
    switch(dcSetup->setupData.compType)
    {
    case(CPA_DC_LZS):
        PRINT("LZS\n");
    break;
    case(CPA_DC_DEFLATE):
        PRINT("DEFLATE\n");
    break;
    default:
        PRINT("Unsupported        %d\n",dcSetup->setupData.compType);
        break;
    }

    PRINT("Huffman Type           ");
    switch(dcSetup->setupData.huffType)
    {
    case(CPA_DC_HT_STATIC):
        PRINT("STATIC\n");
    break;
    case(CPA_DC_HT_FULL_DYNAMIC):
        PRINT("DYNAMIC\n");
    break;
    default:
        PRINT("Unsupported        %d\n",dcSetup->setupData.huffType);
        break;
    }

    PRINT("Mode                   ");
    switch(dcSetup->syncFlag)
    {
    case(CPA_SAMPLE_SYNCHRONOUS):
        PRINT("SYNCHRONOUS\n");
    break;
    case(CPA_SAMPLE_ASYNCHRONOUS):
        PRINT("ASYNCHRONOUS\n");
    break;
    default:
        PRINT("Unsupported %d\n",dcSetup->syncFlag);
        break;
    }
    PRINT("Direction              ");
    switch(dcSetup->dcSessDir)
    {
    case(CPA_DC_DIR_COMPRESS):
        PRINT("COMPRESS");
    break;
    case(CPA_DC_DIR_DECOMPRESS):
        PRINT("DECOMPRESS");
    break;
    case(CPA_DC_DIR_COMBINED):
        PRINT("COMBINED");
    break;
    default:
        PRINT("Unsupported        %d\n",dcSetup->setupData.sessDirection);
        break;
    }
    if(useZlib_g)
    {
        PRINT("(from zLib compressed data\n");
    }
    else
    {
        PRINT("\n");
    }

    PRINT("Packet Size            %d\n",dcSetup->bufferSize);

    PRINT("Compression Level      %d\n",dcSetup->setupData.compLevel);

    PRINT("Corpus                 ");
    switch(dcSetup->corpus)
    {
    case(CANTERBURY_CORPUS):
        PRINT("CANTERBURY_CORPUS\n");
    break;
    case(CALGARY_CORPUS):
        PRINT("CALGARY_CORPUS\n");
    break;
    default:
        PRINT("Unsupported        %d\n",dcSetup->corpus);
        break;
    }
}
#endif

void dcDpSetBytesProducedAndConsumed(CpaDcDpOpData ***opdata,
        perf_data_t *perfData, compression_test_params_t* setup)
{
    Cpa32U i = 0, j = 0;

    for(i=0; i < corpus_g.numFilesInCorpus; i++)
    {
        for(j = 0; j < setup->numberOfBuffers[i]; j++)
        {
            perfData->bytesConsumedPerLoop += opdata[i][j]->results.consumed;
            perfData->bytesProducedPerLoop += opdata[i][j]->results.produced;
        }
    }
}

void dcSetBytesProducedAndConsumed(CpaDcRqResults ***cmpResult,
        perf_data_t *perfData, compression_test_params_t* setup)
{

    Cpa32U i = 0, j = 0;

    for(i=0; i < corpus_g.numFilesInCorpus; i++)
    {
        for(j = 0; j < setup->numberOfBuffers[i]; j++)
        {

            perfData->bytesConsumedPerLoop += cmpResult[i][j]->consumed;
            perfData->bytesProducedPerLoop += cmpResult[i][j]->produced;
        }
    }
}
#ifndef NEWDISPLAY
CpaStatus dcCalculateAndPrintCompressionRatio(Cpa32U bytesConsumed,
        Cpa32U bytesProduced)
{
    Cpa32U ratio = 0, remainder = 0;

    if(0 == bytesConsumed)
    {
        PRINT("Divide by zero error on calculating compression ratio\n");
        return CPA_STATUS_FAIL;
    }
#ifdef USER_SPACE
    PRINT("Compression Ratio      %.02f\n",
                                        ((float)bytesProduced/bytesConsumed));
    return CPA_STATUS_SUCCESS;
#endif

    ratio = bytesProduced * SCALING_FACTOR_1000;
    do_div(ratio,bytesConsumed);
    remainder = ratio % BASE_10;
    ratio = bytesProduced * SCALING_FACTOR_100;
    do_div(ratio,bytesConsumed);
    PRINT("Compression Ratio      %d.%d%%\n",ratio,remainder);
    return CPA_STATUS_SUCCESS;
}
#endif

Cpa32U getDcThroughput(Cpa32U totalBytes, perf_cycles_t cycles,
            Cpa32U numOfLoops)
{
    unsigned long long bytesSent = 0;
    unsigned long long time = cycles;
    unsigned long long rate = 0;
    /* declare frequency in kiloHertz*/
    Cpa32U freq = sampleCodeGetCpuFreq();
    bytesSent = totalBytes;

    /*get time in milli seconds by dividing numberOfClockCycles by frequency
     * in kilohertz ie: cycles/(cycles/millsec) = time (mSec) */
    do_div(time,freq);
    /*check that the sample time was not to small*/
    if(time ==0)
    {
        PRINT_ERR("Sample time is too small to calculate throughput\n");
        return 0;
    }
    /*set rate to be bytesSent, once we perform the do_div rate changes from
     * bytes to bytes/milli second or kiloBytes/second*/
    rate = bytesSent * numOfLoops;
    /*rate in kBps*/
   do_div(rate,time);
   /*check that the rate is high enough to convert to Megabits per second*/
   if(rate ==0)
   {
       PRINT_ERR("no data was sent to calculate throughput\n");
       return 0;
   }
   /* convert Kilobytes/second to Kilobits/second*/
    rate = rate*NUM_BITS_IN_BYTE;
    /*then convert rate from Kilobits/second to Megabits/second*/
    do_div(rate,KILOBITS_IN_MEGABITS);
    return (Cpa32U)rate;
}

/*This function tells the compression sample code to use zLib software to
 * compress the data prior to calling the decompression*/
CpaStatus useZlib(void)
{
#ifdef USE_ZLIB
    useZlib_g = CPA_TRUE;
#else
    PRINT_ERR("Your version of zlib is not on this kernel version\n");
#endif
    return CPA_STATUS_SUCCESS;
}
EXPORT_SYMBOL(useZlib);

/*This function tells the compression sample code to use zLib software to
 * compress the data prior to calling the decompression*/
CpaStatus useAccelCompression(void)
{
    useZlib_g = CPA_FALSE;
    return CPA_STATUS_SUCCESS;
}
EXPORT_SYMBOL(useAccelCompression);
