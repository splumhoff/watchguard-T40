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
	.cs[0].config = 0x80040322,
	.cs[0].bnds = 0x1FF,
	.cs[1].config = 0x80000322,
	.cs[1].bnds = 0x1FF,
	.sdram_cfg[0] = 0xE5004000,
	.sdram_cfg[1] = 0x401151,
	.timing_cfg[0] = 0xD1770018,
	.timing_cfg[1] = 0xF2FC9245,
	.timing_cfg[2] = 0x594197,
	.timing_cfg[3] = 0x2101100,
	.timing_cfg[4] = 0x220002,
	.timing_cfg[5] = 0x5401400,
	.timing_cfg[7] = 0x26600000,
	.timing_cfg[8] = 0x5446A00,
	.dq_map[0] = 0x32C57554,
	.dq_map[1] = 0xD4BB0BD4,
	.dq_map[2] = 0x2EC2F554,
	.dq_map[3] = 0xD95D4001,
	.sdram_mode[0] = 0x3010631,
	.sdram_mode[1] = 0x100200,
	.sdram_mode[9] = 0x8400000,
	.sdram_mode[8] = 0x500,
	.sdram_mode[2] = 0x10631,
	.sdram_mode[3] = 0x100200,
	.sdram_mode[10] = 0x400,
	.sdram_mode[11] = 0x8400000,
	.sdram_mode[4] = 0x10631,
	.sdram_mode[5] = 0x100200,
	.sdram_mode[12] = 0x400,
	.sdram_mode[13] = 0x8400000,
	.sdram_mode[6] = 0x10631,
	.sdram_mode[7] = 0x100200,
	.sdram_mode[14] = 0x400,
	.sdram_mode[15] = 0x8400000,
	.interval = 0x1FFE07FF,
	.zq_cntl = 0x8A090705,
	.clk_cntl = 0x2000000,
	.cdr[0] = 0x80040000,
	.cdr[1] = 0xC1,
	.wrlvl_cntl[0] = 0x86750609,
	.wrlvl_cntl[1] = 0xA0B0C0D,
	.wrlvl_cntl[2] = 0xF10110E,
};

const struct ddr_cfg_regs static_1800 = {
	.cs[0].config = 0x80040322,
	.cs[0].bnds = 0x1FF,
	.cs[1].config = 0x80000322,
	.cs[1].bnds = 0x1FF,
	.sdram_cfg[0] = 0xE5004000,
	.sdram_cfg[1] = 0x401151,
	.timing_cfg[0] = 0x91660018,
	.timing_cfg[1] = 0xDDD82045,
	.timing_cfg[2] = 0x512153,
	.timing_cfg[3] = 0x10E1100,
	.timing_cfg[4] = 0x220002,
	.timing_cfg[5] = 0x4401400,
	.timing_cfg[7] = 0x14400000,
	.timing_cfg[8] = 0x3335900,
	.dq_map[0] = 0x32C57554,
	.dq_map[1] = 0xD4BB0BD4,
	.dq_map[2] = 0x2EC2F554,
	.dq_map[3] = 0xD95D4001,
	.sdram_mode[0] = 0x3010421,
	.sdram_mode[1] = 0x80200,
	.sdram_mode[9] = 0x4400000,
	.sdram_mode[8] = 0x500,
	.sdram_mode[2] = 0x10421,
	.sdram_mode[3] = 0x80200,
	.sdram_mode[10] = 0x400,
	.sdram_mode[11] = 0x4400000,
	.sdram_mode[4] = 0x10421,
	.sdram_mode[5] = 0x80200,
	.sdram_mode[12] = 0x400,
	.sdram_mode[13] = 0x4400000,
	.sdram_mode[6] = 0x10421,
	.sdram_mode[7] = 0x80200,
	.sdram_mode[14] = 0x400,
	.sdram_mode[15] = 0x4400000,
	.interval = 0x1B6C06DB,
	.zq_cntl = 0x8A090705,
	.clk_cntl = 0x2000000,
	.cdr[0] = 0x80040000,
	.cdr[1] = 0xC1,
	.wrlvl_cntl[0] = 0x86750607,
	.wrlvl_cntl[1] = 0x8090A0B,
	.wrlvl_cntl[2] = 0xD0E0F0C,
};

const struct ddr_cfg_regs static_1600 = {
	.cs[0].config = 0x80040322,
	.cs[0].bnds = 0x1FF,
	.cs[1].config = 0x80000322,
	.cs[1].bnds = 0x1FF,
	.sdram_cfg[0] = 0xE5004000,
	.sdram_cfg[1] = 0x401151,
	.sdram_cfg[2] = 0x0,
	.timing_cfg[0] = 0x91550018,
	.timing_cfg[1] = 0xBAB48E44,
	.timing_cfg[2] = 0x490111,
	.timing_cfg[3] = 0x10C1000,
	.timing_cfg[4] = 0x220002,
	.timing_cfg[5] = 0x3401400,
	.timing_cfg[6] = 0x0,
	.timing_cfg[7] = 0x13300000,
	.timing_cfg[8] = 0x1224800,
	.timing_cfg[9] = 0x0,
	.dq_map[0] = 0x32C57554,
	.dq_map[1] = 0xD4BB0BD4,
	.dq_map[2] = 0x2EC2F554,
	.dq_map[3] = 0xD95D4001,
	.sdram_mode[0] = 0x3010211,
	.sdram_mode[1] = 0x0,
	.sdram_mode[9] = 0x400000,
	.sdram_mode[8] = 0x500,
	.sdram_mode[2] = 0x10211,
	.sdram_mode[3] = 0x0,
	.sdram_mode[10] = 0x400,
	.sdram_mode[11] = 0x400000,
	.sdram_mode[4] = 0x10211,
	.sdram_mode[5] = 0x0,
	.sdram_mode[12] = 0x400,
	.sdram_mode[13] = 0x400000,
	.sdram_mode[6] = 0x10211,
	.sdram_mode[7] = 0x0,
	.sdram_mode[14] = 0x400,
	.sdram_mode[15] = 0x400000,
	.interval = 0x18600618,
	.zq_cntl = 0x8A090705,
	.ddr_sr_cntr = 0x0,
	.clk_cntl = 0x2000000,
	.cdr[0] = 0x80040000,
	.cdr[1] = 0xC1,
	.wrlvl_cntl[0] = 0x86750607,
	.wrlvl_cntl[1] = 0x8090A0B,
	.wrlvl_cntl[2] = 0xD0E0F0C,
};

struct static_table {
	unsigned long rate;
	const struct ddr_cfg_regs *regs;
};

const struct static_table table[] = {
	{1600, &static_1600},
	{1800, &static_1800},
	{2100, &static_2100},
};

long long board_static_ddr(struct ddr_info *priv)
{
	const unsigned long clk = priv->clk / 1000000;
	long long size = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(table); i++) {
		if (table[i].rate >= clk)
			break;
	}
	if (i < ARRAY_SIZE(table)) {
		VERBOSE("Found static setting for rate %ld\n", table[i].rate);
		memcpy(&priv->ddr_reg, table[i].regs,
		       sizeof(struct ddr_cfg_regs));
		size = 0x200000000UL;
	} else {
		ERROR("Not static settings for rate %ld\n", clk);
	}

	return size;
}
#else
static const struct rc_timing rce[] = {
#ifdef CONFIG_WG1008_PX1
	{1600, 9, 6},
	{1867, 9, 6},
#ifdef CONFIG_MICRON_DDR
	{2100, 9, 7}, //Micron
#else
	{2100, 10, 8}, //CRI Code Warrior
#endif
#else
	{1600, 8, 7},
	{1867, 8, 7},
	{2134, 8, 9},
#endif
	{}
};

static const struct board_timing udimm[] = {
#ifdef CONFIG_WG1008_PX1
	{0x1f, rce, 0x00010104, 0x04050502}, //CRI Code Warrior
#else
	{0x04, rce, 0x01020304, 0x06070805},
#endif
};

int ddr_board_options(struct ddr_info *priv)
{
	int ret;
	struct memctl_opt *popts = &priv->opt;

	if (popts->rdimm) {
		debug("RDIMM parameters not set.\n");
		return -EINVAL;
	}

	ret = cal_board_params(priv, udimm, ARRAY_SIZE(udimm));
	if (ret)
		return ret;

	popts->wrlvl_override = 1;
	popts->wrlvl_sample = 0x0;	/* 32 clocks */
#ifdef CONFIG_WG1008_PX1
	popts->cpo_sample = 0x4d;
#else
	popts->cpo_sample = 0x61;
#endif
	popts->ddr_cdr1 = DDR_CDR1_DHC_EN	|
			  DDR_CDR1_ODT(DDR_CDR_ODT_80ohm);
	popts->ddr_cdr2 = DDR_CDR2_ODT(DDR_CDR_ODT_80ohm)	|
			  DDR_CDR2_VREF_TRAIN_EN		|
			  DDR_CDR2_VREF_RANGE_2;
	popts->bstopre = 0;

	return 0;
}

/*Add for fixed dram init*/
#ifdef CONFIG_WG1008_PX1
/* DDR model number: MT40A512M8HX-093E */
struct dimm_params ddr_raw_timing = {
	.n_ranks = 1,
	.rank_density = 4294967296u,
	.capacity = 4294967296u,
	.primary_sdram_width = 64,
	.ec_sdram_width = 8,
	.mirrored_dimm = 0,
	.device_width = 16,
	.n_row_addr = 16,
	.n_col_addr = 10,
	.bank_addr_bits = 2,
	.bank_group_bits = 1,
	.edc_config = 2,
	.burst_lengths_bitmask = 0x0c,
	.caslat_x = 0x000DFA00,
#if 0
	.tckmin_x_ps = 1250,
	.tckmax_ps = 1500,
	.taa_ps = 13500,
	.trcd_ps = 13500,
	.trp_ps = 13500,
	.tras_ps = 35000,
	.trc_ps = 48500,
	.twr_ps = 15000,
	.trfc1_ps = 350000,
	.trfc2_ps = 260000,
	.trfc4_ps = 160000,
	.tfaw_ps = 35000,
	.trrds_ps = 6000,
	.trrdl_ps = 7500,
	.tccdl_ps = 6250,
#else
	.tckmin_x_ps = 937,
	.tckmax_ps = 1071,
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
	#ifdef CONFIG_MICRON_DDR
	.tccdl_ps = 5355,
	#else
	.tccdl_ps = 5625,
	#endif
#endif
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
#ifdef CONFIG_MICRON_DDR
	static const char ddr_src[] = "DDR 2nd Source (MICRON: MT40A512M16TB-062E)";
#else
	static const char ddr_src[] = "DDR Main Source (SAMSUNG: K4A8G165WC-BCTD)";
#endif

	conf->dimm_in_use[0] = 1;
	memcpy(pdimm, &ddr_raw_timing, sizeof(struct dimm_params));
	memcpy(pdimm->mpart, dimm_model, sizeof(dimm_model) - 1);
	printf("\n<< %s >>\n", ddr_src);
	printf("<< Func %s: line %d, rc_timing rce[2] = {%d, %d, %d},  ddr_raw_timing.tccdl_ps = %d >>\n",
		__FUNCTION__, __LINE__, rce[2].speed_bin, rce[2].clk_adj, rce[2].wrlvl, pdimm->tccdl_ps);
	return 1;
}
#endif
#endif

long long _init_ddr(void)
{
	int spd_addr[] = { NXP_SPD_EEPROM0 };
	struct ddr_info info;
	struct sysinfo sys;
	long long dram_size;

	zeromem(&sys, sizeof(sys));
	get_clocks(&sys);
	debug("platform clock %lu\n", sys.freq_platform);
	debug("DDR PLL1 %lu\n", sys.freq_ddr_pll0);
	debug("DDR PLL2 %lu\n", sys.freq_ddr_pll1);

	zeromem(&info, sizeof(struct ddr_info));
	info.num_ctlrs = 1;
	info.dimm_on_ctlr = 1;
	info.clk = get_ddr_freq(&sys, 0);
	info.spd_addr = spd_addr;
	info.ddr[0] = (void *)NXP_DDR_ADDR;

	dram_size = dram_init(&info);

	if (dram_size < 0)
		ERROR("DDR init failed.\n");

	erratum_a008850_post();

	return dram_size;
}
