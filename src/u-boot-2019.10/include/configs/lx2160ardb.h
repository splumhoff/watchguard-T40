/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2018,2020 NXP
 */

#ifndef __LX2_RDB_H
#define __LX2_RDB_H

#include "lx2160a_common.h"

/* Qixis */
#define QIXIS_XMAP_MASK			0x07
#define QIXIS_XMAP_SHIFT		5
#define QIXIS_RST_CTL_RESET_EN		0x30
#define QIXIS_LBMAP_DFLTBANK		0x00
#define QIXIS_LBMAP_ALTBANK		0x20
#define QIXIS_LBMAP_QSPI		0x00
#define QIXIS_RCW_SRC_QSPI		0xff
#define QIXIS_RST_CTL_RESET		0x31
#define QIXIS_RCFG_CTL_RECONFIG_IDLE	0x20
#define QIXIS_RCFG_CTL_RECONFIG_START	0x21
#define QIXIS_RCFG_CTL_WATCHDOG_ENBLE	0x08
#define QIXIS_LBMAP_MASK		0x0f
#define QIXIS_LBMAP_SD
#define QIXIS_LBMAP_EMMC
#define QIXIS_RCW_SRC_SD           0x08
#define QIXIS_RCW_SRC_EMMC         0x09
#define NON_EXTENDED_DUTCFG

/* VID */

#define I2C_MUX_CH_VOL_MONITOR		0xA
/* Voltage monitor on channel 2*/
#define I2C_VOL_MONITOR_ADDR		0x63
#define I2C_VOL_MONITOR_BUS_V_OFFSET	0x2
#define I2C_VOL_MONITOR_BUS_V_OVF	0x1
#define I2C_VOL_MONITOR_BUS_V_SHIFT	3
#define CONFIG_VID_FLS_ENV		"lx2160ardb_vdd_mv"
#define CONFIG_VID

/* The lowest and highest voltage allowed*/
#define VDD_MV_MIN			775
#define VDD_MV_MAX			855

/* PM Bus commands code for LTC3882*/
#define PMBUS_CMD_PAGE                  0x0
#define PMBUS_CMD_READ_VOUT             0x8B
#define PMBUS_CMD_PAGE_PLUS_WRITE       0x05
#define PMBUS_CMD_VOUT_COMMAND          0x21
#define PWM_CHANNEL0                    0x0

#define CONFIG_VOL_MONITOR_LTC3882_SET
#define CONFIG_VOL_MONITOR_LTC3882_READ

/* RTC */
#define CONFIG_SYS_RTC_BUS_NUM		4

/* MAC/PHY configuration */
#if defined(CONFIG_FSL_MC_ENET)
#define CONFIG_MII
#define CONFIG_ETHPRIME		"DPMAC3@xgmii"

#define AQR107_PHY_ADDR1	0x04
#define AQR107_PHY_ADDR2	0x05
#define AQR107_IRQ_MASK		0x0C
#define AQR113C_PHY_ADDR1	0x00
#define AQR113C_PHY_ADDR2	0x08
#define USXGMII_PHY_ADDR10	0x0A

#define CORTINA_NO_FW_UPLOAD
#define CORTINA_PHY_ADDR1	0x0
#define INPHI_PHY_ADDR1		0x0
#ifdef CONFIG_SD_BOOT
#define IN112525_FW_ADDR        0x980000
#else
#define IN112525_FW_ADDR        0x20980000
#endif
#define IN112525_FW_LENGTH      0x40000

#define RGMII_PHY_ADDR1		0x01
#define RGMII_PHY_ADDR2		0x02

#define CONFIG_MV88E6191X_SWITCH
#define CONFIG_MV88E6191X_SWITCH_CPU_ATTACHED
#define SMI_I 1
#define SMI_E (!SMI_I)
#endif

/* EMC2305 */
#define I2C_MUX_CH_EMC2305		0x09
#define I2C_EMC2305_ADDR		0x4D
#define I2C_EMC2305_CMD		0x40
#define I2C_EMC2305_PWM		0x80

/* EEPROM */
#if 0
#define CONFIG_ID_EEPROM
#define CONFIG_SYS_I2C_EEPROM_NXID
#define CONFIG_SYS_EEPROM_BUS_NUM	           0
#define CONFIG_SYS_I2C_EEPROM_ADDR	           0x57
#define CONFIG_SYS_I2C_EEPROM_ADDR_LEN	    1
#define CONFIG_SYS_EEPROM_PAGE_WRITE_BITS     3
#define CONFIG_SYS_EEPROM_PAGE_WRITE_DELAY_MS 5
#endif

/* GPIO */
#define LX2160A_GPIO1_DIR    0x2300000
#define LX2160A_GPIO1_GPIBE  0x2300018
#define LX2160A_GPIO1_DATA   0x2300008
#define LX2160A_GPIO2_DIR    0x2310000
#define LX2160A_GPIO2_GPIBE  0x2310018
#define LX2160A_GPIO2_DATA   0x2310008
#define LX2160A_GPIO3_DIR    0x2320000
#define LX2160A_GPIO3_GPIBE  0x2320018
#define LX2160A_GPIO3_DATA   0x2320008
#define LX2160A_GPIO4_DIR    0x2330000
#define LX2160A_GPIO4_GPIBE  0x2330018
#define LX2160A_GPIO4_DATA   0x2330008

#define GPIO_BUS_CFG_ENF_USE2 1
#define GPIO_BUS_IIC6_SDA 1
#define GPIO_BUS_IIC6_SCL 1
#define GPIO_BUS_IIC5_SDA 1
#define GPIO_BUS_IIC5_SCL 1
#define GPIO_BUS_INT_CPLD_CPU_1V8 1
#define GPIO_BUS_RST_CPU_N 1
#define GPIO_BUS_RST_CPU_TPM_N 1
#define GPIO_BUS_INT_TPM_PIRQ_N 1
#define GPIO_BUS_DDR4_CH1_SPD_EVENT_1V8 1
#define GPIO_BUS_CFG_RCW_SRC2 2
#define GPIO_BUS_CFG_RCW_SRC3 2
#define GPIO_BUS_RESET_REQ_B 2
#define GPIO_BUS_INT_ALL_ALM_7904D_CPU_1V8_N 2
#define GPIO_BUS_NCT_3961S_FAULT_CPU_2_LF 2
#define GPIO_BUS_NCT_3961S_FAULT_CPU_1_LF 2
#define GPIO_BUS_EN_CPU_NCT3961S_2_LF 2
#define GPIO_BUS_EN_CPU_NCT3961S_1_LF 2
#define GPIO_BUS_NCT_3961S_MODE_2_LF 2
#define GPIO_BUS_NCT_3961S_MODE_1_LF 2
#define GPIO_BUS_NCT_3961S_FTI_2_LF 2
#define GPIO_BUS_NCT_3961S_FTI_1_LF 2
#define GPIO_BUS_ASLEEP 2
#define GPIO_BUS_BOOT_STRAP 2
#define GPIO_BUS_USB1_DRVVBUS 4
#define GPIO_BUS_USB1_PWRFAULT 4
#define GPIO_BUS_USB2_DRVVBUS 4
#define GPIO_BUS_USB2_PWRFAULT 4
#define GPIO_BUS_FRONT_PORT_LED 4

#define GPIO_SHIFT_CFG_ENF_USE2 5
#define GPIO_SHIFT_IIC6_SDA 22
#define GPIO_SHIFT_IIC6_SCL 23
#define GPIO_SHIFT_IIC5_SDA 24
#define GPIO_SHIFT_IIC5_SCL 25
#define GPIO_SHIFT_INT_CPLD_CPU_1V8 16
#define GPIO_SHIFT_RST_CPU_N 17
#define GPIO_SHIFT_RST_CPU_TPM_N 18
#define GPIO_SHIFT_INT_TPM_PIRQ_N 19
#define GPIO_SHIFT_DDR4_CH1_SPD_EVENT_1V8 20
#define GPIO_SHIFT_CFG_RCW_SRC2 6
#define GPIO_SHIFT_CFG_RCW_SRC3 7
#define GPIO_SHIFT_RESET_REQ_B 8
#define GPIO_SHIFT_INT_ALL_ALM_7904D_CPU_1V8_N 9
#define GPIO_SHIFT_NCT_3961S_FAULT_CPU_2_LF 11
#define GPIO_SHIFT_NCT_3961S_FAULT_CPU_1_LF 12
#define GPIO_SHIFT_EN_CPU_NCT3961S_2_LF 13
#define GPIO_SHIFT_EN_CPU_NCT3961S_1_LF 14
#define GPIO_SHIFT_NCT_3961S_MODE_2_LF 15
#define GPIO_SHIFT_NCT_3961S_MODE_1_LF 16
#define GPIO_SHIFT_NCT_3961S_FTI_2_LF 17
#define GPIO_SHIFT_NCT_3961S_FTI_1_LF 18
#define GPIO_SHIFT_ASLEEP 6
#define GPIO_SHIFT_BOOT_STRAP 7
#define GPIO_SHIFT_USB1_DRVVBUS 25
#define GPIO_SHIFT_USB1_PWRFAULT 26
#define GPIO_SHIFT_USB2_DRVVBUS 27
#define GPIO_SHIFT_USB2_PWRFAULT 28
#define GPIO_SHIFT_GPIO_DATA0 0
#define GPIO_SHIFT_GPIO_DATA1 1
#define GPIO_SHIFT_GPIO_DATA2 2
#define GPIO_SHIFT_GPIO_DATA3 3
#define GPIO_SHIFT_GPIO_DATA5 5
#define GPIO_SHIFT_GPIO_DATA6 6
#define GPIO_SHIFT_GPIO_DATA7 7
#define GPIO_SHIFT_GPIO_DATA8 8
#define GPIO_SHIFT_GPIO_DATA9 9
#define GPIO_SHIFT_GPIO_DATA10 10
#define GPIO_SHIFT_GPIO_DATA11 11
#define GPIO_SHIFT_GPIO_DATA12 12
#define GPIO_SHIFT_GPIO_DATA13 13
#define GPIO_SHIFT_GPIO_DATA14 14
#define GPIO_SHIFT_GPIO_DATA15 15
#define GPIO_SHIFT_GPIO_DATA17 17

	/* GPIO Direction, 1:output; 0:input */
#define GPIO_DIR_CFG_ENF_USE2 0
#define GPIO_DIR_IIC6_SDA 1
#define GPIO_DIR_IIC6_SCL 1
#define GPIO_DIR_IIC5_SDA 1
#define GPIO_DIR_IIC5_SCL 1
#define GPIO_DIR_INT_CPLD_CPU_1V8 0
#define GPIO_DIR_RST_CPU_N 0
#define GPIO_DIR_RST_CPU_TPM_N 1
#define GPIO_DIR_INT_TPM_PIRQ_N 0
#define GPIO_DIR_DDR4_CH1_SPD_EVENT_1V8 0
#define GPIO_DIR_CFG_RCW_SRC2 0
#define GPIO_DIR_CFG_RCW_SRC3 0
#define GPIO_DIR_RESET_REQ_B 0
#define GPIO_DIR_INT_ALL_ALM_7904D_CPU_1V8_N 0
#define GPIO_DIR_NCT_3961S_FAULT_CPU_2_LF 0
#define GPIO_DIR_NCT_3961S_FAULT_CPU_1_LF 0
#define GPIO_DIR_EN_CPU_NCT3961S_2_LF 1
#define GPIO_DIR_EN_CPU_NCT3961S_1_LF 1
#define GPIO_DIR_NCT_3961S_MODE_2_LF 1
#define GPIO_DIR_NCT_3961S_MODE_1_LF 1
#define GPIO_DIR_NCT_3961S_FTI_2_LF 0
#define GPIO_DIR_NCT_3961S_FTI_1_LF 0
#define GPIO_DIR_ASLEEP 0
#define GPIO_DIR_BOOT_STRAP 0
#define GPIO_DIR_USB1_DRVVBUS 1
#define GPIO_DIR_USB1_PWRFAULT 0
#define GPIO_DIR_USB2_DRVVBUS 1
#define GPIO_DIR_USB2_PWRFAULT 0
#define GPIO_DIR_FRONT_PORT_LED 1

#define GPIO_TYPE_DIR 0
#define GPIO_TYPE_DATA 1
#define GPIO_TYPE_GPIBE 2

#define GPIO_LED_MODE_OFF    0
#define GPIO_LED_MODE_ON     1
#define GPIO_LED_MODE_NORMAL 2


/* Initial environment variables */
#define CONFIG_EXTRA_ENV_SETTINGS		\
	EXTRA_ENV_SETTINGS			\
	"boot_scripts=lx2160ardb_boot.scr\0"	\
	"boot_script_hdr=hdr_lx2160ardb_bs.out\0"	\
	"BOARD=lx2160ardb\0"			\
	"xspi_bootcmd=echo Trying load from flexspi..;"		\
		"sf probe 0:0 && sf read $load_addr "		\
		"$kernel_start $kernel_size ; env exists secureboot &&"	\
		"sf read $kernelheader_addr_r $kernelheader_start "	\
		"$kernelheader_size && esbc_validate ${kernelheader_addr_r}; "\
		" bootm $load_addr#$BOARD\0"			\
	"sd_bootcmd=echo Trying load from sd card..;"		\
		"mmcinfo; mmc read $load_addr "			\
		"$kernel_addr_sd $kernel_size_sd ;"		\
		"env exists secureboot && mmc read $kernelheader_addr_r "\
		"$kernelhdr_addr_sd $kernelhdr_size_sd "	\
		" && esbc_validate ${kernelheader_addr_r};"	\
		"bootm $load_addr#$BOARD\0"			\
	"emmc_bootcmd=echo Trying load from emmc card..;"	\
		"mmc dev 1; mmcinfo; mmc read $load_addr "	\
		"$kernel_addr_sd $kernel_size_sd ;"		\
		"env exists secureboot && mmc read $kernelheader_addr_r "\
		"$kernelhdr_addr_sd $kernelhdr_size_sd "	\
		" && esbc_validate ${kernelheader_addr_r};"	\
		"bootm $load_addr#$BOARD\0"			\
	"bootcmd_nvme0=sf read 0x80600000 0x600000 0x100000; "	\
			"env exists mcinitcmd && "			\
			"fsl_mc lazyapply dpl 0x80600000; "		\
			"setenv devnum 0; nvme scan; run nvme_boot\0"	\
	"bootcmd_usb0=devnum=0; run usb_boot; devnum=1; run usb_boot\0"		\
	"nvme_boot=if nvme dev ${devnum}; then setenv devtype nvme;"	\
		"run scan_dev_for_boot_part; fi\0"	\
	"consoledev=ttyAMA0\0"					\
	"WGKernelfile=kernel_M590_M690.itb\0"                                \
	"SysARoot=nvme0n1p3\0"	\
	"SysBRoot=nvme0n1p2\0"	\
	"wgBootODMOS=run bootcmd_nvme0;\0" \
	"wgBootSysA=nvme scan;sf probe 0:0;sf read 0x80600000 0x600000 0x100000; fsl_mc lazyapply dpl 0x80600000;" \
	"setenv bootargs root=/dev/$SysARoot rw " \
	"console=$consoledev,$baudrate $othbootargs "	\
	"earlycon=pl011,mmio32,0x21c0000 pci=pcie_bus_perf; "	\
	"ext2load nvme 0:3 0x80000000 dsa_ptoto_v2.spb; " \
	"fsl_mc apply spb 0x80000000; " \
	"ext2load nvme 0:3 $load_addr $WGKernelfile; " \
	"bootm $load_addr;\0"             \
	"wgBootSysB=nvme scan;sf probe 0:0;sf read 0x80600000 0x600000 0x100000; fsl_mc lazyapply dpl 0x80600000;" \
	"setenv bootargs root=/dev/$SysBRoot rw " \
	"console=$consoledev,$baudrate $othbootargs "	\
	"earlycon=pl011,mmio32,0x21c0000 pci=pcie_bus_perf; "	\
	"ext2load nvme 0:2 $load_addr $WGKernelfile; " \
	"bootm $load_addr;\0"             \
	"nuke_env=sf probe 0;sf erase 0x200000 0x10000;\0" \
	"ipaddr=10.0.1.1\0"     \
	"serverip=10.0.1.13\0"     \
	"gatewayip=10.0.1.13\0"     \
	"netmask=255.255.255.0\0"     \
	"boot_targets=usb0 nvme0\0" \
	"m590_flash_wg_bootloader=tftp 0xa0000000 u-boot_ls2088ardb_m590_complete.bin;" \
	"&& sf probe 0; sf erase 0x0 0x210000; sf write 0xa0000000 0x0 $filesize; sf read 0x80000000 0 $filesize; cmp.b 0xa0000000 0x80000000 $filesize;\0" \
	"m690_flash_wg_bootloader=tftp 0xa0000000 u-boot_ls2088ardb_m690_complete.bin;" \
	"&& sf probe 0; sf erase 0x0 0x210000; sf write 0xa0000000 0x0 $filesize; sf read 0x80000000 0 $filesize; cmp.b 0xa0000000 0x80000000 $filesize;\0"

#include <asm/fsl_secure_boot.h>

#endif /* __LX2_RDB_H */
