#########################################################################
#
# @par
# This file is provided under a dual BSD/GPLv2 license.  When using or 
#   redistributing this file, you may do so under either license.
# 
#   GPL LICENSE SUMMARY
# 
#   Copyright(c) 2007-2012 Intel Corporation. All rights reserved.
# 
#   This program is free software; you can redistribute it and/or modify 
#   it under the terms of version 2 of the GNU General Public License as
#   published by the Free Software Foundation.
# 
#   This program is distributed in the hope that it will be useful, but 
#   WITHOUT ANY WARRANTY; without even the implied warranty of 
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
#   General Public License for more details.
# 
#   You should have received a copy of the GNU General Public License 
#   along with this program; if not, write to the Free Software 
#   Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
#   The full GNU General Public License is included in this distribution 
#   in the file called LICENSE.GPL.
# 
#   Contact Information:
#   Intel Corporation
# 
#   BSD LICENSE 
# 
#   Copyright(c) 2007-2012 Intel Corporation. All rights reserved.
#   All rights reserved.
# 
#   Redistribution and use in source and binary forms, with or without 
#   modification, are permitted provided that the following conditions 
#   are met:
# 
#     * Redistributions of source code must retain the above copyright 
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright 
#       notice, this list of conditions and the following disclaimer in 
#       the documentation and/or other materials provided with the 
#       distribution.
#     * Neither the name of Intel Corporation nor the names of its 
#       contributors may be used to endorse or promote products derived 
#       from this software without specific prior written permission.
# 
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
#   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
#   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
#   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
#   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
#   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
#   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
#   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
#   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
#   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
#   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# 
# 
#  version: SXXXX.L.0.5.0-46
############################################################################


####################Common variables and definitions########################
# Ensure The ICP_ENV_DIR environmental var is defined.
ifndef ICP_ENV_DIR
$(error ICP_ENV_DIR is undefined. Please set the path to your environment makefile \
        "-> setenv ICP_ENV_DIR <path>")
endif

#Add your project environment Makefile
include $(ICP_ENV_DIR)/environment.mk

#Lac include paths
include $(LAC_DIR)/common.mk

#include the makefile with all the default and common Make variable definitions
include $(ICP_BUILDSYSTEM_PATH)/build_files/common.mk

#Add the name for the executable, Library or Module output definitions
OUTPUT_NAME=init

# List of Source Files to be compiled
ifndef QAT_ONLY
SOURCES=sal_list.c sal_crypto.c sal_compression.c sal_ctrl_services.c sal_create_services.c
else
SOURCES=sal_list.c
EXTRA_CFLAGS += -DQAT_ONLY
endif

#Qat is only in kernel space no need to compile in user space
ifeq ($(ICP_OS_LEVEL),kernel_space)
ifneq ($(ADF_PLATFORM), ACCELDEVVF)
SOURCES+=sal_ctrl_qat.c sal_qat_ctrl.c
endif
endif

#Not possibe to send an Admin msgs from VF
ifeq ($(ADF_PLATFORM), ACCELDEVVF)
EXTRA_CFLAGS += -DPKE_DISABLED
endif

ifdef ICP_B0_SILICON
EXTRA_CFLAGS += -DICP_B0_SILICON
endif

ifdef ICP_HEARTBEAT
EXTRA_CFLAGS += -DICP_HEARTBEAT
endif


#common includes between all supported OSes
INCLUDES+=-I$(ADF_DIR)/include\
	-I$(LAC_DIR)/include\
	-I$(API_DIR)/dc\
	-I$(API_DIR)/pm\
	-I$(LAC_DIR)/src/common/compression/include\
	-I$(LAC_DIR)/src/common/pm/include\
	-I$(LAC_DIR)/src/common/qat_ctrl/include

include $(ICP_ENV_DIR)/$(ICP_OS)_$(ICP_OS_LEVEL).mk

ifeq ($(ICP_OS_LEVEL),user_space)
EXTRA_CFLAGS += -DUSER_SPACE
else
EXTRA_CFLAGS += -DKERNEL_SPACE
endif

ifeq ($(PKE_DISABLED), 1)
EXTRA_CFLAGS += -DPKE_DISABLED
endif

ifeq ($(TRNG_DISABLED), 1)
EXTRA_CFLAGS += -DTRNG_DISABLED
endif

ifeq ($(NRBG_DISABLED), 1)
EXTRA_CFLAGS += -DNRBG_DISABLED
endif

ifeq ($(DRBG_HT_DISABLED), 1)
EXTRA_CFLAGS += -DDRBG_HT_DISABLED
endif

ifeq ($(DRBG_POLL_AND_WAIT), 1)
EXTRA_CFLAGS += -DDRBG_POLL_AND_WAIT
endif

# On the line directly below list the outputs you wish to build for
install: lib_static

###################Include rules makefiles########################
include $(ICP_BUILDSYSTEM_PATH)/build_files/rules.mk
###################End of Rules inclusion#########################
