/*
  This file is provided under a dual BSD/GPLv2 license.  When using or
  redistributing this file, you may do so under either license.

  GPL LICENSE SUMMARY
  Copyright(c) 2014 Intel Corporation.
  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  Contact Information:
  qat-linux@intel.com

  BSD LICENSE
  Copyright(c) 2014 Intel Corporation.
  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef ADF_FW_COUNTERS_H_
#define ADF_FW_COUNTERS_H_

#include <linux/list.h>
#include <linux/rwsem.h>
#include <linux/debugfs.h>
#include "adf_accel_devices.h"
#include "adf_fw_counters.h"

#define FW_COUNTERS_MAX_STR_LEN 64
#define FW_COUNTERS_MAX_KEY_LEN_IN_BYTES FW_COUNTERS_MAX_STR_LEN
#define FW_COUNTERS_MAX_VAL_LEN_IN_BYTES FW_COUNTERS_MAX_STR_LEN
#define FW_COUNTERS_MAX_SECTION_LEN_IN_BYTES FW_COUNTERS_MAX_STR_LEN

struct adf_fw_counters_val {
	char key[FW_COUNTERS_MAX_KEY_LEN_IN_BYTES];
	char val[FW_COUNTERS_MAX_VAL_LEN_IN_BYTES];
	struct list_head list;
};

struct adf_fw_counters_section {
	char name[FW_COUNTERS_MAX_SECTION_LEN_IN_BYTES];
	struct list_head list;
	struct list_head param_head;
};

struct adf_fw_counters_data {
	struct list_head ae_sec_list;
	struct dentry *debug;
	struct rw_semaphore lock;
};

int adf_fw_counters_add(struct adf_accel_dev *accel_dev);
void adf_fw_counters_remove(struct adf_accel_dev *accel_dev);

#endif
