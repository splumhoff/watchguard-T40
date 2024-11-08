#ifndef _74LV164A_H_
#define _74LV164A_H_
#include <asm/gpio.h>

#ifndef SN74LV164A_CLR_GPIO
#error "GPIO pin for SN74LV164A_CLR_GPIO must be defined."
#endif
#ifndef SN74LV164A_CLK_GPIO
#error "GPIO pin for SN74LV164A_CLK_GPIO must be defined."
#endif
#ifndef SN74LV164A_INA_GPIO
#error "GPIO pin for SN74LV164A_INA_GPIO must be defined."
#endif

enum {
	SN74LV164A_OUT_QA = 0,
	SN74LV164A_OUT_QB,
	SN74LV164A_OUT_QC,
	SN74LV164A_OUT_QD,
	SN74LV164A_OUT_QE,
	SN74LV164A_OUT_QF,
	SN74LV164A_OUT_QG,
	SN74LV164A_OUT_QH,
	SN74LV164A_OUT_NUM,
	SN74LV164A_OUT_MAX = SN74LV164A_OUT_NUM,
};

#define SN74LV164A_OUT_HIGH	1
#define SN74LV164A_OUT_LOW	0

void sn74lv164a_clear(void);
void sn74lv164a_set_value(int pin, int val);
void sn74lv164a_set_value_all_high(void);
void sn74lv164a_set_value_all_low(void);

#endif /* _74LV164A_H_ */
