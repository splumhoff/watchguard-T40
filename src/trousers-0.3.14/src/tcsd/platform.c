
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004, 2005
 *
 */


#if (defined (__FreeBSD__) || defined (__OpenBSD__) || defined (__APPLE__))
#include <sys/param.h>
#include <sys/sysctl.h>
#include <err.h>
#elif (defined (__linux) || defined (linux) || defined(__GLIBC__))
#include <utmp.h>
#elif (defined (SOLARIS))
#include <utmpx.h>
#endif

#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "trousers/tss.h"
#include "trousers_types.h"
#include "tcs_tsp.h"
#include "tcs_int_literals.h"
#include "capabilities.h"
#include "tcsps.h"
#include "tcslog.h"


#if (defined (__linux) || defined (linux) || defined(__GLIBC__))
MUTEX_DECLARE_INIT(utmp_lock);

/*
 * What was platform_get_runlevel() doing?
 *  Detects which runlevel is the user running on.
 *  It was working as an aide to dis-allow certain operations under normal
 *    operating mode.
 *
 *  ANY WHY DISALLOW: To comply with TPM Spec 1.1 'TCG unaware BIOS' which states
 *    that user has to be in physical presence(NO REMOTE ACCESS, NO NETWORK,
 *    INIT/SH on tty console) to perform such operations.
 *
 *  AND WHY IT DOES NOT MATTER ANY MORE: Because Single USER MODE is not what it
 *    used be. Single User mode now-a-days means none of that(it has remote, network
 *    and all processes still running). They just fork a new process and setuid on
 *    the child and exec an '/sbin/init' or '/bin/sh' from there.
 *
 *  GIST-NOTE: This does not matter in modern Linux Distros anymore because
 *    Single User Mode is not what originally intended to be.
 */
char
platform_get_runlevel()
{

	char runlevel;
	struct utmp ut, save, *next = NULL;
	struct timeval tv;
	int flag = 0, counter = 0;

	MUTEX_LOCK(utmp_lock);

	memset(&ut, 0, sizeof(struct utmp));
	memset(&save, 0, sizeof(struct utmp));
	memset(&tv, 0, sizeof(struct timeval));

	ut.ut_type = RUN_LVL;

	setutent();
	next = getutid(&ut);

	while (next != NULL) {
		if (next->ut_tv.tv_sec > tv.tv_sec) {
			memcpy(&save, next, sizeof(*next));
			flag = 1;
		} else if (next->ut_tv.tv_sec == tv.tv_sec) {
			if (next->ut_tv.tv_usec > tv.tv_usec) {
				memcpy(&save, next, sizeof(*next));
				flag = 1;
			}
		}

		counter++;
		next = getutid(&ut);
	}

	if (flag) {
		//printf("prev_runlevel=%c, runlevel=%c\n", save.ut_pid / 256, save.ut_pid % 256);
		runlevel = save.ut_pid % 256;
	} else {
		//printf("unknown\n");
		runlevel = 'u';
	}

	MUTEX_UNLOCK(utmp_lock);

	return runlevel;
}

#define WG_UTM_OS  'U' // UTM-OS, Un-Safe, Not-allowed
#define WG_BASE_OS 'S' // Base-OS, Safe, allowed

/*
 * Description:
 *   tcsd expects some sort of security in place before we perform some sensitive
 *   operations. It wants to refrain executing physical presence operations if not
 *   under an controlled environment. 
 *
 * What is WatchGuard's such controlled env?
 *   IT is nothing but base-OS (mfgtime only).
 *					
 * Why are doing here?
 *   Replace platform_get_runlevel() with detect_wg_os()
 *    which returns the WG-OS flavor (examples: base, utm, mfgsys) in a way that
 *    does not alter the opensrc function.
 *
 * What does it do?
 *   Detects WG-OS and return OS info as below so that the caller can make a decision based on the OS-flavor.
 *
 * return values:
 *	'U' // UTM-OS, Un-Safe, Not-allowed
 *	'S' // Base-OS, Safe, allowed
 *
 */
char
detect_WG_os()
{
	const char *sysinfo = "/info.txt";
	unsigned char *buff = NULL;
	unsigned char *ptr  = NULL;

	struct stat st = {};
	char status = 'U';
	int fd = 0;
	int sz = 0;

	if (stat(sysinfo, &st) != 0) {
		printf("Err: No sysfino file\n");
		goto done;
	}

	/* Open the file (readonly) */
	fd = open((char *)sysinfo, O_RDONLY);
		if (fd <= 0) {
		printf("Err: Could not open file: %s\n", strerror(errno));
		goto done;
	}

	/* Get the size of the file */
	sz = lseek(fd, 0, SEEK_END);
	if (sz <= 0) goto done;

	/* allow what's needed */
	buff = calloc(sz + 16, 1);

	/* make sure we read from the beginning */
	lseek(fd, 0, SEEK_SET);

	/* Read the buffer again */
	if (read(fd, buff, sz) != sz)
		goto done;

	ptr = buff;
	if (((ptr = strstr((char *)buff, "Product")) != NULL)) {
		if (strcasestr((char *)ptr, "utm") != NULL)
			status = WG_UTM_OS;
		else if (strcasestr((char *)ptr, "base") != NULL)
			status = WG_BASE_OS;
	}

done:
	if (fd > 0)
		close(fd);

	return status;
}


#elif (defined (__FreeBSD__) || defined (__OpenBSD__) || defined (__APPLE__))

char
platform_get_runlevel()
{
	int mib[2], rlevel = -1;
	size_t len;

	mib[0] = CTL_KERN;
	mib[1] = KERN_SECURELVL;
	
	len = sizeof(rlevel);
	if (sysctl(mib,2,&rlevel,&len, NULL,0) == -1) {
		err(1,"Could not get runlevel");
		return 'u';
	}
#if defined (__OpenBSD__)
	if (rlevel == 0)
#else
	if (rlevel == -1)
#endif
		return 's';	

	return rlevel + '0';
}
#elif (defined (SOLARIS))

MUTEX_DECLARE_INIT(utmp_lock);
char
platform_get_runlevel()
{
	char runlevel = 'u';	/* unknown run level */
	struct utmpx ut, *utp = NULL;

	MUTEX_LOCK(utmp_lock);

	memset(&ut, 0, sizeof(ut));
	ut.ut_type = RUN_LVL;

	setutxent();
	utp = getutxid(&ut);
	if (utp->ut_type == RUN_LVL &&
	    sscanf(utp->ut_line, "run-level %c", &runlevel) != 1)
			runlevel = 'u';
	endutxent();

	MUTEX_UNLOCK(utmp_lock);

	return runlevel;
}
#endif
