#include <common.h>
#include "sn74lv164a.h"

enum {
	LED_FAIL = SN74LV164A_OUT_QA,
	LED_ATTN = SN74LV164A_OUT_QB,
        LED_STAT = SN74LV164A_OUT_QC,
        LED_MODE = SN74LV164A_OUT_QD,
        LED_WAP0 = SN74LV164A_OUT_QE,
        LED_WAP1 = SN74LV164A_OUT_QF,
	LED_MAX = LED_WAP1,
};

#define LED_ON		0
#define LED_OFF		1

static int do_led(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	if (argc != 2)
		return CMD_RET_USAGE;

	if (!strcmp(argv[1],"on")) {
		sn74lv164a_set_value_all_low();
	} else if (!strcmp(argv[1],"off")) {
		sn74lv164a_set_value_all_high();
	} else if (!strcmp(argv[1],"reset")) {
		sn74lv164a_clear();
	} else if (!strcmp(argv[1],"fail")) {
		sn74lv164a_set_value(LED_FAIL, LED_ON);
	} else if (!strcmp(argv[1],"attn")) {
		sn74lv164a_set_value(LED_ATTN, LED_ON);
	} else if (!strcmp(argv[1],"stat")) {
		sn74lv164a_set_value(LED_STAT, LED_ON);
	} else if (!strcmp(argv[1],"mode")) {
		sn74lv164a_set_value(LED_MODE, LED_ON);
	} else if (!strcmp(argv[1],"2g")) {
		sn74lv164a_set_value(LED_WAP1, LED_OFF);
		sn74lv164a_set_value(LED_WAP0, LED_ON);
	} else if (!strcmp(argv[1],"5g")) {
		sn74lv164a_set_value(LED_WAP1, LED_ON);
		sn74lv164a_set_value(LED_WAP0, LED_OFF);
	} else {
		return CMD_RET_USAGE;
	}

	return 0;
}

U_BOOT_CMD(led, 2, 0, do_led,
	   "LED control",
	   "\n"
	   "\tled on    - Turn on all LEDs, except 2g/5g.\n"
	   "\tled off   - Turn off all LEDs.\n"
	   "\tled reset - Clear 74LV174A buffer\n"
	   "\tled fail\n"
	   "\tled attn \n"
	   "\tled stat \n"
	   "\tled mode \n"
	   "\tled 2g \n"
	   "\tled 5g \n"
	   );
