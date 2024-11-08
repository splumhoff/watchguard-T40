#ifndef	CONFIG_CRASH_DUMP

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/skbuff.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/kthread.h>
#include <linux/kernel_stat.h>
#include <linux/kallsyms.h>
#include <linux/ptrace.h>
#include <linux/netdevice.h>
#include <linux/netfilter_ipv4.h>
#include <linux/errno.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/device.h>
#include <linux/profile.h>
#include <linux/spinlock.h>
#include <net/ip.h>
#include <net/xfrm.h>

struct proc_dir_entry* proc_wg_debug_dir;

// Add entries like this one for additional debug knobs

#undef	_WG_DEBUG_EXAMPLE_
#ifdef	_WG_DEBUG_EXAMPLE_

int wg_example;

struct proc_dir_entry* proc_wg_example_file;

int proc_write_wg_example(struct file *file, const char *buffer,
                          unsigned long count, void *data)
{
  char str[256], *strend;

  if (count > (sizeof(str)-1)) count = sizeof(str)-1;
  if(copy_from_user(str, buffer, count))
    return -EFAULT;

  str[count] = '\0';

  wg_example = simple_strtol(str, &strend, 0);

  return count;
}

int proc_read_wg_example(char *page, char **start, off_t off,
                         int count, int *eof, void *data)
{
  return sprintf(page, "%d\n", wg_example);
}

#endif

struct proc_dir_entry* proc_wg_napi_poll_file;

int proc_read_wg_napi_poll(char *page, char **start, off_t off,
                           int count, int *eof, void *data)
{
  int j, z = 0;

  for (j = 0; j < num_online_cpus(); j++) {
    struct softnet_data* sd   = &per_cpu(softnet_data, j);
    struct list_head*    head = &(sd->poll_list);
    struct list_head*    this = head;

    if (head->next == head->prev) continue;

    z += sprintf(&page[z], "CPU %2d Polling", j);

    while ((this = this->next) != head)
    if (!this) break; else {
      struct napi_struct* napi = (struct napi_struct*)this;
      if (napi->dev)
        z += sprintf(&page[z], " %s", napi->dev->name);
    }

    z += sprintf(&page[z], "\n");
  }

  return z;
}

#ifdef	CONFIG_WG_PLATFORM_LOCK

// Wait timeout mask
extern	unsigned wg_lock_warn;

// Wait timeout fail point
extern	unsigned wg_lock_fail;

// Locks debugging not locked max
extern	unsigned wg_lock_nl_max;

// Locks debugging free  lock max
extern	unsigned wg_lock_lk_max;

// List of locks we are debugging
extern	struct list_head wg_locks;

// Remove lock being debuged from wg_locks
extern	void wg_lock_debug_remove(struct wg_list_item*);

DEFINE_RWLOCK(wg_lock_test);
DEFINE_SPINLOCK(wg_spin_test);

// Code examples of spin lock
void noinline code_spin_lock(spinlock_t* lock)
{
  spin_lock(lock);
}

void noinline code_spin_unlock(spinlock_t* lock)
{
  spin_unlock(lock);
}

// Actual functions

int  noinline _test_read_trylock(void)
{
  return wg_read_trylock( &wg_lock_test);
}

void noinline _test_read_lock(void)
{
  wg_read_lock(           &wg_lock_test);
}

void noinline _test_read_unlock(void)
{
  wg_read_unlock(         &wg_lock_test);
}


int  noinline _test_write_trylock(void)
{
  return wg_write_trylock(&wg_lock_test);
}

void noinline _test_write_lock(void)
{
  wg_write_lock(          &wg_lock_test);
}

void noinline _test_write_unlock(void)
{
  wg_write_unlock(        &wg_lock_test);
}


int  noinline _test_spin_trylock(void)
{
  return wg_spin_trylock( &wg_spin_test.rlock);
}

void noinline _test_spin_lock(void)
{
  wg_spin_lock(           &wg_spin_test.rlock);
}

void noinline _test_spin_unlock(void)
{
  wg_spin_unlock(         &wg_spin_test.rlock);
}

// Visible functions

int  noinline test_read_trylock(void)
{
  if (_test_read_trylock()) return 1;
  return 0;
}

int  noinline test_read_lock(void)
{
  _test_read_lock();
  return 0;
}

int  noinline test_read_unlock(void)
{
  _test_read_unlock();
  return 0;
}


int  noinline test_write_trylock(void)
{
  if (_test_write_trylock()) return 1;
  return 0;
}

int  noinline test_write_lock(void)
{
  _test_write_lock();
  return 0;
}

int  noinline test_write_unlock(void)
{
  _test_write_unlock();
  return 0;
}


int  noinline test_spin_trylock(void)
{
  if (_test_spin_trylock()) return 1;
  return 0;
}

int  noinline test_spin_lock(void)
{
  _test_spin_lock();
  return 0;
}

int  noinline test_spin_unlock(void)
{
  _test_spin_unlock();
  return 0;
}

// Run the lock tests

#define	COUNT	1000000000

int running = 0;
int threads = 0;

int wg_lock_run_read_lock(void* arg)
{
  while (running) {
    if (atomic_get(&wg_lock_test.wg_lock.rd.count) >= COUNT) running = 0;
    if (atomic_get(&wg_lock_test.wg_lock.wr.count) >= COUNT) running = 0;
    preempt_disable();
    test_read_lock();
    test_read_unlock();
    preempt_enable();
    yield();
  }

  threads--;

  return 0;
}

int wg_lock_run_write_lock(void* arg)
{
  while (running) {
    if (atomic_get(&wg_lock_test.wg_lock.rd.count) >= COUNT) running = 0;
    if (atomic_get(&wg_lock_test.wg_lock.wr.count) >= COUNT) running = 0;
    preempt_disable();
    test_write_lock();
    test_write_unlock();
    preempt_enable();
    yield();
  }

  threads--;

  return 0;
}

int wg_lock_run_write_trylock(void* arg)
{
  while (running) {
    if (atomic_get(&wg_lock_test.wg_lock.rd.count) >= COUNT) running = 0;
    if (atomic_get(&wg_lock_test.wg_lock.wr.count) >= COUNT) running = 0;
    preempt_disable();
    if (test_write_trylock()) test_write_unlock();
    preempt_enable();
    yield();
  }

  threads--;

  return 0;
}

void wg_lock_run_test(void)
{
  wg_lock_debug_clear(&wg_lock_test.wg_lock);
  wg_lock_debug_enable(&wg_lock_test.wg_lock.list);

  running++;

  wake_up_process(kthread_create(wg_lock_run_read_lock,     0, "read0"));
  wake_up_process(kthread_create(wg_lock_run_read_lock,     0, "read1"));
  wake_up_process(kthread_create(wg_lock_run_write_lock,    0, "write"));
  wake_up_process(kthread_create(wg_lock_run_write_trylock, 0, "try"));

  threads = 4;
}

struct proc_dir_entry* proc_wg_locks_dir;
struct proc_dir_entry* proc_wg_locks_file;
struct proc_dir_entry* proc_wg_exceptions_file;
struct proc_dir_entry* proc_wg_debug_locks_file;
struct proc_dir_entry* proc_wg_fail_time_file;
struct proc_dir_entry* proc_wg_warn_time_file;
struct proc_dir_entry* proc_wg_reset_log_max_file;
struct proc_dir_entry* proc_wg_under_log_max_file;

int proc_write_wg_fail_time(struct file *file, const char *buffer,
                            unsigned long count, void *data)
{
  unsigned x;
  char str[256], *strend;

  if (count > (sizeof(str)-1)) count = sizeof(str)-1;
  if(copy_from_user(str, buffer, count))
    return -EFAULT;

  str[count] = '\0';

  x = simple_strtoul(str, &strend, 16);
  if (x >= 0x1000000)
  if (((x - 1) & x) == 0)
    wg_lock_fail = x;

  return count;
}

int proc_read_wg_fail_time(char *page, char **start, off_t off,
                           int count, int *eof, void *data)
{
  return sprintf(page, "%x\n", wg_lock_fail);
}

int proc_write_wg_warn_time(struct file *file, const char *buffer,
                            unsigned long count, void *data)
{
  unsigned x;
  char str[256], *strend;

  if (count > (sizeof(str)-1)) count = sizeof(str)-1;
  if(copy_from_user(str, buffer, count))
    return -EFAULT;

  str[count] = '\0';

  x = simple_strtoul(str, &strend, 16);
  if (x >= 0x0FFFFFF)
  if (((x + 1) & x) == 0)
    wg_lock_warn = x;

  return count;
}

int proc_read_wg_warn_time(char *page, char **start, off_t off,
                           int count, int *eof, void *data)
{
  return sprintf(page, "%x\n", wg_lock_warn);
}

int proc_write_wg_reset_log_max(struct file *file, const char *buffer,
                                unsigned long count, void *data)
{
  char str[256], *strend;

  if (count > (sizeof(str)-1)) count = sizeof(str)-1;
  if(copy_from_user(str, buffer, count))
    return -EFAULT;

  str[count] = '\0';

  wg_lock_lk_max = simple_strtol(str, &strend, 0);

  return count;
}

int proc_read_wg_reset_log_max(char *page, char **start, off_t off,
                           int count, int *eof, void *data)
{
  return sprintf(page, "%d\n", wg_lock_lk_max);
}

int proc_write_wg_under_log_max(struct file *file, const char *buffer,
                                unsigned long count, void *data)
{
  char str[256], *strend;

  if (count > (sizeof(str)-1)) count = sizeof(str)-1;
  if(copy_from_user(str, buffer, count))
    return -EFAULT;

  str[count] = '\0';

  wg_lock_nl_max = simple_strtol(str, &strend, 0);

  return count;
}

int proc_read_wg_under_log_max(char *page, char **start, off_t off,
                           int count, int *eof, void *data)
{
  return sprintf(page, "%d\n", wg_lock_nl_max);
}

int proc_wg_locks_show(struct seq_file *m, void *v)
{
  char sym[KSYM_SYMBOL_LEN];
  struct list_head* this = &wg_locks;
  struct list_head* next = this->next;
  struct list_head* item = NULL;

  while ((this = next))
  if (this == &wg_locks)
    break;
  else {
    wglock_t* lock;
    unsigned  state;
    int       users;
    char*     type  = "None ";

    next = this->next;

    if (!(item = ((struct wg_list_item*)this)->item)) {
      wg_lock_debug_remove((struct wg_list_item*)this);
      continue;
    }

    if ((item->prev != this) || (item->next != this)) {
      wg_lock_debug_remove((struct wg_list_item*)this);
      continue;
    }

    lock  = container_of(item, wglock_t, list);
    state = atomic_get(&lock->state);
    if   (state & WG_LOCK_RD_MASK) {
      type = "Read ";
      if (state & WG_LOCK_WR_MASK) type = "Write";
      if (state & WG_LOCK_TR_MASK) type = "Try  ";
    } else
    if   (state & WG_LOCK_WR_MASK) {
      type = "Spin ";
    }

    users = state & WG_LOCK_STATE;
    if (users &  (WG_LOCK_HI_MASK >> 1))
        users |= ~WG_LOCK_HI_MASK;

    seq_printf(m, "\n");

    sprint_symbol(sym, (unsigned long)lock);
    seq_printf(m, "Lock          [%p] %s\n", lock, sym);
    seq_printf(m, "  State %s %10d\n",    type, users);

    if (*type != 'S') {
    if (atomic_get(&lock->rd.count))
    seq_printf(m, "  Read  Count %10u\n", atomic_get(&lock->rd.count));
    if (atomic_get(&lock->rd.stuck))
    seq_printf(m, "  Read  Stuck %10u\n", atomic_get(&lock->rd.stuck));
    if (atomic_get(&lock->rd.reset))
    seq_printf(m, "  Read  Reset %10u\n", atomic_get(&lock->rd.reset));
    if (atomic_get(&lock->rd.under))
    seq_printf(m, "  Read  Under %10u\n", atomic_get(&lock->rd.under));
    if (atomic_get(&lock->rd.spin))
    seq_printf(m, "  Read  Spin  %10u\n", atomic_get(&lock->rd.spin));
    if (lock->rd.high)
    seq_printf(m, "  Read  High  %10u\n", lock->rd.high);
    }

    if (atomic_get(&lock->wr.count))
    seq_printf(m, "  Write Count %10u\n", atomic_get(&lock->wr.count));
    if (atomic_get(&lock->wr.stuck))
    seq_printf(m, "  Write Stuck %10u\n", atomic_get(&lock->wr.stuck));
    if (atomic_get(&lock->wr.reset))
    seq_printf(m, "  Write Reset %10u\n", atomic_get(&lock->wr.reset));
    if (atomic_get(&lock->wr.under))
    seq_printf(m, "  Write Under %10u\n", atomic_get(&lock->wr.under));
    if (atomic_get(&lock->wr.spin))
    seq_printf(m, "  Write Spin  %10u\n", atomic_get(&lock->wr.spin));
    if (lock->wr.high)
    seq_printf(m, "  Write High  %10u\n", lock->wr.high);

    if (atomic_get(&lock->ctx.in_irq))
    seq_printf(m, "  In    IRQ   %10u\n", atomic_get(&lock->ctx.in_irq));
    if (atomic_get(&lock->ctx.in_fork))
    seq_printf(m, "  In    Fork  %10u\n", atomic_get(&lock->ctx.in_fork));

    if (*type != 'S') {
    if (lock->rd.grab) {
      sprint_symbol(sym,  (unsigned long)(lock->rd.grab));
      seq_printf(m, "  Read  Grab  [%p] %s\n", lock->rd.grab, sym);
    }
    if (lock->rd.toss) {
      sprint_symbol(sym,  (unsigned long)(lock->rd.toss));
      seq_printf(m, "  Read  Toss  [%p] %s\n", lock->rd.toss, sym);
    }
    }

    if (lock->wr.grab) {
      sprint_symbol(sym,  (unsigned long)(lock->wr.grab));
      seq_printf(m, "  Write Grab  [%p] %s\n", lock->wr.grab, sym);
    }
    if (lock->wr.toss) {
      sprint_symbol(sym,  (unsigned long)(lock->wr.toss));
      seq_printf(m, "  Write Toss  [%p] %s\n", lock->wr.toss, sym);
    }

    if (lock->ctx.pc_irq) {
      sprint_symbol(sym,  (unsigned long)(lock->ctx.pc_irq));
      seq_printf(m, "  PC    IRQ   [%p] %s\n", lock->ctx.pc_irq,  sym);
    }
    if (lock->ctx.pc_fork) {
      sprint_symbol(sym,  (unsigned long)(lock->ctx.pc_fork));
      seq_printf(m, "  PC    Fork  [%p] %s\n", lock->ctx.pc_fork, sym);
    }
  }

  if (item) seq_printf(m, "\n");

  return 0;
}

int proc_wg_locks_open(struct inode *inode, struct file *file)
{
  return single_open(file, proc_wg_locks_show, NULL);
}

struct file_operations proc_wg_locks_operations = {
  .open    = proc_wg_locks_open,
  .read    = seq_read,
  .llseek  = seq_lseek,
  .release = single_release,
};

int proc_wg_exceptions_show(struct seq_file *m, void *v)
{
  wg_lock_debug_print(m);
  return 0;
}

int proc_wg_exceptions_open(struct inode *inode, struct file *file)
{
  return single_open(file, proc_wg_exceptions_show, NULL);
}

struct file_operations proc_wg_exceptions_operations = {
  .open    = proc_wg_exceptions_open,
  .read    = seq_read,
  .llseek  = seq_lseek,
  .release = single_release,
};

int proc_wg_debug_locks(struct file *file, const char *buffer,
                        unsigned long count, void *data)
{
  int  sign = 0;
  char str[256], *strend;

  if (count > (sizeof(str)-1)) count = sizeof(str)-1;
  if(copy_from_user(str, buffer, count))
    return -EFAULT;

  str[count] = '\0';

  if (str[0] == '+')
    sign =  1;
  else
  if (str[0] == '-')
    sign = -1;
  else
  if (str[0] <  ' ')
    return count;
  else
  if (str[0] >  ' ') {

    if (((str[0] >= '0') && (str[0] <= '9')) ||
        ((str[0] >= 'a') && (str[0] <= 'f')))
      wg_lock_debug = simple_strtoul(&str[0], &strend, 16);
    else
    if (str[0] == 't')
      wg_lock_run_test();
    else
    if (str[0] == 'h')
      running = 0;
    else
    if ((str[0] == 'x') || (str[0] == 'z')) {
      struct list_head* this = &wg_locks;
      struct list_head* next = this->next;

      while ((this = next))
        if (this == &wg_locks)
          break;
        else {
          next = this->next;
          if (str[0] == 'x')
            wg_lock_debug_remove((struct wg_list_item*)this);
          else {
            struct list_head* item = ((struct wg_list_item*)this)->item;
            if (item)
              wg_lock_debug_clear(container_of(item, wglock_t, list));
          }
        }
    }

    return count;
  }

  if (str[1] == 'r') {
    if (sign)     wg_lock_debug_enable(&wg_lock_test.wg_lock.list);
    if (sign > 0) test_read_lock(); else
    if (sign < 0) test_read_unlock();
  }
  else
  if (str[1] == 'w') {
    if (sign)     wg_lock_debug_enable(&wg_lock_test.wg_lock.list);
    if (sign > 0) test_write_lock(); else
    if (sign < 0) test_write_unlock();
  }
  else
  if (str[1] == 's') {
    if (sign)     wg_lock_debug_enable(&wg_spin_test.rlock.wg_spin.list);
    if (sign > 0) test_spin_lock(); else
    if (sign < 0) test_spin_unlock();
  }
  else
  if ((str[1] >= 'A') && (str[1] <= 'Z')) {
    unsigned long    value = simple_strtoul(&str[2], &strend, 16);
    rwlock_t*     rw_lockP = (rwlock_t*)value;
    spinlock_t* spin_lockP = (spinlock_t*)value;

    if (!value) return count;

    if ((str[1] == 'R') || (str[1] == 'W')) {
      if (sign >  0)
        wg_lock_debug_enable(&rw_lockP->wg_lock.list);
      else
      if (sign <  0)
        wg_lock_debug_disable(&rw_lockP->wg_lock.list);
      else
      if (sign == 0)
        wg_lock_debug_clear(&rw_lockP->wg_lock);
    }
    else
    if ((str[1] == 'S')) {
      if (sign >  0)
        wg_lock_debug_enable(&spin_lockP->rlock.wg_spin.list);
      else
      if (sign <  0)
        wg_lock_debug_disable(&spin_lockP->rlock.wg_spin.list);
      else
      if (sign == 0)
        wg_lock_debug_clear((wglock_t*)&spin_lockP->rlock.wg_spin);
    }
  }

  return count;
}

#endif

// Set up the proc filesystem

int __init wg_debug_init(void)
{
  printk(KERN_INFO "\n%s: Built " __DATE__ " " __TIME__ " CPUs %d\n\n",
         __FUNCTION__, num_online_cpus());

  // Create /proc/wg_debug
  proc_wg_debug_dir = proc_mkdir("wg_debug", NULL);
  if (!proc_wg_debug_dir) return -EPERM;

#ifdef	_WG_DEBUG_EXAMPLE_
  // Create each example entry
  proc_wg_example_file = create_proc_entry("example",
                                           0644, proc_wg_debug_dir);
  if (proc_wg_example_file) {
      proc_wg_example_file->read_proc    = proc_read_wg_example;
      proc_wg_example_file->write_proc   = proc_write_wg_example;
  }
#endif

  // Create each napi_poll entry
  proc_wg_napi_poll_file = create_proc_entry("napi_poll",
                                             0444, proc_wg_debug_dir);
  if (proc_wg_napi_poll_file) {
      proc_wg_napi_poll_file->read_proc  = proc_read_wg_napi_poll;
  }

#ifdef	CONFIG_WG_PLATFORM_LOCK
  {
    atomic_t x;

    atomic_set(&x, WG_LOCK_RD_MASK);

    if (*((long*)&x) != WG_LOCK_RD_MASK) {
      printk(KERN_INFO "%s: Atomic values not %2d bits\n",
             __FUNCTION__, ((int)sizeof(atomic_t)) * 8);
      return -EINVAL;
    }

    // Create /proc/wg_locks
    proc_wg_locks_dir = proc_mkdir("wg_locks", NULL);
    if (!proc_wg_locks_dir) return -EPERM;


    proc_wg_fail_time_file = create_proc_entry("fail_time",
                                               0644, proc_wg_locks_dir);
    if (proc_wg_fail_time_file) {
      proc_wg_fail_time_file->read_proc  = proc_read_wg_fail_time;
      proc_wg_fail_time_file->write_proc = proc_write_wg_fail_time;
    }

    proc_wg_warn_time_file = create_proc_entry("warn_time",
                                               0644, proc_wg_locks_dir);
    if (proc_wg_warn_time_file) {
      proc_wg_warn_time_file->read_proc  = proc_read_wg_warn_time;
      proc_wg_warn_time_file->write_proc = proc_write_wg_warn_time;
    }

    proc_wg_reset_log_max_file = create_proc_entry("reset_log_max",
                                                   0644, proc_wg_locks_dir);
    if (proc_wg_reset_log_max_file) {
      proc_wg_reset_log_max_file->read_proc  = proc_read_wg_reset_log_max;
      proc_wg_reset_log_max_file->write_proc = proc_write_wg_reset_log_max;
    }

    proc_wg_under_log_max_file = create_proc_entry("under_log_max",
                                                   0644, proc_wg_locks_dir);
    if (proc_wg_under_log_max_file) {
      proc_wg_under_log_max_file->read_proc  = proc_read_wg_under_log_max;
      proc_wg_under_log_max_file->write_proc = proc_write_wg_under_log_max;
    }


    proc_wg_locks_file = create_proc_entry("locks",
                                           0444, proc_wg_locks_dir);
    if (proc_wg_locks_file) {
      proc_wg_locks_file->proc_fops = &proc_wg_locks_operations;
    }

    proc_wg_exceptions_file = create_proc_entry("exceptions",
                                                0444, proc_wg_locks_dir);
    if (proc_wg_exceptions_file) {
      proc_wg_exceptions_file->proc_fops = &proc_wg_exceptions_operations;
    }

    proc_wg_debug_locks_file = create_proc_entry("debug_locks",
                                                 0200, proc_wg_locks_dir);
    if (proc_wg_debug_locks_file) {
      proc_wg_debug_locks_file->write_proc = proc_wg_debug_locks;
    }

    write_lock(&wg_lock_test);
    spin_lock(&wg_spin_test);
  }
#endif

  // Return no errors
  return 0;
}

void __exit wg_debug_exit(void)
{
  // Remove /proc entries

  if (proc_wg_napi_poll_file)
    remove_proc_entry("napi_poll",      proc_wg_debug_dir);

#ifdef	_WG_DEBUG_EXAMPLE_
  if (proc_wg_example_file)
    remove_proc_entry("example",        proc_wg_debug_dir);
#endif

  if (proc_wg_debug_dir)
    remove_proc_entry("wg_debug",       NULL);

#ifdef	CONFIG_WG_PLATFORM_LOCK
  running = 0;

  wg_lock_debug_disable(&wg_lock_test.wg_lock.list);
  wg_lock_debug_disable(&wg_spin_test.rlock.wg_spin.list);

  if (proc_wg_fail_time_file)
    remove_proc_entry("fail_time",	proc_wg_locks_dir);
  if (proc_wg_warn_time_file)
    remove_proc_entry("warn_time",	proc_wg_locks_dir);
  if (proc_wg_reset_log_max_file)
    remove_proc_entry("reset_log_max",	proc_wg_locks_dir);
  if (proc_wg_under_log_max_file)
    remove_proc_entry("under_log_max",	proc_wg_locks_dir);

  if (proc_wg_locks_file)
    remove_proc_entry("locks",		proc_wg_locks_dir);
  if (proc_wg_exceptions_file)
    remove_proc_entry("exceptions",	proc_wg_locks_dir);
  if (proc_wg_debug_locks_file)
    remove_proc_entry("debug_locks",	proc_wg_locks_dir);

  if (proc_wg_locks_dir)
    remove_proc_entry("wg_locks",	NULL);

  while (threads > 0) yield();
#endif
}

module_init(wg_debug_init);
module_exit(wg_debug_exit);

MODULE_LICENSE("GPL");

#endif	// !CONFIG_CRASH_DUMP
