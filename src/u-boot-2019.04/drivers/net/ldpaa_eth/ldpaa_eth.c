// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 */

#include <common.h>
#include <asm/io.h>
#include <asm/types.h>
#include <malloc.h>
#include <net.h>
#include <hwconfig.h>
#include <phy.h>
#include <linux/compat.h>
#include <fsl-mc/fsl_dpmac.h>

#include <fsl-mc/ldpaa_wriop.h>
#include "ldpaa_eth.h"
#define DEBUG

#ifdef CONFIG_PHYLIB
static int init_phy(struct eth_device *dev)
{
	struct ldpaa_eth_priv *priv = (struct ldpaa_eth_priv *)dev->priv;
	struct phy_device *phydev = NULL;
	struct mii_dev *bus;
	int phy_addr, phy_num;
	int ret = 0;

	bus = wriop_get_mdio(priv->dpmac_id);
	if (bus == NULL)
		return 0;

	for (phy_num = 0; phy_num < WRIOP_MAX_PHY_NUM; phy_num++) {
		phy_addr = wriop_get_phy_address(priv->dpmac_id, phy_num);
		if (phy_addr < 0)
			continue;

		phydev = phy_connect(bus, phy_addr, dev,
				     wriop_get_enet_if(priv->dpmac_id));
		if (!phydev) {
			printf("Failed to connect\n");
			ret = -ENODEV;
			break;
		}
		wriop_set_phy_dev(priv->dpmac_id, phy_num, phydev);
		ret = phy_config(phydev);
		if (ret)
			break;
	}

	if (ret) {
		for (phy_num = 0; phy_num < WRIOP_MAX_PHY_NUM; phy_num++) {
			phydev = wriop_get_phy_dev(priv->dpmac_id, phy_num);
			if (!phydev)
				continue;

			free(phydev);
			wriop_set_phy_dev(priv->dpmac_id, phy_num, NULL);
		}
	}

	return ret;
}
#endif

#ifdef DEBUG

#define DPNI_STATS_PER_PAGE 6

static const char *dpni_statistics[][DPNI_STATS_PER_PAGE] = {
	{
	"DPNI_CNT_ING_ALL_FRAMES",
	"DPNI_CNT_ING_ALL_BYTES",
	"DPNI_CNT_ING_MCAST_FRAMES",
	"DPNI_CNT_ING_MCAST_BYTES",
	"DPNI_CNT_ING_BCAST_FRAMES",
	"DPNI_CNT_ING_BCAST_BYTES",
	}, {
	"DPNI_CNT_EGR_ALL_FRAMES",
	"DPNI_CNT_EGR_ALL_BYTES",
	"DPNI_CNT_EGR_MCAST_FRAMES",
	"DPNI_CNT_EGR_MCAST_BYTES",
	"DPNI_CNT_EGR_BCAST_FRAMES",
	"DPNI_CNT_EGR_BCAST_BYTES",
	}, {
	"DPNI_CNT_ING_FILTERED_FRAMES",
	"DPNI_CNT_ING_DISCARDED_FRAMES",
	"DPNI_CNT_ING_NOBUFFER_DISCARDS",
	"DPNI_CNT_EGR_DISCARDED_FRAMES",
	"DPNI_CNT_EGR_CNF_FRAMES",
	""
	},
};

static void print_dpni_stats(const char *strings[],
			     struct dpni_statistics dpni_stats)
{
	uint64_t *stat;
	int i;

	stat = (uint64_t *)&dpni_stats;
	for (i = 0; i < DPNI_STATS_PER_PAGE; i++) {
		if (strcmp(strings[i], "\0") == 0)
			break;
		printf("%s= %llu\n", strings[i], *stat);
		stat++;
	}
}

static void ldpaa_eth_get_dpni_counter(void)
{
	int err = 0;
	unsigned int page = 0;
	struct dpni_statistics dpni_stats;

	printf("DPNI counters ..\n");
	for (page = 0; page < 3; page++) {
		err = dpni_get_statistics(dflt_mc_io, MC_CMD_NO_FLAGS,
					  dflt_dpni->dpni_handle, page,
					  &dpni_stats);
		if (err < 0) {
			printf("dpni_get_statistics: failed:");
			printf("%d for page[%d]\n", err, page);
			return;
		}
		print_dpni_stats(dpni_statistics[page], dpni_stats);
	}
}

static void ldpaa_eth_get_dpmac_counter(struct eth_device *net_dev)
{
	struct ldpaa_eth_priv *priv = (struct ldpaa_eth_priv *)net_dev->priv;
	int err = 0;
	u64 value;

	err = dpmac_get_counter(dflt_mc_io, MC_CMD_NO_FLAGS,
		     priv->dpmac_handle,
		     DPMAC_CNT_ING_BYTE,
		     &value);
	if (err < 0) {
		printf("dpmac_get_counter: DPMAC_CNT_ING_BYTE failed\n");
		return;
	}
	printf("\nDPMAC counters ..\n");
	printf("DPMAC_CNT_ING_BYTE=%lld\n", value);

	err = dpmac_get_counter(dflt_mc_io, MC_CMD_NO_FLAGS,
		     priv->dpmac_handle,
		     DPMAC_CNT_ING_FRAME_DISCARD,
		     &value);
	if (err < 0) {
		printf("dpmac_get_counter: DPMAC_CNT_ING_FRAME_DISCARD failed\n");
		return;
	}
	printf("DPMAC_CNT_ING_FRAME_DISCARD=%lld\n", value);

	err = dpmac_get_counter(dflt_mc_io, MC_CMD_NO_FLAGS,
		     priv->dpmac_handle,
		     DPMAC_CNT_ING_ALIGN_ERR,
		     &value);
	if (err < 0) {
		printf("dpmac_get_counter: DPMAC_CNT_ING_ALIGN_ERR failed\n");
		return;
	}
	printf("DPMAC_CNT_ING_ALIGN_ERR =%lld\n", value);

	err = dpmac_get_counter(dflt_mc_io, MC_CMD_NO_FLAGS,
		     priv->dpmac_handle,
		     DPMAC_CNT_ING_BYTE,
		     &value);
	if (err < 0) {
		printf("dpmac_get_counter: DPMAC_CNT_ING_BYTE failed\n");
		return;
	}
	printf("DPMAC_CNT_ING_BYTE=%lld\n", value);

	err = dpmac_get_counter(dflt_mc_io, MC_CMD_NO_FLAGS,
		     priv->dpmac_handle,
		     DPMAC_CNT_ING_ERR_FRAME,
		     &value);
	if (err < 0) {
		printf("dpmac_get_counter: DPMAC_CNT_ING_ERR_FRAME failed\n");
		return;
	}
	printf("DPMAC_CNT_ING_ERR_FRAME=%lld\n", value);

	err = dpmac_get_counter(dflt_mc_io, MC_CMD_NO_FLAGS,
		     priv->dpmac_handle,
		     DPMAC_CNT_EGR_BYTE ,
		     &value);
	if (err < 0) {
		printf("dpmac_get_counter: DPMAC_CNT_EGR_BYTE failed\n");
		return;
	}
	printf("DPMAC_CNT_EGR_BYTE =%lld\n", value);

	err = dpmac_get_counter(dflt_mc_io, MC_CMD_NO_FLAGS,
		     priv->dpmac_handle,
		     DPMAC_CNT_EGR_ERR_FRAME ,
		     &value);
	if (err < 0) {
		printf("dpmac_get_counter: DPMAC_CNT_EGR_ERR_FRAME failed\n");
		return;
	}
	printf("DPMAC_CNT_EGR_ERR_FRAME =%lld\n", value);
}
#endif

static void ldpaa_eth_rx(struct ldpaa_eth_priv *priv,
			 const struct dpaa_fd *fd)
{
	u64 fd_addr;
	uint16_t fd_offset;
	uint32_t fd_length;
	struct ldpaa_fas *fas;
	uint32_t status, err;
	u32 timeo = (CONFIG_SYS_HZ * 2) / 1000;
	u32 time_start;
	struct qbman_release_desc releasedesc;
	struct qbman_swp *swp = dflt_dpio->sw_portal;

	fd_addr = ldpaa_fd_get_addr(fd);
	fd_offset = ldpaa_fd_get_offset(fd);
	fd_length = ldpaa_fd_get_len(fd);

	debug("Rx frame:data addr=0x%p size=0x%x\n", (u64 *)fd_addr, fd_length);

	if (fd->simple.frc & LDPAA_FD_FRC_FASV) {
		/* Read the frame annotation status word and check for errors */
		fas = (struct ldpaa_fas *)
				((uint8_t *)(fd_addr) +
				dflt_dpni->buf_layout.private_data_size);
		status = le32_to_cpu(fas->status);
		if (status & LDPAA_ETH_RX_ERR_MASK) {
			printf("Rx frame error(s): 0x%08x\n",
			       status & LDPAA_ETH_RX_ERR_MASK);
			goto error;
		} else if (status & LDPAA_ETH_RX_UNSUPP_MASK) {
			printf("Unsupported feature in bitmask: 0x%08x\n",
			       status & LDPAA_ETH_RX_UNSUPP_MASK);
			goto error;
		}
	}

	debug("Rx frame: To Upper layer\n");
	net_process_received_packet((uint8_t *)(fd_addr) + fd_offset,
				    fd_length);

error:
	flush_dcache_range(fd_addr, fd_addr + LDPAA_ETH_RX_BUFFER_SIZE);
	qbman_release_desc_clear(&releasedesc);
	qbman_release_desc_set_bpid(&releasedesc, dflt_dpbp->dpbp_attr.bpid);
	time_start = get_timer(0);
	do {
		/* Release buffer into the QBMAN */
		err = qbman_swp_release(swp, &releasedesc, &fd_addr, 1);
	} while (get_timer(time_start) < timeo && err == -EBUSY);

	if (err == -EBUSY)
		printf("Rx frame: QBMAN buffer release fails\n");

	return;
}

static int ldpaa_eth_pull_dequeue_rx(struct eth_device *dev)
{
	struct ldpaa_eth_priv *priv = (struct ldpaa_eth_priv *)dev->priv;
	const struct ldpaa_dq *dq;
	const struct dpaa_fd *fd;
	int i = 5, err = 0, status;
	u32 timeo = (CONFIG_SYS_HZ * 2) / 1000;
	u32 time_start;
	static struct qbman_pull_desc pulldesc;
	struct qbman_swp *swp = dflt_dpio->sw_portal;

	while (--i) {
		qbman_pull_desc_clear(&pulldesc);
		qbman_pull_desc_set_numframes(&pulldesc, 1);
		qbman_pull_desc_set_fq(&pulldesc, priv->rx_dflt_fqid);

		err = qbman_swp_pull(swp, &pulldesc);
		if (err < 0) {
			printf("Dequeue frames error:0x%08x\n", err);
			continue;
		}

		time_start = get_timer(0);

		 do {
			dq = qbman_swp_dqrr_next(swp);
		} while (get_timer(time_start) < timeo && !dq);

		if (dq) {
			/* Check for valid frame. If not sent a consume
			 * confirmation to QBMAN otherwise give it to NADK
			 * application and then send consume confirmation to
			 * QBMAN.
			 */
			status = (uint8_t)ldpaa_dq_flags(dq);
			if ((status & LDPAA_DQ_STAT_VALIDFRAME) == 0) {
				debug("Dequeue RX frames:");
				debug("No frame delivered\n");

				qbman_swp_dqrr_consume(swp, dq);
				continue;
			}

			fd = ldpaa_dq_fd(dq);

			/* Obtain FD and process it */
			ldpaa_eth_rx(priv, fd);
			qbman_swp_dqrr_consume(swp, dq);
			break;
		} else {
			err = -ENODATA;
			debug("No DQRR entries\n");
			break;
		}
	}

	return err;
}

static int ldpaa_eth_tx(struct eth_device *net_dev, void *buf, int len)
{
	struct ldpaa_eth_priv *priv = (struct ldpaa_eth_priv *)net_dev->priv;
	struct dpaa_fd fd;
	u64 buffer_start;
	int data_offset, err;
	u32 timeo = (CONFIG_SYS_HZ * 10) / 1000;
	u32 time_start;
	struct qbman_swp *swp = dflt_dpio->sw_portal;
	struct qbman_eq_desc ed;
	struct qbman_release_desc releasedesc;

	/* Setup the FD fields */
	memset(&fd, 0, sizeof(fd));

	data_offset = priv->tx_data_offset;

	do {
		err = qbman_swp_acquire(dflt_dpio->sw_portal,
					dflt_dpbp->dpbp_attr.bpid,
					&buffer_start, 1);
	} while (err == -EBUSY);

	if (err <= 0) {
		printf("qbman_swp_acquire() failed\n");
		return -ENOMEM;
	}

	debug("TX data: malloc buffer start=0x%p\n", (u64 *)buffer_start);

	memcpy(((uint8_t *)(buffer_start) + data_offset), buf, len);

	flush_dcache_range(buffer_start, buffer_start +
					LDPAA_ETH_RX_BUFFER_SIZE);

	ldpaa_fd_set_addr(&fd, (u64)buffer_start);
	ldpaa_fd_set_offset(&fd, (uint16_t)(data_offset));
	ldpaa_fd_set_bpid(&fd, dflt_dpbp->dpbp_attr.bpid);
	ldpaa_fd_set_len(&fd, len);

	fd.simple.ctrl = LDPAA_FD_CTRL_ASAL | LDPAA_FD_CTRL_PTA |
				LDPAA_FD_CTRL_PTV1;

	qbman_eq_desc_clear(&ed);
	qbman_eq_desc_set_no_orp(&ed, 0);
	qbman_eq_desc_set_qd(&ed, priv->tx_qdid, priv->tx_flow_id, 0);

	time_start = get_timer(0);

	while (get_timer(time_start) < timeo) {
		err = qbman_swp_enqueue(swp, &ed,
				(const struct qbman_fd *)(&fd));
		if (err != -EBUSY)
			break;
	}

	if (err < 0) {
		printf("error enqueueing Tx frame\n");
		goto error;
	}

	return err;

error:
	qbman_release_desc_clear(&releasedesc);
	qbman_release_desc_set_bpid(&releasedesc, dflt_dpbp->dpbp_attr.bpid);
	time_start = get_timer(0);
	do {
		/* Release buffer into the QBMAN */
		err = qbman_swp_release(swp, &releasedesc, &buffer_start, 1);
	} while (get_timer(time_start) < timeo && err == -EBUSY);

	if (err == -EBUSY)
		printf("TX data: QBMAN buffer release fails\n");

	return err;
}

static int ldpaa_get_dpmac_state(struct ldpaa_eth_priv *priv,
				 struct dpmac_link_state *state)
{
	phy_interface_t enet_if;
	int phys_detected;
#ifdef CONFIG_PHYLIB
	struct phy_device *phydev = NULL;
	int err, phy_num;
#endif

	/* let's start off with maximum capabilities */
	enet_if = wriop_get_enet_if(priv->dpmac_id);
	switch (enet_if) {
	case PHY_INTERFACE_MODE_XGMII:
		state->rate = SPEED_10000;
		break;
	default:
		state->rate = SPEED_1000;
		break;
	}

	state->up = 1;

	phys_detected = 0;
#ifdef CONFIG_PHYLIB
	state->options |= DPMAC_LINK_OPT_AUTONEG;

	/* start the phy devices one by one and update the dpmac state */
	for (phy_num = 0; phy_num < WRIOP_MAX_PHY_NUM; phy_num++) {
		phydev = wriop_get_phy_dev(priv->dpmac_id, phy_num);
		if (!phydev)
			continue;

		phys_detected++;
		err = phy_startup(phydev);
		if (err) {
			printf("%s: Could not initialize\n", phydev->dev->name);
			state->up = 0;
			break;
		}
		if (phydev->link) {
			state->rate = min(state->rate, (uint32_t)phydev->speed);
			if (!phydev->duplex)
				state->options |= DPMAC_LINK_OPT_HALF_DUPLEX;
			if (!phydev->autoneg)
				state->options &= ~DPMAC_LINK_OPT_AUTONEG;
		} else {
			/* break out of loop even if one phy is down */
			state->up = 0;
			break;
		}
	}
#endif
	if (!phys_detected)
		state->options &= ~DPMAC_LINK_OPT_AUTONEG;

	if (!state->up) {
		state->rate = 0;
		state->options = 0;
		return -ENOLINK;
	}

#ifdef CONFIG_WG1008_PX2
	state->options &= ~DPMAC_LINK_OPT_AUTONEG;
#endif
	return 0;
}

static int ldpaa_eth_open(struct eth_device *net_dev, bd_t *bd)
{
	struct ldpaa_eth_priv *priv = (struct ldpaa_eth_priv *)net_dev->priv;
	struct dpmac_link_state	dpmac_link_state = { 0 };
#ifdef DEBUG
	struct dpni_link_state link_state;
#endif
	int err = 0;
	struct dpni_queue d_queue;

	if (net_dev->state == ETH_STATE_ACTIVE)
		return 0;

	if (get_mc_boot_status() != 0) {
		printf("ERROR (MC is not booted)\n");
		return -ENODEV;
	}

	if (get_dpl_apply_status() == 0) {
		printf("ERROR (DPL is deployed. No device available)\n");
		return -ENODEV;
	}

	/* DPMAC initialization */
	err = ldpaa_dpmac_setup(priv);
	if (err < 0)
		goto err_dpmac_setup;

	err = ldpaa_get_dpmac_state(priv, &dpmac_link_state);
	if (err < 0)
		goto err_dpmac_bind;

	/* DPMAC binding DPNI */
	err = ldpaa_dpmac_bind(priv);
	if (err)
		goto err_dpmac_bind;

	/* DPNI initialization */
	err = ldpaa_dpni_setup(priv);
	if (err < 0)
		goto err_dpni_setup;

	err = ldpaa_dpbp_setup();
	if (err < 0)
		goto err_dpbp_setup;

	/* DPNI binding DPBP */
	err = ldpaa_dpni_bind(priv);
	if (err)
		goto err_dpni_bind;

	err = dpni_add_mac_addr(dflt_mc_io, MC_CMD_NO_FLAGS,
				dflt_dpni->dpni_handle, net_dev->enetaddr);
	if (err) {
		printf("dpni_add_mac_addr() failed\n");
		return err;
	}

	err = dpni_enable(dflt_mc_io, MC_CMD_NO_FLAGS, dflt_dpni->dpni_handle);
	if (err < 0) {
		printf("dpni_enable() failed\n");
		return err;
	}

	err = dpmac_set_link_state(dflt_mc_io, MC_CMD_NO_FLAGS,
				  priv->dpmac_handle, &dpmac_link_state);
	if (err < 0) {
		printf("dpmac_set_link_state() failed\n");
		return err;
	}

#ifdef DEBUG
	printf("DPMAC link status: %d - ", dpmac_link_state.up);
	dpmac_link_state.up == 0 ? printf("down\n") :
	dpmac_link_state.up == 1 ? printf("up\n") : printf("error state\n");

	err = dpni_get_link_state(dflt_mc_io, MC_CMD_NO_FLAGS,
				  dflt_dpni->dpni_handle, &link_state);
	if (err < 0) {
		printf("dpni_get_link_state() failed\n");
		return err;
	}

	printf("DPNI link status: %d - ", link_state.up);
	link_state.up == 0 ? printf("down\n") :
	link_state.up == 1 ? printf("up\n") : printf("error state\n");
#endif

	memset(&d_queue, 0, sizeof(struct dpni_queue));
	err = dpni_get_queue(dflt_mc_io, MC_CMD_NO_FLAGS,
			     dflt_dpni->dpni_handle, DPNI_QUEUE_RX,
			     0, 0, &d_queue);
	if (err) {
		printf("dpni_get_queue failed\n");
		goto err_get_queue;
	}

	priv->rx_dflt_fqid = d_queue.fqid;

	err = dpni_get_qdid(dflt_mc_io, MC_CMD_NO_FLAGS, dflt_dpni->dpni_handle,
			    &priv->tx_qdid);
	if (err) {
		printf("dpni_get_qdid() failed\n");
		goto err_qdid;
	}

	return dpmac_link_state.up;

err_qdid:
err_get_queue:
	dpni_disable(dflt_mc_io, MC_CMD_NO_FLAGS, dflt_dpni->dpni_handle);
err_dpni_bind:
	ldpaa_dpbp_free();
err_dpbp_setup:
	dpni_close(dflt_mc_io, MC_CMD_NO_FLAGS, dflt_dpni->dpni_handle);
err_dpni_setup:
err_dpmac_bind:
	dpmac_close(dflt_mc_io, MC_CMD_NO_FLAGS, priv->dpmac_handle);
	dpmac_destroy(dflt_mc_io,
		      dflt_dprc_handle,
		      MC_CMD_NO_FLAGS, priv->dpmac_id);
err_dpmac_setup:
	return err;
}

static void ldpaa_eth_stop(struct eth_device *net_dev)
{
	struct ldpaa_eth_priv *priv = (struct ldpaa_eth_priv *)net_dev->priv;
	int err = 0;
#ifdef CONFIG_PHYLIB
	struct phy_device *phydev = NULL;
	int phy_num;
#endif

	if ((net_dev->state == ETH_STATE_PASSIVE) ||
	    (net_dev->state == ETH_STATE_INIT))
		return;

#ifdef DEBUG
	ldpaa_eth_get_dpni_counter();
	ldpaa_eth_get_dpmac_counter(net_dev);
#endif

	err = dprc_disconnect(dflt_mc_io, MC_CMD_NO_FLAGS,
			      dflt_dprc_handle, &dpmac_endpoint);
	if (err < 0)
		printf("dprc_disconnect() failed dpmac_endpoint\n");

	err = dpmac_close(dflt_mc_io, MC_CMD_NO_FLAGS, priv->dpmac_handle);
	if (err < 0)
		printf("dpmac_close() failed\n");

	err = dpmac_destroy(dflt_mc_io,
			    dflt_dprc_handle,
			    MC_CMD_NO_FLAGS,
			    priv->dpmac_id);
	if (err < 0)
		printf("dpmac_destroy() failed\n");

	/* Stop Tx and Rx traffic */
	err = dpni_disable(dflt_mc_io, MC_CMD_NO_FLAGS, dflt_dpni->dpni_handle);
	if (err < 0)
		printf("dpni_disable() failed\n");

#ifdef CONFIG_PHYLIB
	for (phy_num = 0; phy_num < WRIOP_MAX_PHY_NUM; phy_num++) {
		phydev = wriop_get_phy_dev(priv->dpmac_id, phy_num);
		if (phydev)
			phy_shutdown(phydev);
	}
#endif

	/* Free DPBP handle and reset. */
	ldpaa_dpbp_free();

	dpni_reset(dflt_mc_io, MC_CMD_NO_FLAGS, dflt_dpni->dpni_handle);
	if (err < 0)
		printf("dpni_reset() failed\n");

	dpni_close(dflt_mc_io, MC_CMD_NO_FLAGS, dflt_dpni->dpni_handle);
	if (err < 0)
		printf("dpni_close() failed\n");
}

static void ldpaa_dpbp_drain_cnt(int count)
{
	uint64_t buf_array[7];
	void *addr;
	int ret, i;

	BUG_ON(count > 7);

	do {
		ret = qbman_swp_acquire(dflt_dpio->sw_portal,
					dflt_dpbp->dpbp_attr.bpid,
					buf_array, count);
		if (ret < 0) {
			printf("qbman_swp_acquire() failed\n");
			return;
		}
		for (i = 0; i < ret; i++) {
			addr = (void *)buf_array[i];
			debug("Free: buffer addr =0x%p\n", addr);
			free(addr);
		}
	} while (ret);
}

static void ldpaa_dpbp_drain(void)
{
	int i;
	for (i = 0; i < LDPAA_ETH_NUM_BUFS; i += 7)
		ldpaa_dpbp_drain_cnt(7);
}

static int ldpaa_bp_add_7(uint16_t bpid)
{
	uint64_t buf_array[7];
	u8 *addr;
	int i;
	struct qbman_release_desc rd;

	for (i = 0; i < 7; i++) {
		addr = memalign(LDPAA_ETH_BUF_ALIGN, LDPAA_ETH_RX_BUFFER_SIZE);
		if (!addr) {
			printf("addr allocation failed\n");
			goto err_alloc;
		}
		memset(addr, 0x00, LDPAA_ETH_RX_BUFFER_SIZE);
		flush_dcache_range((u64)addr,
				   (u64)(addr + LDPAA_ETH_RX_BUFFER_SIZE));

		buf_array[i] = (uint64_t)addr;
		debug("Release: buffer addr =0x%p\n", addr);
	}

release_bufs:
	/* In case the portal is busy, retry until successful.
	 * This function is guaranteed to succeed in a reasonable amount
	 * of time.
	 */

	do {
		mdelay(1);
		qbman_release_desc_clear(&rd);
		qbman_release_desc_set_bpid(&rd, bpid);
	} while (qbman_swp_release(dflt_dpio->sw_portal, &rd, buf_array, i));

	return i;

err_alloc:
	if (i)
		goto release_bufs;

	return 0;
}

static int ldpaa_dpbp_seed(uint16_t bpid)
{
	int i;
	int count;

	for (i = 0; i < LDPAA_ETH_NUM_BUFS; i += 7) {
		count = ldpaa_bp_add_7(bpid);
		if (count < 7)
			printf("Buffer Seed= %d\n", count);
	}

	return 0;
}

static int ldpaa_dpbp_setup(void)
{
	int err;

	err = dpbp_open(dflt_mc_io, MC_CMD_NO_FLAGS, dflt_dpbp->dpbp_attr.id,
			&dflt_dpbp->dpbp_handle);
	if (err) {
		printf("dpbp_open() failed\n");
		goto err_open;
	}

	err = dpbp_enable(dflt_mc_io, MC_CMD_NO_FLAGS, dflt_dpbp->dpbp_handle);
	if (err) {
		printf("dpbp_enable() failed\n");
		goto err_enable;
	}

	err = dpbp_get_attributes(dflt_mc_io, MC_CMD_NO_FLAGS,
				  dflt_dpbp->dpbp_handle,
				  &dflt_dpbp->dpbp_attr);
	if (err) {
		printf("dpbp_get_attributes() failed\n");
		goto err_get_attr;
	}

	err = ldpaa_dpbp_seed(dflt_dpbp->dpbp_attr.bpid);

	if (err) {
		printf("Buffer seeding failed for DPBP %d (bpid=%d)\n",
		       dflt_dpbp->dpbp_attr.id, dflt_dpbp->dpbp_attr.bpid);
		goto err_seed;
	}

	return 0;

err_seed:
err_get_attr:
	dpbp_disable(dflt_mc_io, MC_CMD_NO_FLAGS, dflt_dpbp->dpbp_handle);
err_enable:
	dpbp_close(dflt_mc_io, MC_CMD_NO_FLAGS, dflt_dpbp->dpbp_handle);
err_open:
	return err;
}

static void ldpaa_dpbp_free(void)
{
	ldpaa_dpbp_drain();
	dpbp_disable(dflt_mc_io, MC_CMD_NO_FLAGS, dflt_dpbp->dpbp_handle);
	dpbp_reset(dflt_mc_io, MC_CMD_NO_FLAGS, dflt_dpbp->dpbp_handle);
	dpbp_close(dflt_mc_io, MC_CMD_NO_FLAGS, dflt_dpbp->dpbp_handle);
}

static int ldpaa_dpmac_version_check(struct fsl_mc_io *mc_io,
				     struct ldpaa_eth_priv *priv)
{
	int error;
	uint16_t major_ver, minor_ver;

	error = dpmac_get_api_version(dflt_mc_io, 0,
					&major_ver,
					&minor_ver);
	if ((major_ver < DPMAC_VER_MAJOR) ||
	    (major_ver == DPMAC_VER_MAJOR && minor_ver < DPMAC_VER_MINOR)) {
		printf("DPMAC version mismatch found %u.%u,",
		       major_ver, minor_ver);
		printf("supported version is %u.%u\n",
		       DPMAC_VER_MAJOR, DPMAC_VER_MINOR);
		return error;
	}

	return error;
}

static int ldpaa_dpmac_setup(struct ldpaa_eth_priv *priv)
{
	int err = 0;
	struct dpmac_cfg dpmac_cfg;

	dpmac_cfg.mac_id = priv->dpmac_id;

	err = dpmac_create(dflt_mc_io,
			   dflt_dprc_handle,
			   MC_CMD_NO_FLAGS, &dpmac_cfg,
			   &priv->dpmac_id);
	if (err)
		printf("dpmac_create() failed\n");

	err = ldpaa_dpmac_version_check(dflt_mc_io, priv);
	if (err < 0) {
		printf("ldpaa_dpmac_version_check() failed: %d\n", err);
		goto err_version_check;
	}

	err = dpmac_open(dflt_mc_io,
			 MC_CMD_NO_FLAGS,
			 priv->dpmac_id,
			 &priv->dpmac_handle);
	if (err < 0) {
		printf("dpmac_open() failed: %d\n", err);
		goto err_open;
	}

	return err;

err_open:
err_version_check:
	dpmac_destroy(dflt_mc_io,
		      dflt_dprc_handle,
		      MC_CMD_NO_FLAGS, priv->dpmac_id);

	return err;
}

static int ldpaa_dpmac_bind(struct ldpaa_eth_priv *priv)
{
	int err = 0;
	struct dprc_connection_cfg dprc_connection_cfg = {
		/* If both rates are zero the connection */
		/* will be configured in "best effort" mode. */
		.committed_rate = 0,
		.max_rate = 0
	};

#ifdef DEBUG
	struct dprc_endpoint dbg_endpoint;
	int state = 0;
#endif

	memset(&dpmac_endpoint, 0, sizeof(struct dprc_endpoint));
	strcpy(dpmac_endpoint.type, "dpmac");
	dpmac_endpoint.id = priv->dpmac_id;

	memset(&dpni_endpoint, 0, sizeof(struct dprc_endpoint));
	strcpy(dpni_endpoint.type, "dpni");
	dpni_endpoint.id = dflt_dpni->dpni_id;

	err = dprc_connect(dflt_mc_io, MC_CMD_NO_FLAGS,
			     dflt_dprc_handle,
			     &dpmac_endpoint,
			     &dpni_endpoint,
			     &dprc_connection_cfg);
	if (err)
		printf("dprc_connect() failed\n");

#ifdef DEBUG
	err = dprc_get_connection(dflt_mc_io, MC_CMD_NO_FLAGS,
				    dflt_dprc_handle, &dpni_endpoint,
				    &dbg_endpoint, &state);
	printf("%s, DPMAC Type= %s\n", __func__, dbg_endpoint.type);
	printf("%s, DPMAC ID= %d\n", __func__, dbg_endpoint.id);
	printf("%s, DPMAC State= %d\n", __func__, state);

	memset(&dbg_endpoint, 0, sizeof(struct dprc_endpoint));
	err = dprc_get_connection(dflt_mc_io, MC_CMD_NO_FLAGS,
				    dflt_dprc_handle, &dpmac_endpoint,
				    &dbg_endpoint, &state);
	printf("%s, DPNI Type= %s\n", __func__, dbg_endpoint.type);
	printf("%s, DPNI ID= %d\n", __func__, dbg_endpoint.id);
	printf("%s, DPNI State= %d\n", __func__, state);
#endif
	return err;
}

static int ldpaa_dpni_setup(struct ldpaa_eth_priv *priv)
{
	int err;

	/* and get a handle for the DPNI this interface is associate with */
	err = dpni_open(dflt_mc_io, MC_CMD_NO_FLAGS, dflt_dpni->dpni_id,
			&dflt_dpni->dpni_handle);
	if (err) {
		printf("dpni_open() failed\n");
		goto err_open;
	}
	err = dpni_get_attributes(dflt_mc_io, MC_CMD_NO_FLAGS,
				  dflt_dpni->dpni_handle,
				  &dflt_dpni->dpni_attrs);
	if (err) {
		printf("dpni_get_attributes() failed (err=%d)\n", err);
		goto err_get_attr;
	}

	/* Configure our buffers' layout */
	dflt_dpni->buf_layout.options = DPNI_BUF_LAYOUT_OPT_PARSER_RESULT |
				   DPNI_BUF_LAYOUT_OPT_FRAME_STATUS |
				   DPNI_BUF_LAYOUT_OPT_PRIVATE_DATA_SIZE |
				   DPNI_BUF_LAYOUT_OPT_DATA_ALIGN;
	dflt_dpni->buf_layout.pass_parser_result = true;
	dflt_dpni->buf_layout.pass_frame_status = true;
	dflt_dpni->buf_layout.private_data_size = LDPAA_ETH_SWA_SIZE;
	/* HW erratum mandates data alignment in multiples of 256 */
	dflt_dpni->buf_layout.data_align = LDPAA_ETH_BUF_ALIGN;

	/* ...rx, ... */
	err = dpni_set_buffer_layout(dflt_mc_io, MC_CMD_NO_FLAGS,
				     dflt_dpni->dpni_handle,
				     &dflt_dpni->buf_layout, DPNI_QUEUE_RX);
	if (err) {
		printf("dpni_set_buffer_layout() failed");
		goto err_buf_layout;
	}

	/* ... tx, ... */
	/* remove Rx-only options */
	dflt_dpni->buf_layout.options &= ~(DPNI_BUF_LAYOUT_OPT_DATA_ALIGN |
				      DPNI_BUF_LAYOUT_OPT_PARSER_RESULT);
	err = dpni_set_buffer_layout(dflt_mc_io, MC_CMD_NO_FLAGS,
				     dflt_dpni->dpni_handle,
				     &dflt_dpni->buf_layout, DPNI_QUEUE_TX);
	if (err) {
		printf("dpni_set_buffer_layout() failed");
		goto err_buf_layout;
	}

	/* ... tx-confirm. */
	dflt_dpni->buf_layout.options &= ~DPNI_BUF_LAYOUT_OPT_PRIVATE_DATA_SIZE;
	err = dpni_set_buffer_layout(dflt_mc_io, MC_CMD_NO_FLAGS,
				     dflt_dpni->dpni_handle,
				     &dflt_dpni->buf_layout,
				     DPNI_QUEUE_TX_CONFIRM);
	if (err) {
		printf("dpni_set_buffer_layout() failed");
		goto err_buf_layout;
	}

	/* Now that we've set our tx buffer layout, retrieve the minimum
	 * required tx data offset.
	 */
	err = dpni_get_tx_data_offset(dflt_mc_io, MC_CMD_NO_FLAGS,
				      dflt_dpni->dpni_handle,
				      &priv->tx_data_offset);
	if (err) {
		printf("dpni_get_tx_data_offset() failed\n");
		goto err_data_offset;
	}

	/* Warn in case TX data offset is not multiple of 64 bytes. */
	WARN_ON(priv->tx_data_offset % 64);

	/* Accomodate SWA space. */
	priv->tx_data_offset += LDPAA_ETH_SWA_SIZE;
	debug("priv->tx_data_offset=%d\n", priv->tx_data_offset);

	return 0;

err_data_offset:
err_buf_layout:
err_get_attr:
	dpni_close(dflt_mc_io, MC_CMD_NO_FLAGS, dflt_dpni->dpni_handle);
err_open:
	return err;
}

static int ldpaa_dpni_bind(struct ldpaa_eth_priv *priv)
{
	struct dpni_pools_cfg pools_params;
	struct dpni_queue tx_queue;
	int err = 0;

	memset(&pools_params, 0, sizeof(pools_params));
	pools_params.num_dpbp = 1;
	pools_params.pools[0].dpbp_id = (uint16_t)dflt_dpbp->dpbp_attr.id;
	pools_params.pools[0].buffer_size = LDPAA_ETH_RX_BUFFER_SIZE;
	err = dpni_set_pools(dflt_mc_io, MC_CMD_NO_FLAGS,
			     dflt_dpni->dpni_handle, &pools_params);
	if (err) {
		printf("dpni_set_pools() failed\n");
		return err;
	}

	memset(&tx_queue, 0, sizeof(struct dpni_queue));

	err = dpni_set_queue(dflt_mc_io, MC_CMD_NO_FLAGS,
			     dflt_dpni->dpni_handle,
			     DPNI_QUEUE_TX, 0, 0, &tx_queue);

	if (err) {
		printf("dpni_set_queue() failed\n");
		return err;
	}

	err = dpni_set_tx_confirmation_mode(dflt_mc_io, MC_CMD_NO_FLAGS,
					    dflt_dpni->dpni_handle,
					    DPNI_CONF_DISABLE);
	if (err) {
		printf("dpni_set_tx_confirmation_mode() failed\n");
		return err;
	}

	return 0;
}

#ifdef CONFIG_MV88E6191X_SWITCH
#define DBG_PRINTF(format, args...) printf("[%s:%d] "format, __FUNCTION__, __LINE__, ##args)
#define GPORT_START_NUM 1
#define GPORT_END_NUM 8
#define CPU_PORT_9 9
#define CPU_PORT_10 10

#define MV88E6190_PAGE_ACCESS_REG             0x16
#define MV88E6190_COPPER_CTRL_PAGE            0x0
#define MV88E6190_COPPER_CTRL_REG             0x0
#define MV88E6190_COPPER_CTRL_SW_RESET_BIT    15
#define MV88E6190_COPPER_CTRL_LB_BIT          14
#define MV88E6190_COPPER_CTRL_SPD_SEL_LSB_BIT 13 /* Bit 6,3 */
#define MV88E6190_COPPER_CTRL_SPD_SEL_MSB_BIT  6 /* 11 = Reserved, 10 = 1000 Mbps, 01 = 100 Mbps, 00 = 10 Mbps */
#define MV88E6190_COPPER_CTRL_AN_EN_BIT       12
#define MV88E6190_COPPER_CTRL_PWR_DOWN_BIT    11
#define MV88E6190_COPPER_CTRL_RESTART_AN_BIT   9
#define MV88E6190_COPPER_CTRL_DUPLEX_MOD_BIT   8
#define MV88E6190_COPPER_CTRL_DUPLEX_MOD_BIT   8

#define MV88E6190_COPPER_SPECIFIC_CTRL_1_PAGE 0x0
#define MV88E6190_COPPER_SPECIFIC_CTRL_1_REG 0x10
#define MV88E6190_COPPER_SPECIFIC_CTRL_MDI_MOD_0_BIT 5
#define MV88E6190_COPPER_SPECIFIC_CTRL_MDI_MOD_1_BIT 6

#define MV88E6190_COPPER_STATUS_PAGE                 0x0
#define MV88E6190_COPPER_STATUS_REG                  0x1
#define MV88E6190_COPPER_STATUS_COP_AN_COMP_BIT      5
#define MV88E6190_COPPER_STATUS_COP_REMOTE_FAULT_BIT 4
#define MV88E6190_COPPER_STATUS_COP_LINK_STATUS_BIT  2
#define MV88E6190_COPPER_STATUS_JABBER_DETECT_BIT    1
#define MAX_AN_COMPLETE_WAIT_RETRY       100
#define MAX_AN_COMPLETE_WAIT_RETRY_DELAY 100000 /* micro-seconds */
#define DEFAULT_AN_DELAY 4 /* seconds */

#define MV88E6190_COPPER_SPECIFIC_STATUS_1_PAGE                0x0
#define MV88E6190_COPPER_SPECIFIC_STATUS_1_REG                0x11
#define MV88E6190_COPPER_SPECIFIC_STATUS_1_SPEED_BIT            14
#define MV88E6190_COPPER_SPECIFIC_STATUS_1_SPEED_MASK          0x3
#define MV88E6190_COPPER_SPECIFIC_STATUS_1_DUPLEX_BIT           13
#define MV88E6190_COPPER_SPECIFIC_STATUS_1_COP_LINK_RT_BIT      10
#define MV88E6190_COPPER_SPECIFIC_STATUS_1_MDI_BIT               6
#define MV88E6190_COPPER_SPECIFIC_STATUS_1_GLOB_LINK_STATUS_BIT  3
#define MV88E6190_COPPER_SPECIFIC_STATUS_1_DTE_PWR_STATUS_BIT    2
#define MV88E6190_COPPER_SPECIFIC_STATUS_1_POLARITY_RT_BIT       1
#define MV88E6190_COPPER_SPECIFIC_STATUS_1_JABBER_RT_BIT         0

int switch_mv88e6190_port_autonego(u16 port, u32 *an, int read)
{
    u16 val = 0;

    if(port > GPORT_END_NUM || port < GPORT_START_NUM)
    {
        printf("Invalid port %u!!!\n", port);
        return -1;
    }
    /* Switch Page */
    if(marvell_phy_write(port, MV88E6190_PAGE_ACCESS_REG, MV88E6190_COPPER_CTRL_PAGE))
    {
        printf("Failed to write port 0x%x, reg 0x%x, val 0x%x!!!\n", port, MV88E6190_COPPER_CTRL_REG, MV88E6190_COPPER_CTRL_PAGE);
        return -1;
    }

    if(marvell_phy_read(port, MV88E6190_COPPER_CTRL_REG, &val))
    {
        printf("Failed to read port 0x%x, reg 0x%x!!!\n", port, MV88E6190_COPPER_CTRL_REG);
        return -1;
    }

    if(read)
    {
        *an = (val & (1<<MV88E6190_COPPER_CTRL_AN_EN_BIT))?1:0;
        return 0;
    }
    if(*an)
    {
        val |= (1<<MV88E6190_COPPER_CTRL_AN_EN_BIT);
        val |= (1<<MV88E6190_COPPER_CTRL_SW_RESET_BIT);
    }
    else
    {
        val &= ~(1<<MV88E6190_COPPER_CTRL_AN_EN_BIT);
        val |= (1<<MV88E6190_COPPER_CTRL_SW_RESET_BIT);
    }

    if(marvell_phy_write(port, MV88E6190_COPPER_CTRL_REG, val))
    {
        printf("Failed to write port 0x%x, reg 0x%x, val 0x%x!!!\n", port, MV88E6190_COPPER_CTRL_REG, val);
        return -1;
    }

    return 0;
}

int switch_mv88e6190_port_speed(u16 port, u32 *speed, int read) /* speed unit : Mbps */
{
    u16 val = 0;

    if(port > GPORT_END_NUM || port < GPORT_START_NUM)
    {
        printf("Invalid port %u!!!\n", port);
        return -1;
    }
    /* Switch Page */
    if(marvell_phy_write(port, MV88E6190_PAGE_ACCESS_REG, MV88E6190_COPPER_CTRL_PAGE))
    {
        printf("Failed to write port 0x%x, reg 0x%x, val 0x%x!!!\n", port, MV88E6190_COPPER_CTRL_REG, MV88E6190_COPPER_CTRL_PAGE);
        return -1;
    }

    if(marvell_phy_read(port, MV88E6190_COPPER_CTRL_REG, &val))
    {
        printf("Failed to read port 0x%x, reg 0x%x!!!\n", port, MV88E6190_COPPER_CTRL_REG);
        return -1;
    }
    /* Bit 6,3 : 11 = Reserved, 10 = 1000 Mbps, 01 = 100 Mbps, 00 = 10 Mbps */
    if(read)
    {
        u16 msb = (val & (1<<MV88E6190_COPPER_CTRL_SPD_SEL_MSB_BIT))?1:0;
        u16 lsb = (val & (1<<MV88E6190_COPPER_CTRL_SPD_SEL_LSB_BIT))?1:0;
        if(msb && !lsb)
        {
            *speed = 1000;
        }
        else if(!msb && lsb)
        {
            *speed = 100;
        }
        else if(!msb && !lsb)
        {
            *speed = 10;
        }
        else
        {
            *speed = 0;
            return -1;
        }
        return 0;
    }

    /* Bit 6,3 : 11 = Reserved, 10 = 1000 Mbps, 01 = 100 Mbps, 00 = 10 Mbps */
    if(*speed == 10)
    {
        val &= ~(1<<MV88E6190_COPPER_CTRL_SPD_SEL_MSB_BIT);
        val &= ~(1<<MV88E6190_COPPER_CTRL_SPD_SEL_LSB_BIT);
        val |= (1<<MV88E6190_COPPER_CTRL_SW_RESET_BIT);
    }
    else if(*speed == 100)
    {
        val &= ~(1<<MV88E6190_COPPER_CTRL_SPD_SEL_MSB_BIT);
        val |= (1<<MV88E6190_COPPER_CTRL_SPD_SEL_LSB_BIT);
        val |= (1<<MV88E6190_COPPER_CTRL_SW_RESET_BIT);
    }
    else if(*speed == 1000)
    {
        val |= (1<<MV88E6190_COPPER_CTRL_SPD_SEL_MSB_BIT);
        val &= ~(1<<MV88E6190_COPPER_CTRL_SPD_SEL_LSB_BIT);
        val |= (1<<MV88E6190_COPPER_CTRL_SW_RESET_BIT);
    }
    else
    {
        printf("Port %u invalid speed %u!!!\n", port, *speed);
        return -1;
    }

    if(marvell_phy_write(port, MV88E6190_COPPER_CTRL_REG, val))
    {
        printf("Failed to write port 0x%x, reg 0x%x, val 0x%x!!!\n", port, MV88E6190_COPPER_CTRL_REG, val);
        return -1;
    }

    return 0;
}

int switch_mv88e6190_port_duplex(u16 port, u32 *fullDupex, int read)
{
    u16 val = 0;

    if(port > GPORT_END_NUM || port < GPORT_START_NUM)
    {
        printf("Invalid port %u!!!\n", port);
        return -1;
    }
    /* Switch Page */
    if(marvell_phy_write(port, MV88E6190_PAGE_ACCESS_REG, MV88E6190_COPPER_CTRL_PAGE))
    {
        printf("Failed to write port 0x%x, reg 0x%x, val 0x%x!!!\n", port, MV88E6190_COPPER_CTRL_REG, MV88E6190_COPPER_CTRL_PAGE);
        return -1;
    }
    if(marvell_phy_read(port, MV88E6190_COPPER_CTRL_REG, &val))
    {
        printf("Failed to read port 0x%x, reg 0x%x!!!\n", port, MV88E6190_COPPER_CTRL_REG);
        return -1;
    }
    if(read)
    {
        *fullDupex = (val & (1<<MV88E6190_COPPER_CTRL_DUPLEX_MOD_BIT))?1:0;
        return 0;
    }

    if(*fullDupex)
    {
        val |= (1<<MV88E6190_COPPER_CTRL_DUPLEX_MOD_BIT);
        val |= (1<<MV88E6190_COPPER_CTRL_SW_RESET_BIT);
    }
    else
    {
        val &= ~(1<<MV88E6190_COPPER_CTRL_DUPLEX_MOD_BIT);
        val |= (1<<MV88E6190_COPPER_CTRL_SW_RESET_BIT);
    }

    if(marvell_phy_write(port, MV88E6190_COPPER_CTRL_REG, val))
    {
        printf("Failed to write port 0x%x, reg 0x%x, val 0x%x!!!\n", port, MV88E6190_COPPER_CTRL_REG, val);
        return -1;
    }

    return 0;
}

int switch_mv88e6190_port_mdi_mode(u16 port, u32 *mdiMode, int read)
{
    u16 val = 0, msb = 0, lsb = 0;

    if(port > GPORT_END_NUM || port < GPORT_START_NUM)
    {
        printf("Invalid port %u!!!\n", port);
        return -1;
    }
    /* Switch Page */
    if(marvell_phy_write(port, MV88E6190_PAGE_ACCESS_REG, MV88E6190_COPPER_SPECIFIC_CTRL_1_PAGE))
    {
        printf("Failed to write port 0x%x, reg 0x%x, val 0x%x!!!\n", port, MV88E6190_PAGE_ACCESS_REG, MV88E6190_COPPER_SPECIFIC_CTRL_1_PAGE);
        return -1;
    }
    if(marvell_phy_read(port, MV88E6190_COPPER_SPECIFIC_CTRL_1_REG, &val))
    {
        printf("Failed to read port 0x%x, reg 0x%x!!!\n", port, MV88E6190_COPPER_SPECIFIC_CTRL_1_REG);
        return -1;
    }
    /*
     00 = Manual MDI
     01 = Manual MDIX
     10 = Resverved
     11 = Enable auto crossover for all modes
     */
    if(read)
    {
        msb = (val & (1<<MV88E6190_COPPER_SPECIFIC_CTRL_MDI_MOD_1_BIT))?1:0;
        lsb = (val & (1<<MV88E6190_COPPER_SPECIFIC_CTRL_MDI_MOD_0_BIT))?1:0;
        *mdiMode = lsb | (msb << 1);
        return 0;
    }

    lsb = (*mdiMode & (1<<0))?1:0;
    msb = (*mdiMode & (1<<1))?1:0;
    if(lsb)
    {
        val |= (1<<MV88E6190_COPPER_SPECIFIC_CTRL_MDI_MOD_0_BIT);
    }
    else
    {
        val &= ~(1<<MV88E6190_COPPER_SPECIFIC_CTRL_MDI_MOD_0_BIT);
    }
    if(msb)
    {
        val |= (1<<MV88E6190_COPPER_SPECIFIC_CTRL_MDI_MOD_1_BIT);
    }
    else
    {
        val &= ~(1<<MV88E6190_COPPER_SPECIFIC_CTRL_MDI_MOD_1_BIT);
    }

    if(marvell_phy_write(port, MV88E6190_COPPER_SPECIFIC_CTRL_1_REG, val))
    {
        printf("Failed to write port 0x%x, reg 0x%x, val 0x%x!!!\n", port, MV88E6190_COPPER_SPECIFIC_CTRL_1_REG, val);
        return -1;
    }

    /* Software Reset to take effect */
    if(marvell_phy_read(port, MV88E6190_COPPER_CTRL_REG, &val))
    {
        printf("Failed to read port 0x%x, reg 0x%x!!!\n", port, MV88E6190_COPPER_CTRL_REG);
        return -1;
    }
    val |= (1<<MV88E6190_COPPER_CTRL_SW_RESET_BIT);
    if(marvell_phy_write(port, MV88E6190_COPPER_CTRL_REG, val))
    {
        printf("Failed to write port 0x%x, reg 0x%x, val 0x%x!!!\n", port, MV88E6190_COPPER_CTRL_REG, val);
        return -1;
    }
    /* Software Reset to take effect */

    return 0;
}

int switch_mv88e6190_port_copper_link_status_read(u16 port, u32 *status, int read)
{
    u16 val = 0;

    if(port > GPORT_END_NUM || port < GPORT_START_NUM)
    {
        printf("Invalid port %u!!!\n", port);
        return -1;
    }
    /* Switch Page */
    if(marvell_phy_write(port, MV88E6190_PAGE_ACCESS_REG, MV88E6190_COPPER_STATUS_PAGE))
    {
        printf("Failed to write port 0x%x, reg 0x%x, val 0x%x!!!\n", port, MV88E6190_PAGE_ACCESS_REG, MV88E6190_COPPER_STATUS_PAGE);
        return -1;
    }
    if(marvell_phy_read(port, MV88E6190_COPPER_STATUS_REG, &val))
    {
        printf("Failed to read port 0x%x, reg 0x%x!!!\n", port, MV88E6190_COPPER_STATUS_REG);
        return -1;
    }
    *status = (val & (1<<MV88E6190_COPPER_STATUS_COP_LINK_STATUS_BIT))?1:0;

    if(marvell_phy_write(port, MV88E6190_PAGE_ACCESS_REG, 0))
    {
        printf("Failed to write port 0x%x, reg 0x%x, val 0x%x!!!\n", port, MV88E6190_PAGE_ACCESS_REG, 0);
        return -1;
    }

    return 0;
}

int switch_mv88e6190_port_copper_remote_fault_read(u16 port, u32 *fault, int read)
{
    u16 val = 0;

    if(port > GPORT_END_NUM || port < GPORT_START_NUM)
    {
        printf("Invalid port %u!!!\n", port);
        return -1;
    }
    /* Switch Page */
    if(marvell_phy_write(port, MV88E6190_PAGE_ACCESS_REG, MV88E6190_COPPER_STATUS_PAGE))
    {
        printf("Failed to write port 0x%x, reg 0x%x, val 0x%x!!!\n", port, MV88E6190_PAGE_ACCESS_REG, MV88E6190_COPPER_STATUS_PAGE);
        return -1;
    }
    if(marvell_phy_read(port, MV88E6190_COPPER_STATUS_REG, &val))
    {
        printf("Failed to read port 0x%x, reg 0x%x!!!\n", port, MV88E6190_COPPER_STATUS_REG);
        return -1;
    }
    *fault = (val & (1<<MV88E6190_COPPER_STATUS_COP_REMOTE_FAULT_BIT))?1:0;

    if(marvell_phy_write(port, MV88E6190_PAGE_ACCESS_REG, 0))
    {
        printf("Failed to write port 0x%x, reg 0x%x, val 0x%x!!!\n", port, MV88E6190_PAGE_ACCESS_REG, 0);
        return -1;
    }

    return 0;
}

int switch_mv88e6190_port_jabber_detect_read(u16 port, u32 *jabber, int read)
{
    u16 val = 0;

    if(port > GPORT_END_NUM || port < GPORT_START_NUM)
    {
        printf("Invalid port %u!!!\n", port);
        return -1;
    }
    /* Switch Page */
    if(marvell_phy_write(port, MV88E6190_PAGE_ACCESS_REG, MV88E6190_COPPER_STATUS_PAGE))
    {
        printf("Failed to write port 0x%x, reg 0x%x, val 0x%x!!!\n", port, MV88E6190_PAGE_ACCESS_REG, MV88E6190_COPPER_STATUS_PAGE);
        return -1;
    }
    if(marvell_phy_read(port, MV88E6190_COPPER_STATUS_REG, &val))
    {
        printf("Failed to read port 0x%x, reg 0x%x!!!\n", port, MV88E6190_COPPER_STATUS_REG);
        return -1;
    }
    *jabber = (val & (1<<MV88E6190_COPPER_STATUS_JABBER_DETECT_BIT))?1:0;

    if(marvell_phy_write(port, MV88E6190_PAGE_ACCESS_REG, 0))
    {
        printf("Failed to write port 0x%x, reg 0x%x, val 0x%x!!!\n", port, MV88E6190_PAGE_ACCESS_REG, 0);
        return -1;
    }

    return 0;
}

int switch_mv88e6190_port_autonego_complete_read(u16 port, u32 *complete, int read)
{
    u16 val = 0;

    if(port > GPORT_END_NUM || port < GPORT_START_NUM)
    {
        printf("Invalid port %u!!!\n", port);
        return -1;
    }
    /* Switch Page */
    if(marvell_phy_write(port, MV88E6190_PAGE_ACCESS_REG, MV88E6190_COPPER_STATUS_PAGE))
    {
        printf("Failed to write port 0x%x, reg 0x%x, val 0x%x!!!\n", port, MV88E6190_PAGE_ACCESS_REG, MV88E6190_COPPER_STATUS_PAGE);
        return -1;
    }
    if(marvell_phy_read(port, MV88E6190_COPPER_STATUS_REG, &val))
    {
        printf("Failed to read port 0x%x, reg 0x%x!!!\n", port, MV88E6190_COPPER_STATUS_REG);
        return -1;
    }
    *complete = (val & (1<<MV88E6190_COPPER_STATUS_COP_AN_COMP_BIT))?1:0;

    if(marvell_phy_write(port, MV88E6190_PAGE_ACCESS_REG, 0))
    {
        printf("Failed to write port 0x%x, reg 0x%x, val 0x%x!!!\n", port, MV88E6190_PAGE_ACCESS_REG, 0);
        return -1;
    }

    return 0;
}

int switch_mv88e6190_port_cop_specific_speed_read(u16 port, u32 *speed, int read)
{
    u16 val = 0;

    if(port > GPORT_END_NUM || port < GPORT_START_NUM)
    {
        printf("Invalid port %u!!!\n", port);
        return -1;
    }
    /* Switch Page */
    if(marvell_phy_write(port, MV88E6190_PAGE_ACCESS_REG, MV88E6190_COPPER_SPECIFIC_STATUS_1_PAGE))
    {
        printf("Failed to write port 0x%x, reg 0x%x, val 0x%x!!!\n", port, MV88E6190_PAGE_ACCESS_REG, MV88E6190_COPPER_SPECIFIC_STATUS_1_PAGE);
        return -1;
    }
    if(marvell_phy_read(port, MV88E6190_COPPER_SPECIFIC_STATUS_1_REG, &val))
    {
        printf("Failed to read port 0x%x, reg 0x%x!!!\n", port, MV88E6190_COPPER_SPECIFIC_STATUS_1_REG);
        return -1;
    }
    val = (val>>MV88E6190_COPPER_SPECIFIC_STATUS_1_SPEED_BIT) & MV88E6190_COPPER_SPECIFIC_STATUS_1_SPEED_MASK;
    if(val == 2)
    {
        *speed = 1000;
    }
    else if(val == 1)
    {
        *speed = 100;
    }
    else if(val == 0)
    {
        *speed = 10;
    }
    else
    {
        printf("Port 0x%x invalid speed 0x%x!!!\n", port, val);
        return -1;
    }

    if(marvell_phy_write(port, MV88E6190_PAGE_ACCESS_REG, 0))
    {
        printf("Failed to write port 0x%x, reg 0x%x, val 0x%x!!!\n", port, MV88E6190_PAGE_ACCESS_REG, 0);
        return -1;
    }

    return 0;
}

int switch_mv88e6190_port_cop_specific_duplex_read(u16 port, u32 *duplex, int read)
{
    u16 val = 0;

    if(port > GPORT_END_NUM || port < GPORT_START_NUM)
    {
        printf("Invalid port %u!!!\n", port);
        return -1;
    }
    /* Switch Page */
    if(marvell_phy_write(port, MV88E6190_PAGE_ACCESS_REG, MV88E6190_COPPER_SPECIFIC_STATUS_1_PAGE))
    {
        printf("Failed to write port 0x%x, reg 0x%x, val 0x%x!!!\n", port, MV88E6190_PAGE_ACCESS_REG, MV88E6190_COPPER_SPECIFIC_STATUS_1_PAGE);
        return -1;
    }
    if(marvell_phy_read(port, MV88E6190_COPPER_SPECIFIC_STATUS_1_REG, &val))
    {
        printf("Failed to read port 0x%x, reg 0x%x!!!\n", port, MV88E6190_COPPER_SPECIFIC_STATUS_1_REG);
        return -1;
    }
    *duplex = (val & (1<<MV88E6190_COPPER_SPECIFIC_STATUS_1_DUPLEX_BIT))?1:0;

    if(marvell_phy_write(port, MV88E6190_PAGE_ACCESS_REG, 0))
    {
        printf("Failed to write port 0x%x, reg 0x%x, val 0x%x!!!\n", port, MV88E6190_PAGE_ACCESS_REG, 0);
        return -1;
    }

    return 0;
}

int switch_mv88e6190_port_cop_specific_mdix_read(u16 port, u32 *mdix, int read) /* midx : 1; mdi : 0 */
{
    u16 val = 0;

    if(port > GPORT_END_NUM || port < GPORT_START_NUM)
    {
        printf("Invalid port %u!!!\n", port);
        return -1;
    }
    /* Switch Page */
    if(marvell_phy_write(port, MV88E6190_PAGE_ACCESS_REG, MV88E6190_COPPER_SPECIFIC_STATUS_1_PAGE))
    {
        printf("Failed to write port 0x%x, reg 0x%x, val 0x%x!!!\n", port, MV88E6190_PAGE_ACCESS_REG, MV88E6190_COPPER_SPECIFIC_STATUS_1_PAGE);
        return -1;
    }
    if(marvell_phy_read(port, MV88E6190_COPPER_SPECIFIC_STATUS_1_REG, &val))
    {
        printf("Failed to read port 0x%x, reg 0x%x!!!\n", port, MV88E6190_COPPER_SPECIFIC_STATUS_1_REG);
        return -1;
    }
    *mdix = (val & (1<<MV88E6190_COPPER_SPECIFIC_STATUS_1_MDI_BIT))?1:0;

    if(marvell_phy_write(port, MV88E6190_PAGE_ACCESS_REG, 0))
    {
        printf("Failed to write port 0x%x, reg 0x%x, val 0x%x!!!\n", port, MV88E6190_PAGE_ACCESS_REG, 0);
        return -1;
    }

    return 0;
}

int switch_mv88e6190_port_cop_specific_cop_link_rt_read(u16 port, u32 *staus, int read)
{
    u16 val = 0;

    if(port > GPORT_END_NUM || port < GPORT_START_NUM)
    {
        printf("Invalid port %u!!!\n", port);
        return -1;
    }
    /* Switch Page */
    if(marvell_phy_write(port, MV88E6190_PAGE_ACCESS_REG, MV88E6190_COPPER_SPECIFIC_STATUS_1_PAGE))
    {
        printf("Failed to write port 0x%x, reg 0x%x, val 0x%x!!!\n", port, MV88E6190_PAGE_ACCESS_REG, MV88E6190_COPPER_SPECIFIC_STATUS_1_PAGE);
        return -1;
    }
    if(marvell_phy_read(port, MV88E6190_COPPER_SPECIFIC_STATUS_1_REG, &val))
    {
        printf("Failed to read port 0x%x, reg 0x%x!!!\n", port, MV88E6190_COPPER_SPECIFIC_STATUS_1_REG);
        return -1;
    }
    *staus = (val & (1<<MV88E6190_COPPER_SPECIFIC_STATUS_1_COP_LINK_RT_BIT))?1:0;

    if(marvell_phy_write(port, MV88E6190_PAGE_ACCESS_REG, 0))
    {
        printf("Failed to write port 0x%x, reg 0x%x, val 0x%x!!!\n", port, MV88E6190_PAGE_ACCESS_REG, 0);
        return -1;
    }

    return 0;
}

int switch_mv88e6190_port_cop_specific_global_link_status_read(u16 port, u32 *staus, int read)
{
    u16 val = 0;

    if(port > GPORT_END_NUM || port < GPORT_START_NUM)
    {
        printf("Invalid port %u!!!\n", port);
        return -1;
    }
    /* Switch Page */
    if(marvell_phy_write(port, MV88E6190_PAGE_ACCESS_REG, MV88E6190_COPPER_SPECIFIC_STATUS_1_PAGE))
    {
        printf("Failed to write port 0x%x, reg 0x%x, val 0x%x!!!\n", port, MV88E6190_PAGE_ACCESS_REG, MV88E6190_COPPER_SPECIFIC_STATUS_1_PAGE);
        return -1;
    }
    if(marvell_phy_read(port, MV88E6190_COPPER_SPECIFIC_STATUS_1_REG, &val))
    {
        printf("Failed to read port 0x%x, reg 0x%x!!!\n", port, MV88E6190_COPPER_SPECIFIC_STATUS_1_REG);
        return -1;
    }
    *staus = (val & (1<<MV88E6190_COPPER_SPECIFIC_STATUS_1_GLOB_LINK_STATUS_BIT))?1:0;

    if(marvell_phy_write(port, MV88E6190_PAGE_ACCESS_REG, 0))
    {
        printf("Failed to write port 0x%x, reg 0x%x, val 0x%x!!!\n", port, MV88E6190_PAGE_ACCESS_REG, 0);
        return -1;
    }

    return 0;
}

int switch_mv88e6190_port_wait_autonego_complete(u16 port)
{
    u32 complete = 0, retryCount = 0;
    int ret = 0;

    if(port > GPORT_END_NUM || port < GPORT_START_NUM)
    {
        printf("Invalid port %u!!!\n", port);
        return -1;
    }

    do
    {
        ret = switch_mv88e6190_port_autonego_complete_read(port, &complete, 1);
        if(ret)
        {
            complete = 0;
        }
        udelay(MAX_AN_COMPLETE_WAIT_RETRY_DELAY);
        retryCount++;
    }
    while(!complete && (retryCount <= MAX_AN_COMPLETE_WAIT_RETRY));

    if(retryCount > MAX_AN_COMPLETE_WAIT_RETRY)
    {
        printf("Port 0x%x failed to complete autonego!!!\n", port);
        return -1;
    }

    return 0;
}


int switch_mv88e6190_port_cop_failure_recover(u16 port, int isAnWait, u32 *linkup, int isLog)
{
    u32 tmpStaus = 0, rtLink = 0, copLink = 0, glbLink = 0;

    if(port > GPORT_END_NUM || port < GPORT_START_NUM)
    {
        printf("\nInvalid port %u!!!\n", port);
        return -1;
    }
    if(linkup)
        *linkup = 0;

    switch_mv88e6190_port_cop_specific_cop_link_rt_read(port, &rtLink, 1);
    if(isLog)printf("\nPort %d realtime link status : %u", port, rtLink);
    switch_mv88e6190_port_copper_link_status_read(port, &copLink, 1);
    if(isLog)printf(", copper link status : %u", copLink);
    switch_mv88e6190_port_cop_specific_global_link_status_read(port, &glbLink, 1);
    if(isLog)printf(", global link status : %u", glbLink);
    switch_mv88e6190_port_autonego_complete_read(port, &tmpStaus, 1);
    if(isLog)printf(", autonego complete : %u", tmpStaus);
    switch_mv88e6190_port_copper_remote_fault_read(port, &tmpStaus, 1);
    if(isLog)printf(", remote fault : %u", tmpStaus);
    switch_mv88e6190_port_jabber_detect_read(port, &tmpStaus, 1);
    if(isLog)printf(", jabber detect : %u\n", tmpStaus);

    if((rtLink ^ copLink) || (copLink ^ glbLink) || (glbLink ^ rtLink))
    {
        printf("\nPort %d realtime link status : %u", port, rtLink);
        printf(", copper link status : %u", copLink);
        printf(", global link status : %u are not the same!!!\n", glbLink);
    }

    if(rtLink || copLink || rtLink)
    {
        if(linkup)
            *linkup = 1;
        if(isLog)printf("\nPort %d AN restart...", port);
        marvell_phy_write(port, 0,0x1340); // Restart AN
        if(isAnWait)switch_mv88e6190_port_wait_autonego_complete(port);
        if(isLog)printf("done\n");
    }

    return 0;
}

int switch_mv88e6190_port_cop_failure_recover_all(int isLog)
{
    u16 i = 0;
    u32 LinkStatus[8] = {0};

    for (i = GPORT_START_NUM; i <= GPORT_END_NUM; i ++)
    {
        switch_mv88e6190_port_cop_failure_recover(i, 0, &LinkStatus[i - 1], isLog);
    }
    for (i = GPORT_START_NUM; i <= GPORT_END_NUM; i ++)
    {
        if(LinkStatus[i - 1])
        {
            switch_mv88e6190_port_wait_autonego_complete(i);
        }
    }

    return 0;
}

void phyInit(void)
{
    unsigned short PHY = 0;
#if 0
    //Begin Errata workarounds

    //C22RegWrite(PHY, 0, 0x1140, 0);

    //Begin PHY recalibration
    for (PHY = GPORT_START_NUM; PHY <= GPORT_END_NUM; PHY ++)
    {
        marvell_phy_write(PHY, 0, 0x1140);
    }
    //osDelay(1);
    udelay(1000000); // use udelay for 1sec delay ..


    for (PHY = GPORT_START_NUM; PHY <= GPORT_END_NUM; PHY ++)
    {
        marvell_phy_write(PHY, 0x16, 0x00F8);
    }
    for (PHY = GPORT_START_NUM; PHY <= GPORT_END_NUM; PHY ++)
    {
        marvell_phy_write(PHY, 0x8, 0x0036);
    }
    for (PHY = GPORT_START_NUM; PHY <= GPORT_END_NUM; PHY ++)
    {
        marvell_phy_write(PHY, 0x16, 0x0000);
    }
    for (PHY = GPORT_START_NUM; PHY <= GPORT_END_NUM; PHY ++)
    {
        marvell_phy_write(PHY, 0x0, 0x9140);
    }
#endif
    // Adjust 10M IEEE VOD
    for (PHY = GPORT_START_NUM; PHY <= GPORT_END_NUM; PHY ++)
    {
        marvell_phy_write(PHY, 0x16, 0xfc);
        marvell_phy_write(PHY, 0x11, 0xaaff);
        marvell_phy_write(PHY, 0x16, 0x0);
        marvell_phy_write(PHY, 0x14, 0x8020); //Set 10BASE-Te Disable
    }
}

void PeridotErrata(void)
{
	//End PHY recalibration
	
	//Begin stuck port issue
	//Disable all Ports
	//Perform Workaround
	WriteReg(5,0x1A,0x01C0);
	WriteReg(4,0x1A,0xFC00);
	WriteReg(4,0x1A,0xFC20);
	WriteReg(4,0x1A,0xFC40);
	WriteReg(4,0x1A,0xFC60);
	WriteReg(4,0x1A,0xFC80);
	WriteReg(4,0x1A,0xFCA0);
	WriteReg(4,0x1A,0xFCC0);
	WriteReg(4,0x1A,0xFCE0);
	WriteReg(4,0x1A,0xFD00);
	WriteReg(4,0x1A,0xFD20);
	WriteReg(4,0x1A,0xFD40);

	/* reset */
	WriteReg(0x1b, 0x4, 0xC001);

	//End Stuck port issue
}

void mv88e6191X_Errata(void)
{
	u16 val;
	xsmi_read(SMI_I, 0xa, 4, 0xf002, &val);
	DBG_PRINTF("Before write 4.f002 = 0x%0x\n", val);
	xsmi_write(SMI_I, 0xa, 4, 0xf002, 0x804d);
	udelay(10000);
	xsmi_read(SMI_I, 0xa, 4, 0xf002, &val);
	DBG_PRINTF("After  write 4.f002 = 0x%0x\n", val);

	xsmi_write(SMI_I, 0xa, 4, 0x2004, 0x20);
	udelay(10000);
	xsmi_read(SMI_I, 0xa, 4, 0x2004, &val);
	DBG_PRINTF("After  write 4.2004 = 0x%0x\n", val);

	xsmi_write(SMI_I, 0xa, 4, 0x2000, 0x9140);
	udelay(10000);
	xsmi_read(SMI_I, 0xa, 4, 0x2000, &val);
	DBG_PRINTF("After  write 4.2000 = 0x%0x\n", val);
	
	xsmi_read(SMI_I, 0xa, 4, 0xf074, &val);
	DBG_PRINTF("Before write 4.f074 = 0x%0x\n", val);
	xsmi_write(SMI_I, 0xa, 4, 0xf074, 0x6150);
	udelay(10000);
	xsmi_read(SMI_I, 0xa, 4, 0xf074, &val);
	DBG_PRINTF("After  write 4.f074 = 0x%0x\n", val);
}

void mv88e6191X_Serdes(void)
{
	u16 val;
	printf("SERDES Initialization procedures... \n");

	xsmi_write(SMI_I, 0xa, 30, 0x8093, 0xcb5a);
	udelay(10000);
	xsmi_read(SMI_I, 0xa, 30, 0x8093, &val);
	DBG_PRINTF("After  write 30.8093 = 0x%0x\n", val);

	xsmi_write(SMI_I, 0xa, 30, 0x8171, 0x7088);
	udelay(10000);
	xsmi_read(SMI_I, 0xa, 30, 0x8171, &val);
	DBG_PRINTF("After  write 30.8171 = 0x%0x\n", val);

	xsmi_write(SMI_I, 0xa, 30, 0x80c9, 0x311a);
	udelay(10000);
	xsmi_read(SMI_I, 0xa, 30, 0x80c9, &val);
	DBG_PRINTF("After  write 30.80c9 = 0x%0x\n", val);

	xsmi_read(SMI_I, 0xa, 30, 0x80a2, &val);
	DBG_PRINTF("Before write 30.80a2 = 0x%0x\n", val);
	val |= (1<<15);
	val &= 0x8080;
	xsmi_write(SMI_I, 0xa, 30, 0x80a2, val);
	udelay(10000);
	xsmi_read(SMI_I, 0xa, 30, 0x80a2, &val);
	DBG_PRINTF("After  write 30.80a2 = 0x%0x\n", val);

	xsmi_read(SMI_I, 0xa, 30, 0x80a9, &val);
	DBG_PRINTF("Before write 30.80a9 = 0x%0x\n", val);
	val &= 0x000f;
	xsmi_write(SMI_I, 0xa, 30, 0x80a9, val);
	udelay(10000);
	xsmi_read(SMI_I, 0xa, 30, 0x80a9, &val);
	DBG_PRINTF("After  write 30.80a9 = 0x%0x\n", val);
	
	xsmi_read(SMI_I, 0xa, 30, 0x80a3, &val);
	DBG_PRINTF("Before write 30.80a3 = 0x%0x\n", val);
	val &= 0xf8ff;
	xsmi_write(SMI_I, 0xa, 30, 0x80a3, val);
	udelay(10000);
	xsmi_read(SMI_I, 0xa, 30, 0x80a3, &val);
	DBG_PRINTF("After  write 30.80a3 = 0x%0x\n", val);

	xsmi_read(SMI_I, 0xa, 4, 0xf002, &val);
	DBG_PRINTF("Before write 4.f002 = 0x%0x\n", val);
	val |= (1<<15);
	xsmi_write(SMI_I, 0xa, 4, 0xf002, val);
	udelay(10000);
	xsmi_read(SMI_I, 0xa, 4, 0xf002, &val);
	DBG_PRINTF("After  write 4.f002 = 0x%0x\n", val);

	xsmi_read(SMI_I, 0xa, 4, 0x2000, &val);
	DBG_PRINTF("Before write 4.2000 = 0x%0x\n", val);
	val |= (1<<15);
	xsmi_write(SMI_I, 0xa, 4, 0x2000, val);
	udelay(10000);
	xsmi_read(SMI_I, 0xa, 4, 0x2000, &val);
	DBG_PRINTF("After  write 4.2000 = 0x%0x\n", val);

	//Enhance 10GBase-KR for Eye diagram
	xsmi_write(SMI_I, 0xa, 30, 0x8101, 0xbe8);
	udelay(10000);
	xsmi_read(SMI_I, 0xa, 30, 0x8101, &val);
	DBG_PRINTF("After  write 30.8101 = 0x%0x\n", val);
}

int switch_mv88e6190_andelay = DEFAULT_AN_DELAY;

int resetBtnPressFlag = 0;

int switch_mv88e6190_all_port_enable_once(void)
{
    int i = 0;
    static int has_run = 0;

    if(has_run)
    {
        return 0;
    }
    else
    {
        has_run = 1;
    }
    printf("Switch 88E6191X all ports enable.\n");
    for (i=GPORT_START_NUM;i<=GPORT_END_NUM;i++)
    {
        WriteReg(i,4,0x7f); //Set port as fowarding ..
    }

	WriteReg(CPU_PORT_9,4,0x7f);  //Set port as fowarding ..
    WriteReg(CPU_PORT_10,4,0x7f); //Set port as fowarding ..
    udelay(100000);

    return 0;
}

int switch_mv88e6191X_init(void)
{
    int i = 0;
    char *anDelayChar = NULL;
	u16 val;

#if 0
    resetBtnPressFlag = resetBtnPressed();
    if(resetBtnPressFlag)
    {
        printf("Reset button pressed.\n");
    }
#endif

    printf("Marvell Switch 88E6191X init... \n");
    anDelayChar = env_get("andelay");
    if (anDelayChar)
        switch_mv88e6190_andelay = simple_strtol(anDelayChar, NULL, 10);

    DBG_PRINTF("===== Change all pot to disabled mode ..\n"); //undefined reference

	WriteReg(CPU_PORT_9, 0x1, 0x13);
    WriteReg(CPU_PORT_10, 0x1, 0x13);	//Port 10 Turn off Link in default ..

    DBG_PRINTF("=====Marvell 1G Port PHYs Reset delay...");
    udelay(1000000);
    DBG_PRINTF("done\n");

    // Power up on-chip serdes 
    // 10GBASE-R PCS Control 1
	//xsmi_write(SMI_I,CPU_PORT_10,4, 0x2000, 0xA040);	//Clear power down bit and set bit 15 for reset ,16 and 3 for speed
    //udelay(100000);

	for (i = GPORT_START_NUM; i <= GPORT_END_NUM; i ++)
    {
		WriteReg(i, 0x16, 0x8033);
    	marvell_phy_write(i, 0, 0x1140);
		udelay(10000);
    }

	//xsmi_write(SMI_I, 0, 0x1e, 0x8101, 0x0be8);	//Failure to meet SFF-8431 transmitter jitter requirements
    DBG_PRINTF("===== Change all pot to fowarding mode ..\n");
    for (i=GPORT_START_NUM;i<=GPORT_END_NUM;i++)
    {
        WriteReg(i,0x1,0x102); // Turn off integrated gigabit port EEE from MAC side
        WriteReg(i,4,0x7c);    //Set port as disabled ..
    }

	WriteReg(CPU_PORT_9, 0x1, 0x103);
	WriteReg(CPU_PORT_10, 0x1, 0x103);	//Force EEE mode off for P10 for lower core power when not linked
	WriteReg(CPU_PORT_9, 4, 0x7c);
    WriteReg(CPU_PORT_10, 4, 0x7c);		//Set port as disabled ..
	udelay(10000);
	WriteReg(CPU_PORT_9, 0x0, 0xD);
    WriteReg(CPU_PORT_10, 0x0, 0xD);	//Write 10G-BASE-R C_MODE bits for specific mode
    udelay(10000);

	phyInit();
	mv88e6191X_Errata();
	mv88e6191X_Serdes();
#if 0
	for (i = GPORT_START_NUM; i <= GPORT_END_NUM; i++)
	{
        marvell_phy_write(i, 0x9, 0x1e00); // Set Port Type as Master
		marvell_phy_write(i, 0x0, 0x9140); // Power up
	}
    udelay(300000);
#endif

//  switch_mv88e6190_port_cop_failure_recover_all(1);

    printf("done\n");

    return 0;
}

int marvell_nxp_xgmii_init(void)
{
    volatile u32 *tmpPtr = NULL, tmpVal = 0;

#ifdef CONFIG_MV88E6191X_SWITCH
#ifdef CONFIG_MV88E6191X_SWITCH_CPU_ATTACHED
    switch_mv88e6191X_init();
#else
    WriteReg(0xa, 0x0, 0xb);
    DBG_PRINTF("WriteReg(0xa, 0x0) = 0x%04x\n", ReadReg(0xa, 0x0));
#endif /* CONFIG_MV88E6191X_SWITCH_CPU_ATTACHED */
#endif /* CONFIG_MV88E6191X_SWITCH */
#ifdef CONFIG_WG1008_PX2
	tmpPtr = (volatile u32 *)0x8c07034;
    *((u32 *)tmpPtr) = 0x0000801f;
    DBG_PRINTF("%p : 0x%08x\n", tmpPtr, *((u32 *)tmpPtr));
	tmpPtr = (volatile u32 *)0x8c0703c;
    *((u32 *)tmpPtr) = 0x1;
    DBG_PRINTF("%p : 0x%08x\n", tmpPtr, *((u32 *)tmpPtr));
	tmpPtr = (volatile u32 *)0x8c07038;
    DBG_PRINTF("%p : 0x%08x\n", tmpPtr, *((u32 *)tmpPtr));
#else
    tmpPtr = (volatile u32 *)0x1af0034;
    *((u32 *)tmpPtr) = 0x00800000;
    DBG_PRINTF("%p : 0x%08x\n", tmpPtr, *((u32 *)tmpPtr));
    tmpPtr = (volatile u32 *)0x1af0038;
    tmpVal = *tmpPtr;
    *((u32 *)tmpPtr) = tmpVal & ~(1<<20);
    DBG_PRINTF("%p : 0x%08x\n", tmpPtr, *((u32 *)tmpPtr));

    tmpPtr = (volatile u32 *)0x1af2034;
    *((u32 *)tmpPtr) = 0x00800000;
    DBG_PRINTF("%p : 0x%08x\n", tmpPtr, *((u32 *)tmpPtr));
    tmpPtr = (volatile u32 *)0x1af2038;
    tmpVal = *tmpPtr;
    *((u32 *)tmpPtr) = tmpVal & ~(1<<20);
    DBG_PRINTF("%p : 0x%08x\n", tmpPtr, *((u32 *)tmpPtr));
#endif
    return 0;
}
#endif

static int ldpaa_eth_netdev_init(struct eth_device *net_dev,
				 phy_interface_t enet_if)
{
	int err;
	struct ldpaa_eth_priv *priv = (struct ldpaa_eth_priv *)net_dev->priv;

	snprintf(net_dev->name, ETH_NAME_LEN, "DPMAC%d@%s", priv->dpmac_id,
		 phy_interface_strings[enet_if]);

	net_dev->iobase = 0;
	net_dev->init = ldpaa_eth_open;
	net_dev->halt = ldpaa_eth_stop;
	net_dev->send = ldpaa_eth_tx;
	net_dev->recv = ldpaa_eth_pull_dequeue_rx;

#ifdef CONFIG_PHYLIB
	err = init_phy(net_dev);
	if (err < 0)
		return err;
#endif

	err = eth_register(net_dev);
	if (err < 0) {
		printf("eth_register() = %d\n", err);
		return err;
	}

	return 0;
}

int ldpaa_eth_init(int dpmac_id, phy_interface_t enet_if)
{
	struct eth_device		*net_dev = NULL;
	struct ldpaa_eth_priv		*priv = NULL;
	int				err = 0;

	/* Net device */
	net_dev = (struct eth_device *)malloc(sizeof(struct eth_device));
	if (!net_dev) {
		printf("eth_device malloc() failed\n");
		return -ENOMEM;
	}
	memset(net_dev, 0, sizeof(struct eth_device));

	/* alloc the ldpaa ethernet private struct */
	priv = (struct ldpaa_eth_priv *)malloc(sizeof(struct ldpaa_eth_priv));
	if (!priv) {
		printf("ldpaa_eth_priv malloc() failed\n");
		free(net_dev);
		return -ENOMEM;
	}
	memset(priv, 0, sizeof(struct ldpaa_eth_priv));

	net_dev->priv = (void *)priv;
	priv->net_dev = (struct eth_device *)net_dev;
	priv->dpmac_id = dpmac_id;
	debug("%s dpmac_id=%d\n", __func__, dpmac_id);
	DBG_PRINTF("%s dpmac_id=%d\n", __func__, dpmac_id);

	err = ldpaa_eth_netdev_init(net_dev, enet_if);
	if (err)
		goto err_netdev_init;

	debug("ldpaa ethernet: Probed interface %s\n", net_dev->name);
	DBG_PRINTF("ldpaa ethernet: Probed interface %s\n", net_dev->name);
	return 0;

err_netdev_init:
	free(priv);
	net_dev->priv = NULL;
	free(net_dev);

	return err;
}
