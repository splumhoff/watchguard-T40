/*
 * net/dsa/tag_dsa.c - (Non-ethertype) DSA tagging
 * Copyright (c) 2008-2009 Marvell Semiconductor
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/etherdevice.h>
#include <linux/list.h>
#include <linux/slab.h>

#include "dsa_priv.h"

#define DSA_HLEN	4

static struct sk_buff *dsa_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct dsa_slave_priv *p = netdev_priv(dev);
	u8 *dsa_header;

	/*
	 * Convert the outermost 802.1q tag to a DSA tag for tagged
	 * packets, or insert a DSA tag between the addresses and
	 * the ethertype field for untagged packets.
	 */
	if (skb->protocol == htons(ETH_P_8021Q)) {
		if (skb_cow_head(skb, 0) < 0)
			return NULL;

		/*
		 * Construct tagged FROM_CPU DSA tag from 802.1q tag.
		 */
		dsa_header = skb->data + 2 * ETH_ALEN;
		dsa_header[0] = 0x60 | p->dp->ds->index;
		dsa_header[1] = p->dp->index << 3;

		/*
		 * Move CFI field from byte 2 to byte 1.
		 */
		if (dsa_header[2] & 0x10) {
			dsa_header[1] |= 0x01;
			dsa_header[2] &= ~0x10;
		}
	} else {
		if (skb_cow_head(skb, DSA_HLEN) < 0)
			return NULL;
		skb_push(skb, DSA_HLEN);

		memmove(skb->data, skb->data + DSA_HLEN, 2 * ETH_ALEN);

		/*
		 * Construct untagged FROM_CPU DSA tag.
		 */
		dsa_header = skb->data + 2 * ETH_ALEN;
		dsa_header[0] = 0x40 | p->dp->ds->index;
		
#if defined(CONFIG_WG_PLATFORM_DSA_MODULE) || defined (CONFIG_WG_PLATFORM_DSA)
		if (wg_cpu_model == WG_CPU_686(3558) || wg_cpu_model == WG_CPU_686(3758))
			dsa_header[1] = ((p->dp->index + 1) % p->dp->ds->num_ports) << 3;
		else
#endif
		dsa_header[1] = p->dp->index << 3;
		dsa_header[2] = 0x00;
		dsa_header[3] = 0x00;
	}

	return skb;
}

static struct sk_buff *dsa_rcv(struct sk_buff *skb, struct net_device *dev,
			       struct packet_type *pt)
{
	struct dsa_switch_tree *dst = dev->dsa_ptr;
	struct dsa_switch *ds;
	u8 *dsa_header;
	int source_device;
	int source_port;

	if (unlikely(!pskb_may_pull(skb, DSA_HLEN)))
		return NULL;

	/*
	 * The ethertype field is part of the DSA header.
	 */
	dsa_header = skb->data - 2;

	/*
	 * Check that frame type is either TO_CPU or FORWARD.
	 */
#if defined (CONFIG_WG_PLATFORM_M390) || defined(CONFIG_WG_PLATFORM_M590_M690)
	// WG:XD FBX-21685
	if ((dsa_header[0] & 0xc0) != 0x00 && (dsa_header[0] & 0xc0) != 0xc0
		&& (dsa_header[0] & 0xc0) != 0x80)
#else
	if ((dsa_header[0] & 0xc0) != 0x00 && (dsa_header[0] & 0xc0) != 0xc0)
#endif
		return NULL;

	/*
	 * Determine source device and port.
	 */
	source_device = dsa_header[0] & 0x1f;
	source_port = (dsa_header[1] >> 3) & 0x1f;

	/*
	 * Check that the source device exists and that the source
	 * port is a registered DSA port.
	 */
	if (source_device >= DSA_MAX_SWITCHES)
		return NULL;

	ds = dst->ds[source_device];
	if (!ds)
		return NULL;

#if defined(CONFIG_WG_PLATFORM_DSA_MODULE) || defined (CONFIG_WG_PLATFORM_DSA)
	if (wg_cpu_model == WG_CPU_686(3558) || wg_cpu_model == WG_CPU_686(3758)) {
		if (source_port == 0)
			source_port = ds->num_ports;
		source_port--;
	}
#endif

	if (unlikely(ds->cpu_port_mask & BIT(source_port)))
		return NULL;

	/*
	 * Convert the DSA header to an 802.1q header if the 'tagged'
	 * bit in the DSA header is set.  If the 'tagged' bit is clear,
	 * delete the DSA header entirely.
	 */
	if (dsa_header[0] & 0x20) {
		u8 new_header[4];

		/*
		 * Insert 802.1q ethertype and copy the VLAN-related
		 * fields, but clear the bit that will hold CFI (since
		 * DSA uses that bit location for another purpose).
		 */
		new_header[0] = (ETH_P_8021Q >> 8) & 0xff;
		new_header[1] = ETH_P_8021Q & 0xff;
		new_header[2] = dsa_header[2] & ~0x10;
		new_header[3] = dsa_header[3];

		/*
		 * Move CFI bit from its place in the DSA header to
		 * its 802.1q-designated place.
		 */
		if (dsa_header[1] & 0x01)
			new_header[2] |= 0x10;

		/*
		 * Update packet checksum if skb is CHECKSUM_COMPLETE.
		 */
		if (skb->ip_summed == CHECKSUM_COMPLETE) {
			__wsum c = skb->csum;
			c = csum_add(c, csum_partial(new_header + 2, 2, 0));
			c = csum_sub(c, csum_partial(dsa_header + 2, 2, 0));
			skb->csum = c;
		}

		memcpy(dsa_header, new_header, DSA_HLEN);
	} else {
		/*
		 * Remove DSA tag and update checksum.
		 */
		skb_pull_rcsum(skb, DSA_HLEN);
		memmove(skb->data - ETH_HLEN,
			skb->data - ETH_HLEN - DSA_HLEN,
			2 * ETH_ALEN);
	}

	skb->dev = ds->ports[source_port].netdev;

	return skb;
}

const struct dsa_device_ops dsa_netdev_ops = {
	.xmit	= dsa_xmit,
	.rcv	= dsa_rcv,
};

#if defined (CONFIG_WG_PLATFORM_M390) || defined(CONFIG_WG_PLATFORM_M590_M690)
// WG:XD FBX-21685. helper function
struct dsa_port* get_dsa_port_by_netdev(const struct net_device *dev)
{
    struct dsa_slave_priv *p = netdev_priv(dev);

	if (!p || !p->dp || !p->dp->cpu_dp) {
		printk(KERN_WARNING "@@ p: %p, p->dp: %p\n", p, p->dp);
		return NULL;
	}
	return p->dp->cpu_dp;
}
EXPORT_SYMBOL(get_dsa_port_by_netdev);
#endif
