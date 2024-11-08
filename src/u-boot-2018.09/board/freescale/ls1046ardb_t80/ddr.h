/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 */

#ifndef __DDR_H__
#define __DDR_H__

void erratum_a008850_post(void);

struct board_specific_parameters {
	u32 n_ranks;
	u32 datarate_mhz_high;
	u32 rank_gb;
	u32 clk_adjust;
	u32 wrlvl_start;
	u32 wrlvl_ctl_2;
	u32 wrlvl_ctl_3;
};

/*
 * These tables contain all valid speeds we want to override with board
 * specific parameters. datarate_mhz_high values need to be in ascending order
 * for each n_ranks group.
 */
static const struct board_specific_parameters udimm0[] = {
	/*
	 * memory controller 0
	 *   num|  hi| rank|  clk| wrlvl |   wrlvl   |  wrlvl
	 * ranks| mhz| GB  |adjst| start |   ctl2    |  ctl3
	 */
	{2,  1350, 0, 8,     6, 0x0708090B, 0x0C0D0E09,},
	{2,  1666, 0, 8,     7, 0x08090A0C, 0x0D0F100B,},
	{2,  1900, 0, 8,     7, 0x09090B0D, 0x0E10120B,},
	{2,  2300, 0, 8,     9, 0x0A0B0C10, 0x1213140E,},
#ifdef CONFIG_WG1008_VM1P
/*Albert.Ke.B*/
/*
    {1,  1350, 0, 8,     6, 0x0708090B, 0x0C0D0E09,},
    {1,  1666, 0, 8,     7, 0x08090A0C, 0x0D0F100B,},
    {1,  1900, 0, 8,     7, 0x09090B0D, 0x0E10120B,},
    {1,  2300, 0, 8,     9, 0x0A0B0C10, 0x1213140E,},
*/
    {1,  1350, 0, 9,     6, 0x06070709, 0x090A0A0C,},
    {1,  1666, 0, 9,     6, 0x06070709, 0x090A0A0C,},
    {1,  1900, 0, 9,     6, 0x06070709, 0x090A0A0C,},
    {1,  2300, 0, 9,     6, 0x06070709, 0x090A0A0C,},
/*Albert.Ke.E*/
#endif /* ifdef CONFIG_WG1008_VM1P */
    {}
};

static const struct board_specific_parameters *udimms[] = {
	udimm0,
};

static const struct board_specific_parameters rdimm0[] = {
	/*
	 * memory controller 0
	 *   num|  hi| rank|  clk| wrlvl |   wrlvl   |  wrlvl
	 * ranks| mhz| GB  |adjst| start |   ctl2    |  ctl3
	 */
	{2,  1666, 0, 0x8,     0x0D, 0x0C0B0A08, 0x0A0B0C08,},
	{2,  1900, 0, 0x8,     0x0E, 0x0D0C0B09, 0x0B0C0D09,},
	{2,  2300, 0, 0xa,     0x12, 0x100F0D0C, 0x0E0F100C,},
	{1,  1666, 0, 0x8,     0x0D, 0x0C0B0A08, 0x0A0B0C08,},
	{1,  1900, 0, 0x8,     0x0E, 0x0D0C0B09, 0x0B0C0D09,},
	{1,  2300, 0, 0xa,     0x12, 0x100F0D0C, 0x0E0F100C,},
	{}
};

static const struct board_specific_parameters *rdimms[] = {
	rdimm0,
};

/*Albert.Ke.B*/
#ifdef CONFIG_WG1008_VM1P
fsl_ddr_cfg_regs_t ddr_cfg_regs_1600 = {
	.cs[0].bnds = 0x0000007F,
	.cs[1].bnds = 0,
	.cs[2].bnds = 0,
	.cs[3].bnds = 0,
	.cs[0].config = 0x80040322,
	.cs[0].config_2 = 0,
	.cs[1].config = 0,
	.cs[1].config_2 = 0,
	.cs[2].config = 0,
	.cs[3].config = 0,
	.timing_cfg_3 = 0x010C1000,
	.timing_cfg_0 = 0x91550018,
	.timing_cfg_1 = 0xBBB48C42,
	.timing_cfg_2 = 0x0048C111,
	.ddr_sdram_cfg = 0xC50C0008,
	.ddr_sdram_cfg_2 = 0x00401100,
	.ddr_sdram_cfg_3 = 0,
	.ddr_sdram_mode = 0x03010210,
	.ddr_sdram_mode_2 = 0,
	.ddr_sdram_mode_3 = 0x00010210,
	.ddr_sdram_mode_4 = 0,
	.ddr_sdram_mode_5 = 0x00010210,
	.ddr_sdram_mode_6 = 0,
	.ddr_sdram_mode_7 = 0x00010210,
	.ddr_sdram_mode_8 = 0,
	.ddr_sdram_mode_9 = 0x00000500,
	.ddr_sdram_mode_10 = 0x04000000,
	.ddr_sdram_mode_11 = 0x00000400,
	.ddr_sdram_mode_12 = 0x04000000,
	.ddr_sdram_mode_13 = 0x00000400,
	.ddr_sdram_mode_14 = 0x04000000,
	.ddr_sdram_mode_15 = 0x00000400,
	.ddr_sdram_mode_16 = 0x04000000,
	.ddr_sdram_interval = 0x18600618,
	.ddr_data_init = 0xDEADBEEF,
	.ddr_sdram_clk_cntl = 0x03000000,
	.ddr_init_addr = 0,
	.ddr_init_ext_addr = 0,
	.timing_cfg_4 = 0x00000002,
	.timing_cfg_5 = 0x03401400,
	.timing_cfg_6 = 0,
	.timing_cfg_7 = 0x13300000,
	.timing_cfg_8 = 0x02115600,
	.timing_cfg_9 = 0,
	.ddr_zq_cntl = 0x8A090705,
	.ddr_wrlvl_cntl = 0x8675F607,
	.ddr_wrlvl_cntl_2 = 0x07090800,
	.ddr_wrlvl_cntl_3 = 0,
	.ddr_sr_cntr = 0,
	.ddr_sdram_rcw_1 = 0,
	.ddr_sdram_rcw_2 = 0,
	.ddr_cdr1 = 0x80040000,
	.ddr_cdr2 = 0x0000A181,
	.dq_map_0 = 0,
	.dq_map_1 = 0,
	.dq_map_2 = 0,
	.dq_map_3 = 0,
	.debug[28] = 0x00700046,

};

fixed_ddr_parm_t fixed_ddr_parm_0[] = {
	{1550, 1650, &ddr_cfg_regs_1600},
	{0, 0, NULL}
};
#endif
/*Albert.Ke.E*/
#endif
