// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

/* #define	DEBUG	*/

#include <common.h>
#include <autoboot.h>
#include <cli.h>
#include <console.h>
#include <env.h>
#include <version.h>

/*
 * Board-specific Platform code can reimplement show_boot_progress () if needed
 */
__weak void show_boot_progress(int val) {}

static void run_preboot_environment_command(void)
{
	char *p;

	p = env_get("preboot");
	if (p != NULL) {
		int prev = 0;

		if (IS_ENABLED(CONFIG_AUTOBOOT_KEYED))
			prev = disable_ctrlc(1); /* disable Ctrl-C checking */

		run_command_list(p, -1, 0);

		if (IS_ENABLED(CONFIG_AUTOBOOT_KEYED))
			disable_ctrlc(prev);	/* restore Ctrl-C checking */
	}
}

#if defined (CONFIG_WG2010_PX3) || defined (CONFIG_WG2012_PX4)
#define RESET_BUTTON_GPIO_ADDR 0x2300008
#define BUTTON_RESET                (14)

/*
* Read button status by given gpio number
*/
int read_btn_status(unsigned int btn)
{
        unsigned int reg_val = *(int *)(RESET_BUTTON_GPIO_ADDR);

        /* printf("read_btn_status: btn = %d, reg_val(%p) = 0x%x\n", btn, RESET_BUTTON_GPIO_ADDR, reg_val); */
        /* Low active */
        if (reg_val & (0x1 << btn) )
                return 0;
        else
                return 1;
}

unsigned char detect_SYSB_keypress(void)
{
        int status = 0;
        int i;
        for(i = 0; i < 3; i++) {

                if (read_btn_status(BUTTON_RESET)) {
                        printf("booting into SYSB ... \n");
                        status = 1;
                        break;
                }
                udelay(1000);
        }

        return status;
}
#endif /* end of #if defined (CONFIG_WG2010_PX3) || defined (CONFIG_WG2012_PX4) */

#ifdef CONFIG_MV88E6191X_SWITCH
extern int switch_mv88e6190_all_port_enable_once(void);
#endif
/* We come here after U-Boot is initialised and ready to process commands */
void main_loop(void)
{
	const char *s;

	bootstage_mark_name(BOOTSTAGE_ID_MAIN_LOOP, "main_loop");

	if (IS_ENABLED(CONFIG_VERSION_VARIABLE))
		env_set("ver", version_string);  /* set version variable */

#if defined (CONFIG_WG2010_PX3) || defined (CONFIG_WG2012_PX4)
    extern int resetBtnPressFlag;
	char othbootargsStr[1024] = "";
	char *firstMacAddr = env_get("ethaddr");

    resetBtnPressFlag = detect_SYSB_keypress();
    snprintf(othbootargsStr, sizeof(othbootargsStr) - 1, "DUT=%d ethaddr='%s' uboot_ver='%s' iommu.passthrough=1", resetBtnPressFlag?1:0, firstMacAddr, U_BOOT_VERSION_STRING);
	//snprintf(othbootargsStr, sizeof(othbootargsStr) - 1, "DUT=%d ethaddr='%s' uboot_ver='%s' default_hugepagesz=1024m hugepagesz=1024m hugepages=2 pci=pcie_bus_perf iommu.passthrough=1", resetBtnPressFlag?1:0, firstMacAddr, U_BOOT_VERSION_STRING);
    env_set("othbootargs", othbootargsStr);  /* set version variable */
#endif

	cli_init();

	if (IS_ENABLED(CONFIG_USE_PREBOOT))
		run_preboot_environment_command();

	if (IS_ENABLED(CONFIG_UPDATE_TFTP))
		update_tftp(0UL, NULL, NULL);

	s = bootdelay_process();
	if (cli_process_fdt(&s))
		cli_secure_boot_cmd(s);

	if(resetBtnPressFlag == 1) {
		if(env_get("Burn_In_Time") != NULL) {
			 /* Assuming that mfgsys is the only user of "Burn_In_Time" */
			env_set("bootcmd", "run wgBootODMOS");
		} else {
			if(!env_get("wgBootSysB")) {
				/* throw an errors and exit booting */
				printf("Reset button is pressed, but fail to boot into SYSB since env variable \"wgBootSysB\" can't be found!\n");
				run_command("print wgBootSysB", 0);
				cli_loop();
			}
			else {
				env_set("bootcmd", "run wgBootSysB");
			}
		}
		s = env_get("bootcmd");
		run_command (s, 0);
	}

	autoboot_command(s);

#ifdef CONFIG_MV88E6191X_SWITCH
    switch_mv88e6190_all_port_enable_once(); /* Make switch forward for network commands */
#endif

	cli_loop();
	panic("No CLI available");
}
