/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __eth_defs_h
#define __eth_defs_h

/*
 * This file is autogenerated from
 *   file:           eth.r
 *     id:           eth_regs.r,v 1.16 2005/05/20 15:41:22 perz Exp
 *     last modfied: Mon Jan  9 06:06:41 2006
 *
 *   by /n/asic/design/tools/rdesc/rdes2c eth.r
 *      id: $Id$
 * Any changes here will be lost.
 *
 * -*- buffer-read-only: t -*-
 */
/* Main access macros */
#ifndef REG_RD
#define REG_RD( scope, inst, reg ) \
  REG_READ( reg_##scope##_##reg, \
            (inst) + REG_RD_ADDR_##scope##_##reg )
#endif

#ifndef REG_WR
#define REG_WR( scope, inst, reg, val ) \
  REG_WRITE( reg_##scope##_##reg, \
             (inst) + REG_WR_ADDR_##scope##_##reg, (val) )
#endif

#ifndef REG_RD_VECT
#define REG_RD_VECT( scope, inst, reg, index ) \
  REG_READ( reg_##scope##_##reg, \
            (inst) + REG_RD_ADDR_##scope##_##reg + \
	    (index) * STRIDE_##scope##_##reg )
#endif

#ifndef REG_WR_VECT
#define REG_WR_VECT( scope, inst, reg, index, val ) \
  REG_WRITE( reg_##scope##_##reg, \
             (inst) + REG_WR_ADDR_##scope##_##reg + \
	     (index) * STRIDE_##scope##_##reg, (val) )
#endif

#ifndef REG_RD_INT
#define REG_RD_INT( scope, inst, reg ) \
  REG_READ( int, (inst) + REG_RD_ADDR_##scope##_##reg )
#endif

#ifndef REG_WR_INT
#define REG_WR_INT( scope, inst, reg, val ) \
  REG_WRITE( int, (inst) + REG_WR_ADDR_##scope##_##reg, (val) )
#endif

#ifndef REG_RD_INT_VECT
#define REG_RD_INT_VECT( scope, inst, reg, index ) \
  REG_READ( int, (inst) + REG_RD_ADDR_##scope##_##reg + \
	    (index) * STRIDE_##scope##_##reg )
#endif

#ifndef REG_WR_INT_VECT
#define REG_WR_INT_VECT( scope, inst, reg, index, val ) \
  REG_WRITE( int, (inst) + REG_WR_ADDR_##scope##_##reg + \
	     (index) * STRIDE_##scope##_##reg, (val) )
#endif

#ifndef REG_TYPE_CONV
#define REG_TYPE_CONV( type, orgtype, val ) \
  ( { union { orgtype o; type n; } r; r.o = val; r.n; } )
#endif

#ifndef reg_page_size
#define reg_page_size 8192
#endif

#ifndef REG_ADDR
#define REG_ADDR( scope, inst, reg ) \
  ( (inst) + REG_RD_ADDR_##scope##_##reg )
#endif

#ifndef REG_ADDR_VECT
#define REG_ADDR_VECT( scope, inst, reg, index ) \
  ( (inst) + REG_RD_ADDR_##scope##_##reg + \
    (index) * STRIDE_##scope##_##reg )
#endif

/* C-code for register scope eth */

/* Register rw_ma0_lo, scope eth, type rw */
typedef struct {
  unsigned int addr : 32;
} reg_eth_rw_ma0_lo;
#define REG_RD_ADDR_eth_rw_ma0_lo 0
#define REG_WR_ADDR_eth_rw_ma0_lo 0

/* Register rw_ma0_hi, scope eth, type rw */
typedef struct {
  unsigned int addr : 16;
  unsigned int dummy1 : 16;
} reg_eth_rw_ma0_hi;
#define REG_RD_ADDR_eth_rw_ma0_hi 4
#define REG_WR_ADDR_eth_rw_ma0_hi 4

/* Register rw_ma1_lo, scope eth, type rw */
typedef struct {
  unsigned int addr : 32;
} reg_eth_rw_ma1_lo;
#define REG_RD_ADDR_eth_rw_ma1_lo 8
#define REG_WR_ADDR_eth_rw_ma1_lo 8

/* Register rw_ma1_hi, scope eth, type rw */
typedef struct {
  unsigned int addr : 16;
  unsigned int dummy1 : 16;
} reg_eth_rw_ma1_hi;
#define REG_RD_ADDR_eth_rw_ma1_hi 12
#define REG_WR_ADDR_eth_rw_ma1_hi 12

/* Register rw_ga_lo, scope eth, type rw */
typedef struct {
  unsigned int tbl : 32;
} reg_eth_rw_ga_lo;
#define REG_RD_ADDR_eth_rw_ga_lo 16
#define REG_WR_ADDR_eth_rw_ga_lo 16

/* Register rw_ga_hi, scope eth, type rw */
typedef struct {
  unsigned int tbl : 32;
} reg_eth_rw_ga_hi;
#define REG_RD_ADDR_eth_rw_ga_hi 20
#define REG_WR_ADDR_eth_rw_ga_hi 20

/* Register rw_gen_ctrl, scope eth, type rw */
typedef struct {
  unsigned int en         : 1;
  unsigned int phy        : 2;
  unsigned int protocol   : 1;
  unsigned int loopback   : 1;
  unsigned int flow_ctrl  : 1;
  unsigned int gtxclk_out : 1;
  unsigned int phyrst_n   : 1;
  unsigned int dummy1     : 24;
} reg_eth_rw_gen_ctrl;
#define REG_RD_ADDR_eth_rw_gen_ctrl 24
#define REG_WR_ADDR_eth_rw_gen_ctrl 24

/* Register rw_rec_ctrl, scope eth, type rw */
typedef struct {
  unsigned int ma0        : 1;
  unsigned int ma1        : 1;
  unsigned int individual : 1;
  unsigned int broadcast  : 1;
  unsigned int undersize  : 1;
  unsigned int oversize   : 1;
  unsigned int bad_crc    : 1;
  unsigned int duplex     : 1;
  unsigned int max_size   : 16;
  unsigned int dummy1     : 8;
} reg_eth_rw_rec_ctrl;
#define REG_RD_ADDR_eth_rw_rec_ctrl 28
#define REG_WR_ADDR_eth_rw_rec_ctrl 28

/* Register rw_tr_ctrl, scope eth, type rw */
typedef struct {
  unsigned int crc         : 1;
  unsigned int pad         : 1;
  unsigned int retry       : 1;
  unsigned int ignore_col  : 1;
  unsigned int cancel      : 1;
  unsigned int hsh_delay   : 1;
  unsigned int ignore_crs  : 1;
  unsigned int carrier_ext : 1;
  unsigned int dummy1      : 24;
} reg_eth_rw_tr_ctrl;
#define REG_RD_ADDR_eth_rw_tr_ctrl 32
#define REG_WR_ADDR_eth_rw_tr_ctrl 32

/* Register rw_clr_err, scope eth, type rw */
typedef struct {
  unsigned int clr : 1;
  unsigned int dummy1 : 31;
} reg_eth_rw_clr_err;
#define REG_RD_ADDR_eth_rw_clr_err 36
#define REG_WR_ADDR_eth_rw_clr_err 36

/* Register rw_mgm_ctrl, scope eth, type rw */
typedef struct {
  unsigned int mdio : 1;
  unsigned int mdoe : 1;
  unsigned int mdc  : 1;
  unsigned int dummy1 : 29;
} reg_eth_rw_mgm_ctrl;
#define REG_RD_ADDR_eth_rw_mgm_ctrl 40
#define REG_WR_ADDR_eth_rw_mgm_ctrl 40

/* Register r_stat, scope eth, type r */
typedef struct {
  unsigned int mdio    : 1;
  unsigned int exc_col : 1;
  unsigned int urun    : 1;
  unsigned int clk_125 : 1;
  unsigned int dummy1  : 28;
} reg_eth_r_stat;
#define REG_RD_ADDR_eth_r_stat 44

/* Register rs_rec_cnt, scope eth, type rs */
typedef struct {
  unsigned int crc_err    : 8;
  unsigned int align_err  : 8;
  unsigned int oversize   : 8;
  unsigned int congestion : 8;
} reg_eth_rs_rec_cnt;
#define REG_RD_ADDR_eth_rs_rec_cnt 48

/* Register r_rec_cnt, scope eth, type r */
typedef struct {
  unsigned int crc_err    : 8;
  unsigned int align_err  : 8;
  unsigned int oversize   : 8;
  unsigned int congestion : 8;
} reg_eth_r_rec_cnt;
#define REG_RD_ADDR_eth_r_rec_cnt 52

/* Register rs_tr_cnt, scope eth, type rs */
typedef struct {
  unsigned int single_col : 8;
  unsigned int mult_col   : 8;
  unsigned int late_col   : 8;
  unsigned int deferred   : 8;
} reg_eth_rs_tr_cnt;
#define REG_RD_ADDR_eth_rs_tr_cnt 56

/* Register r_tr_cnt, scope eth, type r */
typedef struct {
  unsigned int single_col : 8;
  unsigned int mult_col   : 8;
  unsigned int late_col   : 8;
  unsigned int deferred   : 8;
} reg_eth_r_tr_cnt;
#define REG_RD_ADDR_eth_r_tr_cnt 60

/* Register rs_phy_cnt, scope eth, type rs */
typedef struct {
  unsigned int carrier_loss : 8;
  unsigned int sqe_err      : 8;
  unsigned int dummy1       : 16;
} reg_eth_rs_phy_cnt;
#define REG_RD_ADDR_eth_rs_phy_cnt 64

/* Register r_phy_cnt, scope eth, type r */
typedef struct {
  unsigned int carrier_loss : 8;
  unsigned int sqe_err      : 8;
  unsigned int dummy1       : 16;
} reg_eth_r_phy_cnt;
#define REG_RD_ADDR_eth_r_phy_cnt 68

/* Register rw_test_ctrl, scope eth, type rw */
typedef struct {
  unsigned int snmp_inc : 1;
  unsigned int snmp     : 1;
  unsigned int backoff  : 1;
  unsigned int dummy1   : 29;
} reg_eth_rw_test_ctrl;
#define REG_RD_ADDR_eth_rw_test_ctrl 72
#define REG_WR_ADDR_eth_rw_test_ctrl 72

/* Register rw_intr_mask, scope eth, type rw */
typedef struct {
  unsigned int crc          : 1;
  unsigned int align        : 1;
  unsigned int oversize     : 1;
  unsigned int congestion   : 1;
  unsigned int single_col   : 1;
  unsigned int mult_col     : 1;
  unsigned int late_col     : 1;
  unsigned int deferred     : 1;
  unsigned int carrier_loss : 1;
  unsigned int sqe_test_err : 1;
  unsigned int orun         : 1;
  unsigned int urun         : 1;
  unsigned int exc_col      : 1;
  unsigned int mdio         : 1;
  unsigned int dummy1       : 18;
} reg_eth_rw_intr_mask;
#define REG_RD_ADDR_eth_rw_intr_mask 76
#define REG_WR_ADDR_eth_rw_intr_mask 76

/* Register rw_ack_intr, scope eth, type rw */
typedef struct {
  unsigned int crc          : 1;
  unsigned int align        : 1;
  unsigned int oversize     : 1;
  unsigned int congestion   : 1;
  unsigned int single_col   : 1;
  unsigned int mult_col     : 1;
  unsigned int late_col     : 1;
  unsigned int deferred     : 1;
  unsigned int carrier_loss : 1;
  unsigned int sqe_test_err : 1;
  unsigned int orun         : 1;
  unsigned int urun         : 1;
  unsigned int exc_col      : 1;
  unsigned int mdio         : 1;
  unsigned int dummy1       : 18;
} reg_eth_rw_ack_intr;
#define REG_RD_ADDR_eth_rw_ack_intr 80
#define REG_WR_ADDR_eth_rw_ack_intr 80

/* Register r_intr, scope eth, type r */
typedef struct {
  unsigned int crc          : 1;
  unsigned int align        : 1;
  unsigned int oversize     : 1;
  unsigned int congestion   : 1;
  unsigned int single_col   : 1;
  unsigned int mult_col     : 1;
  unsigned int late_col     : 1;
  unsigned int deferred     : 1;
  unsigned int carrier_loss : 1;
  unsigned int sqe_test_err : 1;
  unsigned int orun         : 1;
  unsigned int urun         : 1;
  unsigned int exc_col      : 1;
  unsigned int mdio         : 1;
  unsigned int dummy1       : 18;
} reg_eth_r_intr;
#define REG_RD_ADDR_eth_r_intr 84

/* Register r_masked_intr, scope eth, type r */
typedef struct {
  unsigned int crc          : 1;
  unsigned int align        : 1;
  unsigned int oversize     : 1;
  unsigned int congestion   : 1;
  unsigned int single_col   : 1;
  unsigned int mult_col     : 1;
  unsigned int late_col     : 1;
  unsigned int deferred     : 1;
  unsigned int carrier_loss : 1;
  unsigned int sqe_test_err : 1;
  unsigned int orun         : 1;
  unsigned int urun         : 1;
  unsigned int exc_col      : 1;
  unsigned int mdio         : 1;
  unsigned int dummy1       : 18;
} reg_eth_r_masked_intr;
#define REG_RD_ADDR_eth_r_masked_intr 88


/* Constants */
enum {
  regk_eth_discard                         = 0x00000000,
  regk_eth_ether                           = 0x00000000,
  regk_eth_full                            = 0x00000001,
  regk_eth_gmii                            = 0x00000003,
  regk_eth_gtxclk                          = 0x00000001,
  regk_eth_half                            = 0x00000000,
  regk_eth_hsh                             = 0x00000001,
  regk_eth_mii                             = 0x00000001,
  regk_eth_mii_arec                        = 0x00000002,
  regk_eth_mii_clk                         = 0x00000000,
  regk_eth_no                              = 0x00000000,
  regk_eth_phyrst                          = 0x00000000,
  regk_eth_rec                             = 0x00000001,
  regk_eth_rw_ga_hi_default                = 0x00000000,
  regk_eth_rw_ga_lo_default                = 0x00000000,
  regk_eth_rw_gen_ctrl_default             = 0x00000000,
  regk_eth_rw_intr_mask_default            = 0x00000000,
  regk_eth_rw_ma0_hi_default               = 0x00000000,
  regk_eth_rw_ma0_lo_default               = 0x00000000,
  regk_eth_rw_ma1_hi_default               = 0x00000000,
  regk_eth_rw_ma1_lo_default               = 0x00000000,
  regk_eth_rw_mgm_ctrl_default             = 0x00000000,
  regk_eth_rw_test_ctrl_default            = 0x00000000,
  regk_eth_size1518                        = 0x000005ee,
  regk_eth_size1522                        = 0x000005f2,
  regk_eth_yes                             = 0x00000001
};
#endif /* __eth_defs_h */
