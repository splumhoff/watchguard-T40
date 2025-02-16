/**
 * host.c - DesignWare USB3 DRD Controller Host Glue
 *
 * Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com
 *
 * Authors: Felipe Balbi <balbi@ti.com>,
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2  of
 * the License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/platform_device.h>

#include "../host/xhci.h"

#include "core.h"


#define XHCI_HCSPARAMS1		0x4
#define XHCI_PORTSC_BASE	0x400

/*
 * dwc3_power_off_all_roothub_ports - Power off all Root hub ports
 * @dwc3: Pointer to our controller context structure
 */
static void dwc3_power_off_all_roothub_ports(struct dwc3 *dwc)
{
	int i, port_num;
	u32 reg, op_regs_base, offset;
	void __iomem *xhci_regs;

	/* xhci regs is not mapped yet, do it temperary here */
	if (dwc->xhci_resources[0].start) {
		xhci_regs = ioremap(dwc->xhci_resources[0].start,
				DWC3_XHCI_REGS_END);
		if (IS_ERR(xhci_regs)) {
			dev_err(dwc->dev, "Failed to ioremap xhci_regs\n");
			return;
		}

		op_regs_base = HC_LENGTH(readl(xhci_regs));
		reg = readl(xhci_regs + XHCI_HCSPARAMS1);
		port_num = HCS_MAX_PORTS(reg);

		for (i = 1; i <= port_num; i++) {
			offset = op_regs_base + XHCI_PORTSC_BASE + 0x10*(i-1);
			reg = readl(xhci_regs + offset);
			reg &= ~PORT_POWER;
			writel(reg, xhci_regs + offset);
		}

		iounmap(xhci_regs);
	} else
		dev_err(dwc->dev, "xhci base reg invalid\n");
}

static int dwc3_host_get_irq(struct dwc3 *dwc)
{
	struct platform_device	*dwc3_pdev = to_platform_device(dwc->dev);
	int irq;

	irq = platform_get_irq_byname(dwc3_pdev, "host");
	if (irq > 0)
		goto out;

	if (irq == -EPROBE_DEFER)
		goto out;

	irq = platform_get_irq_byname(dwc3_pdev, "dwc_usb3");
	if (irq > 0)
		goto out;

	if (irq == -EPROBE_DEFER)
		goto out;

	irq = platform_get_irq(dwc3_pdev, 0);
	if (irq > 0)
		goto out;

	if (irq != -EPROBE_DEFER)
		dev_err(dwc->dev, "missing host IRQ\n");

	if (!irq)
		irq = -EINVAL;

out:
	return irq;
}

int dwc3_host_init(struct dwc3 *dwc)
{
	struct property_entry	props[3];
	struct platform_device	*xhci;
	int			ret, irq;
	struct resource		*res;
	struct platform_device	*dwc3_pdev = to_platform_device(dwc->dev);
	int			prop_idx = 0;

	/*
	 * We have to power off all Root hub ports immediately after DWC3 set
	 * to host mode to avoid VBUS glitch happen when xhci get reset later.
	 */
	if (dwc->host_vbus_glitches)
		dwc3_power_off_all_roothub_ports(dwc);

	irq = dwc3_host_get_irq(dwc);
	if (irq < 0)
		return irq;

	res = platform_get_resource_byname(dwc3_pdev, IORESOURCE_IRQ, "host");
	if (!res)
		res = platform_get_resource_byname(dwc3_pdev, IORESOURCE_IRQ,
				"dwc_usb3");
	if (!res)
		res = platform_get_resource(dwc3_pdev, IORESOURCE_IRQ, 0);
	if (!res)
		return -ENOMEM;

	dwc->xhci_resources[1].start = irq;
	dwc->xhci_resources[1].end = irq;
	dwc->xhci_resources[1].flags = res->flags;
	dwc->xhci_resources[1].name = res->name;

	xhci = platform_device_alloc("xhci-hcd", PLATFORM_DEVID_AUTO);
	if (!xhci) {
		dev_err(dwc->dev, "couldn't allocate xHCI device\n");
		return -ENOMEM;
	}

	xhci->dev.parent	= dwc->dev;

	dwc->xhci = xhci;

	ret = platform_device_add_resources(xhci, dwc->xhci_resources,
						DWC3_XHCI_RESOURCES_NUM);
	if (ret) {
		dev_err(dwc->dev, "couldn't add resources to xHCI device\n");
		goto err1;
	}

	memset(props, 0, sizeof(struct property_entry) * ARRAY_SIZE(props));

	if (dwc->quirk_reverse_in_out)
		props[prop_idx++].name = "quirk-reverse-in-out";

	if (dwc->quirk_stop_transfer_in_block)
		props[prop_idx++].name = "quirk-stop-transfer-in-block";

	if (dwc->quirk_stop_ep_in_u1)
		props[prop_idx++].name = "quirk-stop-ep-in-u1";

	if (dwc->usb3_lpm_capable)
		props[prop_idx++].name = "usb3-lpm-capable";

	/**
	 * WORKAROUND: dwc3 revisions <=3.00a have a limitation
	 * where Port Disable command doesn't work.
	 *
	 * The suggested workaround is that we avoid Port Disable
	 * completely.
	 *
	 * This following flag tells XHCI to do just that.
	 */
	if (dwc->revision <= DWC3_REVISION_300A)
		props[prop_idx++].name = "quirk-broken-port-ped";

	if (prop_idx) {
		ret = platform_device_add_properties(xhci, props);
		if (ret) {
			dev_err(dwc->dev, "failed to add properties to xHCI\n");
			goto err1;
		}
	}

	phy_create_lookup(dwc->usb2_generic_phy, "usb2-phy",
			  dev_name(dwc->dev));
	phy_create_lookup(dwc->usb3_generic_phy, "usb3-phy",
			  dev_name(dwc->dev));

	ret = platform_device_add(xhci);
	if (ret) {
		dev_err(dwc->dev, "failed to register xHCI device\n");
		goto err2;
	}

	return 0;
err2:
	phy_remove_lookup(dwc->usb2_generic_phy, "usb2-phy",
			  dev_name(dwc->dev));
	phy_remove_lookup(dwc->usb3_generic_phy, "usb3-phy",
			  dev_name(dwc->dev));
err1:
	platform_device_put(xhci);
	return ret;
}

void dwc3_host_exit(struct dwc3 *dwc)
{
	phy_remove_lookup(dwc->usb2_generic_phy, "usb2-phy",
			  dev_name(dwc->dev));
	phy_remove_lookup(dwc->usb3_generic_phy, "usb3-phy",
			  dev_name(dwc->dev));
	platform_device_unregister(dwc->xhci);
}
