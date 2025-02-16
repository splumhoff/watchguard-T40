/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __iop_sap_in_defs_asm_h
#define __iop_sap_in_defs_asm_h

/*
 * This file is autogenerated from
 *   file:           ../../inst/io_proc/rtl/iop_sap_in.r
 *     id:           <not found>
 *     last modfied: Mon Apr 11 16:08:45 2005
 *
 *   by /n/asic/design/tools/rdesc/src/rdes2c -asm --outfile asm/iop_sap_in_defs_asm.h ../../inst/io_proc/rtl/iop_sap_in.r
 *      id: $Id$
 * Any changes here will be lost.
 *
 * -*- buffer-read-only: t -*-
 */

#ifndef REG_FIELD
#define REG_FIELD( scope, reg, field, value ) \
  REG_FIELD_X_( value, reg_##scope##_##reg##___##field##___lsb )
#define REG_FIELD_X_( value, shift ) ((value) << shift)
#endif

#ifndef REG_STATE
#define REG_STATE( scope, reg, field, symbolic_value ) \
  REG_STATE_X_( regk_##scope##_##symbolic_value, reg_##scope##_##reg##___##field##___lsb )
#define REG_STATE_X_( k, shift ) (k << shift)
#endif

#ifndef REG_MASK
#define REG_MASK( scope, reg, field ) \
  REG_MASK_X_( reg_##scope##_##reg##___##field##___width, reg_##scope##_##reg##___##field##___lsb )
#define REG_MASK_X_( width, lsb ) (((1 << width)-1) << lsb)
#endif

#ifndef REG_LSB
#define REG_LSB( scope, reg, field ) reg_##scope##_##reg##___##field##___lsb
#endif

#ifndef REG_BIT
#define REG_BIT( scope, reg, field ) reg_##scope##_##reg##___##field##___bit
#endif

#ifndef REG_ADDR
#define REG_ADDR( scope, inst, reg ) REG_ADDR_X_(inst, reg_##scope##_##reg##_offset)
#define REG_ADDR_X_( inst, offs ) ((inst) + offs)
#endif

#ifndef REG_ADDR_VECT
#define REG_ADDR_VECT( scope, inst, reg, index ) \
         REG_ADDR_VECT_X_(inst, reg_##scope##_##reg##_offset, index, \
			 STRIDE_##scope##_##reg )
#define REG_ADDR_VECT_X_( inst, offs, index, stride ) \
                          ((inst) + offs + (index) * stride)
#endif

/* Register rw_bus0_sync, scope iop_sap_in, type rw */
#define reg_iop_sap_in_rw_bus0_sync___byte0_sel___lsb 0
#define reg_iop_sap_in_rw_bus0_sync___byte0_sel___width 2
#define reg_iop_sap_in_rw_bus0_sync___byte0_ext_src___lsb 2
#define reg_iop_sap_in_rw_bus0_sync___byte0_ext_src___width 3
#define reg_iop_sap_in_rw_bus0_sync___byte0_edge___lsb 5
#define reg_iop_sap_in_rw_bus0_sync___byte0_edge___width 2
#define reg_iop_sap_in_rw_bus0_sync___byte0_delay___lsb 7
#define reg_iop_sap_in_rw_bus0_sync___byte0_delay___width 1
#define reg_iop_sap_in_rw_bus0_sync___byte0_delay___bit 7
#define reg_iop_sap_in_rw_bus0_sync___byte1_sel___lsb 8
#define reg_iop_sap_in_rw_bus0_sync___byte1_sel___width 2
#define reg_iop_sap_in_rw_bus0_sync___byte1_ext_src___lsb 10
#define reg_iop_sap_in_rw_bus0_sync___byte1_ext_src___width 3
#define reg_iop_sap_in_rw_bus0_sync___byte1_edge___lsb 13
#define reg_iop_sap_in_rw_bus0_sync___byte1_edge___width 2
#define reg_iop_sap_in_rw_bus0_sync___byte1_delay___lsb 15
#define reg_iop_sap_in_rw_bus0_sync___byte1_delay___width 1
#define reg_iop_sap_in_rw_bus0_sync___byte1_delay___bit 15
#define reg_iop_sap_in_rw_bus0_sync___byte2_sel___lsb 16
#define reg_iop_sap_in_rw_bus0_sync___byte2_sel___width 2
#define reg_iop_sap_in_rw_bus0_sync___byte2_ext_src___lsb 18
#define reg_iop_sap_in_rw_bus0_sync___byte2_ext_src___width 3
#define reg_iop_sap_in_rw_bus0_sync___byte2_edge___lsb 21
#define reg_iop_sap_in_rw_bus0_sync___byte2_edge___width 2
#define reg_iop_sap_in_rw_bus0_sync___byte2_delay___lsb 23
#define reg_iop_sap_in_rw_bus0_sync___byte2_delay___width 1
#define reg_iop_sap_in_rw_bus0_sync___byte2_delay___bit 23
#define reg_iop_sap_in_rw_bus0_sync___byte3_sel___lsb 24
#define reg_iop_sap_in_rw_bus0_sync___byte3_sel___width 2
#define reg_iop_sap_in_rw_bus0_sync___byte3_ext_src___lsb 26
#define reg_iop_sap_in_rw_bus0_sync___byte3_ext_src___width 3
#define reg_iop_sap_in_rw_bus0_sync___byte3_edge___lsb 29
#define reg_iop_sap_in_rw_bus0_sync___byte3_edge___width 2
#define reg_iop_sap_in_rw_bus0_sync___byte3_delay___lsb 31
#define reg_iop_sap_in_rw_bus0_sync___byte3_delay___width 1
#define reg_iop_sap_in_rw_bus0_sync___byte3_delay___bit 31
#define reg_iop_sap_in_rw_bus0_sync_offset 0

/* Register rw_bus1_sync, scope iop_sap_in, type rw */
#define reg_iop_sap_in_rw_bus1_sync___byte0_sel___lsb 0
#define reg_iop_sap_in_rw_bus1_sync___byte0_sel___width 2
#define reg_iop_sap_in_rw_bus1_sync___byte0_ext_src___lsb 2
#define reg_iop_sap_in_rw_bus1_sync___byte0_ext_src___width 3
#define reg_iop_sap_in_rw_bus1_sync___byte0_edge___lsb 5
#define reg_iop_sap_in_rw_bus1_sync___byte0_edge___width 2
#define reg_iop_sap_in_rw_bus1_sync___byte0_delay___lsb 7
#define reg_iop_sap_in_rw_bus1_sync___byte0_delay___width 1
#define reg_iop_sap_in_rw_bus1_sync___byte0_delay___bit 7
#define reg_iop_sap_in_rw_bus1_sync___byte1_sel___lsb 8
#define reg_iop_sap_in_rw_bus1_sync___byte1_sel___width 2
#define reg_iop_sap_in_rw_bus1_sync___byte1_ext_src___lsb 10
#define reg_iop_sap_in_rw_bus1_sync___byte1_ext_src___width 3
#define reg_iop_sap_in_rw_bus1_sync___byte1_edge___lsb 13
#define reg_iop_sap_in_rw_bus1_sync___byte1_edge___width 2
#define reg_iop_sap_in_rw_bus1_sync___byte1_delay___lsb 15
#define reg_iop_sap_in_rw_bus1_sync___byte1_delay___width 1
#define reg_iop_sap_in_rw_bus1_sync___byte1_delay___bit 15
#define reg_iop_sap_in_rw_bus1_sync___byte2_sel___lsb 16
#define reg_iop_sap_in_rw_bus1_sync___byte2_sel___width 2
#define reg_iop_sap_in_rw_bus1_sync___byte2_ext_src___lsb 18
#define reg_iop_sap_in_rw_bus1_sync___byte2_ext_src___width 3
#define reg_iop_sap_in_rw_bus1_sync___byte2_edge___lsb 21
#define reg_iop_sap_in_rw_bus1_sync___byte2_edge___width 2
#define reg_iop_sap_in_rw_bus1_sync___byte2_delay___lsb 23
#define reg_iop_sap_in_rw_bus1_sync___byte2_delay___width 1
#define reg_iop_sap_in_rw_bus1_sync___byte2_delay___bit 23
#define reg_iop_sap_in_rw_bus1_sync___byte3_sel___lsb 24
#define reg_iop_sap_in_rw_bus1_sync___byte3_sel___width 2
#define reg_iop_sap_in_rw_bus1_sync___byte3_ext_src___lsb 26
#define reg_iop_sap_in_rw_bus1_sync___byte3_ext_src___width 3
#define reg_iop_sap_in_rw_bus1_sync___byte3_edge___lsb 29
#define reg_iop_sap_in_rw_bus1_sync___byte3_edge___width 2
#define reg_iop_sap_in_rw_bus1_sync___byte3_delay___lsb 31
#define reg_iop_sap_in_rw_bus1_sync___byte3_delay___width 1
#define reg_iop_sap_in_rw_bus1_sync___byte3_delay___bit 31
#define reg_iop_sap_in_rw_bus1_sync_offset 4

#define STRIDE_iop_sap_in_rw_gio 4
/* Register rw_gio, scope iop_sap_in, type rw */
#define reg_iop_sap_in_rw_gio___sync_sel___lsb 0
#define reg_iop_sap_in_rw_gio___sync_sel___width 2
#define reg_iop_sap_in_rw_gio___sync_ext_src___lsb 2
#define reg_iop_sap_in_rw_gio___sync_ext_src___width 3
#define reg_iop_sap_in_rw_gio___sync_edge___lsb 5
#define reg_iop_sap_in_rw_gio___sync_edge___width 2
#define reg_iop_sap_in_rw_gio___delay___lsb 7
#define reg_iop_sap_in_rw_gio___delay___width 1
#define reg_iop_sap_in_rw_gio___delay___bit 7
#define reg_iop_sap_in_rw_gio___logic___lsb 8
#define reg_iop_sap_in_rw_gio___logic___width 2
#define reg_iop_sap_in_rw_gio_offset 8


/* Constants */
#define regk_iop_sap_in_and                       0x00000002
#define regk_iop_sap_in_ext_clk200                0x00000003
#define regk_iop_sap_in_gio1                      0x00000000
#define regk_iop_sap_in_gio13                     0x00000005
#define regk_iop_sap_in_gio18                     0x00000003
#define regk_iop_sap_in_gio19                     0x00000004
#define regk_iop_sap_in_gio21                     0x00000006
#define regk_iop_sap_in_gio23                     0x00000005
#define regk_iop_sap_in_gio29                     0x00000007
#define regk_iop_sap_in_gio5                      0x00000004
#define regk_iop_sap_in_gio6                      0x00000001
#define regk_iop_sap_in_gio7                      0x00000002
#define regk_iop_sap_in_inv                       0x00000001
#define regk_iop_sap_in_neg                       0x00000002
#define regk_iop_sap_in_no                        0x00000000
#define regk_iop_sap_in_no_del_ext_clk200         0x00000001
#define regk_iop_sap_in_none                      0x00000000
#define regk_iop_sap_in_or                        0x00000003
#define regk_iop_sap_in_pos                       0x00000001
#define regk_iop_sap_in_pos_neg                   0x00000003
#define regk_iop_sap_in_rw_bus0_sync_default      0x02020202
#define regk_iop_sap_in_rw_bus1_sync_default      0x02020202
#define regk_iop_sap_in_rw_gio_default            0x00000002
#define regk_iop_sap_in_rw_gio_size               0x00000020
#define regk_iop_sap_in_timer_grp0_tmr3           0x00000006
#define regk_iop_sap_in_timer_grp1_tmr3           0x00000004
#define regk_iop_sap_in_timer_grp2_tmr3           0x00000005
#define regk_iop_sap_in_timer_grp3_tmr3           0x00000007
#define regk_iop_sap_in_tmr_clk200                0x00000000
#define regk_iop_sap_in_two_clk200                0x00000002
#define regk_iop_sap_in_yes                       0x00000001
#endif /* __iop_sap_in_defs_asm_h */
