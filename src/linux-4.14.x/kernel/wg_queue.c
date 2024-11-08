#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>
#include <linux/netdevice.h>

#define	WG_QUEUE_LIMIT		100000

#define	WG_QUEUE_NETPOLL	on
#define	WG_QUEUE_COUNTERS	on

// dummy src file to avoid further changes in .pkgspec
