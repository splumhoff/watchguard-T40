#ifndef _LINUX_WG_KERNEL_H
#define _LINUX_WG_KERNEL_H

#define	CONFIG_WG_4_1_8_KERNEL	1

#ifdef	CONFIG_WG_PLATFORM_BACKPORT
#define	CONFIG_WG_PLATFORM_CVE	1
#endif

#ifdef	CONFIG_WG_ARCH_FREESCALE
#define	CONFIG_NXP_SDK		1
#endif

#ifdef	CONFIG_PPC32
#define	WG_TICKER(a) { a = mftbl(); }
#define	CONFIG_WG_ARCH_PPC_32
#endif

#ifdef	CONFIG_PPC64
#define	WG_TICKER(a) { a = mftb();  }
#define	CONFIG_WG_ARCH_PPC_64
#endif

#ifdef  CONFIG_ARM64
#define WG_TICKER(a) { a = read_sysreg(pmccntr_el0);  }
#endif


#ifdef	CONFIG_X86
#define	WG_TICKER(a) rdtscll(a)
#endif

#define	WG_TIMECALL(call) {                                                \
    static u64 high = 0; unsigned u64 start, used;                         \
    WG_TICKER(start); {call;} WG_TICKER(used); used -= start;              \
    if (unlikely(used > high)) printk(KERN_EMERG "Highwater %9lu %s@%d\n", \
                                      high = used, __FUNCTION__, __LINE__); }

#define WG_CONSOLE(a, ...) { int old = console_loglevel; \
                             console_loglevel = 9;       \
                             printk(a, ##__VA_ARGS__);   \
                             console_loglevel = old; }

extern	void  wg_setup_arch(char*, int); // Setup WG boot stuff
extern	int   wg_fault_report(char*);	 // Record a fault report item

extern	char* wg_get_vm_name(void);	 // Get VM vendor name

extern	void  wg_noop(void);		 // NOOP function pointer

#define	WG_CPU_T2081	   2081		 // Freescale T2081
#define	WG_CPU_T1042	   1042		 // Freescale T1042
#define	WG_CPU_T1024	   1024		 // Freescale T1024

#define	WG_CPU_P2020	   2020		 // Freescale P2020
#define	WG_CPU_P1020	   1020		 // Freescale P1020
#define	WG_CPU_P1011	   1011		 // Freescale P1011
#define	WG_CPU_P1010	   1010		 // Freescale P1010

#define	WG_CPU_LS1043	   1043		 // Freescale ARM64 LS1043 (T20/T40)
#define	WG_CPU_LS1046	   1046		 // Freescale ARM64 LS1046 (T80)
#define	WG_CPU_LS2088	   2088 	 // Freescale ARM64 LS2088 (M390)
#define	WG_CPU_LX2160	   2160 	 // Freescale ARM64 LX2160 (M590/M690)


#define	WG_CPU_X86(m)   ( 86000000+(m))	 // Generic Intel X86
#define	WG_CPU_686(m)   (686000000+(m))	 // Generic Intel 686

#define	isT2081		((wg_cpu_model == WG_CPU_T2081))
#define	isT1042		((wg_cpu_model == WG_CPU_T1042))
#define	isT1024		((wg_cpu_model == WG_CPU_T1024))

#define	isLS1046 	((wg_cpu_model == WG_CPU_LS1046))      /* T80 platform */
#define	isLS2088 	((wg_cpu_model == WG_CPU_LS2088))      /* M390 platform */
#define	isLX2160 	((wg_cpu_model == WG_CPU_LX2160))      /* M590/M690 platform */

#define	isTx0xx		((isT2081) || (isT1042) || (isT1024))

#define	isC2xxx		((wg_cpu_model == WG_CPU_686(2758)))
#define	isC3xxx		((wg_cpu_model == WG_CPU_686(3558)) || (wg_cpu_model == WG_CPU_686(3758)))

#define	has88E6176	((isT1024) || (wg_westport))
#define	has88E6190	((isC3xxx) || (isLS1046))
#define	has88E6191X	((isLS2088) || (isLX2160))
#define	has98DX3035	((isC2xxx))

extern	int wg_get_cpu_model(void);	 // Get CPU model

extern	int wg_cpu_model;		 // The CPU model
extern	int wg_cpu_version;		 // The CPU version

#ifdef	CONFIG_WG_ARCH_X86

extern	int wg_seattle;			 // Seattle   model
extern	int wg_kirkland;		 // Kirkland  model
extern	int wg_colfax;			 // Colfax    model
extern	int wg_westport;		 // Westport  model
extern	int wg_winthrop;		 // Winthrop  model
extern  int wg_clarkston;		 // Clarkston model

#else

#define	wg_seattle	0		 // Seattle   model
#define	wg_kirkland	0		 // Kirkland  model
#define	wg_colfax	0		 // Colfax    model
#define	wg_westport	0		 // Westport  model
#define	wg_winthrop	0		 // Winthrop  model
#define	wg_clarkston	0		 // Clarkston model

#endif

#if defined(CONFIG_PPC)
extern	int wg_boren;			 // Boren     model
#else
/*
 * FIXME: No one should refer this.
 *  If being referred, should be in ifdef CONFIG_PPC.
 *  This do not matter if you are here. Someday, redo this mess
 */
#define	wg_boren	0		 // Boren     model
#endif

#ifdef  CONFIG_WG_ARCH_ARM_64
extern  int wg_castlerock;               // Castlerock model
#if defined(CONFIG_WG_PLATFORM_M290_T80) || defined(CONFIG_WG_PLATFORM_M390) || \
	defined(CONFIG_WG_PLATFORM_M590_M690)
	/* It's not recommend to use CONFIG_WG_PLATFORM_CASTLEROCK any more.
	 * The exact meaning for CONFIG_WG_PLATFORM_CASTLEROCK is actually 
	 * CONFIG_WG_PLATFORM_CASTLEROCK_T80 after CL # 645919
	 */
	#define CONFIG_WG_PLATFORM_CASTLEROCK	1
	#define is_m290() (wg_castlerock == 1)
#endif
#endif

#define	wg_dpa_bug	(0)		 // No Freescale DPA on X86 and PPC

#define	SW0_NAME	"sw10"	// Physical switch device name sw10
#define	SW1_NAME	"sw11"	// Physical switch device name sw11

#define	SW0_ADDR	(0x10)	// Physical switch device sw10 MDIO address
#define	SW1_ADDR	(0x11)	// Physical switch device sw11 MDIO address

#define	WG_DSA_NSW	(2)		 // Max switch	we handle
#define	WG_DSA_NETH	(8)		 // Max ethn    we handle
#define	WG_DSA_PHY	(32)		 // Max phys    we handle
#define	WG_DSA_NIF	(WG_DSA_NETH*2)	 // Max ifindex we handle

extern	int wg_nitrox_model;		 // Nitrox    crypto chip model (if any)
extern	int wg_cavecreek_model;		 // Cavecreek crypto chip model (if any)

extern	int wg_talitos_model;		 // Freescale talitos crypto
extern	int wg_caam_model;		 // Freescale caam    crypto

extern	int wg_dsa_type;		 // DSA switch type
extern	int wg_dsa_count;		 // DSA switch chip count
extern	int wg_dsa_debug;		 // DSA switch chip debug flags
extern	int wg_dsa_smi_reg;		 // Base address for SMI commands
extern	int wg_dsa_phy_num;		 // Number of PHYs on switch chip

#ifdef	CONFIG_WG_ARCH_X86
#define	DSA_PHY		wg_dsa_phy_num
#define	DSA_PHY_MAP(x)	((x) + wg_dsa_phy_map[x])
#endif
#ifdef	CONFIG_WG_ARCH_FREESCALE
#define	DSA_PHY		5
#define	DSA_PHY_MAP(x)	((x))
#endif

#ifdef  CONFIG_WG_ARCH_ARM_64
#define DSA_PHY         8
#define DSA_PHY_MAP(x)  ((x))
#endif

#define	DSA_PORT	(DSA_PHY + 2)

extern	s8  wg_dsa_phy_map[WG_DSA_PHY];	 // Map normalized PHYs to actual

#define	MARVELL_HLEN	2		 // Marvell header length

// DSA parent and Host Device
extern	struct	device*	wg_dsa_parent[WG_DSA_NSW];
extern	struct	device*	wg_dsa_host[WG_DSA_NSW];

// Slave device structs indexed by ifindex
// Points to switch device if in EDSA mode
extern	struct	net_device* wg_slave_dev[WG_DSA_NIF];

// Outgoing Marvell tags indexed by ifindex
extern	int		    wg_slave_tag[WG_DSA_NIF];

// The master switch net device that owns each slave indexed by ifindex
extern	struct	net_device* wg_master_dev[WG_DSA_NIF];

// Slave device structs indexed by ifindex
extern	struct	net_device* wg_named_dev[WG_DSA_NIF];

// Pointer to DSA MII PHY Bus
extern	struct	mii_bus*    wg_dsa_bus;

// MDIO bus release function pointer
extern	void (*wg_dsa_mdio_release)(void);

// SGMII link poll function pointer
extern	int  (*wg_dsa_sgmii_poll) (int);
extern	int    wg_ixgbe_sgmii_poll(int);

// Global Mutex for DSA
extern	struct mutex wg_dsa_mutex;

extern	int wg_soft_lockup;		 // Count of CPUs in soft lockup

#ifdef	CONFIG_32BIT
typedef	u32 wg_br_map;
typedef	u32 wg_sl_map;
#define	WG_BR_MAP(x)	(((x) < 32) ? (((wg_br_map)1) << (x)) : 0)
#else
typedef	u64 wg_br_map;
typedef	u64 wg_sl_map;
#define	WG_BR_MAP(x)	(((x) < 64) ? (((wg_br_map)1) << (x)) : 0)
#endif

extern	wg_br_map wg_br_bridged;	 // HW bridging         interfaces
extern	wg_br_map wg_br_primary;	 // HW bridging primary interfaces
extern	wg_br_map wg_br_dropped;	 // HW bridging dropped interfaces
extern	wg_sl_map wg_sl_slaved;		 // Slaved eth  device  interfaces

// Both of these map to a common queue vector

extern	int	  wg_fips_sha;		 // SHA type
extern	int	  wg_fips_sha_err;	 // SHA auth errors
extern	int	  wg_fips_sha_len;	 // SHA key  length
extern	u8*	  wg_fips_sha_key;	 // SHA key  addrees
extern	int	  wg_fips_sha_mode0;	 // SHA QAT  mode  0

extern	int	  wg_fips_iv_len;	 // Deterministic FIPS IV   length
extern	u8*	  wg_fips_iv;		 // Deterministic FIPS IV   address

extern	u8*	  wg_fips_hmac;		 // Deterministic FIPS HMAC address

extern	int	  wg_fips_aad_len;	 // AAD length

extern	int	  fips_status;		 // FIPS status variables
extern	int	  fips_hw_status;

#ifdef	CONFIG_CRYPTO_FIPS
extern	int	  fips_enabled;
#endif

// Get the  current PC
extern	void* wg_pc(void);

// Do a mutex lock with timeout
extern	int	 wg_mutex_lock(void* lock, int timeout);

#define	PSS_HLEN	4	   // PSS header tag size

#define	PSS_TAG_HI	0x8E	   // Special tag 0x8E8E
#define	PSS_TAG_LO	0x8E

#define	ETH_P_PSS	((PSS_TAG_HI << 8) | PSS_TAG_LO)

struct	sk_buff;

extern	int  (*wg_pss_untag)(struct sk_buff*, __u8*);

struct	pt_regs;

extern	int    register_timer_hook(int (*hook)(struct pt_regs*));
extern	void unregister_timer_hook(int (*hook)(struct pt_regs*));

// Define deprecated __dev values for older drivers
#define	__devinit
#define	__devinitdata
#define	__devexit
#define	__devexit_p(x)	(x)

#define	VM_RESERVED	(0)

struct	wg_counter	{
	struct	list_head list;
	char		  name[16];
	atomic_t	  count;
};

#define	WG_INC_COUNT(x)	    do { extern struct wg_counter* x; if (likely(x)) atomic_inc(&(x)->count); } while(0)
#define	WG_DEC_COUNT(x)	    do { extern struct wg_counter* x; if (likely(x)) atomic_dec(&(x)->count); } while(0)

extern	struct	wg_counter* wg_add_counter(char*);
extern	void                wg_del_counter(struct wg_counter**);

extern	const	char	linux_banner[];
extern	const	char	linux_proc_banner[];

extern	int	wg_rootdev_mounted;

extern	void	wg_check_rootdev(char*, u32);
extern	int	wg_fix_xen_rootdev(char*);

extern	void	wg_check_smp_cpus(long);

#define	DSA_MAX_SWITCHES	4
#define	DSA_MAX_PORTS		12

#define	EDSA_HLEN	8	 // Length of EDSA header from tag_edsa.c
#define	EDSA_FLAG	8	 // Flag for  EDSA header mode
#define	MARVELL_HLEN	2	 // Length of Marvell header mode tag
#define	MARVELL_FLAG	2	 // Flag for  Marvell header mdde

extern	struct mii_bus*    wg_dsa_bus;
extern	struct mii_bus*    wg_pss_bus;

extern	struct dsa_switch* wg_dsa_sw[DSA_MAX_SWITCHES];

extern	int		   wg_dsa_L2_offset[DSA_MAX_SWITCHES];

extern	u16	wg_dsa_sw0_map[DSA_MAX_PORTS];
extern	u16	wg_dsa_sw1_map[DSA_MAX_PORTS];
extern	u16	wg_dsa_sw0_out[DSA_MAX_PORTS];
extern	u16	wg_dsa_sw1_out[DSA_MAX_PORTS];

extern	int	wg_dsa_flood;
extern	int	wg_dsa_learning;
extern	int	wg_dsa_lock_static;
extern	int	wg_dsa_type;

#define	CONFIG_WG_PLATFORM_OLD_TIMER_HOOK	1

extern int (*wg_timer_hook)(struct pt_regs *);

#define	CONFIG_WG_PLATFORM_OLD_PROC_API		1

struct	file;
struct	proc_dir_entry;

typedef	int (read_proc_t)(char *page, char **start, off_t off,
			  int count, int *eof, void *data);
typedef	int (write_proc_t)(struct file *file, const char __user *buffer,
			   unsigned long count, void *data);

extern	int	proc_wg_init(void);

extern	struct	proc_dir_entry *proc_wg_root(void);
extern	struct	proc_dir_entry *create_proc_entry(const char *name, umode_t mode,
						  struct proc_dir_entry *parent);

extern	void	set_proc_read( struct proc_dir_entry *entry, read_proc_t*  func);
extern	void	set_proc_write(struct proc_dir_entry *entry, write_proc_t* func);

extern	int	wg_xfrm_bypass; // Bypass flag for ESP packets

extern	int	wg_debug_flag;	// Generic kernel debug flag

static inline unsigned compare_ether_addr(const u8 *addr1, const u8 *addr2)
{
	const u16 *a = (const u16 *) addr1;
	const u16 *b = (const u16 *) addr2;

	return ((a[0] ^ b[0]) | (a[1] ^ b[1]) | (a[2] ^ b[2])) != 0;
}

#define SET_ETHTOOL_OPS(netdev,ops) ( (netdev)->ethtool_ops = (ops) )

struct genl_family;
struct genl_multicast_group;

#define	WG_DUMP			(0x40000000)

#define	WG_TRACE		wg_trace_dump(__FUNCTION__, __LINE__,  0,      0);
#define	WG_TRACE_DUMP		wg_trace_dump(__FUNCTION__, __LINE__, WG_DUMP, 0);
#define	WG_TRACE_CODE(x)	wg_trace_dump(__FUNCTION__, __LINE__,  0,     (x));
#define	WG_TRACE_AT(x)		wg_trace_dump(__FUNCTION__, __LINE__, (x),     0);

#define	WG_TRACE_SGL(s,t)	wg_dump_sgl(__FUNCTION__, __LINE__,  0,         (s), (t));
#define	WG_TRACE_SGL_DUMP(s,t)	wg_dump_sgl(__FUNCTION__, __LINE__, 16|WG_DUMP, (s), (t));

#define	CONFIG_WG_PLATFORM_TRACE	1

#ifdef	CONFIG_WG_PLATFORM_TRACE

struct	scatterlist;

extern	int	wg_trace_dump(const char*, int, int, int);
extern	void	wg_dump_hex(const u8*, u32, const u8*);
extern	void	wg_dump_sgl(const char*, int, int, struct scatterlist*, const char*);

#else

#define	wg_trace_dump(func, line, flag, code)
#define	wg_dump_hex(buf, len, tag)
#define	wg_dump_sgl(func, line, flag, sgl, tag)

#endif

#define	WG_SAMPLE_RETURN_PC(x)	wg_sample_pc((unsigned long)__builtin_return_address(x))

extern	void	wg_sample_nop(unsigned long);
extern	void  (*wg_sample_pc)(unsigned long);

#ifdef	CONFIG_WG_ARCH_X86
struct	cpuinfo_x86;
extern	void	wg_kernel_get_x86_model(struct cpuinfo_x86*);
#endif

extern	int    wg_crash_memory;

// From 3.12.19

static inline int SENSORS_LIMIT(long value, long low, long high)
{
	if (value < low)
		return low;
	else if (value > high)
		return high;
	else
		return value;
}

#ifdef	CONFIG_WG_PLATFORM_DEV_REF_DEBUG // WG:JB Track device ref counts
extern	void wg_dev_get(struct net_device*);
extern	void wg_dev_put(struct net_device*);
#endif

#if defined(CONFIG_WG_ARCH_FREESCALE) || defined(CONFIG_WG_ARCH_ARM_64) // WG:JB Return Linux errors

#define	linux_error(x) (((x & 0xF00000FF) == 0x4000001C) ? -ETIMEDOUT : -EIO)
#define	LINUX_ERROR(x) (unlikely(x) ? linux_error(x) : 0)

//#ifdef	CONFIG_WG_PLATFORM_DSA_MODULE // WG:JB Lookup slave device
struct	dpa_priv_s;

extern	int wg_dpaa_dsa_type_trans(struct sk_buff*,
                                   struct net_device*,
                                   const struct dpa_priv_s*);
//#endif	// CONFIG_WG_PLATFORM_DSA_MODULE
#endif

#define	f_dentry	f_path.dentry

#define	net_random()	prandom_u32()

#define	strict_strtoul	kstrtoul
#define	strict_strtol	kstrtol
#define	strict_strtoull	kstrtoull
#define	strict_strtoll	kstrtoll

extern	int wg_seq_file_timeout; // Mutex timeout

extern	int wg_br_xmit(struct sk_buff*);

#ifdef	CONFIG_WG_KERNEL_4_14

struct	net;
struct	sock;

extern	int wg_br_xmit_netsk(struct net*, struct sock*, struct sk_buff*);

#define	wg_ifmark		wg.ifmark

// Fixes for 4.14.x needed by Intel out of tree Ethernet drivers

#define	LL_FLUSH_FAILED		-1
#define	LL_FLUSH_BUSY		-2

#define	NETIF_F_ALL_CSUM	 0

// WG:JB For Cavecreek
struct	pci_dev;
struct	msix_entry;
extern	int pci_enable_msix(struct pci_dev*, struct msix_entry*, int);

typedef	u64 cycle_t;

// Fixes for 4.14.x needed for MDIO and PHY devices

#define	wg_mdio_addr	mdio.addr
#define	wg_mdio_phy(p)	mdiobus_get_phy(wg_pss_bus, (p))

#else

#define	wg_mdio_addr	addr
#define	wg_mdio_phy(p)	(wg_pss_bus->phy_map[(p)])

#endif

#endif	// _LINUX_WG_KERNEL_H
