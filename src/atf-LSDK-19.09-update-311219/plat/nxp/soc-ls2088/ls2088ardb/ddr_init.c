/*
 * Copyright 2018-2019 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Author York Sun <york.sun@nxp.com>
 */

#include <platform_def.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <debug.h>
#include <errno.h>
#include <string.h>
#include <io.h>
#include <ddr.h>
#include <utils.h>
#include <utils_def.h>
#include <errata.h>

#ifdef CONFIG_STATIC_DDR
const struct ddr_cfg_regs static_2100 = {
	.cs[0].config = 0x80048412,
	.cs[0].bnds = 0xff,
	.sdram_cfg[0] = 0xe5004000,
	.sdram_cfg[1] = 0x401150,
	.sdram_cfg[2] = 0x0,
	.timing_cfg[0] = 0xd1770018,
	.timing_cfg[1] = 0xf5fc8275,
	.timing_cfg[2] = 0x5941a5,
	.timing_cfg[3] = 0x2161100,
	.timing_cfg[4] = 0x220002,
	.timing_cfg[5] = 0x5401400,
	.timing_cfg[7] = 0x26600000,
	.timing_cfg[8] = 0x6558a00,
	.dq_map[0] = 0,
	.dq_map[1] = 0,
	.dq_map[2] = 0,
	.dq_map[3] = 0,
	.sdram_mode[0] = 0x3010631,
	.sdram_mode[1] = 0x100200,
	.sdram_mode[9] = 0xc400000,
	.sdram_mode[8] = 0x500,
	.sdram_mode[2] = 0x10631,
	.sdram_mode[3] = 0x100200,
	.sdram_mode[10] = 0x400,
	.sdram_mode[11] = 0xc400000,
	.sdram_mode[4] = 0x10631,
	.sdram_mode[5] = 0x100200,
	.sdram_mode[12] = 0x400,
	.sdram_mode[13] = 0xc400000,
	.sdram_mode[6] = 0x10631,
	.sdram_mode[7] = 0x100200,
	.sdram_mode[14] = 0x400,
	.sdram_mode[15] = 0xc400000,
	.interval = 0x1FFE07FF,
	.zq_cntl = 0x8A090705,
	.clk_cntl = 0x2400000,
	.cdr[0] = 0x80040000,
	.cdr[1] = 0xC1,
	.wrlvl_cntl[0] = 0x86750606,
	.wrlvl_cntl[1] = 0x708090a,
	.wrlvl_cntl[2] = 0xc0d0e0b,
	.debug[28] = 0x4d,
};

long long board_static_ddr(struct ddr_info *priv)
{
	long long size = 0;
	memcpy(&priv->ddr_reg, &static_2100, sizeof(static_2100));

	size = 0x200000000UL; //8G
	return size;
}
#else

static const struct rc_timing rcb_1[] = {
	{1600, 10, 9},
	{1867, 12, 0xB},
	//{2134, 12, 0xB},
	//{2134, 10, 8}, //CRI  Code Warrior
#ifdef CONFIG_DDR_SEC
	{2134, 10, 7},   //CRII Code Warrior 2nd
#else
	{2134, 9, 7},    //CRII Code Warrior
#endif
	{}
};
#if 0
static const struct rc_timing rce_1[] = {
	{1600, 10, 9},
	{1867, 12, 0xA},
	{2134, 12, 0xB},
	{}
};
#endif
static const struct board_timing udimm1[] = {
	//{0x01, rcb_1, 0x01020306, 0x07090A00},
	//{0x04, rce_1, 0x01020407, 0x090A0B05},
	//{0x1f, rcb_1, 0x00010104, 0x04050502}, //CRI  Code Warrior
	{0x1f, rcb_1, 0x00000002, 0x02040301},   //CRII Code Warrior
};

#if 1
static const struct rc_timing rcb_2[] = {
	{1600, 8, 0xD},
	{}
};

static const struct rc_timing rce_2[] = {
	{1600, 8, 0xD},
	{}
};

static const struct board_timing udimm2[] = {
	{0x01, rcb_2, 0xFEFCFD00, 0x00000000},
	{0x04, rce_2, 0xFEFCFD00, 0x000000FD},
};

static const struct rc_timing rcb[] = {
	{1600, 8, 0x0F},
	{1867, 8, 0x10},
	{2134, 8, 0x13},
	{}
};

static const struct board_timing rdimm[] = {
	{0x01, rcb, 0xFEFCFAFA, 0xFAFCFEF9},
	{0x04, rcb, 0xFEFCFAFA, 0xFAFCFEF9},
};
#endif

int ddr_board_options(struct ddr_info *priv)
{
	struct memctl_opt *popts = &priv->opt;
	struct dimm_params *pdimm = &priv->dimm;
	const struct ddr_conf *conf = &priv->conf;
	const unsigned long ddr_freq = priv->clk / 1000000;
	unsigned int dq_mapping_0, dq_mapping_2, dq_mapping_3;
	int ret;
	int i;
	int is_dpddr = 0;

	if (NXP_DDR3_ADDR == (unsigned long)priv->ddr[0]) { /* DP-DDR */
		//is_dpddr = 1;
		if (popts->rdimm) {
			ERROR("RDIMM parameters not set.\n");
			return -EINVAL;
		}
		ret = cal_board_params(priv, udimm2, ARRAY_SIZE(udimm2));
	}
 
	if (popts->rdimm) {
		ret = cal_board_params(priv, rdimm,
					       ARRAY_SIZE(rdimm));
	} else {
		ret = cal_board_params(priv, udimm1,
					       ARRAY_SIZE(udimm1));
	}
	if (ret)
		return ret;

	//popts->cpo_sample = 0x78;
	popts->cpo_sample = 0x4d; //VM1P

	if (is_dpddr) {
		/* DPDDR bus width 32 bits */
		popts->data_bus_used = DDR_DBUS_32;
		popts->otf_burst_chop_en = 0;
		popts->burst_length = DDR_BL8;
		popts->bstopre = 0;	/* enable auto precharge */
		/*
		 * Layout optimization results byte mapping
		 * Byte 0 -> Byte ECC
		 * Byte 1 -> Byte 3
		 * Byte 2 -> Byte 2
		 * Byte 3 -> Byte 1
		 * Byte ECC -> Byte 0
		 */
		dq_mapping_0 = pdimm->dq_mapping[0];
		dq_mapping_2 = pdimm->dq_mapping[2];
		dq_mapping_3 = pdimm->dq_mapping[3];
		pdimm->dq_mapping[0] = pdimm->dq_mapping[8];
		pdimm->dq_mapping[1] = pdimm->dq_mapping[9];
		pdimm->dq_mapping[2] = pdimm->dq_mapping[6];
		pdimm->dq_mapping[3] = pdimm->dq_mapping[7];
		pdimm->dq_mapping[6] = dq_mapping_2;
		pdimm->dq_mapping[7] = dq_mapping_3;
		pdimm->dq_mapping[8] = dq_mapping_0;
		for (i = 9; i < 18; i++)
			pdimm->dq_mapping[i] = 0;
		popts->cpo_sample = 0x5a;
	}

	popts->wrlvl_override = 1;
	popts->wrlvl_sample = 0x0;	/* 32 clocks */

	if (ddr_freq < 2350) {
		if (conf->cs_in_use == 0xf) {
			popts->ddr_cdr1 = DDR_CDR1_DHC_EN |
					  DDR_CDR1_ODT(DDR_CDR_ODT_80ohm);
			popts->ddr_cdr2 = DDR_CDR2_ODT(DDR_CDR_ODT_80ohm);
			popts->twot_en = 1;	/* enable 2T timing */
		} else {
			popts->ddr_cdr1 = DDR_CDR1_DHC_EN |
					  DDR_CDR1_ODT(DDR_CDR_ODT_60ohm);
			popts->ddr_cdr2 = DDR_CDR2_ODT(DDR_CDR_ODT_60ohm) |
					  //DDR_CDR2_VREF_TRAIN_EN		|
					  DDR_CDR2_VREF_RANGE_2;
		}
	} else {
		popts->ddr_cdr1 = DDR_CDR1_DHC_EN |
				  DDR_CDR1_ODT(DDR_CDR_ODT_100ohm);
		popts->ddr_cdr2 = DDR_CDR2_ODT(DDR_CDR_ODT_100ohm) |
				  DDR_CDR2_VREF_RANGE_2;
	}

	return 0;
}

/* WG1008-VM1P Add for fixed dram init */ 
/* DDR model number: MT40A512M8HX-093E */
struct dimm_params ddr_raw_timing = {
	.n_ranks = 1,
	.rank_density = 8589934592u,
	.capacity = 8589934592u,
	.primary_sdram_width = 64,
	.ec_sdram_width = 8,
	.mirrored_dimm = 0,
	.device_width = 16,
	.n_row_addr = 17,
	.n_col_addr = 10,
	.bank_addr_bits = 2,
	.bank_group_bits = 1,
	.edc_config = 2,
	.burst_lengths_bitmask = 0x0c,
	.tckmin_x_ps = 937,
	.tckmax_ps = 1071,
	.caslat_x = 0x000DFA00,
	.taa_ps = 14060,
	.trcd_ps = 14060,
	.trp_ps = 14060,
	.tras_ps = 33000,
	.trc_ps = 47060,
	.twr_ps = 15000,
	.trfc1_ps = 350000,
	.trfc2_ps = 260000,
	.trfc4_ps = 160000,
	.tfaw_ps = 30000,
	.trrds_ps = 5300,
	.trrdl_ps = 6400,
	.tccdl_ps = 5355,
	.refresh_rate_ps = 7800000,
	.rc = 0x1f,
	.dq_mapping[0] = 0x0,
	.dq_mapping[1] = 0x0,
	.dq_mapping[2] = 0x0,
	.dq_mapping[3] = 0x0,
	.dq_mapping[4] = 0x0,
	.dq_mapping[5] = 0x0,
	.dq_mapping[6] = 0x0,
	.dq_mapping[7] = 0x0,
	.dq_mapping[8] = 0x0,
	.dq_mapping[9] = 0x0,
	.dq_mapping[10] = 0x0,
	.dq_mapping[11] = 0x0,
	.dq_mapping[12] = 0x0,
	.dq_mapping[13] = 0x0,
	.dq_mapping[14] = 0x0,
	.dq_mapping[15] = 0x0,
	.dq_mapping[16] = 0x0,
	.dq_mapping[17] = 0x0,
	.dq_mapping_ors = 0,
};

int ddr_get_ddr_params(struct dimm_params *pdimm,
			    struct ddr_conf *conf)
{
	static const char dimm_model[] = "Fixed DDR on board";
#ifdef CONFIG_DDR_SEC
	static const char ddr_src[] = "DDR 2nd Source (SAMSUNG: K4AAG165WA-BCTD)";
#else
	static const char ddr_src[] = "DDR Main Source (SAMSUNG: K4AAG165WA-BCWE)";
#endif

	conf->dimm_in_use[0] = 1;
	memcpy(pdimm, &ddr_raw_timing, sizeof(struct dimm_params));
	memcpy(pdimm->mpart, dimm_model, sizeof(dimm_model) - 1);
	printf("\n<< %s >>\n", ddr_src);
	printf("<< Func %s: line %d, rc_timing rcb_1[2] = {%d, %d, %d} >>\n",
		__FUNCTION__, __LINE__, rcb_1[2].speed_bin, rcb_1[2].clk_adj, rcb_1[2].wrlvl);
	return 1;
}
#endif

long long _init_ddr(void)
{
	int spd_addr[] = { 0x51, 0x52, 0x53, 0x54 };
	//int dpddr_spd_addr[] = { 0x55 };
	struct ddr_info info;
	struct sysinfo sys;
	long long dram_size;
	//long long dp_dram_size;

	zeromem(&sys, sizeof(sys));
	get_clocks(&sys);
	debug("platform clock %lu\n", sys.freq_platform);
	debug("DDR PLL1 %lu\n", sys.freq_ddr_pll0);
	debug("DDR PLL2 %lu\n", sys.freq_ddr_pll1);

	zeromem(&info, sizeof(struct ddr_info));
	/* Set two DDRC here. Unused DDRC will be removed automatically. */
	info.num_ctlrs = NUM_OF_DDRC;
	info.dimm_on_ctlr = DDRC_NUM_DIMM;
	info.spd_addr = spd_addr;
	info.ddr[0] = (void *)NXP_DDR_ADDR;
	info.ddr[1] = (void *)NXP_DDR2_ADDR;
	info.clk = get_ddr_freq(&sys, 0);
	if (!info.clk)
		info.clk = get_ddr_freq(&sys, 1);

	dram_size = dram_init(&info);

	if (dram_size < 0)
		ERROR("DDR init failed.\n");
#if 0
	zeromem(&info, sizeof(info));
	info.num_ctlrs = 1;
	info.dimm_on_ctlr = 1;
	info.spd_addr = dpddr_spd_addr;
	info.ddr[0] = (void *)NXP_DDR3_ADDR;
	info.clk = get_ddr_freq(&sys, 2);

	dp_dram_size = dram_init(&info);
	if (dp_dram_size < 0)
		debug("DPDDR init failed.\n");
#endif
	//erratum_a008850_post(); //LS1046A
	
	return dram_size;
}
