#include <common.h>
#include <mapmem.h>
#include <asm/gpio.h>

#define GPIO_BASE		0x2300000
#define GPIO_DIRECTION		0x0
#define GPIO_DATA               0x8

#define GPIO1_BASE		(GPIO_BASE + 0x00000)
#define GPIO1_DIRECTION         (GPIO1_BASE + GPIO_DIRECTION)
#define GPIO1_DATA              (GPIO1_BASE + GPIO_DATA)

#define GPIO2_BASE		(GPIO_BASE + 0x10000)
#define GPIO2_DIRECTION         (GPIO2_BASE + GPIO_DIRECTION)
#define GPIO2_DATA              (GPIO2_BASE + GPIO_DATA)

#define GPIO3_BASE		(GPIO_BASE + 0x20000)
#define GPIO3_DIRECTION         (GPIO3_BASE + GPIO_DIRECTION)
#define GPIO3_DATA              (GPIO3_BASE + GPIO_DATA)

#define GPIO4_BASE		(GPIO_BASE + 0x30000)
#define GPIO4_DIRECTION         (GPIO4_BASE + GPIO_DIRECTION)
#define GPIO4_DATA              (GPIO4_BASE + GPIO_DATA)

#define BYTE_SWAP(x)		(((x / 8) * 16 + 7) - x)

#ifndef GPIO1_SUPPORT
#define GPIO1_SUPPORT		(0x0 << 0)
#endif
#ifndef GPIO2_SUPPORT
#define GPIO2_SUPPORT		(0x0 << 0)
#endif
#ifndef GPIO3_SUPPORT
#define GPIO3_SUPPORT		(0x0 << 0)
#endif
#ifndef GPIO4_SUPPORT
#define GPIO4_SUPPORT		(0x0 << 0)
#endif

uint32_t gpio_direction[4] = {
	GPIO1_DIRECTION,
	GPIO2_DIRECTION,
	GPIO3_DIRECTION,
	GPIO4_DIRECTION,
};

uint32_t gpio_data[4] = {
	GPIO1_DATA,
	GPIO2_DATA,
	GPIO3_DATA,
	GPIO4_DATA,
};

uint32_t gpio_support[4] = {
	GPIO1_SUPPORT,
	GPIO2_SUPPORT,
	GPIO3_SUPPORT,
	GPIO4_SUPPORT,
};

static int is_support(unsigned gpio)
{
	int grp, pos, sup = 0;

	grp = (gpio/100);
	pos = (gpio%100);
	sup = ((gpio_support[grp-1] >> pos) & 0x1);

	return (!sup);
}

int gpio_request(unsigned gpio, const char *label)
{
	if (!is_support(gpio))
		return 0;

	return -1;
}

int gpio_free(unsigned gpio)
{
	return 0;
}

int gpio_direction_input(unsigned gpio)
{
	if (is_support(gpio))
		return -1;

	ulong bytes;
	uint32_t grp, pos, addr, val;

	grp = (gpio/100);
	pos = (BYTE_SWAP(gpio%100));
	addr = (gpio_direction[grp-1]);

	const void *buf = map_sysmem(addr, bytes);
	val = *(volatile uint32_t *)buf;
	val &= ~(1 << pos);
	*(volatile uint32_t *)buf = val;
	unmap_sysmem(buf);

	return 0;
}

int gpio_direction_output(unsigned gpio, int value)
{
	if (is_support(gpio))
		return -1;

	ulong bytes;
	uint32_t grp, pos, addr, val;

	grp = (gpio/100);
	pos = (BYTE_SWAP(gpio%100));
	addr = (gpio_direction[grp-1]);

	gpio_set_value(gpio, value);

	const void *buf = map_sysmem(addr, bytes);
	val = *(volatile uint32_t *)buf;
	val |= (1 << pos);
	*(volatile uint32_t *)buf = (val);
	unmap_sysmem(buf);

	return 0;
}

int gpio_get_value(unsigned gpio)
{
	if (is_support(gpio))
		return -1;

	ulong bytes;
	uint32_t grp, pos, addr, val;

	grp = (gpio/100);
	pos = (BYTE_SWAP(gpio%100));
	addr = (gpio_data[grp-1]);

	const void *buf = map_sysmem(addr, bytes);
	val = *(volatile uint32_t *)buf;
	unmap_sysmem(buf);

	return ((val >> pos) & 0x1);
}

int gpio_set_value(unsigned gpio, int value)
{
	if (is_support(gpio))
		return -1;

	ulong bytes;
	uint32_t grp, pos, addr, val;

	grp = (gpio/100);
	pos = (BYTE_SWAP(gpio%100));
	addr = (gpio_data[grp-1]);

	const void *buf = map_sysmem(addr, bytes);
	val = *(volatile uint32_t *)buf;
	if (value)
		val |= (1 << pos);
	else
		val &= ~(1 << pos);

	*(volatile uint32_t *)buf = (val);
	unmap_sysmem(buf);

	return 0;
}
