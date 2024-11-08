/*********************************************************
 * Copyright (C) 1998-2018 VMware, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation version 2.1 and no later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the Lesser GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA.
 *
 *********************************************************/

/*********************************************************
 * The contents of this file are subject to the terms of the Common
 * Development and Distribution License (the "License") version 1.0
 * and no later version.  You may not use this file except in
 * compliance with the License.
 *
 * You can obtain a copy of the License at
 *         http://www.opensource.org/licenses/cddl1.php
 *
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 *********************************************************/

/*
 * backdoor_def.h --
 *
 * This contains backdoor defines that can be included from
 * an assembly language file.
 */

#ifndef _BACKDOOR_DEF_H_
#define _BACKDOOR_DEF_H_

#define INCLUDE_ALLOW_MODULE
#define INCLUDE_ALLOW_USERLEVEL
#define INCLUDE_ALLOW_VMCORE
#define INCLUDE_ALLOW_VMKERNEL
#include "includeCheck.h"

#if defined __cplusplus
extern "C" {
#endif

/*
 * If you want to add a new low-level backdoor call for a guest userland
 * application, please consider using the GuestRpc mechanism instead. --hpreg
 */

#define BDOOR_MAGIC 0x564D5868

/* Low-bandwidth backdoor port. --hpreg */

#define BDOOR_PORT 0x5658

#define   BDOOR_CMD_GETMHZ                    1
/*
 * BDOOR_CMD_APMFUNCTION is used by:
 *
 * o The FrobOS code, which instead should either program the virtual chipset
 *   (like the new BIOS code does, matthias offered to implement that), or not
 *   use any VM-specific code (which requires that we correctly implement
 *   "power off on CLI HLT" for SMP VMs, boris offered to implement that)
 *
 * o The old BIOS code, which will soon be jettisoned
 *
 *  --hpreg
 */
#define   BDOOR_CMD_APMFUNCTION               2 /* CPL0 only. */
#define   BDOOR_CMD_GETDISKGEO                3
#define   BDOOR_CMD_GETPTRLOCATION            4
#define   BDOOR_CMD_SETPTRLOCATION            5
#define   BDOOR_CMD_GETSELLENGTH              6
#define   BDOOR_CMD_GETNEXTPIECE              7
#define   BDOOR_CMD_SETSELLENGTH              8
#define   BDOOR_CMD_SETNEXTPIECE              9
#define   BDOOR_CMD_GETVERSION               10
#define   BDOOR_CMD_GETDEVICELISTELEMENT     11
#define   BDOOR_CMD_TOGGLEDEVICE             12
#define   BDOOR_CMD_GETGUIOPTIONS            13
#define   BDOOR_CMD_SETGUIOPTIONS            14
#define   BDOOR_CMD_GETSCREENSIZE            15
#define   BDOOR_CMD_MONITOR_CONTROL          16 /* Disabled by default. */
#define   BDOOR_CMD_GETHWVERSION             17
#define   BDOOR_CMD_OSNOTFOUND               18 /* CPL0 only. */
#define   BDOOR_CMD_GETUUID                  19
#define   BDOOR_CMD_GETMEMSIZE               20
//#define BDOOR_CMD_HOSTCOPY                 21 /* Not in use. Was devel only. */
//#define BDOOR_CMD_SERVICE_VM               22 /* Not in use. Never shipped. */
#define   BDOOR_CMD_GETTIME                  23 /* Deprecated -> GETTIMEFULL. */
#define   BDOOR_CMD_STOPCATCHUP              24
#define   BDOOR_CMD_PUTCHR                   25 /* Disabled by default. */
#define   BDOOR_CMD_ENABLE_MSG               26 /* Devel only. */
//#define BDOOR_CMD_GOTO_TCL                 27 /* Not in use. Was devel only */
#define   BDOOR_CMD_INITPCIOPROM             28 /* CPL 0 only. */
//#define BDOOR_CMD_INT13                    29 /* Not in use. */
#define   BDOOR_CMD_MESSAGE                  30
#define   BDOOR_CMD_SIDT                     31
#define   BDOOR_CMD_SGDT                     32
#define   BDOOR_CMD_SLDT_STR                 33
#define   BDOOR_CMD_ISACPIDISABLED           34
//#define BDOOR_CMD_TOE                      35 /* Not in use. */
#define   BDOOR_CMD_ISMOUSEABSOLUTE          36
#define   BDOOR_CMD_PATCH_SMBIOS_STRUCTS     37 /* CPL 0 only. */
#define   BDOOR_CMD_MAPMEM                   38 /* Devel only */
#define   BDOOR_CMD_ABSPOINTER_DATA          39
#define   BDOOR_CMD_ABSPOINTER_STATUS        40
#define   BDOOR_CMD_ABSPOINTER_COMMAND       41
//#define BDOOR_CMD_TIMER_SPONGE             42 /* Not in use. */
#define   BDOOR_CMD_PATCH_ACPI_TABLES        43 /* CPL 0 only. */
//#define BDOOR_CMD_DEVEL_FAKEHARDWARE       44 /* Not in use. */
#define   BDOOR_CMD_GETHZ                    45
#define   BDOOR_CMD_GETTIMEFULL              46
//#define BDOOR_CMD_STATELOGGER              47 /* Not in use. */
#define   BDOOR_CMD_CHECKFORCEBIOSSETUP      48 /* CPL 0 only. */
#define   BDOOR_CMD_LAZYTIMEREMULATION       49 /* CPL 0 only. */
#define   BDOOR_CMD_BIOSBBS                  50 /* CPL 0 only. */
//#define BDOOR_CMD_VASSERT                  51 /* Not in use. */
#define   BDOOR_CMD_ISGOSDARWIN              52
#define   BDOOR_CMD_DEBUGEVENT               53
#define   BDOOR_CMD_OSNOTMACOSXSERVER        54 /* CPL 0 only. */
#define   BDOOR_CMD_GETTIMEFULL_WITH_LAG     55
#define   BDOOR_CMD_ACPI_HOTPLUG_DEVICE      56 /* Devel only. */
#define   BDOOR_CMD_ACPI_HOTPLUG_MEMORY      57 /* Devel only. */
#define   BDOOR_CMD_ACPI_HOTPLUG_CBRET       58 /* Devel only. */
//#define BDOOR_CMD_GET_HOST_VIDEO_MODES     59 /* Not in use. */
#define   BDOOR_CMD_ACPI_HOTPLUG_CPU         60 /* Devel only. */
//#define BDOOR_CMD_USB_HOTPLUG_MOUSE        61 /* Not in use. Never shipped. */
#define   BDOOR_CMD_XPMODE                   62 /* CPL 0 only. */
#define   BDOOR_CMD_NESTING_CONTROL          63
#define   BDOOR_CMD_FIRMWARE_INIT            64 /* CPL 0 only. */
#define   BDOOR_CMD_FIRMWARE_ACPI_SERVICES   65 /* CPL 0 only. */
#  define BDOOR_CMD_FAS_GET_TABLE_SIZE        0
#  define BDOOR_CMD_FAS_GET_TABLE_DATA        1
#  define BDOOR_CMD_FAS_GET_PLATFORM_NAME     2
#  define BDOOR_CMD_FAS_GET_PCIE_OSC_MASK     3
#  define BDOOR_CMD_FAS_GET_APIC_ROUTING      4
#  define BDOOR_CMD_FAS_GET_TABLE_SKIP        5
#  define BDOOR_CMD_FAS_GET_SLEEP_ENABLES     6
#  define BDOOR_CMD_FAS_GET_HARD_RESET_ENABLE 7
#  define BDOOR_CMD_FAS_GET_MOUSE_HID         8
#  define BDOOR_CMD_FAS_GET_SMBIOS_VERSION    9
#  define BDOOR_CMD_FAS_GET_64BIT_PCI_HOLE_SIZE 10
//#define BDOOR_CMD_FAS_GET_NVDIMM_FMT_CODE  11 /* Not in use. Never shipped. */
#  define BDOOR_CMD_FAS_SRP_ENABLED          12
#  define BDOOR_CMD_FAS_EXIT_BOOT_SERVICES   13
#define   BDOOR_CMD_SENDPSHAREHINTS          66 /* Not in use. Deprecated. */
#define   BDOOR_CMD_ENABLE_USB_MOUSE         67
#define   BDOOR_CMD_GET_VCPU_INFO            68
#  define BDOOR_CMD_VCPU_SLC64                0
#  define BDOOR_CMD_VCPU_SYNC_VTSCS           1
#  define BDOOR_CMD_VCPU_HV_REPLAY_OK         2
#  define BDOOR_CMD_VCPU_LEGACY_X2APIC_OK     3
#  define BDOOR_CMD_VCPU_MMIO_HONORS_PAT      4
#  define BDOOR_CMD_VCPU_RESERVED            31
#define   BDOOR_CMD_EFI_SERIALCON_CONFIG     69 /* CPL 0 only. */
#define   BDOOR_CMD_BUG328986                70 /* CPL 0 only. */
#define   BDOOR_CMD_FIRMWARE_ERROR           71 /* CPL 0 only. */
#  define BDOOR_CMD_FE_INSUFFICIENT_MEM       0
#  define BDOOR_CMD_FE_EXCEPTION              1
#  define BDOOR_CMD_FE_SGX                    2
#  define BDOOR_CMD_FE_PCI_MMIO               3
#define   BDOOR_CMD_VMK_INFO                 72
#define   BDOOR_CMD_EFI_BOOT_CONFIG          73 /* CPL 0 only. */
#  define BDOOR_CMD_EBC_LEGACYBOOT_ENABLED        0
#  define BDOOR_CMD_EBC_GET_ORDER                 1
#  define BDOOR_CMD_EBC_SHELL_ACTIVE              2
#  define BDOOR_CMD_EBC_GET_NETWORK_BOOT_PROTOCOL 3
#  define BDOOR_CMD_EBC_QUICKBOOT_ENABLED         4
#  define BDOOR_CMD_EBC_GET_PXE_ARCH              5
#define   BDOOR_CMD_GET_HW_MODEL             74 /* CPL 0 only. */
#define   BDOOR_CMD_GET_SVGA_CAPABILITIES    75 /* CPL 0 only. */
#define   BDOOR_CMD_GET_FORCE_X2APIC         76 /* CPL 0 only  */
#define   BDOOR_CMD_SET_PCI_HOLE             77 /* CPL 0 only  */
#define   BDOOR_CMD_GET_PCI_HOLE             78 /* CPL 0 only  */
#define   BDOOR_CMD_GET_PCI_BAR              79 /* CPL 0 only  */
#define   BDOOR_CMD_SHOULD_GENERATE_SYSTEMID 80 /* CPL 0 only  */
#define   BDOOR_CMD_READ_DEBUG_FILE          81 /* Devel only. */
#define   BDOOR_CMD_SCREENSHOT               82 /* Devel only. */
#define   BDOOR_CMD_INJECT_KEY               83 /* Devel only. */
#define   BDOOR_CMD_INJECT_MOUSE             84 /* Devel only. */
#define   BDOOR_CMD_MKS_GUEST_STATS          85 /* CPL 0 only. */
#  define BDOOR_CMD_MKSGS_RESET               0
#  define BDOOR_CMD_MKSGS_ADD_PPN             1
#  define BDOOR_CMD_MKSGS_REMOVE_PPN          2
#define   BDOOR_CMD_ABSPOINTER_RESTRICT      86
#define   BDOOR_CMD_GUEST_INTEGRITY          87
#  define BDOOR_CMD_GI_GET_CAPABILITIES       0
#  define BDOOR_CMD_GI_SETUP_ENTRY_POINT      1
#  define BDOOR_CMD_GI_SETUP_ALERTS           2
#  define BDOOR_CMD_GI_SETUP_STORE            3
#  define BDOOR_CMD_GI_SETUP_EVENT_RING       4
#  define BDOOR_CMD_GI_SETUP_NON_FAULT_READ   5
#  define BDOOR_CMD_GI_ENTER_INTEGRITY_MODE   6
#  define BDOOR_CMD_GI_EXIT_INTEGRITY_MODE    7
#  define BDOOR_CMD_GI_RESET_INTEGRITY_MODE   8
#  define BDOOR_CMD_GI_GET_EVENT_RING_STATE   9
#  define BDOOR_CMD_GI_CONSUME_RING_EVENTS   10
#  define BDOOR_CMD_GI_WATCH_MAPPINGS_START  11
#  define BDOOR_CMD_GI_WATCH_MAPPINGS_STOP   12
#  define BDOOR_CMD_GI_CHECK_MAPPINGS_NOW    13
#  define BDOOR_CMD_GI_WATCH_PPNS_START      14
#  define BDOOR_CMD_GI_WATCH_PPNS_STOP       15
#  define BDOOR_CMD_GI_SEND_MSG              16
#  define BDOOR_CMD_GI_TEST_READ_MOB        128
#  define BDOOR_CMD_GI_TEST_ADD_EVENT       129
#  define BDOOR_CMD_GI_TEST_MAPPING         130
#  define BDOOR_CMD_GI_TEST_PPN             131
#  define BDOOR_CMD_GI_MAX                  131
#define   BDOOR_CMD_MKSSTATS_SNAPSHOT        88 /* Devel only. */
#  define BDOOR_CMD_MKSSTATS_START            0
#  define BDOOR_CMD_MKSSTATS_STOP             1
#define   BDOOR_CMD_SECUREBOOT               89
#define   BDOOR_CMD_COPY_PHYSMEM             90 /* Devel only. */
#define   BDOOR_CMD_STEALCLOCK               91 /* CPL 0 only. */
#  define BDOOR_STEALCLOCK_STATUS_DISABLED    0
#  define BDOOR_STEALCLOCK_STATUS_ENABLED     1
#define   BDOOR_CMD_GUEST_PAGE_HINTS         92 /* CPL 0 only  */
#define   BDOOR_CMD_FIRMWARE_UPDATE          93 /* CPL 0 only. */
#  define BDOOR_CMD_FU_GET_HOST_VERSION       0
#  define BDOOR_CMD_FU_UPDATE_FROM_HOST       1
#  define BDOOR_CMD_FU_LOCK                   2
#define   BDOOR_CMD_FUZZER_HELPER            94 /* Devel only. */
#  define BDOOR_CMD_FUZZER_INIT               0
#  define BDOOR_CMD_FUZZER_NEXT               1
#define   BDOOR_CMD_PUTCHR12                 95
#define   BDOOR_CMD_MAX                      96


/*
 * IMPORTANT NOTE: When modifying the behavior of an existing backdoor command,
 * you must adhere to the semantics expected by the oldest Tools who use that
 * command. Specifically, do not alter the way in which the command modifies
 * the registers. Otherwise backwards compatibility will suffer.
 */

/* Nesting control operations */

#define NESTING_CONTROL_RESTRICT_BACKDOOR 0
#define NESTING_CONTROL_OPEN_BACKDOOR     1
#define NESTING_CONTROL_QUERY             2
#define NESTING_CONTROL_MAX               2

/* EFI Boot Order options, nibble-sized. */
#define EFI_BOOT_ORDER_TYPE_EFI           0x0
#define EFI_BOOT_ORDER_TYPE_LEGACY        0x1
#define EFI_BOOT_ORDER_TYPE_NONE          0xf

#define BDOOR_NETWORK_BOOT_PROTOCOL_NONE  0x0
#define BDOOR_NETWORK_BOOT_PROTOCOL_IPV4  0x1
#define BDOOR_NETWORK_BOOT_PROTOCOL_IPV6  0x2

#define BDOOR_SECUREBOOT_STATUS_DISABLED  0xFFFFFFFFUL
#define BDOOR_SECUREBOOT_STATUS_APPROVED  1
#define BDOOR_SECUREBOOT_STATUS_DENIED    2

/* High-bandwidth backdoor port. --hpreg */

#define BDOORHB_PORT 0x5659
#define BDOORHB_CMD_MESSAGE 0
#define BDOORHB_CMD_VASSERT 1
#define BDOORHB_CMD_MAX 2

/*
 * There is another backdoor which allows access to certain TSC-related
 * values using otherwise illegal PMC indices when the pseudo_perfctr
 * control flag is set.
 */

#define BDOOR_PMC_HW_TSC      0x10000
#define BDOOR_PMC_REAL_NS     0x10001
#define BDOOR_PMC_APPARENT_NS 0x10002
#define BDOOR_PMC_PSEUDO_TSC  0x10003

#define IS_BDOOR_PMC(index)  (((index) | 3) == 0x10003)
#define BDOOR_CMD(ecx)       ((ecx) & 0xffff)

/* Sub commands for BDOOR_CMD_VMK_INFO */
#define BDOOR_CMD_VMK_INFO_ENTRY   1

/*
 * Current format for the guest page hints is:
 *
 * Arg0: BDOOR_MAGIC, Arg3: BDOOR_PORT
 *
 * Arg1: (rbx on x86)
 *
 *  0         64
 *  |   PPN   |
 *
 * Arg2: (rcx on x86)
 *
 *  0         16        32         64
 *  | Command |  Type   | Reserved |
 *
 * Arg4: (rsi on x86)
 *
 *  0          16         64
 *  | numPages | Reserved |
 *
 */
#define BDOOR_GUEST_PAGE_HINTS_NOT_SUPPORTED ((unsigned)-1)
#define BDOOR_GUEST_PAGE_HINTS_MAX_PAGES     (0xffff)
#define BDOOR_GUEST_PAGE_HINTS_TYPE_PSHARE   (0)
#define BDOOR_GUEST_PAGE_HINTS_TYPE(reg)     (((reg) >> 16) & 0xffff)

#ifdef VMM
/*
 *----------------------------------------------------------------------
 *
 * Backdoor_CmdRequiresFullyValidVCPU --
 *
 *    A few backdoor commands require the full VCPU to be valid
 *    (including GDTR, IDTR, TR and LDTR). The rest get read/write
 *    access to GPRs and read access to Segment registers (selectors).
 *
 * Result:
 *    True iff VECX contains a command that require the full VCPU to
 *    be valid.
 *
 *----------------------------------------------------------------------
 */
static INLINE Bool
Backdoor_CmdRequiresFullyValidVCPU(unsigned cmd)
{
   return cmd == BDOOR_CMD_SIDT ||
          cmd == BDOOR_CMD_SGDT ||
          cmd == BDOOR_CMD_SLDT_STR;
}
#endif

#ifdef VM_ARM_64

#define BDOOR_ARM64_LB_PORT      (BDOOR_PORT)
#define BDOOR_ARM64_HB_PORT_IN   (BDOORHB_PORT)
#define BDOOR_ARM64_HB_PORT_OUT  (BDOORHB_PORT +1)

#define BDOOR_ARG0 REG_X0
#define BDOOR_ARG1 REG_X1
#define BDOOR_ARG2 REG_X2
#define BDOOR_ARG3 REG_X3
#define BDOOR_ARG4 REG_X4
#define BDOOR_ARG5 REG_X5
#define BDOOR_ARG6 REG_X6

#else

#define BDOOR_ARG0 REG_RAX
#define BDOOR_ARG1 REG_RBX
#define BDOOR_ARG2 REG_RCX
#define BDOOR_ARG3 REG_RDX
#define BDOOR_ARG4 REG_RSI
#define BDOOR_ARG5 REG_RDI
#define BDOOR_ARG6 REG_RBP

#endif


#if defined __cplusplus
}
#endif

#endif // _BACKDOOR_DEF_H_
