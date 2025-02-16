/*
 * Copyright 2018-2019 NXP
 *
 * SPDX-License-Identifier: GPL-2.0+
 *
 */
#include <linux/clk.h>
#include <linux/irq.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/component.h>
#include <linux/mfd/syscon.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>

#include "imx-hdp.h"
#include "imx-hdmi.h"
#include "imx-dp.h"
#include "../imx-drm.h"

struct drm_display_mode *g_mode;
uint8_t g_default_mode = 3;
static struct drm_display_mode edid_cea_modes[] = {
	/* 3 - 720x480@60Hz */
	{ DRM_MODE("720x480", DRM_MODE_TYPE_DRIVER, 27000, 720, 736,
		   798, 858, 0, 480, 489, 495, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 4 - 1280x720@60Hz */
	{ DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 74250, 1280, 1390,
		   1430, 1650, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 16 - 1920x1080@60Hz */
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 148500, 1920, 2008,
		   2052, 2200, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 97 - 3840x2160@60Hz */
	{ DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 594000,
		   3840, 4016, 4104, 4400, 0,
		   2160, 2168, 2178, 2250, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 60, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
	/* 96 - 3840x2160@30Hz */
	{ DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 297000,
		   3840, 4016, 4104, 4400, 0,
		   2160, 2168, 2178, 2250, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	  .vrefresh = 30, .picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9, },
};

int edid_cea_modes_enabled[ARRAY_SIZE(edid_cea_modes)] = {0};
static inline struct imx_hdp *enc_to_imx_hdp(struct drm_encoder *e)
{
	return container_of(e, struct imx_hdp, encoder);
}

static void imx_hdp_state_init(struct imx_hdp *hdp)
{
	state_struct *state = &hdp->state;

	memset(state, 0, sizeof(state_struct));
	mutex_init(&state->mutex);

	state->mem = &hdp->mem;
	state->rw = hdp->rw;
	state->edp = hdp->is_edp;
}

static void imx8qm_pixel_link_mux(state_struct *state,
				  const struct drm_display_mode *mode)
{
	struct imx_hdp *hdp = state_to_imx_hdp(state);
	u32 val;

	val = 0x4; /* RGB */
	if (mode->flags & DRM_MODE_FLAG_PVSYNC)
		val |= 1 << PL_MUX_CTL_VCP_OFFSET;
	if (mode->flags & DRM_MODE_FLAG_PHSYNC)
		val |= 1 << PL_MUX_CTL_HCP_OFFSET;
	if (mode->flags & DRM_MODE_FLAG_INTERLACE)
		val |= 0x2;

	writel(val, hdp->mem.ss_base + CSR_PIXEL_LINK_MUX_CTL);
}

#ifndef CONFIG_ARCH_LAYERSCAPE
static int imx8qm_pixel_link_validate(state_struct *state)
{
	struct imx_hdp *hdp = state_to_imx_hdp(state);
	sc_err_t sciErr;

	sciErr = sc_ipc_getMuID(&hdp->mu_id);
	if (sciErr != SC_ERR_NONE) {
		DRM_ERROR("Cannot obtain MU ID\n");
		return -EINVAL;
	}

	sciErr = sc_ipc_open(&hdp->ipcHndl, hdp->mu_id);
	if (sciErr != SC_ERR_NONE) {
		DRM_ERROR("sc_ipc_open failed! (sciError = %d)\n",
			  sciErr);
		return -EINVAL;
	}

	sciErr = sc_misc_set_control(hdp->ipcHndl, SC_R_DC_0,
					SC_C_PXL_LINK_MST1_VLD, 1);
	if (sciErr != SC_ERR_NONE) {
		DRM_ERROR("SC_R_DC_0:SC_C_PXL_LINK_MST1_VLD sc_misc_set_control failed! (sciError = %d)\n",
			   sciErr);
		return -EINVAL;
	}

	sc_ipc_close(hdp->mu_id);

	return 0;
}

static int imx8qm_pixel_link_invalidate(state_struct *state)
{
	struct imx_hdp *hdp = state_to_imx_hdp(state);
	sc_err_t sciErr;

	sciErr = sc_ipc_getMuID(&hdp->mu_id);
	if (sciErr != SC_ERR_NONE) {
		DRM_ERROR("Cannot obtain MU ID\n");
		return -EINVAL;
	}

	sciErr = sc_ipc_open(&hdp->ipcHndl, hdp->mu_id);
	if (sciErr != SC_ERR_NONE) {
		DRM_ERROR("sc_ipc_open failed! (sciError = %d)\n", sciErr);
		return -EINVAL;
	}

	sciErr = sc_misc_set_control(hdp->ipcHndl, SC_R_DC_0,
				     SC_C_PXL_LINK_MST1_VLD, 0);
	if (sciErr != SC_ERR_NONE) {
		DRM_ERROR("SC_R_DC_0:SC_C_PXL_LINK_MST1_VLD sc_misc_set_control failed! (sciError = %d)\n", sciErr);
		return -EINVAL;
	}

	sc_ipc_close(hdp->mu_id);

	return 0;
}

static int imx8qm_pixel_link_sync_ctrl_enable(state_struct *state)
{
	struct imx_hdp *hdp = state_to_imx_hdp(state);
	sc_err_t sciErr;

	sciErr = sc_ipc_getMuID(&hdp->mu_id);
	if (sciErr != SC_ERR_NONE) {
		DRM_ERROR("Cannot obtain MU ID\n");
		return -EINVAL;
	}

	sciErr = sc_ipc_open(&hdp->ipcHndl, hdp->mu_id);
	if (sciErr != SC_ERR_NONE) {
		DRM_ERROR("sc_ipc_open failed! (sciError = %d)\n", sciErr);
		return -EINVAL;
	}

	if (hdp->dual_mode) {
		sciErr = sc_misc_set_control(hdp->ipcHndl, SC_R_DC_0, SC_C_SYNC_CTRL, 3);
		if (sciErr != SC_ERR_NONE) {
			DRM_ERROR("SC_R_DC_0:SC_C_SYNC_CTRL sc_misc_set_control failed! (sciError = %d)\n", sciErr);
			return -EINVAL;
		}
	} else {
		sciErr = sc_misc_set_control(hdp->ipcHndl, SC_R_DC_0, SC_C_SYNC_CTRL0, 1);
		if (sciErr != SC_ERR_NONE) {
			DRM_ERROR("SC_R_DC_0:SC_C_SYNC_CTRL0 sc_misc_set_control failed! (sciError = %d)\n", sciErr);
			return -EINVAL;
		}
	}

	sc_ipc_close(hdp->mu_id);

	return 0;
}

static int imx8qm_pixel_link_sync_ctrl_disable(state_struct *state)
{
	struct imx_hdp *hdp = state_to_imx_hdp(state);
	sc_err_t sciErr;

	sciErr = sc_ipc_getMuID(&hdp->mu_id);
	if (sciErr != SC_ERR_NONE) {
		DRM_ERROR("Cannot obtain MU ID\n");
		return -EINVAL;
	}

	sciErr = sc_ipc_open(&hdp->ipcHndl, hdp->mu_id);
	if (sciErr != SC_ERR_NONE) {
		DRM_ERROR("sc_ipc_open failed! (sciError = %d)\n", sciErr);
		return -EINVAL;
	}


	if (hdp->dual_mode) {
		sciErr = sc_misc_set_control(hdp->ipcHndl, SC_R_DC_0, SC_C_SYNC_CTRL, 0);
		if (sciErr != SC_ERR_NONE) {
			DRM_ERROR("SC_R_DC_0:SC_C_SYNC_CTRL sc_misc_set_control failed! (sciError = %d)\n", sciErr);
			return -EINVAL;
		}
	} else {
		sciErr = sc_misc_set_control(hdp->ipcHndl, SC_R_DC_0, SC_C_SYNC_CTRL0, 0);
		if (sciErr != SC_ERR_NONE) {
			DRM_ERROR("SC_R_DC_0:SC_C_SYNC_CTRL0 sc_misc_set_control failed! (sciError = %d)\n", sciErr);
			return -EINVAL;
		}
	}

	sc_ipc_close(hdp->mu_id);

	return 0;
}

void imx8qm_phy_reset(sc_ipc_t ipcHndl, struct hdp_mem *mem, u8 reset)
{
	sc_err_t sciErr;
	/* set the pixel link mode and pixel type */
	sciErr = sc_misc_set_control(ipcHndl, SC_R_HDMI, SC_C_PHY_RESET, reset);
	if (sciErr != SC_ERR_NONE)
		DRM_ERROR("SC_R_HDMI PHY reset failed %d!\n", sciErr);
}

void imx8mq_phy_reset(sc_ipc_t ipcHndl, struct hdp_mem *mem, u8 reset)
{
	void *tmp_addr = mem->rst_base;

	if (reset)
		__raw_writel(0x8,
			     (volatile unsigned int *)(tmp_addr+0x4)); /*set*/
	else
		__raw_writel(0x8,
			     (volatile unsigned int *)(tmp_addr+0x8)); /*clear*/


	return;
}
#endif

static const struct of_device_id scfg_device_ids[] = {
	{ .compatible = "fsl,ls1028a-scfg", },
	{}
};

void ls1028a_phy_reset(uint32_t ipcHndl, struct hdp_mem *mem, u8 reset)
{
	struct device_node *scfg_node;
	void __iomem *scfg_base = NULL;

	scfg_node = of_find_matching_node(NULL, scfg_device_ids);
	if (scfg_node)
		scfg_base = of_iomap(scfg_node, 0);

	iowrite32(reset, scfg_base + EDP_PHY_RESET);
}

int ls1028a_clock_init(struct hdp_clks *clks)
{
	struct imx_hdp *hdp = clks_to_imx_hdp(clks);
	struct device *dev = hdp->dev;

	clks->clk_ipg = devm_clk_get(dev, "clk_ipg");
	if (IS_ERR(clks->clk_ipg)) {
		dev_warn(dev, "failed to get dp ipg clk\n");
		return PTR_ERR(clks->clk_ipg);
	}

	clks->clk_core = devm_clk_get(dev, "clk_core");
	if (IS_ERR(clks->clk_core)) {
		dev_warn(dev, "failed to get hdp core clk\n");
		return PTR_ERR(clks->clk_core);
	}

	clks->clk_pxl = devm_clk_get(dev, "clk_pxl");
	if (IS_ERR(clks->clk_pxl)) {
		dev_warn(dev, "failed to get pxl clk\n");
		return PTR_ERR(clks->clk_pxl);
	}

	clks->clk_pxl_mux = devm_clk_get(dev, "clk_pxl_mux");
	if (IS_ERR(clks->clk_pxl_mux)) {
		dev_warn(dev, "failed to get pxl mux clk\n");
		return PTR_ERR(clks->clk_pxl_mux);
	}

	clks->clk_pxl_link = devm_clk_get(dev, "clk_pxl_link");
	if (IS_ERR(clks->clk_pxl_mux)) {
		dev_warn(dev, "failed to get pxl link clk\n");
		return PTR_ERR(clks->clk_pxl_link);
	}

	if (IS_ERR(clks->clk_apb)) {
		dev_warn(dev, "failed to get apb clk\n");
		return PTR_ERR(clks->clk_apb);
	}

	clks->clk_vif = devm_clk_get(dev, "clk_vif");
	if (IS_ERR(clks->clk_vif)) {
		dev_warn(dev, "failed to get vif clk\n");
		return PTR_ERR(clks->clk_vif);
	}

	return true;

}

int ls1028a_pixel_clock_enable(struct hdp_clks *clks)
{
	struct imx_hdp *hdp = clks_to_imx_hdp(clks);
	struct device *dev = hdp->dev;
	int ret;

	ret = clk_prepare_enable(clks->clk_pxl);
	if (ret < 0) {
		dev_err(dev, "%s, pre clk pxl error\n", __func__);
		return ret;
	}
	ret = clk_prepare_enable(clks->clk_pxl_mux);
	if (ret < 0) {
		dev_err(dev, "%s, pre clk pxl mux error\n", __func__);
		return ret;
	}

	ret = clk_prepare_enable(clks->clk_pxl_link);
	if (ret < 0) {
		dev_err(dev, "%s, pre clk pxl link error\n", __func__);
		return ret;
	}
	ret = clk_prepare_enable(clks->clk_vif);
	if (ret < 0) {
		dev_err(dev, "%s, pre clk vif error\n", __func__);
		return ret;
	}
	return ret;

}

void ls1028a_pixel_clock_disable(struct hdp_clks *clks)
{
	clk_disable_unprepare(clks->clk_vif);
	clk_disable_unprepare(clks->clk_pxl);
	clk_disable_unprepare(clks->clk_pxl_link);
	clk_disable_unprepare(clks->clk_pxl_mux);
}

int ls1028a_ipg_clock_enable(struct hdp_clks *clks)
{
	int ret;
	struct imx_hdp *hdp = clks_to_imx_hdp(clks);
	struct device *dev = hdp->dev;

	ret = clk_prepare_enable(clks->clk_ipg);
	if (ret < 0) {
		dev_err(dev, "%s, pre clk_ipg error\n", __func__);
		return ret;
	}

	ret = clk_prepare_enable(clks->clk_apb);
	if (ret < 0) {
		dev_err(dev, "%s, pre clk apb error\n", __func__);
		return ret;
	}
	return ret;
}

void ls1028a_ipg_clock_disable(struct hdp_clks *clks)
{
}

int imx8qm_clock_init(struct hdp_clks *clks)
{
	struct imx_hdp *hdp = clks_to_imx_hdp(clks);
	struct device *dev = hdp->dev;

	clks->av_pll = devm_clk_get(dev, "av_pll");
	if (IS_ERR(clks->av_pll)) {
		dev_warn(dev, "failed to get av pll clk\n");
		return PTR_ERR(clks->av_pll);
	}

	clks->dig_pll = devm_clk_get(dev, "dig_pll");
	if (IS_ERR(clks->dig_pll)) {
		dev_warn(dev, "failed to get dig pll clk\n");
		return PTR_ERR(clks->dig_pll);
	}

	clks->clk_ipg = devm_clk_get(dev, "clk_ipg");
	if (IS_ERR(clks->clk_ipg)) {
		dev_warn(dev, "failed to get dp ipg clk\n");
		return PTR_ERR(clks->clk_ipg);
	}

	clks->clk_core = devm_clk_get(dev, "clk_core");
	if (IS_ERR(clks->clk_core)) {
		dev_warn(dev, "failed to get hdp core clk\n");
		return PTR_ERR(clks->clk_core);
	}

	clks->clk_pxl = devm_clk_get(dev, "clk_pxl");
	if (IS_ERR(clks->clk_pxl)) {
		dev_warn(dev, "failed to get pxl clk\n");
		return PTR_ERR(clks->clk_pxl);
	}

	clks->clk_pxl_mux = devm_clk_get(dev, "clk_pxl_mux");
	if (IS_ERR(clks->clk_pxl_mux)) {
		dev_warn(dev, "failed to get pxl mux clk\n");
		return PTR_ERR(clks->clk_pxl_mux);
	}

	clks->clk_pxl_link = devm_clk_get(dev, "clk_pxl_link");
	if (IS_ERR(clks->clk_pxl_mux)) {
		dev_warn(dev, "failed to get pxl link clk\n");
		return PTR_ERR(clks->clk_pxl_link);
	}

	clks->clk_hdp = devm_clk_get(dev, "clk_hdp");
	if (IS_ERR(clks->clk_hdp)) {
		dev_warn(dev, "failed to get hdp clk\n");
		return PTR_ERR(clks->clk_hdp);
	}

	clks->clk_phy = devm_clk_get(dev, "clk_phy");
	if (IS_ERR(clks->clk_phy)) {
		dev_warn(dev, "failed to get phy clk\n");
		return PTR_ERR(clks->clk_phy);
	}
	clks->clk_apb = devm_clk_get(dev, "clk_apb");
	if (IS_ERR(clks->clk_apb)) {
		dev_warn(dev, "failed to get apb clk\n");
		return PTR_ERR(clks->clk_apb);
	}
	clks->clk_lis = devm_clk_get(dev, "clk_lis");
	if (IS_ERR(clks->clk_lis)) {
		dev_warn(dev, "failed to get lis clk\n");
		return PTR_ERR(clks->clk_lis);
	}
	clks->clk_msi = devm_clk_get(dev, "clk_msi");
	if (IS_ERR(clks->clk_msi)) {
		dev_warn(dev, "failed to get msi clk\n");
		return PTR_ERR(clks->clk_msi);
	}
	clks->clk_lpcg = devm_clk_get(dev, "clk_lpcg");
	if (IS_ERR(clks->clk_lpcg)) {
		dev_warn(dev, "failed to get lpcg clk\n");
		return PTR_ERR(clks->clk_lpcg);
	}
	clks->clk_even = devm_clk_get(dev, "clk_even");
	if (IS_ERR(clks->clk_even)) {
		dev_warn(dev, "failed to get even clk\n");
		return PTR_ERR(clks->clk_even);
	}
	clks->clk_dbl = devm_clk_get(dev, "clk_dbl");
	if (IS_ERR(clks->clk_dbl)) {
		dev_warn(dev, "failed to get dbl clk\n");
		return PTR_ERR(clks->clk_dbl);
	}
	clks->clk_vif = devm_clk_get(dev, "clk_vif");
	if (IS_ERR(clks->clk_vif)) {
		dev_warn(dev, "failed to get vif clk\n");
		return PTR_ERR(clks->clk_vif);
	}
	clks->clk_apb_csr = devm_clk_get(dev, "clk_apb_csr");
	if (IS_ERR(clks->clk_apb_csr)) {
		dev_warn(dev, "failed to get apb csr clk\n");
		return PTR_ERR(clks->clk_apb_csr);
	}
	clks->clk_apb_ctrl = devm_clk_get(dev, "clk_apb_ctrl");
	if (IS_ERR(clks->clk_apb_ctrl)) {
		dev_warn(dev, "failed to get apb ctrl clk\n");
		return PTR_ERR(clks->clk_apb_ctrl);
	}

	return true;
}

int imx8qm_pixel_clock_enable(struct hdp_clks *clks)
{
	struct imx_hdp *hdp = clks_to_imx_hdp(clks);
	struct device *dev = hdp->dev;
	int ret;

	ret = clk_prepare_enable(clks->av_pll);
	if (ret < 0) {
		dev_err(dev, "%s, pre av pll  error\n", __func__);
		return ret;
	}

	ret = clk_prepare_enable(clks->clk_pxl);
	if (ret < 0) {
		dev_err(dev, "%s, pre clk pxl error\n", __func__);
		return ret;
	}
	ret = clk_prepare_enable(clks->clk_pxl_mux);
	if (ret < 0) {
		dev_err(dev, "%s, pre clk pxl mux error\n", __func__);
		return ret;
	}

	ret = clk_prepare_enable(clks->clk_pxl_link);
	if (ret < 0) {
		dev_err(dev, "%s, pre clk pxl link error\n", __func__);
		return ret;
	}
	ret = clk_prepare_enable(clks->clk_vif);
	if (ret < 0) {
		dev_err(dev, "%s, pre clk vif error\n", __func__);
		return ret;
	}
	return ret;

}

void imx8qm_pixel_clock_disable(struct hdp_clks *clks)
{
	clk_disable_unprepare(clks->clk_vif);
	clk_disable_unprepare(clks->clk_pxl);
	clk_disable_unprepare(clks->clk_pxl_link);
	clk_disable_unprepare(clks->clk_pxl_mux);
	clk_disable_unprepare(clks->av_pll);
}

void imx8qm_dp_pixel_clock_set_rate(struct hdp_clks *clks)
{
	struct imx_hdp *hdp = clks_to_imx_hdp(clks);
	unsigned int pclock = hdp->video.cur_mode.clock * 1000;

	if (hdp->dual_mode == true) {
		clk_set_rate(clks->clk_pxl, pclock/2);
		clk_set_rate(clks->clk_pxl_link, pclock/2);
	} else {
		clk_set_rate(clks->clk_pxl, pclock);
		clk_set_rate(clks->clk_pxl_link, pclock);
	}
	clk_set_rate(clks->clk_pxl_mux, pclock);
}

void imx8qm_hdmi_pixel_clock_set_rate(struct hdp_clks *clks)
{
	struct imx_hdp *hdp = clks_to_imx_hdp(clks);
	unsigned int pclock = hdp->video.cur_mode.clock * 1000;

	/* pixel clock for HDMI */
	clk_set_rate(clks->av_pll, pclock);

	if (hdp->dual_mode == true) {
		clk_set_rate(clks->clk_pxl, pclock/2);
		clk_set_rate(clks->clk_pxl_link, pclock/2);
	} else {
		clk_set_rate(clks->clk_pxl_link, pclock);
		clk_set_rate(clks->clk_pxl, pclock);
	}
	clk_set_rate(clks->clk_pxl_mux, pclock);
}

int imx8qm_ipg_clock_enable(struct hdp_clks *clks)
{
	int ret;
	struct imx_hdp *hdp = clks_to_imx_hdp(clks);
	struct device *dev = hdp->dev;

	ret = clk_prepare_enable(clks->dig_pll);
	if (ret < 0) {
		dev_err(dev, "%s, pre dig pll error\n", __func__);
		return ret;
	}

	ret = clk_prepare_enable(clks->clk_ipg);
	if (ret < 0) {
		dev_err(dev, "%s, pre clk_ipg error\n", __func__);
		return ret;
	}

	ret = clk_prepare_enable(clks->clk_core);
	if (ret < 0) {
		dev_err(dev, "%s, pre clk core error\n", __func__);
		return ret;
	}

	ret = clk_prepare_enable(clks->clk_hdp);
	if (ret < 0) {
		dev_err(dev, "%s, pre clk hdp error\n", __func__);
		return ret;
	}

	ret = clk_prepare_enable(clks->clk_phy);
	if (ret < 0) {
		dev_err(dev, "%s, pre clk phy\n", __func__);
		return ret;
	}

	ret = clk_prepare_enable(clks->clk_apb);
	if (ret < 0) {
		dev_err(dev, "%s, pre clk apb error\n", __func__);
		return ret;
	}
	ret = clk_prepare_enable(clks->clk_lis);
	if (ret < 0) {
		dev_err(dev, "%s, pre clk lis error\n", __func__);
		return ret;
	}
	ret = clk_prepare_enable(clks->clk_lpcg);
	if (ret < 0) {
		dev_err(dev, "%s, pre clk lpcg error\n", __func__);
		return ret;
	}
	ret = clk_prepare_enable(clks->clk_msi);
	if (ret < 0) {
		dev_err(dev, "%s, pre clk msierror\n", __func__);
		return ret;
	}
	ret = clk_prepare_enable(clks->clk_even);
	if (ret < 0) {
		dev_err(dev, "%s, pre clk even error\n", __func__);
		return ret;
	}
	ret = clk_prepare_enable(clks->clk_dbl);
	if (ret < 0) {
		dev_err(dev, "%s, pre clk dbl error\n", __func__);
		return ret;
	}
	ret = clk_prepare_enable(clks->clk_apb_csr);
	if (ret < 0) {
		dev_err(dev, "%s, pre clk apb csr error\n", __func__);
		return ret;
	}
	ret = clk_prepare_enable(clks->clk_apb_ctrl);
	if (ret < 0) {
		dev_err(dev, "%s, pre clk apb ctrl error\n", __func__);
		return ret;
	}
	return ret;
}

void imx8qm_ipg_clock_disable(struct hdp_clks *clks)
{
}

void imx8qm_ipg_clock_set_rate(struct hdp_clks *clks)
{
	struct imx_hdp *hdp = clks_to_imx_hdp(clks);
	u32 clk_rate, desired_rate;

	if (hdp->is_digpll_dp_pclock)
		desired_rate = PLL_1188MHZ;
	else
#ifndef CONFIG_ARCH_LAYERSCAPE
		desired_rate =  PLL_800MHZ;
#else
		desired_rate = PLL_675MHZ;
#endif

	/* hdmi/dp ipg/core clock */
	clk_rate = clk_get_rate(clks->dig_pll);

	if (clk_rate != desired_rate) {
		pr_warn("%s, dig_pll was %u MHz, changing to %u MHz\n",
			__func__, clk_rate/1000000,
			desired_rate/1000000);
	}

	if (hdp->is_digpll_dp_pclock) {
		clk_set_rate(clks->dig_pll,  desired_rate);
		clk_set_rate(clks->clk_core, desired_rate/10);
		clk_set_rate(clks->clk_ipg,  desired_rate/12);
		clk_set_rate(clks->av_pll, 24000000);
	} else {
		clk_set_rate(clks->dig_pll,  desired_rate);
		clk_set_rate(clks->clk_core, desired_rate/5);
		clk_set_rate(clks->clk_ipg,  desired_rate/8);
	}
}

static u8 imx_hdp_link_rate(struct drm_display_mode *mode)
{
#ifndef CONFIG_ARCH_LAYERSCAPE
	if (mode->clock < 297000)
#else
	if (mode->clock < 74250)
#endif
		return AFE_LINK_RATE_1_6;
	else if (mode->clock > 297000)
		return AFE_LINK_RATE_5_4;
	else
		return AFE_LINK_RATE_2_7;
}

static void imx_hdp_mode_setup(struct imx_hdp *hdp,
			       const struct drm_display_mode *mode)
{
	int ret;

	/* set pixel clock before video mode setup */
	imx_hdp_call(hdp, pixel_clock_disable, &hdp->clks);

	imx_hdp_call(hdp, pixel_clock_set_rate, &hdp->clks);

	imx_hdp_call(hdp, pixel_clock_enable, &hdp->clks);

	/* Config pixel link mux */
	imx_hdp_call(hdp, pixel_link_mux, &hdp->state, mode);

	/* mode set */
	ret = imx_hdp_call(hdp, phy_init, &hdp->state, mode, hdp->format,
			   hdp->bpc);
	if (ret < 0) {
		DRM_ERROR("Failed to initialise HDP PHY\n");
		return;
	}
	imx_hdp_call(hdp, mode_set, &hdp->state, mode,
		     hdp->format, hdp->bpc, hdp->link_rate);

	/* Get vic of CEA-861 */
	hdp->vic = drm_match_cea_mode(mode);
}

static void imx_hdp_bridge_mode_set(struct drm_bridge *bridge,
				    struct drm_display_mode *orig_mode,
				    struct drm_display_mode *mode)
{
	struct imx_hdp *hdp = bridge->driver_private;

	mutex_lock(&hdp->mutex);

	memcpy(&hdp->video.cur_mode, mode, sizeof(hdp->video.cur_mode));
	imx_hdp_mode_setup(hdp, mode);
	/* Store the display mode for plugin/DKMS poweron events */
	memcpy(&hdp->video.pre_mode, mode, sizeof(hdp->video.pre_mode));

	mutex_unlock(&hdp->mutex);
}

static void imx_hdp_bridge_disable(struct drm_bridge *bridge)
{
}

static void imx_hdp_bridge_enable(struct drm_bridge *bridge)
{
	struct imx_hdp *hdp = bridge->driver_private;

	/*
	 * When switching from 10-bit to 8-bit color depths, iMX8MQ needs the
	 * PHY pixel engine to be reset after all clocks are ON, not before.
	 * So, we do it in the enable callback.
	 *
	 * Since the reset does not do any harm when switching from a 8-bit mode
	 * to another 8-bit mode, or from 8-bit to 10-bit, we can safely do it
	 * all the time.
	 */
#ifndef CONFIG_ARCH_LAYERSCAPE
	if (cpu_is_imx8mq())
#endif
		imx_hdp_call(hdp, pixel_engine_reset, &hdp->state);
}

static enum drm_connector_status
imx_hdp_connector_detect(struct drm_connector *connector, bool force)
{
	struct imx_hdp *hdp = container_of(connector,
						struct imx_hdp, connector);
	int ret;
	u8 hpd = 0xf;

	ret = imx_hdp_call(hdp, get_hpd_state, &hdp->state, &hpd);
	if (ret > 0)
		return connector_status_unknown;

	if (hpd == 1)
		/* Cable Connected */
		return connector_status_connected;
	else if (hpd == 0)
		/* Cable Disconnedted */
		return connector_status_disconnected;
	else {
		/* Cable status unknown */
		DRM_INFO("Unknow cable status, hdp=%u\n", hpd);
		return connector_status_unknown;
	}
}

static int imx_hdp_default_video_modes(struct drm_connector *connector)
{
	struct drm_display_mode *mode;
#ifdef CONFIG_ARCH_LAYERSCAPE
	struct imx_hdp *hdp = container_of(connector, struct imx_hdp,
					   connector);
#endif
	int i;

	for (i = 0; i < ARRAY_SIZE(edid_cea_modes); i++) {
		mode = drm_mode_create(connector->dev);
		if (!mode)
			return -EINVAL;
		drm_mode_copy(mode, &edid_cea_modes[i]);
#ifdef CONFIG_ARCH_LAYERSCAPE
		if (hdp->num_res != 0 && edid_cea_modes_enabled[i] == 0)
			continue;
#endif
		mode->type |= DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
		drm_mode_probed_add(connector, mode);
	}
	return i;
}

static int imx_hdp_connector_get_modes(struct drm_connector *connector)
{
	struct imx_hdp *hdp = container_of(connector, struct imx_hdp,
					   connector);
	struct edid *edid;
	int num_modes = 0;

	if (!hdp->no_edid) {
		edid = drm_do_get_edid(connector, hdp->ops->get_edid_block,
				       &hdp->state);
		if (edid) {
			dev_info(hdp->dev, "%x,%x,%x,%x,%x,%x,%x,%x\n",
				 edid->header[0], edid->header[1],
				 edid->header[2], edid->header[3],
				 edid->header[4], edid->header[5],
				 edid->header[6], edid->header[7]);
                        drm_mode_connector_update_edid_property(connector,
								edid);
			num_modes = drm_add_edid_modes(connector, edid);
			if (num_modes == 0) {
				dev_warn(hdp->dev, "Invalid edid, use default video modes\n");
				num_modes =
					imx_hdp_default_video_modes(connector);
			}
			kfree(edid);
		} else {
			dev_info(hdp->dev, "failed to get edid, use default video modes\n");
			num_modes = imx_hdp_default_video_modes(connector);
		}
	} else {
		dev_warn(hdp->dev,
			 "No EDID function, use default video mode\n");
		num_modes = imx_hdp_default_video_modes(connector);
	}

	return num_modes;
}

static enum drm_mode_status
imx_hdp_connector_mode_valid(struct drm_connector *connector,
			     struct drm_display_mode *mode)
{
	enum drm_mode_status mode_status = MODE_OK;

	if (mode->clock > 594000)
		return MODE_CLOCK_HIGH;

	return mode_status;
}

static void imx_hdp_connector_force(struct drm_connector *connector)
{
	struct imx_hdp *hdp = container_of(connector, struct imx_hdp,
					     connector);
	mutex_lock(&hdp->mutex);
	hdp->force = connector->force;
	mutex_unlock(&hdp->mutex);
}

static const struct drm_connector_funcs imx_hdp_connector_funcs = {
	.fill_modes = drm_helper_probe_single_connector_modes,
	.detect = imx_hdp_connector_detect,
	.destroy = drm_connector_cleanup,
	.force = imx_hdp_connector_force,
	.reset = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
};

static const struct drm_connector_helper_funcs
imx_hdp_connector_helper_funcs = {
	.get_modes = imx_hdp_connector_get_modes,
	.mode_valid = imx_hdp_connector_mode_valid,
};

static const struct drm_bridge_funcs imx_hdp_bridge_funcs = {
	.enable = imx_hdp_bridge_enable,
	.disable = imx_hdp_bridge_disable,
	.mode_set = imx_hdp_bridge_mode_set,
};


static void imx_hdp_imx_encoder_disable(struct drm_encoder *encoder)
{
}

static void imx_hdp_imx_encoder_enable(struct drm_encoder *encoder)
{
}

static int imx_hdp_imx_encoder_atomic_check(struct drm_encoder *encoder,
				    struct drm_crtc_state *crtc_state,
				    struct drm_connector_state *conn_state)
{
	struct imx_crtc_state *imx_crtc_state = to_imx_crtc_state(crtc_state);

	imx_crtc_state->bus_format = MEDIA_BUS_FMT_RGB101010_1X30;
	return 0;
}

static const struct drm_encoder_helper_funcs
imx_hdp_imx_encoder_helper_funcs = {
	.enable     = imx_hdp_imx_encoder_enable,
	.disable    = imx_hdp_imx_encoder_disable,
	.atomic_check = imx_hdp_imx_encoder_atomic_check,
};

static const struct drm_encoder_funcs imx_hdp_imx_encoder_funcs = {
	.destroy = drm_encoder_cleanup,
};

static int imx8mq_hdp_read(struct hdp_mem *mem,
			   unsigned int addr,
			   unsigned int *value)
{
	unsigned int temp;
	void *tmp_addr;

	mutex_lock(&mem->mutex);
	tmp_addr = mem->regs_base + addr;
	temp = __raw_readl((volatile unsigned int *)tmp_addr);
	*value = temp;
	mutex_unlock(&mem->mutex);
	return 0;
}

static int imx8mq_hdp_write(struct hdp_mem *mem,
			    unsigned int addr,
			    unsigned int value)
{
	void *tmp_addr;

	mutex_lock(&mem->mutex);
	tmp_addr = mem->regs_base + addr;
	__raw_writel(value, (volatile unsigned int *)tmp_addr);
	mutex_unlock(&mem->mutex);
	return 0;
}

static int imx8mq_hdp_sread(struct hdp_mem *mem,
			    unsigned int addr,
			    unsigned int *value)
{
	unsigned int temp;
	void *tmp_addr;

	mutex_lock(&mem->mutex);
	tmp_addr = mem->ss_base + addr;
	temp = __raw_readl((volatile unsigned int *)tmp_addr);
	*value = temp;
	mutex_unlock(&mem->mutex);
	return 0;
}

static int imx8mq_hdp_swrite(struct hdp_mem *mem,
			     unsigned int addr,
			     unsigned int value)
{
	void *tmp_addr;

	mutex_lock(&mem->mutex);
	tmp_addr = mem->ss_base + addr;
	__raw_writel(value, (volatile unsigned int *)tmp_addr);
	mutex_unlock(&mem->mutex);
	return 0;
}

static int imx8qm_hdp_read(struct hdp_mem *mem,
			   unsigned int addr,
			   unsigned int *value)
{
	unsigned int temp;
	void *tmp_addr;
	void *off_addr;

	mutex_lock(&mem->mutex);
	tmp_addr = (addr & 0xfff) + mem->regs_base;
	off_addr = 0x8 + mem->ss_base;
	__raw_writel(addr >> 12, off_addr);
	temp = __raw_readl((volatile unsigned int *)tmp_addr);

	*value = temp;
	mutex_unlock(&mem->mutex);
	return 0;
}

static int imx8qm_hdp_write(struct hdp_mem *mem,
			    unsigned int addr,
			    unsigned int value)
{
	void *tmp_addr;
	void *off_addr;

	mutex_lock(&mem->mutex);
	tmp_addr = (addr & 0xfff) + mem->regs_base;
	off_addr = 0x8 + mem->ss_base;
	__raw_writel(addr >> 12, off_addr);

	__raw_writel(value, (volatile unsigned int *) tmp_addr);
	mutex_unlock(&mem->mutex);

	return 0;
}

static int imx8qm_hdp_sread(struct hdp_mem *mem,
			    unsigned int addr,
			    unsigned int *value)
{
	unsigned int temp;
	void *tmp_addr;
	void *off_addr;

	mutex_lock(&mem->mutex);
	tmp_addr = (addr & 0xfff) + mem->regs_base;
	off_addr = 0xc + mem->ss_base;
	__raw_writel(addr >> 12, off_addr);

	temp = __raw_readl((volatile unsigned int *)tmp_addr);
	*value = temp;
	mutex_unlock(&mem->mutex);
	return 0;
}

static int imx8qm_hdp_swrite(struct hdp_mem *mem,
			     unsigned int addr,
			     unsigned int value)
{
	void *tmp_addr;
	void *off_addr;

	mutex_lock(&mem->mutex);
	tmp_addr = (addr & 0xfff) + mem->regs_base;
	off_addr = 0xc + mem->ss_base;
	__raw_writel(addr >> 12, off_addr);
	__raw_writel(value, (volatile unsigned int *)tmp_addr);
	mutex_unlock(&mem->mutex);

	return 0;
}

static int ls1028a_hdp_read(struct hdp_mem *mem, unsigned int addr,
			    unsigned int *value)
{
	unsigned int temp;
	void *tmp_addr = mem->regs_base + addr;

	temp = __raw_readl((unsigned int *)tmp_addr);
	*value = temp;
	return 0;
}

static int ls1028a_hdp_write(struct hdp_mem *mem, unsigned int addr,
			     unsigned int value)
{
	void *tmp_addr = mem->regs_base + addr;

	__raw_writel(value, (unsigned int *)tmp_addr);
	return 0;
}

static struct hdp_rw_func imx8qm_rw = {
	.read_reg = imx8qm_hdp_read,
	.write_reg = imx8qm_hdp_write,
	.sread_reg = imx8qm_hdp_sread,
	.swrite_reg = imx8qm_hdp_swrite,
};

static struct hdp_ops imx8qm_dp_ops = {
#ifdef DEBUG_FW_LOAD
	.fw_load = hdp_fw_load,
#endif
	.fw_init = hdp_fw_init,
	.phy_init = dp_phy_init,
	.mode_set = dp_mode_set,
	.get_edid_block = dp_get_edid_block,
	.get_hpd_state = dp_get_hpd_state,
#ifndef CONFIG_ARCH_LAYERSCAPE
	.phy_reset = imx8qm_phy_reset,
	.pixel_link_validate = imx8qm_pixel_link_validate,
	.pixel_link_invalidate = imx8qm_pixel_link_invalidate,
	.pixel_link_sync_ctrl_enable = imx8qm_pixel_link_sync_ctrl_enable,
	.pixel_link_sync_ctrl_disable = imx8qm_pixel_link_sync_ctrl_disable,
#endif
	.pixel_link_mux = imx8qm_pixel_link_mux,

	.clock_init = imx8qm_clock_init,
	.ipg_clock_set_rate = imx8qm_ipg_clock_set_rate,
	.ipg_clock_enable = imx8qm_ipg_clock_enable,
	.ipg_clock_disable = imx8qm_ipg_clock_disable,
	.pixel_clock_set_rate = imx8qm_dp_pixel_clock_set_rate,
	.pixel_clock_enable = imx8qm_pixel_clock_enable,
	.pixel_clock_disable = imx8qm_pixel_clock_disable,
};

static struct hdp_ops imx8qm_hdmi_ops = {
#ifdef DEBUG_FW_LOAD
	.fw_load = hdp_fw_load,
#endif
	.fw_init = hdp_fw_init,
#ifndef CONFIG_ARCH_LAYERSCAPE
	.phy_init = hdmi_phy_init_ss28fdsoi,
	.mode_set = hdmi_mode_set_ss28fdsoi,
	.get_edid_block = hdmi_get_edid_block,
	.get_hpd_state = hdmi_get_hpd_state,

	.phy_reset = imx8qm_phy_reset,
	.pixel_link_validate = imx8qm_pixel_link_validate,
	.pixel_link_invalidate = imx8qm_pixel_link_invalidate,
	.pixel_link_sync_ctrl_enable = imx8qm_pixel_link_sync_ctrl_enable,
	.pixel_link_sync_ctrl_disable = imx8qm_pixel_link_sync_ctrl_disable,
#endif
	.pixel_link_mux = imx8qm_pixel_link_mux,

	.clock_init = imx8qm_clock_init,
	.ipg_clock_set_rate = imx8qm_ipg_clock_set_rate,
	.ipg_clock_enable = imx8qm_ipg_clock_enable,
	.ipg_clock_disable = imx8qm_ipg_clock_disable,
	.pixel_clock_set_rate = imx8qm_hdmi_pixel_clock_set_rate,
	.pixel_clock_enable = imx8qm_pixel_clock_enable,
	.pixel_clock_disable = imx8qm_pixel_clock_disable,
};

static struct hdp_devtype imx8qm_dp_devtype = {
	.ops = &imx8qm_dp_ops,
	.rw = &imx8qm_rw,
};

static struct hdp_devtype imx8qm_hdmi_devtype = {
	.ops = &imx8qm_hdmi_ops,
	.rw = &imx8qm_rw,
};

static struct hdp_rw_func imx8mq_rw = {
	.read_reg = imx8mq_hdp_read,
	.write_reg = imx8mq_hdp_write,
	.sread_reg = imx8mq_hdp_sread,
	.swrite_reg = imx8mq_hdp_swrite,
};

static struct hdp_ops imx8mq_ops = {
#ifndef CONFIG_ARCH_LAYERSCAPE
	.fw_init = hdp_fw_check,
	.phy_init = hdmi_phy_init_t28hpc,
	.mode_set = hdmi_mode_set_t28hpc,
	.mode_fixup = hdmi_mode_fixup_t28hpc,
	.get_edid_block = hdmi_get_edid_block,
	.get_hpd_state = hdmi_get_hpd_state,
	.write_hdr_metadata = hdmi_write_hdr_metadata,
	.pixel_clock_range = pixel_clock_range_t28hpc,
	.pixel_engine_reset = hdmi_phy_pix_engine_reset_t28hpc,
#endif
};

static struct hdp_devtype imx8mq_hdmi_devtype = {
	.ops = &imx8mq_ops,
	.rw = &imx8mq_rw,
};

static struct hdp_ops imx8mq_dp_ops = {
	.phy_init = dp_phy_init_t28hpc,
	.mode_set = dp_mode_set,
	.get_edid_block = dp_get_edid_block,
	.get_hpd_state = dp_get_hpd_state,
#ifndef CONFIG_ARCH_LAYERSCAPE
	.phy_reset = imx8mq_phy_reset,
#endif
};

static struct hdp_devtype imx8mq_dp_devtype = {
	.ops = &imx8mq_dp_ops,
	.rw = &imx8mq_rw,
};


static struct hdp_rw_func ls1028a_rw = {
	.read_reg = ls1028a_hdp_read,
	.write_reg = ls1028a_hdp_write,
};

static struct hdp_ops ls1028a_dp_ops = {
#ifdef DEBUG_FW_LOAD
	.fw_load = hdp_fw_load,
#endif
	.fw_init = hdp_fw_init,
	.phy_init = dp_phy_init_t28hpc,
	.mode_set = dp_mode_set,
	.get_edid_block = dp_get_edid_block,
	.get_hpd_state = dp_get_hpd_state,
	.phy_reset = ls1028a_phy_reset,
	.clock_init = ls1028a_clock_init,
	.ipg_clock_enable = ls1028a_ipg_clock_enable,
	.ipg_clock_disable = ls1028a_ipg_clock_disable,
	.pixel_clock_set_rate = imx8qm_dp_pixel_clock_set_rate,
	.pixel_clock_enable = ls1028a_pixel_clock_enable,
	.pixel_clock_disable = ls1028a_pixel_clock_disable,
};

static struct hdp_devtype ls1028a_dp_devtype = {
	.ops = &ls1028a_dp_ops,
	.rw = &ls1028a_rw,
	.connector_type = DRM_MODE_CONNECTOR_DisplayPort,
};
static const struct of_device_id imx_hdp_dt_ids[] = {
	{ .compatible = "fsl,imx8qm-hdmi", .data = &imx8qm_hdmi_devtype},
	{ .compatible = "fsl,imx8qm-dp", .data = &imx8qm_dp_devtype},
	{ .compatible = "fsl,imx8mq-hdmi", .data = &imx8mq_hdmi_devtype},
	{ .compatible = "fsl,imx8mq-dp", .data = &imx8mq_dp_devtype},
	{ .compatible = "fsl,ls1028a-dp", .data = &ls1028a_dp_devtype},
	{ }
};
MODULE_DEVICE_TABLE(of, imx_hdp_dt_ids);

static void hotplug_work_func(struct work_struct *work)
{
	struct imx_hdp *hdp = container_of(work,
					   struct imx_hdp,
					   hotplug_work.work);
	struct drm_connector *connector = &hdp->connector;

	drm_helper_hpd_irq_event(connector->dev);

	if (connector->status == connector_status_connected) {
		/* Cable Connected */
		/* For HDMI2.0 SCDC should setup again.
		 * So recovery pre video mode if it is 4Kp60 */
		if (drm_mode_equal(&hdp->video.pre_mode,
				   &edid_cea_modes[g_default_mode]))
			imx_hdp_mode_setup(hdp, &hdp->video.pre_mode);
		DRM_INFO("HDMI/DP Cable Plug In\n");
#ifdef CONFIG_ARCH_LAYERSCAPE
		if (hdp->is_hpd_irq)
#endif
			enable_irq(hdp->irq[HPD_IRQ_OUT]);
	} else if (connector->status == connector_status_disconnected) {
		/* Cable Disconnedted  */
		DRM_INFO("HDMI/DP Cable Plug Out\n");
#ifdef CONFIG_ARCH_LAYERSCAPE
		if (hdp->is_hpd_irq)
#endif
			enable_irq(hdp->irq[HPD_IRQ_IN]);
	}
}

static irqreturn_t imx_hdp_irq_thread(int irq, void *data)
{
	struct imx_hdp *hdp = data;

	disable_irq_nosync(irq);

	mod_delayed_work(system_wq, &hdp->hotplug_work,
			msecs_to_jiffies(HOTPLUG_DEBOUNCE_MS));

	return IRQ_HANDLED;
}

static int imx_hdp_hpd_thread(void *data)
{
	struct imx_hdp *hdp = data;

	mod_delayed_work(system_wq, &hdp->hotplug_work,
			msecs_to_jiffies(HOTPLUG_DEBOUNCE_MS));

	return 0;
}

static int parse_enable_res(const char *resolution)
{
	const char *name;
	unsigned int namelen;
	int xres = 0, yres = 0, refresh = 0;
	int i;
	bool is_digi = false;

	name = resolution;
	namelen = strlen(name);
	for (i = namelen-1; i >= 0; i--) {
		switch (name[i]) {
		case '@':
			if (is_digi) {
				refresh = simple_strtol(&name[i+1], NULL, 10);
				is_digi = false;
			}
			break;
		case 'x':
			if (is_digi) {
				yres = simple_strtol(&name[i+1], NULL, 10);
				is_digi = false;
			}
			break;
		case '0' ... '9':
			is_digi = true;
			break;
		default:
			DRM_WARN("Enable resolution %s failed\n", resolution);
			break;
		}
	}
	if (i < 0 && is_digi) {
		char *ch;

		xres = simple_strtol(name, &ch, 10);
		if (*ch != 'x')
			goto done;
	} else
		goto done;


	for (i = 0; i < ARRAY_SIZE(edid_cea_modes); i++) {
		if (edid_cea_modes[i].hdisplay == xres &&
				edid_cea_modes[i].vdisplay == yres &&
				edid_cea_modes[i].vrefresh == refresh) {
			edid_cea_modes_enabled[i] = 1;
			DRM_INFO("Resolution %dx%d@%d is enabled\n",
							xres, yres, refresh);
		}
	}

	return 0;

done:
	DRM_WARN("Enable resolution %s failed\n", resolution);

	return 1;
}

static int imx_hdp_imx_bind(struct device *dev, struct device *master,
			    void *data)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct drm_device *drm = data;
	struct imx_hdp *hdp;
	const struct of_device_id *of_id =
			of_match_device(imx_hdp_dt_ids, dev);
	const struct hdp_devtype *devtype = of_id->data;
	struct drm_encoder *encoder;
	struct drm_bridge *bridge;
	struct drm_connector *connector;
	struct resource *res;
#ifdef CONFIG_ARCH_LAYERSCAPE
	struct task_struct *hpd_thread;
	const char *resolution;
	int i;
#endif
	u8 hpd;
	int ret;

	if (!pdev->dev.of_node)
		return -ENODEV;

	hdp = devm_kzalloc(&pdev->dev, sizeof(*hdp), GFP_KERNEL);
	if (!hdp)
		return -ENOMEM;

	hdp->dev = &pdev->dev;
	encoder = &hdp->encoder;
	bridge = &hdp->bridge;
	connector = &hdp->connector;

	mutex_init(&hdp->mutex);

	hdp->is_hpd_irq = true;
#ifdef CONFIG_ARCH_LAYERSCAPE
	hdp->is_hpd_irq = of_property_read_bool(pdev->dev.of_node,
						"fsl,hpd_irq");
	if (hdp->is_hpd_irq)
#endif
	{       hdp->irq[HPD_IRQ_IN] =
			platform_get_irq_byname(pdev, "plug_in");
		if (hdp->irq[HPD_IRQ_IN] < 0)
			dev_info(&pdev->dev, "No plug_in irq number\n");

		hdp->irq[HPD_IRQ_OUT] =
			platform_get_irq_byname(pdev, "plug_out");
		if (hdp->irq[HPD_IRQ_OUT] < 0)
			dev_info(&pdev->dev, "No plug_out irq number\n");
	}

	mutex_init(&hdp->mem.mutex);

#ifndef CONFIG_ARCH_LAYERSCAPE
	/* register map */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	hdp->mem.regs_base = devm_ioremap_resource(dev, res);
	if (IS_ERR(hdp->mem.regs_base)) {
		dev_err(dev, "Failed to get HDP CTRL base register\n");
		return -EINVAL;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	hdp->mem.ss_base = devm_ioremap_resource(dev, res);
	if (IS_ERR(hdp->mem.ss_base)) {
		dev_err(dev, "Failed to get HDP CRS base register\n");
		return -EINVAL;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	hdp->mem.rst_base = devm_ioremap_resource(dev, res);
	if (IS_ERR(hdp->mem.rst_base))
		dev_warn(dev, "Failed to get HDP RESET base register\n");
#else

	/* register map */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	hdp->mem.regs_base = devm_ioremap_resource(dev, res);
	if (IS_ERR(hdp->mem.regs_base)) {
		dev_err(dev, "Failed to get HDP CTRL base register\n");
		return -EINVAL;
	}

#endif

	hdp->is_edp = of_property_read_bool(pdev->dev.of_node, "fsl,edp");

	hdp->no_edid = of_property_read_bool(pdev->dev.of_node, "fsl,no_edid");
#ifndef CONFIG_ARCH_LAYERSCAPE
	/* EDID function is not supported by iMX8QM A0 */
	if (cpu_is_imx8qm() && (imx8_get_soc_revision() < B0_SILICON_ID))
		hdp->no_edid = true;
#else
	if (hdp->no_edid && of_property_read_bool(pdev->dev.of_node,
								"resolution")) {
		hdp->num_res = of_property_count_strings(pdev->dev.of_node,
								"resolution");
		if (hdp->num_res < 0) {
			hdp->num_res = 0;
		} else
			for (i = 0; i < hdp->num_res; i++) {
				ret = of_property_read_string_index(
						pdev->dev.of_node,
						"resolution", i, &resolution);
				if (ret) {
					dev_warn(dev, "Resolution index %d property read error:%d\n",
							i, ret);
					hdp->num_res = 0;
					break;
				}
				ret = parse_enable_res(resolution);
				if (ret) {
					hdp->num_res = 0;
					break;
				}
			}
	}
#endif


	ret = of_property_read_u32(pdev->dev.of_node,
				       "lane_mapping",
				       &hdp->lane_mapping);
	if (ret) {
		hdp->lane_mapping = 0x1b;
		dev_warn(dev, "Failed to get lane_mapping - using default\n");
	}
	dev_info(dev, "lane_mapping 0x%02x\n", hdp->lane_mapping);

	ret = of_property_read_u32(pdev->dev.of_node,
				       "edp_link_rate",
				       &hdp->edp_link_rate);
	if (ret) {
		hdp->edp_link_rate = 0;
		dev_warn(dev, "Failed to get dp_link_rate - using default\n");
	}
	dev_info(dev, "edp_link_rate 0x%02x\n", hdp->edp_link_rate);

	ret = of_property_read_u32(pdev->dev.of_node,
				       "edp_num_lanes",
				       &hdp->edp_num_lanes);
	if (ret) {
		hdp->edp_num_lanes = 4;
		dev_warn(dev, "Failed to get dp_num_lanes - using default\n");
	}
	dev_info(dev, "dp_num_lanes 0x%02x\n", hdp->edp_num_lanes);

	hdp->ops = devtype->ops;
	hdp->rw = devtype->rw;
	hdp->bpc = 8;
	hdp->format = PXL_RGB;
	hdp->ipcHndl = 0;

	imx_hdp_state_init(hdp);

	hdp->link_rate = AFE_LINK_RATE_5_4;

	hdp->dual_mode = false;

	ret = imx_hdp_call(hdp, clock_init, &hdp->clks);
	if (ret < 0) {
		DRM_ERROR("Failed to initialize clock\n");
		return ret;
	}

	imx_hdp_call(hdp, ipg_clock_set_rate, &hdp->clks);

	ret = imx_hdp_call(hdp, ipg_clock_enable, &hdp->clks);
	if (ret < 0) {
		DRM_ERROR("Failed to initialize IPG clock\n");
		return ret;
	}

	imx_hdp_call(hdp, pixel_clock_set_rate, &hdp->clks);

	imx_hdp_call(hdp, pixel_clock_enable, &hdp->clks);

	imx_hdp_call(hdp, phy_reset, hdp->ipcHndl, &hdp->mem, 0);

	imx_hdp_call(hdp, fw_load, &hdp->state);

	ret = imx_hdp_call(hdp, fw_init, &hdp->state);
	if (ret < 0) {
		DRM_ERROR("Failed to initialise HDP firmware\n");
		return ret;
	}

	/* Pixel Format - 1 RGB, 2 YCbCr 444, 3 YCbCr 420 */
	/* bpp (bits per subpixel) - 8 24bpp, 10 30bpp, 12 36bpp, 16 48bpp */
	ret = imx_hdp_call(hdp, phy_init, &hdp->state, &edid_cea_modes[g_default_mode],
			   hdp->format, hdp->bpc);
	if (ret < 0) {
		DRM_ERROR("Failed to initialise HDP PHY\n");
		return ret;
	}

	/* encoder */
	encoder->possible_crtcs = drm_of_find_possible_crtcs(drm, dev->of_node);
	/*
	 * If we failed to find the CRTC(s) which this encoder is
	 * supposed to be connected to, it's because the CRTC has
	 * not been registered yet.  Defer probing, and hope that
	 * the required CRTC is added later.
	 */
	if (encoder->possible_crtcs == 0)
		return -EPROBE_DEFER;

	/* encoder */
	drm_encoder_helper_add(encoder, &imx_hdp_imx_encoder_helper_funcs);
	drm_encoder_init(drm, encoder, &imx_hdp_imx_encoder_funcs,
			 DRM_MODE_ENCODER_TMDS, NULL);

	/* bridge */
	bridge->driver_private = hdp;
	bridge->funcs = &imx_hdp_bridge_funcs;
	ret = drm_bridge_attach(encoder, bridge, NULL);
	if (ret) {
		DRM_ERROR("Failed to initialize bridge with drm\n");
		return -EINVAL;
	}

	encoder->bridge = bridge;

#ifndef CONFIG_ARCH_LAYERSCAPE
	hdp->connector.polled = DRM_CONNECTOR_POLL_HPD;
#else
	/* There is no interrupt for hotplug on ls1028a platform */
	hdp->connector.polled = DRM_CONNECTOR_POLL_CONNECT |
		DRM_CONNECTOR_POLL_DISCONNECT;
#endif
	/* connector */
	drm_connector_helper_add(connector,
				 &imx_hdp_connector_helper_funcs);

	drm_connector_init(drm, connector,
			   &imx_hdp_connector_funcs,
			   devtype->connector_type);

	drm_mode_connector_attach_encoder(connector, encoder);

	dev_set_drvdata(dev, hdp);

	INIT_DELAYED_WORK(&hdp->hotplug_work, hotplug_work_func);

	/* Check cable states before enable irq */
	imx_hdp_call(hdp, get_hpd_state, &hdp->state, &hpd);

	if (hdp->is_hpd_irq) {
		/* Enable Hotplug Detect IRQ thread */
		if (hdp->irq[HPD_IRQ_IN] > 0) {
			irq_set_status_flags(hdp->irq[HPD_IRQ_IN],
					     IRQ_NOAUTOEN);
			ret = devm_request_threaded_irq(dev,
							hdp->irq[HPD_IRQ_IN],
							NULL,
							imx_hdp_irq_thread,
							IRQF_ONESHOT,
							dev_name(dev),
							hdp);
			if (ret) {
				dev_err(&pdev->dev, "can't claim irq %d\n",
					hdp->irq[HPD_IRQ_IN]);
				goto err_irq;
			}
			/* Cable Disconnedted, enable Plug in IRQ */
			if (hpd == 0)
				enable_irq(hdp->irq[HPD_IRQ_IN]);
		}
		if (hdp->irq[HPD_IRQ_OUT] > 0) {
			irq_set_status_flags(hdp->irq[HPD_IRQ_OUT],
					     IRQ_NOAUTOEN);
			ret = devm_request_threaded_irq(dev,
							hdp->irq[HPD_IRQ_OUT],
							NULL,
							imx_hdp_irq_thread,
							IRQF_ONESHOT,
							dev_name(dev),
							hdp);
			if (ret) {
				dev_err(&pdev->dev, "can't claim irq %d\n",
					hdp->irq[HPD_IRQ_OUT]);
				goto err_irq;
			}
			/* Cable Connected, enable Plug out IRQ */
			if (hpd == 1)
				enable_irq(hdp->irq[HPD_IRQ_OUT]);
		}
	} else {
		hpd_thread = kthread_create(imx_hdp_hpd_thread, hdp, "hdp-hpd");
		if (IS_ERR(hpd_thread))
			dev_err(&pdev->dev, "failed  create hpd thread\n");

			wake_up_process(hpd_thread);
	}

	return 0;
err_irq:
	drm_encoder_cleanup(encoder);
	return ret;
}

static void imx_hdp_imx_unbind(struct device *dev, struct device *master,
			       void *data)
{
	struct imx_hdp *hdp = dev_get_drvdata(dev);

	imx_hdp_call(hdp, pixel_clock_disable, &hdp->clks);
}

static const struct component_ops imx_hdp_imx_ops = {
	.bind	= imx_hdp_imx_bind,
	.unbind	= imx_hdp_imx_unbind,
};

static int imx_hdp_imx_probe(struct platform_device *pdev)
{
	return component_add(&pdev->dev, &imx_hdp_imx_ops);
}

static int imx_hdp_imx_remove(struct platform_device *pdev)
{
	component_del(&pdev->dev, &imx_hdp_imx_ops);

	return 0;
}

static struct platform_driver imx_hdp_imx_platform_driver = {
	.probe  = imx_hdp_imx_probe,
	.remove = imx_hdp_imx_remove,
	.driver = {
		.name = "i.mx8-hdp",
		.of_match_table = imx_hdp_dt_ids,
	},
};

module_platform_driver(imx_hdp_imx_platform_driver);

MODULE_AUTHOR("Sandor Yu <Sandor.yu@nxp.com>");
MODULE_DESCRIPTION("IMX8QM DP Display Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:dp-hdmi-imx");
