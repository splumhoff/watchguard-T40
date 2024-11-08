// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2015 Freescale Semiconductor, Inc.
 */

#include <common.h>
#include <i2c.h>
#include <fdt_support.h>
#include <fsl_ddr_sdram.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/fsl_serdes.h>
#include <asm/arch/ppa.h>
#include <asm/arch/fdt.h>
#include <asm/arch/mmu.h>
#include <asm/arch/cpu.h>
#include <asm/arch/soc.h>
#include <asm/arch-fsl-layerscape/fsl_icid.h>
#include <asm/gpio.h>
#include <ahci.h>
#include <hwconfig.h>
#include <mmc.h>
#include <scsi.h>
#include <fm_eth.h>
#include <fsl_esdhc.h>
#include <fsl_ifc.h>
#include <spl.h>

#include "../common/qixis.h"
#include "ls1043aqds_qixis.h"

DECLARE_GLOBAL_DATA_PTR;

enum {
	MUX_TYPE_GPIO,
};

/* LS1043AQDS serdes mux */
#define CFG_SD_MUX1_SLOT2	0x0 /* SLOT2 TX/RX0 */
#define CFG_SD_MUX1_SLOT1	0x1 /* SLOT1 TX/RX1 */
#define CFG_SD_MUX2_SLOT3	0x0 /* SLOT3 TX/RX0 */
#define CFG_SD_MUX2_SLOT1	0x1 /* SLOT1 TX/RX2 */
#define CFG_SD_MUX3_SLOT4	0x0 /* SLOT4 TX/RX0 */
#define CFG_SD_MUX3_MUX4	0x1 /* MUX4 */
#define CFG_SD_MUX4_SLOT3	0x0 /* SLOT3 TX/RX1 */
#define CFG_SD_MUX4_SLOT1	0x1 /* SLOT1 TX/RX3 */
#define CFG_UART_MUX_MASK	0x6
#define CFG_UART_MUX_SHIFT	1
#define CFG_LPUART_EN		0x1

#ifdef CONFIG_TFABOOT
struct ifc_regs ifc_cfg_nor_boot[CONFIG_SYS_FSL_IFC_BANK_COUNT] = {
	{
		"nor0",
		CONFIG_SYS_NOR0_CSPR,
		CONFIG_SYS_NOR0_CSPR_EXT,
		CONFIG_SYS_NOR_AMASK,
		CONFIG_SYS_NOR_CSOR,
		{
			CONFIG_SYS_NOR_FTIM0,
			CONFIG_SYS_NOR_FTIM1,
			CONFIG_SYS_NOR_FTIM2,
			CONFIG_SYS_NOR_FTIM3
		},

	},
	{
		"nor1",
		CONFIG_SYS_NOR1_CSPR,
		CONFIG_SYS_NOR1_CSPR_EXT,
		CONFIG_SYS_NOR_AMASK,
		CONFIG_SYS_NOR_CSOR,
		{
			CONFIG_SYS_NOR_FTIM0,
			CONFIG_SYS_NOR_FTIM1,
			CONFIG_SYS_NOR_FTIM2,
			CONFIG_SYS_NOR_FTIM3
		},
	},
	{
		"nand",
		CONFIG_SYS_NAND_CSPR,
		CONFIG_SYS_NAND_CSPR_EXT,
		CONFIG_SYS_NAND_AMASK,
		CONFIG_SYS_NAND_CSOR,
		{
			CONFIG_SYS_NAND_FTIM0,
			CONFIG_SYS_NAND_FTIM1,
			CONFIG_SYS_NAND_FTIM2,
			CONFIG_SYS_NAND_FTIM3
		},
	},
	{
		"fpga",
		CONFIG_SYS_FPGA_CSPR,
		CONFIG_SYS_FPGA_CSPR_EXT,
		CONFIG_SYS_FPGA_AMASK,
		CONFIG_SYS_FPGA_CSOR,
		{
			CONFIG_SYS_FPGA_FTIM0,
			CONFIG_SYS_FPGA_FTIM1,
			CONFIG_SYS_FPGA_FTIM2,
			CONFIG_SYS_FPGA_FTIM3
		},
	}
};

struct ifc_regs ifc_cfg_nand_boot[CONFIG_SYS_FSL_IFC_BANK_COUNT] = {
	{
		"nand",
		CONFIG_SYS_NAND_CSPR,
		CONFIG_SYS_NAND_CSPR_EXT,
		CONFIG_SYS_NAND_AMASK,
		CONFIG_SYS_NAND_CSOR,
		{
			CONFIG_SYS_NAND_FTIM0,
			CONFIG_SYS_NAND_FTIM1,
			CONFIG_SYS_NAND_FTIM2,
			CONFIG_SYS_NAND_FTIM3
		},
	},
	{
		"nor0",
		CONFIG_SYS_NOR0_CSPR,
		CONFIG_SYS_NOR0_CSPR_EXT,
		CONFIG_SYS_NOR_AMASK,
		CONFIG_SYS_NOR_CSOR,
		{
			CONFIG_SYS_NOR_FTIM0,
			CONFIG_SYS_NOR_FTIM1,
			CONFIG_SYS_NOR_FTIM2,
			CONFIG_SYS_NOR_FTIM3
		},
	},
	{
		"nor1",
		CONFIG_SYS_NOR1_CSPR,
		CONFIG_SYS_NOR1_CSPR_EXT,
		CONFIG_SYS_NOR_AMASK,
		CONFIG_SYS_NOR_CSOR,
		{
			CONFIG_SYS_NOR_FTIM0,
			CONFIG_SYS_NOR_FTIM1,
			CONFIG_SYS_NOR_FTIM2,
			CONFIG_SYS_NOR_FTIM3
		},
	},
	{
		"fpga",
		CONFIG_SYS_FPGA_CSPR,
		CONFIG_SYS_FPGA_CSPR_EXT,
		CONFIG_SYS_FPGA_AMASK,
		CONFIG_SYS_FPGA_CSOR,
		{
			CONFIG_SYS_FPGA_FTIM0,
			CONFIG_SYS_FPGA_FTIM1,
			CONFIG_SYS_FPGA_FTIM2,
			CONFIG_SYS_FPGA_FTIM3
		},
	}
};

void ifc_cfg_boot_info(struct ifc_regs_info *regs_info)
{
	enum boot_src src = get_boot_src();

	if (src == BOOT_SOURCE_IFC_NAND)
		regs_info->regs = ifc_cfg_nand_boot;
	else
		regs_info->regs = ifc_cfg_nor_boot;
	regs_info->cs_size = CONFIG_SYS_FSL_IFC_BANK_COUNT;
}
#endif

int checkboard(void)
{
#ifdef CONFIG_TFABOOT
	enum boot_src src = get_boot_src();
#endif
#ifndef CONFIG_SD_BOOT
	u8 sw;
#endif

	puts("Board: LS1043AQDS, boot from ");

#ifdef CONFIG_TFABOOT
	if (src == BOOT_SOURCE_SD_MMC)
		puts("SD\n");
	else {
#endif

#ifdef CONFIG_SD_BOOT
		puts("SD\n");
#else
		sw = QIXIS_READ(brdcfg[0]);
		sw = (sw & QIXIS_LBMAP_MASK) >> QIXIS_LBMAP_SHIFT;

		if (sw < 0x8)
			printf("vBank: %d\n", sw);
		else if (sw == 0x8)
			puts("PromJet\n");
		else if (sw == 0x9)
			puts("NAND\n");
		else if (sw == 0xF)
			printf("QSPI\n");
		else
			printf("invalid setting of SW%u\n", QIXIS_LBMAP_SWITCH);
#endif

#ifdef CONFIG_TFABOOT
	}
#endif

	return 0;
}

bool if_board_diff_clk(void)
{
	u8 diff_conf = QIXIS_READ(brdcfg[11]);

	return diff_conf & 0x40;
}

unsigned long get_board_sys_clk(void)
{
	u8 sysclk_conf = QIXIS_READ(brdcfg[1]);

	switch (sysclk_conf & 0x0f) {
	case QIXIS_SYSCLK_64:
		return 64000000;
	case QIXIS_SYSCLK_83:
		return 83333333;
	case QIXIS_SYSCLK_100:
		return 100000000;
	case QIXIS_SYSCLK_125:
		return 125000000;
	case QIXIS_SYSCLK_133:
		return 133333333;
	case QIXIS_SYSCLK_150:
		return 150000000;
	case QIXIS_SYSCLK_160:
		return 160000000;
	case QIXIS_SYSCLK_166:
		return 166666666;
	}

	return 66666666;
}

unsigned long get_board_ddr_clk(void)
{
	u8 ddrclk_conf = QIXIS_READ(brdcfg[1]);

	if (if_board_diff_clk())
		return get_board_sys_clk();
	switch ((ddrclk_conf & 0x30) >> 4) {
	case QIXIS_DDRCLK_100:
		return 100000000;
	case QIXIS_DDRCLK_125:
		return 125000000;
	case QIXIS_DDRCLK_133:
		return 133333333;
	}

	return 66666666;
}

int dram_init(void)
{
	fsl_initdram();
#if (!defined(CONFIG_SPL) && !defined(CONFIG_TFABOOT)) || \
	defined(CONFIG_SPL_BUILD)
	/* This will break-before-make MMU for DDR */
	update_early_mmu_table();
#endif

	return 0;
}

void board_usb_cfg_reset(void)
{
#ifdef CONFIG_HAS_FSL_XHCI_USB
	struct ccsr_scfg *scfg = (struct ccsr_scfg *)CONFIG_SYS_FSL_SCFG_ADDR;
	u32 usb_pwrfault;

	if(is_t40cw()) /* Senao LTE patch for T40-CW */
	{
		out_be32(&scfg->rcwpmuxcr0, 0x1111);
		out_be32(&scfg->usbdrvvbus_selcr, SCFG_USBDRVVBUS_SELCR_USB1);
		usb_pwrfault =
			(SCFG_USBPWRFAULT_DEDICATED << SCFG_USBPWRFAULT_USB3_SHIFT) |
			(SCFG_USBPWRFAULT_INACTIVE << SCFG_USBPWRFAULT_USB2_SHIFT)  |
			(SCFG_USBPWRFAULT_SHARED << SCFG_USBPWRFAULT_USB1_SHIFT);
		out_be32(&scfg->usbpwrfault_selcr, usb_pwrfault);
		/* Senao LTE patch for T40-CW: reset IGT */
		gpio_direction_output(403, 1);
		udelay(120000);
		gpio_direction_output(403, 0);
		puts("T40-CW: USB configuration for LTE... OK\n");
	}
#endif
}

int board_early_init_f(void)
{
#ifdef CONFIG_HAS_FSL_XHCI_USB
	struct ccsr_scfg *scfg = (struct ccsr_scfg *)CONFIG_SYS_FSL_SCFG_ADDR;
	u32 usb_pwrfault;
#endif
#ifdef CONFIG_LPUART
	u8 uart;
#endif

#ifdef CONFIG_SYS_I2C_EARLY_INIT
	i2c_early_init_f();
#endif
	fsl_lsch2_early_init_f();

#ifdef CONFIG_HAS_FSL_XHCI_USB
	out_be32(&scfg->rcwpmuxcr0, 0x3333);
	out_be32(&scfg->usbdrvvbus_selcr, SCFG_USBDRVVBUS_SELCR_USB1);
	usb_pwrfault =
		(SCFG_USBPWRFAULT_DEDICATED << SCFG_USBPWRFAULT_USB3_SHIFT) |
		(SCFG_USBPWRFAULT_DEDICATED << SCFG_USBPWRFAULT_USB2_SHIFT) |
		(SCFG_USBPWRFAULT_SHARED << SCFG_USBPWRFAULT_USB1_SHIFT);
	out_be32(&scfg->usbpwrfault_selcr, usb_pwrfault);
#endif

#ifdef CONFIG_HAS_GPIO4_12_13
	out_be32(&scfg->rcwpmuxcr0, (in_be32(&scfg->rcwpmuxcr0) & 0xff00) | 0x11);
#endif

#ifdef CONFIG_LPUART
	/* We use lpuart0 as system console */
	uart = QIXIS_READ(brdcfg[14]);
	uart &= ~CFG_UART_MUX_MASK;
	uart |= CFG_LPUART_EN << CFG_UART_MUX_SHIFT;
	QIXIS_WRITE(brdcfg[14], uart);
#endif

	return 0;
}

#ifdef CONFIG_FSL_DEEP_SLEEP
/* determine if it is a warm boot */
bool is_warm_boot(void)
{
#define DCFG_CCSR_CRSTSR_WDRFR	(1 << 3)
	struct ccsr_gur __iomem *gur = (void *)CONFIG_SYS_FSL_GUTS_ADDR;

	if (in_be32(&gur->crstsr) & DCFG_CCSR_CRSTSR_WDRFR)
		return 1;

	return 0;
}
#endif

int config_board_mux(int ctrl_type)
{
	u8 reg14;

	reg14 = QIXIS_READ(brdcfg[14]);

	switch (ctrl_type) {
	case MUX_TYPE_GPIO:
		reg14 = (reg14 & (~0x30)) | 0x20;
		break;
	default:
		puts("Unsupported mux interface type\n");
		return -1;
	}

	QIXIS_WRITE(brdcfg[14], reg14);

	return 0;
}

int config_serdes_mux(void)
{
	return 0;
}


#ifdef CONFIG_MISC_INIT_R
int misc_init_r(void)
{
	if (hwconfig("gpio"))
		config_board_mux(MUX_TYPE_GPIO);

	return 0;
}
#endif

int board_init(void)
{
#ifdef CONFIG_SYS_FSL_ERRATUM_A010315
	erratum_a010315();
#endif

#ifdef CONFIG_SYS_FSL_SERDES
	config_serdes_mux();
#endif

#ifdef CONFIG_FSL_LS_PPA
	ppa_init();
#endif

	/* SENAO: init gpio */

	gpio_direction_output(114, 1); /* Watchdog pulse output */
	gpio_direction_output(123, 1); /* TPM_WakeupN */
	gpio_direction_output(124, 1); /* RST_TPM_N */
	gpio_direction_output(125, 1); /* TPM_Express */

	gpio_direction_output(127, 1); /* LED_CLK */
	gpio_direction_output(128, 1); /* LED_DATA */
	gpio_direction_output(129, 1); /* RST_PCIE0_N reset QCA9880 */
	gpio_direction_output(130, 1); /* POE_RST_N_33 */
	gpio_direction_output(205, 1); /* LED_IC_RST_N_R_1V8 */
	gpio_direction_output(206, 1); /* BLUETOOTH_RST_1V8 */
	gpio_direction_output(211, 1); /* RST to VSC8514 */
	gpio_direction_output(212, 1); /* RST to AR8035 */
	gpio_direction_output(226, 1); /* QSPI_RST_N */
	gpio_direction_output(315, 1);
	gpio_direction_output(316, 1);
	gpio_direction_output(326, 1); /* WatchDog enabl>> Active Low */
	gpio_direction_output(413, 1);

	/* Workaround for resetting QCA9880 */
	gpio_direction_output(129, 0); /* RST_PCIE0_N reset QCA9880 */
	udelay(10000); //keeping 10ms L
	gpio_direction_output(129, 1); /* RST_PCIE0_N reset QCA9880 */

	/* reset AR8035 */
	gpio_direction_output(212, 1); /* RST to AR8035 */

	/*
	   Watchguard Wants to support both NEW (88E1545) and OLD (VSC8514) Boards.
	   We will extend the udelay to 20 ms so that way it supports both PHYs
	*/
	/* reset 88E1545 or VSC8514 */
	gpio_direction_output(211, 0); /* RST to 88E1545 or VSC8514 */
	udelay(20000); //keeping 20ms L
	gpio_direction_output(211, 1); /* RST to 88E1545 or VSC8514 */

	/* Talor Lin: If model is T40-CW, then USB configuration should be changed for LTE */
	board_usb_cfg_reset();

	return 0;
}

#ifdef CONFIG_OF_BOARD_SETUP
int ft_board_setup(void *blob, bd_t *bd)
{
	u64 base[CONFIG_NR_DRAM_BANKS];
	u64 size[CONFIG_NR_DRAM_BANKS];
	u8 reg;

	/* fixup DT for the two DDR banks */
	base[0] = gd->bd->bi_dram[0].start;
	size[0] = gd->bd->bi_dram[0].size;
	base[1] = gd->bd->bi_dram[1].start;
	size[1] = gd->bd->bi_dram[1].size;

	fdt_fixup_memory_banks(blob, base, size, 2);
	ft_cpu_setup(blob, bd);

#ifdef CONFIG_SYS_DPAA_FMAN
	fdt_fixup_fman_ethernet(blob);
#endif

	fdt_fixup_icid(blob);

	reg = QIXIS_READ(brdcfg[0]);
	reg = (reg & QIXIS_LBMAP_MASK) >> QIXIS_LBMAP_SHIFT;

	/* Disable IFC if QSPI is enabled */
	if (reg == 0xF)
		do_fixup_by_compat(blob, "fsl,ifc",
				   "status", "disabled", 8 + 1, 1);

	return 0;
}
#endif

u8 flash_read8(void *addr)
{
	return __raw_readb(addr + 1);
}

void flash_write16(u16 val, void *addr)
{
	u16 shftval = (((val >> 8) & 0xff) | ((val << 8) & 0xff00));

	__raw_writew(shftval, addr);
}

u16 flash_read16(void *addr)
{
	u16 val = __raw_readw(addr);

	return (((val) >> 8) & 0x00ff) | (((val) << 8) & 0xff00);
}

#ifdef CONFIG_TFABOOT
void *env_sf_get_env_addr(void)
{
	return (void *)(CONFIG_SYS_FSL_QSPI_BASE + CONFIG_ENV_OFFSET);
}
#endif
