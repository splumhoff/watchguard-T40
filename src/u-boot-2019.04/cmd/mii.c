// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2001
 * Gerald Van Baren, Custom IDEAS, vanbaren@cideas.com
 */

/*
 * MII Utilities
 */

#include <common.h>
#include <command.h>
#include <miiphy.h>

#if !defined(CONFIG_WG1008_PX2)
typedef struct _MII_reg_desc_t {
	ushort regno;
	char * name;
} MII_reg_desc_t;

static const MII_reg_desc_t reg_0_5_desc_tbl[] = {
	{ MII_BMCR,      "PHY control register" },
	{ MII_BMSR,      "PHY status register" },
	{ MII_PHYSID1,   "PHY ID 1 register" },
	{ MII_PHYSID2,   "PHY ID 2 register" },
	{ MII_ADVERTISE, "Autonegotiation advertisement register" },
	{ MII_LPA,       "Autonegotiation partner abilities register" },
};
#endif

typedef struct _MII_field_desc_t {
	ushort hi;
	ushort lo;
	ushort mask;
	const char *name;
} MII_field_desc_t;

static const MII_field_desc_t reg_0_desc_tbl[] = {
	{ 15, 15, 0x01, "reset"                        },
	{ 14, 14, 0x01, "loopback"                     },
	{ 13,  6, 0x81, "speed selection"              }, /* special */
	{ 12, 12, 0x01, "A/N enable"                   },
	{ 11, 11, 0x01, "power-down"                   },
	{ 10, 10, 0x01, "isolate"                      },
	{  9,  9, 0x01, "restart A/N"                  },
	{  8,  8, 0x01, "duplex"                       }, /* special */
	{  7,  7, 0x01, "collision test enable"        },
	{  5,  0, 0x3f, "(reserved)"                   }
};

static const MII_field_desc_t reg_1_desc_tbl[] = {
	{ 15, 15, 0x01, "100BASE-T4 able"              },
	{ 14, 14, 0x01, "100BASE-X  full duplex able"  },
	{ 13, 13, 0x01, "100BASE-X  half duplex able"  },
	{ 12, 12, 0x01, "10 Mbps    full duplex able"  },
	{ 11, 11, 0x01, "10 Mbps    half duplex able"  },
	{ 10, 10, 0x01, "100BASE-T2 full duplex able"  },
	{  9,  9, 0x01, "100BASE-T2 half duplex able"  },
	{  8,  8, 0x01, "extended status"              },
	{  7,  7, 0x01, "(reserved)"                   },
	{  6,  6, 0x01, "MF preamble suppression"      },
	{  5,  5, 0x01, "A/N complete"                 },
	{  4,  4, 0x01, "remote fault"                 },
	{  3,  3, 0x01, "A/N able"                     },
	{  2,  2, 0x01, "link status"                  },
	{  1,  1, 0x01, "jabber detect"                },
	{  0,  0, 0x01, "extended capabilities"        },
};

static const MII_field_desc_t reg_2_desc_tbl[] = {
	{ 15,  0, 0xffff, "OUI portion"                },
};

static const MII_field_desc_t reg_3_desc_tbl[] = {
	{ 15, 10, 0x3f, "OUI portion"                },
	{  9,  4, 0x3f, "manufacturer part number"   },
	{  3,  0, 0x0f, "manufacturer rev. number"   },
};

static const MII_field_desc_t reg_4_desc_tbl[] = {
	{ 15, 15, 0x01, "next page able"               },
	{ 14, 14, 0x01, "(reserved)"                   },
	{ 13, 13, 0x01, "remote fault"                 },
	{ 12, 12, 0x01, "(reserved)"                   },
	{ 11, 11, 0x01, "asymmetric pause"             },
	{ 10, 10, 0x01, "pause enable"                 },
	{  9,  9, 0x01, "100BASE-T4 able"              },
	{  8,  8, 0x01, "100BASE-TX full duplex able"  },
	{  7,  7, 0x01, "100BASE-TX able"              },
	{  6,  6, 0x01, "10BASE-T   full duplex able"  },
	{  5,  5, 0x01, "10BASE-T   able"              },
	{  4,  0, 0x1f, "selector"                     },
};

static const MII_field_desc_t reg_5_desc_tbl[] = {
	{ 15, 15, 0x01, "next page able"               },
	{ 14, 14, 0x01, "acknowledge"                  },
	{ 13, 13, 0x01, "remote fault"                 },
	{ 12, 12, 0x01, "(reserved)"                   },
	{ 11, 11, 0x01, "asymmetric pause able"        },
	{ 10, 10, 0x01, "pause able"                   },
	{  9,  9, 0x01, "100BASE-T4 able"              },
	{  8,  8, 0x01, "100BASE-X full duplex able"   },
	{  7,  7, 0x01, "100BASE-TX able"              },
	{  6,  6, 0x01, "10BASE-T full duplex able"    },
	{  5,  5, 0x01, "10BASE-T able"                },
	{  4,  0, 0x1f, "partner selector"             },
};

#if defined(CONFIG_WG1008_PX2)
static const MII_field_desc_t reg_9_desc_tbl[] = {
	{ 15, 13, 0x07, "test mode"		       },
	{ 12, 12, 0x01, "manual master/slave enable"   },
	{ 11, 11, 0x01, "manual master/slave value"    },
	{ 10, 10, 0x01, "multi/single port"            },
	{  9,  9, 0x01, "1000BASE-T full duplex able"  },
	{  8,  8, 0x01, "1000BASE-T half duplex able"  },
	{  7,  7, 0x01, "automatic TDR on link down"   },
	{  6,  6, 0x7f, "(reserved)"                   },
};

static const MII_field_desc_t reg_10_desc_tbl[] = {
	{ 15, 15, 0x01, "master/slave config fault"    },
	{ 14, 14, 0x01, "master/slave config result"   },
	{ 13, 13, 0x01, "local receiver status OK"     },
	{ 12, 12, 0x01, "remote receiver status OK"    },
	{ 11, 11, 0x01, "1000BASE-T full duplex able"  },
	{ 10, 10, 0x01, "1000BASE-T half duplex able"  },
	{  9,  8, 0x03, "(reserved)"                   },
	{  7,  0, 0xff, "1000BASE-T idle error counter"},
};

typedef struct _MII_reg_desc_t {
	ushort regno;
	const MII_field_desc_t *pdesc;
	ushort len;
	const char *name;
} MII_reg_desc_t;

static const MII_reg_desc_t mii_reg_desc_tbl[] = {
	{ MII_BMCR,      reg_0_desc_tbl, ARRAY_SIZE(reg_0_desc_tbl),
		"PHY control register" },
	{ MII_BMSR,      reg_1_desc_tbl, ARRAY_SIZE(reg_1_desc_tbl),
		"PHY status register" },
	{ MII_PHYSID1,   reg_2_desc_tbl, ARRAY_SIZE(reg_2_desc_tbl),
		"PHY ID 1 register" },
	{ MII_PHYSID2,   reg_3_desc_tbl, ARRAY_SIZE(reg_3_desc_tbl),
		"PHY ID 2 register" },
	{ MII_ADVERTISE, reg_4_desc_tbl, ARRAY_SIZE(reg_4_desc_tbl),
		"Autonegotiation advertisement register" },
	{ MII_LPA,       reg_5_desc_tbl, ARRAY_SIZE(reg_5_desc_tbl),
		"Autonegotiation partner abilities register" },
	{ MII_CTRL1000,	 reg_9_desc_tbl, ARRAY_SIZE(reg_9_desc_tbl),
		"1000BASE-T control register" },
	{ MII_STAT1000,	 reg_10_desc_tbl, ARRAY_SIZE(reg_10_desc_tbl),
		"1000BASE-T status register" },
};

static void dump_reg(
	ushort             regval,
	const MII_reg_desc_t *prd);

static bool special_field(ushort regno, const MII_field_desc_t *pdesc,
			  ushort regval);
#else

typedef struct _MII_field_desc_and_len_t {
	const MII_field_desc_t *pdesc;
	ushort len;
} MII_field_desc_and_len_t;

static const MII_field_desc_and_len_t desc_and_len_tbl[] = {
	{ reg_0_desc_tbl, ARRAY_SIZE(reg_0_desc_tbl)   },
	{ reg_1_desc_tbl, ARRAY_SIZE(reg_1_desc_tbl)   },
	{ reg_2_desc_tbl, ARRAY_SIZE(reg_2_desc_tbl)   },
	{ reg_3_desc_tbl, ARRAY_SIZE(reg_3_desc_tbl)   },
	{ reg_4_desc_tbl, ARRAY_SIZE(reg_4_desc_tbl)   },
	{ reg_5_desc_tbl, ARRAY_SIZE(reg_5_desc_tbl)   },
};

static void dump_reg(
	ushort             regval,
	const MII_reg_desc_t *prd,
	const MII_field_desc_and_len_t *pdl);

static int special_field(
	ushort regno,
	const MII_field_desc_t *pdesc,
	ushort regval);

#endif

#if defined(CONFIG_WG1008_PX2)
static void MII_dump(const ushort *regvals, uchar reglo, uchar reghi)
{
	ulong i;

	for (i = 0; i < ARRAY_SIZE(mii_reg_desc_tbl); i++) {
		const uchar reg = mii_reg_desc_tbl[i].regno;

		if (reg >= reglo && reg <= reghi)
			dump_reg(regvals[reg - reglo], &mii_reg_desc_tbl[i]);
	}
}
#else
static void MII_dump_0_to_5(
	ushort regvals[6],
	uchar reglo,
	uchar reghi)
{
	ulong i;

	for (i = 0; i < 6; i++) {
		if ((reglo <= i) && (i <= reghi))
			dump_reg(regvals[i], &reg_0_5_desc_tbl[i],
				&desc_and_len_tbl[i]);
	}
}
#endif

#if defined(CONFIG_WG1008_PX2)
/* Print out field position, value, name */
static void dump_field(const MII_field_desc_t *pdesc, ushort regval)
{
	if (pdesc->hi == pdesc->lo)
		printf("%2u   ", pdesc->lo);
	else
		printf("%2u-%2u", pdesc->hi, pdesc->lo);

	printf(" = %5u	  %s", (regval >> pdesc->lo) & pdesc->mask,
	       pdesc->name);
}
#endif

static void dump_reg(
	ushort             regval,
#if defined(CONFIG_WG1008_PX2)
	const MII_reg_desc_t *prd)
#else
	const MII_reg_desc_t *prd,
	const MII_field_desc_and_len_t *pdl)
#endif
{
	ulong i;
	ushort mask_in_place;
	const MII_field_desc_t *pdesc;

	printf("%u.     (%04hx)                 -- %s --\n",
		prd->regno, regval, prd->name);

#if defined(CONFIG_WG1008_PX2)
	for (i = 0; i < prd->len; i++) {
		pdesc = &prd->pdesc[i];
#else
	for (i = 0; i < pdl->len; i++) {
		pdesc = &pdl->pdesc[i];
#endif

		mask_in_place = pdesc->mask << pdesc->lo;

		printf("  (%04hx:%04x) %u.",
		       mask_in_place,
		       regval & mask_in_place,
		       prd->regno);
#if defined(CONFIG_WG1008_PX2)
		if (!special_field(prd->regno, pdesc, regval))
			dump_field(pdesc, regval);
#else
		if (special_field(prd->regno, pdesc, regval)) {
		}
		else {
			if (pdesc->hi == pdesc->lo)
				printf("%2u   ", pdesc->lo);
			else
				printf("%2u-%2u", pdesc->hi, pdesc->lo);
			printf(" = %5u    %s",
				(regval & mask_in_place) >> pdesc->lo,
				pdesc->name);
		}
#endif
		printf("\n");

	}
	printf("\n");
}

/* Special fields:
** 0.6,13
** 0.8
** 2.15-0
** 3.15-0
** 4.4-0
** 5.4-0
*/

#if defined(CONFIG_WG1008_PX2)
static bool special_field(ushort regno, const MII_field_desc_t *pdesc,
			  ushort regval)
#else
static int special_field(
	ushort regno,
	const MII_field_desc_t *pdesc,
	ushort regval)
#endif
{
	const ushort sel_bits = (regval >> pdesc->lo) & pdesc->mask;

	if ((regno == MII_BMCR) && (pdesc->lo == 6)) {
		ushort speed_bits = regval & (BMCR_SPEED1000 | BMCR_SPEED100);
		printf("%2u,%2u =   b%u%u    speed selection = %s Mbps",
			6, 13,
			(regval >>  6) & 1,
			(regval >> 13) & 1,
			speed_bits == BMCR_SPEED1000 ? "1000" :
			speed_bits == BMCR_SPEED100  ? "100" :
			"10");
		return 1;
	}

	else if ((regno == MII_BMCR) && (pdesc->lo == 8)) {
#if defined(CONFIG_WG1008_PX2)
		dump_field(pdesc, regval);
		printf(" = %s", ((regval >> pdesc->lo) & 1) ? "full" : "half");
#else
		printf("%2u    = %5u    duplex = %s",
			pdesc->lo,
			(regval >>  pdesc->lo) & 1,
			((regval >> pdesc->lo) & 1) ? "full" : "half");
#endif

		return 1;
	}

	else if ((regno == MII_ADVERTISE) && (pdesc->lo == 0)) {
#if defined(CONFIG_WG1008_PX2)
		dump_field(pdesc, regval);
		printf(" = %s",
		       sel_bits == PHY_ANLPAR_PSB_802_3 ? "IEEE 802.3 CSMA/CD" :
		       sel_bits == PHY_ANLPAR_PSB_802_9 ?
		       "IEEE 802.9 ISLAN-16T" : "???");
#else
		ushort sel_bits = (regval >> pdesc->lo) & pdesc->mask;
		printf("%2u-%2u = %5u    selector = %s",
			pdesc->hi, pdesc->lo, sel_bits,
			sel_bits == PHY_ANLPAR_PSB_802_3 ?
				"IEEE 802.3" :
			sel_bits == PHY_ANLPAR_PSB_802_9 ?
				"IEEE 802.9 ISLAN-16T" :
			"???");
#endif
		return 1;
	}

	else if ((regno == MII_LPA) && (pdesc->lo == 0)) {
#if defined(CONFIG_WG1008_PX2)
		dump_field(pdesc, regval);
		printf(" = %s",
		       sel_bits == PHY_ANLPAR_PSB_802_3 ? "IEEE 802.3 CSMA/CD" :
		       sel_bits == PHY_ANLPAR_PSB_802_9 ?
		       "IEEE 802.9 ISLAN-16T" : "???");
#else
		ushort sel_bits = (regval >> pdesc->lo) & pdesc->mask;
		printf("%2u-%2u =     %u    selector = %s",
			pdesc->hi, pdesc->lo, sel_bits,
			sel_bits == PHY_ANLPAR_PSB_802_3 ?
				"IEEE 802.3" :
			sel_bits == PHY_ANLPAR_PSB_802_9 ?
				"IEEE 802.9 ISLAN-16T" :
			"???");
#endif
		return 1;
	}

	return 0;
}

static char last_op[2];
static uint last_data;
static uint last_addr_lo;
static uint last_addr_hi;
static uint last_reg_lo;
static uint last_reg_hi;
static uint last_mask;

#ifndef CONFIG_MV88E6190_SWITCH
static void extract_range(
	char * input,
	unsigned char * plo,
	unsigned char * phi)
{
	char * end;
	*plo = simple_strtoul(input, &end, 16);
    if (*end == '-')
    {
		end++;
		*phi = simple_strtoul(end, NULL, 16);
	}
    else
    {
		*phi = *plo;
	}
}
#else
static void extract_range(
    char * input,
    unsigned short * plo,
    unsigned short * phi)
{
    unsigned char * end;
    *plo = simple_strtoul(input, &end, 16);
    if (*end == '-')
    {
        end++;
        *phi = simple_strtoul(end, NULL, 16);
    }
    else
    {
        *phi = *plo;
    }
}
#endif

#if defined(CONFIG_MV88E6190_SWITCH) || defined(CONFIG_MV88E6191X_SWITCH)
extern int switch_mv88e6190_port_autonego(u16 port, u32 *an, int read);
extern int switch_mv88e6190_port_speed(u16 port, u32 *speed, int read); /* speed unit : Mbps */
extern int switch_mv88e6190_port_duplex(u16 port, u32 *fullDupex, int read);
extern int switch_mv88e6190_port_mdi_mode(u16 port, u32 *mdiMode, int read);
extern int switch_mv88e6190_port_autonego_complete_read(u16 port, u32 *complete, int read);
extern int switch_mv88e6190_port_copper_link_status_read(u16 port, u32 *status, int read);
extern int switch_mv88e6190_port_copper_remote_fault_read(u16 port, u32 *fault, int read);
extern int switch_mv88e6190_port_jabber_detect_read(u16 port, u32 *jabber, int read);
extern int switch_mv88e6190_port_cop_specific_speed_read(u16 port, u32 *speed, int read);
extern int switch_mv88e6190_port_cop_specific_duplex_read(u16 port, u32 *duplex, int read);
extern int switch_mv88e6190_port_cop_specific_mdix_read(u16 port, u32 *mdix, int read); /* midx : 1; mdi : 0 */
extern int switch_mv88e6190_port_cop_specific_cop_link_rt_read(u16 port, u32 *staus, int read);
extern int switch_mv88e6190_port_cop_specific_global_link_status_read(u16 port, u32 *staus, int read);
#endif
/* ---------------------------------------------------------------- */
static int do_mii(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char		op[2];
#if defined(CONFIG_MV88E6190_SWITCH)
    unsigned short	addrlo, addrhi, reglo, reghi;
    unsigned short	addr, reg;
#else
	unsigned char	addrlo, addrhi, reglo, reghi;
	unsigned char	addr, reg;
#endif
	unsigned short	data, mask;
	int		rcode = 0;
	const char	*devname;

	if (argc < 2)
		return CMD_RET_USAGE;

#if defined(CONFIG_MII_INIT)
	mii_init ();
#endif

	/*
	 * We use the last specified parameters, unless new ones are
	 * entered.
	 */
	op[0] = last_op[0];
	op[1] = last_op[1];
	addrlo = last_addr_lo;
	addrhi = last_addr_hi;
	reglo  = last_reg_lo;
	reghi  = last_reg_hi;
	data   = last_data;
	mask   = last_mask;

	if ((flag & CMD_FLAG_REPEAT) == 0) {
		op[0] = argv[1][0];
		if (strlen(argv[1]) > 1)
			op[1] = argv[1][1];
		else
			op[1] = '\0';

		if (argc >= 3)
			extract_range(argv[2], &addrlo, &addrhi);
		if (argc >= 4)
		{
#if defined(CONFIG_MV88E6190_SWITCH) || defined(CONFIG_MV88E6191X_SWITCH)
        	if(!strncmp(argv[3], "mvl", strlen("mvl")))
        	{
            		extract_range(argv[2], &addrlo, &addrhi);
        	}
		else
#endif
			extract_range(argv[3], &reglo, &reghi);
		}
		if (argc >= 5)
		{
#if defined(CONFIG_MV88E6191X_SWITCH)
		    if(!strncmp(argv[4], "mvl", strlen("mvl")) || !strncmp(argv[4], "srds", strlen("srds")))
		    {
		        extract_range(argv[3], &reglo, &reghi);
		    }
		    else
#elif defined(CONFIG_MV88E6190_SWITCH)
		    if(!strncmp(argv[4], "mvl", strlen("mvl")))
		    {
		        extract_range(argv[3], &reglo, &reghi);
		    }
		    else
#endif
                data = simple_strtoul(argv[4], NULL, 16);
		}
		if (argc >= 6)
		{
#if defined(CONFIG_MV88E6191X_SWITCH)
			if(strncmp(argv[5], "mvl", strlen("mvl")) || strncmp(argv[5], "srds", strlen("srds")))
#elif defined(CONFIG_MV88E6190_SWITCH)
			if(strncmp(argv[5], "mvl", strlen("mvl")))
#endif
			mask = simple_strtoul(argv[5], NULL, 16);
		}
	}

#if defined(CONFIG_MV88E6190_SWITCH)
	if (addrhi > 31) {
#else
	if (addrhi > 31 && strncmp(op, "de", 2)) {
#endif
		printf("Incorrect PHY address. Range should be 0-31\n");
		return CMD_RET_USAGE;
	}

	/* use current device */
	devname = miiphy_get_current_dev();

	/*
	 * check info/read/write.
	 */
	if (op[0] == 'i') {
		unsigned char j, start, end;
		unsigned int oui;
		unsigned char model;
		unsigned char rev;

		/*
		 * Look for any and all PHYs.  Valid addresses are 0..31.
		 */
#if defined(CONFIG_MV88E6190_SWITCH)
		if (argc >= 3) {
			start = addrlo;
			end = addrhi;
		} else 	{
			start = 0;
			end = 31;
		}
#else
		if (argc >= 3) {
			start = addrlo; end = addrhi;
		} else {
			start = 0; end = 31;
		}
#endif

		for (j = start; j <= end; j++) {
			if (miiphy_info (devname, j, &oui, &model, &rev) == 0) {
				printf("PHY 0x%02X: "
					"OUI = 0x%04X, "
					"Model = 0x%02X, "
					"Rev = 0x%02X, "
					"%3dbase%s, %s\n",
					j, oui, model, rev,
					miiphy_speed (devname, j),
					miiphy_is_1000base_x (devname, j)
						? "X" : "T",
					(miiphy_duplex (devname, j) == FULL)
						? "FDX" : "HDX");
			}
		}
	}
#if defined(CONFIG_MV88E6190_SWITCH) || defined(CONFIG_MV88E6191X_SWITCH)
    else if ((!strncmp(argv[1], "an", 2) || !strncmp(argv[1], "speed", 5) || !strncmp(argv[1], "duplex", 6) || !strncmp(argv[1], "mdi", 3)
           || !strncmp(argv[1], "ancomp", 6) || !strncmp(argv[1], "coplink", 7) || !strncmp(argv[1], "rtlink", 6) || !strncmp(argv[1], "glblink", 7) 
           || !strncmp(argv[1], "remflt", 6) || !strncmp(argv[1], "jabdet", 6) || !strncmp(argv[1], "speedrd", 7) || !strncmp(argv[1], "duplexrd", 8) || !strncmp(argv[1], "mdixrd", 6))
          && (!strncmp(argv[argc - 1], "mvl", strlen("mvl"))))
    {
        int(*mvOp)(u16, u32*, int);
        u32 val = 0;

        if(!strncmp(argv[1], "ancomp", 6) && (argc == 4))
        {
            mvOp = switch_mv88e6190_port_autonego_complete_read;
        }
        else if(!strncmp(argv[1], "coplink", 7) && (argc == 4))
        {
            mvOp = switch_mv88e6190_port_copper_link_status_read;
        }
        else if(!strncmp(argv[1], "rtlink", 6) && (argc == 4))
        {
            mvOp = switch_mv88e6190_port_cop_specific_cop_link_rt_read;
        }
        else if(!strncmp(argv[1], "glblink", 7) && (argc == 4))
        {
            mvOp = switch_mv88e6190_port_cop_specific_global_link_status_read;
        }
        else if(!strncmp(argv[1], "remflt", 6) && (argc == 4))
        {
            mvOp = switch_mv88e6190_port_copper_remote_fault_read;
        }
        else if(!strncmp(argv[1], "jabdet", 6) && (argc == 4))
        {
            mvOp = switch_mv88e6190_port_jabber_detect_read;
        }
        else if(!strncmp(argv[1], "speedrd", 7) && (argc == 4))
        {
            mvOp = switch_mv88e6190_port_cop_specific_speed_read;
        }
        else if(!strncmp(argv[1], "duplexrd", 8) && (argc == 4))
        {
            mvOp = switch_mv88e6190_port_cop_specific_duplex_read;
        }
        else if(!strncmp(argv[1], "mdixrd", 6) && (argc == 4))
        {
            mvOp = switch_mv88e6190_port_cop_specific_mdix_read;
        }
        else if(!strncmp(argv[1], "an", 2))
        {
            mvOp = switch_mv88e6190_port_autonego;
        }
        else if(!strncmp(argv[1], "speed", 5))
        {
            mvOp = switch_mv88e6190_port_speed;
        }
        else if(!strncmp(argv[1], "duplex", 6))
        {
            mvOp = switch_mv88e6190_port_duplex;
        }
        else if(!strncmp(argv[1], "mdi", 3))
        {
            mvOp = switch_mv88e6190_port_mdi_mode;
        }

        if (argc == 5)
        {
#if 0
            addr = simple_strtoul(argv[2], NULL, 16); 
            reg = simple_strtoul(argv[3], NULL, 16);
#else
            for (addr = addrlo; addr <= addrhi; addr++)
#endif
            {
                for (reg = reglo; reg <= reghi; reg++)
                {
                    val = (u32)reg;
                    if (mvOp && mvOp(addr, &val, 0))
                    {
                        printf("Error writing %s from the port:", argv[1]);
                        printf(" port=%02x", addr);
                        printf(" %s=0x%x(%u)\n", argv[1], val, val);
                        rcode = 1;
                    }
                    else
                    {
                        printf("Written %s from the port:", argv[1]);
                        printf(" port=%02x", addr);
                        printf(" %s=0x%x(%u)\n", argv[1], val, val);
                    }
                }
            }
        }
        else if (argc == 4)
        {
#if 0
            addr = simple_strtoul(argv[2], NULL, 16); 
#else
            for (addr = addrlo; addr <= addrhi; addr++)
#endif
            {
                if (mvOp && mvOp(addr, &val, 1))
                {
                    printf("Error reading %s from the port:", argv[1]);
                    printf(" port=%02x\n", addr);
                    rcode = 1;
                }
                else
                {
                    printf("Reading %s from the port:", argv[1]);
                    printf(" port=%02x", addr);
                    printf(" %s=0x%x(%u)\n", argv[1], val, val);
                }
            }
        }
        else
        {
            rcode = CMD_RET_USAGE;
        }
    }
#endif //  defined(CONFIG_MV88E6190_SWITCH) || defined(CONFIG_MV88E6191X_SWITCH)
	else if (op[0] == 'r') {
		for (addr = addrlo; addr <= addrhi; addr++) {
			for (reg = reglo; reg <= reghi; reg++) {
				data = 0xffff;
#if defined(CONFIG_MV88E6190_SWITCH) || defined(CONFIG_MV88E6191X_SWITCH)
                if(!strncmp(argv[4], "mvl", strlen("mvl")))
                {
                    if (marvell_phy_read (addr, reg, &data) != 0)
                    {
                        printf(
                            "Error reading from the Marvell PHY addr=0x%02x reg=0x%02x\n",
                            addr, reg);
                        rcode = 1;
                    }
                    else
                    {
                        if ((addrlo != addrhi) || (reglo != reghi))
                            printf("Marvell addr=0x%02x reg=0x%02x data=",
                                   (uint)addr, (uint)reg);
                        printf("0x%04X\n", data & 0x0000FFFF);
                    }
                }
		else
#endif
#if defined(CONFIG_MV88E6191X_SWITCH)
		if (!strncmp(argv[4], "srds", strlen("srds")))
		{
					unsigned short offset= 0x8100 + reg;
					DBG_PRINTF("reglo: 0x%0x reghi: 0x%0x reg: 0x%0x\n ", reglo, reghi, offset);
					
					if (xsmi_read (SMI_I, addr, 30, offset, &data) != 0)
	                {
                        printf(
                            "Error reading from the Marvell PHY addr=0x%02x reg=0x%02x\n",
                            addr, reg);
                        rcode = 1;
                    }
			else
                    {
                        if ((addrlo != addrhi) || (reglo != reghi))
							printf("Marvell addr=0x%02x reg=0x%02x data=",
                                    (uint)addr, offset);
                        printf("0x%0x\n", data);
                    }
		}
		else
#endif // CONFIG_MV88E6191X_SWITCH
				if (miiphy_read (devname, addr, reg, &data) != 0) {
					printf(
					"Error reading from the PHY addr=%02x reg=%02x\n",
						addr, reg);
					rcode = 1;
				} else {
					if ((addrlo != addrhi) || (reglo != reghi))
						printf("addr=%02x reg=%02x data=",
							(uint)addr, (uint)reg);
					printf("%04X\n", data & 0x0000FFFF);
				}
			}
			if ((addrlo != addrhi) && (reglo != reghi))
				printf("\n");
		}
	} 
	else if (op[0] == 'w') {
		for (addr = addrlo; addr <= addrhi; addr++) {
			for (reg = reglo; reg <= reghi; reg++) {
#if defined(CONFIG_MV88E6190_SWITCH) || defined(CONFIG_MV88E6191X_SWITCH)
		        if(!strncmp(argv[5], "mvl", strlen("mvl")))
		        {
		            if (marvell_phy_write (addr, reg, data) != 0)
		            {
		                printf("Error writing to the Marvell PHY addr=%02x reg=%02x\n",
		                       addr, reg);
		                rcode = 1;
		            }
		        }
			else
#endif
#if defined(CONFIG_MV88E6191X_SWITCH)
		        if (!strncmp(argv[5], "srds", strlen("srds")))
			{
				unsigned short offset= 0x8100 + reg;
				DBG_PRINTF("reglo: 0x%0x reghi: 0x%0x reg: 0x%0x data: 0x%0x\n ", reglo, reghi, offset, data);
				printf("Input Offset range: 0x8100~0x8199\n");
				
				if (xsmi_write (SMI_I, addr, 30, offset, data) != 0)
				{
		                printf("Error writing to the Marvell PHY addr=%02x reg=%02x\n",
		                       addr, reg);
		                rcode = 1;
				}
			}
			else
#endif // CONFIG_MV88E6191X_SWITCH
			if (miiphy_write (devname, addr, reg, data) != 0) {
				printf("Error writing to the PHY addr=%02x reg=%02x\n",
					addr, reg);
				rcode = 1;
			}
			}
		}
	} 
	else if (op[0] == 'm') {
		for (addr = addrlo; addr <= addrhi; addr++) {
			for (reg = reglo; reg <= reghi; reg++) {
				unsigned short val = 0;
				if (miiphy_read(devname, addr,
						reg, &val)) {
					printf("Error reading from the PHY");
					printf(" addr=%02x", addr);
					printf(" reg=%02x\n", reg);
					rcode = 1;
				} else {
					val = (val & ~mask) | (data & mask);
					if (miiphy_write(devname, addr,
							 reg, val)) {
						printf("Error writing to the PHY");
						printf(" addr=%02x", addr);
						printf(" reg=%02x\n", reg);
						rcode = 1;
					}
				}
			}
		}
	} 
	else if (strncmp(op, "du", 2) == 0) {
#if defined(CONFIG_MV88E6190_SWITCH)
		ushort regs[6];
#else
		ushort regs[MII_STAT1000 + 1];  /* Last reg is 0x0a */
#endif

		int ok = 1;
#if defined(CONFIG_MV88E6190_SWITCH)
		if ((reglo > 5) || (reghi > 5)) {
#else
		if (reglo > MII_STAT1000 || reghi > MII_STAT1000) {
#endif
#if defined(CONFIG_MV88E6191X_SWITCH)
			printf("The MII dump command only formats the standard MII registers, 0-5, 9-a.\n");
			return 1;
#elif defined (CONFIG_MV88E6190_SWITCH)
			printf(
				"The MII dump command only formats the "
				"standard MII registers, 0-5.\n");
#endif
		}
		for (addr = addrlo; addr <= addrhi; addr++) {
			for (reg = reglo; reg <= reghi; reg++) {
#if defined(CONFIG_MV88E6190_SWITCH)
				if (miiphy_read(devname, addr, reg, &regs[reg]) != 0) {

#else
				if (miiphy_read(devname, addr, reg,
						&regs[reg - reglo]) != 0) {
#endif
					ok = 0;
					printf(
					"Error reading from the PHY addr=%02x reg=%02x\n",
						addr, reg);
					rcode = 1;
				}
			}
#if defined(CONFIG_MV88E6190_SWITCH)
			if (ok)
				MII_dump_0_to_5(regs, reglo, reghi);
#else
			if (ok)
				MII_dump(regs, reglo, reghi);
#endif
			printf("\n");
		}
	} 
	else if (strncmp(op, "de", 2) == 0) {
		if (argc == 2)
			miiphy_listdev ();
		else
			miiphy_set_current_dev (argv[2]);
	} 
	else {
		return CMD_RET_USAGE;
	}

	/*
	 * Save the parameters for repeats.
	 */
	last_op[0] = op[0];
	last_op[1] = op[1];
	last_addr_lo = addrlo;
	last_addr_hi = addrhi;
	last_reg_lo  = reglo;
	last_reg_hi  = reghi;
	last_data    = data;
	last_mask    = mask;

	return rcode;
}

/***************************************************/

U_BOOT_CMD(
	mii, 6, 1, do_mii,
	"MII utility commands",
	"device                            - list available devices\n"
	"mii device <devname>                  - set current device\n"
	"mii info   <addr>                     - display MII PHY info\n"
	"mii read   <addr> <reg>               - read  MII PHY <addr> register <reg>\n"
	"mii write  <addr> <reg> <data>        - write MII PHY <addr> register <reg>\n"
	"mii modify <addr> <reg> <data> <mask> - modify MII PHY <addr> register <reg>\n"
	"                                        updating bits identified in <mask>\n"
	"mii dump   <addr> <reg>               - pretty-print <addr> <reg> (0-5 only)\n"
#if defined(CONFIG_MV88E6190_SWITCH) || defined(CONFIG_MV88E6191X_SWITCH)
    "mii an       <port> [<an>]       mvl  - Marvell PHY read/write <port> <an>\n"
    "mii speed    <port> [<speed>]    mvl  - Marvell PHY read/write <port> <speed>\n"
    "mii duplex   <port> [<duplex>]   mvl  - Marvell PHY read/write <port> <duplex>, 1:full;0:half\n"
    "mii mdi      <port> [<mdi mode>] mvl  - Marvell PHY read/write <port> <mdi/mdix mode>, 0:Manual MDI;1:Manual MDIX;3:Auto MDI/MDIX\n"
    "mii speedrd  <port>              mvl  - Marvell PHY read       <port> speed\n"
    "mii duplexrd <port>              mvl  - Marvell PHY read       <port> duplex\n"
    "mii mdixrd   <port>              mvl  - Marvell PHY read       <port> MDI/MDIX crossover read\n"
    "mii ancomp   <port>              mvl  - Marvell PHY read       <port> autonego complete\n"
    "mii coplink  <port>              mvl  - Marvell PHY read       <port> copper link status\n"
    "mii rtlink   <port>              mvl  - Marvell PHY read       <port> copper link status of realtime\n"
    "mii glblink  <port>              mvl  - Marvell PHY read       <port> global link status\n"
    "mii remflt   <port>              mvl  - Marvell PHY read       <port> remote fault detect\n"
    "mii jabdet   <port>              mvl  - Marvell PHY read       <port> jabber detect\n"
#endif
	"Addr and/or reg may be ranges, e.g. 2-7."
);
