/*
 * Copyright 2018 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Author Pankaj Gupta <pankaj.gupta@nxp.com>
 */

#include <platform_def.h>
#include <arch.h>
#include <ccn.h>
#include <plat_arm.h>

/******************************************************************************
 * Helper function to place current master into coherency
 *****************************************************************************/
void plat_ls_interconnect_enter_coherency(unsigned int num_clusters)
{
	ccn_enter_snoop_dvm_domain(1 << MPIDR_AFFLVL1_VAL(read_mpidr_el1()));

	for (uint32_t index = 1; index < num_clusters; index++) {
		ccn_enter_snoop_dvm_domain(1 << index);
	}
}

/******************************************************************************
 * Helper function to remove current master from coherency
 *****************************************************************************/
void plat_ls_interconnect_exit_coherency(void)
{
	ccn_exit_snoop_dvm_domain(1 << MPIDR_AFFLVL1_VAL(read_mpidr_el1()));
}
