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
#include <linux/firmware.h>
#include <linux/pci.h>
#include "adf_cfg.h"
#include "adf_accel_devices.h"
#include "adf_common_drv.h"
#include "icp_qat_uclo.h"

int adf_ae_fw_load(struct adf_accel_dev *accel_dev)
{
	struct adf_fw_loader_data *loader_data = accel_dev->fw_loader;
	struct adf_hw_device_data *hw_device = accel_dev->hw_device;
	void *uof_addr, *mmp_addr;
	u32 uof_size, mmp_size;
#ifdef QAT_UIO
	struct adf_accel_pci *pci_info = &accel_dev->accel_pci_dev;
	unsigned long storage_enabled = 0;
	unsigned long pke_disabled = 0;
#ifdef KPT
	u32 kpt_enabled = 0;
#endif
	char uofname[ADF_CFG_MAX_VAL_LEN_IN_BYTES];
	char key[ADF_CFG_MAX_KEY_LEN_IN_BYTES];
	char val[ADF_CFG_MAX_VAL_LEN_IN_BYTES];
#endif

	if (!hw_device->fw_name)
		return 0;

	if (request_firmware(&loader_data->mmp_fw, hw_device->fw_mmp_name,
			     &accel_dev->accel_pci_dev.pci_dev->dev)) {
		dev_err(&GET_DEV(accel_dev), "Failed to load MMP firmware %s\n",
			hw_device->fw_mmp_name);
		return -EFAULT;
	}
	if (request_firmware(&loader_data->uof_fw, hw_device->fw_name,
			     &accel_dev->accel_pci_dev.pci_dev->dev)) {
		dev_err(&GET_DEV(accel_dev), "Failed to load UOF firmware %s\n",
			hw_device->fw_name);
		goto out_err;
	}

	uof_size = loader_data->uof_fw->size;
	uof_addr = (void *)loader_data->uof_fw->data;
	mmp_size = loader_data->mmp_fw->size;
	mmp_addr = (void *)loader_data->mmp_fw->data;
#ifdef QAT_UIO
	if (!adf_cfg_get_param_value(accel_dev, ADF_GENERAL_SEC,
				     ADF_PKE_DISABLED, val)) {
		if (kstrtoul(val, 0, &pke_disabled))
			return -EFAULT;
	}
	if (!pke_disabled) {
		if (qat_uclo_wr_mimage(loader_data->fw_loader, mmp_addr,
				       mmp_size)) {
			dev_err(&GET_DEV(accel_dev), "Failed to load MMP\n");
			goto out_err;
		}
	}
#else
	if (qat_uclo_wr_mimage(loader_data->fw_loader, mmp_addr, mmp_size)) {
		dev_err(&GET_DEV(accel_dev), "Failed to load MMP\n");
		goto out_err;
	}
#endif
#ifdef QAT_UIO
	strlcpy(key, ADF_STORAGE_FIRMWARE_ENABLED, sizeof(key));
	if (!adf_cfg_get_param_value(accel_dev, ADF_GENERAL_SEC, key, val)) {
		if (kstrtoul(val, 0, &storage_enabled))
			return -EFAULT;
	}
#ifdef KPT
	if (hw_device->get_kpt_enabled)
		hw_device->get_kpt_enabled(accel_dev, &kpt_enabled);
#endif
	if (storage_enabled) {
		if (pci_info->pci_dev->device == ADF_DH895XCC_PCI_DEVICE_ID) {
			strlcpy(uofname, ADF_DH895XCC_AE_FW_NAME_STORAGE,
				sizeof(uofname));
		} else {
			strlcpy(uofname, ADF_CXXX_AE_FW_NAME_STORAGE,
				sizeof(uofname));
		}
#ifdef KPT
	} else if (kpt_enabled) {
		if (pci_info->pci_dev->device == ADF_C62X_PCI_DEVICE_ID) {
			strlcpy(uofname, ADF_CXXX_AE_FW_NAME_KPT,
				sizeof(uofname));
		} else {
			dev_err(&GET_DEV(accel_dev), "Failed to support KPT\n");
			return -EFAULT;
		}
#endif
	} else {
		if (pci_info->pci_dev->device == ADF_DH895XCC_PCI_DEVICE_ID) {
			strlcpy(uofname, ADF_DH895XCC_AE_FW_NAME,
				sizeof(uofname));
		} else {
			strlcpy(uofname, ADF_CXXX_AE_FW_NAME, sizeof(uofname));
		}
	}
	if (qat_uclo_map_obj(loader_data->fw_loader, uof_addr,
			     uof_size, uofname)) {
		dev_err(&GET_DEV(accel_dev), "Failed to map FW\n");
		goto out_err;
	}
#else
	if (qat_uclo_map_obj(loader_data->fw_loader, uof_addr, uof_size)) {
		dev_err(&GET_DEV(accel_dev), "Failed to map FW\n");
		goto out_err;
	}
#endif
	if (qat_uclo_wr_all_uimage(loader_data->fw_loader)) {
		dev_err(&GET_DEV(accel_dev), "Failed to load UOF\n");
		goto out_err;
	}
	return 0;

out_err:
	adf_ae_fw_release(accel_dev);
	return -EFAULT;
}

void adf_ae_fw_release(struct adf_accel_dev *accel_dev)
{
	struct adf_fw_loader_data *loader_data = accel_dev->fw_loader;
	struct adf_hw_device_data *hw_device = accel_dev->hw_device;

	if (!hw_device->fw_name)
		return;

	qat_uclo_del_uof_obj(loader_data->fw_loader);
	qat_hal_deinit(loader_data->fw_loader);
	release_firmware(loader_data->uof_fw);
	release_firmware(loader_data->mmp_fw);
	loader_data->uof_fw = NULL;
	loader_data->mmp_fw = NULL;
	loader_data->fw_loader = NULL;
}

int adf_ae_start(struct adf_accel_dev *accel_dev)
{
	struct adf_fw_loader_data *loader_data = accel_dev->fw_loader;
	struct adf_hw_device_data *hw_data = accel_dev->hw_device;
	uint32_t ae_ctr, ae, max_aes = GET_MAX_ACCELENGINES(accel_dev);

	if (!hw_data->fw_name)
		return 0;

	for (ae = 0, ae_ctr = 0; ae < max_aes; ae++) {
		if (hw_data->ae_mask & (1 << ae)) {
			qat_hal_start(loader_data->fw_loader, ae, 0xFF);
			ae_ctr++;
		}
	}
	dev_info(&GET_DEV(accel_dev),
		 "qat_dev%d started %d acceleration engines\n",
		 accel_dev->accel_id, ae_ctr);
	return 0;
}

int adf_ae_stop(struct adf_accel_dev *accel_dev)
{
	struct adf_fw_loader_data *loader_data = accel_dev->fw_loader;
	struct adf_hw_device_data *hw_data = accel_dev->hw_device;
	uint32_t ae_ctr, ae, max_aes = GET_MAX_ACCELENGINES(accel_dev);

	if (!hw_data->fw_name)
		return 0;

	for (ae = 0, ae_ctr = 0; ae < max_aes; ae++) {
		if (hw_data->ae_mask & (1 << ae)) {
			qat_hal_stop(loader_data->fw_loader, ae, 0xFF);
			ae_ctr++;
		}
	}
	dev_info(&GET_DEV(accel_dev),
		 "qat_dev%d stopped %d acceleration engines\n",
		 accel_dev->accel_id, ae_ctr);
	return 0;
}

static int adf_ae_reset(struct adf_accel_dev *accel_dev, int ae)
{
	struct adf_fw_loader_data *loader_data = accel_dev->fw_loader;

	qat_hal_reset(loader_data->fw_loader);
	if (qat_hal_clr_reset(loader_data->fw_loader))
		return -EFAULT;

	return 0;
}

int adf_ae_init(struct adf_accel_dev *accel_dev)
{
	struct adf_fw_loader_data *loader_data;
	struct adf_hw_device_data *hw_device = accel_dev->hw_device;

	if (!hw_device->fw_name)
		return 0;

	loader_data = kzalloc(sizeof(*loader_data), GFP_KERNEL);
	if (!loader_data)
		return -ENOMEM;

	accel_dev->fw_loader = loader_data;
	if (qat_hal_init(accel_dev)) {
		dev_err(&GET_DEV(accel_dev), "Failed to init the AEs\n");
		kfree(loader_data);
		return -EFAULT;
	}
	if (adf_ae_reset(accel_dev, 0)) {
		dev_err(&GET_DEV(accel_dev), "Failed to reset the AEs\n");
		qat_hal_deinit(loader_data->fw_loader);
		kfree(loader_data);
		return -EFAULT;
	}
	return 0;
}

int adf_ae_shutdown(struct adf_accel_dev *accel_dev)
{
	struct adf_fw_loader_data *loader_data = accel_dev->fw_loader;
	struct adf_hw_device_data *hw_device = accel_dev->hw_device;

	if (!hw_device->fw_name)
		return 0;

	qat_hal_deinit(loader_data->fw_loader);
	kfree(accel_dev->fw_loader);
	accel_dev->fw_loader = NULL;
	return 0;
}
