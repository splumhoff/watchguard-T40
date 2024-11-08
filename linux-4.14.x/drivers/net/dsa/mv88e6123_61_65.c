/*
 * net/dsa/mv88e6123_61_65.c - Marvell 88e6123/6161/6165 switch chip support
 * Copyright (c) 2008-2009 Marvell Semiconductor
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/phy.h>
#include <net/dsa.h>
#include "mv88e6xxx.h"

#ifdef	CONFIG_WG_PLATFORM

#define	pd		cd
#define	phys_port_mask	enabled_port_mask 

// Define which ports are CPU ports
static	int		CPU0;
static	int		CPU1;

#define	CPU0_BIT	(1 << DSA_PHY_MAP(CPU0))
#define	CPU1_BIT	(1 << DSA_PHY_MAP(CPU1))

#define	PORT_SWITCH_ID_6190	0x1900

void wg_dsa_88e617x_init(struct mii_bus *bus, int sw_addr)
{
	int j;
	u16 cpumap = (wg_dsa_sw0_map[0] | wg_dsa_sw0_map[1]) & 0x60;

	// Set up so all ports talk only to CPU
	for (j = 0; j <= 6; j++)
	if  (j <= 4) {
		__mv88e6xxx_reg_write(bus, sw_addr, 16|j, 6, cpumap);
		__mv88e6xxx_reg_write(bus, sw_addr, 16|j, 4, 0x007F);
		__mv88e6xxx_reg_write(bus, sw_addr,    j, 0, 0x1140);
		printk(KERN_ERR "%s:  phy %2d id %4x %4x\n",
		       __FUNCTION__, j,
		       __mv88e6xxx_reg_read(bus, sw_addr, j, 2),
		       __mv88e6xxx_reg_read(bus, sw_addr, j, 3));
	} else
	if  (cpumap & (1 << j)) {
		__mv88e6xxx_reg_write(bus, sw_addr, 16|j, 6, 0x001F);
		__mv88e6xxx_reg_write(bus, sw_addr, 16|j, 4, 0x007F);
		printk(KERN_ERR "%s:  cpu %2d id %4x %4x\n",
		       __FUNCTION__, j,
		       __mv88e6xxx_reg_read(bus, sw_addr, j, 2),
		       __mv88e6xxx_reg_read(bus, sw_addr, j, 3));
	} else {
		__mv88e6xxx_reg_write(bus, sw_addr, 16|j, 6, 0x0000);
		__mv88e6xxx_reg_write(bus, sw_addr, 16|j, 4, 0x007C);
	}
}

#endif

static struct mv88e6xxx_priv_state priv_state[2];

static const char *mv88e6123_61_65_probe(struct device *dsa_dev,
				   struct device *host_dev, int sw_addr,
				   void **priv)
{
	struct mii_bus *bus = dsa_host_dev_to_mii_bus(host_dev);
	int ret;
	int sw = sw_addr & 1;

	if (sw_addr) {
		*priv = &priv_state[sw];
		wg_dsa_host[sw]   = host_dev;
		wg_dsa_parent[sw] = dsa_dev;
		printk(KERN_INFO "%s: SW%02d  parent %s host %s\n",
		       __FUNCTION__, sw_addr,
		       dsa_dev  ? dsa_dev->init_name  : "(nodev)",
		       host_dev ? host_dev->init_name : "(nodev)");
	}

	if (bus == NULL)
		return NULL;

	ret = __mv88e6xxx_reg_read(bus, sw_addr, REG_PORT(0), PORT_SWITCH_ID);
	if (ret >= 0) {
		if (ret == PORT_SWITCH_ID_6123_A1)
			return "Marvell 88E6123 (A1)";
		if (ret == PORT_SWITCH_ID_6123_A2)
			return "Marvell 88E6123 (A2)";
		if ((ret & 0xfff0) == PORT_SWITCH_ID_6123)
			return "Marvell 88E6123";

		if (ret == PORT_SWITCH_ID_6161_A1)
			return "Marvell 88E6161 (A1)";
		if (ret == PORT_SWITCH_ID_6161_A2)
			return "Marvell 88E6161 (A2)";
		if ((ret & 0xfff0) == PORT_SWITCH_ID_6161)
			return "Marvell 88E6161";

		if (ret == PORT_SWITCH_ID_6165_A1)
			return "Marvell 88E6165 (A1)";
		if (ret == PORT_SWITCH_ID_6165_A2)
			return "Marvell 88e6165 (A2)";
		if ((ret & 0xfff0) == PORT_SWITCH_ID_6165)
			return "Marvell 88E6165";
#ifdef	CONFIG_WG_PLATFORM
		// Recognize type of MV88E61XX switch chip
		if ((ret & 0x0fff0) == PORT_SWITCH_ID_6171) {
			wg_dsa_type =                 61710;
			printk(KERN_INFO  "%s: SW%02d  88E6171\n",
				__FUNCTION__, sw_addr);
			return (wg_dsa_count > 0) ? "Marvell 88E6171" : NULL;
		}
		if ((ret & 0x0fff0) == PORT_SWITCH_ID_6176) {
			wg_dsa_type =                 61760;
			printk(KERN_INFO  "%s: SW%02d  88E6176\n",
				__FUNCTION__, sw_addr);
			wg_dsa_88e617x_init(bus, sw_addr);
			return (wg_dsa_count > 0) ? "Marvell 88E6176" : NULL;
		}
		if ((ret & 0x0fff0) == PORT_SWITCH_ID_6190) {
			wg_dsa_type =                 61900;
			wg_dsa_phy_num =              8;
			printk(KERN_INFO  "%s: SW%02d  88E6190\n",
				__FUNCTION__, sw_addr);
			return (wg_dsa_count > 0) ? "Marvell 88E6190" : NULL;
		}
		else
			printk(KERN_EMERG "%s: SW%02d  ID %4x Unknown\n",
				__FUNCTION__, sw_addr, ret);

		// Try single chip addressing mode as well
		if (sw_addr)
			return mv88e6123_61_65_probe(dsa_dev, host_dev, 0, priv);
#endif	// CONFIG_WG_PLATFORM
	}

	return NULL;
}

#ifdef	CONFIG_WG_PLATFORM  // WG:JB WG functions

static int mv88e6123_61_65_switch_reset(struct dsa_switch *ds)
{
	int i;
	int ret;
	unsigned long timeout;

	/* Set all ports to the disabled state. */
	wg_dsa_sw[ds->pd->sw_addr & 1] = ds; // Export ds to wg_dsa.ko

	for (i = 0; i < DSA_PORT; i++) {
		ret = REG_READ(REG_PORT(i), 0x04);
		REG_WRITE(REG_PORT(i), 0x04, ret & 0xfffc);
	}

	/* Wait for transmit queues to drain. */
	usleep_range(2000, 4000);

	/* Reset the switch. */
#ifdef	CONFIG_WG_PLATFORM // WG:JB // 6171/6176 have different reset bits
	if ((ds == wg_dsa_sw[0]) || (wg_dsa_count >= 1)) {
	if (wg_dsa_type < 61760)
		REG_WRITE(REG_GLOBAL, 0x04, 0x8000);
	else
		REG_WRITE(REG_GLOBAL, 0x04, 0xc001);
	printk(KERN_DEBUG  "%s: 88E%4d ADDR %2d soft reset\n",
	       __FUNCTION__, wg_dsa_type/10, ds->pd->sw_addr);
#ifdef	CONFIG_WG_ARCH_X86	// Only on 6176 on X86
	if (wg_dsa_type >= 61900)
		mdelay(100); // WG:JB Add delay for 6190
	else
	if (wg_dsa_type >= 61760) {
		REG_WRITE(0x0F, 0x16, 0x0001);
		REG_WRITE(0x0F, 0x00, 0x9340);
	}
#endif
        }
#else
	REG_WRITE(REG_GLOBAL, 0x04, 0x8000);
#endif

	/* Wait up to one second for reset to complete. */
	timeout = jiffies + 1 * HZ;
	while (time_before(jiffies, timeout)) {
		ret = REG_READ(REG_GLOBAL, 0x00);
		if ((ret & 0x0800) == 0x0800) // 6171 has different reset bits
			break;

		usleep_range(1000, 2000);
	}
	if (time_after(jiffies, timeout))
		return -ETIMEDOUT;

	return 0;
}

static int mv88e6123_61_65_port_to_phy_addr(int port)
{
#ifdef	CONFIG_WG_PLATFORM
	if (port >= 0 && port < DSA_PORT)
#else
	if (port >= 0 && port <= 4)
#endif
		return port;
	return -1;
}

static int
mv88e6123_61_65_phy_read(struct dsa_switch *ds, int port, int regnum)
{
	int addr = mv88e6123_61_65_port_to_phy_addr(port);
#ifdef	CONFIG_WG_ARCH_FREESCALE
	if (wg_dsa_count  == 2) // Use PHY cross connect for RGMII port
	if (unlikely(port == 6)) {
		if (ds->pd->sw_addr & 1) addr = 0;
		else
		if (ds->pd->sw_addr > 0) addr = 4;
	}
#endif
	return mv88e6xxx_phy_read(ds, addr, regnum);
}

static int
mv88e6123_61_65_phy_write(struct dsa_switch *ds,
			      int port, int regnum, u16 val)
{
	int addr = mv88e6123_61_65_port_to_phy_addr(port);
	return mv88e6xxx_phy_write(ds, addr, regnum, val);
}

#endif	// CONFIG_WG_PLATFORM

static int mv88e6123_61_65_setup_global(struct dsa_switch *ds)
{
	int ret;
	int i;

	/* Disable the PHY polling unit (since there won't be any
	 * external PHYs to poll), don't discard packets with
	 * excessive collisions, and mask all interrupt sources.
	 */
#ifdef	CONFIG_WG_PLATFORM
	// Skip this on 6176
	if (wg_dsa_type < 61760)
	REG_WRITE(REG_GLOBAL, 0x04, 0x0000);
	// Reset ATU as well
	REG_WRITE(REG_GLOBAL, 0x0B, 0xA000);
#else	// CONFIG_WG_PLATFORM
	REG_WRITE(REG_GLOBAL, 0x04, 0x0000);
#endif	// CONFIG_WG_PLATFORM

	/* Set the default address aging time to 5 minutes, and
	 * enable address learn messages to be sent to all message
	 * ports.
	 */
#ifdef	CONFIG_WG_PLATFORM
	if (wg_dsa_type >= 61760)
	REG_WRITE(REG_GLOBAL, 0x0a, 0x0149);
	else
#endif
	REG_WRITE(REG_GLOBAL, 0x0a, 0x0148);

	/* Configure the priority mapping registers. */
	ret = mv88e6xxx_config_prio(ds);
	if (ret < 0)
		return ret;

	/* Configure the upstream port, and configure the upstream
	 * port as the port to which ingress and egress monitor frames
	 * are to be sent.
	 */
#ifdef	CONFIG_WG_PLATFORM
	if (wg_dsa_type >= 61900) {
	REG_WRITE(REG_GLOBAL, 0x1a, 0xa000 | DSA_PHY_MAP(dsa_upstream_port(ds)));
	REG_WRITE(REG_GLOBAL, 0x1a, 0xa100 | DSA_PHY_MAP(dsa_upstream_port(ds)));
	REG_WRITE(REG_GLOBAL, 0x1a, 0xa200 | DSA_PHY_MAP(dsa_upstream_port(ds)));
	} else
#endif
	REG_WRITE(REG_GLOBAL, 0x1a, (dsa_upstream_port(ds) * 0x1110));

	/* Disable remote management for now, and set the switch's
	 * DSA device number.
	 */
	REG_WRITE(REG_GLOBAL, 0x1c, ds->index & 0x1f);

	/* Send all frames with destination addresses matching
	 * 01:80:c2:00:00:2x to the CPU port.
	 */
	REG_WRITE(REG_GLOBAL2, 0x02, 0xffff);

	/* Send all frames with destination addresses matching
	 * 01:80:c2:00:00:0x to the CPU port.
	 */
	REG_WRITE(REG_GLOBAL2, 0x03, 0xffff);

	/* Disable the loopback filter, disable flow control
	 * messages, disable flood broadcast override, disable
	 * removing of provider tags, disable ATU age violation
	 * interrupts, disable tag flow control, force flow
	 * control priority to the highest, and send all special
	 * multicast frames to the CPU at the highest priority.
	 */
#ifdef	CONFIG_WG_PLATFORM // WG:JB Enable Flood broadcast
	if (wg_dsa_L2_offset[ds->pd->sw_addr & 1] & MARVELL_FLAG)
		REG_WRITE(REG_GLOBAL2, 0x05, (wg_dsa_flood & 8) ? 0x10ff : 0x10f0);
	else
#endif	// CONFIG_WG_PLATFORM
	REG_WRITE(REG_GLOBAL2, 0x05, 0x00ff);

	/* Program the DSA routing table. */
	for (i = 0; i < 32; i++) {
		int nexthop;

		nexthop = 0x1f;

		REG_WRITE(REG_GLOBAL2, 0x06, 0x8000 | (i << 8) | nexthop);
	}

	/* Clear all trunk masks. */
	for (i = 0; i < 8; i++)
#ifdef	CONFIG_WG_PLATFORM
	if (wg_dsa_type <= 61760)
		REG_WRITE(REG_GLOBAL2, 0x07, 0x8000 | (i << 12) | 0x07f);
	else
		REG_WRITE(REG_GLOBAL2, 0x07, 0x8000 | (i << 12) | 0x7ff);
#else
		REG_WRITE(REG_GLOBAL2, 0x07, 0x8000 | (i << 12) | 0xff);
#endif

	/* Clear all trunk mappings. */
	for (i = 0; i < 16; i++)
		REG_WRITE(REG_GLOBAL2, 0x08, 0x8000 | (i << 11));

	/* Disable ingress rate limiting by resetting all ingress
	 * rate limit registers to their initial state.
	 */
#ifdef	CONFIG_WG_PLATFORM
	for (i = 0; i < DSA_PORT; i++)
#else	// CONFIG_WG_PLATFORM
	for (i = 0; i < 6; i++)
#endif	// CONFIG_WG_PLATFORM
		REG_WRITE(REG_GLOBAL2, 0x09, 0x9000 | (i << 8));

	/* Initialise cross-chip port VLAN table to reset defaults. */
	REG_WRITE(REG_GLOBAL2, 0x0b, 0x9000);

	/* Clear the priority override table. */
	for (i = 0; i < 16; i++)
		REG_WRITE(REG_GLOBAL2, 0x0f, 0x8000 | (i << 8));

	/* @@@ initialise AVB (22/23) watchdog (27) sdet (29) registers */

	return 0;
}

static int mv88e6123_61_65_setup_port(struct dsa_switch *ds, int p)
{
	int addr = REG_PORT(p);
	u16 val;

	/* MAC Forcing register: don't force link, speed, duplex
	 * or flow control state to any particular values on physical
	 * ports, but force the CPU port and all DSA ports to 1000 Mb/s
	 * full duplex.
	 */
#ifdef	CONFIG_WG_PLATFORM // WG:JB Set RGMII timing per vendor's requirements
	u16 ctl;
	u16 tag = ETH_P_EDSA; // Assume EDSA

	// Set which ports are CPU ports based on platform
	CPU0 = DSA_PHY;
	CPU1 = (wg_dsa_count == 2) ? DSA_PHY : (DSA_PHY + 1);

	ctl = (wg_dsa_type >= 61900) ? 0x313f : 0xc03e;

	if (p == (CPU0 + 0))
		REG_WRITE(addr, 0x01, ctl);
	else
	if (p == (CPU0 + 1)) {
		if (wg_dsa_count <= 0) {
			printk(KERN_EMERG "%s: No DSA Switch\n", __FUNCTION__);
			return -ENODEV;
		}
		if (wg_dsa_count == 1) {
			// CPU ports get RX and TX delays
			REG_WRITE(addr, 0x01, ctl);
		}
#ifdef	CONFIG_WG_ARCH_FREESCALE
		if (wg_dsa_count == 2) {
			// Cross ports get only RX delays
			REG_WRITE(addr, 0x01, ctl &= ~0x4000);
		}
#endif
	}
	else
	if (wg_dsa_type >= 61900)
		REG_WRITE(addr, 0x01, 0x0103);
#else	// CONFIG_WG_PLATFORM
 	if (dsa_is_cpu_port(ds, p) || ds->dsa_port_mask & (1 << p))
 		REG_WRITE(addr, 0x01, 0x003e);
#endif	// CONFIG_WG_PLATFORM
	else
		REG_WRITE(addr, 0x01, 0x0003);

	/* Do not limit the period of time that this port can be
	 * paused for by the remote end or the period of time that
	 * this port can pause the remote end.
	 */
	REG_WRITE(addr, 0x02, 0x0000);

	/* Port Control: disable Drop-on-Unlock, disable Drop-on-Lock,
	 * disable Header mode, enable IGMP/MLD snooping, disable VLAN
	 * tunneling, determine priority by looking at 802.1p and IP
	 * priority fields (IP prio has precedence), and set STP state
	 * to Forwarding.
	 *
	 * If this is the CPU link, use DSA or EDSA tagging depending
	 * on which tagging mode was configured.
	 *
	 * If this is a link to another switch, use DSA tagging mode.
	 *
	 * If this is the upstream port for this switch, enable
	 * forwarding of unknown unicasts and multicasts.
	 */
	val = 0x0433;
#ifdef	CONFIG_WG_PLATFORM
	if (dsa_is_cpu_port(ds, p) || p == CPU0 || p == CPU1) {
		if (ds->ops->get_tag_protocol(ds) == DSA_TAG_PROTO_EDSA)
#else
	if (dsa_is_cpu_port(ds, p)) {
		if (ds->dst->tag_protocol == DSA_TAG_PROTO_EDSA)
#endif
			val |= 0x3300;
		else
			val |= 0x0100;
	}
	if (ds->dsa_port_mask & (1 << p))
		val |= 0x0100;
	if (p == dsa_upstream_port(ds))
		val |= 0x000c;
	REG_WRITE(addr, 0x04, val);

#ifdef	CONFIG_WG_PLATFORM
	/* Port Control 1: disable trunking.  Also, if this is the
	 * CPU port, enable learn messages to be sent to this port.
	 */
	ctl = val;

	/* Enable learning on all CPU ports */
	REG_WRITE(addr, 0x05, ((p == CPU0) || (p == CPU1)) ? 0x8000 : 0x0000);

	/* Port based VLAN map: give each port its own address
	 * database, allow the CPU port to talk to each of the 'real'
	 * ports, and allow each of the 'real' ports to only talk to
	 * the upstream port.
	 */
	val = (p & 0xf) << 12;
	/* Force CPU Port VLAN Tag out of the way */
	if ((p == CPU0) || (p == CPU1))
		val  = ds->phys_port_mask | (0xf << 12);
	else
		val |= 1 << dsa_upstream_port(ds);

	/* Set up port maps from table */
	if ((p == CPU0) || (p == CPU1))
		val |= ds->dsa_port_mask;
	else
	if (((ds->dsa_port_mask | ds->phys_port_mask) & (1 << p)) == 0)
		val &= 0xf000;

	if (ds->pd->sw_addr & 1)
		val = wg_dsa_sw1_map[p] ? wg_dsa_sw1_map[p] : val;
	else
		val = wg_dsa_sw0_map[p] ? wg_dsa_sw0_map[p] : val;

	// If Marvell header mode flood all unknown DA's
	if (wg_dsa_flood & 7)
	if (wg_dsa_L2_offset[ds->pd->sw_addr & 1] & MARVELL_FLAG)
		REG_WRITE(addr, 0x04, ctl |= 0x000c);

	printk(KERN_DEBUG "%s: cpu %d/%d port %d/%d/%d ctl %04x map %04x\n",
	       __FUNCTION__, CPU0, CPU1, ds->index, ds->pd->sw_addr,
	       p, ctl, val);
	REG_WRITE(addr, 0x06, val);

	/* Default VLAN ID and priority: don't set a default VLAN
	 * ID, and set the default packet priority to zero.
	 */
	REG_WRITE(addr, 0x07, 0x0000);
#endif

	/* Port Control 2: don't force a good FCS, set the maximum
	 * frame size to 10240 bytes, don't let the switch add or
	 * strip 802.1q tags, don't discard tagged or untagged frames
	 * on this port, do a destination address lookup on all
	 * received packets as usual, disable ARP mirroring and don't
	 * send a copy of all transmitted/received frames on this port
	 * to the CPU.
	 */
	REG_WRITE(addr, 0x08, 0x2080);

	/* Egress rate control: disable egress rate control. */
	REG_WRITE(addr, 0x09, 0x0001);

	/* Egress rate control 2: disable egress rate control. */
	REG_WRITE(addr, 0x0a, 0x0000);

	/* Port Association Vector: when learning source addresses
	 * of packets, add the address to the address database using
	 * a port bitmap that has only the bit for this port set and
	 * the other bits clear.
	 */
#ifdef	CONFIG_WG_PLATFORM  // WG:JB Prevent static entry station moves
	val = (1 << DSA_PHY_MAP(p));
	if (wg_dsa_lock_static)
		REG_WRITE(addr, 0x0b, val | 0x1000);
	else
		REG_WRITE(addr, 0x0b, val | 0x0000);
#else	// CONFIG_WG_PLATFORM
	REG_WRITE(addr, 0x0b, 1 << p);
#endif	// CONFIG_WG_PLATFORM

	/* Port ATU control: disable limiting the number of address
	 * database entries that this port is allowed to use.
	 */
	REG_WRITE(addr, 0x0c, 0x0000);

	/* Priority Override: disable DA, SA and VTU priority override. */
	REG_WRITE(addr, 0x0d, 0x0000);

	/* Port Ethertype: use the Ethertype DSA Ethertype value. */
#ifdef	CONFIG_WG_PLATFORM // WG:JB Set tag if EDSA else 0
	REG_WRITE(addr, 0x0f, tag);
#else	// CONFIG_WG_PLATFORM
 	REG_WRITE(addr, 0x0f, ETH_P_EDSA);
#endif	// CONFIG_WG_PLATFORM

	/* Tag Remap: use an identity 802.1p prio -> switch prio
	 * mapping.
	 */
#ifdef	CONFIG_WG_PLATFORM // WG:JB This register is only on 617x
	if (wg_dsa_type <= 61760)
#endif
	REG_WRITE(addr, 0x18, 0x3210);

	/* Tag Remap 2: use an identity 802.1p prio -> switch prio
	 * mapping.
	 */
#ifdef	CONFIG_WG_PLATFORM // WG:JB This register is only on 617x
	if (wg_dsa_type <= 61760)
#endif
	REG_WRITE(addr, 0x19, 0x7654);

#ifdef	CONFIG_WG_PLATFORM
	return 0;
#else

	return mv88e6xxx_setup_port_common(ds, p);

#endif	// CONFIG_WG_PLATFORM
}

static int mv88e6123_61_65_setup(struct dsa_switch *ds)
{
	struct mv88e6xxx_priv_state *ps = ds_to_priv(ds);
	int i;
	int ret;

#ifdef	CONFIG_WG_PLATFORM // WG:JB Setup private state
	mutex_init(&ps->smi_mutex);
	mutex_init(&ps->stats_mutex);
	mutex_init(&ps->phy_mutex);

	ps->id = wg_dsa_type & 0xfff0;
	ps->num_ports = (wg_dsa_type < 61900) ? 7 : 10;

	ret = mv88e6123_61_65_switch_reset(ds);
#else	// CONFIG_WG_PLATFORM
	ret = mv88e6xxx_setup_common(ds);
	if (ret < 0)
		return ret;

	switch (ps->id) {
	case PORT_SWITCH_ID_6123:
		ps->num_ports = 3;
		break;
	case PORT_SWITCH_ID_6161:
	case PORT_SWITCH_ID_6165:
		ps->num_ports = 6;
		break;
	default:
		return -ENODEV;
	}

	ret = mv88e6xxx_switch_reset(ds, false);
#endif	// CONFIG_WG_PLATFORM
	if (ret < 0)
		return ret;

	/* @@@ initialise vtu and atu */

	ret = mv88e6123_61_65_setup_global(ds);
	if (ret < 0)
		return ret;

	for (i = 0; i < ps->num_ports; i++) {
		ret = mv88e6123_61_65_setup_port(ds, i);
		if (ret < 0)
			return ret;
	}

#ifdef	CONFIG_WG_PLATFORM // WG:JB Release MDIO if needed
	if (wg_dsa_mdio_release) wg_dsa_mdio_release();
#endif

	return 0;
}

#ifdef CONFIG_WG_PLATFORM
static inline enum dsa_tag_protocol mv88e6123_61_65_get_tag_protocol(struct dsa_switch *ds)
{
	return DSA_TAG_PROTO_DSA;
}
#endif

struct dsa_switch_ops mv88e6123_61_65_switch_ops = {
#ifdef CONFIG_WG_PLATFORM
	.get_tag_protocol	= mv88e6123_61_65_get_tag_protocol,
#endif
	.probe			= mv88e6123_61_65_probe,
	.setup			= mv88e6123_61_65_setup,
	.set_addr		= mv88e6xxx_set_addr_indirect,
#ifdef	CONFIG_WG_PLATFORM // WG:JB Use our routine
	.phy_read		= mv88e6123_61_65_phy_read,
	.phy_write		= mv88e6123_61_65_phy_write,
	.poll_link		= mv88e6xxx_poll_link,
#else	// CONFIG_WG_PLATFORM
	.phy_read		= mv88e6xxx_phy_read,
	.phy_write		= mv88e6xxx_phy_write,
#endif	// CONFIG_WG_PLATFORM
	.get_strings		= mv88e6xxx_get_strings,
	.get_ethtool_stats	= mv88e6xxx_get_ethtool_stats,
	.get_sset_count		= mv88e6xxx_get_sset_count,
#ifdef CONFIG_NET_DSA_HWMON
	.get_temp		= mv88e6xxx_get_temp,
#endif
	.get_regs_len		= mv88e6xxx_get_regs_len,
	.get_regs		= mv88e6xxx_get_regs,
};

struct dsa_switch_driver mv88e6123_61_65_switch_driver = {
	.ops = &mv88e6123_61_65_switch_ops
};

MODULE_ALIAS("platform:mv88e6123");
MODULE_ALIAS("platform:mv88e6161");
MODULE_ALIAS("platform:mv88e6165");
