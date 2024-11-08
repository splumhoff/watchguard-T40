/*
 * Copyright 2005, Red Hat, Inc., Ingo Molnar
 * Released under the General Public License (GPL).
 *
 * This file contains the spinlock/rwlock implementations for
 * DEBUG_SPINLOCK.
 */

#include <linux/spinlock.h>
#include <linux/nmi.h>
#include <linux/interrupt.h>
#include <linux/debug_locks.h>
#include <linux/delay.h>
#include <linux/export.h>

void __raw_spin_lock_init(raw_spinlock_t *lock, const char *name,
			  struct lock_class_key *key)
{
#ifdef CONFIG_DEBUG_LOCK_ALLOC
	/*
	 * Make sure we are not reinitializing a held lock:
	 */
	debug_check_no_locks_freed((void *)lock, sizeof(*lock));
	lockdep_init_map(&lock->dep_map, name, key, 0);
#endif
	lock->raw_lock = (arch_spinlock_t)__ARCH_SPIN_LOCK_UNLOCKED;
	lock->magic = SPINLOCK_MAGIC;
	lock->owner = SPINLOCK_OWNER_INIT;
	lock->owner_cpu = -1;
}

EXPORT_SYMBOL(__raw_spin_lock_init);

void __rwlock_init(rwlock_t *lock, const char *name,
		   struct lock_class_key *key)
{
#ifdef CONFIG_DEBUG_LOCK_ALLOC
	/*
	 * Make sure we are not reinitializing a held lock:
	 */
	debug_check_no_locks_freed((void *)lock, sizeof(*lock));
	lockdep_init_map(&lock->dep_map, name, key, 0);
#endif
	lock->raw_lock = (arch_rwlock_t) __ARCH_RW_LOCK_UNLOCKED;
	lock->magic = RWLOCK_MAGIC;
	lock->owner = SPINLOCK_OWNER_INIT;
	lock->owner_cpu = -1;
}

EXPORT_SYMBOL(__rwlock_init);

static void spin_dump(raw_spinlock_t *lock, const char *msg)
{
	struct task_struct *owner = NULL;

	if (lock->owner && lock->owner != SPINLOCK_OWNER_INIT)
		owner = lock->owner;
	printk(KERN_EMERG "BUG: spinlock %s on CPU#%d, %s/%d\n",
		msg, raw_smp_processor_id(),
		current->comm, task_pid_nr(current));
	printk(KERN_EMERG " lock: %pS, .magic: %08x, .owner: %s/%d, "
			".owner_cpu: %d\n",
		lock, lock->magic,
		owner ? owner->comm : "<none>",
		owner ? task_pid_nr(owner) : -1,
		lock->owner_cpu);
	dump_stack();
}

#ifdef	CONFIG_WG_PLATFORM

#define	CONFIG_WG_PLATFORM_SPINLOCK_DEBUG 1

unsigned long LockRA[NR_CPUS];	// WG:JB Lock return address
EXPORT_SYMBOL(LockRA);

#endif	// CONFIG_WG_PLATFORM

#ifdef	CONFIG_WG_PLATFORM_SPINLOCK_DEBUG

#include <linux/kallsyms.h>

#undef	SPINLOCK_MAGIC
#define	SPINLOCK_MAGIC	(lock->magic)

#undef	RWLOCK_MAGIC
#define	RWLOCK_MAGIC	(lock->magic)

#define	SPINLOCK_MASK	0x1FFFFFF
#define	READLOCK_MASK	0x1FFFFFF
#define	WRITELOCK_MASK	0x1FFFFFF

#endif	// CONFIG_WG_PLATFORM_SPINLOCK_DEBUG

static void spin_bug(raw_spinlock_t *lock, const char *msg)
{
	if (!debug_locks_off())
		return;

	spin_dump(lock, msg);
}

#define SPIN_BUG_ON(cond, lock, msg) if (unlikely(cond)) spin_bug(lock, msg)

static inline void
debug_spin_lock_before(raw_spinlock_t *lock)
{
	SPIN_BUG_ON(lock->magic != SPINLOCK_MAGIC, lock, "bad magic");
	SPIN_BUG_ON(lock->owner == current, lock, "recursion");
	SPIN_BUG_ON(lock->owner_cpu == raw_smp_processor_id(),
							lock, "cpu recursion");
}

static inline void debug_spin_lock_after(raw_spinlock_t *lock)
{
	lock->owner_cpu = raw_smp_processor_id();
	lock->owner = current;
}

static inline void debug_spin_unlock(raw_spinlock_t *lock)
{
	SPIN_BUG_ON(lock->magic != SPINLOCK_MAGIC, lock, "bad magic");
	SPIN_BUG_ON(!raw_spin_is_locked(lock), lock, "already unlocked");
	SPIN_BUG_ON(lock->owner != current, lock, "wrong owner");
	SPIN_BUG_ON(lock->owner_cpu != raw_smp_processor_id(),
							lock, "wrong CPU");
	lock->owner = SPINLOCK_OWNER_INIT;
	lock->owner_cpu = -1;
}

#ifdef	CONFIG_WG_PLATFORM_SPINLOCK_DEBUG

// WG:JB Save lock return address function
static inline void WG_SPIN_PC(unsigned long pc)
{
	if (!in_lock_functions(pc)) {
#if NR_CPUS > 1
		LockRA[(unsigned)raw_smp_processor_id()] = pc;
#else
		LockRA[0] = pc;
#endif
	}
}

static void __spin_lock_debug_pc(raw_spinlock_t *lock, unsigned pc)
{
	u64 i = 0;

	WG_SPIN_PC(pc); /* WG:JB Save lock return address */

	while (1) {
		if (arch_spin_trylock(&lock->raw_lock))
			return;
		if (((++i & SPINLOCK_MASK) == 0)) {
			console_loglevel = 8;
			printk("\n");
			printk("%s: lockup on CPU#%d %s/%d lock %p from %p\n",
			       __FUNCTION__,
			       raw_smp_processor_id(), current->comm,
			       current->pid, lock, (void*)lock->magic);
			print_symbol("lock %s, ", (int)lock);
			print_symbol("function %s\n\n", lock->magic);
			dump_stack();
		}

		__delay(1);
	}
}

#endif	// CONFIG_WG_PLATFORM_SPINLOCK_DEBUG

/*
 * We are now relying on the NMI watchdog to detect lockup instead of doing
 * the detection here with an unfair lock which can cause problem of its own.
 */
void do_raw_spin_lock(raw_spinlock_t *lock)
{
	debug_spin_lock_before(lock);
#ifdef	CONFIG_WG_PLATFORM_SPINLOCK_DEBUG
	if (unlikely(!arch_spin_trylock(&lock->raw_lock)))
		__spin_lock_debug_pc(lock, _RET_IP_);
	else
		lock->magic = _RET_IP_;
#else
	arch_spin_lock(&lock->raw_lock);
#endif	// CONFIG_WG_PLATFORM_SPINLOCK_DEBUG
	debug_spin_lock_after(lock);
}

int do_raw_spin_trylock(raw_spinlock_t *lock)
{
	int ret = arch_spin_trylock(&lock->raw_lock);

	if (ret)
		debug_spin_lock_after(lock);
#ifndef CONFIG_SMP
	/*
	 * Must not happen on UP:
	 */
	SPIN_BUG_ON(!ret, lock, "trylock failure on UP");
#endif
	return ret;
}

void do_raw_spin_unlock(raw_spinlock_t *lock)
{
	debug_spin_unlock(lock);
	arch_spin_unlock(&lock->raw_lock);
}

static void rwlock_bug(rwlock_t *lock, const char *msg)
{
	if (!debug_locks_off())
		return;

	printk(KERN_EMERG "BUG: rwlock %s on CPU#%d, %s/%d, %p\n",
		msg, raw_smp_processor_id(), current->comm,
		task_pid_nr(current), lock);
	dump_stack();
}

#define RWLOCK_BUG_ON(cond, lock, msg) if (unlikely(cond)) rwlock_bug(lock, msg)

#ifdef	CONFIG_WG_PLATFORM_SPINLOCK_DEBUG
static void __read_lock_debug_pc(rwlock_t *lock, unsigned pc)
{
	u64 i = 0;

	while (1) {
		if (arch_read_trylock(&lock->raw_lock))
			return;
		if (((++i & READLOCK_MASK) == 0)) {
			console_loglevel = 8;
			printk("\n");
			printk("%s: lockup on CPU#%d %s/%d lock %p from %p\n",
			       __FUNCTION__,
			       raw_smp_processor_id(), current->comm,
			       current->pid, lock, (void*)lock->magic);
			print_symbol("lock %s, ", (int)lock);
			print_symbol("function %s\n\n", lock->magic);
			dump_stack();
		}

		__delay(1);
	}
}

#endif	// CONFIG_WG_PLATFORM_SPINLOCK_DEBUG

void do_raw_read_lock(rwlock_t *lock)
{
	RWLOCK_BUG_ON(lock->magic != RWLOCK_MAGIC, lock, "bad magic");
#ifdef	CONFIG_WG_PLATFORM_SPINLOCK_DEBUG
	if (unlikely(!arch_read_trylock(&lock->raw_lock)))
		__read_lock_debug_pc(lock, _RET_IP_);
	else
		lock->magic = _RET_IP_;
#else
	arch_read_lock(&lock->raw_lock);
#endif	// CONFIG_WG_PLATFORM_SPINLOCK_DEBUG
}

int do_raw_read_trylock(rwlock_t *lock)
{
	int ret = arch_read_trylock(&lock->raw_lock);

#ifndef CONFIG_SMP
	/*
	 * Must not happen on UP:
	 */
	RWLOCK_BUG_ON(!ret, lock, "trylock failure on UP");
#endif
	return ret;
}

void do_raw_read_unlock(rwlock_t *lock)
{
	RWLOCK_BUG_ON(lock->magic != RWLOCK_MAGIC, lock, "bad magic");
	arch_read_unlock(&lock->raw_lock);
}

static inline void debug_write_lock_before(rwlock_t *lock)
{
	RWLOCK_BUG_ON(lock->magic != RWLOCK_MAGIC, lock, "bad magic");
	RWLOCK_BUG_ON(lock->owner == current, lock, "recursion");
	RWLOCK_BUG_ON(lock->owner_cpu == raw_smp_processor_id(),
							lock, "cpu recursion");
}

static inline void debug_write_lock_after(rwlock_t *lock)
{
	lock->owner_cpu = raw_smp_processor_id();
	lock->owner = current;
}

static inline void debug_write_unlock(rwlock_t *lock)
{
	RWLOCK_BUG_ON(lock->magic != RWLOCK_MAGIC, lock, "bad magic");
	RWLOCK_BUG_ON(lock->owner != current, lock, "wrong owner");
	RWLOCK_BUG_ON(lock->owner_cpu != raw_smp_processor_id(),
							lock, "wrong CPU");
	lock->owner = SPINLOCK_OWNER_INIT;
	lock->owner_cpu = -1;
}

#ifdef	CONFIG_WG_PLATFORM_SPINLOCK_DEBUG
static void __write_lock_debug_pc(rwlock_t *lock, unsigned pc)
{
	u64 i = 0;

	while (1) {
		if (arch_write_trylock(&lock->raw_lock))
			return;
		if (((++i & WRITELOCK_MASK) == 0)) {
			console_loglevel = 8;
			printk("\n");
			printk("%s: lockup on CPU#%d %s/%d lock %p from %p\n",
			       __FUNCTION__,
			       raw_smp_processor_id(), current->comm,
			       current->pid, lock, (void*)lock->magic);
			print_symbol("lock %s, ", (int)lock);
			print_symbol("function %s\n\n", lock->magic);
			dump_stack();
		}

		__delay(1);
	}
}
#endif	// CONFIG_WG_PLATFORM_SPINLOCK_DEBUG

void do_raw_write_lock(rwlock_t *lock)
{
	debug_write_lock_before(lock);
#ifdef	CONFIG_WG_PLATFORM_SPINLOCK_DEBUG
	if (unlikely(!arch_write_trylock(&lock->raw_lock)))
		__write_lock_debug_pc(lock, _RET_IP_);
	else
		lock->magic = _RET_IP_;
#else
	arch_write_lock(&lock->raw_lock);
#endif	// CONFIG_WG_PLATFORM_SPINLOCK_DEBUG
	debug_write_lock_after(lock);
}

int do_raw_write_trylock(rwlock_t *lock)
{
	int ret = arch_write_trylock(&lock->raw_lock);

	if (ret)
		debug_write_lock_after(lock);
#ifndef CONFIG_SMP
	/*
	 * Must not happen on UP:
	 */
	RWLOCK_BUG_ON(!ret, lock, "trylock failure on UP");
#endif
	return ret;
}

void do_raw_write_unlock(rwlock_t *lock)
{
	debug_write_unlock(lock);
	arch_write_unlock(&lock->raw_lock);
}
