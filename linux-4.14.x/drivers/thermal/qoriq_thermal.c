/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#if defined(CONFIG_WG_PLATFORM_M590_M690)
#include <linux/device_cooling.h>
#endif
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/thermal.h>

#include "thermal_core.h"

#define SITES_MAX	16
#if defined(CONFIG_WG_PLATFORM_M590_M690)
#define TMU_TEMP_PASSIVE_COOL_DELTA	10000
#define TMR_DISABLE		0x0
#define TMR_ME			0x80000000
#define TMR_ALPF		0x0c000000
#define TMR_ALPF_V2		0x03000000
#define TMTMIR_DEFAULT	0x0000000f
#define TIER_DISABLE	0x0
#define TEUMR0_V2		0x51009c00
#define TMSARA_V2		0xe
#define TMU_VER1		0x1
#define TMU_VER2		0x2
#endif

/*
 * QorIQ TMU Registers
 */
struct qoriq_tmu_site_regs {
	u32 tritsr;		/* Immediate Temperature Site Register */
	u32 tratsr;		/* Average Temperature Site Register */
	u8 res0[0x8];
};

#if defined(CONFIG_WG_PLATFORM_M590_M690)
struct qoriq_tmu_tmsar {
	u32 res0;
	u32 tmsar;
	u32 res1;
	u32 res2;
};

struct qoriq_tmu_regs_v1 {
	u32 tmr;		/* Mode Register */
	u32 tsr;		/* Status Register */
	u32 tmtmir;		/* Temperature measurement interval Register */
	u8 res0[0x14];
	u32 tier;		/* Interrupt Enable Register */
	u32 tidr;		/* Interrupt Detect Register */
	u32 tiscr;		/* Interrupt Site Capture Register */
	u32 ticscr;		/* Interrupt Critical Site Capture Register */
	u8 res1[0x10];
	u32 tmhtcrh;		/* High Temperature Capture Register */
	u32 tmhtcrl;		/* Low Temperature Capture Register */
	u8 res2[0x8];
	u32 tmhtitr;		/* High Temperature Immediate Threshold */
	u32 tmhtatr;		/* High Temperature Average Threshold */
	u32 tmhtactr;	/* High Temperature Average Crit Threshold */
	u8 res3[0x24];
	u32 ttcfgr;		/* Temperature Configuration Register */
	u32 tscfgr;		/* Sensor Configuration Register */
	u8 res4[0x78];
	struct qoriq_tmu_site_regs site[SITES_MAX];
	u8 res5[0x9f8];
	u32 ipbrr0;		/* IP Block Revision Register 0 */
	u32 ipbrr1;		/* IP Block Revision Register 1 */
	u8 res6[0x310];
	u32 ttrcr[4];		/* Temperature Range Control Register */
};

struct qoriq_tmu_regs_v2 {
	u32 tmr;		/* Mode Register */
	u32 tsr;		/* Status Register */
	u32 tmsr;		/* monitor site register */
	u32 tmtmir;		/* Temperature measurement interval Register */
	u8 res0[0x10];
	u32 tier;		/* Interrupt Enable Register */
	u32 tidr;		/* Interrupt Detect Register */
	u8 res1[0x8];
	u32 tiiscr;		/* interrupt immediate site capture register */
	u32 tiascr;		/* interrupt average site capture register */
	u32 ticscr;		/* Interrupt Critical Site Capture Register */
	u32 res2;
	u32 tmhtcr;		/* monitor high temperature capture register */
	u32 tmltcr;		/* monitor low temperature capture register */
	u32 tmrtrcr;	/* monitor rising temperature rate capture register */
	u32 tmftrcr;	/* monitor falling temperature rate capture register */
	u32 tmhtitr;	/* High Temperature Immediate Threshold */
	u32 tmhtatr;	/* High Temperature Average Threshold */
	u32 tmhtactr;	/* High Temperature Average Crit Threshold */
	u32 res3;
	u32 tmltitr;	/* monitor low temperature immediate threshold */
	u32 tmltatr;	/* monitor low temperature average threshold register */
	u32 tmltactr;	/* monitor low temperature average critical threshold */
	u32 res4;
	u32 tmrtrctr;	/* monitor rising temperature rate critical threshold */
	u32 tmftrctr;	/* monitor falling temperature rate critical threshold*/
	u8 res5[0x8];
	u32 ttcfgr;	/* Temperature Configuration Register */
	u32 tscfgr;	/* Sensor Configuration Register */
	u8 res6[0x78];
	struct qoriq_tmu_site_regs site[SITES_MAX];
	u8 res10[0x100];
	struct qoriq_tmu_tmsar tmsar[16];
	u8 res7[0x7f8];
	u32 ipbrr0;		/* IP Block Revision Register 0 */
	u32 ipbrr1;		/* IP Block Revision Register 1 */
	u8 res8[0x300];
	u32 teumr0;
	u32 teumr1;
	u32 teumr2;
	u32 res9;
	u32 ttrcr[4];	/* Temperature Range Control Register */
 };
#else

struct qoriq_tmu_regs {
	u32 tmr;		/* Mode Register */
#define TMR_DISABLE	0x0
#define TMR_ME		0x80000000
#define TMR_ALPF	0x0c000000
	u32 tsr;		/* Status Register */
	u32 tmtmir;		/* Temperature measurement interval Register */
#define TMTMIR_DEFAULT	0x0000000f
	u8 res0[0x14];
	u32 tier;		/* Interrupt Enable Register */
#define TIER_DISABLE	0x0
	u32 tidr;		/* Interrupt Detect Register */
	u32 tiscr;		/* Interrupt Site Capture Register */
	u32 ticscr;		/* Interrupt Critical Site Capture Register */
	u8 res1[0x10];
	u32 tmhtcrh;		/* High Temperature Capture Register */
	u32 tmhtcrl;		/* Low Temperature Capture Register */
	u8 res2[0x8];
	u32 tmhtitr;		/* High Temperature Immediate Threshold */
	u32 tmhtatr;		/* High Temperature Average Threshold */
	u32 tmhtactr;	/* High Temperature Average Crit Threshold */
	u8 res3[0x24];
	u32 ttcfgr;		/* Temperature Configuration Register */
	u32 tscfgr;		/* Sensor Configuration Register */
	u8 res4[0x78];
	struct qoriq_tmu_site_regs site[SITES_MAX];
	u8 res5[0x9f8];
	u32 ipbrr0;		/* IP Block Revision Register 0 */
	u32 ipbrr1;		/* IP Block Revision Register 1 */
	u8 res6[0x310];
	u32 ttr0cr;		/* Temperature Range 0 Control Register */
	u32 ttr1cr;		/* Temperature Range 1 Control Register */
	u32 ttr2cr;		/* Temperature Range 2 Control Register */
	u32 ttr3cr;		/* Temperature Range 3 Control Register */
};
#endif

struct qoriq_tmu_data;

/*
 * Thermal zone data
 */
struct qoriq_sensor {
	struct thermal_zone_device	*tzd;
	struct qoriq_tmu_data		*qdata;
	int				id;
#if defined(CONFIG_WG_PLATFORM_M590_M690)
	int				temp_passive;
	int				temp_critical;
	struct thermal_cooling_device 	*cdev;
#endif
};

struct qoriq_tmu_data {
#if defined(CONFIG_WG_PLATFORM_M590_M690)
	int ver;
	struct qoriq_tmu_regs_v1 __iomem *regs;
	struct qoriq_tmu_regs_v2 __iomem *regs_v2;
	struct clk *clk;
#else
	struct qoriq_tmu_regs __iomem *regs;
#endif
	bool little_endian;
	struct qoriq_sensor	*sensor[SITES_MAX];
};

#if defined(CONFIG_WG_PLATFORM_M590_M690)
enum tmu_trip {
	TMU_TRIP_PASSIVE,
	TMU_TRIP_CRITICAL,
	TMU_TRIP_NUM,
};
#endif

static void tmu_write(struct qoriq_tmu_data *p, u32 val, void __iomem *addr)
{
	if (p->little_endian)
		iowrite32(val, addr);
	else
		iowrite32be(val, addr);
}

static u32 tmu_read(struct qoriq_tmu_data *p, void __iomem *addr)
{
	if (p->little_endian)
		return ioread32(addr);
	else
		return ioread32be(addr);
}

static int tmu_get_temp(void *p, int *temp)
{
	struct qoriq_sensor *qsensor = p;
	struct qoriq_tmu_data *qdata = qsensor->qdata;
	u32 val;

	val = tmu_read(qdata, &qdata->regs->site[qsensor->id].tritsr);
#if defined(CONFIG_WG_PLATFORM_M590_M690)
	if (qdata->ver == TMU_VER1)
		*temp = (val & 0xff) * 1000;
	else
		*temp = (val & 0x1ff) * 1000 - 273150;
#else
	*temp = (val & 0xff) * 1000;
#endif

	return 0;
}

#if defined(CONFIG_WG_PLATFORM_M590_M690)
static int tmu_get_trend(void *p, int trip, enum thermal_trend *trend)
{
	struct qoriq_sensor *qsensor = p;
	int trip_temp;

	if (!qsensor->tzd)
		return 0;

	trip_temp = (trip == TMU_TRIP_PASSIVE) ? qsensor->temp_passive :
					     qsensor->temp_critical;

	if (qsensor->tzd->temperature >=
		(trip_temp - TMU_TEMP_PASSIVE_COOL_DELTA))
		*trend = THERMAL_TREND_RAISE_FULL;
	else
		*trend = THERMAL_TREND_DROP_FULL;

	return 0;
}

static int tmu_set_trip_temp(void *p, int trip,
			     int temp)
{
	struct qoriq_sensor *qsensor = p;

	if (trip == TMU_TRIP_CRITICAL)
		qsensor->temp_critical = temp;

	if (trip == TMU_TRIP_PASSIVE)
		qsensor->temp_passive = temp;

	return 0;
}
#endif

static const struct thermal_zone_of_device_ops tmu_tz_ops = {
	.get_temp = tmu_get_temp,
#if defined(CONFIG_WG_PLATFORM_M590_M690)
	.get_trend = tmu_get_trend,
	.set_trip_temp = tmu_set_trip_temp,
#endif
};

static int qoriq_tmu_register_tmu_zone(struct platform_device *pdev)
{
	struct qoriq_tmu_data *qdata = platform_get_drvdata(pdev);
#if defined(CONFIG_WG_PLATFORM_M590_M690)
	const struct thermal_trip *trip;
	int id, sites = 0, ret;
#else
	int id, sites = 0;
#endif

	for (id = 0; id < SITES_MAX; id++) {
		qdata->sensor[id] = devm_kzalloc(&pdev->dev,
				sizeof(struct qoriq_sensor), GFP_KERNEL);
		if (!qdata->sensor[id])
			return -ENOMEM;

		qdata->sensor[id]->id = id;
		qdata->sensor[id]->qdata = qdata;

		qdata->sensor[id]->tzd = devm_thermal_zone_of_sensor_register(
				&pdev->dev, id, qdata->sensor[id], &tmu_tz_ops);
		if (IS_ERR(qdata->sensor[id]->tzd)) {
			if (PTR_ERR(qdata->sensor[id]->tzd) == -ENODEV)
				continue;
			else
				return PTR_ERR(qdata->sensor[id]->tzd);

		}

#if defined(CONFIG_WG_PLATFORM_M590_M690)
#if 0
		/* first thermal zone takes care of system-wide device cooling */
		if (id == 0) {
			qdata->sensor[id]->cdev = devfreq_cooling_register();
			if (IS_ERR(qdata->sensor[id]->cdev)) {
				ret = PTR_ERR(qdata->sensor[id]->cdev);
				pr_err("failed to register devfreq cooling device: %d\n",
					ret);
				return ret;
			}

			ret = thermal_zone_bind_cooling_device(qdata->sensor[id]->tzd,
				TMU_TRIP_PASSIVE,
				qdata->sensor[id]->cdev,
				THERMAL_NO_LIMIT,
				THERMAL_NO_LIMIT,
				THERMAL_WEIGHT_DEFAULT);
			if (ret) {
				pr_err("binding zone %s with cdev %s failed:%d\n",
					qdata->sensor[id]->tzd->type,
					qdata->sensor[id]->cdev->type,
					ret);
				devfreq_cooling_unregister(qdata->sensor[id]->cdev);
				return ret;
			}

			trip = of_thermal_get_trip_points(qdata->sensor[id]->tzd);
			qdata->sensor[id]->temp_passive = trip[0].temperature;
			qdata->sensor[id]->temp_critical = trip[1].temperature;
		}
#endif
#endif // #if defined(CONFIG_WG_PLATFORM_M590_M690)

#if defined(CONFIG_WG_PLATFORM_M590_M690)
		if (qdata->ver == TMU_VER1)
			sites |= 0x1 << (15 - id);
		else
			sites |= 0x1 << id;
#else
		sites |= 0x1 << (15 - id);
#endif
	}

	/* Enable monitoring */
#if defined(CONFIG_WG_PLATFORM_M590_M690)
	if (sites != 0) {
		if (qdata->ver == TMU_VER1) {
			tmu_write(qdata, sites | TMR_ME | TMR_ALPF,
					&qdata->regs->tmr);
		} else {
			tmu_write(qdata, sites, &qdata->regs_v2->tmsr);
			tmu_write(qdata, TMR_ME | TMR_ALPF_V2,
					&qdata->regs_v2->tmr);
		}
	}
#else
	if (sites != 0)
		tmu_write(qdata, sites | TMR_ME | TMR_ALPF, &qdata->regs->tmr);
#endif

	return 0;
}

static int qoriq_tmu_calibration(struct platform_device *pdev)
{
	int i, val, len;
	u32 range[4];
	const u32 *calibration;
	struct device_node *np = pdev->dev.of_node;
	struct qoriq_tmu_data *data = platform_get_drvdata(pdev);

#if defined(CONFIG_WG_PLATFORM_M590_M690)
	len = of_property_count_u32_elems(np, "fsl,tmu-range");
	if (len < 0 || len > 4) {
		dev_err(&pdev->dev, "invalid range data.\n");
		return len;
	}

	val = of_property_read_u32_array(np, "fsl,tmu-range", range, len);
	if (val != 0) {
		dev_err(&pdev->dev, "failed to read range data.\n");
		return val;
	}

	/* Init temperature range registers */
	for (i = 0; i < len; i++)
		tmu_write(data, range[i], &data->regs->ttrcr[i]);
#else
	if (of_property_read_u32_array(np, "fsl,tmu-range", range, 4)) {
		dev_err(&pdev->dev, "missing calibration range.\n");
		return -ENODEV;
	}

	/* Init temperature range registers */
	tmu_write(data, range[0], &data->regs->ttr0cr);
	tmu_write(data, range[1], &data->regs->ttr1cr);
	tmu_write(data, range[2], &data->regs->ttr2cr);
	tmu_write(data, range[3], &data->regs->ttr3cr);
#endif

	calibration = of_get_property(np, "fsl,tmu-calibration", &len);
	if (calibration == NULL || len % 8) {
		dev_err(&pdev->dev, "invalid calibration data.\n");
		return -ENODEV;
	}

	for (i = 0; i < len; i += 8, calibration += 2) {
		val = of_read_number(calibration, 1);
		tmu_write(data, val, &data->regs->ttcfgr);
		val = of_read_number(calibration + 1, 1);
		tmu_write(data, val, &data->regs->tscfgr);
	}

	return 0;
}

static void qoriq_tmu_init_device(struct qoriq_tmu_data *data)
{
#if defined(CONFIG_WG_PLATFORM_M590_M690)
	int i;
#endif
	/* Disable interrupt, using polling instead */
	tmu_write(data, TIER_DISABLE, &data->regs->tier);

	/* Set update_interval */
#if defined(CONFIG_WG_PLATFORM_M590_M690)
	if (data->ver == TMU_VER1) {
		tmu_write(data, TMTMIR_DEFAULT, &data->regs->tmtmir);
	} else {
		tmu_write(data, TMTMIR_DEFAULT, &data->regs_v2->tmtmir);
		tmu_write(data, TEUMR0_V2, &data->regs_v2->teumr0);
		for (i = 0; i < 7; i++)
			tmu_write(data, TMSARA_V2, &data->regs_v2->tmsar[i].tmsar);
	}
#else
	tmu_write(data, TMTMIR_DEFAULT, &data->regs->tmtmir);
#endif

	/* Disable monitoring */
	tmu_write(data, TMR_DISABLE, &data->regs->tmr);
}

static int qoriq_tmu_probe(struct platform_device *pdev)
{
	int ret;
#if defined(CONFIG_WG_PLATFORM_M590_M690)
	u32 ver;
#endif
	struct qoriq_tmu_data *data;
	struct device_node *np = pdev->dev.of_node;

#if !defined(CONFIG_WG_PLATFORM_M590_M690)
	if (!np) {
		dev_err(&pdev->dev, "Device OF-Node is NULL");
		return -ENODEV;
	}
#endif

	data = devm_kzalloc(&pdev->dev, sizeof(struct qoriq_tmu_data),
			    GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	platform_set_drvdata(pdev, data);

	data->little_endian = of_property_read_bool(np, "little-endian");

	data->regs = of_iomap(np, 0);
	if (!data->regs) {
		dev_err(&pdev->dev, "Failed to get memory region\n");
		ret = -ENODEV;
		goto err_iomap;
	}
#if defined(CONFIG_WG_PLATFORM_M590_M690)
	/* version register offset at: 0xbf8 on both v1 and v2 */
	ver = tmu_read(data, &data->regs->ipbrr0);
	data->ver = (ver >> 8) & 0xff;
	if (data->ver == TMU_VER2)
		data->regs_v2 = (void __iomem *)data->regs;
#endif

	qoriq_tmu_init_device(data);	/* TMU initialization */

	ret = qoriq_tmu_calibration(pdev);	/* TMU calibration */
	if (ret < 0)
		goto err_tmu;

	ret = qoriq_tmu_register_tmu_zone(pdev);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to register sensors\n");
		ret = -ENODEV;
		goto err_iomap;
	}

	return 0;

err_tmu:
	iounmap(data->regs);

err_iomap:
	platform_set_drvdata(pdev, NULL);

	return ret;
}

static int qoriq_tmu_remove(struct platform_device *pdev)
{
	struct qoriq_tmu_data *data = platform_get_drvdata(pdev);

	/* Disable monitoring */
	tmu_write(data, TMR_DISABLE, &data->regs->tmr);

	iounmap(data->regs);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int qoriq_tmu_suspend(struct device *dev)
{
	u32 tmr;
	struct qoriq_tmu_data *data = dev_get_drvdata(dev);

	/* Disable monitoring */
	tmr = tmu_read(data, &data->regs->tmr);
	tmr &= ~TMR_ME;
	tmu_write(data, tmr, &data->regs->tmr);

	return 0;
}

static int qoriq_tmu_resume(struct device *dev)
{
	u32 tmr;
	struct qoriq_tmu_data *data = dev_get_drvdata(dev);

	/* Enable monitoring */
	tmr = tmu_read(data, &data->regs->tmr);
	tmr |= TMR_ME;
	tmu_write(data, tmr, &data->regs->tmr);

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(qoriq_tmu_pm_ops,
			 qoriq_tmu_suspend, qoriq_tmu_resume);

static const struct of_device_id qoriq_tmu_match[] = {
	{ .compatible = "fsl,qoriq-tmu", },
	{},
};
MODULE_DEVICE_TABLE(of, qoriq_tmu_match);

static struct platform_driver qoriq_tmu = {
	.driver	= {
		.name		= "qoriq_thermal",
		.pm		= &qoriq_tmu_pm_ops,
		.of_match_table	= qoriq_tmu_match,
	},
	.probe	= qoriq_tmu_probe,
	.remove	= qoriq_tmu_remove,
};
module_platform_driver(qoriq_tmu);

MODULE_AUTHOR("Jia Hongtao <hongtao.jia@nxp.com>");
MODULE_DESCRIPTION("QorIQ Thermal Monitoring Unit driver");
MODULE_LICENSE("GPL v2");
