#ifndef	_LINUX_WG_LOCK_H
#define	_LINUX_WG_LOCK_H

#ifdef	CONFIG_WG_PLATFORM_LOCK

#include <linux/types.h>

// Read  lock bit is one next to the sign bit
#define	WG_LOCK_RD_BIT		((sizeof(atomic_t)*8)-2)
#define	WG_LOCK_RD_MASK		(1<<WG_LOCK_RD_BIT)

// Write lock bit is the next bit
#define	WG_LOCK_WR_BIT		((sizeof(atomic_t)*8)-3)
#define	WG_LOCK_WR_MASK		(1<<WG_LOCK_WR_BIT)

// Try   lock bit is the next bit
#define	WG_LOCK_TR_BIT		((sizeof(atomic_t)*8)-4)
#define	WG_LOCK_TR_MASK		(1<<WG_LOCK_TR_BIT)

// High count bit is the next bit
#define	WG_LOCK_HI_BIT		((sizeof(atomic_t)*8)-5)
#define	WG_LOCK_HI_MASK		(1<<WG_LOCK_HI_BIT)

// Writer bits
#define	WG_LOCK_WRITERS		(WG_LOCK_WR_MASK|WG_LOCK_TR_MASK)

// Reader bits
#define	WG_LOCK_READERS		(WG_LOCK_RD_MASK)

// Users  bits
#define	WG_LOCK_USERS		(WG_LOCK_READERS|WG_LOCK_WRITERS)

// State is not the flags bits
#define	WG_LOCK_STATE		(~(WG_LOCK_USERS|WG_LOCK_HI_MASK))

// Lock freed state has  read  lock only
#define	WG_LOCK_FREED		((unsigned)(WG_LOCK_READERS|WG_LOCK_HI_MASK))
// Lock owner state adds write lock and ref count of 1
#define	WG_LOCK_OWNER		((unsigned)(WG_LOCK_FREED|WG_LOCK_WR_MASK|1))
// Lock query state adds try   lock and ref count of 1
#define	WG_LOCK_QUERY		((unsigned)(WG_LOCK_FREED|WG_LOCK_TR_MASK|1))

// Spin freed state has  write lock only
#define	WG_SPIN_FREED		((unsigned)(WG_LOCK_WRITERS|WG_LOCK_HI_MASK))
// Spin owner state adds ref count of 1
#define	WG_SPIN_OWNER		((unsigned)(WG_SPIN_FREED|1))

struct wg_lock_ctx {
  atomic_t in_irq;	    // Count of locks done while in hard irq
  void*    pc_irq;	    // PC    when in hard irq
  atomic_t in_fork;	    // Count of locks done while in fork context
  void*    pc_fork;	    // PC    when in fork context
};

struct wg_lock_data {
  atomic_t count;	    // Lock total count
  atomic_t stuck;	    // Lock stuck count
  atomic_t reset;	    // Lock reset count
  atomic_t under;	    // Lock under count
  atomic_t spin;	    // Lock spin  count
  unsigned high;	    // Lock wait  time high water
  void*    grab;	    // Lock grab  PC
  void*    toss;	    // Lock toss  PC
};

// Lock structure, simple counter unless debug
typedef	struct wg_spin {
  atomic_t state;	    // Lock state ref count
  struct   list_head  list; // Debug list
  struct   wg_lock_ctx ctx; // Lock  context
  struct   wg_lock_data wr; // Write lock stats
  struct   wg_lock_data rd; // Read  lock stats
} wgspin_t;

typedef	struct wg_lock {
  atomic_t state;	    // Lock state ref count
  struct   list_head  list; // Debug list
  struct   wg_lock_ctx ctx; // Lock  context
  struct   wg_lock_data wr; // Write lock stats
  struct   wg_lock_data rd; // Read  lock stats
} wglock_t;

// Locks start with a ref count of zero
#define	DEFINE_WGLOCK(x)    wglock_t x = { ATOMIC_INIT(WG_LOCK_FREED) }

// Spins start with a ref count of zero
#define	DEFINE_WGSPIN(x)    wgspin_t x = { ATOMIC_INIT(WG_SPIN_FREED) }

// Don't do inlining of these functions
#undef CONFIG_INLINE_SPIN_TRYLOCK
#undef CONFIG_INLINE_SPIN_TRYLOCK_BH
#undef CONFIG_INLINE_SPIN_LOCK
#undef CONFIG_INLINE_SPIN_LOCK_BH
#undef CONFIG_INLINE_SPIN_LOCK_IRQ
#undef CONFIG_INLINE_SPIN_LOCK_IRQSAVE
#undef CONFIG_INLINE_SPIN_UNLOCK
#undef CONFIG_INLINE_SPIN_UNLOCK_BH
#undef CONFIG_INLINE_SPIN_UNLOCK_IRQ
#undef CONFIG_INLINE_SPIN_UNLOCK_IRQRESTORE
#undef CONFIG_INLINE_READ_TRYLOCK
#undef CONFIG_INLINE_READ_LOCK
#undef CONFIG_INLINE_READ_LOCK_BH
#undef CONFIG_INLINE_READ_LOCK_IRQ
#undef CONFIG_INLINE_READ_LOCK_IRQSAVE
#undef CONFIG_INLINE_READ_UNLOCK
#undef CONFIG_INLINE_READ_UNLOCK_BH
#undef CONFIG_INLINE_READ_UNLOCK_IRQ
#undef CONFIG_INLINE_READ_UNLOCK_IRQRESTORE
#undef CONFIG_INLINE_WRITE_TRYLOCK
#undef CONFIG_INLINE_WRITE_LOCK
#undef CONFIG_INLINE_WRITE_LOCK_BH
#undef CONFIG_INLINE_WRITE_LOCK_IRQ
#undef CONFIG_INLINE_WRITE_LOCK_IRQSAVE
#undef CONFIG_INLINE_WRITE_UNLOCK
#undef CONFIG_INLINE_WRITE_UNLOCK_BH
#undef CONFIG_INLINE_WRITE_UNLOCK_IRQ
#undef CONFIG_INLINE_WRITE_UNLOCK_IRQRESTORE

#endif	// CONFIG_WG_PLATFORM_LOCK

#endif	// _LINUX_WG_LOCK_H
