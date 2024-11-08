// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2015 Freescale Semiconductor
 * Copyright 2017 NXP
 */
#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <netdev.h>
#include <fsl_ifc.h>
#include <fsl_ddr.h>
#include <asm/io.h>
#include <hwconfig.h>
#include <fdt_support.h>
#include <linux/libfdt.h>
#include <fsl-mc/fsl_mc.h>
#include <environment.h>
#include <efi_loader.h>
#include <i2c.h>
#include <asm/arch/mmu.h>
#include <asm/arch/soc.h>
#include <asm/arch/ppa.h>
#include <fsl_sec.h>

#ifdef CONFIG_FSL_QIXIS
#include "../common/qixis.h"
#include "ls2080ardb_qixis.h"
#endif
#include "../common/vid.h"

#define PIN_MUX_SEL_SDHC	0x00
#define PIN_MUX_SEL_DSPI	0x0a

#define SET_SDHC_MUX_SEL(reg, value)	((reg & 0xf0) | value)
DECLARE_GLOBAL_DATA_PTR;

enum {
	MUX_TYPE_SDHC,
	MUX_TYPE_DSPI,
};

unsigned long long get_qixis_addr(void)
{
	unsigned long long addr;

	if (gd->flags & GD_FLG_RELOC)
		addr = QIXIS_BASE_PHYS;
	else
		addr = QIXIS_BASE_PHYS_EARLY;

	/*
	 * IFC address under 256MB is mapped to 0x30000000, any address above
	 * is mapped to 0x5_10000000 up to 4GB.
	 */
	addr = addr  > 0x10000000 ? addr + 0x500000000ULL : addr + 0x30000000;

	return addr;
}

int checkboard(void)
{
#ifdef CONFIG_FSL_QIXIS
	u8 sw;
#endif
	char buf[15];

	cpu_name(buf);
	printf("Board: %s-RDB, ", buf);

#ifdef CONFIG_TARGET_LS2081ARDB
#ifdef CONFIG_FSL_QIXIS
	sw = QIXIS_READ(arch);
	printf("Board version: %c, ", (sw & 0xf) + 'A');

	sw = QIXIS_READ(brdcfg[0]);
	sw = (sw >> QIXIS_QMAP_SHIFT) & QIXIS_QMAP_MASK;
	switch (sw) {
	case 0:
		puts("boot from QSPI DEV#0\n");
		puts("QSPI_CSA_1 mapped to QSPI DEV#1\n");
		break;
	case 1:
		puts("boot from QSPI DEV#1\n");
		puts("QSPI_CSA_1 mapped to QSPI DEV#0\n");
		break;
	case 2:
		puts("boot from QSPI EMU\n");
		puts("QSPI_CSA_1 mapped to QSPI DEV#0\n");
		break;
	case 3:
		puts("boot from QSPI EMU\n");
		puts("QSPI_CSA_1 mapped to QSPI DEV#1\n");
		break;
	case 4:
		puts("boot from QSPI DEV#0\n");
		puts("QSPI_CSA_1 mapped to QSPI EMU\n");
		break;
	default:
		printf("invalid setting of SW%u\n", sw);
		break;
	}
	printf("FPGA: v%d.%d\n", QIXIS_READ(scver), QIXIS_READ(tagdata));
#endif
	puts("SERDES1 Reference : ");
	printf("Clock1 = 100MHz ");
	printf("Clock2 = 161.13MHz");
#else
#ifdef CONFIG_FSL_QIXIS
	sw = QIXIS_READ(arch);
	printf("Board Arch: V%d, ", sw >> 4);
	printf("Board version: %c, boot from ", (sw & 0xf) + 'A');

	sw = QIXIS_READ(brdcfg[0]);
	sw = (sw & QIXIS_LBMAP_MASK) >> QIXIS_LBMAP_SHIFT;

	if (sw < 0x8)
		printf("vBank: %d\n", sw);
	else if (sw == 0x9)
		puts("NAND\n");
	else
		//printf("invalid setting of SW%u\n", QIXIS_LBMAP_SWITCH);
		printf("vBank: 0\n");

	//printf("FPGA: v%d.%d\n", QIXIS_READ(scver), QIXIS_READ(tagdata));
#endif
	puts("SERDES1 Reference : ");
	printf("Clock1 = 100MHz ");
	printf("Clock2 = 156.25MHz");
#endif

	puts("\nSERDES2 Reference : ");
	printf("Clock1 = 100MHz ");
	printf("Clock2 = 100MHz\n");

	return 0;
}

unsigned long get_board_sys_clk(void)
{
#ifdef CONFIG_FSL_QIXIS
	u8 sysclk_conf = QIXIS_READ(brdcfg[1]);

	switch (sysclk_conf & 0x0F) {
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
#endif
	return 100000000;
}

int select_i2c_ch_pca9547(u8 ch)
{
	int ret;

#ifndef CONFIG_DM_I2C
#ifdef CONFIG_WG1008_PX2
	ret = i2c_write(I2C_MUX_PCA_ADDR_PRI, 3, 1, &ch, 1);
#else 
	ret = i2c_write(I2C_MUX_PCA_ADDR_PRI, 0, 1, &ch, 1);
#endif
#else
	struct udevice *dev;

	ret = i2c_get_chip_for_busnum(0, I2C_MUX_PCA_ADDR_PRI, 1, &dev);
	if (!ret)
		ret = dm_i2c_write(dev, 0, &ch, 1);
#endif

	if (ret) {
		puts("PCA: failed to select proper channel\n");
		return ret;
	}

	return 0;
}

int i2c_multiplexer_select_vid_channel(u8 channel)
{
	return select_i2c_ch_pca9547(channel);
}

int config_board_mux(int ctrl_type)
{
#ifdef CONFIG_FSL_QIXIS
	u8 reg5;

	reg5 = QIXIS_READ(brdcfg[5]);

	switch (ctrl_type) {
	case MUX_TYPE_SDHC:
		reg5 = SET_SDHC_MUX_SEL(reg5, PIN_MUX_SEL_SDHC);
		break;
	case MUX_TYPE_DSPI:
		reg5 = SET_SDHC_MUX_SEL(reg5, PIN_MUX_SEL_DSPI);
		break;
	default:
		printf("Wrong mux interface type\n");
		return -1;
	}

	QIXIS_WRITE(brdcfg[5], reg5);
#endif
	return 0;
}

int board_init(void)
{
#ifdef CONFIG_FSL_MC_ENET
	u32 __iomem *irq_ccsr = (u32 __iomem *)ISC_BASE;
#endif

	init_final_memctl_regs();

#ifdef CONFIG_ENV_IS_NOWHERE
	gd->env_addr = (ulong)&default_environment[0];
#endif
	select_i2c_ch_pca9547(I2C_MUX_CH_DEFAULT);

#ifdef CONFIG_FSL_QIXIS
	QIXIS_WRITE(rst_ctl, QIXIS_RST_CTL_RESET_EN);
#endif

#ifdef CONFIG_FSL_CAAM
	sec_init();
#endif
#ifdef CONFIG_FSL_LS_PPA
	ppa_init();
#endif

#ifdef CONFIG_FSL_MC_ENET
	/* invert AQR405 IRQ pins polarity */
	out_le32(irq_ccsr + IRQCR_OFFSET / 4, AQR405_IRQ_MASK);
#endif
#ifdef CONFIG_FSL_CAAM
	sec_init();
#endif

	return 0;
}

int board_early_init_f(void)
{
#ifdef CONFIG_SYS_I2C_EARLY_INIT
	i2c_early_init_f();
#endif
	fsl_lsch3_early_init_f();
	return 0;
}

#ifdef CONFIG_WG1008_PX2
//#define DBG_GPIO
//#define ENDIAN_SWAP
//#define BIT_SWAP
#define ENDIAN_SWAP_AND_BIT_SWAP

GPIO_t wg1008_px2_gpio[] =
{
    {GPIO_BUS_RST_CPU_N                	,GPIO_SHIFT_RST_CPU_N               	,GPIO_DIR_RST_CPU_N                	,1},
    {GPIO_BUS_Module_Bay_Present_N    	,GPIO_SHIFT_Module_Bay_Present_N		,GPIO_DIR_Module_Bay_Present_N     	,1},
    {GPIO_BUS_RST_CPU_TPM_N         	,GPIO_SHIFT_RST_CPU_TPM_N           	,GPIO_DIR_RST_CPU_TPM_N           	,1},
    {GPIO_BUS_EN_CPU_DRV_LED_1V8_N      ,GPIO_SHIFT_EN_CPU_DRV_LED_1V8_N    	,GPIO_DIR_EN_CPU_DRV_LED_1V8_N      ,0},
    {GPIO_BUS_M2_CLKREQ_CPU_1V8_N       ,GPIO_SHIFT_M2_CLKREQ_CPU_1V8_N     	,GPIO_DIR_M2_CLKREQ_CPU_1V8_N       ,1},
    {GPIO_BUS_RST_CPU_PoE_N             ,GPIO_SHIFT_RST_CPU_PoE_N           	,GPIO_DIR_RST_CPU_PoE_N             ,1},
    {GPIO_BUS_EN_LAN_Port_LED_1V8_N     ,GPIO_SHIFT_EN_LAN_Port_LED_1V8_N   	,GPIO_DIR_EN_LAN_Port_LED_1V8_N     ,0},
    {GPIO_BUS_INT_ALL_TCA9555_N    		,GPIO_SHIFT_INT_ALL_TCA9555_N    		,GPIO_DIR_INT_ALL_TCA9555_N    		,1},
    {GPIO_BUS_EN_GPIO_BUFFER_N          ,GPIO_SHIFT_EN_GPIO_BUFFER_N        	,GPIO_DIR_EN_GPIO_BUFFER_N          ,0},
    {GPIO_BUS_PCIe_Module_WAKE_BUF_N    ,GPIO_SHIFT_PCIe_Module_WAKE_BUF_N  	,GPIO_DIR_PCIe_Module_WAKE_BUF_N    ,1},
    {GPIO_BUS_EN_CPU_NCT3961S_1V8_1     ,GPIO_SHIFT_EN_CPU_NCT3961S_1V8_1 		,GPIO_DIR_EN_CPU_NCT3961S_1V8_1 	,1},
    {GPIO_BUS_EN_CPU_NCT3961S_1V8_2     ,GPIO_SHIFT_EN_CPU_NCT3961S_1V8_2   	,GPIO_DIR_EN_CPU_NCT3961S_1V8_2     ,1},
    {GPIO_BUS_EN_CPU_NCT3961S_1V8_3     ,GPIO_SHIFT_EN_CPU_NCT3961S_1V8_3   	,GPIO_DIR_EN_CPU_NCT3961S_1V8_3     ,1},
    {GPIO_BUS_NCT_3961S_MODE_1V8_1      ,GPIO_SHIFT_NCT_3961S_MODE_1V8_1    	,GPIO_DIR_NCT_3961S_MODE_1V8_1      ,1},
    {GPIO_BUS_NCT_3961S_MODE_1V8_2      ,GPIO_SHIFT_NCT_3961S_MODE_1V8_2    	,GPIO_DIR_NCT_3961S_MODE_1V8_2      ,1},
    {GPIO_BUS_NCT_3961S_MODE_1V8_3      ,GPIO_SHIFT_NCT_3961S_MODE_1V8_3    	,GPIO_DIR_NCT_3961S_MODE_1V8_3      ,1},
    {GPIO_BUS_NCT_3961S_FTI_ALL_1V8_1   ,GPIO_SHIFT_NCT_3961S_FTI_ALL_1V8_1 	,GPIO_DIR_NCT_3961S_FTI_ALL_1V8_1   ,1},
    {GPIO_BUS_RST_Module_PERST_N        ,GPIO_SHIFT_RST_Module_PERST_N      	,GPIO_DIR_RST_Module_PERST_N        ,1},
    {GPIO_BUS_INT_ALL_IRQ_N           	,GPIO_SHIFT_INT_ALL_IRQ_N           	,GPIO_DIR_INT_ALL_IRQ_N           	,1},
	{GPIO_BUS_Module_Type_CONN_CPU_0    ,GPIO_SHIFT_Module_Type_CONN_CPU_0  	,GPIO_DIR_Module_Type_CONN_CPU_0    ,1},
	{GPIO_BUS_Module_Type_CONN_CPU_1    ,GPIO_SHIFT_Module_Type_CONN_CPU_1  	,GPIO_DIR_Module_Type_CONN_CPU_1   	,1},
#if 0
	{GPIO_BUS_POWER_LED           		,GPIO_SHIFT_POWER_LED           		,GPIO_DIR_POWER_LED           		,0},
	{GPIO_BUS_STATUS_GREEN_LED          ,GPIO_SHIFT_STATUS_GREEN_LED        	,GPIO_DIR_STATUS_GREEN_LED          ,0},
	{GPIO_BUS_STATUS_RED_LED           	,GPIO_SHIFT_STATUS_RED_LED          	,GPIO_DIR_STATUS_RED_LED           	,1},
	{GPIO_BUS_STORAGE_LED           	,GPIO_SHIFT_STORAGE_LED           		,GPIO_DIR_STORAGE_LED           	,0},
#endif
	{GPIO_BUS_PORT1_AMBER_LED    		,GPIO_SHIFT_PORT1_AMBER_LED  			,GPIO_DIR_PORT1_AMBER_LED    		,1},
    {GPIO_BUS_PORT1_GREEN_LED    		,GPIO_SHIFT_PORT1_GREEN_LED  			,GPIO_DIR_PORT1_GREEN_LED    		,1},
    {GPIO_BUS_PORT2_AMBER_LED    		,GPIO_SHIFT_PORT2_AMBER_LED  			,GPIO_DIR_PORT2_AMBER_LED    		,1},
    {GPIO_BUS_PORT2_GREEN_LED    		,GPIO_SHIFT_PORT2_GREEN_LED  			,GPIO_DIR_PORT2_GREEN_LED    		,1},
	{GPIO_BUS_PORT3_AMBER_LED    		,GPIO_SHIFT_PORT3_AMBER_LED  			,GPIO_DIR_PORT3_AMBER_LED    		,1},
    {GPIO_BUS_PORT3_GREEN_LED    		,GPIO_SHIFT_PORT3_GREEN_LED  			,GPIO_DIR_PORT3_GREEN_LED    		,1},
	{GPIO_BUS_PORT4_AMBER_LED    		,GPIO_SHIFT_PORT4_AMBER_LED  			,GPIO_DIR_PORT4_AMBER_LED    		,1},
    {GPIO_BUS_PORT4_GREEN_LED    		,GPIO_SHIFT_PORT4_GREEN_LED  			,GPIO_DIR_PORT4_GREEN_LED    		,1},
	{GPIO_BUS_PORT5_AMBER_LED    		,GPIO_SHIFT_PORT5_AMBER_LED  			,GPIO_DIR_PORT5_AMBER_LED    		,1},
    {GPIO_BUS_PORT5_GREEN_LED    		,GPIO_SHIFT_PORT5_GREEN_LED  			,GPIO_DIR_PORT5_GREEN_LED    		,1},
	{GPIO_BUS_PORT6_AMBER_LED    		,GPIO_SHIFT_PORT6_AMBER_LED  			,GPIO_DIR_PORT6_AMBER_LED    		,1},
    {GPIO_BUS_PORT6_GREEN_LED    		,GPIO_SHIFT_PORT6_GREEN_LED  			,GPIO_DIR_PORT6_GREEN_LED    		,1},
	{GPIO_BUS_PORT7_AMBER_LED    		,GPIO_SHIFT_PORT7_AMBER_LED  			,GPIO_DIR_PORT7_AMBER_LED    		,1},
    {GPIO_BUS_PORT7_GREEN_LED    		,GPIO_SHIFT_PORT7_GREEN_LED  			,GPIO_DIR_PORT7_GREEN_LED    		,1},
	{GPIO_BUS_PORT8_AMBER_LED    		,GPIO_SHIFT_PORT8_AMBER_LED  			,GPIO_DIR_PORT8_AMBER_LED    		,1},
    {GPIO_BUS_PORT8_GREEN_LED    		,GPIO_SHIFT_PORT8_GREEN_LED  			,GPIO_DIR_PORT8_GREEN_LED    		,1},
};

LED_t wg1008_px2_led[] =
{
    {GPIO_NUM_ATT_LED       ,"Attn"     ,0},
    {GPIO_NUM_SW_STATUS_LED ,"Status"   ,0},
    {GPIO_NUM_SW_MODE_LED   ,"Mode"     ,0},
    {GPIO_NUM_FAILOVER_LED  ,"Failover" ,0},
    {GPIO_NUM_POWER_LED     ,"Power"    ,0},
    {0                      ,NULL       ,0},
};

u32 reverseEndian(u32 value)
{
    return (((value & 0x000000FF) << 24) |
            ((value & 0x0000FF00) <<  8) |
            ((value & 0x00FF0000) >>  8) |
            ((value & 0xFF000000) >> 24));
}

u32 reverseBits8(u8 num)
{
    u8 count = sizeof(num) * 8 - 1;
    u8 reverse_num = num;

    num >>= 1;
    while(num)
    {
       reverse_num <<= 1;
       reverse_num |= num & 1;
       num >>= 1;
       count--;
    }
    reverse_num <<= count;
//    printf("reverse_num  0x%02x\n", reverse_num);
    return (u32)reverse_num;
}

u32 reverseBits32(u32 num)
{
    u32 count = sizeof(num) * 8 - 1;
    u32 reverse_num = num;

    num >>= 1;
    while(num)
    {
       reverse_num <<= 1;
       reverse_num |= num & 1;
       num >>= 1;
       count--;
    }
    reverse_num <<= count;
//    printf("reverse_num  0x%02x\n", reverse_num);
    return (u32)reverse_num;
}


u32 reverse32(u32 value) 
{
    u32 tmpVal = 0;
#if defined(ENDIAN_SWAP)
    tmpVal = reverseEndian(value);
#elif defined(BIT_SWAP)
    int idx = 0;
    u32 tmpByte = 0;

    tmpVal = value;
    for(idx = 0; idx < 4; idx++)
    {
        tmpByte = (tmpVal & (0x000000FF<<(idx*8)))>>(idx*8);
        tmpVal &= ~(0x000000FF<<(idx*8));
        tmpVal |= (reverseBits8(tmpByte)<<(idx*8));
    }
#elif defined(ENDIAN_SWAP_AND_BIT_SWAP)
    //tmpVal = reverseEndian(value);
    //tmpVal = reverseBits32(tmpVal);
	tmpVal = reverseBits32(value);
#else
    tmpVal = value;
#endif

    return tmpVal;
}

int regRead(volatile u32 *regAddr, volatile u32 *regVal)
{
    int ret = 0;

#ifdef DBG_GPIO
    printf("Before swap  0x%08x\n", *regAddr);
#endif
    *regVal = reverse32(*regAddr);
#ifdef DBG_GPIO
    printf("After swap  0x%08x\n", *regVal);
#endif
    return ret;
}

int regWrite(volatile u32 *regAddr, u32 regVal)
{
    u32 write_val = 0;
    int ret = 0;

#ifdef DBG_GPIO
    printf("Before swap  0x%08x\n", regVal);
#endif
    write_val = reverse32(regVal);
    *regAddr = write_val;
#ifdef DBG_GPIO
    printf("After swap  0x%08x\n", write_val);
#endif

    return ret;
}

int gpioWrite(u32 gpio_bus, u32 gpio_shift, u32 gpio_type, u32 val)
{
    u32 gpio_reg = NULL;
    u32 read_val = 0;
    int ret = 0;

    if(GPIO_TYPE_DIR == gpio_type)
    {
        gpio_reg = LS2084A_GPIO1_DIR;
    }
    else if(GPIO_TYPE_DATA == gpio_type)
    {
        gpio_reg = LS2084A_GPIO1_DATA;
    }
    else
    {
        printf("Wrong gpio type %d!!!\n", gpio_type);
        return -1;
    }
    gpio_reg += ((gpio_bus - 1)*0x10000);

#ifdef DBG_GPIO
    printf("\nGPIO%u_%02u REG 0x%08x\n", gpio_bus, gpio_shift, gpio_reg);
#endif

    ret = regRead(gpio_reg, &read_val);
    if(ret)
    {
        printf("regRead failed!!!\n");
        return -1;
    }
#ifdef DBG_GPIO
    printf("Read value  0x%08x\n", read_val);
#endif
    if(val)
    {
        read_val |= (1<<gpio_shift);
    }
    else
    {
        read_val &= ~(1<<gpio_shift);
    }
    ret = regWrite(gpio_reg, read_val);
    if(ret)
    {
        printf("regWrite failed!!!\n");
        return -1;
    }
#ifdef DBG_GPIO
    printf("Write value 0x%08x\n", read_val);
#endif

    return 0;
}

int gpioRead(u32 gpio_bus, u32 gpio_shift, u32 gpio_type, u32 *val)
{
    u32 gpio_reg = NULL;
    u32 read_val = 0;
    int ret = 0;

    if(GPIO_TYPE_DIR == gpio_type)
    {
        gpio_reg = LS2084A_GPIO1_DIR;
    }
    else if(GPIO_TYPE_DATA == gpio_type)
    {
        gpio_reg = LS2084A_GPIO1_DATA;
    }
    else
    {
        printf("Wrong gpio type %d!!!\n", gpio_type);
        return -1;
    }
    gpio_reg += ((gpio_bus - 1)*0x10000);
#ifdef DBG_GPIO
    printf("\nGPIO%u_%02u REG 0x%08x\n", gpio_bus, gpio_shift, gpio_reg);
#endif

    ret = regRead(gpio_reg, &read_val);
    if(ret)
    {
        printf("regRead failed!!!\n");
        return -1;
    }

#ifdef DBG_GPIO
    printf("Read value  0x%08x\n", read_val);
#endif
    *val = (read_val & (1<<gpio_shift))?1:0;

    return 0;
}

int getGpioNumByLedName(char *ledName, u32 *gpioNum, u32 *active_high)
{
    int ledLen = 0, idx =0;
    if(!ledName || !gpioNum || !active_high)
    {
        printf("NULL arguments!!!\n");
        return -1;
    }

    ledLen = sizeof(wg1008_px2_led)/sizeof(wg1008_px2_led[0]);
    for(idx = 0; idx < ledLen; idx++)
    {
        if(!strncmp(ledName, wg1008_px2_led[idx].led_name, strlen(ledName)))
        {
            *gpioNum = wg1008_px2_led[idx].gpio_num;
            *active_high = wg1008_px2_led[idx].active_high;
            break;
        }
    }

    if(idx == ledLen)
    {
        printf("Invalid LED name %s!!!\n", ledName);
        printf("Supported LED names: ");
        for(idx =0; idx < ledLen; idx++)
        {
            printf("%s ", wg1008_px2_led[idx].led_name);
        }
        printf("\n");
        return -1;
    }
    else
    {
        return 0;
    }
}


int setGpioLedAllOnOff(u32 mode)
{
    int ledLen = 0, idx =0;
    u32 gpioNum = 0, active_high = 0, gpio_bus = 0, gpio_shift = 0, val = 0;

    ledLen = sizeof(wg1008_px2_led)/sizeof(wg1008_px2_led[0]);
    for(idx = 0; idx < ledLen; idx++)
    {
        gpioNum = wg1008_px2_led[idx].gpio_num;
        active_high = wg1008_px2_led[idx].active_high;
        if(mode == GPIO_LED_MODE_ON)
        {
            val = active_high?1:0;
        }
        else if(mode == GPIO_LED_MODE_OFF)
        {
            val = active_high?0:1;
        }
        else if(mode == GPIO_LED_MODE_NORMAL)
        {
            if(!strncmp("Power", wg1008_px2_led[idx].led_name, strlen("Power")))
            {
                val = active_high?1:0;
            }
            else
            {
                val = active_high?0:1;
            }
        }
        else
        {
            break;
        }
        gpio_bus = wg1008_px2_gpio[gpioNum].gpio_bus;
        gpio_shift = wg1008_px2_gpio[gpioNum].gpio_shift;
        gpioWrite(gpio_bus, gpio_shift, GPIO_TYPE_DATA, val);
    }
#if 0
    if(mode == GPIO_LED_MODE_ON)
    {
        switch_mv88e6190_port_led(PORT_LED_MODE_GREEN);
        udelay(1000000);
        switch_mv88e6190_port_led(PORT_LED_MODE_AMBER);
        udelay(1000000);
        switch_mv88e6190_port_led(PORT_LED_MODE_OFF);
        udelay(1000000);
        switch_mv88e6190_port_led(PORT_LED_MODE_ALLON);
    }
    else if(mode == GPIO_LED_MODE_OFF)
    {
        switch_mv88e6190_port_led(PORT_LED_MODE_OFF);
    }
    else if(mode == GPIO_LED_MODE_NORMAL)
    {
        switch_mv88e6190_port_led(PORT_LED_MODE_NORMAL);
    }
    else
    {
        return -1;
    }
#endif
    return 0;
}


int wg1008_px2_gpio_init(void)
{
    int idx = 0, len = 0;
    volatile u32 *gpio_dir_reg = NULL, *gpio_data_reg = NULL;
    u32 gpio_bus = 0,default_val = 0, gpio_dir = 0, gpio_shift = 0;
    int ret = 0;

    printf("\nInitializing GPIO...");
    len = sizeof(wg1008_px2_gpio)/sizeof(wg1008_px2_gpio[0]);

    for(idx = 0; idx < len; idx++)
    {
        gpio_bus = wg1008_px2_gpio[idx].gpio_bus;
        gpio_shift = wg1008_px2_gpio[idx].gpio_shift;
        gpio_dir = wg1008_px2_gpio[idx].gpio_dir;
        default_val = wg1008_px2_gpio[idx].gpio_default_val;
#ifdef DBG_GPIO
        printf("Initializing GPIO%u_%02u, direction : %s, default value : %u...", gpio_bus, gpio_shift, gpio_dir?"out":"in", default_val);
#endif

        ret = gpioWrite(gpio_bus, gpio_shift, GPIO_TYPE_DATA, default_val);
        if(ret)
        {
            printf("\nFailed to set default value : %u!!!\n", default_val);
            continue;
        }
        ret = gpioWrite(gpio_bus, gpio_shift, GPIO_TYPE_DIR, gpio_dir);
        if(ret)
        {
            printf("\nFailed to set direction : %s!!!\n", gpio_dir?"out":"in");
            continue;
        }
#ifdef DBG_GPIO
        printf("done\n");
#endif
    }
    printf("done\n");

    return ret;
}
#endif

int misc_init_r(void)
{
	char *env_hwconfig;
	u32 __iomem *dcfg_ccsr = (u32 __iomem *)DCFG_BASE;
	u32 val;
	struct ccsr_gur __iomem *gur = (void *)(CONFIG_SYS_FSL_GUTS_ADDR);
	u32 svr = gur_in32(&gur->svr);

	val = in_le32(dcfg_ccsr + DCFG_RCWSR13 / 4);

	env_hwconfig = env_get("hwconfig");

	if (hwconfig_f("dspi", env_hwconfig) &&
	    DCFG_RCWSR13_DSPI == (val & (u32)(0xf << 8)))
		config_board_mux(MUX_TYPE_DSPI);
	else
		config_board_mux(MUX_TYPE_SDHC);

	/*
	 * LS2081ARDB RevF board has smart voltage translator
	 * which needs to be programmed to enable high speed SD interface
	 * by setting GPIO4_10 output to zero
	 */
#ifdef CONFIG_TARGET_LS2081ARDB
		out_le32(GPIO4_GPDIR_ADDR, (1 << 21 |
					    in_le32(GPIO4_GPDIR_ADDR)));
		out_le32(GPIO4_GPDAT_ADDR, (~(1 << 21) &
					    in_le32(GPIO4_GPDAT_ADDR)));
#endif
	if (hwconfig("sdhc"))
		config_board_mux(MUX_TYPE_SDHC);
#if 0
	if (adjust_vdd(0))
		printf("Warning: Adjusting core voltage failed.\n");
#endif
	/*
	 * Default value of board env is based on filename which is
	 * ls2080ardb. Modify board env for other supported SoCs
	 */
	if ((SVR_SOC_VER(svr) == SVR_LS2088A) ||
	    (SVR_SOC_VER(svr) == SVR_LS2048A) ||
		(SVR_SOC_VER(svr) == SVR_LS2084A))
		env_set("board", "ls2088ardb");
	else if ((SVR_SOC_VER(svr) == SVR_LS2081A) ||
	    (SVR_SOC_VER(svr) == SVR_LS2041A))
		env_set("board", "ls2081ardb");

	return 0;
}

void detail_board_ddr_info(void)
{
	puts("\nDDR    ");
	print_size(gd->bd->bi_dram[0].size + gd->bd->bi_dram[1].size, "");
	print_ddr_info(0);
#ifdef CONFIG_SYS_FSL_HAS_DP_DDR
	if (soc_has_dp_ddr() && gd->bd->bi_dram[2].size) {
		puts("\nDP-DDR ");
		print_size(gd->bd->bi_dram[2].size, "");
		print_ddr_info(CONFIG_DP_DDR_CTRL);
	}
#endif
}

#if defined(CONFIG_ARCH_MISC_INIT)
int arch_misc_init(void)
{
	return 0;
}
#endif

#ifdef CONFIG_FSL_MC_ENET
void fdt_fixup_board_enet(void *fdt)
{
	int offset;

	offset = fdt_path_offset(fdt, "/soc/fsl-mc");

	if (offset < 0)
		offset = fdt_path_offset(fdt, "/fsl-mc");

	if (offset < 0) {
		printf("%s: ERROR: fsl-mc node not found in device tree (error %d)\n",
		       __func__, offset);
		return;
	}

	if (get_mc_boot_status() == 0 &&
	    (is_lazy_dpl_addr_valid() || get_dpl_apply_status() == 0))
		fdt_status_okay(fdt, offset);
	else
		fdt_status_fail(fdt, offset);
}

void board_quiesce_devices(void)
{
	fsl_mc_ldpaa_exit(gd->bd);
}
#endif

#ifdef CONFIG_OF_BOARD_SETUP
void fsl_fdt_fixup_flash(void *fdt)
{
	int offset;
#ifdef CONFIG_TFABOOT
	u32 __iomem *dcfg_ccsr = (u32 __iomem *)DCFG_BASE;
	u32 val;
#endif

/*
 * IFC and QSPI are muxed on board.
 * So disable IFC node in dts if QSPI is enabled or
 * disable QSPI node in dts in case QSPI is not enabled.
 */
#ifdef CONFIG_TFABOOT
	enum boot_src src = get_boot_src();
	bool disable_ifc = false;

	switch (src) {
	case BOOT_SOURCE_IFC_NOR:
		disable_ifc = false;
		break;
	case BOOT_SOURCE_QSPI_NOR:
		disable_ifc = true;
		break;
	default:
		val = in_le32(dcfg_ccsr + DCFG_RCWSR15 / 4);
		if (DCFG_RCWSR15_IFCGRPABASE_QSPI == (val & (u32)0x3))
			disable_ifc = true;
		break;
	}

	if (disable_ifc) {
		offset = fdt_path_offset(fdt, "/soc/ifc");

		if (offset < 0)
			offset = fdt_path_offset(fdt, "/ifc");
	} else {
		offset = fdt_path_offset(fdt, "/soc/quadspi");

		if (offset < 0)
			offset = fdt_path_offset(fdt, "/quadspi");
	}

#else
#ifdef CONFIG_FSL_QSPI
	offset = fdt_path_offset(fdt, "/soc/ifc");

	if (offset < 0)
		offset = fdt_path_offset(fdt, "/ifc");
#else
	offset = fdt_path_offset(fdt, "/soc/quadspi");

	if (offset < 0)
		offset = fdt_path_offset(fdt, "/quadspi");
#endif
#endif

	if (offset < 0)
		return;

	fdt_status_disabled(fdt, offset);
}

int ft_board_setup(void *blob, bd_t *bd)
{
	int i;
	bool mc_memory_bank = false;

	u64 *base;
	u64 *size;
	u64 mc_memory_base = 0;
	u64 mc_memory_size = 0;
	u16 total_memory_banks;

	ft_cpu_setup(blob, bd);

	fdt_fixup_mc_ddr(&mc_memory_base, &mc_memory_size);

	if (mc_memory_base != 0)
		mc_memory_bank = true;

	total_memory_banks = CONFIG_NR_DRAM_BANKS + mc_memory_bank;

	base = calloc(total_memory_banks, sizeof(u64));
	size = calloc(total_memory_banks, sizeof(u64));

	/* fixup DT for the two GPP DDR banks */
	base[0] = gd->bd->bi_dram[0].start;
	size[0] = gd->bd->bi_dram[0].size;
	base[1] = gd->bd->bi_dram[1].start;
	size[1] = gd->bd->bi_dram[1].size;

#ifdef CONFIG_RESV_RAM
	/* reduce size if reserved memory is within this bank */
	if (gd->arch.resv_ram >= base[0] &&
	    gd->arch.resv_ram < base[0] + size[0])
		size[0] = gd->arch.resv_ram - base[0];
	else if (gd->arch.resv_ram >= base[1] &&
		 gd->arch.resv_ram < base[1] + size[1])
		size[1] = gd->arch.resv_ram - base[1];
#endif

	if (mc_memory_base != 0) {
		for (i = 0; i <= total_memory_banks; i++) {
			if (base[i] == 0 && size[i] == 0) {
				base[i] = mc_memory_base;
				size[i] = mc_memory_size;
				break;
			}
		}
	}

	fdt_fixup_memory_banks(blob, base, size, total_memory_banks);

	fdt_fsl_mc_fixup_iommu_map_entry(blob);

	fsl_fdt_fixup_dr_usb(blob, bd);

	fsl_fdt_fixup_flash(blob);

#ifdef CONFIG_FSL_MC_ENET
	fdt_fixup_board_enet(blob);
#endif

	return 0;
}
#endif

void qixis_dump_switch(void)
{
#ifdef CONFIG_FSL_QIXIS
	int i, nr_of_cfgsw;

	QIXIS_WRITE(cms[0], 0x00);
	nr_of_cfgsw = QIXIS_READ(cms[1]);

	puts("DIP switch settings dump:\n");
	for (i = 1; i <= nr_of_cfgsw; i++) {
		QIXIS_WRITE(cms[0], i);
		printf("SW%d = (0x%02x)\n", i, QIXIS_READ(cms[1]));
	}
#endif
}

/*
 * Board rev C and earlier has duplicated I2C addresses for 2nd controller.
 * Both slots has 0x54, resulting 2nd slot unusable.
 */
void update_spd_address(unsigned int ctrl_num,
			unsigned int slot,
			unsigned int *addr)
{
#ifndef CONFIG_TARGET_LS2081ARDB
#ifdef CONFIG_FSL_QIXIS
	u8 sw;

	sw = QIXIS_READ(arch);
	if ((sw & 0xf) < 0x3) {
		if (ctrl_num == 1 && slot == 0)
			*addr = SPD_EEPROM_ADDRESS4;
		else if (ctrl_num == 1 && slot == 1)
			*addr = SPD_EEPROM_ADDRESS3;
	}
#endif
#endif
}
