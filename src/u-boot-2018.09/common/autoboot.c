// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */
#define DEBUG

#include <common.h>
#include <autoboot.h>
#include <bootretry.h>
#include <cli.h>
#include <console.h>
#include <fdtdec.h>
#include <menu.h>
#include <post.h>
#include <u-boot/sha256.h>
#include <bootcount.h>

DECLARE_GLOBAL_DATA_PTR;

#define MAX_DELAY_STOP_STR 32

#ifndef DEBUG_BOOTKEYS
#define DEBUG_BOOTKEYS 0
#endif
#define debug_bootkeys(fmt, args...)		\
	debug_cond(DEBUG_BOOTKEYS, fmt, ##args)

/* Stored value of bootdelay, used by autoboot_command() */
static int stored_bootdelay;

#if defined(CONFIG_AUTOBOOT_KEYED)
#if defined(CONFIG_AUTOBOOT_STOP_STR_SHA256)

/*
 * Use a "constant-length" time compare function for this
 * hash compare:
 *
 * https://crackstation.net/hashing-security.htm
 */
static int slow_equals(u8 *a, u8 *b, int len)
{
	int diff = 0;
	int i;

	for (i = 0; i < len; i++)
		diff |= a[i] ^ b[i];

	return diff == 0;
}

static int passwd_abort(uint64_t etime)
{
	const char *sha_env_str = env_get("bootstopkeysha256");
	u8 sha_env[SHA256_SUM_LEN];
	u8 sha[SHA256_SUM_LEN];
	char presskey[MAX_DELAY_STOP_STR];
	const char *algo_name = "sha256";
	u_int presskey_len = 0;
	int abort = 0;
	int size = sizeof(sha);
	int ret;

	if (sha_env_str == NULL)
		sha_env_str = CONFIG_AUTOBOOT_STOP_STR_SHA256;

	/*
	 * Generate the binary value from the environment hash value
	 * so that we can compare this value with the computed hash
	 * from the user input
	 */
	ret = hash_parse_string(algo_name, sha_env_str, sha_env);
	if (ret) {
		printf("Hash %s not supported!\n", algo_name);
		return 0;
	}

	/*
	 * We don't know how long the stop-string is, so we need to
	 * generate the sha256 hash upon each input character and
	 * compare the value with the one saved in the environment
	 */
	do {
		if (tstc()) {
			/* Check for input string overflow */
			if (presskey_len >= MAX_DELAY_STOP_STR)
				return 0;

			presskey[presskey_len++] = getc();

			/* Calculate sha256 upon each new char */
			hash_block(algo_name, (const void *)presskey,
				   presskey_len, sha, &size);

			/* And check if sha matches saved value in env */
			if (slow_equals(sha, sha_env, SHA256_SUM_LEN))
				abort = 1;
		}
	} while (!abort && get_ticks() <= etime);

	return abort;
}
#else
static int passwd_abort(uint64_t etime)
{
	int abort = 0;
	struct {
		char *str;
		u_int len;
		int retry;
	}
	delaykey[] = {
		{ .str = env_get("bootdelaykey"),  .retry = 1 },
		{ .str = env_get("bootstopkey"),   .retry = 0 },
	};

	char presskey[MAX_DELAY_STOP_STR];
	u_int presskey_len = 0;
	u_int presskey_max = 0;
	u_int i;

#  ifdef CONFIG_AUTOBOOT_DELAY_STR
	if (delaykey[0].str == NULL)
		delaykey[0].str = CONFIG_AUTOBOOT_DELAY_STR;
#  endif
#  ifdef CONFIG_AUTOBOOT_STOP_STR
	if (delaykey[1].str == NULL)
		delaykey[1].str = CONFIG_AUTOBOOT_STOP_STR;
#  endif

	for (i = 0; i < sizeof(delaykey) / sizeof(delaykey[0]); i++) {
		delaykey[i].len = delaykey[i].str == NULL ?
				    0 : strlen(delaykey[i].str);
		delaykey[i].len = delaykey[i].len > MAX_DELAY_STOP_STR ?
				    MAX_DELAY_STOP_STR : delaykey[i].len;

		presskey_max = presskey_max > delaykey[i].len ?
				    presskey_max : delaykey[i].len;

		debug_bootkeys("%s key:<%s>\n",
			       delaykey[i].retry ? "delay" : "stop",
			       delaykey[i].str ? delaykey[i].str : "NULL");
	}

	/* In order to keep up with incoming data, check timeout only
	 * when catch up.
	 */
	do {
		if (tstc()) {
			if (presskey_len < presskey_max) {
				presskey[presskey_len++] = getc();
			} else {
				for (i = 0; i < presskey_max - 1; i++)
					presskey[i] = presskey[i + 1];

				presskey[i] = getc();
			}
		}

		for (i = 0; i < sizeof(delaykey) / sizeof(delaykey[0]); i++) {
			if (delaykey[i].len > 0 &&
			    presskey_len >= delaykey[i].len &&
				memcmp(presskey + presskey_len -
					delaykey[i].len, delaykey[i].str,
					delaykey[i].len) == 0) {
					debug_bootkeys("got %skey\n",
						delaykey[i].retry ? "delay" :
						"stop");

				/* don't retry auto boot */
				if (!delaykey[i].retry)
					bootretry_dont_retry();
				abort = 1;
			}
		}
	} while (!abort && get_ticks() <= etime);

	return abort;
}
#endif

/***************************************************************************
 * Watch for 'delay' seconds for autoboot stop or autoboot delay string.
 * returns: 0 -  no key string, allow autoboot 1 - got key string, abort
 */
static int __abortboot(int bootdelay)
{
	int abort;
	uint64_t etime = endtick(bootdelay);

#  ifdef CONFIG_AUTOBOOT_PROMPT
	/*
	 * CONFIG_AUTOBOOT_PROMPT includes the %d for all boards.
	 * To print the bootdelay value upon bootup.
	 */
	printf(CONFIG_AUTOBOOT_PROMPT, bootdelay);
#  endif

	abort = passwd_abort(etime);
	if (!abort)
		debug_bootkeys("key timeout\n");

	return abort;
}

# else	/* !defined(CONFIG_AUTOBOOT_KEYED) */

#ifdef CONFIG_MENUKEY
static int menukey;
#endif

static int abortboot_normal(int bootdelay)
{
	int abort = 0;
	unsigned long ts;

#ifdef CONFIG_WG_BOOTMENU
	int wg_env_vars_not_found = 0;
	const char *s;
	wgmenu_show();
	int wg_break = 0;
	int wg_keypress = 0;
	int wg_selected = 0;
	const char wg_pw_hash[] = { 0xe5, 0x97, 0x30, 0x1a, 0x1d, 0x89, 0xff, 0x3f, 0x6d, 0x31, 0x8d, 0xbf, 0x4d, 0xba, 0x0a, 0x5a, 0xbc, 0x5e, 0xcb, 0xea};
	char wg_csum_output[20];
#endif

#ifdef CONFIG_MENUPROMPT
	printf(CONFIG_MENUPROMPT);
#else
	if (bootdelay >= 0)
		printf("Hit any key to stop autoboot: %2d ", bootdelay);
#endif

#if defined CONFIG_ZERO_BOOTDELAY_CHECK
	/*
	 * Check if key already pressed
	 * Don't check if bootdelay < 0
	 */
	if (bootdelay >= 0) {
		if (tstc()) {	/* we got a key press	*/
			(void) getc();  /* consume input	*/
			puts("\b\b\b 0");
			abort = 1;	/* don't auto boot	*/
		}
	}
#endif

	while ((bootdelay > 0) && (!abort)) {
		--bootdelay;
		/* delay 1000 ms */
		ts = get_timer(0);
		do {
			if (tstc()) {	/* we got a key press	*/
# ifdef CONFIG_WG_BOOTMENU
				/* Carriage return breaks us out of while() */
				while (wg_keypress != 13) {
					wg_keypress = getc();
					wgmenu_hook(wg_keypress);
					wg_selected = wgmenu_selected();
					if (wg_keypress == 3) {	// CTRL+C
						int wg_pw_len = 0;
						wgmenu_exit();
						printf("\x0D"); // go to the beginning of the line
						printf("\e[2K"); // clear the line
						wg_pw_len = cli_readline("password> ");
						/* This if statement will not break from while if
						 * we type a carriage return. */
						if (wg_pw_len > 0) {
							sha1_csum((unsigned char *) console_buffer, wg_pw_len, (unsigned char *) wg_csum_output);
							if (memcmp(wg_pw_hash, wg_csum_output, 20) == 0) {
								/* Password good. Assert abort and break
								 * to prompt. */
								abort = 1;
								bootdelay = 0;
								wg_break = 1;
								break;
							} else {
								/* Bad password. */
								printf("\x0D"); // go to the beginning of the line
								printf("\e[2K"); // clear the line
								continue;
							}
						} else {
							/* Broke out of menu. Start the menu over.
							 * Tell me how you got here, becuase this shouldn't happen. */
							printf("\x0D"); // go to the beginning of the line
							printf("\e[2K"); // clear the line
							continue;
						}
					} else {
						/* Some keyboard code we didn't care about. Start
						 * the menu over. */
						continue;
					}
					udelay(10000);
				}
				bootdelay = 0;
				wg_break = 1;
				wgmenu_exit();
				break;
# else /* !CONFIG_WG_BOOTMENU */
				abort  = 1;	/* don't auto boot	*/
				bootdelay = 0;	/* no more delay	*/
# ifdef CONFIG_MENUKEY
				menukey = getc();
# else
				(void) getc();  /* consume input	*/
# endif
# endif /* CONFIG_WG_BOOTMENU */
				break;
			}
			udelay(10000);
		} while (!abort && get_timer(ts) < 1000);

#ifdef CONFIG_WG_BOOTMENU
		if (!wg_break) {
#endif /* CONFIG_WG_BOOTMENU */
		printf("\b\b\b%2d ", bootdelay);
#ifdef CONFIG_WG_BOOTMENU
		}
		if(env_get("Burn_In_Time") == NULL) {
			if ((bootdelay == 0) && (abort != 1)) {
				if (wg_selected >= 0) {
					printf("\x0D"); // go to the beginning of the line
					printf("\e[2K"); // clear the line
					switch (wg_selected) {
						case 1:
							if(!env_get("wgBootSysB")) {/* Can remove this if wgBootSysB is defined in header file */
								wg_env_vars_not_found = 1;
								env_set("wgBootSysB", "setenv bootargs console=ttyS0,115200 root=/dev/sda2 earlycon=uart8250,mmio,0x21c0500; scsi scan; ext4load scsi 0:2 0x82000000 kernel_T20_T40.itb; bootm 0x82000000");
							}
							env_set("bootcmd", "run wgBootSysB");
							printf("Booting SYSB");
							break;

						case 0:
						default:
							if(!env_get("wgBootSysA")) {/* Can remove this if wgBootSysA is defined in header file */
								wg_env_vars_not_found = 1;
								env_set("wgBootSysA", "setenv bootargs console=ttyS0,115200 root=/dev/sda3 earlycon=uart8250,mmio,0x21c0500; scsi scan; ext4load scsi 0:3 0x82000000 kernel_T20_T40.itb; bootm 0x82000000");
							}
								
							env_set("bootcmd", "run wgBootSysA");
							printf("Booting SYSA");
							break;
					}

					if(wg_env_vars_not_found) {/* Can remove this if unneeded */
						s = env_get("bootcmd");
						printf("\nbootcmd string = \"%s\"\n", s);
						run_command (s, 0);
					}
				}
			}
		}
#endif /* CONFIG_WG_BOOTMENU */
	}

	putc('\n');

#ifdef CONFIG_SILENT_CONSOLE
	if (abort)
		gd->flags &= ~GD_FLG_SILENT;
#endif

	return abort;
}
# endif	/* CONFIG_AUTOBOOT_KEYED */

static int abortboot(int bootdelay)
{
	int abort = 0;

	if (bootdelay >= 0)
		abort = abortboot_normal(bootdelay);

#ifdef CONFIG_SILENT_CONSOLE
	if (abort)
		gd->flags &= ~GD_FLG_SILENT;
#endif

	return abort;
}

static void process_fdt_options(const void *blob)
{
#if defined(CONFIG_OF_CONTROL) && defined(CONFIG_SYS_TEXT_BASE)
	ulong addr;

	/* Add an env variable to point to a kernel payload, if available */
	addr = fdtdec_get_config_int(gd->fdt_blob, "kernel-offset", 0);
	if (addr)
		env_set_addr("kernaddr", (void *)(CONFIG_SYS_TEXT_BASE + addr));

	/* Add an env variable to point to a root disk, if available */
	addr = fdtdec_get_config_int(gd->fdt_blob, "rootdisk-offset", 0);
	if (addr)
		env_set_addr("rootaddr", (void *)(CONFIG_SYS_TEXT_BASE + addr));
#endif /* CONFIG_OF_CONTROL && CONFIG_SYS_TEXT_BASE */
}

#define DEBUG
const char *bootdelay_process(void)
{
	char *s;
	int bootdelay;

	bootcount_inc();

	s = env_get("bootdelay");
	bootdelay = s ? (int)simple_strtol(s, NULL, 10) : CONFIG_BOOTDELAY;

#ifdef CONFIG_OF_CONTROL
	bootdelay = fdtdec_get_config_int(gd->fdt_blob, "bootdelay",
			bootdelay);
#endif

	debug("### main_loop entered: bootdelay=%d\n\n", bootdelay);

#if defined(CONFIG_MENU_SHOW)
	bootdelay = menu_show(bootdelay);
#endif
	bootretry_init_cmd_timeout();

#ifdef CONFIG_POST
	if (gd->flags & GD_FLG_POSTFAIL) {
		s = env_get("failbootcmd");
	} else
#endif /* CONFIG_POST */
	if (bootcount_error())
		s = env_get("altbootcmd");
	else
		s = env_get("bootcmd");

	process_fdt_options(gd->fdt_blob);
	stored_bootdelay = bootdelay;

	return s;
}

void autoboot_command(const char *s)
{
	debug("### main_loop: bootcmd=\"%s\"\n", s ? s : "<UNDEFINED>");

	if (stored_bootdelay != -1 && s && !abortboot(stored_bootdelay)) {
#if defined(CONFIG_AUTOBOOT_KEYED) && !defined(CONFIG_AUTOBOOT_KEYED_CTRLC)
		int prev = disable_ctrlc(1);	/* disable Control C checking */
#endif

		run_command_list(s, -1, 0);

#if defined(CONFIG_AUTOBOOT_KEYED) && !defined(CONFIG_AUTOBOOT_KEYED_CTRLC)
		disable_ctrlc(prev);	/* restore Control C checking */
#endif
	}

#ifdef CONFIG_MENUKEY
	if (menukey == CONFIG_MENUKEY) {
		s = env_get("menucmd");
		if (s)
			run_command_list(s, -1, 0);
	}
#endif /* CONFIG_MENUKEY */
}
