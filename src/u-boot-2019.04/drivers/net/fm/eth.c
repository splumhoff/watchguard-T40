// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2009-2012 Freescale Semiconductor, Inc.
 *	Dave Liu <daveliu@freescale.com>
 */
#include <common.h>
#include <asm/io.h>
#include <malloc.h>
#include <net.h>
#include <hwconfig.h>
#include <fm_eth.h>
#include <fsl_mdio.h>
#include <miiphy.h>
#include <phy.h>
#include <fsl_dtsec.h>
#include <fsl_tgec.h>
#include <fsl_memac.h>

#include "fm.h"

static struct eth_device *devlist[NUM_FM_PORTS];
static int num_controllers;

#if defined(CONFIG_MII) || defined(CONFIG_CMD_MII) && !defined(BITBANGMII)

#define TBIANA_SETTINGS (TBIANA_ASYMMETRIC_PAUSE | TBIANA_SYMMETRIC_PAUSE | \
			 TBIANA_FULL_DUPLEX)

#define TBIANA_SGMII_ACK 0x4001

#define TBICR_SETTINGS (TBICR_ANEG_ENABLE | TBICR_RESTART_ANEG | \
			TBICR_FULL_DUPLEX | TBICR_SPEED1_SET)

/* Configure the TBI for SGMII operation */
static void dtsec_configure_serdes(struct fm_eth *priv)
{
#ifdef CONFIG_SYS_FMAN_V3
	u32 value;
	struct mii_dev bus;
	bus.priv = priv->mac->phyregs;
	bool sgmii_2500 = (priv->enet_if ==
			PHY_INTERFACE_MODE_SGMII_2500) ? true : false;
	int i = 0;

qsgmii_loop:
	/* SGMII IF mode + AN enable only for 1G SGMII, not for 2.5G */
	if (sgmii_2500)
		value = PHY_SGMII_CR_PHY_RESET |
			PHY_SGMII_IF_SPEED_GIGABIT |
			PHY_SGMII_IF_MODE_SGMII;
	else
		value = PHY_SGMII_IF_MODE_SGMII | PHY_SGMII_IF_MODE_AN;

	memac_mdio_write(&bus, i, MDIO_DEVAD_NONE, 0x14, value);

	/* Dev ability according to SGMII specification */
	value = PHY_SGMII_DEV_ABILITY_SGMII;
	memac_mdio_write(&bus, i, MDIO_DEVAD_NONE, 0x4, value);

	if (sgmii_2500) {
		/* Adjust link timer for 2.5G SGMII,
		 * 1.6 ms in units of 3.2 ns:
		 * 1.6ms / 3.2ns = 5 * 10^5 = 0x7a120.
		 */
		memac_mdio_write(&bus, i, MDIO_DEVAD_NONE, 0x13, 0x0007);
		memac_mdio_write(&bus, i, MDIO_DEVAD_NONE, 0x12, 0xa120);
	} else {
		/* Adjust link timer for SGMII,
		 * 1.6 ms in units of 8 ns:
		 * 1.6ms / 8ns = 2 * 10^5 = 0x30d40.
		 */
		memac_mdio_write(&bus, i, MDIO_DEVAD_NONE, 0x13, 0x0003);
		memac_mdio_write(&bus, i, MDIO_DEVAD_NONE, 0x12, 0x0d40);
	}

	/* Restart AN */
	value = PHY_SGMII_CR_DEF_VAL | PHY_SGMII_CR_RESET_AN;
	memac_mdio_write(&bus, i, MDIO_DEVAD_NONE, 0, value);

	if ((priv->enet_if == PHY_INTERFACE_MODE_QSGMII) && (i < 3)) {
		i++;
		goto qsgmii_loop;
	}
#else
	struct dtsec *regs = priv->mac->base;
	struct tsec_mii_mng *phyregs = priv->mac->phyregs;

	/*
	 * Access TBI PHY registers at given TSEC register offset as
	 * opposed to the register offset used for external PHY accesses
	 */
	tsec_local_mdio_write(phyregs, in_be32(&regs->tbipa), 0, TBI_TBICON,
			TBICON_CLK_SELECT);
	tsec_local_mdio_write(phyregs, in_be32(&regs->tbipa), 0, TBI_ANA,
			TBIANA_SGMII_ACK);
	tsec_local_mdio_write(phyregs, in_be32(&regs->tbipa), 0,
			TBI_CR, TBICR_SETTINGS);
#endif
}

static void dtsec_init_phy(struct eth_device *dev)
{
	struct fm_eth *fm_eth = dev->priv;
#ifndef CONFIG_SYS_FMAN_V3
	struct dtsec *regs = (struct dtsec *)CONFIG_SYS_FSL_FM1_DTSEC1_ADDR;

	/* Assign a Physical address to the TBI */
	out_be32(&regs->tbipa, CONFIG_SYS_TBIPA_VALUE);
#endif

	if (fm_eth->enet_if == PHY_INTERFACE_MODE_SGMII ||
	    fm_eth->enet_if == PHY_INTERFACE_MODE_QSGMII ||
	    fm_eth->enet_if == PHY_INTERFACE_MODE_SGMII_2500)
		dtsec_configure_serdes(fm_eth);
}

#ifdef CONFIG_PHYLIB
static int tgec_is_fibre(struct eth_device *dev)
{
	struct fm_eth *fm = dev->priv;
	char phyopt[20];

	sprintf(phyopt, "fsl_fm%d_xaui_phy", fm->fm_index + 1);

	return hwconfig_arg_cmp(phyopt, "xfi");
}
#endif
#endif

static u16 muram_readw(u16 *addr)
{
	ulong base = (ulong)addr & ~0x3UL;
	u32 val32 = in_be32((void *)base);
	int byte_pos;
	u16 ret;

	byte_pos = (ulong)addr & 0x3UL;
	if (byte_pos)
		ret = (u16)(val32 & 0x0000ffff);
	else
		ret = (u16)((val32 & 0xffff0000) >> 16);

	return ret;
}

static void muram_writew(u16 *addr, u16 val)
{
	ulong base = (ulong)addr & ~0x3UL;
	u32 org32 = in_be32((void *)base);
	u32 val32;
	int byte_pos;

	byte_pos = (ulong)addr & 0x3UL;
	if (byte_pos)
		val32 = (org32 & 0xffff0000) | val;
	else
		val32 = (org32 & 0x0000ffff) | ((u32)val << 16);

	out_be32((void *)base, val32);
}

static void bmi_rx_port_disable(struct fm_bmi_rx_port *rx_port)
{
	int timeout = 1000000;

	clrbits_be32(&rx_port->fmbm_rcfg, FMBM_RCFG_EN);

	/* wait until the rx port is not busy */
	while ((in_be32(&rx_port->fmbm_rst) & FMBM_RST_BSY) && timeout--)
		;
}

static void bmi_rx_port_init(struct fm_bmi_rx_port *rx_port)
{
	/* set BMI to independent mode, Rx port disable */
	out_be32(&rx_port->fmbm_rcfg, FMBM_RCFG_IM);
	/* clear FOF in IM case */
	out_be32(&rx_port->fmbm_rim, 0);
	/* Rx frame next engine -RISC */
	out_be32(&rx_port->fmbm_rfne, NIA_ENG_RISC | NIA_RISC_AC_IM_RX);
	/* Rx command attribute - no order, MR[3] = 1 */
	clrbits_be32(&rx_port->fmbm_rfca, FMBM_RFCA_ORDER | FMBM_RFCA_MR_MASK);
	setbits_be32(&rx_port->fmbm_rfca, FMBM_RFCA_MR(4));
	/* enable Rx statistic counters */
	out_be32(&rx_port->fmbm_rstc, FMBM_RSTC_EN);
	/* disable Rx performance counters */
	out_be32(&rx_port->fmbm_rpc, 0);
}

static void bmi_tx_port_disable(struct fm_bmi_tx_port *tx_port)
{
	int timeout = 1000000;

	clrbits_be32(&tx_port->fmbm_tcfg, FMBM_TCFG_EN);

	/* wait until the tx port is not busy */
	while ((in_be32(&tx_port->fmbm_tst) & FMBM_TST_BSY) && timeout--)
		;
}

static void bmi_tx_port_init(struct fm_bmi_tx_port *tx_port)
{
	/* set BMI to independent mode, Tx port disable */
	out_be32(&tx_port->fmbm_tcfg, FMBM_TCFG_IM);
	/* Tx frame next engine -RISC */
	out_be32(&tx_port->fmbm_tfne, NIA_ENG_RISC | NIA_RISC_AC_IM_TX);
	out_be32(&tx_port->fmbm_tfene, NIA_ENG_RISC | NIA_RISC_AC_IM_TX);
	/* Tx command attribute - no order, MR[3] = 1 */
	clrbits_be32(&tx_port->fmbm_tfca, FMBM_TFCA_ORDER | FMBM_TFCA_MR_MASK);
	setbits_be32(&tx_port->fmbm_tfca, FMBM_TFCA_MR(4));
	/* enable Tx statistic counters */
	out_be32(&tx_port->fmbm_tstc, FMBM_TSTC_EN);
	/* disable Tx performance counters */
	out_be32(&tx_port->fmbm_tpc, 0);
}

static int fm_eth_rx_port_parameter_init(struct fm_eth *fm_eth)
{
	struct fm_port_global_pram *pram;
	u32 pram_page_offset;
	void *rx_bd_ring_base;
	void *rx_buf_pool;
	u32 bd_ring_base_lo, bd_ring_base_hi;
	u32 buf_lo, buf_hi;
	struct fm_port_bd *rxbd;
	struct fm_port_qd *rxqd;
	struct fm_bmi_rx_port *bmi_rx_port = fm_eth->rx_port;
	int i;

	/* alloc global parameter ram at MURAM */
	pram = (struct fm_port_global_pram *)fm_muram_alloc(fm_eth->fm_index,
		FM_PRAM_SIZE, FM_PRAM_ALIGN);
	if (!pram) {
		printf("%s: No muram for Rx global parameter\n", __func__);
		return -ENOMEM;
	}

	fm_eth->rx_pram = pram;

	/* parameter page offset to MURAM */
	pram_page_offset = (void *)pram - fm_muram_base(fm_eth->fm_index);

	/* enable global mode- snooping data buffers and BDs */
	out_be32(&pram->mode, PRAM_MODE_GLOBAL);

	/* init the Rx queue descriptor pionter */
	out_be32(&pram->rxqd_ptr, pram_page_offset + 0x20);

	/* set the max receive buffer length, power of 2 */
	muram_writew(&pram->mrblr, MAX_RXBUF_LOG2);

	/* alloc Rx buffer descriptors from main memory */
	rx_bd_ring_base = malloc(sizeof(struct fm_port_bd)
			* RX_BD_RING_SIZE);
	if (!rx_bd_ring_base)
		return -ENOMEM;

	memset(rx_bd_ring_base, 0, sizeof(struct fm_port_bd)
			* RX_BD_RING_SIZE);

	/* alloc Rx buffer from main memory */
	rx_buf_pool = malloc(MAX_RXBUF_LEN * RX_BD_RING_SIZE);
	if (!rx_buf_pool)
		return -ENOMEM;

	memset(rx_buf_pool, 0, MAX_RXBUF_LEN * RX_BD_RING_SIZE);
	debug("%s: rx_buf_pool = %p\n", __func__, rx_buf_pool);

	/* save them to fm_eth */
	fm_eth->rx_bd_ring = rx_bd_ring_base;
	fm_eth->cur_rxbd = rx_bd_ring_base;
	fm_eth->rx_buf = rx_buf_pool;

	/* init Rx BDs ring */
	rxbd = (struct fm_port_bd *)rx_bd_ring_base;
	for (i = 0; i < RX_BD_RING_SIZE; i++) {
		muram_writew(&rxbd->status, RxBD_EMPTY);
		muram_writew(&rxbd->len, 0);
		buf_hi = upper_32_bits(virt_to_phys(rx_buf_pool +
					i * MAX_RXBUF_LEN));
		buf_lo = lower_32_bits(virt_to_phys(rx_buf_pool +
					i * MAX_RXBUF_LEN));
		muram_writew(&rxbd->buf_ptr_hi, (u16)buf_hi);
		out_be32(&rxbd->buf_ptr_lo, buf_lo);
		rxbd++;
	}

	/* set the Rx queue descriptor */
	rxqd = &pram->rxqd;
	muram_writew(&rxqd->gen, 0);
	bd_ring_base_hi = upper_32_bits(virt_to_phys(rx_bd_ring_base));
	bd_ring_base_lo = lower_32_bits(virt_to_phys(rx_bd_ring_base));
	muram_writew(&rxqd->bd_ring_base_hi, (u16)bd_ring_base_hi);
	out_be32(&rxqd->bd_ring_base_lo, bd_ring_base_lo);
	muram_writew(&rxqd->bd_ring_size, sizeof(struct fm_port_bd)
			* RX_BD_RING_SIZE);
	muram_writew(&rxqd->offset_in, 0);
	muram_writew(&rxqd->offset_out, 0);

	/* set IM parameter ram pointer to Rx Frame Queue ID */
	out_be32(&bmi_rx_port->fmbm_rfqid, pram_page_offset);

	return 0;
}

static int fm_eth_tx_port_parameter_init(struct fm_eth *fm_eth)
{
	struct fm_port_global_pram *pram;
	u32 pram_page_offset;
	void *tx_bd_ring_base;
	u32 bd_ring_base_lo, bd_ring_base_hi;
	struct fm_port_bd *txbd;
	struct fm_port_qd *txqd;
	struct fm_bmi_tx_port *bmi_tx_port = fm_eth->tx_port;
	int i;

	/* alloc global parameter ram at MURAM */
	pram = (struct fm_port_global_pram *)fm_muram_alloc(fm_eth->fm_index,
		FM_PRAM_SIZE, FM_PRAM_ALIGN);
	if (!pram) {
		printf("%s: No muram for Tx global parameter\n", __func__);
		return -ENOMEM;
	}
	fm_eth->tx_pram = pram;

	/* parameter page offset to MURAM */
	pram_page_offset = (void *)pram - fm_muram_base(fm_eth->fm_index);

	/* enable global mode- snooping data buffers and BDs */
	out_be32(&pram->mode, PRAM_MODE_GLOBAL);

	/* init the Tx queue descriptor pionter */
	out_be32(&pram->txqd_ptr, pram_page_offset + 0x40);

	/* alloc Tx buffer descriptors from main memory */
	tx_bd_ring_base = malloc(sizeof(struct fm_port_bd)
			* TX_BD_RING_SIZE);
	if (!tx_bd_ring_base)
		return -ENOMEM;

	memset(tx_bd_ring_base, 0, sizeof(struct fm_port_bd)
			* TX_BD_RING_SIZE);
	/* save it to fm_eth */
	fm_eth->tx_bd_ring = tx_bd_ring_base;
	fm_eth->cur_txbd = tx_bd_ring_base;

	/* init Tx BDs ring */
	txbd = (struct fm_port_bd *)tx_bd_ring_base;
	for (i = 0; i < TX_BD_RING_SIZE; i++) {
		muram_writew(&txbd->status, TxBD_LAST);
		muram_writew(&txbd->len, 0);
		muram_writew(&txbd->buf_ptr_hi, 0);
		out_be32(&txbd->buf_ptr_lo, 0);
		txbd++;
	}

	/* set the Tx queue decriptor */
	txqd = &pram->txqd;
	bd_ring_base_hi = upper_32_bits(virt_to_phys(tx_bd_ring_base));
	bd_ring_base_lo = lower_32_bits(virt_to_phys(tx_bd_ring_base));
	muram_writew(&txqd->bd_ring_base_hi, (u16)bd_ring_base_hi);
	out_be32(&txqd->bd_ring_base_lo, bd_ring_base_lo);
	muram_writew(&txqd->bd_ring_size, sizeof(struct fm_port_bd)
			* TX_BD_RING_SIZE);
	muram_writew(&txqd->offset_in, 0);
	muram_writew(&txqd->offset_out, 0);

	/* set IM parameter ram pointer to Tx Confirmation Frame Queue ID */
	out_be32(&bmi_tx_port->fmbm_tcfqid, pram_page_offset);

	return 0;
}

static int fm_eth_init(struct fm_eth *fm_eth)
{
	int ret;

	ret = fm_eth_rx_port_parameter_init(fm_eth);
	if (ret)
		return ret;

	ret = fm_eth_tx_port_parameter_init(fm_eth);
	if (ret)
		return ret;

	return 0;
}

static int fm_eth_startup(struct fm_eth *fm_eth)
{
	struct fsl_enet_mac *mac;
	int ret;

	mac = fm_eth->mac;

	/* Rx/TxBDs, Rx/TxQDs, Rx buff and parameter ram init */
	ret = fm_eth_init(fm_eth);
	if (ret)
		return ret;
	/* setup the MAC controller */
	mac->init_mac(mac);

	/* For some reason we need to set SPEED_100 */
	if (((fm_eth->enet_if == PHY_INTERFACE_MODE_SGMII) ||
	     (fm_eth->enet_if == PHY_INTERFACE_MODE_SGMII_2500) ||
	     (fm_eth->enet_if == PHY_INTERFACE_MODE_QSGMII)) &&
	      mac->set_if_mode)
		mac->set_if_mode(mac, fm_eth->enet_if, SPEED_100);

	/* init bmi rx port, IM mode and disable */
	bmi_rx_port_init(fm_eth->rx_port);
	/* init bmi tx port, IM mode and disable */
	bmi_tx_port_init(fm_eth->tx_port);

	return 0;
}

static void fmc_tx_port_graceful_stop_enable(struct fm_eth *fm_eth)
{
	struct fm_port_global_pram *pram;

	pram = fm_eth->tx_pram;
	/* graceful stop transmission of frames */
	setbits_be32(&pram->mode, PRAM_MODE_GRACEFUL_STOP);
	sync();
}

static void fmc_tx_port_graceful_stop_disable(struct fm_eth *fm_eth)
{
	struct fm_port_global_pram *pram;

	pram = fm_eth->tx_pram;
	/* re-enable transmission of frames */
	clrbits_be32(&pram->mode, PRAM_MODE_GRACEFUL_STOP);
	sync();
}

static int fm_eth_open(struct eth_device *dev, bd_t *bd)
{
	struct fm_eth *fm_eth;
	struct fsl_enet_mac *mac;
#ifdef CONFIG_PHYLIB
	int ret;
#endif

	fm_eth = (struct fm_eth *)dev->priv;
	mac = fm_eth->mac;

	/* setup the MAC address */
	if (dev->enetaddr[0] & 0x01) {
		printf("%s: MacAddress is multcast address\n",	__func__);
		return 1;
	}
	mac->set_mac_addr(mac, dev->enetaddr);

	/* enable bmi Rx port */
	setbits_be32(&fm_eth->rx_port->fmbm_rcfg, FMBM_RCFG_EN);
	/* enable MAC rx/tx port */
	mac->enable_mac(mac);
	/* enable bmi Tx port */
	setbits_be32(&fm_eth->tx_port->fmbm_tcfg, FMBM_TCFG_EN);
	/* re-enable transmission of frame */
	fmc_tx_port_graceful_stop_disable(fm_eth);

#ifdef CONFIG_PHYLIB
	if (fm_eth->phydev) {
		ret = phy_startup(fm_eth->phydev);
		if (ret) {
			printf("%s: Could not initialize\n",
			       fm_eth->phydev->dev->name);
			return ret;
		}
	} else {
		return 0;
	}
#else
	fm_eth->phydev->speed = SPEED_1000;
	fm_eth->phydev->link = 1;
	fm_eth->phydev->duplex = DUPLEX_FULL;
#endif
#ifdef CONFIG_WG1008_PX1
    fm_eth->phydev->speed = 2500;
    fm_eth->phydev->link = 1;
    fm_eth->phydev->duplex = DUPLEX_FULL;
#endif
	/* set the MAC-PHY mode */
	mac->set_if_mode(mac, fm_eth->enet_if, fm_eth->phydev->speed);

	if (!fm_eth->phydev->link)
		printf("%s: No link.\n", fm_eth->phydev->dev->name);

	return fm_eth->phydev->link ? 0 : -1;
}

static void fm_eth_halt(struct eth_device *dev)
{
	struct fm_eth *fm_eth;
	struct fsl_enet_mac *mac;

	fm_eth = (struct fm_eth *)dev->priv;
	mac = fm_eth->mac;

	/* graceful stop the transmission of frames */
	fmc_tx_port_graceful_stop_enable(fm_eth);
	/* disable bmi Tx port */
	bmi_tx_port_disable(fm_eth->tx_port);
	/* disable MAC rx/tx port */
	mac->disable_mac(mac);
	/* disable bmi Rx port */
	bmi_rx_port_disable(fm_eth->rx_port);

#ifdef CONFIG_PHYLIB
	if (fm_eth->phydev)
		phy_shutdown(fm_eth->phydev);
#endif
}

static int fm_eth_send(struct eth_device *dev, void *buf, int len)
{
	struct fm_eth *fm_eth;
	struct fm_port_global_pram *pram;
	struct fm_port_bd *txbd, *txbd_base;
	u16 offset_in;
	int i;

	fm_eth = (struct fm_eth *)dev->priv;
	pram = fm_eth->tx_pram;
	txbd = fm_eth->cur_txbd;

	/* find one empty TxBD */
	for (i = 0; muram_readw(&txbd->status) & TxBD_READY; i++) {
		udelay(100);
		if (i > 0x1000) {
			printf("%s: Tx buffer not ready, txbd->status = 0x%x\n",
			       dev->name, muram_readw(&txbd->status));
			return 0;
		}
	}
	/* setup TxBD */
	muram_writew(&txbd->buf_ptr_hi, (u16)upper_32_bits(virt_to_phys(buf)));
	out_be32(&txbd->buf_ptr_lo, lower_32_bits(virt_to_phys(buf)));
	muram_writew(&txbd->len, len);
	sync();
	muram_writew(&txbd->status, TxBD_READY | TxBD_LAST);
	sync();

	/* update TxQD, let RISC to send the packet */
	offset_in = muram_readw(&pram->txqd.offset_in);
	offset_in += sizeof(struct fm_port_bd);
	if (offset_in >= muram_readw(&pram->txqd.bd_ring_size))
		offset_in = 0;
	muram_writew(&pram->txqd.offset_in, offset_in);
	sync();

	/* wait for buffer to be transmitted */
	for (i = 0; muram_readw(&txbd->status) & TxBD_READY; i++) {
		udelay(100);
		if (i > 0x10000) {
			printf("%s: Tx error, txbd->status = 0x%x\n",
			       dev->name, muram_readw(&txbd->status));
			return 0;
		}
	}

	/* advance the TxBD */
	txbd++;
	txbd_base = (struct fm_port_bd *)fm_eth->tx_bd_ring;
	if (txbd >= (txbd_base + TX_BD_RING_SIZE))
		txbd = txbd_base;
	/* update current txbd */
	fm_eth->cur_txbd = (void *)txbd;

	return 1;
}

static int fm_eth_recv(struct eth_device *dev)
{
	struct fm_eth *fm_eth;
	struct fm_port_global_pram *pram;
	struct fm_port_bd *rxbd, *rxbd_base;
	u16 status, len;
	u32 buf_lo, buf_hi;
	u8 *data;
	u16 offset_out;
	int ret = 1;

	fm_eth = (struct fm_eth *)dev->priv;
	pram = fm_eth->rx_pram;
	rxbd = fm_eth->cur_rxbd;
	status = muram_readw(&rxbd->status);

	while (!(status & RxBD_EMPTY)) {
		if (!(status & RxBD_ERROR)) {
			buf_hi = muram_readw(&rxbd->buf_ptr_hi);
			buf_lo = in_be32(&rxbd->buf_ptr_lo);
			data = (u8 *)((ulong)(buf_hi << 16) << 16 | buf_lo);
			len = muram_readw(&rxbd->len);
			net_process_received_packet(data, len);
		} else {
			printf("%s: Rx error\n", dev->name);
			ret = 0;
		}

		/* clear the RxBDs */
		muram_writew(&rxbd->status, RxBD_EMPTY);
		muram_writew(&rxbd->len, 0);
		sync();

		/* advance RxBD */
		rxbd++;
		rxbd_base = (struct fm_port_bd *)fm_eth->rx_bd_ring;
		if (rxbd >= (rxbd_base + RX_BD_RING_SIZE))
			rxbd = rxbd_base;
		/* read next status */
		status = muram_readw(&rxbd->status);

		/* update RxQD */
		offset_out = muram_readw(&pram->rxqd.offset_out);
		offset_out += sizeof(struct fm_port_bd);
		if (offset_out >= muram_readw(&pram->rxqd.bd_ring_size))
			offset_out = 0;
		muram_writew(&pram->rxqd.offset_out, offset_out);
		sync();
	}
	fm_eth->cur_rxbd = (void *)rxbd;

	return ret;
}

static int fm_eth_init_mac(struct fm_eth *fm_eth, struct ccsr_fman *reg)
{
	struct fsl_enet_mac *mac;
	int num;
	void *base, *phyregs = NULL;

	num = fm_eth->num;

#ifdef CONFIG_SYS_FMAN_V3
#ifndef CONFIG_FSL_FM_10GEC_REGULAR_NOTATION
	if (fm_eth->type == FM_ETH_10G_E) {
		/* 10GEC1/10GEC2 use mEMAC9/mEMAC10 on T2080/T4240.
		 * 10GEC3/10GEC4 use mEMAC1/mEMAC2 on T2080.
		 * 10GEC1 uses mEMAC1 on T1024.
		 * so it needs to change the num.
		 */
		if (fm_eth->num >= 2)
			num -= 2;
		else
			num += 8;
	}
#endif
	base = &reg->memac[num].fm_memac;
	phyregs = &reg->memac[num].fm_memac_mdio;
#else
	/* Get the mac registers base address */
	if (fm_eth->type == FM_ETH_1G_E) {
		base = &reg->mac_1g[num].fm_dtesc;
		phyregs = &reg->mac_1g[num].fm_mdio.miimcfg;
	} else {
		base = &reg->mac_10g[num].fm_10gec;
		phyregs = &reg->mac_10g[num].fm_10gec_mdio;
	}
#endif

	/* alloc mac controller */
	mac = malloc(sizeof(struct fsl_enet_mac));
	if (!mac)
		return -ENOMEM;
	memset(mac, 0, sizeof(struct fsl_enet_mac));

	/* save the mac to fm_eth struct */
	fm_eth->mac = mac;

#ifdef CONFIG_SYS_FMAN_V3
	init_memac(mac, base, phyregs, MAX_RXBUF_LEN);
#else
	if (fm_eth->type == FM_ETH_1G_E)
		init_dtsec(mac, base, phyregs, MAX_RXBUF_LEN);
	else
		init_tgec(mac, base, phyregs, MAX_RXBUF_LEN);
#endif

	return 0;
}

static int init_phy(struct eth_device *dev)
{
	struct fm_eth *fm_eth = dev->priv;
#ifdef CONFIG_PHYLIB
	struct phy_device *phydev = NULL;
	u32 supported;
#endif

	if (fm_eth->type == FM_ETH_1G_E)
		dtsec_init_phy(dev);

#ifdef CONFIG_PHYLIB
	if (fm_eth->bus) {
		phydev = phy_connect(fm_eth->bus, fm_eth->phyaddr, dev,
					fm_eth->enet_if);
		if (!phydev) {
			printf("Failed to connect\n");
			return -1;
		}
	} else {
		return 0;
	}

	if (fm_eth->type == FM_ETH_1G_E) {
		supported = (SUPPORTED_10baseT_Half |
				SUPPORTED_10baseT_Full |
				SUPPORTED_100baseT_Half |
				SUPPORTED_100baseT_Full |
				SUPPORTED_1000baseT_Full);
	} else {
		supported = SUPPORTED_10000baseT_Full;

		if (tgec_is_fibre(dev))
			phydev->port = PORT_FIBRE;
	}

	phydev->supported &= supported;
	phydev->advertising = phydev->supported;

	fm_eth->phydev = phydev;

	phy_config(phydev);
#endif

	return 0;
}

#ifdef CONFIG_MV88E6190_SWITCH
//#define CONFIG_ERRATA_FIX
#if 1
#define GPORT_START_NUM 1
#define GPORT_END_NUM 8
#else
#define GPORT_START_NUM 0
#define GPORT_END_NUM -1
#endif
#define CPU_PORT_START_NUM 9
#define CPU_PORT_END_NUM 10
#define SW_DBG_PRINTF(format, args...) printf("[%s:%d] "format, __FUNCTION__, __LINE__, ##args)

int switch_mv88e6190_port_led(u32 mode)
{
    int i = 0;
    u16 regVal = 0x0;

    switch(mode)
    {
        case PORT_LED_MODE_OFF:
            regVal = 0x80ee;
            break;
        case PORT_LED_MODE_GREEN:
            regVal = 0x80fe;
            break;
        case PORT_LED_MODE_AMBER:
            regVal = 0x80ef;
            break;
        case PORT_LED_MODE_NORMAL:
            regVal = 0x8012;
            break;
        case PORT_LED_MODE_ALLON:
            regVal = 0x80ff;
            break;
        default:
            return -1;
    }
    for (i=1; i<=8; i++)
    {
        WriteReg(i,0x16,regVal); // Set the LED mode
    }

    return 0;
}

#define MV88E6190_PAGE_ACCESS_REG            0x16

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
	// Adjust 10M IEEE VOD
    for (PHY = GPORT_START_NUM; PHY <= GPORT_END_NUM; PHY ++)
    {
        marvell_phy_write(PHY, 0x16, 0xfc);
        marvell_phy_write(PHY, 0x11, 0xaaff);
        marvell_phy_write(PHY, 0x16, 0x0);
		marvell_phy_write(PHY, 0x14, 0x0020);
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
    printf("Switch 88E6190 all ports enable.\n");
    for (i=GPORT_START_NUM;i<=GPORT_END_NUM;i++)
    {
        WriteReg(i,4,0x7f); //Set port as fowarding ..
    }

    for (i=CPU_PORT_START_NUM;i<=CPU_PORT_END_NUM;i++)
    {
        WriteReg(i,4,0x7f); //Set port as fowarding ..
    }
    udelay(100000);

    return 0;
}

unsigned int detect_SYSB_keypress(void)
{
	int status = 0;
	int i;
	for(i = 0; i < 3; i++) {

	        if (resetBtnPressed()) {
			printf("booting into SYSB ... \n");
			status = 1;
			break;
		}
		udelay(1000);
	}

	return status;
}



int switch_mv88e6190_init(void)
{
    int i = 0;
    char *anDelayChar = NULL;

    // resetBtnPressFlag = resetBtnPressed();
    resetBtnPressFlag = detect_SYSB_keypress();
    if(resetBtnPressFlag)
    {
        printf("Reset button pressed.\n");
    }

    printf("Marvell Switch 88E6190 init...");
    anDelayChar = env_get("andelay");
    if (anDelayChar)
        switch_mv88e6190_andelay = simple_strtol(anDelayChar, NULL, 10);

    DBG_PRINTF("===== Change all pot to disabled mode ..\n");

    WriteReg(0,1,0x13);     // Port 0
    WriteReg(CPU_PORT_START_NUM,1,0x13);    // Port 9/10
    WriteReg(CPU_PORT_END_NUM,1,0x13);  // Turn off Link in default ..

    //PeridotErrata(); //Move to end of init procedure 

    // Set C_MODE for port 9 and 10
    WriteReg(CPU_PORT_START_NUM,0,0xb);
    WriteReg(CPU_PORT_END_NUM,0,0xb);

    DBG_PRINTF("=====Marvell 1G Port PHYs Reset delay...");
    udelay(1000000);
    DBG_PRINTF("done\n");

    // Power up on-chip serdes 
    // Port 9, Lane 9 
    xsmi_write(SMI_I,CPU_PORT_START_NUM,4,0x2000,0xA040); //Clear power down bit and set bit 15 for reset ,16 and 3 for speed
    // Port 10 Lane 0xA 
    xsmi_write(SMI_I,CPU_PORT_END_NUM,4,0x2000,0xA040);   //Clear power down bit and set bit 15 for reset ,16 and 3 for speed
    udelay(100000);

    PeridotErrata();

    phyInit();

    DBG_PRINTF("===== Change all pot to fowarding mode ..\n");
    for (i=GPORT_START_NUM;i<=GPORT_END_NUM;i++)
    {
        WriteReg(i,0x1,0x102); // Turn off integrated gigabit port EEE from MAC side
        WriteReg(i,4,0x7c); //Set port as disabled .. 
    }

    for (i=CPU_PORT_START_NUM;i<=CPU_PORT_END_NUM;i++)
    {
        WriteReg(i,0x1,0x103); //Force EEE mode off for P9 and P10 for lower core power when not linked
        WriteReg(i,4,0x7c); //Set port as disabled .. 
    }
    udelay(100000);

#if 0
	for (i = GPORT_START_NUM; i <= GPORT_END_NUM; i++)
	{
        marvell_phy_write(i, 0x9, 0x1e00); // Set Port Type as Master
		marvell_phy_write(i, 0x0, 0x9140); // Power up
	}
    udelay(300000);
#endif

//    switch_mv88e6190_port_cop_failure_recover_all(1);

    printf("done\n");

    return 0;
}
#endif

int marvell_nxp_sgmii_init(void)
{
    volatile u32 *tmpPtr = NULL, tmpVal = 0;

#ifdef CONFIG_MV88E6190_SWITCH
#ifdef CONFIG_MV88E6190_SWITCH_CPU_ATTACHED
    switch_mv88e6190_init();
#else
    WriteReg(0x9, 0x0, 0xb);
    DBG_PRINTF("WriteReg(0x9, 0x0) = 0x%04x\n", ReadReg(0x9, 0x0));
    WriteReg(0xa, 0x0, 0xb);
    DBG_PRINTF("WriteReg(0xa, 0x0) = 0x%04x\n", ReadReg(0xa, 0x0));
#endif /* CONFIG_MV88E6190_SWITCH_CPU_ATTACHED */
#endif /* CONFIG_MV88E6190_SWITCH */
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

    return 0;
}

int fm_eth_initialize(struct ccsr_fman *reg, struct fm_eth_info *info)
{
	struct eth_device *dev;
	struct fm_eth *fm_eth;
	int i, num = info->num;
	int ret;

	/* alloc eth device */
	dev = (struct eth_device *)malloc(sizeof(struct eth_device));
	if (!dev)
		return -ENOMEM;
	memset(dev, 0, sizeof(struct eth_device));

	/* alloc the FMan ethernet private struct */
	fm_eth = (struct fm_eth *)malloc(sizeof(struct fm_eth));
	if (!fm_eth)
		return -ENOMEM;
	memset(fm_eth, 0, sizeof(struct fm_eth));

	/* save off some things we need from the info struct */
	fm_eth->fm_index = info->index - 1; /* keep as 0 based for muram */
	fm_eth->num = num;
	fm_eth->type = info->type;

	fm_eth->rx_port = (void *)&reg->port[info->rx_port_id - 1].fm_bmi;
	fm_eth->tx_port = (void *)&reg->port[info->tx_port_id - 1].fm_bmi;

	/* set the ethernet max receive length */
	fm_eth->max_rx_len = MAX_RXBUF_LEN;

	/* init global mac structure */
	ret = fm_eth_init_mac(fm_eth, reg);
	if (ret)
		return ret;

	/* keep same as the manual, we call FMAN1, FMAN2, DTSEC1, DTSEC2, etc */
	if (fm_eth->type == FM_ETH_1G_E)
		sprintf(dev->name, "FM%d@DTSEC%d", info->index, num + 1);
	else
		sprintf(dev->name, "FM%d@TGEC%d", info->index, num + 1);

	devlist[num_controllers++] = dev;
	dev->iobase = 0;
	dev->priv = (void *)fm_eth;
	dev->init = fm_eth_open;
	dev->halt = fm_eth_halt;
	dev->send = fm_eth_send;
	dev->recv = fm_eth_recv;
	fm_eth->dev = dev;
	fm_eth->bus = info->bus;
	fm_eth->phyaddr = info->phy_addr;
	fm_eth->enet_if = info->enet_if;

	/* startup the FM im */
	ret = fm_eth_startup(fm_eth);
	if (ret)
		return ret;

	init_phy(dev);

	/* clear the ethernet address */
	for (i = 0; i < 6; i++)
		dev->enetaddr[i] = 0;
	eth_register(dev);

	return 0;
}
