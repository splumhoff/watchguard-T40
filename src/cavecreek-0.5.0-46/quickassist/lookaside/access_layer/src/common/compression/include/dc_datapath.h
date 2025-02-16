/****************************************************************************
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
 * @file dc_datapath.h
 *
 * @ingroup Dc_DataCompression
 *
 * @description
 *      Definition of the Data Compression datapath parameters.
 *
 *****************************************************************************/
#ifndef DC_DATAPATH_H_
#define DC_DATAPATH_H_

/* Restriction on the source buffer size for compression due to the firmware
 * processing */
#define DC_SRC_BUFFER_MIN_SIZE (15)

/* Restriction on the destination buffer size for compression due to
 * the management of skid buffers in the firmware */
#define DC_DEST_BUFFER_DYN_MIN_SIZE (128)
#define DC_DEST_BUFFER_STA_MIN_SIZE (64)

/* Restriction on the destination buffer size for decompression to avoid a
 * potential constant overflow */
#define DC_DEST_BUFFER_DEC_MIN_SIZE (16)

/* Restriction on the source and destination buffer sizes for compression due
 * to the firmware taking 32 bits parameters. The max size is 2^32-1 */
#define DC_BUFFER_MAX_SIZE (0xFFFFFFFF)

/* Mask used to set the most significant bit to zero */
#define DC_STATE_REGISTER_ZERO_MSB_MASK (0x7F)

/* Mask used to keep only the most significant bit and set the others to zero */
#define DC_STATE_REGISTER_KEEP_MSB_MASK (0x80)

/* Compression state register word containing the parity bit */
#define DC_STATE_REGISTER_PARITY_BIT_WORD (5)

/* Location of the parity bit within the compression state register word */
#define DC_STATE_REGISTER_PARITY_BIT (7)

/* Reserve enough space for dc request parameters  */
#define DC_REQ_PARAMS_MAX_SIZE (sizeof(icp_qat_fw_comp_req_params_t) \
                              + sizeof(icp_qat_fw_trans_req_params_t))

/* Pad out to 64-byte multiple to ensure optimal alignment */
#define DC_REQ_PARAMS_SIZE_PADDED                       \
    LAC_ALIGN_POW2_ROUNDUP(DC_REQ_PARAMS_MAX_SIZE,      \
                           LAC_OPTIMAL_ALIGNMENT_SHIFT)

/**
*******************************************************************************
 * @ingroup cpaDc Data Compression
 *      Compression cookie
 * @description
 *      This cookie stores information for a particular compression perform op.
 *      This includes various user-supplied parameters for the operation which
 *      will be needed in our callback function.
 *      A pointer to this cookie is stored in the opaque data field of the QAT
 *      message so that it can be accessed in the asynchronous callback.
 * @note
 *      The order of the parameters within this structure is important. It needs
 *      to match the order of the parameters in CpaDcDpOpData up to the
 *      pSessionHandle. This allows the correct processing of the callback.
 *****************************************************************************/
typedef struct dc_compression_cookie_s
{
    Cpa8U dcReqParamsBuffer[DC_REQ_PARAMS_SIZE_PADDED];
    /**< Memory block reserved for request parameters
     * NOTE: Field must be correctly aligned in memory for access by QAT engine
     */
    CpaDcRqResults reserved;
    /**< This is only used to correctly align the structure to match the one
     * from the data plane API */
    CpaInstanceHandle dcInstance;
    /**< Compression instance handle */
    CpaDcSessionHandle pSessionHandle;
    /**< Pointer to the session handle */
    icp_qat_fw_comp_req_t request;
    /**< Compression request */
    CpaPhysicalAddr dcReqParamsBufferPhyAddr;
    /**< Physical address of the request parameter */
    void *callbackTag;
    /**< Opaque data supplied by the client */
    dc_session_desc_t* pSessionDesc;
    /**< Pointer to the session descriptor */
    CpaDcFlush flushFlag;
    /**< Flush flag */
    CpaDcRqResults *pResults;
    /**< Pointer to result buffer holding consumed and produced data */
    Cpa32U srcTotalDataLenInBytes;
    /**< Total length of the source data */
    Cpa32U dstTotalDataLenInBytes;
    /**< Total length of the destination data */
    dc_request_dir_t compDecomp;
    /**< Used to know whether the request is compression or decompression.
     * Useful when defining the session as combined */
    Cpa8U srcBuffLastByte;
    /**< Last byte of the source buffer */
#ifdef ICP_B0_SILICON
    CpaBoolean updateHistory;
    /**< Used to know whether to update the history buffer or not */
#endif
    CpaBufferList *pSourceBuffer;
    /**< This property is used to keep a copy of the address of the source
     * buffer in the event we get a -7 error in the callback function */
    CpaBufferList *pDestinationBuffer;
    /**< This property is used to keep a copy of the address of the destination
     * buffer in the event we get a -7 error in the callback function */
} dc_compression_cookie_t;

/**
 *****************************************************************************
 * @ingroup Dc_DataCompression
 *      Callback function called for compression and decompression requests in
 *      asynchronous mode
 *
 * @description
 *      Called to process compression and decompression response messages. This
 *      callback will check for errors, update the statistics and will call the
 *      user callback
 *
 * @param[in]   transHandle     Handle
 * @param[in]   pRespMsg        Response message
 *
 *****************************************************************************/
void
dcCompression_ProcessCallback(icp_comms_trans_handle transHandle,
                              void *pRespMsg);

#endif /*DC_DATAPATH_H_*/
