// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 */

#include <common.h>
#include <i2c.h>
#include <fdt_support.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/fsl_serdes.h>
#include <asm/arch/ppa.h>
#include <asm/arch/soc.h>
#include <asm/arch-fsl-layerscape/fsl_icid.h>
#include <hwconfig.h>
#include <ahci.h>
#include <mmc.h>
#include <scsi.h>
#include <fm_eth.h>
#include <fsl_csu.h>
#include <fsl_esdhc.h>
#include <power/mc34vr500_pmic.h>
#include "cpld.h"
#include <fsl_sec.h>

DECLARE_GLOBAL_DATA_PTR;

int board_early_init_f(void)
{
	fsl_lsch2_early_init_f();

	return 0;
}

#ifdef CONFIG_WG1008_PX1
//#define DBG_GPIO
//#define ENDIAN_SWAP
//#define BIT_SWAP
#define ENDIAN_SWAP_AND_BIT_SWAP

extern int switch_mv88e6190_port_led(u32 mode);

GPIO_t wg1008_px1_gpio[] =
{
#ifdef CONFIG_WG1008_PX1
    {GPIO_BUS_EN_CPU_1V8_NCT3961S_1         ,GPIO_SHIFT_EN_CPU_1V8_NCT3961S_1           ,GPIO_DIR_EN_CPU_1V8_NCT3961S_1             ,1},
    {GPIO_BUS_EN_CPU_1V8_NCT3961S_2         ,GPIO_SHIFT_EN_CPU_1V8_NCT3961S_2           ,GPIO_DIR_EN_CPU_1V8_NCT3961S_2             ,1},
    {GPIO_BUS_RST_CPU_N                     ,GPIO_SHIFT_RST_CPU_N                       ,GPIO_DIR_RST_CPU_N                         ,1},
    {GPIO_BUS_Module_Bay_Present_N          ,GPIO_SHIFT_Module_Bay_Present_N            ,GPIO_DIR_Module_Bay_Present_N              ,1},
    {GPIO_BUS_RST_Module_PERST_N            ,GPIO_SHIFT_RST_Module_PERST_N              ,GPIO_DIR_RST_Module_PERST_N                ,1},
    {GPIO_BUS_INT_ALL_ALM_7904D_CPU_N       ,GPIO_SHIFT_INT_ALL_ALM_7904D_CPU_N         ,GPIO_DIR_INT_ALL_ALM_7904D_CPU_N           ,1},
    {GPIO_BUS_INT_RTC_PHY_N                 ,GPIO_SHIFT_INT_RTC_PHY_N                   ,GPIO_DIR_INT_RTC_PHY_N                     ,1},
    {GPIO_BUS_CLKREQ_CPU_AND_N              ,GPIO_SHIFT_CLKREQ_CPU_AND_N                ,GPIO_DIR_CLKREQ_CPU_AND_N                  ,1},
    {GPIO_BUS_RST_CPU_PoE_1V8_N             ,GPIO_SHIFT_RST_CPU_PoE_1V8_N               ,GPIO_DIR_RST_CPU_PoE_1V8_N                 ,1},
    {GPIO_BUS_INT_POE_R_N                   ,GPIO_SHIFT_INT_POE_R_N                     ,GPIO_DIR_INT_POE_R_N                       ,1},
    {GPIO_BUS_NCT_3961S_FLT_CPU_1V8_N_ALL   ,GPIO_SHIFT_NCT_3961S_FLT_CPU_1V8_N_ALL     ,GPIO_DIR_NCT_3961S_FLT_CPU_1V8_N_ALL       ,1},
    {GPIO_BUS_PCIE_WAKE_BUF_R_N             ,GPIO_SHIFT_PCIE_WAKE_BUF_R_N               ,GPIO_DIR_PCIE_WAKE_BUF_R_N                 ,1},
    {GPIO_BUS_INT_TS_TEMP_ALM_3V3_R_N       ,GPIO_SHIFT_INT_TS_TEMP_ALM_3V3_R_N         ,GPIO_DIR_INT_TS_TEMP_ALM_3V3_R_N           ,1},
    {GPIO_BUS_EN_LAN_Port_LED_1V8_N         ,GPIO_SHIFT_EN_LAN_Port_LED_1V8_N           ,GPIO_DIR_EN_LAN_Port_LED_1V8_N             ,0},
    {GPIO_BUS_EN_CPU_DRV_LED_1V8_N          ,GPIO_SHIFT_EN_CPU_DRV_LED_1V8_N            ,GPIO_DIR_EN_CPU_DRV_LED_1V8_N              ,0},
    {GPIO_BUS_RST_CPU_TPM_N                 ,GPIO_SHIFT_RST_CPU_TPM_N                   ,GPIO_DIR_RST_CPU_TPM_N                     ,0},
    {GPIO_BUS_RST_CPU_TPM_N                 ,GPIO_SHIFT_RST_CPU_TPM_N                   ,GPIO_DIR_RST_CPU_TPM_N                     ,1},
    {GPIO_BUS_NCT_3961S_MODE_CPU_1V8_1      ,GPIO_SHIFT_NCT_3961S_MODE_CPU_1V8_1        ,GPIO_DIR_NCT_3961S_MODE_CPU_1V8_1          ,1},
    {GPIO_BUS_NCT_3961S_MODE_CPU_1V8_2      ,GPIO_SHIFT_NCT_3961S_MODE_CPU_1V8_2        ,GPIO_DIR_NCT_3961S_MODE_CPU_1V8_2          ,1},
    {GPIO_BUS_NCT_3961S_FTI_CPU_1V8_1       ,GPIO_SHIFT_NCT_3961S_FTI_CPU_1V8_1         ,GPIO_DIR_NCT_3961S_FTI_CPU_1V8_1           ,1},
    {GPIO_BUS_NCT_3961S_FTI_CPU_1V8_2       ,GPIO_SHIFT_NCT_3961S_FTI_CPU_1V8_2         ,GPIO_DIR_NCT_3961S_FTI_CPU_1V8_2           ,1},
    {GPIO_BUS_INT_TPM_PIRQ_1V8_N            ,GPIO_SHIFT_INT_TPM_PIRQ_1V8_N              ,GPIO_DIR_INT_TPM_PIRQ_1V8_N                ,1},
    {GPIO_BUS_Module_Type_CONN_CPU_1V8_0    ,GPIO_SHIFT_Module_Type_CONN_CPU_1V8_0      ,GPIO_DIR_Module_Type_CONN_CPU_1V8_0        ,1},
    {GPIO_BUS_Module_Type_CONN_CPU_1V8_1    ,GPIO_SHIFT_Module_Type_CONN_CPU_1V8_1      ,GPIO_DIR_Module_Type_CONN_CPU_1V8_1        ,1},
    {GPIO_BUS_PORT1_LED                     ,GPIO_SHIFT_PORT1_AMBER_LED                 ,GPIO_DIR_PORT1_LED                         ,1},
    {GPIO_BUS_PORT1_LED                     ,GPIO_SHIFT_PORT1_GREEN_LED                 ,GPIO_DIR_PORT1_LED                         ,1},
    {GPIO_BUS_PORT2_LED                     ,GPIO_SHIFT_PORT2_AMBER_LED                 ,GPIO_DIR_PORT2_LED                         ,1},
    {GPIO_BUS_PORT2_LED                     ,GPIO_SHIFT_PORT2_GREEN_LED                 ,GPIO_DIR_PORT2_LED                         ,1},
    {GPIO_BUS_PORT3_LED                     ,GPIO_SHIFT_PORT3_AMBER_LED                 ,GPIO_DIR_PORT3_LED                         ,1},
    {GPIO_BUS_PORT3_LED                     ,GPIO_SHIFT_PORT3_GREEN_LED                 ,GPIO_DIR_PORT3_LED                         ,1},
    {GPIO_BUS_PORT4_LED                     ,GPIO_SHIFT_PORT4_AMBER_LED                 ,GPIO_DIR_PORT4_LED                         ,1},
    {GPIO_BUS_PORT4_LED                     ,GPIO_SHIFT_PORT4_GREEN_LED                 ,GPIO_DIR_PORT4_LED                         ,1},
    {GPIO_BUS_PORT5_LED                     ,GPIO_SHIFT_PORT5_AMBER_LED                 ,GPIO_DIR_PORT5_LED                         ,1},
    {GPIO_BUS_PORT5_LED                     ,GPIO_SHIFT_PORT5_GREEN_LED                 ,GPIO_DIR_PORT5_LED                         ,1},
    {GPIO_BUS_PORT6_LED                     ,GPIO_SHIFT_PORT6_AMBER_LED                 ,GPIO_DIR_PORT6_LED                         ,1},
    {GPIO_BUS_PORT6_LED                     ,GPIO_SHIFT_PORT6_GREEN_LED                 ,GPIO_DIR_PORT6_LED                         ,1},
    {GPIO_BUS_PORT7_LED                     ,GPIO_SHIFT_PORT7_AMBER_LED                 ,GPIO_DIR_PORT7_LED                         ,1},
    {GPIO_BUS_PORT7_LED                     ,GPIO_SHIFT_PORT7_GREEN_LED                 ,GPIO_DIR_PORT7_LED                         ,1},
    {GPIO_BUS_PORT8_LED                     ,GPIO_SHIFT_PORT8_AMBER_LED                 ,GPIO_DIR_PORT8_LED                         ,1},
    {GPIO_BUS_PORT8_LED                     ,GPIO_SHIFT_PORT8_GREEN_LED                 ,GPIO_DIR_PORT8_LED                         ,1},
#if 0
    {GPIO_BUS_POWER_LED                     ,GPIO_SHIFT_POWER_LED                       ,GPIO_DIR_POWER_LED                         ,0},
    {GPIO_BUS_STATUS_LED                    ,GPIO_SHIFT_STATUS_RED_LED                  ,GPIO_DIR_STATUS_LED                        ,0},
    {GPIO_BUS_STATUS_LED                    ,GPIO_SHIFT_STATUS_GREEN_LED                ,GPIO_DIR_STATUS_LED                        ,1},
    {GPIO_BUS_STORAGE_LED                   ,GPIO_SHIFT_STORAGE_LED                     ,GPIO_DIR_STORAGE_LED                       ,0},
#endif
#else
    {GPIO_BUS_RESET_IN                ,GPIO_SHIFT_RESET_IN                ,GPIO_DIR_RESET_IN                ,1},
    {GPIO_BUS_ATT_LED                 ,GPIO_SHIFT_ATT_LED                 ,GPIO_DIR_ATT_LED                 ,1},
    {GPIO_BUS_SW_STATUS_LED           ,GPIO_SHIFT_SW_STATUS_LED           ,GPIO_DIR_SW_STATUS_LED           ,1},
    {GPIO_BUS_SW_MODE_LED             ,GPIO_SHIFT_SW_MODE_LED             ,GPIO_DIR_SW_MODE_LED             ,1},
    {GPIO_BUS_FAILOVER_LED            ,GPIO_SHIFT_FAILOVER_LED            ,GPIO_DIR_FAILOVER_LED            ,1},
    //{GPIO_BUS_POWER_LED               ,GPIO_SHIFT_POWER_LED               ,GPIO_DIR_POWER_LED               ,0},
    {GPIO_BUS_TMP_RESET               ,GPIO_SHIFT_TMP_RESET               ,GPIO_DIR_TMP_RESET               ,1},
    {GPIO_BUS_MARVELL_SWITCH_RESET    ,GPIO_SHIFT_MARVELL_SWITCH_RESET    ,GPIO_DIR_MARVELL_SWITCH_RESET    ,1},
    {GPIO_BUS_RST_MOD_RESET           ,GPIO_SHIFT_RST_MOD_RESET           ,GPIO_DIR_RST_MOD_RESET           ,1},
    {GPIO_BUS_FAN_DRIVER_FON          ,GPIO_SHIFT_FAN_DRIVER_FON          ,GPIO_DIR_FAN_DRIVER_FON          ,1},
    {GPIO_BUS_INT_ALL_ALM_7904D_CPU_N ,GPIO_SHIFT_INT_ALL_ALM_7904D_CPU_N ,GPIO_DIR_INT_ALL_ALM_7904D_CPU_N ,1},
    {GPIO_BUS_MOD_BRD_TYPE_0          ,GPIO_SHIFT_MOD_BRD_TYPE_0          ,GPIO_DIR_MOD_BRD_TYPE_0          ,1},
    {GPIO_BUS_MOD_BRD_TYPE_1          ,GPIO_SHIFT_MOD_BRD_TYPE_1          ,GPIO_DIR_MOD_BRD_TYPE_1          ,1},
    {GPIO_BUS_INT_RTC_PHY_N           ,GPIO_SHIFT_INT_RTC_PHY_N           ,GPIO_DIR_INT_RTC_PHY_N           ,1},
    {GPIO_BUS_MOD_BAY_PRESENT_N       ,GPIO_SHIFT_MOD_BAY_PRESENT_N       ,GPIO_DIR_MOD_BAY_PRESENT_N       ,1},
    //{GPIO_BUS_LED_CPU_MOD_1           ,GPIO_SHIFT_LED_CPU_MOD_1           ,GPIO_DIR_LED_CPU_MOD_1           ,1},
    {GPIO_BUS_LED_CPU_MOD_2           ,GPIO_SHIFT_LED_CPU_MOD_2           ,GPIO_DIR_LED_CPU_MOD_2           ,1},
    {GPIO_BUS_LED_CPU_MOD_3           ,GPIO_SHIFT_LED_CPU_MOD_3           ,GPIO_DIR_LED_CPU_MOD_3           ,1},
    {GPIO_BUS_LED_CPU_MOD_4           ,GPIO_SHIFT_LED_CPU_MOD_4           ,GPIO_DIR_LED_CPU_MOD_4           ,1},
    {1, 24, 1, 1},
    {2, 0, 0, 1},
    {3, 23, 1, 0},
    {3, 5, 1, 1},
    {3, 4, 1, 1},
    {3, 3, 1, 1},
    {3, 2, 1, 1},
    {3, 6, 1, 1},
    {3, 7, 1, 1},
    {3, 12, 1, 1},
    {3, 11, 1, 1},
    {3, 10, 1, 1},
    {3, 9, 1, 1},
    {3, 14, 1, 1},
    {3, 13, 1, 1},
    {3, 8, 1, 1},
    {3, 18, 1, 1},
    {3, 17, 1, 1},
    {3, 16, 1, 1},
#endif
};

LED_t wg1008_px1_led[] =
{
    {GPIO_NUM_ATT_LED       ,"Attn"     ,0}, //Power LED
    {GPIO_NUM_SW_STATUS_LED ,"Status"   ,0}, //GPIO_25
    {GPIO_NUM_SW_MODE_LED   ,"Mode"     ,0}, //GPIO_26
    {GPIO_NUM_FAILOVER_LED  ,"Failover" ,0}, //Storage LED
    {GPIO_NUM_POWER_LED     ,"Power"    ,0},
    {GPIO_NUM_LED_CPU_MOD_1 ,"Module1"  ,0},
    {GPIO_NUM_LED_CPU_MOD_2 ,"Module2"  ,0},
    {GPIO_NUM_LED_CPU_MOD_3 ,"Module3"  ,0},
    {GPIO_NUM_LED_CPU_MOD_4 ,"Module4"  ,0},
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
    tmpVal = reverseEndian(value);
    tmpVal = reverseBits32(tmpVal);
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
        gpio_reg = LS1046A_GPIO1_DIR;
    }
    else if(GPIO_TYPE_DATA == gpio_type)
    {
        gpio_reg = LS1046A_GPIO1_DATA;
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
        gpio_reg = LS1046A_GPIO1_DIR;
    }
    else if(GPIO_TYPE_DATA == gpio_type)
    {
        gpio_reg = LS1046A_GPIO1_DATA;
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

    ledLen = sizeof(wg1008_px1_led)/sizeof(wg1008_px1_led[0]);
    for(idx = 0; idx < ledLen; idx++)
    {
        if(!strncmp(ledName, wg1008_px1_led[idx].led_name, strlen(ledName)))
        {
            *gpioNum = wg1008_px1_led[idx].gpio_num;
            *active_high = wg1008_px1_led[idx].active_high;
            break;
        }
    }

    if(idx == ledLen)
    {
        printf("Invalid LED name %s!!!\n", ledName);
        printf("Supported LED names: ");
        for(idx =0; idx < ledLen; idx++)
        {
            printf("%s ", wg1008_px1_led[idx].led_name);
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

    ledLen = sizeof(wg1008_px1_led)/sizeof(wg1008_px1_led[0]);
    for(idx = 0; idx < ledLen; idx++)
    {
        gpioNum = wg1008_px1_led[idx].gpio_num;
        active_high = wg1008_px1_led[idx].active_high;
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
            if(!strncmp("Power", wg1008_px1_led[idx].led_name, strlen("Power")))
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
        gpio_bus = wg1008_px1_gpio[gpioNum].gpio_bus;
        gpio_shift = wg1008_px1_gpio[gpioNum].gpio_shift;
        gpioWrite(gpio_bus, gpio_shift, GPIO_TYPE_DATA, val);
    }

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

    return 0;
}

int resetBtnPressed(void)
{
    u32 val = 0, gpioBus = 0, gpioShift = 0, gpioNum = 0, activeHigh = 0;

    gpioBus = GPIO_BUS_RST_CPU_N;
    gpioShift = GPIO_SHIFT_RST_CPU_N;
    if(gpioRead(gpioBus, gpioShift, GPIO_TYPE_DATA, &val))
    {
        return 0;
    }
    if(getGpioNumByLedName("Status", &gpioNum, &activeHigh))
    {
        return -1;
    }

    gpioBus = wg1008_px1_gpio[gpioNum].gpio_bus;
    gpioShift = wg1008_px1_gpio[gpioNum].gpio_shift;

    if(!val) /* Pressed and light on Status LED */
    {
        gpioWrite(gpioBus, gpioShift, GPIO_TYPE_DATA, activeHigh?1:0);
    }

    return val?0:1; /* Active Low */
}

int wg1008_px1_gpio_init(void)
{
    int idx = 0, len = 0;
    volatile u32 *gpio_dir_reg = NULL, *gpio_data_reg = NULL;
    u32 gpio_bus = 0,default_val = 0, gpio_dir = 0, gpio_shift = 0;
    int ret = 0;

    printf("\nInitializing GPIO...");
    len = sizeof(wg1008_px1_gpio)/sizeof(wg1008_px1_gpio[0]);

    for(idx = 0; idx < len; idx++)
    {
        gpio_bus = wg1008_px1_gpio[idx].gpio_bus;
        gpio_shift = wg1008_px1_gpio[idx].gpio_shift;
        gpio_dir = wg1008_px1_gpio[idx].gpio_dir;
        default_val = wg1008_px1_gpio[idx].gpio_default_val;
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

#ifndef CONFIG_SPL_BUILD
int checkboard(void)
{
	static const char *freq[2] = {"100.00MHZ", "156.25MHZ"};
	u8 cfg_rcw_src1, cfg_rcw_src2;
	u16 cfg_rcw_src;
	u8 sd1refclk_sel;

	puts("Board: LS1046ARDB, boot from ");

	cfg_rcw_src1 = CPLD_READ(cfg_rcw_src1);
	cfg_rcw_src2 = CPLD_READ(cfg_rcw_src2);
	cpld_rev_bit(&cfg_rcw_src1);
	cfg_rcw_src = cfg_rcw_src1;
	cfg_rcw_src = (cfg_rcw_src << 1) | cfg_rcw_src2;

	if (cfg_rcw_src == 0x44)
		printf("QSPI vBank %d\n", CPLD_READ(vbank));
	else if (cfg_rcw_src == 0x40)
		puts("SD\n");
	else
#ifdef CONFIG_WG1008_PX1
        printf("QSPI vBank 0\n");
#else
		puts("Invalid setting of SW5\n");

	printf("CPLD:  V%x.%x\nPCBA:  V%x.0\n", CPLD_READ(cpld_ver),
	       CPLD_READ(cpld_ver_sub), CPLD_READ(pcba_ver));
#endif
	puts("SERDES Reference Clocks:\n");
#ifdef CONFIG_WG1008_PX1
    sd1refclk_sel = 0;
#else
	sd1refclk_sel = CPLD_READ(sd1refclk_sel);
#endif
	printf("SD1_CLK1 = %s, SD1_CLK2 = %s\n", freq[sd1refclk_sel], freq[sd1refclk_sel^1]);

	return 0;
}

int board_init(void)
{
	struct ccsr_scfg *scfg = (struct ccsr_scfg *)CONFIG_SYS_FSL_SCFG_ADDR;

#ifdef CONFIG_SECURE_BOOT
	/*
	 * In case of Secure Boot, the IBR configures the SMMU
	 * to allow only Secure transactions.
	 * SMMU must be reset in bypass mode.
	 * Set the ClientPD bit and Clear the USFCFG Bit
	 */
	u32 val;
	val = (in_le32(SMMU_SCR0) | SCR0_CLIENTPD_MASK) & ~(SCR0_USFCFG_MASK);
	out_le32(SMMU_SCR0, val);
	val = (in_le32(SMMU_NSCR0) | SCR0_CLIENTPD_MASK) & ~(SCR0_USFCFG_MASK);
	out_le32(SMMU_NSCR0, val);
#endif

#ifdef CONFIG_FSL_CAAM
	sec_init();
#endif

#ifdef CONFIG_FSL_LS_PPA
	ppa_init();
#endif

	/* invert AQR105 IRQ pins polarity */
	out_be32(&scfg->intpcr, AQR105_IRQ_MASK);

	return 0;
}

int board_setup_core_volt(u32 vdd)
{
	bool en_0v9;

	en_0v9 = (vdd == 900) ? true : false;
	cpld_select_core_volt(en_0v9);

	return 0;
}

int get_serdes_volt(void)
{
	return mc34vr500_get_sw_volt(SW4);
}

int set_serdes_volt(int svdd)
{
	return mc34vr500_set_sw_volt(SW4, svdd);
}

int power_init_board(void)
{
	int ret;

	ret = power_mc34vr500_init(0);
	if (ret)
		return ret;

	setup_chip_volt();

	return 0;
}

void config_board_mux(void)
{
#ifdef CONFIG_HAS_FSL_XHCI_USB
	struct ccsr_scfg *scfg = (struct ccsr_scfg *)CONFIG_SYS_FSL_SCFG_ADDR;
	u32 usb_pwrfault;

	/* USB3 is not used, configure mux to IIC4_SCL/IIC4_SDA */
	out_be32(&scfg->rcwpmuxcr0, 0x3300);
	out_be32(&scfg->usbdrvvbus_selcr, SCFG_USBDRVVBUS_SELCR_USB1);
	usb_pwrfault = (SCFG_USBPWRFAULT_DEDICATED <<
			SCFG_USBPWRFAULT_USB3_SHIFT) |
			(SCFG_USBPWRFAULT_DEDICATED <<
			SCFG_USBPWRFAULT_USB2_SHIFT) |
			(SCFG_USBPWRFAULT_SHARED <<
			SCFG_USBPWRFAULT_USB1_SHIFT);
	out_be32(&scfg->usbpwrfault_selcr, usb_pwrfault);
#endif
}

#ifdef CONFIG_MISC_INIT_R
int misc_init_r(void)
{
	config_board_mux();
	return 0;
}
#endif

int ft_board_setup(void *blob, bd_t *bd)
{
	u64 base[CONFIG_NR_DRAM_BANKS];
	u64 size[CONFIG_NR_DRAM_BANKS];

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

	return 0;
}
#endif
