// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Alex Zuepke <azu@sysgo.de>
 *
 * (C) Copyright 2002
 * Gary Jennejohn, DENX Software Engineering, <garyj@denx.de>
 *
 * (C) Copyright 2004
 * DAVE Srl
 * http://www.dave-tech.it
 * http://www.wawnet.biz
 * mailto:info@wawnet.biz
 *
 * (C) Copyright 2004 Texas Insturments
 */

#include <common.h>
#ifdef CONFIG_WG1008_PX1
#include <i2c.h>
#endif

__weak void reset_misc(void)
{
}

#ifdef CONFIG_WG1008_PX1
enum i2c_err_op {
	I2C_ERR_READ,
	I2C_ERR_WRITE,
};

static int i2c_report_err(int ret, enum i2c_err_op op)
{
	printf("Error %s the chip: %d\n",
	       op == I2C_ERR_READ ? "reading" : "writing", ret);

	return CMD_RET_FAILURE;
}

int set_cpld_reset(void)
{
	int ret; int bus = 3; 
	uint8_t buf = 0x2; uint8_t cpld_rst = 0xff;

	ret = i2c_set_bus_num(bus);
	if (ret) {
		printf("Failure changing bus number (%d)\n", ret);
		return 0;
	}
	
	ret = i2c_write(0x71, 0x0, 1, &buf, 1);
	if (ret) {
		printf("Failed to access PCA9546 Channel\n");
		return i2c_report_err(ret, I2C_ERR_WRITE);
	}
	
	ret = i2c_write(0x32, 0x3, 1, &cpld_rst, 1);
	if (ret) {
		printf("Failed to write reset value via CPLD\n");
		return i2c_report_err(ret, I2C_ERR_WRITE);
	}
	
	return 0;
}
#endif // CONFIG_WG1008_PX1

#ifdef CONFIG_WG1008_PX2

int set_cpld_reset(void)
{
	struct udevice *dev;
	int ret;
	uint8_t buf = 0xf; uint8_t cpld_rst = 0xff;

	ret = i2c_get_chip_for_busnum(2, 0x71, 1, &dev);
	if (ret) {
		printf("Can't find PCA9546: %d\n", ret);
		return 0;
	}

	ret = dm_i2c_write(dev, 0x0, &buf, 1);
	if (ret) {
		printf("Failed to access PCA9546 Channel\n");
		return 0;
	}

#if 1
	ret = i2c_get_chip_for_busnum(2, 0x32, 1, &dev);
#else
	ret = i2c_get_chip_for_busnum(2, 0x2d, 1, &dev);
#endif
	if (ret) {
		printf("Can't find CPLD: %d\n", ret);
		return 0;
	}

#if 1
	ret = dm_i2c_write(dev, 0x3, &cpld_rst, 1);
#else
	ret = dm_i2c_write(dev, 0x1, &buf, 1);
#endif
	if (ret) {
		printf("Failed to write reset value via CPLD\n");
		return 0;
	}
	
	return 0;
}
#endif // CONFIG_WG1008_PX2


int do_reset(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	puts ("resetting ...\n");

	udelay (50000);				/* wait 50 ms */

	disable_interrupts();

	reset_misc();
	reset_cpu(0);
	set_cpld_reset();

	/*NOTREACHED*/
	return 0;
}
