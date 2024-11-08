#ifndef	_LINUX_WG_LOCK_API_H
#define	_LINUX_WG_LOCK_API_H

#include <linux/wg_lock_types.h>

#ifdef	CONFIG_WG_PLATFORM_LOCK

#include <linux/string.h>
#include <linux/seq_file.h>
#include <asm/atomic.h>

#define	WG_LOCK_SP_DBG		0x001	// Debug spin lock
#define	WG_LOCK_WR_DBG		0x002	// Debug writelock
#define	WG_LOCK_RD_DBG		0x004	// Debug read lock
#define	WG_LOCK_PC_DBG		0x008	// Debug contexts
#define	WG_LOCK_NL_DBG		0x010	// Debug not locked
#define	WG_LOCK_LK_DBG		0x020	// Debug fix locked
#define	WG_LOCK_SD_DBG		0x800	// Debug stack dump

// Unsigned atomic read
#define	atomic_get(a)		((unsigned)atomic_read(a))
// Unsigned atomic increment
#define	atomic_bump(a)		((unsigned)atomic_inc_return(a))

// Lock debug item
struct wg_list_item {
  struct list_head  list;	// List debug chain
  struct list_head* item;	// Lock being debugged
};

// Locks debugging flag
extern	int	 wg_lock_debug;

// Print debug exceptions
extern	void	 wg_lock_debug_print(struct seq_file*);

// Clear debug info on lock
extern	void	 wg_lock_debug_clear(wglock_t*);
// Enable  debugging on lock, insert into debug list
extern	void	 wg_lock_debug_enable(struct list_head*);
// Disable debugging on lock, remove from debug list
extern	void	 wg_lock_debug_disable(struct list_head*);

// Check for read  lock timeouts
extern	unsigned wg_read_lock_wait(rwlock_t* lock, unsigned wait, unsigned long pc);
// Report    read  lock not locked errors
extern	void	 wg_read_lock_not_locked(rwlock_t* lock, void* toss, unsigned long pc);

// Check for write lock timeouts
extern	unsigned wg_write_lock_wait(rwlock_t* lock, unsigned wait, unsigned long pc);
// Report    write lock not locked errors
extern	void	 wg_write_lock_not_locked(rwlock_t* lock, void* toss, unsigned long pc);

// Check for spin  lock timeouts
extern	unsigned wg_spin_lock_wait(raw_spinlock_t* lock, unsigned wait, unsigned long pc);
// Report    spin  lock not locked errors
extern	void	 wg_spin_lock_not_locked(raw_spinlock_t* lock, void* toss, unsigned long pc);

// Save counts
static __always_inline void wg_lock_count(struct wg_lock_data* data, unsigned long pc)
{
  // Bump count
  atomic_inc(&data->count);
  // Save who grabbed us
  data->grab = (void*)pc;
}

// Init lock
static __always_inline void wg_rwlock_init(rwlock_t* lock)
{
  memset(&lock->wg_lock, 0, sizeof(wglock_t));
  atomic_set(&lock->wg_lock.state, WG_LOCK_FREED);
}

// Read lock
static __always_inline void wg_read_lock(rwlock_t* lock)
{
  if (unlikely(wg_lock_debug & WG_LOCK_RD_DBG)) {
    wglock_t* lckP = &lock->wg_lock;

    // Grab read lock, look for any non-negative ref count
    if (unlikely((atomic_inc_return(&lckP->state) & WG_LOCK_WRITERS))) {
      unsigned wait = 0;

      while (1) {
        // Check for timeout
        if (unlikely((wait = wg_read_lock_wait(lock, wait, _RET_IP_)) == 0))
          return;

        // Grab read lock, look for any non-negative ref count
        if (likely(!(atomic_inc_return(&lckP->state) & WG_LOCK_WRITERS))) break;
      }
    }

    // Read counts
    wg_lock_count(&lckP->rd, _RET_IP_);
  }
}

// Read trylock
static __always_inline int  wg_read_trylock(rwlock_t* lock)
{
  if (unlikely(wg_lock_debug & WG_LOCK_RD_DBG)) {
    wglock_t* lckP = &lock->wg_lock;

    // Grab read lock, check for any writers
    if (unlikely((atomic_inc_return(&lckP->state) & WG_LOCK_WRITERS))) {
      // Check for bad locks
      wg_read_lock_wait(lock, 0, 0);

      return 0;
    }

    // Read counts
    wg_lock_count(&lckP->rd, _RET_IP_);
  }

  // Got lock
  return 1;
}

// Read unlock
static __always_inline void wg_read_unlock(rwlock_t* lock)
{
  if (unlikely(wg_lock_debug & WG_LOCK_RD_DBG)) {
    wglock_t* lckP = &lock->wg_lock;

    // Decrement the ref count
    atomic_dec(&lckP->state);

    // Check for too many unlocks
    if (unlikely(!test_bit(WG_LOCK_HI_BIT, (long*)&lckP->state)))
      wg_read_lock_not_locked(lock, lckP->rd.toss, _RET_IP_);

    // Save who tossed us
    else lckP->rd.toss = (void*)_RET_IP_;
  }
}

// Write lock
static __always_inline void wg_write_lock(rwlock_t* lock)
{
  if (unlikely(wg_lock_debug & WG_LOCK_WR_DBG)) {
    wglock_t* lckP = &lock->wg_lock;

    // Mark that we want a write lock
    set_bit(WG_LOCK_WR_BIT, (long*)&lckP->state);

    // Check for specific write lock ownership
    if (unlikely(atomic_inc_return(&lckP->state) != WG_LOCK_OWNER)) {
      unsigned wait = 0;

      while (1) {
        // Check for timeout
        if (unlikely((wait = wg_write_lock_wait(lock, wait, _RET_IP_)) == 0))
          return;

        // Mark that we want a write lock
        set_bit(WG_LOCK_WR_BIT, (long*)&lckP->state);

        // Check for specific write lock ownership
        if (likely(atomic_inc_return(&lckP->state) == WG_LOCK_OWNER)) break;
      }
    }

    // Write counts
    wg_lock_count(&lckP->wr, _RET_IP_);
  }
}

// Write trylock
static __always_inline int  wg_write_trylock(rwlock_t* lock)
{
  if (unlikely(wg_lock_debug & WG_LOCK_WR_DBG)) {
    wglock_t* lckP = &lock->wg_lock;

    // Mark that we want a try lock
    set_bit(WG_LOCK_TR_BIT, (long*)&lckP->state);

    // Check for specific write lock ownership
    if (unlikely(atomic_inc_return(&lckP->state) != WG_LOCK_QUERY)) {
      // Check for bad locks
      wg_write_lock_wait(lock, 0, 0);

      // Clear try bit
      clear_bit(WG_LOCK_TR_BIT, (long*)&lckP->state);

      return 0;
    }

    // Write counts
    wg_lock_count(&lckP->wr, _RET_IP_);

    // Set the write bit, clear the try bit
    set_bit(  WG_LOCK_WR_BIT, (long*)&lckP->state);
    clear_bit(WG_LOCK_TR_BIT, (long*)&lckP->state);
  }

  // Got lock
  return 1;
}

// Write unlock
static __always_inline void wg_write_unlock(rwlock_t* lock)
{
  if (unlikely(wg_lock_debug & WG_LOCK_WR_DBG)) {
    wglock_t* lckP = &lock->wg_lock;

    // Decrement ref count
    atomic_dec(&lckP->state);

    // Check for too many unlocks
    if (unlikely(!test_bit(WG_LOCK_HI_BIT, (long*)&lckP->state)))
      wg_write_lock_not_locked(lock, lckP->wr.toss, _RET_IP_);

    // Save who tossed us
    else lckP->wr.toss = (void*)_RET_IP_;

    // Clear write bit
    clear_bit(WG_LOCK_WR_BIT, (long*)&lckP->state);
  }
}

// Init spin
static __always_inline void wg_spin_lock_init(raw_spinlock_t* lock)
{
  memset(&lock->wg_spin, 0, sizeof(wgspin_t));
  atomic_set(&lock->wg_spin.state, WG_SPIN_FREED);
}

// Spin lock
static __always_inline void wg_spin_lock(raw_spinlock_t* lock)
{
  if (unlikely(wg_lock_debug & WG_LOCK_SP_DBG)) {
    wgspin_t* lckP = &lock->wg_spin;

    if (unlikely(atomic_inc_return(&lckP->state) != WG_SPIN_OWNER)) {
      unsigned wait = 0;

      while (1) {
        // Check for timeout
        if (unlikely((wait = wg_spin_lock_wait(lock, wait, _RET_IP_)) == 0))
          return;

        // Check for specific spin lock ownership
        if (likely(atomic_inc_return(&lckP->state) == WG_SPIN_OWNER)) break;
      }
    }

    // Write counts
    wg_lock_count(&lckP->wr, _RET_IP_);
  }
}

// Spin trylock
static __always_inline int  wg_spin_trylock(raw_spinlock_t* lock)
{
  if (unlikely(wg_lock_debug & WG_LOCK_SP_DBG)) {
    wgspin_t* lckP = &lock->wg_spin;

    // Check for specific spin  lock ownership
    if (unlikely(atomic_inc_return(&lckP->state) != WG_SPIN_OWNER)) {
      // Check for bad locks
      wg_spin_lock_wait(lock, 0, 0);

      return 0;
    }

    // Write counts
    wg_lock_count(&lckP->wr, _RET_IP_);
  }

  // Got lock
  return 1;
}

// Spin unlock
static __always_inline void wg_spin_unlock(raw_spinlock_t* lock)
{
  if (unlikely(lock->wg_spin.wr.grab)) {
    wgspin_t* lckP = &lock->wg_spin;

    // Decrement ref count
    atomic_dec(&lckP->state);

    // Check for too many unlocks
    if (unlikely(!test_bit(WG_LOCK_HI_BIT, (long*)&lckP->state)))
      wg_spin_lock_not_locked(lock, lckP->wr.toss, _RET_IP_);

    // Save who tossed us
    else lckP->wr.toss = (void*)_RET_IP_;
  }
}

// From linux/hardirq.h
// Duplicate it here since we can't pull in all of what hardirq.h
// does just to get these few symbols. If they don't match we will
// get a compiler warning.

#define PREEMPT_BITS		8
#define SOFTIRQ_BITS		8
#define HARDIRQ_BITS		10

#define PREEMPT_SHIFT		0
#define SOFTIRQ_SHIFT		(PREEMPT_SHIFT + PREEMPT_BITS)
#define HARDIRQ_SHIFT		(SOFTIRQ_SHIFT + SOFTIRQ_BITS)

#define __IRQ_MASK(x)		((1UL << (x))-1)

#define SOFTIRQ_MASK		(__IRQ_MASK(SOFTIRQ_BITS) << SOFTIRQ_SHIFT)
#define HARDIRQ_MASK		(__IRQ_MASK(HARDIRQ_BITS) << HARDIRQ_SHIFT)

// Save lock context data
static __always_inline void wg_lock_context(struct wg_lock_ctx* ctx, unsigned long pc)
{
  if (unlikely(wg_lock_debug & WG_LOCK_PC_DBG)) {
    if (unlikely( (preempt_count() & HARDIRQ_MASK))) {
      ctx->pc_irq  = (void*)pc;
      atomic_inc(&ctx->in_irq);
    }
    if (unlikely(!(preempt_count() & SOFTIRQ_MASK))) {
      ctx->pc_fork = (void*)pc;
      atomic_inc(&ctx->in_fork);
    }
  }
}

#else	// CONFIG_WG_PLATFORM_LOCK

#define	wg_rwlock_init(a)

#define wg_read_lock(a)
#define wg_read_unlock(a)
#define wg_read_trylock(a)	(1)

#define wg_write_lock(a)
#define wg_write_unlock(a)
#define wg_write_trylock(a)	(1)

#define	wg_spin_lock_init(a)

#define wg_spin_lock(a)
#define wg_spin_unlock(a)
#define wg_spin_trylock(a)	(1)

#define	wg_lock_context(a,b)

#endif	// CONFIG_WG_PLATFORM_LOCK

#endif	// _LINUX_WG_LOCK_API_H
