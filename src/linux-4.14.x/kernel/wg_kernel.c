#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kallsyms.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/netdevice.h>
#include <linux/scatterlist.h>

#include <net/dsa.h>

#include <asm/atomic.h>

static char	wg_cloud[16];	// Cloud environment (if any)

static char*	wg_vm_name = NULL; // VM  name

static int	wg_cpus	= 1;	// CPU count

int	wg_cpu_model;	// CPU model
EXPORT_SYMBOL(wg_cpu_model);

int	wg_cpu_version;	// CPU version
EXPORT_SYMBOL(wg_cpu_version);

int wg_get_cpu_model(void)
{
  return wg_cpu_model;
}
EXPORT_SYMBOL(wg_get_cpu_model);

spinlock_t    wg_fault_lock;	   // Lock for       fault reporter
int	      wg_fault_used;	   // Used bytes in  fault reporter
char	      wg_fault_text[2000]; // Fault reporter error buffer

atomic_t      wg_dma_errors;	   // DMA null errors
EXPORT_SYMBOL(wg_dma_errors);

atomic_long_t wg_drop_backlog;	   // Drop due to backlog queue full
EXPORT_SYMBOL(wg_drop_backlog);

atomic_long_t wg_drop_length;	   // Drop due to length error
EXPORT_SYMBOL(wg_drop_length);

atomic_long_t wg_drop_unknown;	   // Drop due to unknown type
EXPORT_SYMBOL(wg_drop_unknown);

int	      wg_backlog_high;	   // Backlog queue high    water mark
EXPORT_SYMBOL(wg_backlog_high);

int	      wg_backlog_highest;  // Backlog queue highest water mark ever
EXPORT_SYMBOL(wg_backlog_highest);

int	      wg_fips_sha;	   // FIPS SHA type
EXPORT_SYMBOL(wg_fips_sha);

int	      wg_fips_sha_err;	   // FIPS SHA auth errors
EXPORT_SYMBOL(wg_fips_sha_err);

int	      wg_fips_sha_len;	   // FIPS SHA key  length
EXPORT_SYMBOL(wg_fips_sha_len);

u8*	      wg_fips_sha_key;	   // FIPS SHA key  address
EXPORT_SYMBOL(wg_fips_sha_key);

int	      wg_fips_sha_mode0;   // FIPS SHA QAT  mode  0
EXPORT_SYMBOL(wg_fips_sha_mode0);

int	      wg_fips_iv_len;	   // FIPS deterministic IV   length
EXPORT_SYMBOL(wg_fips_iv_len);

u8*	      wg_fips_iv;	   // FIPS deterministic IV   address
EXPORT_SYMBOL(wg_fips_iv);

u8*	      wg_fips_hmac;	   // FIPS deterministic HMAC address
EXPORT_SYMBOL(wg_fips_hmac);

int	      wg_fips_aad_len;	   // FIPS AAD length
EXPORT_SYMBOL(wg_fips_aad_len);

int	      fips_status;	   // FIPS status for WG crypto
EXPORT_SYMBOL(fips_status);

int	      fips_hw_status;
EXPORT_SYMBOL(fips_hw_status);

// NOOP function pointer
void	      wg_noop(void)    {}
EXPORT_SYMBOL(wg_noop);

#ifdef	CONFIG_X86

#include <asm/cpufeature.h>
#include <asm/processor.h>
#include <asm/hypervisor.h>

int wg_sysb_mode;
EXPORT_SYMBOL(wg_sysb_mode);
static int is_sysb(const char* cmdline)
{
	int sysb = 0;

	if (strstr(cmdline, "root=/dev/ram0") ||
		strstr(cmdline, "wgmode=recovery"))
		sysb = 1;

	return sysb;
}

char* wg_get_vm_name(void)
{
#ifdef	CONFIG_HYPERVISOR_GUEST
  if (wg_vm_name == NULL) {
    /*
     * Do nothing.  wg_vm_name should be primed
     * to "xtmv" for virtual platforms, by wg_setup_arch() 
     * based on wg_ptfm=xtmv in boot command line.
     */ ;
  } else
  if (hypervisor_is_type(X86_HYPER_VMWARE)) {
    wg_vm_name = "VMware";
  } else
  if (hypervisor_is_type(X86_HYPER_MS_HYPERV)) {
    wg_vm_name = "Microsoft HyperV";
  } else
  if (hypervisor_is_type(X86_HYPER_XEN_PV)) {
    wg_vm_name = "Xen PV";
  } else
  if (hypervisor_is_type(X86_HYPER_XEN_HVM)) {
    wg_vm_name = "Xen HVM";
  } else
  if (hypervisor_is_type(X86_HYPER_KVM)) {
    wg_vm_name = "KVM";
  } else 
  if (hypervisor_is_type(X86_HYPER_NATIVE)) {
    wg_vm_name = "Native";
  }
#endif
  return wg_vm_name;
}
EXPORT_SYMBOL(wg_get_vm_name);

int  wg_nitrox_model;		// Nitrox    crypto chip model (if any)
EXPORT_SYMBOL(wg_nitrox_model);

int  wg_cavecreek_model;	// Cavecreek crypto chip model (if any)
EXPORT_SYMBOL(wg_cavecreek_model);

struct akcipher_alg;

#else // In WG scheme of things, this applies to all non-x86 WG platforms (powerpc/ppc_64 and arm64)

char* wg_get_vm_name(void)
{
  // Freescale don't have VMs
  return 0;
}
EXPORT_SYMBOL(wg_get_vm_name);

#endif

int	      wg_dsa_count;		// DSA switch chip count
EXPORT_SYMBOL(wg_dsa_count);

int	      wg_dsa_debug;		// DSA switch chip debug flags
EXPORT_SYMBOL(wg_dsa_debug);

#if defined(CONFIG_WG_PLATFORM_DSA_MODULE) || defined (CONFIG_WG_PLATFORM_DSA)

// Slave device structs indexed by ifindex
// Points to switch device if in EDSA mode
struct	net_device* wg_slave_dev[WG_DSA_NIF];
EXPORT_SYMBOL(      wg_slave_dev);

// Outgoing Marvell tags indexed by ifindex
int		    wg_slave_tag[WG_DSA_NIF];
EXPORT_SYMBOL(      wg_slave_tag);

// The master switch net device that owns each slave indexed by ifindex
struct	net_device* wg_master_dev[WG_DSA_NIF];
EXPORT_SYMBOL(      wg_master_dev);

// Slave device structs indexed by ifindex
struct	net_device* wg_named_dev[WG_DSA_NIF];
EXPORT_SYMBOL(      wg_named_dev);

// L2 offset for each Ethernet port connected to a switch
int		    wg_dsa_L2_offset[DSA_MAX_SWITCHES];
EXPORT_SYMBOL(      wg_dsa_L2_offset);

#endif	// WG_PLATFORM_DSA_MODULE || WG_PLATFORM_DSA

// Type    of DSA         Device
int	      wg_dsa_type;
EXPORT_SYMBOL(wg_dsa_type);

// Pointer to DSA Parent  Device
struct device*	   wg_dsa_parent[WG_DSA_NSW];
EXPORT_SYMBOL	  (wg_dsa_parent);

// Pointer to DSA Host    Device
struct device*	   wg_dsa_host[WG_DSA_NSW];
EXPORT_SYMBOL	  (wg_dsa_host);

// Normalized to actual PHY map
s8		   wg_dsa_phy_map[WG_DSA_PHY];
EXPORT_SYMBOL     (wg_dsa_phy_map);

// Pointer to DSA ETH SW  Device
struct net_device* wg_dsa_dev[WG_DSA_NSW];
EXPORT_SYMBOL	  (wg_dsa_dev);

// Pointer to DSA MII PHY Bus
struct mii_bus*	   wg_dsa_bus;
EXPORT_SYMBOL	  (wg_dsa_bus);

#ifdef	CONFIG_X86
// Pointer to PSS MII PHY Bus
struct mii_bus*    wg_pss_bus;
EXPORT_SYMBOL	  (wg_pss_bus);

// Pointer to PSS untag
int		 (*wg_pss_untag)(struct sk_buff*, __u8*);
EXPORT_SYMBOL_GPL (wg_pss_untag);
#endif

// MDIO bus release function pointer
void        (*wg_dsa_mdio_release)(void) = NULL;
EXPORT_SYMBOL(wg_dsa_mdio_release);

// SGMII link poll function pointer
int         (*wg_dsa_sgmii_poll)(int)  = NULL;
EXPORT_SYMBOL(wg_dsa_sgmii_poll);

// Global mutex for DSA
DEFINE_MUTEX (wg_dsa_mutex);
EXPORT_SYMBOL(wg_dsa_mutex);

#if defined(CONFIG_PPC) || defined(CONFIG_ARM64)

#include <linux/of_platform.h>

#ifdef CONFIG_PPC
int	wg_boren;		// Boren  model # if any
EXPORT_SYMBOL(wg_boren);
#endif

/* For CONFIG_ARM64 */
int	wg_castlerock = -1;	// Castlerock models => 1: M290, 2:M390, 3:M590/M690, other value: N/A
EXPORT_SYMBOL(wg_castlerock);

int	wg_talitos_model;	// Freescale talitos crypto
EXPORT_SYMBOL(wg_talitos_model);

int	wg_caam_model;		// Freescale caam    crypto
EXPORT_SYMBOL(wg_caam_model);

#ifdef	CONFIG_PPC64 // WG:JB Dynamic flags for Freescale errata

int FSL_ERRATUM_A_005127 =  0;
EXPORT_SYMBOL(FSL_ERRATUM_A_005127);

int FSL_ERRATUM_A_005337 =  0;
EXPORT_SYMBOL(FSL_ERRATUM_A_005337);

// This is always on now so not need for dynamic flag

// int FSL_ERRATUM_A_006184 = 43;
// EXPORT_SYMBOL(FSL_ERRATUM_A_006184);

// These are not currently used

// int FSL_ERRATUM_A_006198 =  0;
// EXPORT_SYMBOL(FSL_ERRATUM_A_006198);

// int FSL_ERRATUM_A_008007 =  0;
// EXPORT_SYMBOL(FSL_ERRATUM_A_008007);

#endif	// CONFIG_PPC64

#endif /* defined(CONFIG_PPC) || defined(CONFIG_ARM64) */

void __wg_dsa_set_L2_off(struct net_device* sw_dev, int n)
{

}

void (*wg_dsa_set_L2_off)(struct net_device*, int) = __wg_dsa_set_L2_off;
EXPORT_SYMBOL(wg_dsa_set_L2_off);

int	wg_crash_memory;	// Min crash memory for kdump
EXPORT_SYMBOL(wg_crash_memory);

// Return where the PC is now for debugging purposes
void* wg_pc(void)
{
  return (void*)_RET_IP_;
}
EXPORT_SYMBOL(wg_pc);

// Add to error text to fault report
int   wg_fault_report(char* err)
{
  int rc = -ENOSPC;
  int z  = strlen(err);

  printk(KERN_EMERG "%s\n", err);

  spin_lock_bh  (&wg_fault_lock);

  if ((wg_fault_used + z) < (sizeof(wg_fault_text)-2)) {

    strcpy(&wg_fault_text[wg_fault_used], err);
    wg_fault_used += z;

    if (wg_fault_text[wg_fault_used-1] != '\n')
        wg_fault_text[wg_fault_used++] =  '\n';

    wg_fault_text[wg_fault_used] = 0;

    rc = wg_fault_used;
  }

  spin_unlock_bh(&wg_fault_lock);

  return rc;
}
EXPORT_SYMBOL(wg_fault_report);

#ifdef	CONFIG_WG_PLATFORM_LOCK

// Wait timeout free
#define	WG_LOCK_FREE		2

// Locks debugging flag
int wg_lock_debug;
EXPORT_SYMBOL(wg_lock_debug);

// Wait timeout mask
unsigned wg_lock_warn   = 0x07FFFFFF;	// Logs approx every 3 seconds
EXPORT_SYMBOL(wg_lock_warn);

// Wait timeout fail point
unsigned wg_lock_fail   = 0x20000000;	// Fail is 4 times the warn time
EXPORT_SYMBOL(wg_lock_fail);

// Locks debugging not locked max
unsigned wg_lock_nl_max = 8;
EXPORT_SYMBOL(wg_lock_nl_max);

// Locks debugging free  lock max
unsigned wg_lock_lk_max = 8;
EXPORT_SYMBOL(wg_lock_lk_max);

// List of locks we are debugging
LIST_HEAD(wg_locks);
EXPORT_SYMBOL(wg_locks);

// Number of debug items we keep
#define	NITEMS	256

//  Struct for printing out lock exceptions
struct wg_debug_item {
  const char*	what;
  void*		lock;
  void*		from;
  void*		last;
  unsigned	state;
  u64		stamp;
} wg_lock_debug_items[NITEMS];

// Reader/Writer pointers in the debug item ring
atomic_t wg_lock_debug_rd = ATOMIC_INIT(0);
atomic_t wg_lock_debug_wr = ATOMIC_INIT(0);

// Lock error reporting
void wg_lock_error(const char* what, wglock_t* lock, unsigned state,
		   unsigned long from, void* last)
{
  int j = atomic_get(&wg_lock_debug_wr) & (NITEMS-1);

  if (strstr(what, "_lock_stuck"))
    wg_lock_debug_enable(&lock->list);
  else
  if (strstr(what, "_lock_reset"))
    wg_lock_debug_enable(&lock->list);

  wg_lock_debug_items[j].what	= what;
  wg_lock_debug_items[j].lock	= lock;
  wg_lock_debug_items[j].from	= (void*)from;
  wg_lock_debug_items[j].last	= last;
  wg_lock_debug_items[j].state	= state;
  wg_lock_debug_items[j].stamp	= cpu_clock(smp_processor_id());

  atomic_inc(&wg_lock_debug_wr);

  if (wg_lock_debug & WG_LOCK_SD_DBG) dump_stack();
}

// Print lock exceptions
void wg_lock_debug_print(struct seq_file *m)
{
  struct wg_debug_item* Q;

  while ((atomic_get(&wg_lock_debug_wr) - atomic_get(&wg_lock_debug_rd)) > 0) {

    Q = &wg_lock_debug_items[atomic_get(&wg_lock_debug_rd) & (NITEMS-1)];

    if (Q->what) {
      int z = 0;
      char buf[KSYM_SYMBOL_LEN*4];

      z += sprintf(&buf[z], "%s:",                  Q->what);
      z += sprintf(&buf[z], " lock [%p] %8x ",      Q->lock, Q->state);
      z += sprint_symbol(&buf[z],    (unsigned long)Q->lock);

      if (Q->from) {
	z += sprintf(&buf[z], " from [%p] ", (void*)Q->from);
	z += sprint_symbol(&buf[z],  (unsigned long)Q->from);
      }

      if (Q->last) {
	z += sprintf(&buf[z], " last [%p] ",        Q->last);
	z += sprint_symbol(&buf[z],  (unsigned long)Q->last);
      }

      if (z > (buf[z] = 0)) {
	u64 t = Q->stamp;
	u32 r = do_div(t, 1000000000);
	seq_printf(m, "[%5u.%06u] %s\n", (u32)t, r / 1000, buf);
      }

      Q->what = NULL;
    }

    atomic_inc(&wg_lock_debug_rd);
  }
}
EXPORT_SYMBOL(wg_lock_debug_print);

// Clear debug info on lock
void wg_lock_debug_clear(wglock_t* lock)
{
  unsigned state = atomic_get(&lock->state);
  memset(&lock->ctx, 0, sizeof(struct wg_lock_ctx));
  memset(&lock->wr,  0, sizeof(struct wg_lock_data));
  if (state & WG_LOCK_RD_MASK)
  memset(&lock->rd,  0, sizeof(struct wg_lock_data));
}
EXPORT_SYMBOL(wg_lock_debug_clear);

// Remove lock being debuged from wg_locks
void wg_lock_debug_remove(struct wg_list_item* entry)
{
  struct list_head* item;

  if (!entry) return;

  item = entry->item;
  entry->item = NULL;
  list_del(&entry->list);
  kfree(entry);

  if (item) {
    if ((item->prev == &entry->list) && (item->next == &entry->list)) {
      item->prev = NULL;
      item->next = NULL;
    } else
      printk(KERN_INFO "%s: Lock [%p] Item [%p] Next [%p] Prev [%p] abandoned\n",
	     __FUNCTION__, container_of(item, wglock_t, list),
	     entry, item->next, item->prev);
  }
}
EXPORT_SYMBOL(wg_lock_debug_remove);

// Enable  debugging on lock, insert into debug list
void wg_lock_debug_enable(struct list_head* list)
{
  if (!list) return;

  if (!(list->prev)) {
    struct wg_list_item* this = kmalloc(sizeof(struct wg_list_item), GFP_ATOMIC);
    if (this) {
      if (000000)
      printk(KERN_DEBUG "%s: Lock [%p] Item [%p] enabled\n",
	     __FUNCTION__, container_of(list, wglock_t, list), this);
      list_add(&this->list, &wg_locks);
      this->item = list;
      list->next = &this->list;
      list->prev = &this->list;
    }
  }
}
EXPORT_SYMBOL(wg_lock_debug_enable);

// Disable debugging on lock, remove from debug list
void wg_lock_debug_disable(struct list_head* list)
{
  if (list)
    wg_lock_debug_remove((struct wg_list_item*)(list->prev));
}
EXPORT_SYMBOL(wg_lock_debug_disable);

// Check for read lock timeouts
unsigned wg_read_lock_wait(rwlock_t* lock, unsigned wait, unsigned long pc)
{
  // Check for debug
  if (likely(wg_lock_debug & WG_LOCK_RD_DBG) == 0) {
    // Didn't get it so backup ref count
    atomic_dec(&lock->wg_lock.state);
    return 1;
  }

  {
    unsigned state;
    wglock_t* lckP = &lock->wg_lock;

    // Bump spin time
    atomic_inc(&lckP->rd.spin);

    // Look for excessive wait times
    if (unlikely(((++wait) & wg_lock_warn) == 0)) {
      atomic_inc(&lckP->rd.stuck);
      wg_lock_error("read_lock_stuck", lckP, atomic_get(&lckP->state), pc, lckP->rd.grab);
    }

    // Record high water wait time
    if (unlikely(lckP->rd.high < wait)) lckP->rd.high = wait;

    // Didn't get it so backup ref count
    if ((state = atomic_dec_return(&lckP->state)) >= WG_LOCK_OWNER)

    // Adjust bad lock count
    if (unlikely(wait >= WG_LOCK_FREE))
    if (unlikely(do_raw_write_trylock(lock))) {
      if (atomic_get(&lckP->state) != WG_LOCK_FREED)
	  atomic_set(&lckP->state,    WG_LOCK_FREED); else pc = 0;
      do_raw_write_unlock(lock);
      if (unlikely(atomic_bump(&lckP->rd.reset) <= wg_lock_lk_max))
      if (unlikely(wg_lock_debug & WG_LOCK_LK_DBG))
      if (unlikely(pc))
	wg_lock_error("read_lock_reset", (wglock_t*)lckP, state, pc, lckP->wr.grab);
    }
  }

  // Return wait time
  return (wait != wg_lock_fail) ? wait : 0;
}
EXPORT_SYMBOL(wg_read_lock_wait);

// Report read lock not locked errors
void wg_read_lock_not_locked(rwlock_t* lock, void* toss, unsigned long pc)
{
  wglock_t* lckP = &lock->wg_lock;
  unsigned state = atomic_get(&lckP->state);

  // Ignore spurious unlocks
  atomic_inc(&lckP->state);

  if (unlikely(wg_lock_debug & WG_LOCK_RD_DBG))
  if (unlikely(atomic_bump(&lckP->rd.under) <= wg_lock_nl_max))
  if (unlikely(wg_lock_debug & WG_LOCK_NL_DBG))
    wg_lock_error("read_lock_under", lckP, state, pc, toss);
}
EXPORT_SYMBOL(wg_read_lock_not_locked);

// Check for write lock timeouts
unsigned wg_write_lock_wait(rwlock_t* lock, unsigned wait, unsigned long pc)
{
  // Check for debug
  if (likely(wg_lock_debug & WG_LOCK_WR_DBG) == 0) {
    // Didn't get it so backup ref count
    atomic_dec(&lock->wg_lock.state);
    return 1;
  }

  {
    unsigned state;
    wglock_t* lckP = &lock->wg_lock;

    // Bump spin time
    atomic_inc(&lckP->wr.spin);

    // Look for excessive wait times
    if (unlikely(((++wait) & wg_lock_warn) == 0)) {
      atomic_inc(&lckP->wr.stuck);
      wg_lock_error("write_lock_stuck", lckP, atomic_get(&lckP->state), pc, lckP->wr.grab);
    }

    // Record high water wait time
    if (unlikely(lckP->wr.high < wait)) lckP->wr.high = wait;

    // Didn't get it so backup ref count
    if ((state = atomic_dec_return(&lckP->state)) >= WG_LOCK_OWNER)

    // Adjust bad lock count
    if (unlikely(wait >= WG_LOCK_FREE))
    if (unlikely(do_raw_write_trylock(lock))) {
      if (atomic_get(&lckP->state) != WG_LOCK_FREED)
	  atomic_set(&lckP->state,    WG_LOCK_FREED); else pc = 0;
      do_raw_write_unlock(lock);
      if (unlikely(atomic_bump(&lckP->wr.reset) <= wg_lock_lk_max))
      if (unlikely(wg_lock_debug & WG_LOCK_LK_DBG))
      if (unlikely(pc))
	wg_lock_error("write_lock_reset", (wglock_t*)lckP, state, pc, lckP->wr.grab);
    }
  }

  // Return wait time
  return (wait != wg_lock_fail) ? wait : 0;
}
EXPORT_SYMBOL(wg_write_lock_wait);

// Report write lock not locked errors
void wg_write_lock_not_locked(rwlock_t* lock, void* toss, unsigned long pc)
{
  wglock_t* lckP = &lock->wg_lock;
  unsigned state = atomic_get(&lckP->state);

  // Reset spurious unlocks
  if (do_raw_write_trylock(lock)) {
    atomic_set(&lckP->state, WG_LOCK_FREED);
    do_raw_write_unlock(lock);
  }

  if (unlikely(wg_lock_debug & WG_LOCK_WR_DBG))
    if (unlikely(atomic_bump(&lckP->wr.under) <= wg_lock_nl_max))
  if (unlikely(wg_lock_debug & WG_LOCK_NL_DBG))
    wg_lock_error("write_lock_under", lckP, state, pc, toss);
}
EXPORT_SYMBOL(wg_write_lock_not_locked);

// Check for spin lock timeouts
unsigned wg_spin_lock_wait(raw_spinlock_t* lock, unsigned wait, unsigned long pc)
{
  // Check for debug
  if (likely(wg_lock_debug & WG_LOCK_SP_DBG) == 0) {
    // Didn't get it so backup ref count
    atomic_dec(&lock->wg_spin.state);
    return 1;
  }

  {
    unsigned state;
    wgspin_t* lckP = &lock->wg_spin;

    // Bump spin time
    atomic_inc(&lckP->wr.spin);

    // Look for excessive wait times
    if (unlikely(((++wait) & wg_lock_warn) == 0)) {
      atomic_inc(&lckP->wr.stuck);
      wg_lock_error("spin_lock_stuck", (wglock_t*)lckP, atomic_get(&lckP->state), pc, lckP->wr.grab);
    }

    // Record high water wait time
    if (unlikely(lckP->wr.high < wait)) lckP->wr.high = wait;

    // Didn't get it so backup ref count
    if ((state = atomic_dec_return(&lckP->state)) != WG_SPIN_FREED)

    // Adjust bad lock count
    if (unlikely(wait >= WG_LOCK_FREE))
    if (unlikely(do_raw_spin_trylock(lock))) {
      if (atomic_get(&lckP->state) != WG_SPIN_FREED)
	  atomic_set(&lckP->state,    WG_SPIN_FREED); else pc = 0;
      do_raw_spin_unlock(lock);
      if (unlikely(atomic_bump(&lckP->wr.reset) <= wg_lock_lk_max))
      if (unlikely(wg_lock_debug & WG_LOCK_LK_DBG))
      if (unlikely(pc))
	wg_lock_error("spin_lock_reset", (wglock_t*)lckP, state, pc, lckP->wr.grab);
    }
  }

  // Return wait time
  return (wait != wg_lock_fail) ? wait : 0;
}
EXPORT_SYMBOL(wg_spin_lock_wait);

// Report spin lock not locked errors
void wg_spin_lock_not_locked(raw_spinlock_t* lock, void* toss, unsigned long pc)
{
  wgspin_t* lckP = &lock->wg_spin;
  unsigned state = atomic_get(&lckP->state);

  // Reset spurious unlocks
  if (do_raw_spin_trylock(lock)) {
    atomic_set(&lckP->state, WG_SPIN_FREED);
    do_raw_spin_unlock(lock);
  }

  if (unlikely(wg_lock_debug & WG_LOCK_SP_DBG))
    if (unlikely(atomic_bump(&lckP->wr.under) <= wg_lock_nl_max))
  if (unlikely(wg_lock_debug & WG_LOCK_NL_DBG))
    wg_lock_error("spin_lock_under", (wglock_t*)lckP, state, pc, toss);
}
EXPORT_SYMBOL(wg_spin_lock_not_locked);

#endif	// CONFIG_WG_PLATFORM_LOCK

// Stuck mutex reporting
void wg_mutex_error(const char* what, struct mutex* lock, unsigned long from)
{
  char sym[KSYM_SYMBOL_LEN];

  sprint_symbol(sym, (unsigned long)lock);

  sprint_symbol(sym, from);
  printk(KERN_EMERG  "%s: called from [%p] %s\n", what, (void*)from, sym);

  printk(KERN_EMERG  "%s: task  pid %6d state %4ld  %s\n",
	 what, current->pid, current->state, current->comm);
#if defined(CONFIG_DEBUG_MUTEXES) || defined(CONFIG_SMP)
#endif
}

/*
 * wg_mutex_lock - acquire a mutex
 * @lock:    the mutex to be acquired
 * @timeout: the timeout period in jiffies
 *
 * Lock the mutex exclusively for this task. If the mutex is not
 * available right now, it will sleep for the timeout specified.
 *
 */

int wg_mutex_lock(void* lock, int timeout)
{
  if (likely(timeout == 0)) {
    // Zero timeout means do the old behavior

    mutex_lock(lock);
    return 1;

  } else {
    // Non-zero timeout so use trylock

    int ret;
    int finish = jiffies + (timeout);	   // Finish time
    int report = jiffies + (timeout >> 2); // Report time

    // Try to lock the mutex, loop on failure
    while (!(ret = mutex_trylock(lock))) {

      // See if time to abort
      if (unlikely((jiffies - finish) > 0)) {
	wg_mutex_error(__FUNCTION__, (struct mutex*)lock, _RET_IP_);
	printk(KERN_EMERG "%s: abort mutex [%p]\n", __FUNCTION__, lock);
	return -EWOULDBLOCK;
      }

      // See if time for a report
      if (unlikely((jiffies - report) > 0)) {
	wg_mutex_error(__FUNCTION__, (struct mutex*)lock, _RET_IP_);
	report = jiffies + (timeout >> 2);
      }

      // Go to sleep
      set_current_state(TASK_INTERRUPTIBLE);
      schedule_timeout(1);
    }

    // Return trylock code
    return ret;
  }
}
EXPORT_SYMBOL(wg_mutex_lock);

#include <linux/root_dev.h>

int	wg_rootdev_mounted = 0;

void wg_init_check_rootdev(char* dev_name, u32 mb);

void wg_check_rootdev(char* dev_name, u32 mb)
{
#ifdef	CONFIG_WG_ARCH_X86 // WG:JB
	if (wg_rootdev_mounted == 0)
		wg_init_check_rootdev(dev_name, mb);
	else if (0)
		printk(KERN_DEBUG "ROOT already mounted on %u:%u\n",
		       MAJOR(ROOT_DEV), MINOR(ROOT_DEV));
#endif
}

/* 
 * We do not provide correct boot line in bootloader
 * config file for our virtual hosts.  Therefore we
 * must fix it up here.
 *
 * rootdev from bootloader config file starts with
 * /dev/hd
 * followed by
 * a or b
 * a number probably 2
 * rootdev expected to be a buffer allowing for one extra byte.
 *
 * Return -- 0 iff rootdev not changed
 */
int __init wg_fix_xen_rootdev(char* rootdev)
{
  int rc = 0; 
#ifdef	CONFIG_WG_ARCH_X86 // WG:JB Change device for Xen
#ifdef	CONFIG_HYPERVISOR_GUEST
  char* vmname = NULL;
  char* newdev = NULL;
  char disk;
  char part;

  vmname = wg_get_vm_name();
  disk = rootdev[7];
  part = rootdev[8];
  if (disk != 'a' && disk != 'b') {
    disk = rootdev[8];
    part = rootdev[9];
  }

  if (vmname == NULL || (disk != 'a' && disk != 'b')) {
    ; /* Not virtual, do nothing */
  } else
  if (strncmp(vmname, "Xen HVM", 7) == 0) {
    newdev = "xvd";
  } else 
  if (strncmp(vmname, "Xen PV", 6) == 0) {
    newdev = "sd";
  } else
  if (strncmp(vmname, "VMware", 6) == 0) {
    newdev = "sd";
  } else
  if (strncmp(vmname, "Microsoft HyperV", 16) == 0) {
    newdev = "sd";
  } else
  if (strncmp(vmname, "KVM", 3) == 0) {
    if (wg_cloud[0]) {	// AWS T3
      newdev = "nvme0n1";
      disk = 'p';
    } else			// FireboxV
      newdev = "sd";
  } else
  if (strncmp(vmname, "Native", 6) == 0) {
    newdev = "sd";
  }

  if (newdev != NULL) {
    sprintf(rootdev, "/dev/%s%c%c", newdev, disk, part); 
    printk(KERN_INFO "ROOT device now %s\n", rootdev);
    rc = 1;
  }
#endif
#endif	// CONFIG_WG_ARCH_X86

return rc;
}

void __init wg_check_smp_cpus(long value)
{
  char err[256];
  volatile long unsigned int flags = 0;

  test_and_set_bit(0, &flags);
  if (flags != value) {
    printk(KERN_EMERG "%s: %lx set   bit not working\n", __FUNCTION__, flags);
    sprintf(err,      "%s: %lx set   bit not working\n", __FUNCTION__, flags);
    wg_fault_report(err);
  }

  clear_bit(0, &flags);
  if (flags != 0) {
    printk(KERN_EMERG "%s: %lx clear bit not working\n", __FUNCTION__, flags);
    sprintf(err,      "%s: %lx clear bit not working\n", __FUNCTION__, flags);
    wg_fault_report(err);
  }

  if (num_online_cpus() != wg_cpus) {
    printk(KERN_EMERG "%s: CPUs offline, found %d, expected %d.",
            __FUNCTION__, num_online_cpus(), wg_cpus);
    sprintf(err,      "%s: CPUs offline, found %d, expected %d.",
            __FUNCTION__, num_online_cpus(), wg_cpus);
    wg_fault_report(err);
  }
}

#ifdef	CONFIG_CRASH_DUMP

#define	proc_wg_init()

#endif

// Switch data, but on XTM3 they both descibe the same physical switch
struct dsa_switch* wg_dsa_sw[DSA_MAX_SWITCHES];
EXPORT_SYMBOL(wg_dsa_sw);

// Port VLAN map register for each switch
u16	      wg_dsa_sw0_map[DSA_MAX_PORTS];
u16	      wg_dsa_sw1_map[DSA_MAX_PORTS];
EXPORT_SYMBOL(wg_dsa_sw0_map);
EXPORT_SYMBOL(wg_dsa_sw1_map);

u16	      wg_dsa_sw0_out[DSA_MAX_PORTS];
u16	      wg_dsa_sw1_out[DSA_MAX_PORTS];
EXPORT_SYMBOL(wg_dsa_sw0_out);
EXPORT_SYMBOL(wg_dsa_sw1_out);

// Flag to enable unknown DA flooding
int	      wg_dsa_flood    = 1;
EXPORT_SYMBOL(wg_dsa_flood);

// Flag to enable switch chip learning
int	      wg_dsa_learning = 1;
EXPORT_SYMBOL(wg_dsa_learning);

// Flag to lock static ATU entries
int	      wg_dsa_lock_static = 1;
EXPORT_SYMBOL(wg_dsa_lock_static);

// Switch device type, 6171X or 6176X
int	      wg_dsa_device = 0;
EXPORT_SYMBOL(wg_dsa_device);

// SMI command register, non-zero in single chip mode
int	      wg_dsa_smi_reg = 0;
EXPORT_SYMBOL(wg_dsa_smi_reg);

// Number of external PHYs on the Marvell Linkstreet switch chip
int	      wg_dsa_phy_num = 5;
EXPORT_SYMBOL(wg_dsa_phy_num);

wg_br_map     wg_br_bridged;	// HW bridging         interfaces
EXPORT_SYMBOL(wg_br_bridged);
wg_br_map     wg_br_primary;	// HW bridging primary interfaces
EXPORT_SYMBOL(wg_br_primary);
wg_br_map     wg_br_dropped;	// HW bridging dropped interfaces
EXPORT_SYMBOL(wg_br_dropped);
wg_sl_map     wg_sl_slaved;	// Slaved eth  device  interfaces
EXPORT_SYMBOL(wg_sl_slaved);

// Pointer to skb to be traced when freed
struct	sk_buff* wg_kfree_skb_trace;
EXPORT_SYMBOL(   wg_kfree_skb_trace);

#ifdef	CONFIG_WG_PLATFORM_OLD_TIMER_HOOK // WG:JB Old timer hook

int (*wg_timer_hook)(struct pt_regs *);

int register_timer_hook(int (*hook)(struct pt_regs *))
{
	if (wg_timer_hook)
		return -EBUSY;
	wg_timer_hook = hook;
	return 0;
}
EXPORT_SYMBOL_GPL(register_timer_hook);

void unregister_timer_hook(int (*hook)(struct pt_regs *))
{
	WARN_ON(hook != wg_timer_hook);
	wg_timer_hook = NULL;
	/* make sure all CPUs see the NULL hook */
	synchronize_sched();  /* Allow ongoing interrupts to complete. */
}
EXPORT_SYMBOL_GPL(unregister_timer_hook);

#endif	// CONFIG_WG_PLATFORM_OLD_TIMER_HOOK

#ifdef	CONFIG_WG_PLATFORM_OLD_PROC_API // WG:JB Put old proc API back in

#include "../fs/proc/internal.h"

/* buffer size is one page but our output routines use some slack for overruns */
#define PROC_BLOCK_SIZE	(PAGE_SIZE - 1024)

enum {BIAS = -1U<<31};

extern	const	struct inode_operations proc_file_inode_operations;
extern	struct	proc_dir_entry* __proc_create(struct proc_dir_entry**, const char*, umode_t, nlink_t);
extern	int	proc_register(struct proc_dir_entry*, struct proc_dir_entry*);

static inline int use_pde(struct proc_dir_entry *pde)
{
	return atomic_inc_unless_negative(&pde->in_use);
}

static void unuse_pde(struct proc_dir_entry *pde)
{
	if (atomic_dec_return(&pde->in_use) == BIAS)
		complete(pde->pde_unload_completion);
}

#define	GFP_TEMPORARY	(__GFP_IO | __GFP_FS | __GFP_RECLAIMABLE)

static ssize_t
__proc_file_read(struct file *file, char __user *buf, size_t nbytes,
	       loff_t *ppos)
{
	struct inode * inode = file_inode(file);
	char 	*page;
	ssize_t	retval=0;
	int	eof=0;
	ssize_t	n, count;
	char	*start;
	struct proc_dir_entry * dp;
	unsigned long long pos;

	/*
	 * Gaah, please just use "seq_file" instead. The legacy /proc
	 * interfaces cut loff_t down to off_t for reads, and ignore
	 * the offset entirely for writes..
	 */
	pos = *ppos;
	if (pos > MAX_NON_LFS)
		return 0;
	if (nbytes > MAX_NON_LFS - pos)
		nbytes = MAX_NON_LFS - pos;

	dp = PDE(inode);
	if (!(page = (char*) __get_free_page(GFP_TEMPORARY)))
		return -ENOMEM;

	while ((nbytes > 0) && !eof) {
		count = min_t(size_t, PROC_BLOCK_SIZE, nbytes);

		start = NULL;
		if (dp->read_proc) {
			/*
			 * How to be a proc read function
			 * ------------------------------
			 * Prototype:
			 *    int f(char *buffer, char **start, off_t offset,
			 *          int count, int *peof, void *dat)
			 *
			 * Assume that the buffer is "count" bytes in size.
			 *
			 * If you know you have supplied all the data you
			 * have, set *peof.
			 *
			 * You have three ways to return data:
			 * 0) Leave *start = NULL.  (This is the default.)
			 *    Put the data of the requested offset at that
			 *    offset within the buffer.  Return the number (n)
			 *    of bytes there are from the beginning of the
			 *    buffer up to the last byte of data.  If the
			 *    number of supplied bytes (= n - offset) is
			 *    greater than zero and you didn't signal eof
			 *    and the reader is prepared to take more data
			 *    you will be called again with the requested
			 *    offset advanced by the number of bytes
			 *    absorbed.  This interface is useful for files
			 *    no larger than the buffer.
			 * 1) Set *start = an unsigned long value less than
			 *    the buffer address but greater than zero.
			 *    Put the data of the requested offset at the
			 *    beginning of the buffer.  Return the number of
			 *    bytes of data placed there.  If this number is
			 *    greater than zero and you didn't signal eof
			 *    and the reader is prepared to take more data
			 *    you will be called again with the requested
			 *    offset advanced by *start.  This interface is
			 *    useful when you have a large file consisting
			 *    of a series of blocks which you want to count
			 *    and return as wholes.
			 *    (Hack by Paul.Russell@rustcorp.com.au)
			 * 2) Set *start = an address within the buffer.
			 *    Put the data of the requested offset at *start.
			 *    Return the number of bytes of data placed there.
			 *    If this number is greater than zero and you
			 *    didn't signal eof and the reader is prepared to
			 *    take more data you will be called again with the
			 *    requested offset advanced by the number of bytes
			 *    absorbed.
			 */
			n = dp->read_proc(page, &start, *ppos,
					  count, &eof, dp->data);
		} else
			break;

		if (n == 0)   /* end of file */
			break;
		if (n < 0) {  /* error */
			if (retval == 0)
				retval = n;
			break;
		}

		if (start == NULL) {
			if (n > PAGE_SIZE)	/* Apparent buffer overflow */
				n = PAGE_SIZE;
			n -= *ppos;
			if (n <= 0)
				break;
			if (n > count)
				n = count;
			start = page + *ppos;
		} else if (start < page) {
			if (n > PAGE_SIZE)	/* Apparent buffer overflow */
				n = PAGE_SIZE;
			if (n > count) {
				/*
				 * Don't reduce n because doing so might
				 * cut off part of a data block.
				 */
				pr_warn("proc_file_read: count exceeded\n");
			}
		} else /* start >= page */ {
			unsigned long startoff = (unsigned long)(start - page);
			if (n > (PAGE_SIZE - startoff))	/* buffer overflow? */
				n = PAGE_SIZE - startoff;
			if (n > count)
				n = count;
		}
		
 		n -= copy_to_user(buf, start < page ? page : start, n);
		if (n == 0) {
			if (retval == 0)
				retval = -EFAULT;
			break;
		}

		*ppos += start < page ? (unsigned long)start : n;
		nbytes -= n;
		buf += n;
		retval += n;
	}
	free_page((unsigned long) page);
	return retval;
}

static ssize_t
proc_file_read(struct file *file, char __user *buf, size_t nbytes,
	       loff_t *ppos)
{
	struct proc_dir_entry *pde = PDE(file_inode(file));
	ssize_t rv = -EIO;

	if (!use_pde(pde)) return -ENOENT;

	spin_lock(&pde->pde_unload_lock);
	if (!pde->proc_fops) {
		spin_unlock(&pde->pde_unload_lock);
		unuse_pde(pde);
		return rv;
	}
	// pde->pde_users++;
	spin_unlock(&pde->pde_unload_lock);

	rv = __proc_file_read(file, buf, nbytes, ppos);

	// pde_users_dec(pde);
	unuse_pde(pde);
	return rv;
}

static ssize_t
proc_file_write(struct file *file, const char __user *buffer,
		size_t count, loff_t *ppos)
{
	struct proc_dir_entry *pde = PDE(file_inode(file));
	ssize_t rv = -EIO;

	if (!use_pde(pde)) return -ENOENT;

	if (pde->write_proc) {
		spin_lock(&pde->pde_unload_lock);
		if (!pde->proc_fops) {
			spin_unlock(&pde->pde_unload_lock);
			unuse_pde(pde);
			return rv;
		}
		// pde->pde_users++;
		spin_unlock(&pde->pde_unload_lock);

		/* FIXME: does this routine need ppos?  probably... */
		rv = pde->write_proc(file, buffer, count, pde->data);
		// pde_users_dec(pde);
	}
	unuse_pde(pde);
	return rv;
}

static loff_t
proc_file_lseek(struct file *file, loff_t offset, int orig)
{
	loff_t retval = -EINVAL;
	switch (orig) {
	case 1:
		offset += file->f_pos;
	/* fallthrough */
	case 0:
		if (offset < 0 || offset > MAX_NON_LFS)
			break;
		file->f_pos = retval = offset;
	}
	return retval;
}

const struct file_operations wg_proc_file_operations = {
	.llseek		= proc_file_lseek,
	.read		= proc_file_read,
	.write		= proc_file_write,
};

struct proc_dir_entry *create_proc_entry(const char *name, umode_t mode,
					 struct proc_dir_entry *parent)
{
	struct proc_dir_entry *ent;
	nlink_t nlink;

	if (S_ISDIR(mode)) {
		if ((mode & S_IALLUGO) == 0)
			mode |= S_IRUGO | S_IXUGO;
		nlink = 2;
	} else {
		if ((mode & S_IFMT) == 0)
			mode |= S_IFREG;
		if ((mode & S_IALLUGO) == 0)
			mode |= S_IRUGO;
		nlink = 1;
	}

	ent = __proc_create(&parent, name, mode, nlink);
	if (ent) {
		if (S_ISREG(mode)) {
			ent->proc_fops = &wg_proc_file_operations;
			ent->proc_iops = &proc_file_inode_operations;
		}
		if (proc_register(parent, ent) < 0) {
			kfree(ent);
			ent = NULL;
		}
	}
	return ent;
}
EXPORT_SYMBOL(create_proc_entry);

void set_proc_read( struct proc_dir_entry *entry, read_proc_t*  func)
{
	entry->read_proc  = func;
}
EXPORT_SYMBOL(set_proc_read);

void set_proc_write(struct proc_dir_entry *entry, write_proc_t* func)
{
	entry->write_proc = func;
}
EXPORT_SYMBOL(set_proc_write);

static	struct proc_dir_entry* proc_wg_kernel_dir;
static	struct proc_dir_entry* proc_cloud_file;
static	struct proc_dir_entry* proc_hyperv_file;
static	struct proc_dir_entry* proc_arch_model_file;
static	struct proc_dir_entry* proc_cpu_model_file;
static	struct proc_dir_entry* proc_cpu_version_file;
static	struct proc_dir_entry* proc_counters_file;
static	struct proc_dir_entry* proc_drops_file;
static	struct proc_dir_entry* proc_dma_errors_file;
static	struct proc_dir_entry* proc_fault_report_file;
static	struct proc_dir_entry* proc_licensed_cpus_file;
static	struct proc_dir_entry* proc_xfrm_bypass_file;
static	struct proc_dir_entry* proc_debug_flag_file;
static	struct proc_dir_entry* proc_tput_tune_file;
static	struct proc_dir_entry* proc_sw_type_file;
#ifdef	CONFIG_WG_ARCH_FREESCALE // WG:JB Only Freesclae has T Series
static	struct proc_dir_entry* proc_t_series_file;
#endif
static	struct proc_dir_entry* proc_version_file;

int proc_read_cloud(char *page, char **start, off_t off,
		    int count, int *eof, void *data)
{
  return sprintf(page, "%s\n", wg_cloud);
}

int proc_read_hyperv(char *page, char **start, off_t off,
		     int count, int *eof, void *data)
{
  char*  name = wg_get_vm_name();
  return sprintf(page, "%s\n", name ? name : "");
}

int proc_read_arch_model(char *page, char **start, off_t off,
			int count, int *eof, void *data)
{
#ifdef	CONFIG_X86
#ifdef	CONFIG_X86_64
  return sprintf(page, "x86_64\n");
#else
  return sprintf(page, "x86_32\n");
#endif
#endif
#ifdef  CONFIG_PPC
#ifdef	CONFIG_PPC64
  return sprintf(page, "ppc_64\n");
#else
  return sprintf(page, "ppc_32\n");
#endif /* CONFIG_PPC64 */
#endif

#ifdef  CONFIG_ARM64
  return sprintf(page, "arm_64\n");
#endif /* CONFIG_ARM64 */
}

int proc_read_cpu_model(char *page, char **start, off_t off,
			int count, int *eof, void *data)
{
  return sprintf(page, "%d\n", wg_get_cpu_model());
}

int proc_read_cpu_version(char *page, char **start, off_t off,
			  int count, int *eof, void *data)
{
#ifdef	CONFIG_PPC
  return sprintf(page, "%x\n", wg_cpu_version);
#else
  return sprintf(page, "%d\n", wg_cpu_version);
#endif
}

static int proc_read_drops(char *page, char **start, off_t off,
			   int count, int *eof, void *data)
{
  int n;

  if (wg_backlog_high > wg_backlog_highest) wg_backlog_highest = wg_backlog_high;

  n = sprintf(page, "Backlog: High %d Highest %d  Errors: Backlog %ld Unknown %ld Length %ld\n",
	      wg_backlog_high, wg_backlog_highest,
	      atomic_long_read(&wg_drop_backlog),
	      atomic_long_read(&wg_drop_unknown),
	      atomic_long_read(&wg_drop_length));

  wg_backlog_high = 0;

  atomic_long_set(&wg_drop_backlog, 0);
  atomic_long_set(&wg_drop_unknown, 0);
  atomic_long_set(&wg_drop_length,  0);

  return n;
}

static int proc_read_dma_errors(char *page, char **start, off_t off,
				int count, int *eof, void *data)
{
  return sprintf(page, "%d\n", atomic_read(&wg_dma_errors));
}

static int proc_read_version(char *page, char **start, off_t off,
			     int count, int *eof, void *data)
{
  return sprintf(page, "4.14\n");
}

static int proc_read_fault_report(char *page, char **start, off_t off,
                                  int count, int *eof, void *data)
{
  int z;

  spin_lock_bh  (&wg_fault_lock);
  z = sprintf(page, "%s", wg_fault_text);
  spin_unlock_bh(&wg_fault_lock);

  return z;
}

static int proc_write_fault_report(struct file *file, const char *buffer,
                                   unsigned long count, void *data)
{
  int  z = count;
  char str[sizeof(wg_fault_text)/4];

  if (z > (sizeof(str)-2))
      z = (sizeof(str)-2);

  if (z < 3)
      z = 0;
  else
  if (copy_from_user(str, buffer, z))
      return -EFAULT;

  spin_lock_bh  (&wg_fault_lock);
  str[wg_fault_used = z] = '\0';
  strcpy(wg_fault_text, str);
  spin_unlock_bh(&wg_fault_lock);

  return count;
}

static int proc_read_licensed_cpus(char *page, char **start, off_t off,
                                   int count, int *eof, void *data)
{
  int z;

  z = sprintf(page, "%d\n", wg_cpus);

  return z;
}

static int proc_write_licensed_cpus(struct file *file, const char *buffer,
                                    unsigned long count, void *data)
{
  int  cpus;
  char str[256], *strend;

  if (count > (sizeof(str)-1)) count = sizeof(str)-1;
  if(copy_from_user(str, buffer, count))
    return -EFAULT;

  str[count] = '\0';

  cpus = simple_strtoul(str, &strend, 10);
  if (cpus > 0) wg_cpus = cpus;

  return count;
}

static int proc_read_sw_type(char *page, char **start, off_t off,
                             int count, int *eof, void *data)
{
  int t = (wg_dsa_count > 0) ? 886171 : 0;

  if (has88E6176)  t = 886176;
  if (has88E6190)  t = 886190;
  if (has98DX3035) t = 983035;

  return sprintf(page, "%d\n", t);
}

#ifdef	CONFIG_WG_ARCH_FREESCALE // WG:JB Only Freesclae has T Series
static int proc_read_t_series(char *page, char **start, off_t off,
                              int count, int *eof, void *data)
{
  return sprintf(page, "%d\n", isTx0xx ? wg_cpu_model : 0);
}
#endif

#ifdef	CONFIG_CRASH_DUMP
static	struct proc_dir_entry* proc_log_file;

static int proc_write_log(struct file *file, const char *buffer,
                          unsigned long count, void *data)
{
  char        str[256];
  static char log[512];
  static int  z = 0;

  if (count > (sizeof(str)-1)) count = sizeof(str)-1;
  if(copy_from_user(str, buffer, count))
      return -EFAULT;

  str[count] = '\0';

  if ((z +  count) >= (sizeof(log)-1))
      return -EFAULT;
  else
       z += count;

  if (strchr(strcat(log, str), '\n'))  {
      printk(log);
      memset(log, z = 0, sizeof(log));
  }

  return count;
}
#endif

int           wg_xfrm_bypass = (1<<NF_INET_PRE_ROUTING); // Bypass PRE_ROUTING
EXPORT_SYMBOL(wg_xfrm_bypass);

static int proc_write_xfrm_bypass(struct file *file, const char *buffer,
                                  unsigned long count, void *data)
{
  char str[256], *strend;

  if (count > (sizeof(str)-1)) count = sizeof(str)-1;
  if(copy_from_user(str, buffer, count))
    return -EFAULT;

  str[count] = '\0';

  // Never bypass FORWARD
  wg_xfrm_bypass = simple_strtol(str, &strend, 16) & (~(1<<NF_INET_FORWARD));

  return count;
}

static int proc_read_xfrm_bypass(char *page, char **start, off_t off,
                                 int count, int *eof, void *data)
{
  return sprintf(page, "%x\n", wg_xfrm_bypass);
}

int           wg_debug_flag = 0;
EXPORT_SYMBOL(wg_debug_flag);

static int proc_write_debug_flag(struct file *file, const char *buffer,
                                  unsigned long count, void *data)
{
  char str[256], *strend;

  if (count > (sizeof(str)-1)) count = sizeof(str)-1;
  if(copy_from_user(str, buffer, count))
    return -EFAULT;

  str[count] = '\0';

  wg_debug_flag = simple_strtol(str, &strend, 16);

  return count;
}

static int proc_read_debug_flag(char *page, char **start, off_t off,
                                 int count, int *eof, void *data)
{
  return sprintf(page, "%x\n", wg_debug_flag);
}

// WG:XD FBX-19123
int           wg_tput_tune = 0;
EXPORT_SYMBOL(wg_tput_tune);

static int proc_write_tput_tune(struct file *file, const char *buffer,
                                  unsigned long count, void *data)
{
  char str[256], *strend;

  if (count > (sizeof(str)-1)) count = sizeof(str)-1;
  if(copy_from_user(str, buffer, count))
    return -EFAULT;

  str[count] = '\0';

  wg_tput_tune = simple_strtol(str, &strend, 16);

  return count;
}

static int proc_read_tput_tune(char *page, char **start, off_t off,
                                 int count, int *eof, void *data)
{
  return sprintf(page, "%x\n", wg_tput_tune);
}

#endif	// CONFIG_WG_PLATFORM_OLD_PROC_API

#ifdef	CONFIG_X86

int	wg_seattle;	// Seattle   model # if any
EXPORT_SYMBOL(wg_seattle);

int	wg_kirkland;	// Kirkland  model # if any
EXPORT_SYMBOL(wg_kirkland);

int	wg_colfax;	// Colfax    model # if any
EXPORT_SYMBOL(wg_colfax);

int	wg_westport;	// Westport  model # if any
EXPORT_SYMBOL(wg_westport);

int	wg_winthrop;	// Winthrop  model # if any
EXPORT_SYMBOL(wg_winthrop);

int	wg_clarkston;	// Clarkston model # if any
EXPORT_SYMBOL(wg_clarkston);

int	wg_rdtsc_trusted_uid; // UIDs above this denied RDTSC

char* __init wg_get_x86_id(char* id)
{
  unsigned int *v;
  char *p, *q;

  v = (unsigned int *)&id[0];
  cpuid(0x80000002, &v[0], &v[1], &v[2],  &v[3]);
  cpuid(0x80000003, &v[4], &v[5], &v[6],  &v[7]);
  cpuid(0x80000004, &v[8], &v[9], &v[10], &v[11]);
  id[48] = 0;

  /*
   * Intel chips right-justify this string for some dumb reason.
   */
  p = q = &id[0];
  while (*p == ' ')
    p++;
  if (p != q) {
    while (*p)
      *q++ = *p++;
    while (q <= &id[48])
      *q++ = 0;
  }

  return id;
}

void __init wg_kernel_get_x86_model(struct cpuinfo_x86* c)
{
  wg_cpu_model = WG_CPU_X86(0);

  if (c) {
    char  ch;
    int   m = 0, v = 0;
    char* p = &c->x86_model_id[0];

    // Check for model 6
    if (c->x86 == 6) {
      wg_cpu_model = WG_CPU_686(0);
      // Get ID if we don't already have it
      if (c->extended_cpuid_level >= 0x80000004)
      if (*p == 0)
        wg_get_x86_id(p);
    }

    // Look for model numbers
    while ((ch = *p++)) {
      if  ((ch == '@')) break;
      if  ((ch >= '0') && (ch <= '9'))
	v = (v * 10) + (ch - '0');
      else {
	if  (v >= 400) m = v;
        v = (INT_MIN);
      }
      if ((m >= 400) && (v >= 0)) break;
    }

    wg_cpu_model  +=  m;
    wg_cpu_version = (v > 0) ? v : 0;

    wg_seattle  = 0;
    wg_kirkland = 0;
    wg_colfax   = 0;
    wg_westport = 0;
    wg_winthrop = 0;
    wg_clarkston = 0;

    if (wg_vm_name) return;

    if (wg_cpu_model == WG_CPU_686(2758)) wg_seattle  =  1;
    if (wg_cpu_model == WG_CPU_686(1820)) wg_kirkland =  1;
    if (wg_cpu_model == WG_CPU_686(3420)) wg_kirkland =  2;
    if (wg_cpu_model == WG_CPU_686(1275)) wg_colfax   =  1;
    if (wg_cpu_model == WG_CPU_686(3060)) wg_westport =  1;
    if (wg_cpu_model == WG_CPU_686(3160)) wg_westport =  2;
    if (wg_cpu_model == WG_CPU_686(3558)) wg_winthrop =  2;
    if (wg_cpu_model == WG_CPU_686(3900)) wg_winthrop =  3;
    if (wg_cpu_model == WG_CPU_686(4400)) wg_winthrop =  4;
    if (wg_cpu_model == WG_CPU_686(6100)) wg_winthrop =  5;
    if (wg_cpu_model == WG_CPU_686(1225)) wg_winthrop =  6;
    if (wg_cpu_model == WG_CPU_686(2176)) wg_clarkston = 1;
    if (wg_cpu_model == WG_CPU_686(6138)) wg_clarkston = 2;
    if (wg_cpu_model == WG_CPU_686(6230)) wg_clarkston = 2;

    if (wg_cpu_model == WG_CPU_686(2758)) wg_cpus =  8;
    if (wg_cpu_model == WG_CPU_686(1820)) wg_cpus =  2;
    if (wg_cpu_model == WG_CPU_686(3420)) wg_cpus =  2;
    if (wg_cpu_model == WG_CPU_686(1275)) wg_cpus =  8;
    if (wg_cpu_model == WG_CPU_686(2680)) wg_cpus = 20;
    if (wg_cpu_model == WG_CPU_686(3060)) wg_cpus =  2;
    if (wg_cpu_model == WG_CPU_686(3160)) wg_cpus =  4;
    if (wg_cpu_model == WG_CPU_686(3558)) wg_cpus =  4;
    if (wg_cpu_model == WG_CPU_686(3758)) wg_cpus =  8;
    if (wg_cpu_model == WG_CPU_686(3900)) wg_cpus =  2;
    if (wg_cpu_model == WG_CPU_686(4400)) wg_cpus =  2;
    if (wg_cpu_model == WG_CPU_686(6100)) wg_cpus =  4;
    if (wg_cpu_model == WG_CPU_686(1225)) wg_cpus =  4;
    if (wg_cpu_model == WG_CPU_686(2176)) wg_cpus =  12;
    if (wg_cpu_model == WG_CPU_686(6138)) wg_cpus =  40;
    if (wg_cpu_model == WG_CPU_686(6230)) wg_cpus =  40;

    if (has88E6176) wg_dsa_count = 1;
    if (has88E6190) wg_dsa_count = 1;

    wg_rdtsc_trusted_uid = 0;
  }
}
#endif

LIST_HEAD    (wg_counter_list);
spinlock_t    wg_counter_lock;

struct	wg_counter* wg_add_counter(char* name)
{
  struct wg_counter* item = kzalloc(sizeof(struct wg_counter), GFP_ATOMIC);

  if (item) {
    atomic_set(&item->count, 0);
    strncpy(    item->name,  name, sizeof(item->name)-1);

    spin_lock_bh(	    &wg_counter_lock);
    list_add(  &item->list, &wg_counter_list);
    spin_unlock_bh(	    &wg_counter_lock);
  }

  return item;
}
EXPORT_SYMBOL(wg_add_counter);

void  wg_del_counter(struct  wg_counter** item)
{
  if ( item)
  if (*item) {
    spin_lock_bh(	    &wg_counter_lock);
    list_del(&(*item)->list);
    kfree(*item);
    *item = NULL;
    spin_unlock_bh(	    &wg_counter_lock);
  }
}
EXPORT_SYMBOL(wg_del_counter);

int proc_read_counters(char *page, char **start, off_t off,
                       int count, int *eof, void *data)
{
  int	 z = 0;
  struct wg_counter* item;

  spin_lock_bh(		    &wg_counter_lock);

  for (item  = (struct wg_counter*) wg_counter_list.next;
       item != (struct wg_counter*)&wg_counter_list;
       item  = (struct wg_counter*)      item->list.next) {
    unsigned long k = atomic_read(&item->count);

    if (k == 0) continue;

    atomic_set(&item->count, 0);
    if (z <= (PAGE_SIZE-32))
        z += sprintf(page, "%s: %lu\n", item->name, k);
  }

  spin_unlock_bh(	    &wg_counter_lock);

  return  z;
}

int __init wg_kernel_init(void)
{

#ifdef	CONFIG_X86
  char*  name = wg_get_vm_name();
  static int run = 0;

  if (run++) return 0;

  wg_kernel_get_x86_model(&cpu_data(0));

  printk(" WG CPU Model %d Hypervisor '%s'\n",
	 wg_get_cpu_model(), name ? name : "Bare");
#if BITS_PER_LONG < 64
  printk(" WG VMALLOC   RESERVE %4d MB\n", __VMALLOC_RESERVE >> 20);
#endif
#endif

#if defined (CONFIG_PPC) || defined (CONFIG_ARM64)
  struct device_node *root;
  static int run = 0;

  if (run++) return 0;

#ifdef	CONFIG_PPC
  wg_cpu_version = mfspr(SPRN_PVR);
#endif

  if ((root = of_find_node_by_path("/"))) {
    const char *model;

    if ((model = of_get_property(root, "model", NULL))) {
      if (strstr(model, "2080")) {
	wg_cpu_model     = WG_CPU_T2081;
	wg_dsa_count     = 1;
	wg_caam_model    = 1;
	wg_cpus		 = 8;
      } else
      if (strstr(model, "1042")) {
	wg_cpu_model     = WG_CPU_T1042;
	wg_dsa_count     = 1;
	wg_caam_model    = 1;
	wg_cpus		 = 4;
      } else
      if (strstr(model, "1024")) {
	wg_cpu_model     = WG_CPU_T1024;
	wg_dsa_count     = 1;
	wg_caam_model    = 1;
	wg_cpus		 = 2;
      } else
      if (strstr(model, "2020")) {
	wg_cpu_model     = WG_CPU_P2020;
	wg_dsa_count     = 2;
	wg_talitos_model = 1;
	wg_cpus		 = 2;
      } else
      if (strstr(model, "1020")) {
	wg_cpu_model     = WG_CPU_P1020;
	wg_dsa_count     = 1;
	wg_talitos_model = 1;
	wg_cpus		 = 2;
      } else
      if (strstr(model, "1011")) {
	wg_cpu_model     = WG_CPU_P1011;
	wg_dsa_count     = 1;
	wg_talitos_model = 1;
	wg_cpus		 = 1;
      } else
      if (strstr(model, "1010")) {
	wg_cpu_model     = WG_CPU_P1010;
	wg_dsa_count     = 0;
	wg_caam_model    = 1;
	wg_cpus		 = 1;
      } else
      if (strstr(model, "LS1046")) { /* T80/M290 support: (ARM64) */
	wg_cpu_model     = WG_CPU_LS1046;
	if (strstr(model, "M290")) {
		wg_castlerock    = 1;
	}
	wg_dsa_count     = 1; /* Will handle this later */
	wg_caam_model    = 0;
	wg_cpus		 = 4;
      } else
      if (strstr(model, "LS1043")) { /* T20/T40 support: (ARM64) */
	wg_cpu_model     = WG_CPU_LS1043;
	wg_dsa_count     = 0;
	wg_caam_model    = 0;
	wg_cpus		 = 4;
      } else
      if (strstr(model, "2088")) { /* M390 support: (ARM64) */
	wg_cpu_model     = WG_CPU_LS2088;
	wg_castlerock    = 2;
	wg_dsa_count     = 1; /* Will handle this later */
	wg_caam_model    = 0;
	wg_cpus          = 8;
      } else
      if (strstr(model, "LX2160")) { /* M590/M690 support: (ARM64) */
	wg_cpu_model     = WG_CPU_LX2160;
	wg_castlerock    = 3;
	wg_dsa_count     = 1; /* Will handle this later */
	wg_caam_model    = 0;
	wg_cpus          = 16;
      }
      printk(" WG CPU Model %d Count %d\n", wg_get_cpu_model(), wg_cpus);
    }

    of_node_put(root);

#ifdef	CONFIG_PPC64	/* WG:JB Set Dynamic FSL errata */
    if (wg_cpu_model == WG_CPU_T1042) {
      FSL_ERRATUM_A_005337 = 1;
      printk(" Setting RSL Erratum A_005337\n");
    }
    else
    if (wg_cpu_model == WG_CPU_T2081) {
      FSL_ERRATUM_A_005127 = 1;
      printk(" Setting FSL Erratum A_005127\n");
    }
#endif	/* CONFIG_PPC64	 */

#ifdef	CONFIG_PPC
    wg_boren   = 0;
    if ((strstr(model, "BOREN")) ||
	(ppc_proc_freq >= 666666666)) {
      if (wg_cpu_model == WG_CPU_P1011) wg_boren = 1;
      if (wg_cpu_model == WG_CPU_P1020) wg_boren = 2;
    }
#endif /* CONFIG_PPC */

  }

#endif /* defined (CONFIG_PPC) || defined (CONFIG_ARM64) */

  printk(" WG CRASH MIN RESERVE %4d MB\n", wg_crash_memory >> 20);

  printk(KERN_INFO "\n%s: Built " __DATE__ " " __TIME__ "\n", __FUNCTION__);

  atomic_long_set(&wg_drop_backlog, 0);
  atomic_long_set(&wg_drop_unknown, 0);
  atomic_long_set(&wg_drop_length,  0);

#ifdef  CONFIG_WG_PLATFORM_OLD_PROC_API // WG:JB Put old proc API back in

  // Create /proc/wg_kernel
  proc_wg_kernel_dir = proc_mkdir("wg_kernel", NULL);
  if (!proc_wg_kernel_dir) return -EPERM;

  // Create each wg_kernel entry

  proc_cloud_file = create_proc_entry(      "cloud",           0444,
					     proc_wg_kernel_dir);
  if (proc_cloud_file) {
    set_proc_read(proc_cloud_file,	     proc_read_cloud);
  }

  proc_hyperv_file = create_proc_entry(      "hyperv",         0444,
					     proc_wg_kernel_dir);
  if (proc_hyperv_file) {
    set_proc_read(proc_hyperv_file,	     proc_read_hyperv);
  }

  proc_arch_model_file = create_proc_entry(  "arch_model",     0444,
					     proc_wg_kernel_dir);
  if (proc_arch_model_file) {
    set_proc_read(proc_arch_model_file,	     proc_read_arch_model);
  }

  proc_cpu_model_file = create_proc_entry(   "cpu_model",      0444,
					     proc_wg_kernel_dir);
  if (proc_cpu_model_file) {
    set_proc_read(proc_cpu_model_file,	     proc_read_cpu_model);
  }

  proc_cpu_version_file = create_proc_entry( "cpu_version",    0444,
					     proc_wg_kernel_dir);
  if (proc_cpu_version_file) {
    set_proc_read(proc_cpu_version_file,     proc_read_cpu_version);
  }

  proc_counters_file = create_proc_entry(   "counters",        0444,
					     proc_wg_kernel_dir);
  if (proc_counters_file) {
    set_proc_read(proc_counters_file,	     proc_read_counters);
  }

  proc_drops_file = create_proc_entry(       "drops",          0444,
					     proc_wg_kernel_dir);
  if (proc_drops_file) {
    set_proc_read(proc_drops_file,	     proc_read_drops);
  }

  proc_dma_errors_file = create_proc_entry(  "dma_errors",     0444,
					     proc_wg_kernel_dir);
  if (proc_dma_errors_file) {
    set_proc_read(proc_dma_errors_file,	     proc_read_dma_errors);
  }

  proc_fault_report_file = create_proc_entry("fault_report",   0666,
					     proc_wg_kernel_dir);
  if (proc_fault_report_file) {
    set_proc_read(proc_fault_report_file,    proc_read_fault_report);
    set_proc_write(proc_fault_report_file,   proc_write_fault_report);
  }

  proc_licensed_cpus_file = create_proc_entry("licensed_cpus", 0666,
					     proc_wg_kernel_dir);
  if (proc_licensed_cpus_file) {
    set_proc_read(proc_licensed_cpus_file,   proc_read_licensed_cpus);
    set_proc_write(proc_licensed_cpus_file,  proc_write_licensed_cpus);
  }

  proc_sw_type_file = create_proc_entry(     "sw_type",        0444,
					     proc_wg_kernel_dir);
  if (proc_sw_type_file) {
    set_proc_read(proc_sw_type_file,         proc_read_sw_type);
  }

#ifdef	CONFIG_WG_ARCH_FREESCALE // WG:JB Only Freesclae has T Series
  proc_t_series_file = create_proc_entry(    "t_series",       0444,
					     proc_wg_kernel_dir);
  if (proc_t_series_file) {
    set_proc_read(proc_t_series_file,        proc_read_t_series);
  }
#endif

#ifdef	CONFIG_CRASH_DUMP
  proc_log_file  = create_proc_entry(        "log",            0222,
					     proc_wg_kernel_dir);
  if (proc_log_file) {
    set_proc_write(proc_log_file,	     proc_write_log);
  }
#endif

  proc_version_file = create_proc_entry(    "version",     0444,
					     proc_wg_kernel_dir);
  if (proc_version_file) {
    set_proc_read(proc_version_file,	     proc_read_version);
  }

  proc_xfrm_bypass_file = create_proc_entry("xfrm_bypass", 0644,
                                             proc_wg_kernel_dir);
  if (proc_xfrm_bypass_file) {
    set_proc_read(proc_xfrm_bypass_file,     proc_read_xfrm_bypass);
    set_proc_write(proc_xfrm_bypass_file,    proc_write_xfrm_bypass);
  }

  proc_debug_flag_file  = create_proc_entry("debug_flag",  0644,
                                             proc_wg_kernel_dir);
  if (proc_debug_flag_file) {
    set_proc_read(proc_debug_flag_file,      proc_read_debug_flag);
    set_proc_write(proc_debug_flag_file,     proc_write_debug_flag);
  }

  // WG:XD FBX-19123
  proc_tput_tune_file  = create_proc_entry("tput_tune",  0644,
                                             proc_wg_kernel_dir);
  if (proc_tput_tune_file) {
    set_proc_read(proc_tput_tune_file,      proc_read_tput_tune);
    set_proc_write(proc_tput_tune_file,     proc_write_tput_tune);
  }

  proc_wg_init();

#endif	// CONFIG_WG_PLATFORM_OLD_PROC_API // WG:JB Put old proc API back in

  // Return no errors
  return 0;
}

#if defined(CONFIG_WG_ARCH_X86) || defined(CONFIG_WG_PLATFORM_CASTLEROCK)	// WG:XD FBX-21025
int pci_enable_msix(struct pci_dev* dev, struct msix_entry* entries, int nvec)
{
  extern int pci_enable_msix_range(struct pci_dev*, struct msix_entry*, int, int);
  int rc = pci_enable_msix_range(dev, entries, nvec, nvec);
  if ((rc == 0) || (rc == nvec)) return 0;
  return rc;
}
EXPORT_SYMBOL(pci_enable_msix);
#endif

early_initcall(wg_kernel_init);

void __init wg_setup_arch(char* boot_command_line, int size)
{
#ifdef	CONFIG_X86

  // WG:JB fix up root dev as needed
  char*   root  = boot_command_line;
  char*   xtmv  = strstr(root, "wg_ptfm=xtmv");  /* All virtual hosts */
  char*   cloud = strstr(root, "wg_cloud=");

  wg_sysb_mode = is_sysb((const char*) boot_command_line);
  
  // WG:DW root rewriting is a bad idea but we're sorta stuck.
  root = strstr(root, "root=/dev/");
  if (xtmv) {
    if (wg_vm_name == NULL) {
      wg_vm_name = "xtmv";
    }
	// root dev may vary even for the same hypervisor (FireboxV and
	// AWS T3). Will have to detect "cloud" in advance
    if (cloud) {
      // WG:JB Copy cloud environment (if any)
      strncpy(wg_cloud, cloud += 9, sizeof(wg_cloud) - 1);
      for (cloud = wg_cloud; *cloud > ' '; cloud++);
      *cloud = 0;
    }
    if (root) {
      char revroot[32];
      char *pa, *pb;
      int origlen, revlen, limit;
      root += 5;
      limit = sizeof(revroot);
      for (pa=revroot, pb=root; --limit && *pb && *pb != ' '; *pa++ = *pb++);
      *pa = '\0';
      origlen = strlen(revroot);
      if (wg_fix_xen_rootdev(revroot) != 0) {
	revlen = strlen(revroot);
	if (origlen != revlen) {
	  memmove(root+revlen, root+origlen, strlen(root+origlen)+1);
	}
	memmove(root, revroot, revlen);
      }
    }

    if (strstr(boot_command_line, "init=/sbin/init") == NULL) {
      // WG:JB Force running /sbin/init
      strlcat(boot_command_line, " init=/sbin/init", size);
    }
  }

#ifndef	CONFIG_CRASH_DUMP
  // WG:JB Make sure we have as much kdump memory as the platform needs
  {
    int cm = 48;

#ifdef	CONFIG_X86_64 // These Winthrops need more space.
    wg_kernel_get_x86_model(&boot_cpu_data);
    if (wg_winthrop >= 5) cm += 0;
#endif

    // WG:JB Add a crash kernel if none specified
    if (!strstr(boot_command_line, "crashkernel=")) {
      char crash_string[32];
      sprintf(crash_string,       " crashkernel=%dM",  cm);
      strlcat(boot_command_line,    crash_string,    size);
    }

    wg_crash_memory = (cm << 20);
  }

#if BITS_PER_LONG < 64
  // WG:JB Make sure we have as much VM as the platform needs
  {
    int vm = 768;

	// WG: BUG78806 - increase the vmalloc size for xtm-1050 from 512M to 640M
    if (strstr(boot_command_line, "wg_ptfm=xtm10" )) vm = 640; else
    if (strstr(boot_command_line, "wg_ptfm=xtm8"  )) vm = 512; else
    if (strstr(boot_command_line, "wg_ptfm=xtm5"  )) vm = 320;

    // force reserving the resource if it is filton.
    if (strstr(boot_command_line, "wg_ptfm=xtm20" )) {
      strlcat(boot_command_line, " reserve=0x800,0x7f", size);
    }

    if (__VMALLOC_RESERVE < (vm << 20))
	__VMALLOC_RESERVE = (vm << 20);
  }
#endif
#endif
#endif
}

void wg_dev_get(struct net_device* dev)
{
#ifdef	CONFIG_WG_PLATFORM_DEV_REF_DEBUG // WG:JB Track device ref counts
  int  ref;
  char buf[KSYM_SYMBOL_LEN+1];

  atomic_inc(&dev->wg_dev_refcnt);

  if (strncmp(dev->name, "pptp", 4)) return;

  ref = netdev_refcnt_read(dev);
  sprint_symbol(&buf[0], (unsigned long)__builtin_return_address(0));

  printk(KERN_DEBUG "CPU%d PID %5d %-16s %-5s %s # %2d @ %s\n",
	 raw_smp_processor_id(), current->pid, current->comm,
	 dev->name, "Get", ref, buf);

  if (strncmp(current->comm, "bwdrv", 5) == 0) {
    dump_stack();
  }
#endif	// CONFIG_WG_PLATFORM_DEV_REF_DEBUG
}

EXPORT_SYMBOL_GPL(wg_dev_get);

void wg_dev_put(struct net_device* dev)
{
#ifdef	CONFIG_WG_PLATFORM_DEV_REF_DEBUG // WG:JB Track device ref counts
  int  ref;
  char buf[KSYM_SYMBOL_LEN+1];

  atomic_dec(&dev->wg_dev_refcnt);

  if (strncmp(dev->name, "pptp", 4)) return;

  ref = netdev_refcnt_read(dev);
  sprint_symbol(&buf[0], (unsigned long)__builtin_return_address(0));

  printk(KERN_DEBUG "CPU%d PID %5d %-16s %-5s %s # %2d @ %s\n",
	 raw_smp_processor_id(), current->pid, current->comm,
	 dev->name, "Put", ref, buf);

  if (strncmp(current->comm, "bwdrv", 5) == 0) {
    dump_stack();
  }
#endif	// CONFIG_WG_PLATFORM_DEV_REF_DEBUG
}
EXPORT_SYMBOL_GPL(wg_dev_put);

#ifndef	CONFIG_CC_STACKPROTECTOR
void __stack_chk_fail(void)
{
	printk(KERN_EMERG "%s()\n", __FUNCTION__);
}
EXPORT_SYMBOL_GPL(__stack_chk_fail);
#endif

#ifdef	CONFIG_WG_PLATFORM_TRACE

int	wg_trace_dump(const char* func, int line, int flag, int code)
{
	if (console_loglevel < (flag & ~WG_DUMP)) return code;

	if (unlikely(code))
	printk(KERN_EMERG "%s:%-4d code %d\n", func, line, code);
	else
	printk(KERN_EMERG "%s:%-4d\n",         func, line);

	if (flag & WG_DUMP) {
		int lev = console_loglevel;
		console_loglevel =   9;
		dump_stack();
		console_loglevel = lev;
	}

	return code;
}
EXPORT_SYMBOL_GPL(wg_trace_dump);

void	wg_dump_hex(const u8* buf, u32 len, const u8* tag)
{
	if (console_loglevel < 32) len = (len < 256) ? len : 256;

	if (buf)
	if (len > 0)
	print_hex_dump(KERN_CONT, tag, DUMP_PREFIX_OFFSET, 16, 1, buf, len, 0);
}
EXPORT_SYMBOL_GPL(wg_dump_hex);

void	wg_dump_sgl(const char* func, int line, int flag,
		    struct scatterlist* sgl, const char* tag)
{
	int j;

	if (console_loglevel < (flag & ~WG_DUMP)) return;

	for (j = 0; sgl; sgl = sg_next(sgl), j++) {

	printk(KERN_EMERG "%s:%-4d %s%d %p len %4d virt %p\n", func, line,
	       tag, j, sgl, sgl->length, sg_virt(sgl));

	if (flag & WG_DUMP) {
		int lev = console_loglevel;
		console_loglevel =   9;
		wg_dump_hex(sg_virt(sgl), sgl->length + 8, "");
		console_loglevel = lev;
	}

	}
}
EXPORT_SYMBOL_GPL(wg_dump_sgl);

#endif

void	wg_sample_nop(unsigned long pc) {}
EXPORT_SYMBOL_GPL(wg_sample_nop);

void  (*wg_sample_pc)(unsigned long) = wg_sample_nop;
EXPORT_SYMBOL_GPL(wg_sample_pc);

int	wg_seq_file_timeout = (10*HZ);	// Mutex timeout
EXPORT_SYMBOL(wg_seq_file_timeout);

MODULE_LICENSE("GPL");
