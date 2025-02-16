/*
 * net/dsa/slave.c - Slave device handling
 * Copyright (c) 2008-2009 Marvell Semiconductor
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/list.h>
#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/phy.h>
#include <linux/phy_fixed.h>
#include <linux/of_net.h>
#include <linux/of_mdio.h>
#include <linux/mdio.h>
#include <linux/list.h>
#include <net/rtnetlink.h>
#include <net/pkt_cls.h>
#include <net/tc_act/tc_mirred.h>
#include <linux/if_bridge.h>
#include <linux/netpoll.h>

#include "dsa_priv.h"

static bool dsa_slave_dev_check(struct net_device *dev);

/* slave mii_bus handling ***************************************************/
static int dsa_slave_phy_read(struct mii_bus *bus, int addr, int reg)
{
	struct dsa_switch *ds = bus->priv;

	if (ds->phys_mii_mask & (1 << addr))
		return ds->ops->phy_read(ds, addr, reg);

	return 0xffff;
}

static int dsa_slave_phy_write(struct mii_bus *bus, int addr, int reg, u16 val)
{
	struct dsa_switch *ds = bus->priv;

	if (ds->phys_mii_mask & (1 << addr))
		return ds->ops->phy_write(ds, addr, reg, val);

	return 0;
}

void dsa_slave_mii_bus_init(struct dsa_switch *ds)
{
	ds->slave_mii_bus->priv = (void *)ds;
	ds->slave_mii_bus->name = "dsa slave smi";
	ds->slave_mii_bus->read = dsa_slave_phy_read;
	ds->slave_mii_bus->write = dsa_slave_phy_write;
#ifndef CONFIG_WG_PLATFORM_DSA_MODE_T80
	ds->slave_mii_bus->name = (ds->cd->sw_addr & 1) ? "sw11" : "sw10";
	snprintf(ds->slave_mii_bus->id, MII_BUS_ID_SIZE, "dsa-%d.%d",
		 ds->dst->tree, ds->cd->sw_addr);
	printk(KERN_INFO "%s: %s\n", __FUNCTION__, ds->slave_mii_bus->id);
#endif // CONFIG_WG_PLATFORM_DSA_MODE_T80
	ds->slave_mii_bus->parent = ds->dev;
	ds->slave_mii_bus->phy_mask = ~ds->phys_mii_mask;
}


/* slave device handling ****************************************************/
static int dsa_slave_get_iflink(const struct net_device *dev)
{
	struct dsa_slave_priv *p = netdev_priv(dev);

	return dsa_master_netdev(p)->ifindex;
}

static int dsa_slave_open(struct net_device *dev)
{
	struct dsa_slave_priv *p = netdev_priv(dev);
	struct dsa_port *dp = p->dp;
	struct dsa_switch *ds = dp->ds;
	struct net_device *master = dsa_master_netdev(p);
	u8 stp_state = dp->bridge_dev ? BR_STATE_BLOCKING : BR_STATE_FORWARDING;
	int err;
#ifndef CONFIG_WG_PLATFORM_DSA_MODE_T80
#ifdef	CONFIG_WG_PLATFORM // WG:JB Power up
	if (p->phy != NULL) genphy_resume(p->phy);
#endif	// CONFIG_WG_PLATFORM
#endif // CONFIG_WG_PLATFORM_DSA_MODE_T80
	if (!(master->flags & IFF_UP))
		return -ENETDOWN;

	if (!ether_addr_equal(dev->dev_addr, master->dev_addr)) {
		err = dev_uc_add(master, dev->dev_addr);
		if (err < 0)
			goto out;
	}

	if (dev->flags & IFF_ALLMULTI) {
		err = dev_set_allmulti(master, 1);
		if (err < 0)
			goto del_unicast;
	}
	if (dev->flags & IFF_PROMISC) {
		err = dev_set_promiscuity(master, 1);
		if (err < 0)
			goto clear_allmulti;
	}

	if (ds->ops->port_enable) {
		err = ds->ops->port_enable(ds, p->dp->index, p->phy);
		if (err)
			goto clear_promisc;
	}

	dsa_port_set_state_now(p->dp, stp_state);

#ifdef CONFIG_WG_PLATFORM_DSA_MODE_T80	// WG:XD FBX-17816 disable gen phy link polling for
	if (p->phy)			// non-T80 platforms (relies on legacy wg_dsa link polling)
		phy_start(p->phy);
#endif
	return 0;

clear_promisc:
	if (dev->flags & IFF_PROMISC)
		dev_set_promiscuity(master, -1);
clear_allmulti:
	if (dev->flags & IFF_ALLMULTI)
		dev_set_allmulti(master, -1);
del_unicast:
	if (!ether_addr_equal(dev->dev_addr, master->dev_addr))
		dev_uc_del(master, dev->dev_addr);
out:
	return err;
}

static int dsa_slave_close(struct net_device *dev)
{
	struct dsa_slave_priv *p = netdev_priv(dev);
	struct net_device *master = dsa_master_netdev(p);
	struct dsa_switch *ds = p->dp->ds;

	if (p->phy)
		phy_stop(p->phy);
#ifndef CONFIG_WG_PLATFORM_DSA_MODE_T80
#ifdef	CONFIG_WG_PLATFORM // WG:JB Power down
	if (p->phy != NULL) genphy_suspend(p->phy);
#endif	// CONFIG_WG_PLATFORM
#endif // CONFIG_WG_PLATFORM_DSA_MODE_T80
	dev_mc_unsync(master, dev);
	dev_uc_unsync(master, dev);
	if (dev->flags & IFF_ALLMULTI)
		dev_set_allmulti(master, -1);
#ifndef CONFIG_WG_PLATFORM_DSA_MODE_T80	// WG:XD FBX-18636
	if (dev->flags & IFF_PROMISC)
		dev_set_promiscuity(master, -1);
#endif
	if (!ether_addr_equal(dev->dev_addr, master->dev_addr))
		dev_uc_del(master, dev->dev_addr);

	if (ds->ops->port_disable)
		ds->ops->port_disable(ds, p->dp->index, p->phy);

	dsa_port_set_state_now(p->dp, BR_STATE_DISABLED);

	return 0;
}

static void dsa_slave_change_rx_flags(struct net_device *dev, int change)
{
	struct dsa_slave_priv *p = netdev_priv(dev);
	struct net_device *master = dsa_master_netdev(p);

	if (dev->flags & IFF_UP) {
		if (change & IFF_ALLMULTI)
			dev_set_allmulti(master, dev->flags & IFF_ALLMULTI ? 1 : -1);
		if (change & IFF_PROMISC)
		dev_set_promiscuity(master, dev->flags & IFF_PROMISC ? 1 : -1);
	}
}

static void dsa_slave_set_rx_mode(struct net_device *dev)
{
	struct dsa_slave_priv *p = netdev_priv(dev);
	struct net_device *master = dsa_master_netdev(p);

	dev_mc_sync(master, dev);
	dev_uc_sync(master, dev);
}

static int dsa_slave_set_mac_address(struct net_device *dev, void *a)
{
	struct dsa_slave_priv *p = netdev_priv(dev);
	struct net_device *master = dsa_master_netdev(p);
	struct sockaddr *addr = a;
	int err;

	if (!is_valid_ether_addr(addr->sa_data))
		return -EADDRNOTAVAIL;

	if (!(dev->flags & IFF_UP))
		goto out;

	if (!ether_addr_equal(addr->sa_data, master->dev_addr)) {
		err = dev_uc_add(master, addr->sa_data);
		if (err < 0)
			return err;
	}

	if (!ether_addr_equal(dev->dev_addr, master->dev_addr))
		dev_uc_del(master, dev->dev_addr);

out:
	ether_addr_copy(dev->dev_addr, addr->sa_data);
#ifndef CONFIG_WG_PLATFORM_DSA_MODE_T80
#ifdef	CONFIG_WG_PLATFORM // WG:JB Set static ATU entry when changing MACs
	{
		struct dsa_switch  *ds = p->dp->ds;
		if (ds)
		if (ds->ops->set_static)
		    ds->ops->set_static(ds, p->dp->index, NULL, 1);
	}
#endif	// CONFIG_WG_PLATFORM
#endif // CONFIG_WG_PLATFORM_DSA_MODE_T80
	return 0;
}

struct dsa_slave_dump_ctx {
	struct net_device *dev;
	struct sk_buff *skb;
	struct netlink_callback *cb;
	int idx;
};

static int
dsa_slave_port_fdb_do_dump(const unsigned char *addr, u16 vid,
			   bool is_static, void *data)
{
	struct dsa_slave_dump_ctx *dump = data;
	u32 portid = NETLINK_CB(dump->cb->skb).portid;
	u32 seq = dump->cb->nlh->nlmsg_seq;
	struct nlmsghdr *nlh;
	struct ndmsg *ndm;

	if (dump->idx < dump->cb->args[2])
		goto skip;

	nlh = nlmsg_put(dump->skb, portid, seq, RTM_NEWNEIGH,
			sizeof(*ndm), NLM_F_MULTI);
	if (!nlh)
		return -EMSGSIZE;

	ndm = nlmsg_data(nlh);
	ndm->ndm_family  = AF_BRIDGE;
	ndm->ndm_pad1    = 0;
	ndm->ndm_pad2    = 0;
	ndm->ndm_flags   = NTF_SELF;
	ndm->ndm_type    = 0;
	ndm->ndm_ifindex = dump->dev->ifindex;
	ndm->ndm_state   = is_static ? NUD_NOARP : NUD_REACHABLE;

	if (nla_put(dump->skb, NDA_LLADDR, ETH_ALEN, addr))
		goto nla_put_failure;

	if (vid && nla_put_u16(dump->skb, NDA_VLAN, vid))
		goto nla_put_failure;

	nlmsg_end(dump->skb, nlh);

skip:
	dump->idx++;
	return 0;

nla_put_failure:
	nlmsg_cancel(dump->skb, nlh);
	return -EMSGSIZE;
}

static int
dsa_slave_fdb_dump(struct sk_buff *skb, struct netlink_callback *cb,
		   struct net_device *dev, struct net_device *filter_dev,
		   int *idx)
{
	struct dsa_slave_dump_ctx dump = {
		.dev = dev,
		.skb = skb,
		.cb = cb,
		.idx = *idx,
	};
	struct dsa_slave_priv *p = netdev_priv(dev);
	struct dsa_port *dp = p->dp;
	struct dsa_switch *ds = dp->ds;
	int err;

	if (!ds->ops->port_fdb_dump)
		return -EOPNOTSUPP;

	err = ds->ops->port_fdb_dump(ds, dp->index,
				     dsa_slave_port_fdb_do_dump,
				     &dump);
	*idx = dump.idx;
	return err;
}

static int dsa_slave_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	struct dsa_slave_priv *p = netdev_priv(dev);

	if (p->phy != NULL)
		return phy_mii_ioctl(p->phy, ifr, cmd);

	return -EOPNOTSUPP;
}

static int dsa_slave_port_attr_set(struct net_device *dev,
				   const struct switchdev_attr *attr,
				   struct switchdev_trans *trans)
{
	struct dsa_slave_priv *p = netdev_priv(dev);
	struct dsa_port *dp = p->dp;
	int ret;

	switch (attr->id) {
	case SWITCHDEV_ATTR_ID_PORT_STP_STATE:
		ret = dsa_port_set_state(dp, attr->u.stp_state, trans);
		break;
	case SWITCHDEV_ATTR_ID_BRIDGE_VLAN_FILTERING:
		ret = dsa_port_vlan_filtering(dp, attr->u.vlan_filtering,
					      trans);
		break;
	case SWITCHDEV_ATTR_ID_BRIDGE_AGEING_TIME:
		ret = dsa_port_ageing_time(dp, attr->u.ageing_time, trans);
		break;
	default:
		ret = -EOPNOTSUPP;
		break;
	}

	return ret;
}

static int dsa_slave_port_obj_add(struct net_device *dev,
				  const struct switchdev_obj *obj,
				  struct switchdev_trans *trans)
{
	struct dsa_slave_priv *p = netdev_priv(dev);
	struct dsa_port *dp = p->dp;
	int err;

	/* For the prepare phase, ensure the full set of changes is feasable in
	 * one go in order to signal a failure properly. If an operation is not
	 * supported, return -EOPNOTSUPP.
	 */

	switch (obj->id) {
	case SWITCHDEV_OBJ_ID_PORT_MDB:
		err = dsa_port_mdb_add(dp, SWITCHDEV_OBJ_PORT_MDB(obj), trans);
		break;
	case SWITCHDEV_OBJ_ID_PORT_VLAN:
		err = dsa_port_vlan_add(dp, SWITCHDEV_OBJ_PORT_VLAN(obj),
					trans);
		break;
	default:
		err = -EOPNOTSUPP;
		break;
	}

	return err;
}

static int dsa_slave_port_obj_del(struct net_device *dev,
				  const struct switchdev_obj *obj)
{
	struct dsa_slave_priv *p = netdev_priv(dev);
	struct dsa_port *dp = p->dp;
	int err;

	switch (obj->id) {
	case SWITCHDEV_OBJ_ID_PORT_MDB:
		err = dsa_port_mdb_del(dp, SWITCHDEV_OBJ_PORT_MDB(obj));
		break;
	case SWITCHDEV_OBJ_ID_PORT_VLAN:
		err = dsa_port_vlan_del(dp, SWITCHDEV_OBJ_PORT_VLAN(obj));
		break;
	default:
		err = -EOPNOTSUPP;
		break;
	}

	return err;
}

static int dsa_slave_port_attr_get(struct net_device *dev,
				   struct switchdev_attr *attr)
{
	struct dsa_slave_priv *p = netdev_priv(dev);
	struct dsa_switch *ds = p->dp->ds;

	switch (attr->id) {
	case SWITCHDEV_ATTR_ID_PORT_PARENT_ID:
		attr->u.ppid.id_len = sizeof(ds->index);
		memcpy(&attr->u.ppid.id, &ds->index, attr->u.ppid.id_len);
		break;
	case SWITCHDEV_ATTR_ID_PORT_BRIDGE_FLAGS_SUPPORT:
		attr->u.brport_flags_support = 0;
		break;
	default:
		return -EOPNOTSUPP;
	}

	return 0;
}

static inline netdev_tx_t dsa_netpoll_send_skb(struct dsa_slave_priv *p,
					       struct sk_buff *skb)
{
#ifdef CONFIG_NET_POLL_CONTROLLER
	if (p->netpoll)
		netpoll_send_skb(p->netpoll, skb);
#else
	BUG();
#endif
	return NETDEV_TX_OK;
}

static netdev_tx_t dsa_slave_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct dsa_slave_priv *p = netdev_priv(dev);
	struct pcpu_sw_netstats *s;
	struct sk_buff *nskb;

	s = this_cpu_ptr(p->stats64);
	u64_stats_update_begin(&s->syncp);
	s->tx_packets++;
	s->tx_bytes += skb->len;
	u64_stats_update_end(&s->syncp);

	/* Transmit function may have to reallocate the original SKB,
	 * in which case it must have freed it. Only free it here on error.
	 */
	nskb = p->xmit(skb, dev);
	if (!nskb) {
		kfree_skb(skb);
		return NETDEV_TX_OK;
	}

	/* SKB for netpoll still need to be mangled with the protocol-specific
	 * tag to be successfully transmitted
	 */
	if (unlikely(netpoll_tx_running(dev)))
		return dsa_netpoll_send_skb(p, nskb);

	/* Queue the SKB for transmission on the parent interface, but
	 * do not modify its EtherType
	 */
	nskb->dev = dsa_master_netdev(p);
	dev_queue_xmit(nskb);

	return NETDEV_TX_OK;
}

/* ethtool operations *******************************************************/
static int
dsa_slave_get_link_ksettings(struct net_device *dev,
			     struct ethtool_link_ksettings *cmd)
{
	struct dsa_slave_priv *p = netdev_priv(dev);

	if (!p->phy)
		return -EOPNOTSUPP;
#ifdef CONFIG_WG_PLATFORM // WG:XD FBX-20663, from dsa_slave_get_settings() in 3.12.19 kernel
	{
		int err;
		err = phy_read_status(p->phy);
		if (err == 0)
			phy_ethtool_ksettings_get(p->phy, cmd);
		return err;
	}
#else
	phy_ethtool_ksettings_get(p->phy, cmd);

	return 0;
#endif
}

static int
dsa_slave_set_link_ksettings(struct net_device *dev,
			     const struct ethtool_link_ksettings *cmd)
{
	struct dsa_slave_priv *p = netdev_priv(dev);

	if (p->phy != NULL)
		return phy_ethtool_ksettings_set(p->phy, cmd);

	return -EOPNOTSUPP;
}

static void dsa_slave_get_drvinfo(struct net_device *dev,
				  struct ethtool_drvinfo *drvinfo)
{
	strlcpy(drvinfo->driver, "dsa", sizeof(drvinfo->driver));
	strlcpy(drvinfo->fw_version, "N/A", sizeof(drvinfo->fw_version));
	strlcpy(drvinfo->bus_info, "platform", sizeof(drvinfo->bus_info));
}

static int dsa_slave_get_regs_len(struct net_device *dev)
{
	struct dsa_slave_priv *p = netdev_priv(dev);
	struct dsa_switch *ds = p->dp->ds;

	if (ds->ops->get_regs_len)
		return ds->ops->get_regs_len(ds, p->dp->index);

	return -EOPNOTSUPP;
}

static void
dsa_slave_get_regs(struct net_device *dev, struct ethtool_regs *regs, void *_p)
{
	struct dsa_slave_priv *p = netdev_priv(dev);
	struct dsa_switch *ds = p->dp->ds;

	if (ds->ops->get_regs)
		ds->ops->get_regs(ds, p->dp->index, regs, _p);
}

static int dsa_slave_nway_reset(struct net_device *dev)
{
	struct dsa_slave_priv *p = netdev_priv(dev);

	if (p->phy != NULL)
		return genphy_restart_aneg(p->phy);

	return -EOPNOTSUPP;
}

static u32 dsa_slave_get_link(struct net_device *dev)
{
	struct dsa_slave_priv *p = netdev_priv(dev);

	if (p->phy != NULL) {
		genphy_update_link(p->phy);
		return p->phy->link;
	}

	return -EOPNOTSUPP;
}

static int dsa_slave_get_eeprom_len(struct net_device *dev)
{
	struct dsa_slave_priv *p = netdev_priv(dev);
	struct dsa_switch *ds = p->dp->ds;

	if (ds->cd && ds->cd->eeprom_len)
		return ds->cd->eeprom_len;

	if (ds->ops->get_eeprom_len)
		return ds->ops->get_eeprom_len(ds);

	return 0;
}

static int dsa_slave_get_eeprom(struct net_device *dev,
				struct ethtool_eeprom *eeprom, u8 *data)
{
	struct dsa_slave_priv *p = netdev_priv(dev);
	struct dsa_switch *ds = p->dp->ds;

	if (ds->ops->get_eeprom)
		return ds->ops->get_eeprom(ds, eeprom, data);

	return -EOPNOTSUPP;
}

static int dsa_slave_set_eeprom(struct net_device *dev,
				struct ethtool_eeprom *eeprom, u8 *data)
{
	struct dsa_slave_priv *p = netdev_priv(dev);
	struct dsa_switch *ds = p->dp->ds;

	if (ds->ops->set_eeprom)
		return ds->ops->set_eeprom(ds, eeprom, data);

	return -EOPNOTSUPP;
}

static void dsa_slave_get_strings(struct net_device *dev,
				  uint32_t stringset, uint8_t *data)
{
	struct dsa_slave_priv *p = netdev_priv(dev);
	struct dsa_switch *ds = p->dp->ds;

	if (stringset == ETH_SS_STATS) {
		int len = ETH_GSTRING_LEN;

		strncpy(data, "tx_packets", len);
		strncpy(data + len, "tx_bytes", len);
		strncpy(data + 2 * len, "rx_packets", len);
		strncpy(data + 3 * len, "rx_bytes", len);
		if (ds->ops->get_strings)
			ds->ops->get_strings(ds, p->dp->index, data + 4 * len);
	}
}

static void dsa_cpu_port_get_ethtool_stats(struct net_device *dev,
					   struct ethtool_stats *stats,
					   uint64_t *data)
{
	struct dsa_switch_tree *dst = dev->dsa_ptr;
	struct dsa_port *cpu_dp = dsa_get_cpu_port(dst);
	struct dsa_switch *ds = cpu_dp->ds;
	s8 cpu_port = cpu_dp->index;
	int count = 0;

	if (cpu_dp->ethtool_ops.get_sset_count) {
		count = cpu_dp->ethtool_ops.get_sset_count(dev, ETH_SS_STATS);
		cpu_dp->ethtool_ops.get_ethtool_stats(dev, stats, data);
	}

	if (ds->ops->get_ethtool_stats)
		ds->ops->get_ethtool_stats(ds, cpu_port, data + count);
}

static int dsa_cpu_port_get_sset_count(struct net_device *dev, int sset)
{
	struct dsa_switch_tree *dst = dev->dsa_ptr;
	struct dsa_port *cpu_dp = dsa_get_cpu_port(dst);
	struct dsa_switch *ds = cpu_dp->ds;
	int count = 0;

	if (cpu_dp->ethtool_ops.get_sset_count)
		count += cpu_dp->ethtool_ops.get_sset_count(dev, sset);

	if (sset == ETH_SS_STATS && ds->ops->get_sset_count)
		count += ds->ops->get_sset_count(ds);

	return count;
}

static void dsa_cpu_port_get_strings(struct net_device *dev,
				     uint32_t stringset, uint8_t *data)
{
	struct dsa_switch_tree *dst = dev->dsa_ptr;
	struct dsa_port *cpu_dp = dsa_get_cpu_port(dst);
	struct dsa_switch *ds = cpu_dp->ds;
	s8 cpu_port = cpu_dp->index;
	int len = ETH_GSTRING_LEN;
	int mcount = 0, count;
	unsigned int i;
	uint8_t pfx[4];
	uint8_t *ndata;

	snprintf(pfx, sizeof(pfx), "p%.2d", cpu_port);
	/* We do not want to be NULL-terminated, since this is a prefix */
	pfx[sizeof(pfx) - 1] = '_';

	if (cpu_dp->ethtool_ops.get_sset_count) {
		mcount = cpu_dp->ethtool_ops.get_sset_count(dev, ETH_SS_STATS);
		cpu_dp->ethtool_ops.get_strings(dev, stringset, data);
	}

	if (stringset == ETH_SS_STATS && ds->ops->get_strings) {
		ndata = data + mcount * len;
		/* This function copies ETH_GSTRINGS_LEN bytes, we will mangle
		 * the output after to prepend our CPU port prefix we
		 * constructed earlier
		 */
		ds->ops->get_strings(ds, cpu_port, ndata);
		count = ds->ops->get_sset_count(ds);
		for (i = 0; i < count; i++) {
			memmove(ndata + (i * len + sizeof(pfx)),
				ndata + i * len, len - sizeof(pfx));
			memcpy(ndata + i * len, pfx, sizeof(pfx));
		}
	}
}

static void dsa_slave_get_ethtool_stats(struct net_device *dev,
					struct ethtool_stats *stats,
					uint64_t *data)
{
	struct dsa_slave_priv *p = netdev_priv(dev);
	struct dsa_switch *ds = p->dp->ds;
	struct pcpu_sw_netstats *s;
	unsigned int start;
	int i;

#ifdef CONFIG_WG_PLATFORM_DSA_MODE_T80	// WG:XD FBX-17773
	for_each_possible_cpu(i) {
		u64 tx_packets, tx_bytes, rx_packets, rx_bytes;

		s = per_cpu_ptr(p->stats64, i);
		do {
			start = u64_stats_fetch_begin_irq(&s->syncp);
			tx_packets = s->tx_packets;
			tx_bytes = s->tx_bytes;
			rx_packets = s->rx_packets;
			rx_bytes = s->rx_bytes;
		} while (u64_stats_fetch_retry_irq(&s->syncp, start));
		data[0] += tx_packets;
		data[1] += tx_bytes;
		data[2] += rx_packets;
		data[3] += rx_bytes;
	}
#else
	data[0] = dev->stats.tx_packets;
	data[1] = dev->stats.tx_bytes;
	data[2] = dev->stats.rx_packets;
	data[3] = dev->stats.rx_bytes;
#endif

	if (ds->ops->get_ethtool_stats)
		ds->ops->get_ethtool_stats(ds, p->dp->index, data + 4);
}

static int dsa_slave_get_sset_count(struct net_device *dev, int sset)
{
	struct dsa_slave_priv *p = netdev_priv(dev);
	struct dsa_switch *ds = p->dp->ds;

	if (sset == ETH_SS_STATS) {
		int count;

		count = 4;
		if (ds->ops->get_sset_count)
			count += ds->ops->get_sset_count(ds);

		return count;
	}

	return -EOPNOTSUPP;
}

static void dsa_slave_get_wol(struct net_device *dev, struct ethtool_wolinfo *w)
{
	struct dsa_slave_priv *p = netdev_priv(dev);
	struct dsa_switch *ds = p->dp->ds;

	if (ds->ops->get_wol)
		ds->ops->get_wol(ds, p->dp->index, w);
}

static int dsa_slave_set_wol(struct net_device *dev, struct ethtool_wolinfo *w)
{
	struct dsa_slave_priv *p = netdev_priv(dev);
	struct dsa_switch *ds = p->dp->ds;
	int ret = -EOPNOTSUPP;

	if (ds->ops->set_wol)
		ret = ds->ops->set_wol(ds, p->dp->index, w);

	return ret;
}

static int dsa_slave_set_eee(struct net_device *dev, struct ethtool_eee *e)
{
	struct dsa_slave_priv *p = netdev_priv(dev);
	struct dsa_switch *ds = p->dp->ds;
	int ret;

	/* Port's PHY and MAC both need to be EEE capable */
	if (!p->phy)
		return -ENODEV;

	if (!ds->ops->set_mac_eee)
		return -EOPNOTSUPP;

	ret = ds->ops->set_mac_eee(ds, p->dp->index, e);
	if (ret)
		return ret;

	if (e->eee_enabled) {
		ret = phy_init_eee(p->phy, 0);
		if (ret)
			return ret;
	}

	return phy_ethtool_set_eee(p->phy, e);
}

static int dsa_slave_get_eee(struct net_device *dev, struct ethtool_eee *e)
{
	struct dsa_slave_priv *p = netdev_priv(dev);
	struct dsa_switch *ds = p->dp->ds;
	int ret;

	/* Port's PHY and MAC both need to be EEE capable */
	if (!p->phy)
		return -ENODEV;

	if (!ds->ops->get_mac_eee)
		return -EOPNOTSUPP;

	ret = ds->ops->get_mac_eee(ds, p->dp->index, e);
	if (ret)
		return ret;

	return phy_ethtool_get_eee(p->phy, e);
}

#ifdef CONFIG_NET_POLL_CONTROLLER
static int dsa_slave_netpoll_setup(struct net_device *dev,
				   struct netpoll_info *ni)
{
	struct dsa_slave_priv *p = netdev_priv(dev);
	struct net_device *master = dsa_master_netdev(p);
	struct netpoll *netpoll;
	int err = 0;

	netpoll = kzalloc(sizeof(*netpoll), GFP_KERNEL);
	if (!netpoll)
		return -ENOMEM;

	err = __netpoll_setup(netpoll, master);
	if (err) {
		kfree(netpoll);
		goto out;
	}

	p->netpoll = netpoll;
out:
	return err;
}

static void dsa_slave_netpoll_cleanup(struct net_device *dev)
{
	struct dsa_slave_priv *p = netdev_priv(dev);
	struct netpoll *netpoll = p->netpoll;

	if (!netpoll)
		return;

	p->netpoll = NULL;

	__netpoll_free_async(netpoll);
}

static void dsa_slave_poll_controller(struct net_device *dev)
{
}
#endif

static int dsa_slave_get_phys_port_name(struct net_device *dev,
					char *name, size_t len)
{
	struct dsa_slave_priv *p = netdev_priv(dev);

	if (snprintf(name, len, "p%d", p->dp->index) >= len)
		return -EINVAL;

	return 0;
}

static struct dsa_mall_tc_entry *
dsa_slave_mall_tc_entry_find(struct dsa_slave_priv *p,
			     unsigned long cookie)
{
	struct dsa_mall_tc_entry *mall_tc_entry;

	list_for_each_entry(mall_tc_entry, &p->mall_tc_list, list)
		if (mall_tc_entry->cookie == cookie)
			return mall_tc_entry;

	return NULL;
}

static int dsa_slave_add_cls_matchall(struct net_device *dev,
				      struct tc_cls_matchall_offload *cls,
				      bool ingress)
{
	struct dsa_slave_priv *p = netdev_priv(dev);
	struct dsa_mall_tc_entry *mall_tc_entry;
	__be16 protocol = cls->common.protocol;
	struct dsa_switch *ds = p->dp->ds;
	struct net *net = dev_net(dev);
	struct dsa_slave_priv *to_p;
	struct net_device *to_dev;
	const struct tc_action *a;
	int err = -EOPNOTSUPP;
	LIST_HEAD(actions);
	int ifindex;

	if (!ds->ops->port_mirror_add)
		return err;

	if (!tcf_exts_has_one_action(cls->exts))
		return err;

	tcf_exts_to_list(cls->exts, &actions);
	a = list_first_entry(&actions, struct tc_action, list);

	if (is_tcf_mirred_egress_mirror(a) && protocol == htons(ETH_P_ALL)) {
		struct dsa_mall_mirror_tc_entry *mirror;

		ifindex = tcf_mirred_ifindex(a);
		to_dev = __dev_get_by_index(net, ifindex);
		if (!to_dev)
			return -EINVAL;

		if (!dsa_slave_dev_check(to_dev))
			return -EOPNOTSUPP;

		mall_tc_entry = kzalloc(sizeof(*mall_tc_entry), GFP_KERNEL);
		if (!mall_tc_entry)
			return -ENOMEM;

		mall_tc_entry->cookie = cls->cookie;
		mall_tc_entry->type = DSA_PORT_MALL_MIRROR;
		mirror = &mall_tc_entry->mirror;

		to_p = netdev_priv(to_dev);

		mirror->to_local_port = to_p->dp->index;
		mirror->ingress = ingress;

		err = ds->ops->port_mirror_add(ds, p->dp->index, mirror,
					       ingress);
		if (err) {
			kfree(mall_tc_entry);
			return err;
		}

		list_add_tail(&mall_tc_entry->list, &p->mall_tc_list);
	}

	return 0;
}

static void dsa_slave_del_cls_matchall(struct net_device *dev,
				       struct tc_cls_matchall_offload *cls)
{
	struct dsa_slave_priv *p = netdev_priv(dev);
	struct dsa_mall_tc_entry *mall_tc_entry;
	struct dsa_switch *ds = p->dp->ds;

	if (!ds->ops->port_mirror_del)
		return;

	mall_tc_entry = dsa_slave_mall_tc_entry_find(p, cls->cookie);
	if (!mall_tc_entry)
		return;

	list_del(&mall_tc_entry->list);

	switch (mall_tc_entry->type) {
	case DSA_PORT_MALL_MIRROR:
		ds->ops->port_mirror_del(ds, p->dp->index,
					 &mall_tc_entry->mirror);
		break;
	default:
		WARN_ON(1);
	}

	kfree(mall_tc_entry);
}

static int dsa_slave_setup_tc_cls_matchall(struct net_device *dev,
					   struct tc_cls_matchall_offload *cls)
{
	bool ingress;

	if (is_classid_clsact_ingress(cls->common.classid))
		ingress = true;
	else if (is_classid_clsact_egress(cls->common.classid))
		ingress = false;
	else
		return -EOPNOTSUPP;

	if (cls->common.chain_index)
		return -EOPNOTSUPP;

	switch (cls->command) {
	case TC_CLSMATCHALL_REPLACE:
		return dsa_slave_add_cls_matchall(dev, cls, ingress);
	case TC_CLSMATCHALL_DESTROY:
		dsa_slave_del_cls_matchall(dev, cls);
		return 0;
	default:
		return -EOPNOTSUPP;
	}
}

static int dsa_slave_setup_tc(struct net_device *dev, enum tc_setup_type type,
			      void *type_data)
{
	switch (type) {
	case TC_SETUP_CLSMATCHALL:
		return dsa_slave_setup_tc_cls_matchall(dev, type_data);
	default:
		return -EOPNOTSUPP;
	}
}

static void dsa_slave_get_stats64(struct net_device *dev,
				  struct rtnl_link_stats64 *stats)
{
	struct dsa_slave_priv *p = netdev_priv(dev);
	struct pcpu_sw_netstats *s;
	unsigned int start;
	int i;

	netdev_stats_to_stats64(stats, &dev->stats);
	for_each_possible_cpu(i) {
		u64 tx_packets, tx_bytes, rx_packets, rx_bytes;

		s = per_cpu_ptr(p->stats64, i);
		do {
			start = u64_stats_fetch_begin_irq(&s->syncp);
			tx_packets = s->tx_packets;
			tx_bytes = s->tx_bytes;
			rx_packets = s->rx_packets;
			rx_bytes = s->rx_bytes;
		} while (u64_stats_fetch_retry_irq(&s->syncp, start));

		stats->tx_packets += tx_packets;
		stats->tx_bytes += tx_bytes;
		stats->rx_packets += rx_packets;
		stats->rx_bytes += rx_bytes;
	}
}

void dsa_cpu_port_ethtool_init(struct ethtool_ops *ops)
{
	ops->get_sset_count = dsa_cpu_port_get_sset_count;
	ops->get_ethtool_stats = dsa_cpu_port_get_ethtool_stats;
	ops->get_strings = dsa_cpu_port_get_strings;
}

static int dsa_slave_get_rxnfc(struct net_device *dev,
			       struct ethtool_rxnfc *nfc, u32 *rule_locs)
{
	struct dsa_slave_priv *p = netdev_priv(dev);
	struct dsa_switch *ds = p->dp->ds;

	if (!ds->ops->get_rxnfc)
		return -EOPNOTSUPP;

	return ds->ops->get_rxnfc(ds, p->dp->index, nfc, rule_locs);
}

static int dsa_slave_set_rxnfc(struct net_device *dev,
			       struct ethtool_rxnfc *nfc)
{
	struct dsa_slave_priv *p = netdev_priv(dev);
	struct dsa_switch *ds = p->dp->ds;

	if (!ds->ops->set_rxnfc)
		return -EOPNOTSUPP;

	return ds->ops->set_rxnfc(ds, p->dp->index, nfc);
}

static const struct ethtool_ops dsa_slave_ethtool_ops = {
	.get_drvinfo		= dsa_slave_get_drvinfo,
	.get_regs_len		= dsa_slave_get_regs_len,
	.get_regs		= dsa_slave_get_regs,
	.nway_reset		= dsa_slave_nway_reset,
	.get_link		= dsa_slave_get_link,
	.get_eeprom_len		= dsa_slave_get_eeprom_len,
	.get_eeprom		= dsa_slave_get_eeprom,
	.set_eeprom		= dsa_slave_set_eeprom,
	.get_strings		= dsa_slave_get_strings,
	.get_ethtool_stats	= dsa_slave_get_ethtool_stats,
	.get_sset_count		= dsa_slave_get_sset_count,
	.set_wol		= dsa_slave_set_wol,
	.get_wol		= dsa_slave_get_wol,
	.set_eee		= dsa_slave_set_eee,
	.get_eee		= dsa_slave_get_eee,
	.get_link_ksettings	= dsa_slave_get_link_ksettings,
	.set_link_ksettings	= dsa_slave_set_link_ksettings,
	.get_rxnfc		= dsa_slave_get_rxnfc,
	.set_rxnfc		= dsa_slave_set_rxnfc,
};
#ifndef CONFIG_WG_PLATFORM_DSA_MODE_T80
#if defined(CONFIG_WG_PLATFORM_DSA_MODULE) || defined (CONFIG_WG_PLATFORM_DSA)

#include <linux/wg_kernel.h>

#define	NETH	WG_DSA_NETH
#define	NIF	WG_DSA_NIF

int  dsa_change_master_mtu(struct net_device *master)
{
	int j;
	int mtu;

	// Check for master
	if (!master) return -ENODEV;

	// Find largest MTU on this master
	for (mtu = j = 0; j < MAXETH; j++) if (wg_slave_dev[j])
 	if (wg_master_dev[wg_slave_dev[j]->ifindex] == master)
	 if (mtu < wg_slave_dev[j]->mtu)
		mtu = wg_slave_dev[j]->mtu;

	// Assure we can always handle standard frames
	if (mtu <= ETH_DATA_LEN) mtu = ETH_DATA_LEN;

	// Add in overhead
	mtu += VLAN_HLEN;
	if (strcmp(master->name, SW0_NAME) == 0) // Set SW10 MTU
	 	mtu += wg_dsa_L2_offset[0];
	else
	if (strcmp(master->name, SW1_NAME) == 0) // Set SW11 MTU
		mtu += wg_dsa_L2_offset[1];

	// Check if master MTU changed
	if (master->mtu != mtu) {
		dev_set_mtu(master, mtu);
		printk(KERN_INFO "%s: %s MTU %d\n",
			__FUNCTION__, master->name, master->mtu);
	}

	return 0;
}
EXPORT_SYMBOL(dsa_change_master_mtu);

int  dsa_change_mtu(struct net_device *dev, int mtu)
{
	// Range check MTU
	 if ((mtu < 68) || (mtu > 9000))
		  return -EINVAL;

	 // Check for changes
	 if (dev->mtu == mtu)
		  return -EEXIST;

	 // Set MTU
	 dev->mtu = mtu;

	 // Set MTU of master if needed
	 return dsa_change_master_mtu(wg_master_dev[dev->ifindex]);
}
EXPORT_SYMBOL(dsa_change_mtu);

// wg_dsa.ko calls these to set slave properties
void marvell_reset_slave(void)
{
	memset(wg_slave_tag,  0, sizeof(wg_slave_tag));
	memset(wg_slave_dev,  0, sizeof(wg_slave_dev));
	memset(wg_master_dev, 0, sizeof(wg_master_dev));
}
EXPORT_SYMBOL(marvell_reset_slave);

int marvell_set_slave(struct net_device*  sw_dev,
		      struct net_device* eth_dev,
		      int n, int tag)
{
	// Range check eth device
	if (((unsigned)n) >= NETH) {
		printk(KERN_EMERG "%s: %s is not [eth0-eth%d]\n",
		       __FUNCTION__, eth_dev ? eth_dev->name : "none", NETH-1);
		return -ENOMEM;
	}

	// Range check eth device's ifindex
	if (((unsigned)(eth_dev->ifindex)) >= NIF) {
		printk(KERN_EMERG "%s: %s ifindex %d is not [0-%d]\n",
		       __FUNCTION__, eth_dev->name, eth_dev->ifindex, NIF-1);
		return -ENOMEM;
	}

	// Set the tag
	wg_slave_tag[eth_dev->ifindex] = tag;

	// Save the eth device's master switch device
	wg_master_dev[eth_dev->ifindex] = sw_dev;

	// Save the actual device for /proc printout clarity
	wg_named_dev[n] = eth_dev;

	// If the tag is turned off EDSA is on and packets go to switch
	// device first and then to the slave via the DADA protocol hook
	if (tag < 0) eth_dev = sw_dev;

	// Set the eth device once for non bridged mode and at +NETH for bridged
	wg_slave_dev[n+0000] = eth_dev;
	wg_slave_dev[n+NETH] = eth_dev;

	return n;
}
EXPORT_SYMBOL(marvell_set_slave);

// Quick transmit routine to add Marvell headers
netdev_tx_t wg_dsa_slave_xmit(struct sk_buff* skb, struct net_device* dev)
{
	int ifx = dev->ifindex;
	int tag = wg_slave_tag[ifx];

	// Set to DSA master if none specified
	if (unlikely(!(skb->dev = wg_master_dev[ifx]))) {
		struct dsa_slave_priv *p = netdev_priv(dev);
		skb->dev = wg_master_dev[ifx] = p->dp->ds->dst->cpu_dp->netdev;
		printk(KERN_DEBUG "%s: %s [%2d] no master\n",
		       __FUNCTION__, dev->name, ifx);
	}

	// If we have a valid tag
	if (likely(tag > 0)) {
		int pad = 60 - skb->len;

	  	// Pad packets since removing the Marvell tag will create runts otherwise
		if (unlikely(pad > 0)) {
			if (unlikely(skb_pad(skb, pad)))
				goto out_free;
			skb_put(skb, pad);
		}

		// Check headroom
		if (unlikely(skb_cow_head(skb, 2) < 0))
			goto out_free;

		// Put in the Marvell tag
		skb_push(skb, 2);
		*((u16*)skb->data) = htons(tag);

		if (unlikely(wg_dsa_debug & 8))
		printk("%s: Head %p Data %p Len %d Tag %04x Sum %d %s\n",
		       dev->name, skb->head, skb->data, skb->len,
		       tag, skb->ip_summed, skb->dev->name);

		// Send the packet with Marvell header
		dev_queue_xmit(skb);
		return NETDEV_TX_OK;
	}

	// If we have a tag
	if (likely(tag)) {
		// Send the packet like before
		dev_queue_xmit(skb);
		return NETDEV_TX_OK;
	}

	// No tag, toss packets sent to unused bridge ports
out_free:
	kfree_skb(skb);
	return NETDEV_TX_OK;
}

// Jacket routine to also do packet accounting if not using EDSA
netdev_tx_t marvell_xmit(struct sk_buff* skb, struct net_device* dev)
{
	dev->stats.tx_packets++;
	dev->stats.tx_bytes += skb->len;

	return wg_dsa_slave_xmit(skb, dev);
}

// Marvell dispatch table
static const struct net_device_ops marvell_netdev_ops = {
	.ndo_open	 	= dsa_slave_open,
	.ndo_stop		= dsa_slave_close,
	.ndo_start_xmit		= marvell_xmit,
	.ndo_change_mtu		= dsa_change_mtu,
	.ndo_change_rx_flags	= dsa_slave_change_rx_flags,
	.ndo_set_rx_mode	= dsa_slave_set_rx_mode,
	.ndo_set_mac_address	= dsa_slave_set_mac_address,
	.ndo_do_ioctl		= dsa_slave_ioctl,
};

#endif	// WG_PLATFORM_DSA_MODULE || CONFIG_WG_PLATFORM_DSA
#endif // CONFIG_WG_PLATFORM_DSA_MODE_T80

static const struct net_device_ops dsa_slave_netdev_ops = {
	.ndo_open	 	= dsa_slave_open,
	.ndo_stop		= dsa_slave_close,
	.ndo_start_xmit		= dsa_slave_xmit,
	.ndo_change_rx_flags	= dsa_slave_change_rx_flags,
	.ndo_set_rx_mode	= dsa_slave_set_rx_mode,
	.ndo_set_mac_address	= dsa_slave_set_mac_address,
	.ndo_fdb_add		= dsa_legacy_fdb_add,
	.ndo_fdb_del		= dsa_legacy_fdb_del,
	.ndo_fdb_dump		= dsa_slave_fdb_dump,
	.ndo_do_ioctl		= dsa_slave_ioctl,
	.ndo_get_iflink		= dsa_slave_get_iflink,
#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_netpoll_setup	= dsa_slave_netpoll_setup,
	.ndo_netpoll_cleanup	= dsa_slave_netpoll_cleanup,
	.ndo_poll_controller	= dsa_slave_poll_controller,
#endif
	.ndo_get_phys_port_name	= dsa_slave_get_phys_port_name,
	.ndo_setup_tc		= dsa_slave_setup_tc,
	.ndo_get_stats64	= dsa_slave_get_stats64,
};

static const struct switchdev_ops dsa_slave_switchdev_ops = {
	.switchdev_port_attr_get	= dsa_slave_port_attr_get,
	.switchdev_port_attr_set	= dsa_slave_port_attr_set,
	.switchdev_port_obj_add		= dsa_slave_port_obj_add,
	.switchdev_port_obj_del		= dsa_slave_port_obj_del,
};

static struct device_type dsa_type = {
	.name	= "dsa",
};

static void dsa_slave_adjust_link(struct net_device *dev)
{
	struct dsa_slave_priv *p = netdev_priv(dev);
	struct dsa_switch *ds = p->dp->ds;
	unsigned int status_changed = 0;

	if (p->old_link != p->phy->link) {
		status_changed = 1;
		p->old_link = p->phy->link;
	}

	if (p->old_duplex != p->phy->duplex) {
		status_changed = 1;
		p->old_duplex = p->phy->duplex;
	}

	if (p->old_pause != p->phy->pause) {
		status_changed = 1;
		p->old_pause = p->phy->pause;
	}

	if (ds->ops->adjust_link && status_changed)
		ds->ops->adjust_link(ds, p->dp->index, p->phy);

	if (status_changed)
		phy_print_status(p->phy);
}

static int dsa_slave_fixed_link_update(struct net_device *dev,
				       struct fixed_phy_status *status)
{
	struct dsa_slave_priv *p;
	struct dsa_switch *ds;

	if (dev) {
		p = netdev_priv(dev);
		ds = p->dp->ds;
		if (ds->ops->fixed_link_update)
			ds->ops->fixed_link_update(ds, p->dp->index, status);
	}

	return 0;
}

/* slave device setup *******************************************************/
static int dsa_slave_phy_connect(struct dsa_slave_priv *p,
				 struct net_device *slave_dev,
				 int addr)
{
#ifndef CONFIG_WG_PLATFORM_DSA_MODE_T80
#ifdef	CONFIG_WG_PLATFORM // WG:XD FBX-16767
	int ret;
#endif
#endif // CONFIG_WG_PLATFORM_DSA_MODE_T80
	struct dsa_switch *ds = p->dp->ds;

	p->phy = mdiobus_get_phy(ds->slave_mii_bus, addr);
	if (!p->phy) {
		netdev_err(slave_dev, "no phy at %d\n", addr);
		return -ENODEV;
	}

	/* Use already configured phy mode */
	if (p->phy_interface == PHY_INTERFACE_MODE_NA)
		p->phy_interface = p->phy->interface;
#ifndef CONFIG_WG_PLATFORM_DSA_MODE_T80

#ifdef	CONFIG_WG_PLATFORM	// WG:XD FBX-16767, FBX-17816, from WG 3.12.19 kernel slave.c
	ret = phy_attach_direct(slave_dev, p->phy, p->phy->dev_flags, p->phy_interface);
	if (!ret) {
		p->phy->autoneg = AUTONEG_ENABLE;
		p->phy->speed = 0;
		p->phy->duplex = 0;
		p->phy->advertising = p->phy->supported | ADVERTISED_Autoneg;
		phy_start_aneg(p->phy);
	}
	return ret;
#else
	return phy_connect_direct(slave_dev, p->phy, dsa_slave_adjust_link,
				  p->phy_interface);
#endif
#else
	return phy_connect_direct(slave_dev, p->phy, dsa_slave_adjust_link,
				  p->phy_interface);
#endif // CONFIG_WG_PLATFORM_DSA_MODE_T80
}

static int dsa_slave_phy_setup(struct dsa_slave_priv *p,
				struct net_device *slave_dev)
{
	struct dsa_switch *ds = p->dp->ds;
	struct device_node *phy_dn, *port_dn;
	bool phy_is_fixed = false;
	u32 phy_flags = 0;
	int mode, ret;

	port_dn = p->dp->dn;
	mode = of_get_phy_mode(port_dn);
	if (mode < 0)
		mode = PHY_INTERFACE_MODE_NA;
	p->phy_interface = mode;

	phy_dn = of_parse_phandle(port_dn, "phy-handle", 0);
	if (!phy_dn && of_phy_is_fixed_link(port_dn)) {
		/* In the case of a fixed PHY, the DT node associated
		 * to the fixed PHY is the Port DT node
		 */
		ret = of_phy_register_fixed_link(port_dn);
		if (ret) {
			netdev_err(slave_dev, "failed to register fixed PHY: %d\n", ret);
			return ret;
		}
		phy_is_fixed = true;
		phy_dn = of_node_get(port_dn);
	}

	if (ds->ops->get_phy_flags)
		phy_flags = ds->ops->get_phy_flags(ds, p->dp->index);

	if (phy_dn) {
		int phy_id = of_mdio_parse_addr(&slave_dev->dev, phy_dn);

		/* If this PHY address is part of phys_mii_mask, which means
		 * that we need to divert reads and writes to/from it, then we
		 * want to bind this device using the slave MII bus created by
		 * DSA to make that happen.
		 */
		if (!phy_is_fixed && phy_id >= 0 &&
		    (ds->phys_mii_mask & (1 << phy_id))) {
			ret = dsa_slave_phy_connect(p, slave_dev, phy_id);
			if (ret) {
				netdev_err(slave_dev, "failed to connect to phy%d: %d\n", phy_id, ret);
				of_node_put(phy_dn);
				return ret;
			}
		} else {
			p->phy = of_phy_connect(slave_dev, phy_dn,
						dsa_slave_adjust_link,
						phy_flags,
						p->phy_interface);
		}

		of_node_put(phy_dn);
	}

	if (p->phy && phy_is_fixed)
		fixed_phy_set_link_update(p->phy, dsa_slave_fixed_link_update);

	/* We could not connect to a designated PHY, so use the switch internal
	 * MDIO bus instead
	 */
	if (!p->phy) {
		ret = dsa_slave_phy_connect(p, slave_dev, p->dp->index);
		if (ret) {
			netdev_err(slave_dev, "failed to connect to port %d: %d\n",
				   p->dp->index, ret);
			if (phy_is_fixed)
				of_phy_deregister_fixed_link(port_dn);
			return ret;
		}
	}

	phy_attached_info(p->phy);

	return 0;
}

static struct lock_class_key dsa_slave_netdev_xmit_lock_key;
static void dsa_slave_set_lockdep_class_one(struct net_device *dev,
					    struct netdev_queue *txq,
					    void *_unused)
{
	lockdep_set_class(&txq->_xmit_lock,
			  &dsa_slave_netdev_xmit_lock_key);
}

int dsa_slave_suspend(struct net_device *slave_dev)
{
	struct dsa_slave_priv *p = netdev_priv(slave_dev);

	if (!netif_running(slave_dev))
		return 0;

	netif_device_detach(slave_dev);

	if (p->phy) {
		phy_stop(p->phy);
		p->old_pause = -1;
		p->old_link = -1;
		p->old_duplex = -1;
		phy_suspend(p->phy);
	}

	return 0;
}

int dsa_slave_resume(struct net_device *slave_dev)
{
	struct dsa_slave_priv *p = netdev_priv(slave_dev);

	if (!netif_running(slave_dev))
		return 0;

	netif_device_attach(slave_dev);

	if (p->phy) {
		phy_resume(p->phy);
		phy_start(p->phy);
	}

	return 0;
}

int dsa_slave_create(struct dsa_port *port, const char *name)
{
	struct dsa_switch *ds = port->ds;
	struct dsa_switch_tree *dst = ds->dst;
	struct net_device *master;
	struct net_device *slave_dev;
	struct dsa_slave_priv *p;
	struct dsa_port *cpu_dp;
	int ret;

	cpu_dp = ds->dst->cpu_dp;
#ifdef	CONFIG_WG_PLATFORM_DSA_MODE_T80
	int port_cpu = ds->port_cpu[port->index];

	if (port_cpu && ds->port_ethernet[port_cpu])
		master = ds->port_ethernet[port_cpu];
	else
#endif
		master = cpu_dp->netdev;
#ifdef	CONFIG_WG_PLATFORM_DSA_MODE_T80
    	master->dsa_ptr = (void *)ds->dst;
#endif

	if (!ds->num_tx_queues)
		ds->num_tx_queues = 1;

	slave_dev = alloc_netdev_mqs(sizeof(struct dsa_slave_priv), name,
				     NET_NAME_UNKNOWN, ether_setup,
				     ds->num_tx_queues, 1);
	if (slave_dev == NULL)
		return -ENOMEM;

	slave_dev->features = master->vlan_features | NETIF_F_HW_TC;
	slave_dev->hw_features |= NETIF_F_HW_TC;
	slave_dev->ethtool_ops = &dsa_slave_ethtool_ops;
	eth_hw_addr_inherit(slave_dev, master);
	slave_dev->priv_flags |= IFF_NO_QUEUE;
	slave_dev->netdev_ops = &dsa_slave_netdev_ops;
	slave_dev->switchdev_ops = &dsa_slave_switchdev_ops;
	slave_dev->min_mtu = 0;
	slave_dev->max_mtu = ETH_MAX_MTU;
	SET_NETDEV_DEVTYPE(slave_dev, &dsa_type);

	netdev_for_each_tx_queue(slave_dev, dsa_slave_set_lockdep_class_one,
				 NULL);

	SET_NETDEV_DEV(slave_dev, port->ds->dev);
	slave_dev->dev.of_node = port->dn;
	slave_dev->vlan_features = master->vlan_features;

	p = netdev_priv(slave_dev);
	p->stats64 = netdev_alloc_pcpu_stats(struct pcpu_sw_netstats);
	if (!p->stats64) {
		free_netdev(slave_dev);
		return -ENOMEM;
	}
	p->dp = port;
	INIT_LIST_HEAD(&p->mall_tc_list);
	p->xmit = dst->tag_ops->xmit;
#ifdef	CONFIG_WG_PLATFORM_DSA_MODE_T80
	p->master = master;
	struct dsa_port *cpu_dp2 = ds->dst->cpu_dp2;
#endif

	p->old_pause = -1;
	p->old_link = -1;
	p->old_duplex = -1;

	port->netdev = slave_dev;

	netif_carrier_off(slave_dev);

	ret = dsa_slave_phy_setup(p, slave_dev);
	if (ret) {
		netdev_err(master, "error %d setting up slave phy\n", ret);
		goto out_free;
	}

	ret = register_netdev(slave_dev);
	if (ret) {
		netdev_err(master, "error %d registering interface %s\n",
			   ret, slave_dev->name);
		goto out_phy;
	}

	return 0;

out_phy:
	phy_disconnect(p->phy);
	if (of_phy_is_fixed_link(p->dp->dn))
		of_phy_deregister_fixed_link(p->dp->dn);
out_free:
	free_percpu(p->stats64);
	free_netdev(slave_dev);
	port->netdev = NULL;
	return ret;
}

void dsa_slave_destroy(struct net_device *slave_dev)
{
	struct dsa_slave_priv *p = netdev_priv(slave_dev);
	struct device_node *port_dn;

	port_dn = p->dp->dn;

	netif_carrier_off(slave_dev);
	if (p->phy) {
		phy_disconnect(p->phy);

		if (of_phy_is_fixed_link(port_dn))
			of_phy_deregister_fixed_link(port_dn);
	}
	unregister_netdev(slave_dev);
	free_percpu(p->stats64);
	free_netdev(slave_dev);
}

static bool dsa_slave_dev_check(struct net_device *dev)
{
	return dev->netdev_ops == &dsa_slave_netdev_ops;
}

static int dsa_slave_changeupper(struct net_device *dev,
				 struct netdev_notifier_changeupper_info *info)
{
	struct dsa_slave_priv *p = netdev_priv(dev);
	struct dsa_port *dp = p->dp;
	int err = NOTIFY_DONE;

	if (netif_is_bridge_master(info->upper_dev)) {
		if (info->linking) {
			err = dsa_port_bridge_join(dp, info->upper_dev);
			err = notifier_from_errno(err);
		} else {
			dsa_port_bridge_leave(dp, info->upper_dev);
			err = NOTIFY_OK;
		}
	}

	return err;
}

static int dsa_slave_netdevice_event(struct notifier_block *nb,
				     unsigned long event, void *ptr)
{
	struct net_device *dev = netdev_notifier_info_to_dev(ptr);

	if (dev->netdev_ops != &dsa_slave_netdev_ops)
		return NOTIFY_DONE;

	if (event == NETDEV_CHANGEUPPER)
		return dsa_slave_changeupper(dev, ptr);

	return NOTIFY_DONE;
}

struct dsa_switchdev_event_work {
	struct work_struct work;
	struct switchdev_notifier_fdb_info fdb_info;
	struct net_device *dev;
	unsigned long event;
};

static void dsa_slave_switchdev_event_work(struct work_struct *work)
{
	struct dsa_switchdev_event_work *switchdev_work =
		container_of(work, struct dsa_switchdev_event_work, work);
	struct net_device *dev = switchdev_work->dev;
	struct switchdev_notifier_fdb_info *fdb_info;
	struct dsa_slave_priv *p = netdev_priv(dev);
	int err;

	rtnl_lock();
	switch (switchdev_work->event) {
	case SWITCHDEV_FDB_ADD_TO_DEVICE:
		fdb_info = &switchdev_work->fdb_info;
		err = dsa_port_fdb_add(p->dp, fdb_info->addr, fdb_info->vid);
		if (err) {
			netdev_dbg(dev, "fdb add failed err=%d\n", err);
			break;
		}
		call_switchdev_notifiers(SWITCHDEV_FDB_OFFLOADED, dev,
					 &fdb_info->info);
		break;

	case SWITCHDEV_FDB_DEL_TO_DEVICE:
		fdb_info = &switchdev_work->fdb_info;
		err = dsa_port_fdb_del(p->dp, fdb_info->addr, fdb_info->vid);
		if (err) {
			netdev_dbg(dev, "fdb del failed err=%d\n", err);
			dev_close(dev);
		}
		break;
	}
	rtnl_unlock();

	kfree(switchdev_work->fdb_info.addr);
	kfree(switchdev_work);
	dev_put(dev);
}

static int
dsa_slave_switchdev_fdb_work_init(struct dsa_switchdev_event_work *
				  switchdev_work,
				  const struct switchdev_notifier_fdb_info *
				  fdb_info)
{
	memcpy(&switchdev_work->fdb_info, fdb_info,
	       sizeof(switchdev_work->fdb_info));
	switchdev_work->fdb_info.addr = kzalloc(ETH_ALEN, GFP_ATOMIC);
	if (!switchdev_work->fdb_info.addr)
		return -ENOMEM;
	ether_addr_copy((u8 *)switchdev_work->fdb_info.addr,
			fdb_info->addr);
	return 0;
}

/* Called under rcu_read_lock() */
static int dsa_slave_switchdev_event(struct notifier_block *unused,
				     unsigned long event, void *ptr)
{
	struct net_device *dev = switchdev_notifier_info_to_dev(ptr);
	struct dsa_switchdev_event_work *switchdev_work;

	if (!dsa_slave_dev_check(dev))
		return NOTIFY_DONE;

	switchdev_work = kzalloc(sizeof(*switchdev_work), GFP_ATOMIC);
	if (!switchdev_work)
		return NOTIFY_BAD;

	INIT_WORK(&switchdev_work->work,
		  dsa_slave_switchdev_event_work);
	switchdev_work->dev = dev;
	switchdev_work->event = event;

	switch (event) {
	case SWITCHDEV_FDB_ADD_TO_DEVICE: /* fall through */
	case SWITCHDEV_FDB_DEL_TO_DEVICE:
		if (dsa_slave_switchdev_fdb_work_init(switchdev_work,
						      ptr))
			goto err_fdb_work_init;
		dev_hold(dev);
		break;
	default:
		kfree(switchdev_work);
		return NOTIFY_DONE;
	}

	dsa_schedule_work(&switchdev_work->work);
	return NOTIFY_OK;

err_fdb_work_init:
	kfree(switchdev_work);
	return NOTIFY_BAD;
}

static struct notifier_block dsa_slave_nb __read_mostly = {
	.notifier_call  = dsa_slave_netdevice_event,
};

static struct notifier_block dsa_slave_switchdev_notifier = {
	.notifier_call = dsa_slave_switchdev_event,
};

int dsa_slave_register_notifier(void)
{
	int err;

	err = register_netdevice_notifier(&dsa_slave_nb);
	if (err)
		return err;

	err = register_switchdev_notifier(&dsa_slave_switchdev_notifier);
	if (err)
		goto err_switchdev_nb;

	return 0;

err_switchdev_nb:
	unregister_netdevice_notifier(&dsa_slave_nb);
	return err;
}

void dsa_slave_unregister_notifier(void)
{
	int err;

	err = unregister_switchdev_notifier(&dsa_slave_switchdev_notifier);
	if (err)
		pr_err("DSA: failed to unregister switchdev notifier (%d)\n", err);

	err = unregister_netdevice_notifier(&dsa_slave_nb);
	if (err)
		pr_err("DSA: failed to unregister slave notifier (%d)\n", err);
}
