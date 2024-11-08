/*
 * Driver for Aquantia PHY
 *
 * Author: Shaohui Xie <Shaohui.Xie@freescale.com>
 *
 * Copyright 2015 Freescale Semiconductor, Inc.
 * Copyright 2018 NXP
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2.  This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/mii.h>
#include <linux/ethtool.h>
#include <linux/phy.h>
#include <linux/mdio.h>
#if defined(CONFIG_WG_PLATFORM_M590_M690)
#include <linux/device.h>
#include <linux/ctype.h>
#include <linux/hwmon.h>

#include "aquantia.h"
#endif

#define PHY_ID_AQ1202	0x03a1b445
#define PHY_ID_AQ2104	0x03a1b460
#define PHY_ID_AQR105	0x03a1b4a2
#define PHY_ID_AQR106	0x03a1b4d0
#define PHY_ID_AQR107	0x03a1b4e0
#define PHY_ID_AQR405	0x03a1b4b0
#define PHY_ID_AQR112	0x03a1b662
#define PHY_ID_AQR412	0x03a1b712
#if defined(CONFIG_WG_PLATFORM_M590_M690)
#define PHY_ID_AQR113C	0x31c31c12
#endif

#if defined(CONFIG_WG_PLATFORM_M590_M690)
#define PHY_AQUANTIA_FEATURES	(SUPPORTED_10000baseT_Full | \
				 SUPPORTED_1000baseT_Full | \
				 SUPPORTED_2500baseX_Full | \
				 SUPPORTED_100baseT_Full | \
				 SUPPORTED_10baseT_Full | \
				 PHY_DEFAULT_FEATURES)
#else
#define PHY_AQUANTIA_FEATURES	(SUPPORTED_10000baseT_Full | \
				 SUPPORTED_1000baseT_Full | \
				 SUPPORTED_2500baseX_Full | \
				 SUPPORTED_100baseT_Full | \
				 PHY_DEFAULT_FEATURES)
#endif

#define MDIO_PMA_CTRL1_AQ_SPEED10	0
#define MDIO_PMA_CTRL1_AQ_SPEED2500	0x2058
#define MDIO_PMA_CTRL1_AQ_SPEED5000	0x205c
#define MDIO_PMA_CTRL2_AQ_2500BT       0x30
#define MDIO_PMA_CTRL2_AQ_5000BT       0x31
#define MDIO_PMA_CTRL2_AQ_TYPE_MASK    0x3F

#define MDIO_AN_VENDOR_PROV_CTRL       0xc400
#define MDIO_AN_RECV_LP_STATUS         0xe820

/* registers in MDIO_MMD_VEND1 region */
#define AQUANTIA_VND1_GLOBAL_SC			0x000
#define  AQUANTIA_VND1_GLOBAL_SC_LP		BIT(0xb)

/* global start rate, the protocol associated with this speed is used by default
 * on SI.
 */
#define AQUANTIA_VND1_GSTART_RATE		0x31a
#define  AQUANTIA_VND1_GSTART_RATE_OFF		0
#define  AQUANTIA_VND1_GSTART_RATE_100M		1
#define  AQUANTIA_VND1_GSTART_RATE_1G		2
#define  AQUANTIA_VND1_GSTART_RATE_10G		3
#define  AQUANTIA_VND1_GSTART_RATE_2_5G		4
#define  AQUANTIA_VND1_GSTART_RATE_5G		5

/* SYSCFG registers for 100M, 1G, 2.5G, 5G, 10G */
#define AQUANTIA_VND1_GSYSCFG_BASE		0x31b
#define AQUANTIA_VND1_GSYSCFG_100M		0
#define AQUANTIA_VND1_GSYSCFG_1G		1
#define AQUANTIA_VND1_GSYSCFG_2_5G		2
#define AQUANTIA_VND1_GSYSCFG_5G		3
#define AQUANTIA_VND1_GSYSCFG_10G		4

#define MDIO_PHYXS_VEND_PROV2			0xC441
#define MDIO_PHYXS_VEND_PROV2_USX_AN		BIT(3)

#if defined(CONFIG_WG_PLATFORM_M590_M690)
/* Vendor specific 1, MDIO_MMD_VEND2 */
#define VEND1_THERMAL_PROV_HIGH_TEMP_FAIL	0xc421
#define VEND1_THERMAL_PROV_LOW_TEMP_FAIL	0xc422
#define VEND1_THERMAL_PROV_HIGH_TEMP_WARN	0xc423
#define VEND1_THERMAL_PROV_LOW_TEMP_WARN	0xc424
#define VEND1_THERMAL_STAT1			0xc820
#define VEND1_THERMAL_STAT2			0xc821
#define VEND1_THERMAL_STAT2_VALID		BIT(0)
#define VEND1_GENERAL_STAT1			0xc830
#define VEND1_GENERAL_STAT1_HIGH_TEMP_FAIL	BIT(14)
#define VEND1_GENERAL_STAT1_LOW_TEMP_FAIL	BIT(13)
#define VEND1_GENERAL_STAT1_HIGH_TEMP_WARN	BIT(12)
#define VEND1_GENERAL_STAT1_LOW_TEMP_WARN	BIT(11)

#define PHYXS_VENDOR_PROVISIONING2	0xe411
#define PHYXS_VENDOR_PROVISIONING3	0xe412
#define PHYXS_VENDOR_PROVISIONING4	0xe413

#ifdef CONFIG_HWMON
static umode_t aqr_hwmon_is_visible(const void *data,
				    enum hwmon_sensor_types type,
				    u32 attr, int channel)
{
	if (type != hwmon_temp)
		return 0;

	switch (attr) {
	case hwmon_temp_input:
	case hwmon_temp_min_alarm:
	case hwmon_temp_max_alarm:
	case hwmon_temp_lcrit_alarm:
	case hwmon_temp_crit_alarm:
		return 0444;
	case hwmon_temp_min:
	case hwmon_temp_max:
	case hwmon_temp_lcrit:
	case hwmon_temp_crit:
		return 0644;
	default:
		return 0;
	}
}

static int aqr_hwmon_get(struct phy_device *phydev, int reg, long *value)
{
	int temp = phy_read_mmd(phydev, MDIO_MMD_VEND1, reg);

	if (temp < 0)
		return temp;

	/* 16 bit value is 2's complement with LSB = 1/256th degree Celsius */
	*value = (s16)temp * 1000 / 256;

	return 0;
}

static int aqr_hwmon_set(struct phy_device *phydev, int reg, long value)
{
	int temp;

	if (value >= 128000 || value < -128000)
		return -ERANGE;

	temp = value * 256 / 1000;

	/* temp is in s16 range and we're interested in lower 16 bits only */
	return phy_write_mmd(phydev, MDIO_MMD_VEND1, reg, (u16)temp);
}

static int aqr_hwmon_test_bit(struct phy_device *phydev, int reg, int bit)
{
	int val = phy_read_mmd(phydev, MDIO_MMD_VEND1, reg);

	if (val < 0)
		return val;

	return !!(val & bit);
}

static int aqr_hwmon_status1(struct phy_device *phydev, int bit, long *value)
{
	int val = aqr_hwmon_test_bit(phydev, VEND1_GENERAL_STAT1, bit);

	if (val < 0)
		return val;

	*value = val;

	return 0;
}

static int aqr_hwmon_read(struct device *dev, enum hwmon_sensor_types type,
			  u32 attr, int channel, long *value)
{
	struct phy_device *phydev = dev_get_drvdata(dev);
	int reg;

	if (type != hwmon_temp)
		return -EOPNOTSUPP;

	switch (attr) {
	case hwmon_temp_input:
		reg = aqr_hwmon_test_bit(phydev, VEND1_THERMAL_STAT2,
					 VEND1_THERMAL_STAT2_VALID);
		if (reg < 0)
			return reg;
		if (!reg)
			return -EBUSY;

		return aqr_hwmon_get(phydev, VEND1_THERMAL_STAT1, value);

	case hwmon_temp_lcrit:
		return aqr_hwmon_get(phydev, VEND1_THERMAL_PROV_LOW_TEMP_FAIL,
				     value);
	case hwmon_temp_min:
		return aqr_hwmon_get(phydev, VEND1_THERMAL_PROV_LOW_TEMP_WARN,
				     value);
	case hwmon_temp_max:
		return aqr_hwmon_get(phydev, VEND1_THERMAL_PROV_HIGH_TEMP_WARN,
				     value);
	case hwmon_temp_crit:
		return aqr_hwmon_get(phydev, VEND1_THERMAL_PROV_HIGH_TEMP_FAIL,
				     value);
	case hwmon_temp_lcrit_alarm:
		return aqr_hwmon_status1(phydev,
					 VEND1_GENERAL_STAT1_LOW_TEMP_FAIL,
					 value);
	case hwmon_temp_min_alarm:
		return aqr_hwmon_status1(phydev,
					 VEND1_GENERAL_STAT1_LOW_TEMP_WARN,
					 value);
	case hwmon_temp_max_alarm:
		return aqr_hwmon_status1(phydev,
					 VEND1_GENERAL_STAT1_HIGH_TEMP_WARN,
					 value);
	case hwmon_temp_crit_alarm:
		return aqr_hwmon_status1(phydev,
					 VEND1_GENERAL_STAT1_HIGH_TEMP_FAIL,
					 value);
	default:
		return -EOPNOTSUPP;
	}
}

static int aqr_hwmon_write(struct device *dev, enum hwmon_sensor_types type,
			   u32 attr, int channel, long value)
{
	struct phy_device *phydev = dev_get_drvdata(dev);

	if (type != hwmon_temp)
		return -EOPNOTSUPP;

	switch (attr) {
	case hwmon_temp_lcrit:
		return aqr_hwmon_set(phydev, VEND1_THERMAL_PROV_LOW_TEMP_FAIL,
				     value);
	case hwmon_temp_min:
		return aqr_hwmon_set(phydev, VEND1_THERMAL_PROV_LOW_TEMP_WARN,
				     value);
	case hwmon_temp_max:
		return aqr_hwmon_set(phydev, VEND1_THERMAL_PROV_HIGH_TEMP_WARN,
				     value);
	case hwmon_temp_crit:
		return aqr_hwmon_set(phydev, VEND1_THERMAL_PROV_HIGH_TEMP_FAIL,
				     value);
	default:
		return -EOPNOTSUPP;
	}
}

static const struct hwmon_ops aqr_hwmon_ops = {
	.is_visible = aqr_hwmon_is_visible,
	.read = aqr_hwmon_read,
	.write = aqr_hwmon_write,
};

static u32 aqr_hwmon_chip_config[] = {
	HWMON_C_REGISTER_TZ,
	0,
};

static const struct hwmon_channel_info aqr_hwmon_chip = {
	.type = hwmon_chip,
	.config = aqr_hwmon_chip_config,
};

static u32 aqr_hwmon_temp_config[] = {
	HWMON_T_INPUT |
	HWMON_T_MAX | HWMON_T_MIN |
	HWMON_T_MAX_ALARM | HWMON_T_MIN_ALARM |
	HWMON_T_CRIT | HWMON_T_LCRIT |
	HWMON_T_CRIT_ALARM,
	0,
};

static const struct hwmon_channel_info aqr_hwmon_temp = {
	.type = hwmon_temp,
	.config = aqr_hwmon_temp_config,
};

static const struct hwmon_channel_info *aqr_hwmon_info[] = {
	&aqr_hwmon_chip,
	&aqr_hwmon_temp,
	NULL,
};

static const struct hwmon_chip_info aqr_hwmon_chip_info = {
	.ops = &aqr_hwmon_ops,
	.info = aqr_hwmon_info,
};

int aqr_hwmon_probe(struct phy_device *phydev)
{
	struct device *dev = &phydev->mdio.dev;
	struct device *hwmon_dev;
	char *hwmon_name;
	int i, j;

	hwmon_name = devm_kstrdup(dev, dev_name(dev), GFP_KERNEL);
	if (!hwmon_name)
		return -ENOMEM;

	for (i = j = 0; hwmon_name[i]; i++) {
		if (isalnum(hwmon_name[i])) {
			if (i != j)
				hwmon_name[j] = hwmon_name[i];
			j++;
		}
	}
	hwmon_name[j] = '\0';

	// ToDo: remove redudan code for setting up hwmon_name[]? Current
	// patch comes from LiteOn
	hwmon_dev = devm_hwmon_device_register_with_info(dev, "AQR113C",
					phydev, &aqr_hwmon_chip_info, NULL);

	return PTR_ERR_OR_ZERO(hwmon_dev);
}

#endif

int aquantia_pre_emphasis_config(struct phy_device *phydev)
{
	u16 reg1, reg2, reg3;
	int retry;

	for(retry=0; retry < 5; retry++)
	{
		mdelay(10);
		reg3 = phy_read_mmd(phydev, 4, 0xe411);
		printk("%s PROVISIONING2 0x%04x\n", "AQR113C", reg3);
		reg1 = phy_read_mmd(phydev, 4, 0xe412);
		printk("%s PROVISIONING3 0x%04x\n", "AQR113C", reg1);
		reg2 = phy_read_mmd(phydev, 4, 0xe413);
		printk("%s PROVISIONING4 0x%04x\n", "AQR113C", reg2);
		if ((reg1 == 0x400) && (reg2 == 0xa5) && (reg3 == 0))
			return 0;

		phy_write_mmd(phydev, 4, 0xe412, 0x400);
		phy_write_mmd(phydev, 4, 0xe413, 0xa5);
		phy_write_mmd(phydev, 4, 0xe411, 0x200);
		mdelay(10);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(aquantia_pre_emphasis_config);

#endif // CONFIG_WG_PLATFORM_M590_M690

static int aquantia_write_reg(struct phy_device *phydev, int devad,
			      u32 regnum, u16 val)
{
	u32 addr = MII_ADDR_C45 | (devad << 16) | (regnum & 0xffff);

	return mdiobus_write(phydev->mdio.bus, phydev->mdio.addr, addr, val);
}

static int aquantia_read_reg(struct phy_device *phydev, int devad, u32 regnum)
{
	u32 addr = MII_ADDR_C45 | (devad << 16) | (regnum & 0xffff);

	return mdiobus_read(phydev->mdio.bus, phydev->mdio.addr, addr);
}

static int aquantia_pma_setup_forced(struct phy_device *phydev)
{
	int ctrl1, ctrl2, ret;

	/* Half duplex is not supported */
	if (phydev->duplex != DUPLEX_FULL)
		return -EINVAL;

	ctrl1 = aquantia_read_reg(phydev, MDIO_MMD_PMAPMD, MDIO_CTRL1);
	if (ctrl1 < 0)
		return ctrl1;

	ctrl2 = aquantia_read_reg(phydev, MDIO_MMD_PMAPMD, MDIO_CTRL2);
	if (ctrl2 < 0)
		return ctrl2;

	ctrl1 &= ~MDIO_CTRL1_SPEEDSEL;
	ctrl2 &= ~(MDIO_PMA_CTRL2_AQ_TYPE_MASK);

	switch (phydev->speed) {
	case SPEED_10:
		ctrl2 |= MDIO_PMA_CTRL2_10BT;
		break;
	case SPEED_100:
		ctrl1 |= MDIO_PMA_CTRL1_SPEED100;
		ctrl2 |= MDIO_PMA_CTRL2_100BTX;
		break;
	case SPEED_1000:
		ctrl1 |= MDIO_PMA_CTRL1_SPEED1000;
		/* Assume 1000base-T */
		ctrl2 |= MDIO_PMA_CTRL2_1000BT;
		break;
	case SPEED_10000:
		ctrl1 |= MDIO_CTRL1_SPEED10G;
		/* Assume 10Gbase-T */
		ctrl2 |= MDIO_PMA_CTRL2_10GBT;
		break;
	case SPEED_2500:
		ctrl1 |= MDIO_PMA_CTRL1_AQ_SPEED2500;
		ctrl2 |= MDIO_PMA_CTRL2_AQ_2500BT;
		break;
	case SPEED_5000:
		ctrl1 |= MDIO_PMA_CTRL1_AQ_SPEED5000;
		ctrl2 |= MDIO_PMA_CTRL2_AQ_5000BT;
		break;
	default:
		return -EINVAL;
	}

	ret = aquantia_write_reg(phydev, MDIO_MMD_PMAPMD, MDIO_CTRL1, ctrl1);
	if (ret < 0)
		return ret;

	return aquantia_write_reg(phydev, MDIO_MMD_PMAPMD, MDIO_CTRL2, ctrl2);
}

static int aquantia_aneg(struct phy_device *phydev, bool control)
{
	int reg = aquantia_read_reg(phydev, MDIO_MMD_AN, MDIO_CTRL1);

	if (reg < 0)
		return reg;

	if (control)
		reg |= MDIO_AN_CTRL1_ENABLE | MDIO_AN_CTRL1_RESTART;
	else
		reg &= ~(MDIO_AN_CTRL1_ENABLE | MDIO_AN_CTRL1_RESTART);

	return aquantia_write_reg(phydev, MDIO_MMD_AN, MDIO_CTRL1, reg);
}

static int aquantia_config_advert(struct phy_device *phydev)
{
	u32 advertise;
	int oldadv, adv, oldadv1, adv1;
	int err, changed = 0;

	/* Only allow advertising what this PHY supports */
	phydev->advertising &= phydev->supported;
	advertise = phydev->advertising;

	/* Setup standard advertisement */
	oldadv = aquantia_read_reg(phydev, MDIO_MMD_AN,
				   MDIO_AN_10GBT_CTRL);
	if (oldadv < 0)
		return oldadv;

	/* Aquantia vendor specific advertisments */
	oldadv1 = aquantia_read_reg(phydev, MDIO_MMD_AN,
				    MDIO_AN_VENDOR_PROV_CTRL);
	if (oldadv1 < 0)
		return oldadv1;

	adv  = 0;
	adv1 = 0;

	/*100BaseT_full is supported by default*/

	if (advertise & ADVERTISED_1000baseT_Full)
		adv1 |= 0x8000;
	if (advertise & ADVERTISED_10000baseT_Full)
		adv |= 0x1000;
#if defined(CONFIG_WG_PLATFORM_M590_M690) // WG:XD FBX-21540
	if (advertise &  ADVERTISED_2500baseX_Full) {
		adv1 |= 0x400;
		adv  |= 0x0051;	// 2.5G support
	}
	adv1 |= 0x1000;		// rate downshift
#else
	if (advertise &  ADVERTISED_2500baseX_Full)
		adv1 |= 0x400;
#endif

	if (adv != oldadv) {
		err = aquantia_write_reg(phydev, MDIO_MMD_AN,
					 MDIO_AN_10GBT_CTRL, adv);
		if (err < 0)
			return err;
		changed = 1;
	}
	if (adv1 != oldadv1) {
		err = aquantia_write_reg(phydev, MDIO_MMD_AN,
					 MDIO_AN_VENDOR_PROV_CTRL, adv1);
		if (err < 0)
			return err;
		changed = 1;
	}

	return changed;
}

static int aquantia_config_aneg(struct phy_device *phydev)
{
	int ret = 0;

	phydev->supported = PHY_AQUANTIA_FEATURES;
	if (phydev->autoneg == AUTONEG_DISABLE) {
		aquantia_pma_setup_forced(phydev);
		return aquantia_aneg(phydev, false);
	}

	ret = aquantia_config_advert(phydev);
	if (ret > 0)
		/* restart autoneg */
		return aquantia_aneg(phydev, true);

	return ret;
}

static struct {
	u16 syscfg;
	int cnt;
	u16 start_rate;
} aquantia_syscfg[PHY_INTERFACE_MODE_MAX] = {
	[PHY_INTERFACE_MODE_SGMII] =      {0x04b, AQUANTIA_VND1_GSYSCFG_1G,
					   AQUANTIA_VND1_GSTART_RATE_1G},
	[PHY_INTERFACE_MODE_2500BASEX] = {0x144, AQUANTIA_VND1_GSYSCFG_2_5G,
					   AQUANTIA_VND1_GSTART_RATE_2_5G},
	[PHY_INTERFACE_MODE_XGMII] =      {0x100, AQUANTIA_VND1_GSYSCFG_10G,
					   AQUANTIA_VND1_GSTART_RATE_10G},
	[PHY_INTERFACE_MODE_USXGMII] =    {0x080, AQUANTIA_VND1_GSYSCFG_10G,
					   AQUANTIA_VND1_GSTART_RATE_10G},
};

/* Sets up protocol on system side before calling aqr_config_aneg */
static int aquantia_config_aneg_set_prot(struct phy_device *phydev)
{
	int if_type = phydev->interface;
	int i;

	if (!aquantia_syscfg[if_type].cnt)
		return 0;

	/* set PHY in low power mode so we can configure protocols */
	phy_write_mmd(phydev, MDIO_MMD_VEND1, AQUANTIA_VND1_GLOBAL_SC,
		      AQUANTIA_VND1_GLOBAL_SC_LP);
	mdelay(10);

	/* set the default rate to enable the SI link */
	phy_write_mmd(phydev, MDIO_MMD_VEND1, AQUANTIA_VND1_GSTART_RATE,
		      aquantia_syscfg[if_type].start_rate);

	for (i = 0; i <= aquantia_syscfg[if_type].cnt; i++) {
		u16 reg = phy_read_mmd(phydev, MDIO_MMD_VEND1,
				       AQUANTIA_VND1_GSYSCFG_BASE + i);
		if (!reg)
			continue;

		phy_write_mmd(phydev, MDIO_MMD_VEND1,
			      AQUANTIA_VND1_GSYSCFG_BASE + i,
			      aquantia_syscfg[if_type].syscfg);
	}

	if (if_type == PHY_INTERFACE_MODE_USXGMII)
		phy_write_mmd(phydev, MDIO_MMD_PHYXS, MDIO_PHYXS_VEND_PROV2,
			      MDIO_PHYXS_VEND_PROV2_USX_AN);

	/* wake PHY back up */
	phy_write_mmd(phydev, MDIO_MMD_VEND1, AQUANTIA_VND1_GLOBAL_SC, 0);
	mdelay(10);

#if defined(CONFIG_WG_PLATFORM_M590_M690)
	/* apply pre-emphasis configuration */
	printk("aquantia_config_aneg_set_prot\n");
	aquantia_pre_emphasis_config(phydev);
	mdelay(10);
#endif

	return aquantia_config_aneg(phydev);
}

static int aquantia_aneg_done(struct phy_device *phydev)
{
	int reg;

	reg = phy_read_mmd(phydev, MDIO_MMD_AN, MDIO_STAT1);
	return (reg < 0) ? reg : (reg & BMSR_ANEGCOMPLETE);
}

static int aquantia_config_intr(struct phy_device *phydev)
{
	int err;

	if (phydev->interrupts == PHY_INTERRUPT_ENABLED) {
		err = aquantia_write_reg(phydev, MDIO_MMD_AN, 0xd401, 1);
		if (err < 0)
			return err;

		err = aquantia_write_reg(phydev, MDIO_MMD_VEND1, 0xff00, 1);
		if (err < 0)
			return err;

		err = aquantia_write_reg(phydev, MDIO_MMD_VEND1,
					 0xff01, 0x1001);
	} else {
		err = aquantia_write_reg(phydev, MDIO_MMD_AN, 0xd401, 0);
		if (err < 0)
			return err;

		err = aquantia_write_reg(phydev, MDIO_MMD_VEND1, 0xff00, 0);
		if (err < 0)
			return err;

		err = aquantia_write_reg(phydev, MDIO_MMD_VEND1, 0xff01, 0);
	}

	return err;
}

static int aquantia_ack_interrupt(struct phy_device *phydev)
{
	int reg;

	reg = aquantia_read_reg(phydev, MDIO_MMD_AN, 0xcc01);
	return (reg < 0) ? reg : 0;
}

static int aquantia_read_advert(struct phy_device *phydev)
{
	int adv, adv1;

	/* Setup standard advertisement */
	adv = aquantia_read_reg(phydev, MDIO_MMD_AN,
				MDIO_AN_10GBT_CTRL);

	/* Aquantia vendor specific advertisments */
	adv1 = aquantia_read_reg(phydev, MDIO_MMD_AN,
				 MDIO_AN_VENDOR_PROV_CTRL);

	/*100BaseT_full is supported by default*/
	phydev->advertising |= ADVERTISED_100baseT_Full;

	if (adv & 0x1000)
		phydev->advertising |= ADVERTISED_10000baseT_Full;
	else
		phydev->advertising &= ~ADVERTISED_10000baseT_Full;
	if (adv1 & 0x8000)
		phydev->advertising |= ADVERTISED_1000baseT_Full;
	else
		phydev->advertising &= ~ADVERTISED_1000baseT_Full;
	if (adv1 & 0x400)
		phydev->advertising |= ADVERTISED_2500baseX_Full;
	else
		phydev->advertising &= ~ADVERTISED_2500baseX_Full;
	return 0;
}

static int aquantia_read_lp_advert(struct phy_device *phydev)
{
	int adv, adv1;

	/* Read standard link partner advertisement */
	adv = aquantia_read_reg(phydev, MDIO_MMD_AN,
				MDIO_STAT1);

	if (adv & 0x1)
		phydev->lp_advertising |= ADVERTISED_Autoneg |
					  ADVERTISED_100baseT_Full;
	else
		phydev->lp_advertising &= ~(ADVERTISED_Autoneg |
					    ADVERTISED_100baseT_Full);

	/* Read standard link partner advertisement */
	adv = aquantia_read_reg(phydev, MDIO_MMD_AN,
				MDIO_AN_10GBT_STAT);

	/* Aquantia link partner advertisments */
	adv1 = aquantia_read_reg(phydev, MDIO_MMD_AN,
				 MDIO_AN_RECV_LP_STATUS);

	if (adv & 0x800)
		phydev->lp_advertising |= ADVERTISED_10000baseT_Full;
	else
		phydev->lp_advertising &= ~ADVERTISED_10000baseT_Full;
	if (adv1 & 0x8000)
		phydev->lp_advertising |= ADVERTISED_1000baseT_Full;
	else
		phydev->lp_advertising &= ~ADVERTISED_1000baseT_Full;
	if (adv1 & 0x400)
		phydev->lp_advertising |= ADVERTISED_2500baseX_Full;
	else
		phydev->lp_advertising &= ~ADVERTISED_2500baseX_Full;

	return 0;
}

static int aquantia_read_status(struct phy_device *phydev)
{
	int reg;

	/* Read the link status twice; the bit is latching low */
	reg = aquantia_read_reg(phydev, MDIO_MMD_AN, MDIO_STAT1);
	reg = aquantia_read_reg(phydev, MDIO_MMD_AN, MDIO_STAT1);

	if (reg & MDIO_STAT1_LSTATUS)
		phydev->link = 1;
	else
		phydev->link = 0;

	mdelay(10);
	reg = aquantia_read_reg(phydev, MDIO_MMD_PMAPMD, MDIO_CTRL1);

	if ((reg & MDIO_CTRL1_SPEEDSELEXT) == MDIO_CTRL1_SPEEDSELEXT)
		reg &= MDIO_CTRL1_SPEEDSEL;
	else
		reg &= MDIO_CTRL1_SPEEDSELEXT;

	switch (reg) {
	case MDIO_PMA_CTRL1_AQ_SPEED5000:
		phydev->speed = SPEED_5000;
		break;
	case MDIO_PMA_CTRL1_AQ_SPEED2500:
		phydev->speed = SPEED_2500;
		break;
	case MDIO_PMA_CTRL1_AQ_SPEED10:
		phydev->speed = SPEED_10;
		break;
	case MDIO_PMA_CTRL1_SPEED100:
		phydev->speed = SPEED_100;
		break;
	case MDIO_PMA_CTRL1_SPEED1000:
		phydev->speed = SPEED_1000;
		break;
	case MDIO_CTRL1_SPEED10G:
		phydev->speed = SPEED_10000;
		break;
	default:
		phydev->speed = SPEED_UNKNOWN;
		break;
	}

	phydev->duplex = DUPLEX_FULL;

	aquantia_read_advert(phydev);
	aquantia_read_lp_advert(phydev);

	return 0;
}

static struct phy_driver aquantia_driver[] = {
{
	.phy_id		= PHY_ID_AQ1202,
	.phy_id_mask	= 0xfffffff0,
	.name		= "Aquantia AQ1202",
	.features	= PHY_AQUANTIA_FEATURES,
	.flags		= PHY_HAS_INTERRUPT,
	.aneg_done	= aquantia_aneg_done,
	.config_aneg    = aquantia_config_aneg,
	.config_intr	= aquantia_config_intr,
	.ack_interrupt	= aquantia_ack_interrupt,
	.read_status	= aquantia_read_status,
},
{
	.phy_id		= PHY_ID_AQ2104,
	.phy_id_mask	= 0xfffffff0,
	.name		= "Aquantia AQ2104",
	.features	= PHY_AQUANTIA_FEATURES,
	.flags		= PHY_HAS_INTERRUPT,
	.aneg_done	= aquantia_aneg_done,
	.config_aneg    = aquantia_config_aneg,
	.config_intr	= aquantia_config_intr,
	.ack_interrupt	= aquantia_ack_interrupt,
	.read_status	= aquantia_read_status,
},
{
	.phy_id		= PHY_ID_AQR105,
	.phy_id_mask	= 0xfffffff0,
	.name		= "Aquantia AQR105",
	.features	= PHY_AQUANTIA_FEATURES,
	.flags		= PHY_HAS_INTERRUPT,
	.aneg_done	= aquantia_aneg_done,
	.config_aneg    = aquantia_config_aneg,
	.config_intr	= aquantia_config_intr,
	.ack_interrupt	= aquantia_ack_interrupt,
	.read_status	= aquantia_read_status,
},
{
	.phy_id		= PHY_ID_AQR106,
	.phy_id_mask	= 0xfffffff0,
	.name		= "Aquantia AQR106",
	.features	= PHY_AQUANTIA_FEATURES,
	.flags		= PHY_HAS_INTERRUPT,
	.aneg_done	= aquantia_aneg_done,
	.config_aneg    = aquantia_config_aneg,
	.config_intr	= aquantia_config_intr,
	.ack_interrupt	= aquantia_ack_interrupt,
	.read_status	= aquantia_read_status,
},
{
	.phy_id		= PHY_ID_AQR107,
	.phy_id_mask	= 0xfffffff0,
	.name		= "Aquantia AQR107",
	.features	= PHY_AQUANTIA_FEATURES,
	.flags		= PHY_HAS_INTERRUPT,
	.aneg_done	= aquantia_aneg_done,
	.config_aneg    = aquantia_config_aneg,
	.config_intr	= aquantia_config_intr,
	.ack_interrupt	= aquantia_ack_interrupt,
	.read_status	= aquantia_read_status,
},
{
	.phy_id		= PHY_ID_AQR405,
	.phy_id_mask	= 0xfffffff0,
	.name		= "Aquantia AQR405",
	.features	= PHY_AQUANTIA_FEATURES,
	.flags		= PHY_HAS_INTERRUPT,
	.aneg_done	= aquantia_aneg_done,
	.config_aneg    = aquantia_config_aneg,
	.config_intr	= aquantia_config_intr,
	.ack_interrupt	= aquantia_ack_interrupt,
	.read_status	= aquantia_read_status,
},
{
	.phy_id		= PHY_ID_AQR112,
	.phy_id_mask	= 0xfffffff0,
	.name		= "Aquantia AQR112",
	.features	= PHY_AQUANTIA_FEATURES,
	.flags		= PHY_HAS_INTERRUPT,
	.aneg_done	= genphy_c45_aneg_done,
	.config_aneg    = aquantia_config_aneg_set_prot,
	.config_intr	= aquantia_config_intr,
	.ack_interrupt	= aquantia_ack_interrupt,
	.read_status	= aquantia_read_status,
},
{
	.phy_id		= PHY_ID_AQR412,
	.phy_id_mask	= 0xfffffff0,
	.name		= "Aquantia AQR412",
	.features	= PHY_AQUANTIA_FEATURES,
	.flags		= PHY_HAS_INTERRUPT,
	.aneg_done	= genphy_c45_aneg_done,
	.config_aneg    = aquantia_config_aneg_set_prot,
	.config_intr	= aquantia_config_intr,
	.ack_interrupt	= aquantia_ack_interrupt,
	.read_status	= aquantia_read_status,
},
#if defined(CONFIG_WG_PLATFORM_M590_M690)
{
	.phy_id		= PHY_ID_AQR113C,
	.phy_id_mask	= 0xfffffff0,
	.name		= "Aquantia AQR113C",
	.features	= PHY_AQUANTIA_FEATURES,
	.flags		= PHY_HAS_INTERRUPT,
	.probe = aqr_hwmon_probe,
	.aneg_done	= genphy_c45_aneg_done,
	.config_aneg    = aquantia_config_aneg_set_prot,
	.config_intr	= aquantia_config_intr,
	.ack_interrupt	= aquantia_ack_interrupt,
	.read_status	= aquantia_read_status,
},
#endif
};

module_phy_driver(aquantia_driver);

static struct mdio_device_id __maybe_unused aquantia_tbl[] = {
	{ PHY_ID_AQ1202, 0xfffffff0 },
	{ PHY_ID_AQ2104, 0xfffffff0 },
	{ PHY_ID_AQR105, 0xfffffff0 },
	{ PHY_ID_AQR106, 0xfffffff0 },
	{ PHY_ID_AQR107, 0xfffffff0 },
	{ PHY_ID_AQR405, 0xfffffff0 },
	{ PHY_ID_AQR112, 0xfffffff0 },
	{ PHY_ID_AQR412, 0xfffffff0 },
#if defined(CONFIG_WG_PLATFORM_M590_M690)
	{ PHY_ID_AQR113C, 0xfffffff0 },
#endif
	{ }
};

MODULE_DEVICE_TABLE(mdio, aquantia_tbl);

MODULE_DESCRIPTION("Aquantia PHY driver");
MODULE_AUTHOR("Shaohui Xie <Shaohui.Xie@freescale.com>");
MODULE_LICENSE("GPL v2");
