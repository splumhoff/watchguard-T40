/*
 * Copyright 2008 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 as published by the Free Software Foundation.
 */

#include <common.h>
#include <i2c.h>

#include <asm/fsl_ddr_sdram.h>
#include <asm/fsl_ddr_dimm_params.h>

static void get_spd(ddr2_spd_eeprom_t *spd, unsigned char i2c_address)
{
	i2c_read(i2c_address, 0, 1, (uchar *)spd, sizeof(ddr2_spd_eeprom_t));
}

void fsl_ddr_get_spd(ddr2_spd_eeprom_t *ctrl_dimms_spd,
		      unsigned int ctrl_num)
{
	unsigned int i;
	unsigned int i2c_address = 0;

	for (i = 0; i < CONFIG_DIMM_SLOTS_PER_CTLR; i++) {
		if (ctrl_num == 0 && i == 0) {
			i2c_address = SPD_EEPROM_ADDRESS1;
		}
		if (ctrl_num == 1 && i == 0) {
			i2c_address = SPD_EEPROM_ADDRESS2;
		}
		get_spd(&(ctrl_dimms_spd[i]), i2c_address);
	}
}

typedef struct {
	u32 datarate_mhz_low;
	u32 datarate_mhz_high;
	u32 n_ranks;
	u32 clk_adjust;
	u32 cpo;
	u32 write_data_delay;
	u32 force_2T;
} board_specific_parameters_t;

/*
 * CPO value doesn't matter if workaround for errata 111 and 134 enabled.
 *
 * For DDR2 DIMM, all combinations of clk_adjust and write_data_delay have been
 * tested. For RDIMM, clk_adjust = 4 and write_data_delay = 3 is optimized for
 * all clocks from 400MT/s to 800MT/s, verified with Kingston KVR800D2D8P6/2G.
 * For UDIMM, clk_adjust = 8 and write_delay = 5 is optimized for all clocks
 * from 400MT/s to 800MT/s, verified with Micron MT18HTF25672AY-800E1.
 */
const board_specific_parameters_t board_specific_parameters_udimm[][20] = {
	{
	/*
	 *	memory controller 0
	 *	  lo|  hi|  num|  clk| cpo|wrdata|2T
	 *	 mhz| mhz|ranks|adjst|    | delay|
	 */
		{  0, 333,    2,    8,   7,    5,  0},
		{334, 400,    2,    8,   9,    5,  0},
		{401, 549,    2,    8,  11,    5,  0},
		{550, 680,    2,    8,  10,    5,  0},
		{681, 850,    2,    8,  12,    5,  1},
		{  0, 333,    1,    6,   7,    3,  0},
		{334, 400,    1,    6,   9,    3,  0},
		{401, 549,    1,    6,  11,    3,  0},
		{550, 680,    1,    1,  10,    5,  0},
		{681, 850,    1,    1,  12,    5,  0}
	},

	{
	/*
	 *	memory controller 1
	 *	  lo|  hi|  num|  clk| cpo|wrdata|2T
	 *	 mhz| mhz|ranks|adjst|    | delay|
	 */
		{  0, 333,    2,     8,  7,    5,  0},
		{334, 400,    2,     8,  9,    5,  0},
		{401, 549,    2,     8, 11,    5,  0},
		{550, 680,    2,     8, 11,    5,  0},
		{681, 850,    2,     8, 13,    5,  1},
		{  0, 333,    1,     6,  7,    3,  0},
		{334, 400,    1,     6,  9,    3,  0},
		{401, 549,    1,     6, 11,    3,  0},
		{550, 680,    1,     1, 11,    6,  0},
		{681, 850,    1,     1, 13,    6,  0}
	}
};

const board_specific_parameters_t board_specific_parameters_rdimm[][20] = {
	{
	/*
	 *	memory controller 0
	 *	  lo|  hi|  num|  clk| cpo|wrdata|2T
	 *	 mhz| mhz|ranks|adjst|    | delay|
	 */
		{  0, 333,    2,    4,   7,    3,  0},
		{334, 400,    2,    4,   9,    3,  0},
		{401, 549,    2,    4,  11,    3,  0},
		{550, 680,    2,    4,  10,    3,  0},
		{681, 850,    2,    4,  12,    3,  1},
	},

	{
	/*
	 *	memory controller 1
	 *	  lo|  hi|  num|  clk| cpo|wrdata|2T
	 *	 mhz| mhz|ranks|adjst|    | delay|
	 */
		{  0, 333,    2,     4,  7,    3,  0},
		{334, 400,    2,     4,  9,    3,  0},
		{401, 549,    2,     4, 11,    3,  0},
		{550, 680,    2,     4, 11,    3,  0},
		{681, 850,    2,     4, 13,    3,  1},
	}
};

void fsl_ddr_board_options(memctl_options_t *popts,
				dimm_params_t *pdimm,
				unsigned int ctrl_num)
{
	const board_specific_parameters_t *pbsp;
	u32 num_params;
	u32 i;
	ulong ddr_freq;
	int matched = 0;

	if (!pdimm->n_ranks)
		return;

	if (popts->registered_dimm_en) {
		pbsp = &(board_specific_parameters_rdimm[ctrl_num][0]);
		num_params = sizeof(board_specific_parameters_rdimm[ctrl_num]) /
				sizeof(board_specific_parameters_rdimm[0][0]);
	} else {
		pbsp = &(board_specific_parameters_udimm[ctrl_num][0]);
		num_params = sizeof(board_specific_parameters_udimm[ctrl_num]) /
				sizeof(board_specific_parameters_udimm[0][0]);
	}

	/* set odt_rd_cfg and odt_wr_cfg. If the there is only one dimm in
	 * that controller, set odt_wr_cfg to 4 for CS0, and 0 to CS1. If
	 * there are two dimms in the controller, set odt_rd_cfg to 3 and
	 * odt_wr_cfg to 3 for the even CS, 0 for the odd CS.
	 */
	for (i = 0; i < CONFIG_CHIP_SELECTS_PER_CTRL; i++) {
		if (i&1) {	/* odd CS */
			popts->cs_local_opts[i].odt_rd_cfg = 0;
			popts->cs_local_opts[i].odt_wr_cfg = 0;
		} else {	/* even CS */
			if (CONFIG_DIMM_SLOTS_PER_CTLR == 1) {
				popts->cs_local_opts[i].odt_rd_cfg = 0;
				popts->cs_local_opts[i].odt_wr_cfg = 4;
			} else if (CONFIG_DIMM_SLOTS_PER_CTLR == 2) {
			popts->cs_local_opts[i].odt_rd_cfg = 3;
			popts->cs_local_opts[i].odt_wr_cfg = 3;
			}
		}
	}

	/* Get clk_adjust, cpo, write_data_delay,2T, according to the board ddr
	 * freqency and n_banks specified in board_specific_parameters table.
	 */
	ddr_freq = get_ddr_freq(0) / 1000000;
	for (i = 0; i < num_params; i++) {
		if (ddr_freq >= pbsp->datarate_mhz_low &&
		    ddr_freq <= pbsp->datarate_mhz_high &&
		    pdimm->n_ranks == pbsp->n_ranks) {
			popts->clk_adjust = pbsp->clk_adjust;
			popts->cpo_override = pbsp->cpo;
			popts->write_data_delay = pbsp->write_data_delay;
			popts->twoT_en = pbsp->force_2T;
			matched = 1;
			break;
		}
		pbsp++;
	}

	if (!matched)
		printf("Warning: board specific timing not found!\n");

	/*
	 * Factors to consider for half-strength driver enable:
	 *	- number of DIMMs installed
	 */
	popts->half_strength_driver_enable = 0;
}
