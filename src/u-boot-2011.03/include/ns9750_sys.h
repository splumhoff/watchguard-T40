/***********************************************************************
 *
 * Copyright (C) 2004 by FS Forth-Systeme GmbH.
 * All rights reserved.
 *
 * $Id$
 * @Author: Markus Pietrek
 * @Descr: Definitions for SYS Control Module
 * @References: [1] NS9750 Hardware Reference Manual/December 2003 Chap. 4
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 ***********************************************************************/

#ifndef FS_NS9750_SYS_H
#define FS_NS9750_SYS_H

#define NS9750_SYS_MODULE_BASE		(0xA0900000)

#define get_sys_reg_addr(c) \
	((volatile unsigned int *)(NS9750_SYS_MODULE_BASE+(unsigned int) (c)))

/* the register addresses */

#define NS9750_SYS_AHB_GEN		(0x0000)
#define NS9750_SYS_BRC_BASE		(0x0004)
#define NS9750_SYS_AHB_TIMEOUT		(0x0014)
#define NS9750_SYS_AHB_ERROR1		(0x0018)
#define NS9750_SYS_AHB_ERROR2		(0x001C)
#define NS9750_SYS_AHB_MON		(0x0020)
#define NS9750_SYS_TIMER_COUNT_BASE	(0x0044)
#define NS9750_SYS_TIMER_READ_BASE	(0x0084)
#define NS9750_SYS_INT_VEC_ADR_BASE	(0x00C4)
#define NS9750_SYS_INT_CFG_BASE		(0x0144)
#define NS9750_SYS_ISRADDR		(0x0164)
#define NS9750_SYS_INT_STAT_ACTIVE	(0x0168)
#define NS9750_SYS_INT_STAT_RAW		(0x016C)
#define NS9750_SYS_TIMER_INT_STAT	(0x0170)
#define NS9750_SYS_SW_WDOG_CFG		(0x0174)
#define NS9750_SYS_SW_WDOG_TIMER	(0x0178)
#define NS9750_SYS_CLOCK		(0x017C)
#define NS9750_SYS_RESET		(0x0180)
#define NS9750_SYS_MISC			(0x0184)
#define NS9750_SYS_PLL			(0x0188)
#define NS9750_SYS_ACT_INT_STAT		(0x018C)
#define NS9750_SYS_TIMER_CTRL_BASE	(0x0190)
#define NS9750_SYS_CS_DYN_BASE_BASE	(0x01D0)
#define NS9750_SYS_CS_DYN_MASK_BASE	(0x01D4)
#define NS9750_SYS_CS_STATIC_BASE_BASE	(0x01F0)
#define NS9750_SYS_CS_STATIC_MASK_BASE	(0x01F4)
#define NS9750_SYS_GEN_ID		(0x0210)
#define NS9750_SYS_EXT_INT_CTRL_BASE	(0x0214)

/* the vectored register addresses */

#define NS9750_SYS_TIMER_COUNT(c)	(NS9750_SYS_TIMER_COUNT_BASE + (c))
#define NS9750_SYS_TIMER_READ(c)	(NS9750_SYS_TIMER_READ_BASE + (c))
#define NS9750_SYS_INT_VEC_ADR(c)	(NS9750_SYS_INT_VEC_ADR_BASE + (c))
#define NS9750_SYS_TIMER_CTRL(c)	(NS9750_SYS_TIMER_CTRL_BASE + (c))
/* CS_DYN start with 4 */
#define NS9750_SYS_CS_DYN_BASE(c)	(NS9750_SYS_CS_DYN_BASE_BASE + ((c)-4)*2)
#define NS9750_SYS_CS_DYN_MASK(c)	(NS9750_SYS_CS_DYN_MASK_BASE + ((c)-4)*2)
/* CS_STATIC start with 0 */
#define NS9750_SYS_CS_STATIC_BASE(c)	(NS9750_SYS_CS_STATIC_BASE_BASE + (c)*2)
#define NS9750_SYS_CS_STATIC_MASK(c)	(NS9750_SYS_CS_STATIC_MASK_BASE + (c)*2)
#define NS9750_SYS_EXT_INT_CTRL(c)	(NS9750_SYS_EXT_INT_CTRL + (c))

/* register bit fields */

#define NS9750_SYS_AHB_GEN_EXMAM	(0x00000001)

/* need to be n*8bit to BRC channel */
#define NS9750_SYS_BRC_CEB		(0x00000080)
#define NS9750_SYS_BRC_BRF_MA		(0x00000030)
#define NS9750_SYS_BRC_BRF_100		(0x00000000)
#define NS9750_SYS_BRC_BRF_75		(0x00000010)
#define NS9750_SYS_BRC_BRF_50		(0x00000020)
#define NS9750_SYS_BRC_BRF_25		(0x00000030)

#define NS9750_SYS_AHB_TIMEOUT_BAT_MA	(0xFFFF0000)
#define NS9750_SYS_AHB_TIMEOUT_BMT_MA	(0x0000FFFF)

#define NS9750_SYS_AHB_ERROR2_ABL	(0x00040000)
#define NS9750_SYS_AHB_ERROR2_AER	(0x00020000)
#define NS9750_SYS_AHB_ERROR2_ABM	(0x00010000)
#define NS9750_SYS_AHB_ERROR2_ABA	(0x00008000)
#define NS9750_SYS_AHB_ERROR2_HWRT	(0x00004000)
#define NS9750_SYS_AHB_ERROR2_HMID_MA	(0x00003C00)
#define NS9750_SYS_AHB_ERROR2_HTPC_MA	(0x000003C0)
#define NS9750_SYS_AHB_ERROR2_HSZ_MA	(0x00000038)
#define NS9750_SYS_AHB_ERROR2_RR_MA	(0x00000007)

#define NS9750_SYS_AHB_MON_EIC		(0x00800000)
#define NS9750_SYS_AHB_MON_MBII		(0x00400000)
#define NS9750_SYS_AHB_MON_MBL_MA	(0x003FFFC0)
#define NS9750_SYS_AHB_MON_MBLDC	(0x00000020)
#define NS9750_SYS_AHB_MON_SERDC	(0x00000010)
#define NS9750_SYS_AHB_MON_BMTC_MA	(0x0000000C)
#define NS9750_SYS_AHB_MON_BMTC_RECORD	(0x00000000)
#define NS9750_SYS_AHB_MON_BMTC_GEN_IRQ	(0x00000004)
#define NS9750_SYS_AHB_MON_BMTC_GEN_RES	(0x00000008)
#define NS9750_SYS_AHB_MON_BATC_MA	(0x00000003)
#define NS9750_SYS_AHB_MON_BATC_RECORD	(0x00000000)
#define NS9750_SYS_AHB_MON_BATC_GEN_IRQ	(0x00000001)
#define NS9750_SYS_AHB_MON_BATC_GEN_RES	(0x00000002)

/* need to be n*8bit to Int Level */

#define NS9750_SYS_INT_CFG_IE		(0x00000080)
#define NS9750_SYS_INT_CFG_IT		(0x00000020)
#define NS9750_SYS_INT_CFG_IAD_MA	(0x0000001F)

#define NS9750_SYS_TIMER_INT_STAT_MA	(0x0000FFFF)

#define NS9750_SYS_SW_WDOG_CFG_SWWE	(0x00000080)
#define NS9750_SYS_SW_WDOG_CFG_SWWI	(0x00000020)
#define NS9750_SYS_SW_WDOG_CFG_SWWIC	(0x00000010)
#define NS9750_SYS_SW_WDOG_CFG_SWTCS_MA	(0x00000007)
#define NS9750_SYS_SW_WDOG_CFG_SWTCS_2	(0x00000000)
#define NS9750_SYS_SW_WDOG_CFG_SWTCS_4	(0x00000001)
#define NS9750_SYS_SW_WDOG_CFG_SWTCS_8	(0x00000002)
#define NS9750_SYS_SW_WDOG_CFG_SWTCS_16	(0x00000003)
#define NS9750_SYS_SW_WDOG_CFG_SWTCS_32	(0x00000004)
#define NS9750_SYS_SW_WDOG_CFG_SWTCS_64	(0x00000005)

#define NS9750_SYS_CLOCK_LPCS_MA	(0x00000380)
#define NS9750_SYS_CLOCK_LPCS_1		(0x00000000)
#define NS9750_SYS_CLOCK_LPCS_2		(0x00000080)
#define NS9750_SYS_CLOCK_LPCS_4		(0x00000100)
#define NS9750_SYS_CLOCK_LPCS_8		(0x00000180)
#define NS9750_SYS_CLOCK_LPCS_EXT	(0x00000200)
#define NS9750_SYS_CLOCK_BBC		(0x00000040)
#define NS9750_SYS_CLOCK_LCC		(0x00000020)
#define NS9750_SYS_CLOCK_MCC		(0x00000010)
#define NS9750_SYS_CLOCK_PARBC		(0x00000008)
#define NS9750_SYS_CLOCK_PC		(0x00000004)
#define NS9750_SYS_CLOCK_MACC		(0x00000001)

#define NS9750_SYS_RESET_SR		(0x80000000)
#define NS9750_SYS_RESET_I2CW		(0x00100000)
#define NS9750_SYS_RESET_CSE		(0x00080000)
#define NS9750_SYS_RESET_SMWE		(0x00040000)
#define NS9750_SYS_RESET_EWE		(0x00020000)
#define NS9750_SYS_RESET_PI3WE		(0x00010000)
#define NS9750_SYS_RESET_BBT		(0x00000040)
#define NS9750_SYS_RESET_LCDC		(0x00000020)
#define NS9750_SYS_RESET_MEMC		(0x00000010)
#define NS9750_SYS_RESET_PCIAR		(0x00000008)
#define NS9750_SYS_RESET_PCIM		(0x00000004)
#define NS9750_SYS_RESET_MACM		(0x00000001)

#define NS9750_SYS_MISC_REV_MA		(0xFF000000)
#define NS9750_SYS_MISC_PCIA		(0x00002000)
#define NS9750_SYS_MISC_VDIS		(0x00001000)
#define NS9750_SYS_MISC_BMM		(0x00000800)
#define NS9750_SYS_MISC_CS1DB		(0x00000400)
#define NS9750_SYS_MISC_CS1DW_MA	(0x00000300)
#define NS9750_SYS_MISC_MCCM		(0x00000080)
#define NS9750_SYS_MISC_PMSS		(0x00000040)
#define NS9750_SYS_MISC_CS1P		(0x00000020)
#define NS9750_SYS_MISC_ENDM		(0x00000008)
#define NS9750_SYS_MISC_MBAR		(0x00000004)
#define NS9750_SYS_MISC_IRAM0		(0x00000001)

#define NS9750_SYS_PLL_PLLBS		(0x02000000)
#define NS9750_SYS_PLL_PLLFS_MA		(0x01800000)
#define NS9750_SYS_PLL_PLLIS_MA		(0x00600000)
#define NS9750_SYS_PLL_PLLND_MA		(0x001F0000)
#define NS9750_SYS_PLL_PLLSW		(0x00008000)
#define NS9750_SYS_PLL_PLLBSSW		(0x00000200)
#define NS9750_SYS_PLL_FSEL_MA		(0x00000180)
#define NS9750_SYS_PLL_CPCC_MA		(0x00000060)
#define NS9750_SYS_PLL_NDSW_MA		(0x0000001F)

#define NS9750_SYS_ACT_INT_STAT_MA	(0x0000FFFF)

#define NS9750_SYS_TIMER_CTRL_TEN	(0x00008000)
#define NS9750_SYS_TIMER_CTRL_INTC	(0x00000200)
#define NS9750_SYS_TIMER_CTRL_TLCS_MA	(0x000001C0)
#define NS9750_SYS_TIMER_CTRL_TLCS_1	(0x00000000)
#define NS9750_SYS_TIMER_CTRL_TLCS_2	(0x00000040)
#define NS9750_SYS_TIMER_CTRL_TLCS_4	(0x00000080)
#define NS9750_SYS_TIMER_CTRL_TLCS_8	(0x000000C0)
#define NS9750_SYS_TIMER_CTRL_TLCS_16	(0x00000100)
#define NS9750_SYS_TIMER_CTRL_TLCS_32	(0x00000140)
#define NS9750_SYS_TIMER_CTRL_TLCS_64	(0x00000180)
#define NS9750_SYS_TIMER_CTRL_TLCS_EXT	(0x000001C0)
#define NS9750_SYS_TIMER_CTRL_TM_MA	(0x00000030)
#define NS9750_SYS_TIMER_CTRL_TM_INT	(0x00000000)
#define NS9750_SYS_TIMER_CTRL_TM_LOW	(0x00000010)
#define NS9750_SYS_TIMER_CTRL_TM_HIGH	(0x00000020)
#define NS9750_SYS_TIMER_CTRL_INTS	(0x00000008)
#define NS9750_SYS_TIMER_CTRL_UDS	(0x00000004)
#define NS9750_SYS_TIMER_CTRL_TSZ	(0x00000002)
#define NS9750_SYS_TIMER_CTRL_REN	(0x00000001)

#define NS9750_SYS_EXT_INT_CTRL_STS	(0x00000008)
#define NS9750_SYS_EXT_INT_CTRL_CLR	(0x00000004)
#define NS9750_SYS_EXT_INT_CTRL_PLTY	(0x00000002)
#define NS9750_SYS_EXT_INT_CTRL_LVEDG	(0x00000001)

#endif /* FS_NS9750_SYS_H */
