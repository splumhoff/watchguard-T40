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
#include <i2c.h>

__weak void reset_misc(void)
{
}

int set_cpld_reset(void)
{
	struct udevice *dev;
	int ret;
	uint8_t buf = 0xff;
	ret = i2c_get_chip_for_busnum(1, 0x32, 1, &dev);
	if (ret) {
		printf("Cannot find CPLD: %d\n", ret);
		return 0;
	}

	ret = dm_i2c_write(dev, 0x03, &buf, 1);
	if (ret) {
		printf("Failed to write reset value via I2C\n");
		return 0;
	}
	return 0;
}

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
