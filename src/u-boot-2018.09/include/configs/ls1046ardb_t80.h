/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2016 Freescale Semiconductor
 */

#ifndef __LS1046ARDB_T80_H__
#define __LS1046ARDB_T80_H__

#include "ls1046a_common.h"


#if 0
#define DEBUG_UBOOT
#endif
#ifdef DEBUG_UBOOT
#define DBG_PRINTF(format, args...) printf("[%s:%d] "format, __FUNCTION__, __LINE__, ##args)
#else
#define DBG_PRINTF(args...)
#endif

/*Albert.Ke.B*/
#define CONFIG_WG1008_VM1P
/*Albert.Ke.E*/
#define CONFIG_CMD_GPIO

#define CONFIG_SYS_CLK_FREQ		100000000
#define CONFIG_DDR_CLK_FREQ		100000000

#define CONFIG_LAYERSCAPE_NS_ACCESS
#define CONFIG_MISC_INIT_R

#define CONFIG_DIMM_SLOTS_PER_CTLR	1
/* Physical Memory Map */
#define CONFIG_CHIP_SELECTS_PER_CTRL	4

/*Albert.Ke.B*/
#ifdef CONFIG_WG1008_VM1P
/*
#define DEBUG
*/
#define CONFIG_CMD_DM
#define CONFIG_FSL_QSPI
#define CONFIG_DM_SPI
#define CONFIG_DM_SPI_FLASH
#else

#define CONFIG_DDR_SPD
#define SPD_EEPROM_ADDRESS		0x51
#define CONFIG_SYS_SPD_BUS_NUM		0

#endif
/*Albert.Ke.E*/

#define CONFIG_DDR_ECC
#define CONFIG_ECC_INIT_VIA_DDRCONTROLLER
#define CONFIG_MEM_INIT_VALUE           0xdeadbeef
#define CONFIG_FSL_DDR_BIST	/* enable built-in memory test */
#ifndef CONFIG_SPL

/*Albert.Ke.B*/
#ifdef CONFIG_WG1008_VM1P
#define CONFIG_SYS_DDR_RAW_TIMING
#define CONFIG_FSL_DDR_INTERACTIVE  /* Interactive debugging */
#define CONFIG_FSL_DDR_BIST
#define CONFIG_ECC_INIT_VIA_DDRCONTROLLER
#define CONFIG_MEM_INIT_VALUE           0xdeadbeef
#define CONFIG_VERSION_VARIABLE
#else
#define CONFIG_FSL_DDR_INTERACTIVE	/* Interactive debugging */
#endif
/*Albert.Ke.E*/

#define LS1046A_GPIO1_DIR  0x2300000
#define LS1046A_GPIO1_DATA 0x2300008
#define LS1046A_GPIO2_DIR  0x2310000
#define LS1046A_GPIO2_DATA 0x2310008
#define LS1046A_GPIO3_DIR  0x2320000
#define LS1046A_GPIO3_DATA 0x2320008
#define LS1046A_GPIO4_DIR  0x2330000
#define LS1046A_GPIO4_DATA 0x2330008
    
#define GPIO_BUS_RESET_IN 1
#define GPIO_BUS_ATT_LED 1
#define GPIO_BUS_SW_STATUS_LED 1
#define GPIO_BUS_SW_MODE_LED 1
#define GPIO_BUS_FAILOVER_LED 1
#define GPIO_BUS_POWER_LED 1
#define GPIO_BUS_TMP_RESET 1
#define GPIO_BUS_MARVELL_SWITCH_RESET 1
#define GPIO_BUS_RST_MOD_RESET 2
#define GPIO_BUS_FAN_DRIVER_FON 2
#define GPIO_BUS_INT_ALL_ALM_7904D_CPU_N 2
#define GPIO_BUS_MOD_BRD_TYPE_0 2
#define GPIO_BUS_MOD_BRD_TYPE_1 2
#define GPIO_BUS_INT_RTC_PHY_N 2
#define GPIO_BUS_MOD_BAY_PRESENT_N 2
#define GPIO_BUS_LED_CPU_MOD_1 2
#define GPIO_BUS_LED_CPU_MOD_2 2
#define GPIO_BUS_LED_CPU_MOD_3 2
#define GPIO_BUS_LED_CPU_MOD_4 2
    
#define GPIO_SHIFT_RESET_IN 23
#define GPIO_SHIFT_POWER_LED 24
#define GPIO_SHIFT_SW_STATUS_LED 25
#define GPIO_SHIFT_SW_MODE_LED 26
#define GPIO_SHIFT_FAILOVER_LED 27
#define GPIO_SHIFT_ATT_LED 28
#define GPIO_SHIFT_TMP_RESET 29
#define GPIO_SHIFT_MARVELL_SWITCH_RESET 30
#define GPIO_SHIFT_RST_MOD_RESET 4
#define GPIO_SHIFT_FAN_DRIVER_FON 5
#define GPIO_SHIFT_INT_ALL_ALM_7904D_CPU_N 6
#define GPIO_SHIFT_MOD_BRD_TYPE_0 7
#define GPIO_SHIFT_MOD_BRD_TYPE_1 8
#define GPIO_SHIFT_INT_RTC_PHY_N 9
#define GPIO_SHIFT_MOD_BAY_PRESENT_N 10
#define GPIO_SHIFT_LED_CPU_MOD_1 0
#define GPIO_SHIFT_LED_CPU_MOD_2 1
#define GPIO_SHIFT_LED_CPU_MOD_3 2
#define GPIO_SHIFT_LED_CPU_MOD_4 3
    
    /* GPIO Direction, 1:output; 0:input */
#define GPIO_DIR_RESET_IN 0
#define GPIO_DIR_ATT_LED 1
#define GPIO_DIR_SW_STATUS_LED 1
#define GPIO_DIR_SW_MODE_LED 1
#define GPIO_DIR_FAILOVER_LED 1
#define GPIO_DIR_POWER_LED 1
#define GPIO_DIR_TMP_RESET 1
#define GPIO_DIR_MARVELL_SWITCH_RESET 1
#define GPIO_DIR_RST_MOD_RESET 1
#define GPIO_DIR_FAN_DRIVER_FON 1
#define GPIO_DIR_INT_ALL_ALM_7904D_CPU_N 0
#define GPIO_DIR_MOD_BRD_TYPE_0 0
#define GPIO_DIR_MOD_BRD_TYPE_1 0
#define GPIO_DIR_INT_RTC_PHY_N 0
#define GPIO_DIR_MOD_BAY_PRESENT_N 0
#define GPIO_DIR_LED_CPU_MOD_1 1
#define GPIO_DIR_LED_CPU_MOD_2 1
#define GPIO_DIR_LED_CPU_MOD_3 1
#define GPIO_DIR_LED_CPU_MOD_4 1

#define GPIO_TYPE_DIR 0
#define GPIO_TYPE_DATA 1

#define GPIO_LED_MODE_OFF    0
#define GPIO_LED_MODE_ON     1
#define GPIO_LED_MODE_NORMAL 2
#endif

#ifdef CONFIG_RAMBOOT_PBL
#define CONFIG_SYS_FSL_PBL_PBI board/freescale/ls1046ardb/ls1046ardb_pbi.cfg
#endif

#ifdef CONFIG_SD_BOOT
#define CONFIG_SYS_FSL_PBL_PBI board/freescale/ls1046ardb/ls1046ardb_pbi.cfg
#ifdef CONFIG_EMMC_BOOT
#define CONFIG_SYS_FSL_PBL_RCW \
	board/freescale/ls1046ardb/ls1046ardb_rcw_emmc.cfg
#else
#define CONFIG_SYS_FSL_PBL_RCW board/freescale/ls1046ardb/ls1046ardb_rcw_sd.cfg
#endif
#elif defined(CONFIG_QSPI_BOOT)
#define CONFIG_SYS_FSL_PBL_RCW \
	board/freescale/ls1046ardb/ls1046ardb_rcw_qspi.cfg
#define CONFIG_SYS_FSL_PBL_PBI \
	board/freescale/ls1046ardb/ls1046ardb_qspi_pbi.cfg
#define CONFIG_SYS_UBOOT_BASE		0x40100000
#define CONFIG_SYS_SPL_ARGS_ADDR	0x90000000
#endif

#ifndef SPL_NO_IFC
/* IFC */
#define CONFIG_FSL_IFC
/*
 * NAND Flash Definitions
 */
#define CONFIG_NAND_FSL_IFC
#endif

#define CONFIG_SYS_NAND_BASE		0x7e800000
#define CONFIG_SYS_NAND_BASE_PHYS	CONFIG_SYS_NAND_BASE

#define CONFIG_SYS_NAND_CSPR_EXT	(0x0)
#define CONFIG_SYS_NAND_CSPR	(CSPR_PHYS_ADDR(CONFIG_SYS_NAND_BASE_PHYS) \
				| CSPR_PORT_SIZE_8	\
				| CSPR_MSEL_NAND	\
				| CSPR_V)
#define CONFIG_SYS_NAND_AMASK	IFC_AMASK(64 * 1024)
#define CONFIG_SYS_NAND_CSOR	(CSOR_NAND_ECC_ENC_EN	/* ECC on encode */ \
				| CSOR_NAND_ECC_DEC_EN	/* ECC on decode */ \
				| CSOR_NAND_ECC_MODE_8	/* 8-bit ECC */ \
				| CSOR_NAND_RAL_3	/* RAL = 3 Bytes */ \
				| CSOR_NAND_PGS_4K	/* Page Size = 4K */ \
				| CSOR_NAND_SPRZ_224	/* Spare size = 224 */ \
				| CSOR_NAND_PB(64))	/* 64 Pages Per Block */

#define CONFIG_SYS_NAND_ONFI_DETECTION

#define CONFIG_SYS_NAND_FTIM0		(FTIM0_NAND_TCCST(0x7) | \
					FTIM0_NAND_TWP(0x18)   | \
					FTIM0_NAND_TWCHT(0x7) | \
					FTIM0_NAND_TWH(0xa))
#define CONFIG_SYS_NAND_FTIM1		(FTIM1_NAND_TADLE(0x32) | \
					FTIM1_NAND_TWBE(0x39)  | \
					FTIM1_NAND_TRR(0xe)   | \
					FTIM1_NAND_TRP(0x18))
#define CONFIG_SYS_NAND_FTIM2		(FTIM2_NAND_TRAD(0xf) | \
					FTIM2_NAND_TREH(0xa) | \
					FTIM2_NAND_TWHRE(0x1e))
#define CONFIG_SYS_NAND_FTIM3		0x0

#define CONFIG_SYS_NAND_BASE_LIST	{ CONFIG_SYS_NAND_BASE }
#define CONFIG_SYS_MAX_NAND_DEVICE	1
#define CONFIG_MTD_NAND_VERIFY_WRITE

#define CONFIG_SYS_NAND_BLOCK_SIZE	(128 * 1024)

/*
 * CPLD
 */
#define CONFIG_SYS_CPLD_BASE		0x7fb00000
#define CPLD_BASE_PHYS			CONFIG_SYS_CPLD_BASE

#define CONFIG_SYS_CPLD_CSPR_EXT	(0x0)
#define CONFIG_SYS_CPLD_CSPR		(CSPR_PHYS_ADDR(CPLD_BASE_PHYS) | \
					CSPR_PORT_SIZE_8 | \
					CSPR_MSEL_GPCM | \
					CSPR_V)
#define CONFIG_SYS_CPLD_AMASK		IFC_AMASK(64 * 1024)
#define CONFIG_SYS_CPLD_CSOR		CSOR_NOR_ADM_SHIFT(16)

/* CPLD Timing parameters for IFC GPCM */
#define CONFIG_SYS_CPLD_FTIM0		(FTIM0_GPCM_TACSE(0x0e) | \
					FTIM0_GPCM_TEADC(0x0e) | \
					FTIM0_GPCM_TEAHC(0x0e))
#define CONFIG_SYS_CPLD_FTIM1		(FTIM1_GPCM_TACO(0xff) | \
					FTIM1_GPCM_TRAD(0x3f))
#define CONFIG_SYS_CPLD_FTIM2		(FTIM2_GPCM_TCS(0xf) | \
					FTIM2_GPCM_TCH(0xf) | \
					FTIM2_GPCM_TWP(0x3E))
#define CONFIG_SYS_CPLD_FTIM3		0x0

/* IFC Timing Params */
#define CONFIG_SYS_CSPR0_EXT		CONFIG_SYS_NAND_CSPR_EXT
#define CONFIG_SYS_CSPR0		CONFIG_SYS_NAND_CSPR
#define CONFIG_SYS_AMASK0		CONFIG_SYS_NAND_AMASK
#define CONFIG_SYS_CSOR0		CONFIG_SYS_NAND_CSOR
#define CONFIG_SYS_CS0_FTIM0		CONFIG_SYS_NAND_FTIM0
#define CONFIG_SYS_CS0_FTIM1		CONFIG_SYS_NAND_FTIM1
#define CONFIG_SYS_CS0_FTIM2		CONFIG_SYS_NAND_FTIM2
#define CONFIG_SYS_CS0_FTIM3		CONFIG_SYS_NAND_FTIM3

#define CONFIG_SYS_CSPR2_EXT		CONFIG_SYS_CPLD_CSPR_EXT
#define CONFIG_SYS_CSPR2		CONFIG_SYS_CPLD_CSPR
#define CONFIG_SYS_AMASK2		CONFIG_SYS_CPLD_AMASK
#define CONFIG_SYS_CSOR2		CONFIG_SYS_CPLD_CSOR
#define CONFIG_SYS_CS2_FTIM0		CONFIG_SYS_CPLD_FTIM0
#define CONFIG_SYS_CS2_FTIM1		CONFIG_SYS_CPLD_FTIM1
#define CONFIG_SYS_CS2_FTIM2		CONFIG_SYS_CPLD_FTIM2
#define CONFIG_SYS_CS2_FTIM3		CONFIG_SYS_CPLD_FTIM3

/* EEPROM */
#if 0
#define CONFIG_ID_EEPROM
#define CONFIG_SYS_I2C_EEPROM_NXID
#define CONFIG_SYS_EEPROM_BUS_NUM		3
#define CONFIG_SYS_I2C_EEPROM_ADDR		0xac
#define CONFIG_SYS_I2C_EEPROM_ADDR_LEN		1
#define CONFIG_SYS_EEPROM_PAGE_WRITE_BITS	3
#define CONFIG_SYS_EEPROM_PAGE_WRITE_DELAY_MS	5
#define I2C_RETIMER_ADDR			0x18
#endif

/* PMIC */
#define CONFIG_POWER
#ifdef CONFIG_POWER
#define CONFIG_POWER_I2C
#endif

/*
 * Environment
 */
#ifndef SPL_NO_ENV
#define CONFIG_ENV_OVERWRITE
#endif

#ifdef CONFIG_TFABOOT
#define CONFIG_SYS_MMC_ENV_DEV		0

#define CONFIG_ENV_SIZE			0x2000		/* 8KB */
#define CONFIG_ENV_OFFSET		0x500000	/* 5MB */
#define CONFIG_ENV_SECT_SIZE		0x40000		/* 256KB */
#else
#if defined(CONFIG_SD_BOOT)
#define CONFIG_SYS_MMC_ENV_DEV		0
#define CONFIG_ENV_OFFSET		(3 * 1024 * 1024)
#define CONFIG_ENV_SIZE			0x2000
#else
#if 0 /*Forest.Tsao.B*/
#define CONFIG_ENV_SIZE			0x2000		/* 8KB */
#define CONFIG_ENV_OFFSET		0x300000	/* 3MB */
#define CONFIG_ENV_SECT_SIZE		0x40000		/* 256KB */
#else
#define CONFIG_ENV_SIZE			0x2000		/* 8KB */
#define CONFIG_ENV_OFFSET		0x210000	/* 2MB  (approx) */
#define CONFIG_ENV_SECT_SIZE	0x10000	                /* 64KB */
#endif /*Forest.Tsao.E*/
#endif
#endif

#define AQR105_IRQ_MASK			0x80000000
/* FMan */
#ifndef SPL_NO_FMAN

#ifdef CONFIG_NET
#define CONFIG_PHY_REALTEK
/*Forest.Tsao.B*/
#define CONFIG_MV88E6190_SWITCH
#define CONFIG_MV88E6190_SWITCH_CPU_ATTACHED
#define CONFIG_ETHADDR		00:90:4c:06:a5:72
#define PORT_LED_MODE_NORMAL 0
#define PORT_LED_MODE_GREEN  1
#define PORT_LED_MODE_AMBER  2
#define PORT_LED_MODE_ALLON  3
#define PORT_LED_MODE_OFF    4
/*Forest.Tsao.E*/
#endif

#ifdef CONFIG_SYS_DPAA_FMAN
#define CONFIG_FMAN_ENET
#define CONFIG_PHY_AQUANTIA
#define CONFIG_PHYLIB_10G
#define RGMII_PHY1_ADDR			0x1
#define RGMII_PHY2_ADDR			0x2

#ifdef CONFIG_WG1008_VM1P
#define SGMII_PHY1_ADDR			0x9
#define SGMII_PHY2_ADDR			0xa
#else
#define SGMII_PHY1_ADDR			0x3
#define SGMII_PHY2_ADDR			0x4
#endif

#define FM1_10GEC1_PHY_ADDR		0x0

#define FDT_SEQ_MACADDR_FROM_ENV

#ifdef CONFIG_WG1008_VM1P
#define CONFIG_ETHPRIME			"FM1@DTSEC9"
#else
#define CONFIG_ETHPRIME			"FM1@DTSEC3"
#endif
#endif
#endif

/*  Marvell 88E6190 switch */
#define SMI_I 1
#define SMI_E (!SMI_I)



/* QSPI device */
#ifndef SPL_NO_QSPI
#ifdef CONFIG_FSL_QSPI
#define CONFIG_SPI_FLASH_SPANSION
/*Albert.Ke.B*/
#ifdef CONFIG_WG1008_VM1P

/*#define CONFIG_SPI_FLASH*/
#define CONFIG_SPI_FLASH_MACRONIX
#define CONFIG_SPI_FLASH_BAR
#define CONFIG_SF_DEFAULT_BUS		0
#define CONFIG_SF_DEFAULT_CS		0
#define CONFIG_SF_DEFAULT_SPEED		40000000
#define CONFIG_SF_DEFAULT_MODE		SPI_MODE_0
#define FSL_QSPI_FLASH_NUM		1
#define FSL_QSPI_FLASH_SIZE		SZ_4M
/*
#define QSPI0_BASE_ADDR			QSPI1_IPS_BASE_ADDR
#define QSPI0_AMBA_BASE			QSPI0_ARB_BASE_ADDR
*/
#else

#define FSL_QSPI_FLASH_SIZE		(1 << 26)
#define FSL_QSPI_FLASH_NUM		2

#endif
/*Albert.Ke.E*/
#endif
#endif

#ifndef SPL_NO_MISC
#undef CONFIG_BOOTCOMMAND
#ifdef CONFIG_TFABOOT
#define QSPI_NOR_BOOTCOMMAND "run distro_bootcmd; run qspi_bootcmd; "	\
			   "env exists secureboot && esbc_halt;;"
#define SD_BOOTCOMMAND "run distro_bootcmd;run sd_bootcmd; "	\
			   "env exists secureboot && esbc_halt;"
#else
#if defined(CONFIG_QSPI_BOOT)
#define CONFIG_BOOTCOMMAND "run distro_bootcmd; run qspi_bootcmd; "	\
			   "env exists secureboot && esbc_halt;;"
#elif defined(CONFIG_SD_BOOT)
#define CONFIG_BOOTCOMMAND "run distro_bootcmd;run sd_bootcmd; "	\
			   "env exists secureboot && esbc_halt;"
#endif
#endif
#endif

#ifdef CONFIG_WG1008_VM1P

#undef	CONFIG_EXTRA_ENV_SETTINGS
#define CONFIG_EXTRA_ENV_SETTINGS		\
	"hwconfig=fsl_ddr:bank_intlv=auto\0"	\
	"fdt_high=0xffffffffffffffff\0"		\
	"initrd_high=0xffffffffffffffff\0"	\
	"fdt_addr=0x64f00000\0"		 	\
	"kernel_addr=0x61000000\0"		\
	"scriptaddr=0x80000000\0"		\
	"scripthdraddr=0x80080000\0"		\
	"fdtheader_addr_r=0x80100000\0"		\
	"kernelheader_addr_r=0x80200000\0"	\
	"kernel_addr_r=0x81000000\0"		\
	"kernel_start=0x1000000\0"		\
	"kernelheader_start=0x800000\0"		\
	"fdt_addr_r=0x90000000\0"		\
	"load_addr=0xa0000000\0"		\
	"kernelheader_addr=0x60800000\0"	\
	"kernel_size=0x2800000\0"		\
	"kernelheader_size=0x40000\0"		\
	"kernel_addr_sd=0x8000\0"		\
	"kernel_size_sd=0x14000\0"		\
	"kernelhdr_addr_sd=0x4000\0"		\
	"kernelhdr_size_sd=0x10\0"		\
	"boot_os=y\0"				\
	"mtdparts=" CONFIG_MTDPARTS_DEFAULT "\0"	\
	BOOTENV					\
	"boot_scripts=ls1046ardb_boot.scr\0"	\
	"boot_script_hdr=hdr_ls1046ardb_bs.out\0"	\
	"scan_dev_for_boot_part="               \
		"part list ${devtype} ${devnum} devplist; "   \
		"env exists devplist || setenv devplist 1; "  \
		"for distro_bootpart in ${devplist}; do "     \
		  "if fstype ${devtype} "                  \
			"${devnum}:${distro_bootpart} "      \
			"bootfstype; then "                  \
			"run scan_dev_for_boot; "            \
		  "fi; "                                   \
		"done\0"                                   \
	"scan_dev_for_boot="				  \
		"echo Scanning ${devtype} "		  \
				"${devnum}:${distro_bootpart}...; "  \
		"for prefix in ${boot_prefixes}; do "	  \
			"run scan_dev_for_scripts; "	  \
		"done;"					  \
		"\0"					  \
	"consoledev=ttyS0\0"					\
	"loadaddr=82000000\0"					\
	"WGKernelfile=kernel_T80.itb\0"                \
	"SysARoot=sda3\0"	\
	"SysBRoot=sda2\0"	\
	"wgBootSysA=setenv bootargs root=/dev/$SysARoot rw " \
	"console=$consoledev,$baudrate $othbootargs "	\
	"earlycon=uart8250,mmio,0x21c0500; "	\
	"scsi scan; " \
	"ext2load scsi 0:3 $loadaddr $WGKernelfile; " \
	"bootm $loadaddr;\0"             \
	"wgBootSysB=setenv bootargs root=/dev/$SysBRoot rw " \
	"console=$consoledev,$baudrate $othbootargs "	\
	"earlycon=uart8250,mmio,0x21c0500; "	\
	"scsi scan; " \
	"ext2load scsi 0:2 $loadaddr $WGKernelfile; " \
	"bootm $loadaddr;\0"             \
	"wgBootRecovery=setenv bootargs wgmode=safe root=/dev/$SysARoot rw " \
	"console=$consoledev,$baudrate $othbootargs "	\
	"earlycon=uart8250,mmio,0x21c0500; "	\
	"scsi scan; " \
	"ext2load scsi 0:3 $loadaddr $WGKernelfile; " \
	"bootm $loadaddr;\0"  \
	"nuke_env=sf probe 0;sf erase 0x210000 10000;\0"  \
	"ipaddr=10.0.1.1;\0" \
	"serveraddr=10.0.1.13;\0" \
	"netmask=255.255.255.0;\0" \
	"flash_bootloader=tftp 0x82000000 u-boot_ls1046ardb_wg_t80_qspi.bin;" \
	"sf probe 0; sf erase 0x100000 +$filesize; sf write 0x82000000 0x100000 $filesize;" \
	"sf read 0x80000000 0 $filesize; cmp.b 0x82000000 0x80000000 $filesize;\0"

#undef CONFIG_BOOTCOMMAND
#define CONFIG_BOOTCOMMAND		"run wgBootSysA"

#endif /* CONFIG_WG1008_VM1P */

#include <asm/fsl_secure_boot.h>

#endif /* __LS1046ARDB_T80_H__ */
