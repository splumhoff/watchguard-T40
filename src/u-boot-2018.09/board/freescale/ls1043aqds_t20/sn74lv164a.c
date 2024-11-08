#include <common.h>
#include "sn74lv164a.h"

#define SN74LV164A_CLR(v)	{ \
	gpio_direction_output(SN74LV164A_CLR_GPIO, v); \
}

#define SN74LV164A_CLK(v)	{ \
	gpio_direction_output(SN74LV164A_CLK_GPIO, v); \
}

#define SN74LV164A_INA(v)	{ \
	gpio_direction_output(SN74LV164A_INA_GPIO, v); \
}

int SN74LV164A_OUT_VAL[SN74LV164A_OUT_NUM] = { SN74LV164A_OUT_LOW };

void sn74lv164a_set(void) {
	for(int i = 0; i < SN74LV164A_OUT_NUM; i++) {
		SN74LV164A_INA(SN74LV164A_OUT_VAL[(SN74LV164A_OUT_NUM-(i+1))]);
		SN74LV164A_CLK(1);
		SN74LV164A_CLK(0);
	}
}

void sn74lv164a_clear(void)
{
	SN74LV164A_CLR(1); \
	SN74LV164A_CLR(0); \
	SN74LV164A_CLR(1); \
}

void sn74lv164a_set_value(int pin, int val)
{
	SN74LV164A_OUT_VAL[pin] = val;
	sn74lv164a_set();
}

void sn74lv164a_set_value_all_high(void)
{
	for(int i = 0; i < SN74LV164A_OUT_NUM; i++)
		SN74LV164A_OUT_VAL[i] = SN74LV164A_OUT_HIGH;
	sn74lv164a_set();
}

void sn74lv164a_set_value_all_low(void)
{
	for(int i = 0; i < SN74LV164A_OUT_NUM; i++)
		SN74LV164A_OUT_VAL[i] = SN74LV164A_OUT_LOW;
	sn74lv164a_set();
}

static int do_74lv164(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int __maybe_unused pin, val;

	if (argc == 2) {
		if (!strcmp(argv[1], "clear")) {
			sn74lv164a_clear();
		} else if (!strcmp(argv[1], "high")) {
			sn74lv164a_set_value_all_high();
		} else if (!strcmp(argv[1], "low")) {
			sn74lv164a_set_value_all_low();
		} else {
			return CMD_RET_USAGE;
		}
	} else if (argc == 3) {
		if (!strcmp(argv[1], "qa")) {
			pin = SN74LV164A_OUT_QA;
		} else if (!strcmp(argv[1], "qb")) {
			pin = SN74LV164A_OUT_QB;
		} else if (!strcmp(argv[1], "qc")) {
			pin = SN74LV164A_OUT_QC;
		} else if (!strcmp(argv[1], "qd")) {
			pin = SN74LV164A_OUT_QD;
		} else if (!strcmp(argv[1], "qe")) {
			pin = SN74LV164A_OUT_QE;
		} else if (!strcmp(argv[1], "qf")) {
			pin = SN74LV164A_OUT_QF;
		} else if (!strcmp(argv[1], "qg")) {
			pin = SN74LV164A_OUT_QG;
		} else if (!strcmp(argv[1], "qh")) {
			pin = SN74LV164A_OUT_QH;
		} else {
			return CMD_RET_USAGE;
		}
		val = simple_strtoul(argv[2], NULL, 10);
		sn74lv164a_set_value(pin, val);
	} else {
		return CMD_RET_USAGE;
	}

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(74lv164, 3, 0, do_74lv164,
	   "74lv164 control",
	   "\n"
	   "    clear/high/low\n"
	   "    [<qa|qb|qc|qd|qe|qf|qg|qh>  <0|1>] \n"
	   "\n"
	   );
