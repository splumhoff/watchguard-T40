/*
 * Control GPIO pins on the fly
 *
 * Copyright (c) 2008-2011 Analog Devices Inc.
 *
 * Licensed under the GPL-2 or later.
 */

#include <common.h>
#include <command.h>

#ifdef CONFIG_WG1008_VM1P
extern GPIO_t wg1008_vm1p_gpio[];
extern LED_t wg1008_vm1p_led[];
extern int gpioWrite(u32 gpio_bus, u32 gpio_shift, u32 gpio_type, u32 val);
extern int gpioRead(u32 gpio_bus, u32 gpio_shift, u32 gpio_type, u32 *val);
extern int wg1008_vm1p_gpio_init(void);
extern int getGpioNumByLedName(char *ledName, u32 *gpioNum, u32 *active_high);
extern int setGpioLedAllOnOff(u32 mode);

void printHelp(char *daemonName)
{
    int idx =0;

    printf("%s init\n", daemonName);
    printf("%s read <gpio bus> <gpio shift>\n", daemonName);
    printf("%s write <gpio bus> <gpio shift> <value>\n", daemonName);
    printf("%s dir <gpio bus> <gpio shift> <value>\n", daemonName);
    printf("%s led <led name> <on/off>\n", daemonName);
    printf("%s led all <on/off/normal>\n", daemonName);
    printf("%s led list\n", daemonName);
    printf("Supported <led name>: ");
    for(idx =0; wg1008_vm1p_led[idx].led_name != NULL; idx++)
    {
        printf("%s ", wg1008_vm1p_led[idx].led_name);
    }
    printf("\n");
}

int do_gpio(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    int ret = 0, idx =0;
    u32 gpio_bus = 0, gpio_shift = 0, val = 0, gpioNum = 0, active_high = 0;

    if(argc < 2)
    {
        printHelp(argv[0]);
        return CMD_RET_FAILURE;
    }

    if(argc == 2 && !strncmp(argv[1], "init", strlen("init")))
    {
        return wg1008_vm1p_gpio_init();
    }

    if(argc >= 4)
    {
        gpio_bus = strtoul(argv[2], NULL, 0);
        gpio_shift = strtoul(argv[3], NULL, 0);
        if(!strncmp(argv[1], "read", strlen("read")))
        {
            ret = gpioRead(gpio_bus, gpio_shift, GPIO_TYPE_DATA, &val);
            if(ret == 0)
            {
                printf("%u\n", val);
            }
            return ret;
        }
        else if(argc == 5 && !strncmp(argv[1], "write", strlen("write")))
        {
            val = strtoul(argv[4], NULL, 0);
            ret = gpioWrite(gpio_bus, gpio_shift, GPIO_TYPE_DATA, val);
            if(ret == 0)
            {
                printf("%u written.\n", val);
            }
            return ret;
        }
        else if(argc == 5 && !strncmp(argv[1], "dir", strlen("dir")))
        {
            val = strtoul(argv[4], NULL, 0);
            ret = gpioWrite(gpio_bus, gpio_shift, GPIO_TYPE_DIR, val);
            if(ret == 0)
            {
                printf("%u written.\n", val);
            }
            return ret;
        }
        else if(argc == 4 && !strncmp(argv[1], "dir", strlen("dir")))
        {
            ret = gpioRead(gpio_bus, gpio_shift, GPIO_TYPE_DIR, &val);
            if(ret == 0)
            {
                printf("%u\n", val);
            }
            return ret;
        }
        else if(argc == 4 && !strncmp(argv[1], "led", strlen("led")))
        {
            if(!strncmp(argv[2], "all", strlen("all")))
            {
                if(!strncmp(argv[3], "on", strlen("on")))
                {
                    if(setGpioLedAllOnOff(GPIO_LED_MODE_ON))
                    {
                        printHelp(argv[0]);
                        return CMD_RET_FAILURE;
                    }
                    else
                    {
                        return 0;
                    }
                }
                else if(!strncmp(argv[3], "off", strlen("off")))
                {
                    if(setGpioLedAllOnOff(GPIO_LED_MODE_OFF))
                    {
                        printHelp(argv[0]);
                        return CMD_RET_FAILURE;
                    }
                    else
                    {
                        return 0;
                    }
                }
                else if(!strncmp(argv[3], "normal", strlen("normal")))
                {
                    if(setGpioLedAllOnOff(GPIO_LED_MODE_NORMAL))
                    {
                        printHelp(argv[0]);
                        return CMD_RET_FAILURE;
                    }
                    else
                    {
                        return 0;
                    }
                }
                else
                {
                    printf("Invalid arguments!!!\n");
                    printHelp(argv[0]);
                    return CMD_RET_FAILURE;
                }
            }
            ret = getGpioNumByLedName(argv[2], &gpioNum, &active_high);
            if(!ret)
            {
                if(!strncmp(argv[3], "on", strlen("on")))
                {
                    val = active_high?1:0;
                }
                else if(!strncmp(argv[3], "off", strlen("off")))
                {
                    val = active_high?0:1;
                }
                else
                {
                    printf("Invalid arguments!!!\n");
                    printHelp(argv[0]);
                    return CMD_RET_FAILURE;
                }
                gpio_bus = wg1008_vm1p_gpio[gpioNum].gpio_bus;
                gpio_shift = wg1008_vm1p_gpio[gpioNum].gpio_shift;
                if(gpioWrite(gpio_bus, gpio_shift, GPIO_TYPE_DATA, val))
                {
                    printHelp(argv[0]);
                    return CMD_RET_FAILURE;
                }
            }
        }
        else
        {
            printf("Invalid arguments!!!\n");
            printHelp(argv[0]);
            return CMD_RET_FAILURE;
        }
    }
    else if(argc == 3 && !strncmp(argv[1], "led", strlen("led")))
    {
            if(!strncmp(argv[2], "list", strlen("list")))
            {
                for(idx =0; wg1008_vm1p_led[idx].led_name != NULL; idx++)
                {
                    printf("%s ", wg1008_vm1p_led[idx].led_name);
                }
                return 0;
            }
            else
            {
                printf("Invalid arguments!!!\n");
                printHelp(argv[0]);
                return CMD_RET_FAILURE;
            }
    }
    else
    {
        printf("Invalid arguments!!!\n");
        printHelp(argv[0]);
        return CMD_RET_FAILURE;
    }

    return 0;
}


U_BOOT_CMD(
	gpio, 5, 1, do_gpio,
	"manage GPIOs",
	"gpio init\n"
	"gpio read <gpio bus> <gpio shift>\n"
	"gpio write <gpio bus> <gpio shift> <value>\n"
	"gpio dir <gpio bus> <gpio shift> <value>\n"
	"gpio led <led name> <on/off>\n"
	"gpio led all <on/off/normal>\n"
	"gpio led list\t\tshow a list of LEDs"
);
#else
#include <errno.h>
#include <dm.h>
#include <asm/gpio.h>

__weak int name_to_gpio(const char *name)
{
	return simple_strtoul(name, NULL, 10);
}

enum gpio_cmd {
	GPIO_INPUT,
	GPIO_SET,
	GPIO_CLEAR,
	GPIO_TOGGLE,
};

#if defined(CONFIG_DM_GPIO) && !defined(gpio_status)

/* A few flags used by show_gpio() */
enum {
	FLAG_SHOW_ALL		= 1 << 0,
	FLAG_SHOW_BANK		= 1 << 1,
	FLAG_SHOW_NEWLINE	= 1 << 2,
};

static void gpio_get_description(struct udevice *dev, const char *bank_name,
				 int offset, int *flagsp)
{
	char buf[80];
	int ret;

	ret = gpio_get_function(dev, offset, NULL);
	if (ret < 0)
		goto err;
	if (!(*flagsp & FLAG_SHOW_ALL) && ret == GPIOF_UNUSED)
		return;
	if ((*flagsp & FLAG_SHOW_BANK) && bank_name) {
		if (*flagsp & FLAG_SHOW_NEWLINE) {
			putc('\n');
			*flagsp &= ~FLAG_SHOW_NEWLINE;
		}
		printf("Bank %s:\n", bank_name);
		*flagsp &= ~FLAG_SHOW_BANK;
	}

	ret = gpio_get_status(dev, offset, buf, sizeof(buf));
	if (ret)
		goto err;

	printf("%s\n", buf);
	return;
err:
	printf("Error %d\n", ret);
}

static int do_gpio_status(bool all, const char *gpio_name)
{
	struct udevice *dev;
	int banklen;
	int flags;
	int ret;

	flags = 0;
	if (gpio_name && !*gpio_name)
		gpio_name = NULL;
	for (ret = uclass_first_device(UCLASS_GPIO, &dev);
	     dev;
	     ret = uclass_next_device(&dev)) {
		const char *bank_name;
		int num_bits;

		flags |= FLAG_SHOW_BANK;
		if (all)
			flags |= FLAG_SHOW_ALL;
		bank_name = gpio_get_bank_info(dev, &num_bits);
		if (!num_bits) {
			debug("GPIO device %s has no bits\n", dev->name);
			continue;
		}
		banklen = bank_name ? strlen(bank_name) : 0;

		if (!gpio_name || !bank_name ||
		    !strncmp(gpio_name, bank_name, banklen)) {
			const char *p = NULL;
			int offset;

			p = gpio_name + banklen;
			if (gpio_name && *p) {
				offset = simple_strtoul(p, NULL, 10);
				gpio_get_description(dev, bank_name, offset,
						     &flags);
			} else {
				for (offset = 0; offset < num_bits; offset++) {
					gpio_get_description(dev, bank_name,
							     offset, &flags);
				}
			}
		}
		/* Add a newline between bank names */
		if (!(flags & FLAG_SHOW_BANK))
			flags |= FLAG_SHOW_NEWLINE;
	}

	return ret;
}
#endif

static int do_gpio(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned int gpio;
	enum gpio_cmd sub_cmd;
	int value;
	const char *str_cmd, *str_gpio = NULL;
	int ret;
#ifdef CONFIG_DM_GPIO
	bool all = false;
#endif

	if (argc < 2)
 show_usage:
		return CMD_RET_USAGE;
	str_cmd = argv[1];
	argc -= 2;
	argv += 2;
#ifdef CONFIG_DM_GPIO
	if (argc > 0 && !strcmp(*argv, "-a")) {
		all = true;
		argc--;
		argv++;
	}
#endif
	if (argc > 0)
		str_gpio = *argv;
	if (!strncmp(str_cmd, "status", 2)) {
		/* Support deprecated gpio_status() */
#ifdef gpio_status
		gpio_status();
		return 0;
#elif defined(CONFIG_DM_GPIO)
		return cmd_process_error(cmdtp, do_gpio_status(all, str_gpio));
#else
		goto show_usage;
#endif
	}

	if (!str_gpio)
		goto show_usage;

	/* parse the behavior */
	switch (*str_cmd) {
		case 'i': sub_cmd = GPIO_INPUT;  break;
		case 's': sub_cmd = GPIO_SET;    break;
		case 'c': sub_cmd = GPIO_CLEAR;  break;
		case 't': sub_cmd = GPIO_TOGGLE; break;
		default:  goto show_usage;
	}

#if defined(CONFIG_DM_GPIO)
	/*
	 * TODO(sjg@chromium.org): For now we must fit into the existing GPIO
	 * framework, so we look up the name here and convert it to a GPIO number.
	 * Once all GPIO drivers are converted to driver model, we can change the
	 * code here to use the GPIO uclass interface instead of the numbered
	 * GPIO compatibility layer.
	 */
	ret = gpio_lookup_name(str_gpio, NULL, NULL, &gpio);
	if (ret) {
		printf("GPIO: '%s' not found\n", str_gpio);
		return cmd_process_error(cmdtp, ret);
	}
#else
	/* turn the gpio name into a gpio number */
	gpio = name_to_gpio(str_gpio);
	if (gpio < 0)
		goto show_usage;
#endif
	/* grab the pin before we tweak it */
	ret = gpio_request(gpio, "cmd_gpio");
	if (ret && ret != -EBUSY) {
		printf("gpio: requesting pin %u failed\n", gpio);
		return -1;
	}

	/* finally, let's do it: set direction and exec command */
	if (sub_cmd == GPIO_INPUT) {
		gpio_direction_input(gpio);
		value = gpio_get_value(gpio);
	} else {
		switch (sub_cmd) {
		case GPIO_SET:
			value = 1;
			break;
		case GPIO_CLEAR:
			value = 0;
			break;
		case GPIO_TOGGLE:
			value = gpio_get_value(gpio);
			if (!IS_ERR_VALUE(value))
				value = !value;
			break;
		default:
			goto show_usage;
		}
		gpio_direction_output(gpio, value);
	}
	printf("gpio: pin %s (gpio %i) value is ", str_gpio, gpio);
	if (IS_ERR_VALUE(value))
		printf("unknown (ret=%d)\n", value);
	else
		printf("%d\n", value);
	if (sub_cmd != GPIO_INPUT && !IS_ERR_VALUE(value)) {
		int nval = gpio_get_value(gpio);

		if (IS_ERR_VALUE(nval))
			printf("   Warning: no access to GPIO output value\n");
		else if (nval != value)
			printf("   Warning: value of pin is still %d\n", nval);
	}

	if (ret != -EBUSY)
		gpio_free(gpio);

	return value;
}

U_BOOT_CMD(gpio, 4, 0, do_gpio,
	   "query and control gpio pins",
	   "<input|set|clear|toggle> <pin>\n"
	   "    - input/set/clear/toggle the specified pin\n"
	   "gpio status [-a] [<bank> | <pin>]  - show [all/claimed] GPIOs");
#endif
