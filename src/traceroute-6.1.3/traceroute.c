#define SPRAY
#ifndef lint
static char *rcsid =
   "@(#)$Header: Etraceroute.c,v 6.1.3 2001/06/05 gavron(gavron@wetwork.net)";
#endif
#define TR_VERSION "6.1.3 GOLD+emf_prototrace0.2"

/*
 * traceroute host  - trace the route ip packets follow going to "host".
 *
 * Attempt to trace the route an ip packet would follow to some
 * internet host.  We find out intermediate hops by launching probe
 * packets with a small ttl (time to live) then listening for an
 * icmp "time exceeded" reply from a gateway.  We start our probes
 * with a ttl of one and increase by one until we get an icmp "port
 * unreachable" (which means we got to "host") or hit a max (which
 * defaults to 30 hops & can be changed with the -m flag).  Three
 * probes (change with -q flag) are sent at each ttl setting and a
 * line is printed showing the ttl, address of the gateway and
 * round trip time of each probe.  If the probe answers come from
 * different gateways, the address of each responding system will
 * be printed.  If there is no response within a 5 sec. timeout
 * interval (changed with the -w flag), a "*" is printed for that
 * probe.
 *
 * Probe packets are UDP format.  We don't want the destination
 * host to process them so the destination port is set to an
 * unlikely value (if some clod on the destination is using that
 * value, it can be changed with the -p flag).
 *
 * A sample use might be:
 *
 *     [yak 71]% traceroute nis.nsf.net.
 *     traceroute to nis.nsf.net (35.1.1.48), 30 hops max, 56 byte packet
 *      1  helios.ee.lbl.gov (128.3.112.1)  19 ms  19 ms  0 ms
 *      2  lilac-dmc.Berkeley.EDU (128.32.216.1)  39 ms  39 ms  19 ms
 *      3  lilac-dmc.Berkeley.EDU (128.32.216.1)  39 ms  39 ms  19 ms
 *      4  ccngw-ner-cc.Berkeley.EDU (128.32.136.23)  39 ms  40 ms  39 ms
 *      5  ccn-nerif22.Berkeley.EDU (128.32.168.22)  39 ms  39 ms  39 ms
 *      6  128.32.197.4 (128.32.197.4)  40 ms  59 ms  59 ms
 *      7  131.119.2.5 (131.119.2.5)  59 ms  59 ms  59 ms
 *      8  129.140.70.13 (129.140.70.13)  99 ms  99 ms  80 ms
 *      9  129.140.71.6 (129.140.71.6)  139 ms  239 ms  319 ms
 *     10  129.140.81.7 (129.140.81.7)  220 ms  199 ms  199 ms
 *     11  nic.merit.edu (35.1.1.48)  239 ms  239 ms  239 ms
 *
 * Note that lines 2 & 3 are the same.  This is due to a buggy
 * kernel on the 2nd hop system -- lbl-csam.arpa -- that forwards
 * packets with a zero ttl.
 *
 * A more interesting example is:
 *
 *     [yak 72]% traceroute allspice.lcs.mit.edu.
 *     traceroute to allspice.lcs.mit.edu (18.26.0.115), 30 hops max
 *      1  helios.ee.lbl.gov (128.3.112.1)  0 ms  0 ms  0 ms
 *      2  lilac-dmc.Berkeley.EDU (128.32.216.1)  19 ms  19 ms  19 ms
 *      3  lilac-dmc.Berkeley.EDU (128.32.216.1)  39 ms  19 ms  19 ms
 *      4  ccngw-ner-cc.Berkeley.EDU (128.32.136.23)  19 ms  39 ms  39 ms
 *      5  ccn-nerif22.Berkeley.EDU (128.32.168.22)  20 ms  39 ms  39 ms
 *      6  128.32.197.4 (128.32.197.4)  59 ms  119 ms  39 ms
 *      7  131.119.2.5 (131.119.2.5)  59 ms  59 ms  39 ms
 *      8  129.140.70.13 (129.140.70.13)  80 ms  79 ms  99 ms
 *      9  129.140.71.6 (129.140.71.6)  139 ms  139 ms  159 ms
 *     10  129.140.81.7 (129.140.81.7)  199 ms  180 ms  300 ms
 *     11  129.140.72.17 (129.140.72.17)  300 ms  239 ms  239 ms
 *     12  * * *
 *     13  128.121.54.72 (128.121.54.72)  259 ms  499 ms  279 ms
 *     14  * * *
 *     15  * * *
 *     16  * * *
 *     17  * * *
 *     18  ALLSPICE.LCS.MIT.EDU (18.26.0.115)  339 ms  279 ms  279 ms
 *
 * (I start to see why I'm having so much trouble with mail to
 * MIT.)  Note that the gateways 12, 14, 15, 16 & 17 hops away
 * either don't send ICMP "time exceeded" messages or send them
 * with a ttl too small to reach us.  14 - 17 are running the
 * MIT C Gateway code that doesn't send "time exceeded"s.  God
 * only knows what's going on with 12.
 *
 * The silent gateway 12 in the above may be the result of a bug in
 * the 4.[23]BSD network code (and its derivatives):  4.x (x <= 3)
 * sends an unreachable message using whatever ttl remains in the
 * original datagram.  Since, for gateways, the remaining ttl is
 * zero, the icmp "time exceeded" is guaranteed to not make it back
 * to us.  The behavior of this bug is slightly more interesting
 * when it appears on the destination system:
 *
 *      1  helios.ee.lbl.gov (128.3.112.1)  0 ms  0 ms  0 ms
 *      2  lilac-dmc.Berkeley.EDU (128.32.216.1)  39 ms  19 ms  39 ms
 *      3  lilac-dmc.Berkeley.EDU (128.32.216.1)  19 ms  39 ms  19 ms
 *      4  ccngw-ner-cc.Berkeley.EDU (128.32.136.23)  39 ms  40 ms  19 ms
 *      5  ccn-nerif35.Berkeley.EDU (128.32.168.35)  39 ms  39 ms  39 ms
 *      6  csgw.Berkeley.EDU (128.32.133.254)  39 ms  59 ms  39 ms
 *      7  * * *
 *      8  * * *
 *      9  * * *
 *     10  * * *
 *     11  * * *
 *     12  * * *
 *     13  rip.Berkeley.EDU (128.32.131.22)  59 ms !  39 ms !  39 ms !
 *
 * Notice that there are 12 "gateways" (13 is the final
 * destination) and exactly the last half of them are "missing".
 * What's really happening is that rip (a Sun-3 running Sun OS3.5)
 * is using the ttl from our arriving datagram as the ttl in its
 * icmp reply.  So, the reply will time out on the return path
 * (with no notice sent to anyone since icmp's aren't sent for
 * icmp's) until we probe with a ttl that's at least twice the path
 * length.  I.e., rip is really only 7 hops away.  A reply that
 * returns with a ttl of 1 is a clue this problem exists.
 * Traceroute prints a "!" after the time if the ttl is <= 1.
 * Since vendors ship a lot of obsolete (DEC's Ultrix, Sun 3.x) or
 * non-standard (HPUX) software, expect to see this problem
 * frequently and/or take care picking the target host of your
 * probes.
 *
 * Other possible annotations after the time are !H, !N, !P (got a host,
 * network or protocol unreachable, respectively), !S or !F (source
 * route failed or fragmentation needed -- neither of these should
 * ever occur and the associated gateway is busted if you see one).  If
 * almost all the probes result in some kind of unreachable, traceroute
 * will give up and exit.
 *
 * Notes
 * -----
 * This program must be run by root or be setuid.  (I suggest that
 * you *don't* make it setuid -- casual use could result in a lot
 * of unnecessary traffic on our poor, congested nets.)
 *
 * This program requires a kernel mod that does not appear in any
 * system available from Berkeley:  A raw ip socket using proto
 * IPPROTO_RAW must interpret the data sent as an ip datagram (as
 * opposed to data to be wrapped in a ip datagram).  See the README
 * file that came with the source to this program for a description
 * of the mods I made to /sys/netinet/raw_ip.c.  Your mileage may
 * vary.  But, again, ANY 4.x (x < 4) BSD KERNEL WILL HAVE TO BE
 * MODIFIED TO RUN THIS PROGRAM.
 *
 * The udp port usage may appear bizarre (well, ok, it is bizarre).
 * The problem is that an icmp message only contains 8 bytes of
 * data from the original datagram.  8 bytes is the size of a udp
 * header so, if we want to associate replies with the original
 * datagram, the necessary information must be encoded into the
 * udp header (the ip id could be used but there's no way to
 * interlock with the kernel's assignment of ip id's and, anyway,
 * it would have taken a lot more kernel hacking to allow this
 * code to set the ip id).  So, to allow two or more users to
 * use traceroute simultaneously, we use this task's pid as the
 * source port (the high bit is set to move the port number out
 * of the "likely" range).  To keep track of which probe is being
 * replied to (so times and/or hop counts don't get confused by a
 * reply that was delayed in transit), we increment the destination
 * port number before each probe.
 *
 * Don't use this as a coding example.  I was trying to find a
 * routing problem and this code sort-of popped out after 48 hours
 * without sleep.  I was amazed it ever compiled, much less ran.
 *
 * I stole the idea for this program from Steve Deering.  Since
 * the first release, I've learned that had I attended the right
 * IETF working group meetings, I also could have stolen it from Guy
 * Almes or Matt Mathis.  I don't know (or care) who came up with
 * the idea first.  I envy the originators' perspicacity and I'm
 * glad they didn't keep the idea a secret.
 *
 * Tim Seaver, Ken Adelman and C. Philip Wood provided bug fixes and/or
 * enhancements to the original distribution.
 *
 * I've hacked up a round-trip-route version of this that works by
 * sending a loose-source-routed udp datagram through the destination
 * back to yourself.  Unfortunately, SO many gateways botch source
 * routing, the thing is almost worthless.  Maybe one day...
 *
 *  -- Van Jacobson (van@helios.ee.lbl.gov)
 *     Tue Dec 20 03:50:13 PST 1988
 *
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */


/*
 * PSC Changes Copyright (c) 1992 Pittsburgh Supercomputing Center.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the modifications to this
 * software were developed by the Pittsburgh Supercomputing Center.
 * The name of the Center may not be used to endorse or promote
 * products derived from this software without specific prior written
 * permission.  THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE
 * IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE. 
 *
 */

/* DNS lookup portions of this code also subject to the following:  */
/*
 * Use the domain system to resolve a name.
 *
 * Copyright (C) 1988,1990,1992 Dan Nydick, Carnegie-Mellon University
 * Anyone may use this code for non-commercial purposes as long
 * as my name and copyright remain attached.
 */

/*
 *  1/7/92:  Modified to include a new option for MTU discovery.
 *           This option will send out packets with the don't
 *           fragment bit set in order to determine the MTU
 *           of the path being traceroute'd.  Only decreases
 *           in MTU will be detected, and the MTU will initially
 *           be set to the interface MTU which is used for 
 *           routing.  In the event of an MTU decrease in the 
 *           path, the output will include a message of the 
 *           form MTU=#### with the new MTU for the latest 
 *           hop.  This option is invoked with "-M".
 *                            Jamshid Mahdavi,   PSC.
 */

/*
 * 4/12/93:  Modified to include new option (-Q) that will report on
 *           percentage packet loss* in addition to the usual
 *           min/avg/max delay.  When -Q is invoked, delay per packet
 *           reporting is turned off.  Also ^C aborts further packets
 *           sent on that ttl and goes on to the next -- two
 *           consecutive ^Cs will terminate the traceroute entirely.
 *           [modified code rom Matt Mathis' uping.c code] 
 *                          Jon Boone,  PSC.
 */

/*
 * 4/23/93:  Added support for a "-a" switch which will support
 *           automatic cutoff after a certain number of dropped
 *           packets in a row
 *                          Jon Boone, PSC.
 */

/*
 * 10/21/93: (JM) Fixed SGI version, changed the packet sizing scheme
 *           to a saner system based on total packet size (which
 *           also fixed several bugs in the old code w.r.t. 
 *           size of packets).  Added fast timers.  Added network 
 *           owner lookup.  Plan to add AS path lookup eventually...
 */

/*
 * 05/20/95: (Ehud Gavron, gavron@aces.com) Added in the following:
 *		1. -A now looks up ASs
 *		2. When nprobes = 1 it doesn't give up after one hop
 *		3. Fixed in-addr searches to try progressively larger blocks
 *		   since MCI's backbone uses one SOA for a 256-class-c block
 *		4. It now works under VMS
 *		5. Miscellaneous casts to make ANSI compilers happy
 *		6. Changes #ifdef'd as MAY95
 */

/*
 * 07/20/95: (Ehud Gavron, gavron@aces.com) 
 *           1. Added in a fix to correct for an occasional hang.
 *           2. Fixed minor ANSI compiler things so SunOS likes it
 *
 *           The code, from Steffen Baur (baur@noc.dfn.de) is described by
 *           him as follows.
 *
 *           I've discovered another bug in traceroute. Sometimes traceroute
 *           seem to 'hang' if there's no response for a probe.
 *
 *           The problem is, that all icmp messages (even responses for other 
 *           ping or traceroute tasks) are copied to the input socket. After 
 *           every receive (even on messages for other tasks), traceroute 
 *           waits for another timeout intervall. So, if there is much icmp 
 *           traffic on the host, the socket always receives an icmp message 
 *           for another task before the timeout occurs.
 *
 *           I introduced a kind of deadline in wait_for_reply().
 */

/*
 * 07/20/95: (Ehud Gavron, gavron@aces.com) 
 *		More minor fixes.  Now compiles cleanly on Solaris as well.
 */

/*
 * 09/04/95: (Ehud Gavron, gavron@aces.com)
 *		Now uses VMS command language definition module if available
 *	  	Cleaned up code a bit...
 */

/*
 * 10/04/95: (Ehud Gavron, gavron@aces.com)
 *		It used to be that everyone only had one RADB entry, so that
 *		finding the AS given the net was easy.
 *		Then it turns out that lots of people now have multiple
 *		entries.  
 *
 *		Here's the solution:
 *		1) Use the most specific entries in RADB
 *		2) Where entries are equally specific yet list different
 *		   ASs, list all ASs separated by /.
 *		3) Have a beer
 */

/*
 * 10/15/95: (Ehud Gavron, gavron@aces.com)
 *		1) Joey@teleport.com reports that not terminating reply[]
 *		   is a poor idea ;-)  Corrected.
 *		2) Scott Bradner reports it doesn't work on big-endians.
 *		   Disabled new_packet_ok stuff for now
 *		3) Fixed to use correct offsets.  Removed ntohs() stuff
 *		   since buffer must be in n order (duh).
 *		4) Added in stuff so it will build on Suns without the UCB
 *		   optional compiler /usr/ucb/cc.  (-DSUN_WO_UCB)
 *		5) Added in a diagnostic to check for ip version == 4!
 *		6) Made it work with freebsd by modifying send_probe()
 *		   to fill in ip->ip_v and ip->ip_hl.
 */
/*
 * 11/22/95 Ehud Gavron
 *		bcopy doesn't exist on some systems (Suns without UCB)
 *		but we can't use strncpy since it stops on a 0 byte,
 *		which screws up get_origin since whois.ra.net currently
 *		is blah.blah.0.blah :-)
 */

/*
 * 03/28/96 Ehud Gavron
 *		New Cisco code ignores closely-separated packets that
 *		should generate ICMP errors.  As a result, what used
 *		to be !H !H !H now is !H * !H, and of course delay
 *		stats _to_ a Cisco now show 50% loss.
 *
 *		Modify code to check for unreachable+loss.  This mod
 *		is in under the cisco_icmp ifdef.
 *
 */

/*
 * 12/17/96 Ehud Gavron
 *		If we make the terminator flexible, we can do live HTML
 *		(www.opus1.com/www/traceroute.html) instead of waiting
 *		for the whole thing to be done and then displaying it.
 *		Use -Tterminator in printf() parser format.
 */

/*
 * 01/18/97 Ehud Gavron
 * 		MCI administratively prohibits some packets from crossing
 *		routers.  (traceroute www.mci.net).  Check for ICMP type
 *		13 (undefined in include files here :() and display !A
 *
 * 02/11/97 	Change random mishmash of printf, Printf, fprintf(stdout,
 *		fprintf(stderr, into consistent usage of err for errors
 *		and verbosity and stdout for everything else.
 */

/*
 * 05/05/98	Ehud Gavron	gavron@aces.com
 *
 *		1) Make SOLARIS a define that automatically sets
 *		   'SUN_WO_UCB' for Sun without UCB compiler addons and
 *		   'POSIX' to fix the bullshit in RESOLV.H (__res_send)
 *
 *		2) Make defines for
 *		   'STRING'  system should use <string.h> not <strings.h>
 *		   'NOINDEX' system should use strchr(), not index()
 *		   'NOBZERO' system should use memset() not bzero, and memcpy()
 *			     not bcopy()
 *
 *		3) Add function prototypes.  They can be removed via
 *		   'NO_PROTOTYPES'
 *
 *		4) Based on Pansiot and Grad's wanting to ignore loss,
 *		   add -U flag.  (/HURRY on VMS).  This one will only
 *		   sit on a hop until it gets a satisfactory response.
 *		   That way hops with loss will get up to -n probes,
 *		   and hops with no loss will get just one.
 *
 *		5) Recoded some passing of parameters to make ansi-compliant
 *		   compilers even happier.
 */

/*
 * 06/30/98	Ehud Gavron	gavron@aces.com
 *
 *		V2.8.1	Added -P  (/PING) to help out sites that are smurf-safe
 *			by blocking ICMP ECHO reply.
 *			This will do the same as 1 probe, timeout 3, ttl 64
 *			and return a status of 1 (success) or 0 (failure)
 */

/*
 * 01/05/1999	Ehud Gavron	gavron@aces.com
 *		V2.8.2	Added in Brian Murphy's linux code.  His comments:
 * 12/31/98 (Brian Murphy, murphy@u.arizona.edu)
 *      Ported to linux.  Tested on Red Hat 5.0 with the 2.1.128 kernel.
 *
 * 01/01/98
 *      Removed trust from getenv() that could lead to a buffer overflow.
 */

/* 
 * 01/06/1999	Ehud Gavron	gavron@aces.com
 *		V2.8.3	Added in Craig Watkins <Craig.Watkins@innosoft.com>
 *			fix to make SolX86 work (missing htons() in ip_len)
 * 01/08/1999	V2.8.4	Added in Richard Irving <rirving@onecall.net> nit-fixes
 *			to check for null parameters, as well as cast some math
 *			into proper ints.
 * 02/26/1999	V2.9.0	Craig Watkings suggested sending all probes out at
 *			once. What a great idea!  -P for parallel probing of
 *			all TTLs.  Messy on output if we don't want to sit
 *			and definitely wait it out, but useful!  -$ for the
 *			old 'ping style' behavior.  
 * 02/27/1999	V2.9.1	Allow more than one probe per hop, display last one to
 *			successfully return.  Fix so all packets returning will
 *			override the max-wait timer.  Fix so we check the TTL
 *			and make sure our packets (even out of order) are ok.
 *			pretty up the output.  Remove magic number "3" for
 *			ICMP_UNREACH_PORT.
 * 03/01/1999	V2.9.2	Freebsd seems to get packets that generate indices
 *			which are outside the array (line 1299).  This segfault
 *			is corrected by a bounds check on line 1298.
 * 03/03/1999	V2.9.3	Make printing the header and time difference standard
 *			callable routines (print_from, print_time).  Make the
 *			last address be a struct sockaddr_in instead of u_long
 *			since that's the right way to do it.  Make the code
 *			in packet_ok() and send_probe() conditional on spray
 *			mode so we don't jabber all over the data cells!
 * 09/30/1999   V2.9.4  Added "FORCE_NATURAL_MASK" in lookup_as() so that if
 *                      necessary, old-style lookups would still work.  Now,
 *                      the -A option will use the address as is.
 * 11/15/1999   V6.0	Renumber to Version 6.0 and rename to TrACESroute
 *			to avoid 'confusion' with vj/lbl traceroute 2.9.x.
 *			Of course this makes us much more advanced than
 *			Netscape 4.8, I.E. 5.0, or The New AOL 5.0. ;-)
 * 01/18/2000   V6.1    Dan Cohn <dan@internap.com> provided mods to make
 *                      spray mode as functional as normal mode, both in
 *                      terms of missing packets and in multiple probes.
 * 04/29/2000	V6.1.1	As per Thomas Erskine, add info for -A where the
 *			route is not in the RR server queried.
 *
 * 06/05/2001	V6.1.2	When using -A, get_origin() returns 0 for ASs not
 *			found in your particular RA_SERVER.  This, passed
 *			to printf(%s) ended up as <Null> on Linux, and a
 *			coredump on Solaris... fixed by adding a nullstring
 *			<NONE> and checking for zero result from get_origin.
 *			Thanks to Richard Wright for pointing it out and
 *			assisting in the debug. 
 * 06/16/2001   emf_prototrace 0.2
 *                      Added rudimentary support for generic protocol type
 *                      scan via -I option.  It's really really rough right
 *                      now, but it gives me enough ammunition to tell
 *                      some feeb where my packets are stopping.   I
 *                      generally use this with -I 50 to check for IPSec 
 *                      VPN functionality.  (You'd be completely amazed at
 *                      how many people claim their firewall isn't
 *                      interfering with their VPN clients. Sheesh)
 *
 * 10/27/2001   V6.1.3  Disallow max_ttl*nprobes>spraymax. Raise spraymax.
 */

#include <stdio.h>

#define IP_VERSION 4	/* We can't work with anything else... */
#define CISCO_ICMP 1	/* We check for loss+unreachables = probes */

#ifdef __decc		/* DEC C wants strings.h to be called string.h */
#define STRING
#define NOINDEX		
#define NOBZERO
#endif

#ifdef SOLARIS		/* Solaris has UCB stuff gone, and POSIX resolver */
#define SUN_WO_UCB
#define POSIX
#endif

#ifdef SUN_WO_UCB
#define STRING	
#define NOINDEX
#include <signal.h>
#endif

#ifdef _AIX		/* Aix has its own set of fd_* macros */
#include <signal.h>
#include <sys/select.h>           
#endif

#ifdef STRING
#include <string.h>
#else /* ! STRING */
#include <strings.h>
#endif /* STRING */

#include <sys/param.h>

#ifdef NOINDEX
#define index(x,y) strchr(x,y)  /* Use ansi strchr() if no index() */
#endif /* NOINDEX */

#ifndef bzero
#ifdef NOBZERO
#define bzero(x,y) memset((void *)x,(int)0,(size_t) y)
#define bcopy(x,y,z) memcpy((void *)y, (const void *)x, (size_t) z)
#endif /* NOBZERO */
#endif /* bzero*/

#include <stdlib.h>		/* For atof(), etc. */
#include <unistd.h>		/* For getpid(), etc. */

/* The VMS stuff follows */
#ifdef	vms
pid_t decc$getpid(void);	/* Just don't ask...*/
#define getpid decc$getpid	/* Really... don't ask.*/
#define perror socket_perror	/* MultiNet wants this */
#ifdef MULTINET_V3
#define errno socket_errno	/* MultiNet wants this */
#include "multinet_root:[multinet.include]errno.h"
#else /* MULTINET_V4 */
#define MULTINET_V4
#include <errno.h>
#ifdef errno
#undef errno
#include "multinet_root:[multinet.include]errno.h"
#define errno socket_errno	/* Multinet 4.1 */
#endif /* errno defined */
#endif /* MULTINET_V3 */
#define write socket_write	/* MultiNet wants this */
#define read socket_read	/* MultiNet wants this */
#define close socket_close	/* MultiNet wants this */
#include <signal.h>
#ifdef __alpha
#define BYTE_ORDER 1234		/* The include files for Alpha are bad. */
#define LITTLE_ENDIAN 1234	/* They incorrectly swap ip_v and ip_hl */
#define BIG_ENDIAN 4321		/* Which makes packet_ok fail.  New diag */
#endif /* __alpha */		/* Info says:  packet version not 4: 5 */
#ifdef VMS_CLD			/* use separate qualifers instead of options */
#include "clis.h"
#else /* No CLD */
int fixargs(int *, char **, char **);
#endif /* VMS_CLD */
#else /* not VMS */
#include <errno.h>
#endif	/* vms */

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <sys/time.h>

#ifndef vms			/* Not VMS */
#ifdef POSIX				/* Posix */
#define BIND_RES_POSIX3
#include <resolv.h>
#ifdef res_mkquery				/* __res_mkquery() */
#undef res_mkquery
#endif						/* endif */
#ifdef res_send
#undef res_send
#endif
#else /* POSIX */			/* else ! Posix */
#include <resolv.h>
#endif /* POSIX */			/* Endif Posix */
#else /* vms */			/* else VMS */
#ifdef MULTINET_V4	/* they didn't put the prototypes in the file */
#include <resolv.h>
int res_send(unsigned char *, int, unsigned char *, int);
int res_mkquery(int op, const char *dname,
          int class, int type, const char *data, int datalen,
          void *, u_char *buf, int buflen);
int gettimeofday(struct timeval *, void *);
#endif /* MULTINET_V4 */
#endif /* __vms */		/* Endif VMS */

#ifdef __linux__                /* Wrapping this may be excessive */
#define __FAVOR_BSD
#endif


#ifndef MAX_DATALEN
#define MAX_DATALEN 32000	/* Maximum size of MTU discovery packet length*/
#endif

#include <sys/socket.h>
#include <sys/file.h>
#include <sys/ioctl.h>



#include <netinet/in_systm.h>
#include <netinet/ip.h>
#ifndef __linux__
#include <netinet/ip_var.h>
#else /* __linux__ */
#include <sys/time.h>
/* IRD #include <netinet/if_tr.h> */
#endif /* __linux__ */
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include <netdb.h>
#include <ctype.h>
#include <math.h>		/* After resolv.h for gcc2.7/sun __p redef */

#ifndef NO_PROTOTYPES		/* By default, have prototypes */
int send_probe(int, int, int);
int wait_for_reply(int, struct sockaddr_in *, struct timeval *);
int packet_ok(u_char *, int, struct sockaddr_in *, int, int);
int tvsub(struct timeval *, struct timeval *);
void print_time(float *);
void print_from(struct sockaddr_in *);
int print(u_char *, int, struct sockaddr_in *);
int reduce_mtu(int);
int doqd(unsigned char *, int);
int dorr(unsigned char *, int, char **);
int doclass(unsigned char *, int);
int dordata(unsigned char *, int, int, int, char *, char **);
int dottl(unsigned char *,int);
int doname(unsigned char *, int, char *);
int dotype(unsigned char *, int);
void AbortIfNull (char *);
void print_ttl(int);
void print_packet(struct ip *, int);
#endif /* NO_PROTOTYPES */

#define	MAXPACKET	65535	/* max ip packet size */
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN	64
#endif

#define SIZEOFstructip sizeof(struct ip)

#ifndef FD_SET
#define NFDBITS         (8*sizeof(fd_set))
#define FD_SETSIZE      NFDBITS
#define FD_SET(n, p)    ((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define FD_CLR(n, p)    ((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define FD_ISSET(n, p)  ((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define FD_ZERO(p)      bzero((char *)(p), sizeof(*(p)))
#endif

#define Fprintf (void)fprintf
#define Sprintf (void)sprintf
#define Printf (void)printf

#define NOERR(val,msg) {if (((int)(val)) < 0) {perror(msg);exit(1);}}

#ifndef NO_SOA_RECORD
#define NO_SOA_RECORD "no SOA record"
#endif

/*  For some reason, IP_HDRINCL and LSRR don't interact well on SGI;
 *  so turn it off:  */
#ifdef sgi
#undef IP_HDRINCL
#endif

#ifndef vms
extern	int errno;
#endif

extern  char *inet_ntoa();
extern  u_long inet_addr();

#ifndef ULTRIX43
char *index(const char *string, int character);
#endif

void halt();   /* signal handler */

/*
 * format of a (udp) probe packet.
 */
struct opacket {
	struct ip ip;
	struct udphdr udp;
	u_char seq;		/* sequence number of this packet */
	u_char ttl;		/* ttl packet left with */
	struct timeval tv;	/* time packet left */
};

#ifdef SPRAY
/*
 * format of a spray data cell.
 */
#define SPRAYMAX 512		/* We'll only do up to 512 packets at once */
struct {
        u_long  dport;          /* check for matching dport */
        u_char  ttl;            /* ttl we sent it to */
        u_char  type;           /* icmp response type */
        struct  timeval out;    /* time packet left */
        struct  timeval rtn;    /* time packet arrived */
        struct  sockaddr_in from; /* whom from */
} spray[SPRAYMAX];
int *spray_rtn[SPRAYMAX];	/* See which TTLs have responded */
int spray_target;		/* See which TTL the host responds on */
int spray_max;			/* See which is the highest TTL we've seen */
int spray_min;			/* See smallest host-returned TTL */
int spray_total;		/* total of responses seen */
int spray_mode =0;		/* By default, turned off */
#endif /* SPRAY */

u_char	packet[512];		/* last inbound (icmp) packet */
struct opacket	*outpacket;	/* last output packet */
char *inetname();
u_char  optlist[MAX_IPOPTLEN];  /* IP options list  */
int _optlen;
struct icmp *icp;               /* Pointer to ICMP header in packet */


int s;				/* receive (icmp) socket file descriptor */
int sndsock;			/* send (udp) socket file descriptor */
#if defined(FREEBSD) || defined(__linux__)
struct timezone tz;
#else
unsigned long   tz;             /* leftover */
#endif
struct sockaddr whereto;	/* Who to try to reach */
struct sockaddr_in addr_last;	/* last printed address */
int datalen;			/* How much data */


char *source = 0;
char *hostname;
char hnamebuf[MAXHOSTNAMELEN];

int nprobes = 3;
int min_ttl = 1;
int max_ttl = 30;
u_short ident;
u_short port = 32768+666;	/* start udp dest port # for probe packets */
u_short sport = 1000;		/* source port ... */

int options;			/* socket options */
int verbose;
int mtudisc=0;                  /* do MTU discovery in path */
int pingmode=0;			/* replacing ping functionality? */
#ifndef vms
float waittime = 3.0;		/* time to wait for response (in seconds) */
#else /* vms */
double waittime = 3.0;
#endif
int nflag;			/* print addresses numerically */

#define TERM_SIZE 32		/* Size of line terminator... */
char terminator[TERM_SIZE];	/* Line terminator... */
int haltf=0;                    /* signal happened */
int ppdelay=1;                  /* we normally want per-packet delay */
int pploss=0;                   /* we normally don't want packet loss */
int lost;                       /* how many packets did we not get back */ 
double throughput;              /* percentage packets not lost */
int consecutive=0;              /* the number of consecutive lost packets */
int automagic=0;                /* automatically quit after 10 lost packets? */
int hurry_mode=0;		/* only do one on successful ttls */
int utimers=0;                  /* Print timings in microseconds */
int dns_owner_lookup=0;         /* Look up owner email in DNS */
int as_lookup=0;                /* Look up AS path in routing registries */
int got_there;
int unreachable;
int response_mask = 0;
int mtu, new_mtu = 0;

char nullstring[] = "<NONE>";


char usage[] = "%s: TrACESroute Usage: traceroute [-adnruvAMOQ] [-w wait] [-S start_ttl] [-m max_ttl] [-p port#] [-q nqueries] [-g gateway] [-t tos] [-s src_addr] [-g router] [-I proto] host [data size]\n\
      -a: Abort after 10 consecutive drops\n\
      -d: Socket level debugging\n\
      -g: Use this gateway as an intermediate hop (uses LSRR)\n\
      -S: Set start TTL (default 1)\n\
      -m: Set maximum TTL (default 30)\n\
      -n: Report IP addresses only (not hostnames)\n\
      -p: Use an alternate UDP port\n\
      -q: Set the number of queries at each TTL (default 3)\n\
      -r: Set Dont Route option\n\
      -s: Set your source address\n\
      -t: Set the IP TOS field (default 0)\n\
      -u: Use microsecond timestamps\n\
      -v: Verbose\n\
      -w: Set timeout for replies (default 5 sec)\n\
      -A: Report AS# at each hop (from GRR)\n\
      -I: use this IP protocol (currently an integer) instead of UDP\n\
      -M: Do RFC1191 path MTU discovery\n\
      -O: Report owner at each hop (from DNS)\n\
      -P: Parallel probing\n\
      -Q: Report delay statistics at each hop (min/avg+-stddev/max) (ms)\n\
      -T: Terminator (line end terminator)\n\
      -U: Go to next hop on any success\n";

float deltaT();

main(argc, argv)
	int argc;
	char *argv[];
{
	struct sockaddr_in from;
	char **av = argv;
#ifdef VMS_CLD
	char *ptr;
#endif /* VMS_CLD */
#ifdef SPRAY
        float ddt=0;	/* Delta delta time... for subtracting packet time */
#endif

	struct sockaddr_in *to = (struct sockaddr_in *) &whereto;
	int on = 1;
	int alloc_len;
	struct protoent *pe;
	int ttl, probe, i;
        int last_i;
	int last_ttl;
	int ttl_diff;
	int spr_ttl;
	int idx;		/* index to ttl based on spray sequence */
	int seq = 0;
	int tos = 0;
	struct hostent *hp;
	unsigned int lsrr = 0;
	u_long gw;
	u_char *oix;
#ifdef TEST			/* For testing purposes.  This will one day */
	u_long gw_list[10];	/* be the list of intermediate gateways for */
#endif				/* LSRR on netbsd kernels. */	
	u_long curaddr;
	float min;
	float max;
	float sum;
	float sumsq;
	int cc;
	int probe_protocol=IPPROTO_UDP;		/* what IP protocol to use */
	struct timeval tv;
	struct timeval deadline;
	struct ip *ip;



	sprintf(terminator,"\n");	/* Standard line terminator */
	oix = optlist;
	bzero(optlist, sizeof(optlist));

#ifndef VMS_CLD
#ifdef __vms
	if (argc < 3) fixargs(&argc,argv,av);	
#endif
	argc--, av++;
	while (argc && *av[0] == '-')  {
		while (*++av[0])
			switch (*av[0]) {
			case 'a':
			        automagic = 1;
				break;
			case 'U':
				hurry_mode =1;
				break;
			case 'A':
				as_lookup = 1;
				break;
			case 'd':
				options |= SO_DEBUG;
				break;
			case 'g':
				argc--, AbortIfNull((++av)[0]);
				if ((lsrr+1) >= ((MAX_IPOPTLEN-IPOPT_MINOFF)/sizeof(u_long))) {
				  Fprintf(stderr,"No more than %d gateway%s",
					  ((MAX_IPOPTLEN-IPOPT_MINOFF)/sizeof(u_long))-1,terminator);
				  exit(1);
				}
				if (lsrr == 0) {
				  *oix++ = IPOPT_LSRR;
				  *oix++;	/* Fill in total length later */
				  *oix++ = IPOPT_MINOFF; /* Pointer to LSRR addresses */
				}
				lsrr++;
                                if (*av[0] ==0) {
                                    Fprintf(stderr,"Hosts are not blank.%s",terminator);
                                    exit(1);
                                }
				if (isdigit(*av[0])) {
				  gw = inet_addr(*av);
				  if (gw) {
				    bcopy(&gw, oix, sizeof(u_long));
				  } else {
				    Fprintf(stderr, "Unknown host %s%s",av[0],terminator);
				    exit(1);
				  }
				} else {
				  hp = gethostbyname(av[0]);
				  if (hp) {
				    bcopy(hp->h_addr, oix, sizeof(u_long));
				  } else {
				    Fprintf(stderr, "Unknown host %s%s",av[0],terminator);
				    exit(1);
				  }
				}
#ifdef TEST	/* store gateways for netbsd kernels */
                                bcopy(oix,&gw_list[lsrr],sizeof(u_long));
#endif
				oix += sizeof(u_long);
				goto nextarg;
			case 'I':
				argc--, AbortIfNull((++av)[0]);
				/* EMF: we need to check for string names
				 * and feed them to getprotobyname() here
				 */
				probe_protocol = atoi(av[0]);
				if (probe_protocol > 254) {
					Fprintf(stderr, "protocol must be >=254%s",terminator);
					exit(1);
				}
				goto nextarg;
			case 'S':
				argc--, AbortIfNull((++av)[0]);
				min_ttl = atoi(av[0]);
				if (min_ttl > max_ttl) {
					Fprintf(stderr, "min ttl must be >=%d%s",max_ttl,terminator);
					exit(1);
				}
				goto nextarg;
			case 'm':
				argc--, AbortIfNull((++av)[0]);
				max_ttl = atoi(av[0]);
				if (max_ttl < min_ttl) {
					Fprintf(stderr, "max ttl must be >%d%s",min_ttl,terminator);
					exit(1);
				}
				goto nextarg;
			case 'n':
				nflag++;
				break;
			case 'O':
				dns_owner_lookup = 1;
				break;
			case 'p':
				argc--, AbortIfNull((++av)[0]);
				port = atoi(av[0]);
				if (port < 1) {
					Fprintf(stderr, "port must be >0%s",terminator);
					exit(1);
				}
				goto nextarg;
			case 'P':
				spray_mode = 1;
				break;
			case 'f':
				argc--, AbortIfNull((++av)[0]);
				sport = atoi(av[0]);
				goto nextarg;
			case 'q':
				argc--, AbortIfNull((++av)[0]);
				nprobes = atoi(av[0]);
				if (nprobes < 1) {
					Fprintf(stderr, "nprobes must be >0%s",terminator);
					exit(1);
				}
				goto nextarg;
			case 'r':
				options |= SO_DONTROUTE;
				break;
			case 's':
				/*
				 * set the ip source address of the outbound
				 * probe (e.g., on a multi-homed host).
				 */
				argc--, AbortIfNull((++av)[0]);
				source = av[0];
				goto nextarg;
			case 't':
				argc--, AbortIfNull((++av)[0]);
				tos = atoi(av[0]);
				if (tos < 0 || tos > 255) {
					Fprintf(stderr, "tos must be 0 to 255%s",terminator);
					exit(1);
				}
				goto nextarg;
			case 'u':
				utimers = 1;
				break;
			case 'v':
				verbose++;
				break;
			case 'w':
				argc--, AbortIfNull((++av)[0]);
				waittime = atof(av[0]);
				if (waittime <= .01) {
					Fprintf(stderr, "wait must be >10 msec%s",terminator);
					exit(1);
				}
				goto nextarg;
 		        case 'M':
				mtudisc++;
				break;
			case '$':
				min_ttl = 64;
				max_ttl = 64;
				nprobes = 1;
				pingmode = 1;
				break;
                        case 'Q':
				pploss = 1;
				ppdelay = 0;
				break;
                        case 'T':
				av++;
				if (--argc < 1) {
				        Fprintf(stdout,usage,TR_VERSION);
					exit(1);
				}
				strncpy(terminator,av[0],TERM_SIZE);
				terminator[TERM_SIZE -1] = 0;
				goto nextarg;
			default:  
				Fprintf(stdout,usage,TR_VERSION);
				exit(1); 
			}
	nextarg:
		argc--, av++;
	}
#else /* VMS_CLD defined */
#include "clis.h"
#endif /* VMS_CLD */

	if (argc < 1)  {
		Fprintf(stdout,usage,TR_VERSION);
		exit(1);
	}
#ifndef vms
	setlinebuf (stdout);
#endif

	(void) bzero((char *)&whereto, sizeof(struct sockaddr));
	to->sin_family = AF_INET;
#ifdef VMS_CLD
	av[0] = hostname;
#endif
	to->sin_addr.s_addr = inet_addr(av[0]);
	if ((int)to->sin_addr.s_addr != -1) {
		(void) strcpy(hnamebuf, av[0]);
		hostname = hnamebuf;
	} else {
		hp = gethostbyname(av[0]);
		if (hp) {
			to->sin_family = hp->h_addrtype;
			bcopy(hp->h_addr, (caddr_t)&to->sin_addr, hp->h_length);
			hostname = hp->h_name;
		} else {
			Fprintf(stderr,"%s: unknown host %s%s", argv[0], av[0],terminator);
			exit(1);
		}
	}

#ifndef VMS_CLD
	if (argc >= 2)
		datalen = atoi(av[1]);
	if (datalen < 0 || datalen >= MAXPACKET) {
		Fprintf(stderr, "traceroute: packet size must be 0 <= s < %ld%s",
			(long) MAXPACKET - sizeof(struct opacket),terminator);
		exit(1);
	}
#else /* VMS_CLD defined */
#include "clis.h"
#endif /* VMS_CLD */
	if (mtudisc) 
	  /*  Ignore data length as set.  Set it to a large value to 
	      start things off...  */
	  datalen=MAX_DATALEN;

	if (datalen < (int) (sizeof(struct opacket) + MAX_IPOPTLEN)) {
	  alloc_len = sizeof(struct opacket) + MAX_IPOPTLEN;
	} else {
	  alloc_len = datalen;
	}

	if (spray_mode) {
	   if (nprobes*max_ttl >= SPRAYMAX) {
              Fprintf(stderr,"Spray mode limited to %d packets.\n",SPRAYMAX);
              Fprintf(stderr,"Max TTL of %d with %d probes = %d\n",
			max_ttl,nprobes,max_ttl*nprobes);
	      Fprintf(stderr,"Disabling spray mode.\n");
	      spray_mode = 0;
      	   }
	   if (pploss) {
              Fprintf(stderr,"spray and packet stats are incompatible.\n");
	      spray_mode = 0;
	   } 
	   if (mtudisc) {
	      Fprintf(stderr,"spray and MTU discovery are incompatible.\n");
	      spray_mode = 0;
	   }
	   if (lsrr > 0) {
	      Fprintf(stderr,"spray and loose source are incompatible.\n");
	      spray_mode = 0;
	   }
	}	     

	outpacket = (struct opacket *)malloc((unsigned)alloc_len);

	if (! outpacket) {
		perror("traceroute: malloc");
		exit(1);
	}
	(void) bzero((char *)outpacket, alloc_len);
	outpacket->ip.ip_dst = to->sin_addr;
	outpacket->ip.ip_tos = tos;

	ident = (getpid() & 0xffff) | 0x8000;

	if ((pe = getprotobyname("icmp")) == NULL) {
		Fprintf(stderr, "icmp: unknown protocol%s",terminator);
		exit(10);
	}
	if ((s = socket(AF_INET, SOCK_RAW, pe->p_proto)) < 0) {
		perror("traceroute: icmp socket");
		exit(5);
	}
	if (options & SO_DEBUG)
		(void) setsockopt(s, SOL_SOCKET, SO_DEBUG,
				  (char *)&on, sizeof(on));
	if (options & SO_DONTROUTE)
		(void) setsockopt(s, SOL_SOCKET, SO_DONTROUTE,
				  (char *)&on, sizeof(on));

	if ((sndsock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
		perror("traceroute: raw socket");
		exit(5);
	}

	/*  ^C punts you to the next hop.  Twice will exit.  */

#ifndef __linux__
	NOERR(signal(SIGINT,halt),"signal SIGINT");   
#endif /* __linux__ */
	
	if (lsrr > 0) {
	  lsrr++;
	  optlist[IPOPT_OLEN]=IPOPT_MINOFF-1+(lsrr*sizeof(u_long));
	  bcopy((caddr_t)&to->sin_addr, oix, sizeof(u_long));
	  oix += sizeof(u_long);
	  while ((oix - optlist)&3) oix++;		/* Pad to an even boundry */
	  _optlen = (oix - optlist);

	  if ((pe = getprotobyname("ip")) == NULL) {
	    perror("traceroute: unknown protocol ip");
	    exit(10);
	  }
#ifndef TEST
	  if ((setsockopt(sndsock, pe->p_proto, IP_OPTIONS, optlist, oix-optlist)) < 0) {
	    perror("traceroute: lsrr options");
	    exit(5);
	  }
#else /* TEST manual lsrr */
	  Fprintf(stderr,"Current test IP header length: %d\n",outpacket->ip.ip_hl);
#endif
	}

	if (datalen < (int) (sizeof (struct opacket) + _optlen)) {
	  /*  The chosen size is too small to fit everything...
	      make it bigger:  */
	  datalen = sizeof (struct opacket) + _optlen;
      }


#ifdef SO_SNDBUF
	if (setsockopt(sndsock, SOL_SOCKET, SO_SNDBUF, (char *)&datalen,
		       sizeof(datalen)) < 0) {
		perror("traceroute: SO_SNDBUF");
		exit(6);
	}
#endif /* SO_SNDBUF */
#ifdef IP_HDRINCL
	if (setsockopt(sndsock, IPPROTO_IP, IP_HDRINCL, (char *)&on,
		       sizeof(on)) < 0) {
		perror("traceroute: IP_HDRINCL");
		exit(6);
	}
#endif /* IP_HDRINCL */
	if (options & SO_DEBUG)
		(void) setsockopt(sndsock, SOL_SOCKET, SO_DEBUG,
				  (char *)&on, sizeof(on));
	if (options & SO_DONTROUTE)
		(void) setsockopt(sndsock, SOL_SOCKET, SO_DONTROUTE,
				  (char *)&on, sizeof(on));

	if (source) {
		(void) bzero((char *)&from, sizeof(struct sockaddr));
		from.sin_family = AF_INET;
		from.sin_addr.s_addr = inet_addr(source);
		if ((int)from.sin_addr.s_addr == -1) {
			Fprintf(stderr,"traceroute: unknown host %s%s", source,terminator);
			exit(1);
		}
		outpacket->ip.ip_src = from.sin_addr;
#ifndef IP_HDRINCL
		if (bind(sndsock, (struct sockaddr *)&from, sizeof(from)) < 0) {
			perror ("traceroute: bind:");
			exit (1);
		}
#endif /* IP_HDRINCL */
	}

	Fprintf(stderr, "traceroute to %s (%s)", hostname,
		inet_ntoa(to->sin_addr));
	if (source)
		Fprintf(stderr, " from %s", source);
	Fprintf(stderr, ", %d hops max, %d byte packets%s", max_ttl, datalen,
		terminator);
	(void) fflush(stderr);

#ifdef TEST
	if (lsrr != 0) {
	   for (ttl = 1; ttl < lsrr ; ++ttl) {
		Fprintf(stderr,"Lsrr hop %d is %s\n",ttl,inet_ntoa(gw_list[ttl]));
	   }
	}
#endif
   if (!spray_mode) {
      /* For all TTL do */
      for (ttl = min_ttl; ttl <= max_ttl; ++ttl) {
	 bzero(&addr_last,sizeof(addr_last));
         got_there = unreachable = mtu = lost = consecutive =0;
         min = max = sum = sumsq = 0.0;
         throughput = (double) 0.0;

         print_ttl(ttl);
         if (new_mtu != 0) {
            Fprintf(stdout,"MTU=%d ",new_mtu);
            new_mtu=0;
         }
         /* For all probes do */
         for (probe = 0; probe < nprobes; ++probe) {
            (void) gettimeofday(&tv, NULL);
	    send_probe(++seq, ttl, probe_protocol);
	    deadline.tv_sec = tv.tv_sec + (int) waittime; 
	    deadline.tv_usec=tv.tv_usec+((int) (waittime*1000000.0))%1000000;
	    if (deadline.tv_usec >= 1000000) {
	       deadline.tv_usec -= 1000000;
	       deadline.tv_sec++;
	    }
            /* Get an answer */
	    while (cc = wait_for_reply(s, &from, &deadline)) {
	       if ((i = packet_ok(packet, cc, &from, seq, probe_protocol))) {
	          float dt = deltaT(&tv);
	          if (sum == 0) {
	             sum = min = max = dt;
		     sumsq = dt*dt;
	          } else {
	             if (dt < min) min = dt;
	   	     if (dt > max) max = dt;
		     sum += dt;
		     sumsq += dt*dt;
	          }
	          if (hurry_mode) probe = nprobes;

		  print_from(&from);

	          if (ppdelay) {
		     print_time(&dt);
		  }
		    
		  print_packet((struct ip *) &packet, i);

		  consecutive = 0; /* got a packet back! */
		  break;
	       } /* end if packet ok */
            } /* end while */

	       if (cc == 0) {
	          if (pingmode) exit(23);
		  Fprintf(stdout," *");
		  (void) fflush(stdout);

		  lost++;
		  consecutive++;

		  /*  Reset the ^C action from exit to skip TTL  */
 	          if (haltf==0 && lost==1)
#ifndef __linux__
		  NOERR(signal(SIGINT,halt), "signal SIGINT");
#endif /* __linux__ */
		  /* we've missed at least one packet, so let's check for the 
                     signal to go to the next ttl */
	          if (haltf > 0) {
		     haltf = 0;
		     consecutive = 0;
		     break;
		  }
	       } /* end if cc = 0 */
	       if (automagic && (consecutive > 9)) break;
         } /* end for probe */


	 if (pploss) {
	    if (lost < probe) {
	       throughput = ( 100.0 - ( ( lost * 100.0 ) / probe ));
	       Fprintf(stdout,
                       "  (%1.1f ms/%1.1f ms(+-%1.1f ms)/%1.1f ms)", 
	               min, (sum / (probe - lost)), 
		       (float)sqrt((double)sumsq)/(probe-lost), max);
	       Fprintf(stdout," %d/%d (%#3.2f%%)", (probe - lost),
	               probe, throughput);
	       (void) fflush(stdout);
            }
	 }
	 Fprintf(stdout,terminator); 

	 /* If we're running one probe and we get back one packet, that's
	    no excuse to quit unless we're really done! */

         if ( ((nprobes == 1) && (got_there || unreachable)) ||
#ifndef CISCO_ICMP
               (got_there || unreachable > nprobes - 1) )
#else /* CISCO_ICMP meaning not all our packets will be returned. */
               (got_there || unreachable > nprobes - 1) ||
               ( (unreachable+lost > nprobes - 1) && (unreachable > 0) ))
#endif /* CISCO_ICMP */
	    exit(0);
	 if (new_mtu != 0) {
	    ttl--;  /*  Redo the same TTL  */
	    datalen = new_mtu;  /*  Set the new data length  */
	 }
      } /* end for TTL */

/* end non-spray mode */

} else { 

/* 
 * Enter Spray mode
 */
   spray_target = spray_max = spray_total = 0;
   spray_min = SPRAYMAX+1;

   /* For all TTL do */
   for (ttl = min_ttl; ttl <= max_ttl; ++ttl) {
      spray_rtn[ttl] = (int *)malloc(sizeof(int)*nprobes + 1);
      for (probe = 0; probe < nprobes; ++probe) {
	     spray_rtn[ttl][probe]=0;
         send_probe(++seq, ttl, probe_protocol);
      }
   }
   (void) gettimeofday(&tv, NULL);
   deadline.tv_sec = tv.tv_sec + (int) waittime; 
   deadline.tv_usec=tv.tv_usec+((int) (waittime*1000000.0))%1000000;
   if (deadline.tv_usec >= 1000000) {
      deadline.tv_usec -= 1000000;
      deadline.tv_sec++;
   }
   /* Go get responses until either we get them all, or timeout */
   while (  ((((int)pow(2,spray_min)-1)&response_mask) != ((int)pow(2,spray_min)-2)) &&
            (cc = wait_for_reply(s,&from,&deadline))  ) {
      (void) packet_ok(packet, cc, &from, seq, probe_protocol);
   }

   last_i = 1;
   last_ttl = 0;
   bzero(&addr_last, sizeof(addr_last));
   for (i = 1; i <= max_ttl; i++) {
      /* First see if it's valid, and if so play with its time */
      idx = spray_rtn[i][0];
      if ((idx > 0) && (idx < SPRAYMAX) && (spray[idx].from.sin_addr.s_addr != 0)) { 
	      spr_ttl = spray[idx].ttl;
	      if (spr_ttl != i) {
	          Fprintf(stderr,"Check failure spray(rtn[i]) !=i\n");
	          exit(0);
	      }

          /* do not display duplicate entries (responses beyond terminus from same host) */
	      if (0 != memcmp( &spray[idx].from.sin_addr.s_addr,
                          &addr_last.sin_addr.s_addr,
		                  sizeof(addr_last.sin_addr.s_addr) )) {

        	 ttl_diff = (spr_ttl - last_ttl);
        	 if (ttl_diff > 1) {
                   for (last_i = last_ttl+1; last_i < spr_ttl; last_i++) {
                       print_ttl(last_i);
        	       for (probe = 1; probe <= nprobes ; probe++) { 
                          Fprintf(stdout," *");
                       } /* end for probes */
                       Fprintf(stdout,terminator);
                   } /* end for last i */
        	 } /* endf if spr_ttl ... */
        
             if (spr_ttl > last_ttl) {
                print_ttl(spr_ttl);
                last_ttl = spr_ttl;
             }

             print_from(&spray[idx].from);

             for (probe = 0; probe < nprobes; ++probe) {
    		     if (spray_rtn[i][probe] != 0) {
                     tvsub(&spray[spray_rtn[i][probe]].rtn,&spray[spray_rtn[i][probe]].out);
                     ddt=spray[spray_rtn[i][probe]].rtn.tv_sec*1000.0+
                        ((float)spray[spray_rtn[i][probe]].rtn.tv_usec)/1000.0;
                     print_time(&ddt);
    			 } else {
    			     Fprintf(stdout," *");
    			 }
             }

             Fprintf(stdout,terminator);
		 } /* no duplicate entries */
      } /* end if nonzero type */
   } /* end for */
} /* end spray mode */
}

void print_packet(struct ip *ip, int i)
{
		    switch(i - 1) {
			case 13:
				Fprintf(stdout," !A"); /* admin prohibited*/
                                ++unreachable;
                                break;
			case ICMP_UNREACH_PORT:
#ifndef ARCHAIC
				ip = (struct ip *)packet;
				if (ip->ip_ttl <= 1)
				Fprintf(stdout," !");
#endif /* ARCHAIC */
				++got_there;
				break;
			case ICMP_UNREACH_NET:
				++unreachable;
				Fprintf(stdout," !N");
				break;
			case ICMP_UNREACH_HOST:
				++unreachable;
				Fprintf(stdout," !H");
				break;
			case ICMP_UNREACH_PROTOCOL:
				++got_there;
				Fprintf(stdout," !P");
				break;
			case ICMP_UNREACH_NEEDFRAG:
				if (mtudisc) {
				   /* Doing MTU discovery */
				   mtu = (ntohl(icp->icmp_void) & 0xffff);
				   if (mtu >= datalen) {
				    /*  This should never happen.  There is
					a serious bug somewhere... */
				      Fprintf (stdout," !M>");
				   } else if (mtu == 0) {
				      Fprintf (stdout," !M");
				      new_mtu = reduce_mtu(datalen);
				   } else {
				      new_mtu = mtu;
				      Fprintf (stdout," !M=%d", new_mtu);
				   }
				   break;
				} else {
				  /* Not doing MTU discovery */
				  ++unreachable;
				  Fprintf(stdout," !F");
				  break;
				}
			case ICMP_UNREACH_SRCFAIL:
				++unreachable;
				Fprintf(stdout," !S");
				break;
		    } /* end switch */
}

void print_time(float *dt)
{
   if (utimers) {
      Fprintf(stdout,"  %3.3f ms", *dt);
   } else {
      Fprintf(stdout,"  %d ms", (int) (*(dt)+0.5));
   }
   (void) fflush(stdout);
}


void print_ttl(int ttl)
{
   Fprintf(stdout,"%2d ", ttl);
   (void)fflush(stdout);
}


wait_for_reply(sock, from, deadline)
	int sock;
	struct sockaddr_in *from;
        struct timeval     *deadline;
{
	fd_set fds;
	struct timeval wait;
	struct timeval now;
	int cc = 0;
	int fromlen = sizeof (*from);

#ifndef __linux__
	gettimeofday(&now, NULL);
#else /* __linux__ */
	gettimeofday(&now, &tz);
#endif /* __linux__ */
	if ((now.tv_sec > deadline->tv_sec) ||
	    ( (now.tv_sec == deadline->tv_sec) && 
	     (now.tv_usec > deadline->tv_usec) )  ) return (int)NULL;


	wait.tv_sec= deadline->tv_sec- now.tv_sec;
	if (deadline->tv_usec >= now.tv_usec) {
	  wait.tv_usec= deadline->tv_usec- now.tv_usec;
	} else {
	  wait.tv_usec= (1000000 - now.tv_usec)+ deadline->tv_usec;
	  wait.tv_sec--;
	}

	FD_ZERO(&fds);
	FD_SET(sock, &fds);

	if (select(sock+1, &fds, (fd_set *)0, (fd_set *)0, &wait) > 0)
		cc=recvfrom(s, (char *)packet, sizeof(packet), 0,
			    (struct sockaddr *)from, &fromlen);
	return((int)cc);
}


send_probe(seq, ttl, proto)
int ttl;
int seq;
int proto;
{
	struct opacket *op = outpacket;
	struct ip *ip = &op->ip;
	struct udphdr *up = &op->udp;
	int i;

      retry:
	if (mtudisc) {
	  ip->ip_off = (short) IP_DF;
	}
	else {
	  ip->ip_off = 0;
        }

	ip->ip_p = proto;
#ifndef BYTESWAP_IP_LEN
	ip->ip_len = ((u_short)datalen-_optlen);   /*  The OS inserts options  */
#else /* BYTESWAP_IP_LEN */ 
	ip->ip_len = htons((u_short)datalen-_optlen);   /*  The OS inserts options  */
#endif /* BYTESWAP_IP_LEN */
	ip->ip_ttl = ttl;
        ip->ip_v = IP_VERSION;
        ip->ip_hl = sizeof(*ip) >> 2;

	if (proto == IPPROTO_UDP) {
		up->uh_sport = htons(ident);
		up->uh_dport = htons(port+seq);
		up->uh_ulen = htons((u_short)(datalen - sizeof(struct ip) - _optlen));
		up->uh_sum = 0;
	}

	op->seq = seq;
	op->ttl = ttl;
#ifndef __linux__
	(void) gettimeofday(&op->tv, NULL);
#else /* __linux__ */
	(void) gettimeofday(&op->tv, &tz);
#endif /* __linux__ */

#ifdef SPRAY
	if (spray_mode) {
           if (proto == IPPROTO_UDP) spray[seq].dport = up->uh_dport;
           spray[seq].ttl   = ttl;
           bcopy(&op->tv, &spray[seq].out, sizeof(struct timeval));
	}
#endif

	i = sendto(sndsock, (char *)outpacket, datalen - _optlen, 0, &whereto,
		   sizeof(struct sockaddr));

	if (i < 0 || i != datalen - _optlen)  {
		if (i<0) {
		    if (errno == EMSGSIZE) {
		        datalen = reduce_mtu(datalen);
			Fprintf (stdout," MTU=%d", datalen);
			goto retry;
		    }
		    else
			perror("sendto");
		}
		Fprintf(stderr,"traceroute: wrote %s %d chars, ret=%d%s", hostname,
			datalen, i,terminator);
		(void) fflush(stdout);
	}
}



float deltaT(tp)
	struct timeval *tp;
{
	struct timeval tv;

#ifndef __linux__
	(void) gettimeofday(&tv, NULL);
#else /* __linux__ */
	(void) gettimeofday(&tv, &tz);
#endif /* __linux__ */
	tvsub(&tv, tp);
	return (tv.tv_sec*1000.0 + ((float)tv.tv_usec)/1000.0);
}


/*
 * Convert an ICMP "type" field to a printable string.
 */
char *
pr_type(t)
	u_char t;
{
	static char *ttab[] = {
	"Echo Reply",	"ICMP 1",	"ICMP 2",	"Dest Unreachable",
	"Source Quench", "Redirect",	"ICMP 6",	"ICMP 7",
	"Echo",		"ICMP 9",	"ICMP 10",	"Time Exceeded",
	"Param Problem", "Timestamp",	"Timestamp Reply", "Info Request",
	"Info Reply"
	};

	if(t > 16)
		return("OUT-OF-RANGE");

	return(ttab[t]);
}


/*
 * packet_ok - Make sure it's a real ICMP return of a real UDP packet 
 */
packet_ok(buf, cc, from, seq, proto)
	u_char *buf;
	int cc;
	struct sockaddr_in *from;
	int seq;
	int proto;
{
	u_char type, code;
	int hlen, mtu;
	u_short temp;
	int tmp,tmp2;
	int spr_seq;
	int spr_ttl;
	int probecnt;
	
#ifndef ARCHAIC
	struct ip *ip;

	ip = (struct ip *) buf;
	/* get header length and convert from longwords to bytes */
	if ((ip->ip_v) != IP_VERSION) {
		Fprintf(stderr,"packet version not 4: %d%s",ip->ip_v,terminator);
		return (0);
	}
	hlen = ip->ip_hl <<2 ;
	if (cc < hlen + ICMP_MINLEN) {
		if (verbose)
			Fprintf(stderr,"packet too short (%d bytes) from %s%s", cc,
				inet_ntoa(from->sin_addr),terminator);
		return (0);
	}
	/* go from returned length of packet to cc=data portion */
	cc -= hlen;
	/* make icp point to supposed ICMP portion */
	icp = (struct icmp *) ((u_char *)buf+hlen);
#else
	icp = (struct icmp *)buf;
#endif /* ARCHAIC */
	type = icp->icmp_type; code = icp->icmp_code;
	if ((type == ICMP_TIMXCEED && code == ICMP_TIMXCEED_INTRANS) ||
	    type == ICMP_UNREACH) {
		struct ip *hip;
		struct udphdr *up;
		      
                hip = &icp->icmp_ip;
		hlen = hip->ip_hl << 2;
                if (proto == IPPROTO_UDP) up = (struct udphdr *)((u_char *)hip + hlen);
#ifdef SPRAY
/*
 * First we make sure we got a legal response back, and if so we
 * get the sequence number and ttl out of it 
 */
                if (spray_mode) {
		   spr_seq = ntohs(up->uh_dport)-port;
	   	   if ( (spr_seq >=0) && (spr_seq < max_ttl*nprobes+1) ) {
                      spr_ttl = spray[spr_seq].ttl;
					  response_mask |= (1 << spr_ttl);
/*
 * Now we increment the response count for this ttl, and then if it's the 
 * first, increment the total of ttl's seen 
 */
		      if (spray_rtn[spr_ttl][0] == 0) {
                        spray_total++;
		      } 

              for (probecnt = 0; probecnt < nprobes; ++probecnt) {
		          if (spray_rtn[spr_ttl][probecnt] == 0) {
				      spray_rtn[spr_ttl][probecnt] = spr_seq;
					  break;
				  }
			  }
/*
 * We want to do some heuristics on the smallest TTL received from the target
 * host, but we need the type for that...
 */
                spray[spr_seq].type = type;
                if (type == ICMP_UNREACH_PORT) {
		            if (spr_ttl < spray_min)  {
			            spray_min = spr_ttl;
				    }
			        spray_target = spr_ttl;
		        }
/*
 * We also want the largest TTL we've seen...
 */
                      if (spr_ttl > spray_max)
                         spray_max = spr_ttl;
/*
 * And finally, fill the data structure with the other things we'll need
 * to spit out later, namely the packet transit type and the source IP.
 */
                      gettimeofday(&(spray[spr_seq].rtn),0);
                      bcopy(&from->sin_addr, 
			    &spray[spr_seq].from.sin_addr,
                            sizeof(from->sin_addr));

                   } /* end if sequence number valid */
		} /*end if spray mode */
#endif

		if (hlen + 12 <= cc && hip->ip_p == IPPROTO_UDP &&
		    up->uh_sport == htons(ident) &&
		    up->uh_dport == htons(port+seq))
			return (type == ICMP_TIMXCEED? -1 : code+1);

		if (hlen + 12 <= cc && hip->ip_p == proto)
			return (type == ICMP_TIMXCEED? -1 : code+1);

	} /* end if valid ICMP type */
#ifndef ARCHAIC
	if (verbose) {
		int i;
		u_long *lp = (u_long *)&icp->icmp_ip;

		Fprintf(stderr,"\n%d bytes from %s to %s", cc,
			inet_ntoa(from->sin_addr), inet_ntoa(ip->ip_dst));
		Fprintf(stderr,": icmp type %d (%s) code %d%s", type, pr_type(type),
		       icp->icmp_code,terminator);
		for (i = 4; i < cc ; i += sizeof(long))
			Fprintf(stderr,"%2d: x%8.8lx\n", i, *lp++);
	}
#endif /* ARCHAIC */
	return(0);
}




int reduce_mtu(value)
int value;
{
  /*  The following heuristic taken from RFC1191  */
  static int mtuvals[11]={32000, 17914, 8166, 4352, 2002, 1492, 1006, 508, 296, 68, -1};
  int i=0;

  while (value <= mtuvals[i]) i++;
  if (mtuvals[i] > 0) {
    value = mtuvals[i];
  }
  else {
    Fprintf (stderr," No valid MTU!!!%s",terminator);
    exit (1);
  }
  return (value);
}

char *lookup_owner();
char *doresolve ();
char *lookup_as();

void print_from(struct sockaddr_in *from)
{
   if (0 != memcmp( &from->sin_addr.s_addr, 
		    &addr_last.sin_addr.s_addr,
		    sizeof(addr_last.sin_addr.s_addr) )) {
      if (nflag)
   	 Fprintf(stdout, " %s", inet_ntoa(from->sin_addr));
      else
  	 Fprintf(stdout, " %s (%s)", inetname(from->sin_addr),
	         inet_ntoa(from->sin_addr));

      if (as_lookup) 
         Fprintf(stdout," [%s]", lookup_as(from->sin_addr));

      if (dns_owner_lookup)
         Fprintf(stdout," %s", lookup_owner(from->sin_addr));

      memcpy(&addr_last.sin_addr.s_addr,
 	     &from->sin_addr.s_addr,
	     sizeof(addr_last.sin_addr.s_addr));
      }

}

print(buf, cc, from)
	u_char *buf;
	int cc;
	struct sockaddr_in *from;
{
	struct ip *ip;
	int hlen;

	ip = (struct ip *) buf;
	hlen = ip->ip_hl << 2;
	cc -= hlen;

	if (nflag)
		Fprintf(stdout, " %s", inet_ntoa(from->sin_addr));
	else
		Fprintf(stdout, " %s (%s)", inetname(from->sin_addr),
		       inet_ntoa(from->sin_addr));

        if (as_lookup) 
          Fprintf(stdout," [%s]", lookup_as(from->sin_addr));

	if (dns_owner_lookup)
	  Fprintf(stdout," %s", lookup_owner(from->sin_addr));
	
	if (verbose)
	  Fprintf (stderr," %d bytes to %s", cc, inet_ntoa (ip->ip_dst));
      }



/*
 * Subtract 2 timeval structs:  out = out - in.
 * Out is assumed to be >= in.
 */
tvsub(out, in)
     register struct timeval *out, *in;
{
  if ((out->tv_usec -= in->tv_usec) < 0)   {
    out->tv_sec--;
    out->tv_usec += 1000000;
  }
  out->tv_sec -= in->tv_sec;
}


/*
 * Construct an Internet address representation.
 * If the nflag has been supplied, give 
 * numeric value, otherwise try for symbolic name.
 */
char *
inetname(in)
     struct in_addr in;
{
  register char *cp;
  static char line[50];
  struct hostent *hp;
  static char domain[MAXHOSTNAMELEN + 1];
  static int first = 1;

  if (first && !nflag) {
    first = 0;
    if (gethostname(domain, MAXHOSTNAMELEN) == 0 &&
	(cp = index(domain, '.')))
      (void) strcpy(domain, cp + 1);
    else
      domain[0] = 0;
  }
  cp = 0;
  if (!nflag && in.s_addr != INADDR_ANY) {
    hp = gethostbyaddr((char *)&in, sizeof (in), AF_INET);
    if (hp) {
      if ((cp = index(hp->h_name, '.')) &&
	  !strcmp(cp + 1, domain))
	*cp = 0;
      cp = hp->h_name;
    }
  }
  if (cp)
    (void) strcpy(line, cp);
  else {
    in.s_addr = ntohl(in.s_addr);
#define C(x)	((x) & 0xff)
#ifndef __linux__
    sprintf(line, "%lu.%lu.%lu.%lu", C(in.s_addr >> 24),
#else /* __linux__ */
    sprintf(line, "%u.%u.%u.%u", C(in.s_addr >> 24),	/* Why no lu??? */
#endif /* __linux__ */

	    C(in.s_addr >> 16), C(in.s_addr >> 8), C(in.s_addr));
  }
  return (line);
}

void halt()
{
  haltf++;

#ifndef __linux__
  NOERR(signal(SIGINT,0), "signal SIGINT,0");
#endif /* __linux__ */

}


/*
 *  Lookup owner of the net in DNS.
 */


char *lookup_owner(in)
     struct in_addr in;
{
  char dns_query[100];
  char *owner, *dot_ptr;
  unsigned char *addr_ptr;

  addr_ptr = (unsigned char *) (&in.s_addr);
  
  /* Try /24 */
  sprintf (dns_query, "%d.%d.%d.in-addr.arpa", addr_ptr[2], addr_ptr[1], addr_ptr[0]);
  if (!(owner = doresolve(dns_query))) {
    /* Failed, try /16 */
    sprintf (dns_query, "%d.%d.in-addr.arpa", addr_ptr[1], addr_ptr[0]);
    if (!(owner = doresolve(dns_query))) {
    /* Failed.  If eligible try /8 */
       if (addr_ptr[0] < 128) {
          sprintf (dns_query, "%d.in-addr.arpa", addr_ptr[0]);
          owner = doresolve(dns_query);
       } /* tried /8 for A's */
    } /* tried /16 */
  } /* tried /24 */

  /*  reformat slightly  */
  if (owner == NULL) {
     owner = NO_SOA_RECORD;
  } else {
    dot_ptr = (char *)strchr (owner, (int)'.');
    if (dot_ptr != NULL) 
      *dot_ptr = '@';
    
    if (strlen(owner) > 0) {
      dot_ptr=owner + strlen (owner) - 1;
      while (*dot_ptr == ' ' || *dot_ptr == '.' ) {
	*dot_ptr = 0;
        dot_ptr--;
      }
    }
  }

  return (owner);

}


/*
 *  Lookup origin of the net in radb.
 */

char *lookup_as(in)
struct in_addr in;
{
  static char query[100];
  static unsigned char *addr_ptr;
  static char *sp;
  char *get_origin();

  addr_ptr = (unsigned char *) (&in.s_addr);
  
#ifdef FORCE_NATURAL_MASK
  if (addr_ptr[0] >= 192) {
    sprintf (query, "%d.%d.%d.0",addr_ptr[0],addr_ptr[1],addr_ptr[2]);
  } else if (addr_ptr[0] >= 128) {
    sprintf (query, "%d.%d.0.0",addr_ptr[0],addr_ptr[1]);
  } else {
    sprintf (query, "%d.0.0.0",addr_ptr[0]);
  }
#else
  sprintf (query,"%d.%d.%d.%d",addr_ptr[0],addr_ptr[1],addr_ptr[2],addr_ptr[3]);
#endif /* FORCE_NATURAL_MASK */
   
  sp = get_origin(query);
/*  printf("as_lookup: get_origin returned %d\n",sp); */
  if (0==sp) {
     return((char *)&nullstring);
  } else { 
     return(sp);
  }

}


/*
 * get_origin 	- Return origin (ASnnnn) given a network designation
 *
 * char *get_origin(char *net_designation)
 *
 * Returns:	0 - Error occurred, unable to get origin
 *		!0  Pointer to origin string
 *
 * Define STANDALONE to use this as a client.  Also define EXAMPLE_NET for
 * an example for the truly clueless...
 *
 *	20-May-1995	Ehud Gavron	gavron@aces.com
 *	28-Apr-2000			Return error if no string
 */


/* The following are used to determine which service at which host to
   connect to.  A getenv() of the following elements occurs at run-time,
   which may override these values. */

#define RA_SERVER "whois.ra.net"
#define RA_SERVICE "whois"

/* The following determines what fields will be returned for the -A value
   (/AS_LOOKUP for VMS).  This is the "origin" of the route entry in the 
   RADB. */

#define DATA_DELIMITER "origin:"

/* Since now the RADB has multiple route objects, we will list only the
   origin of the most specific one.  To do so we actually have to parse
   the route lines and look for the most specific route.  To do so we
   parse:
		net.net.net.net/prefix
  
   and use the most specific (largest) prefix.  The following determine
   how we get this.  */

#define ROUTE_DELIMITER "route:"
#define PREFIX_DELIMITER "/"

#ifdef STANDALONE
#ifdef __vms
#include "multinet_root:[multinet.include.sys]types.h"
#include "multinet_root:[multinet.include.sys]socket.h"
#include "multinet_root:[multinet.include.netinet]in.h"
#include <stdio.h>
#include "multinet_root:[multinet.include]netdb.h"
#define perror socket_perror
#define write socket_write
#define read socket_read
#define close socket_close
#else /* not VMS */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <netdb.h>
#endif /* VMS */
#endif /* STANDALONE */

#ifndef boolean
#define boolean int
#endif

#ifndef TRUE
#define TRUE (1==1)
#endif

#ifndef FALSE
#define FALSE (!(TRUE))
#endif
#define MAXREPLYLEN 8192

#ifdef STANDALONE
main(argc,argv)
int argc;
char **argv;
{
	char buffer[100];
	char *p;
	char *get_origin();
#ifdef EXAMPLE_NET
	strcpy(buffer,"192.195.240.0/24");
#else
	strcpy(buffer,argv[1]);
#endif /* EXAMPLE_NET */
	p = get_origin(buffer);
	if (p) {
	   strcpy(buffer,p);
	   Fprintf(stdout,"origin is: %s\n",buffer);
	} else {
    	   Fprintf(stderr,"unable to get origin.\n");
	}
}
#endif /* STANDALONE */

char *get_origin(net)
char *net;
{
	char *i,*j,*k;
	char tmp[100],tmp2[100],tmp3[100]; /* store string delimiters */
	char tmp4[100];			/* here's where we store the AS */
	static	char origin[100];	/* the returned route origin */
	char *rp;			/* pointer to route: line */
	char *pp;			/* pointer to /prefix part of route */
	int prefix;			/* prefix off this line (decimal) */
	int best_prefix;		/* best prefix thus far */
	int s, n, count;
	char buf[256];
	boolean done;
	static char reply[MAXREPLYLEN];
	struct sockaddr_in sin;
	struct hostent *hp;
	struct servent *sp;
	char *getenv();

	/*
 	 * Get the IP address of the host which serves the routing arbiter
	 * database.  We use RA_SERVER.  On the offchance that someone wants
	 * to query another database, we check for the environment variable
	 * RA_SERVER to have been set.
	 */
	if ((i = getenv("RA_SERVER")) == 0) {
	   strcpy(tmp,RA_SERVER);
	} else {
           strncpy(tmp,i,sizeof(tmp));
           tmp[(sizeof(tmp))-1] = '\0';          /* strncpy may not null term */
	} 

	hp = gethostbyname(tmp);
	if (hp == NULL) {
	   Fprintf(stderr, "get_origin: localhost unknown%s",terminator);
	   return(0);
	}

	/*
	 *  Create an IP-family socket on which to make the connection
	 */

	s = socket(hp->h_addrtype, SOCK_STREAM, 0);
	if (s < 0) {
	   perror("get_origin: socket");
	   return(0);
	}

	/*
	 *  Get the TCP port number of the "whois" server.
	 *  Again if this needs to be updated, the environment variable
	 *  RA_SERVICE should be set.
	 */
	if ((i = getenv("RA_SERVICE")) == 0) {
	   strcpy(tmp,RA_SERVICE);
	} else {
           strncpy(tmp,i,sizeof(tmp));
           tmp[(sizeof(tmp))-1] = '\0';          /* strncpy may not null term */
	} 

	sp = getservbyname(tmp,"tcp");
	if (sp == NULL) {
	   Fprintf(stderr, "get_origin: getservbyname: unknown service%s",terminator);
	   return(0);
	}

	/*
	 *  Create a "sockaddr_in" structure which describes the remote
	 *  IP address we want to connect to (from gethostbyname()) and
	 *  the remote TCP port number (from getservbyname()).
	 */

	sin.sin_family = hp->h_addrtype;
	bcopy(hp->h_addr, (caddr_t)&sin.sin_addr, hp->h_length);
	sin.sin_port = sp->s_port;

	/*
	 *  Connect to that address...
	 */

	if (connect(s, (struct sockaddr *)&sin, sizeof (sin)) < 0) {
	   perror("get_origin: connect");
	   return(0);
	}

	/*
	 * Now send the request out to the server...
	 */

	done = FALSE;
	sprintf(buf,"%s\r\n",net);
	write(s, buf, strlen(buf));

	/*
	 * Now get the entire answer in one long buffer...
	 */
	count = 0;
	while ((n = read(s, buf, sizeof(buf))) > 0) {
	    strcpy((char *)&reply[count],(char *)buf);
	    count += n;
	}

	if (n < 0) {
	    perror("get_origin: read");
	    return(0);
	}

	reply[count] = '\0';	/* Terminate it - thanks Joey! */

  	/*
   	 * sometimes there's no answer
	 */
	if (strncmp(reply, "%%  No entries found for the selected source(s).",
	    strlen("%%  No entries found for the selected source(s).")) ==0) {
	   return "NONE";
	}

	/* 
	 * So now we have a large string, somewhere in which we can
	 * find  origin:*AS%%%%%%<lf>.  We parse this into AS%%%%%.
	 */
	
	if ((i = getenv("DATA_DELIMITER")) == 0) {
	   strcpy(tmp,DATA_DELIMITER);
	} else {
           strncpy(tmp,i,sizeof(tmp));
           tmp[(sizeof(tmp))-1] = '\0';          /* strncpy may not null term */
	} 

	/* TMP2 will have the route delimiter... */

	if ((i = getenv("ROUTE_DELIMITER")) == 0) {
	   strcpy(tmp2,ROUTE_DELIMITER);
	} else {
           strncpy(tmp,i,sizeof(tmp));
           tmp[(sizeof(tmp))-1] = '\0';          /* strncpy may not null term */
	} 

	if ((i = getenv("PREFIX_DELIMITER")) == 0) {
	   strcpy(tmp3,PREFIX_DELIMITER);
	} else {
           strncpy(tmp3,i,sizeof(tmp3));
           tmp3[(sizeof(tmp3))-1] = '\0';          /* strncpy may not null term */
	} 
	
/*
 * The next while statement was put in because of SPRINTLINK's ingeneous
 * Reasonable Default announcement project.  They registered nets in the 
 * RADB of the ilk of 0.0.0.0/1, 128.0.0.0/1, 192..../2, etc... just so 
 * that ANS wouldn't be such a pain in the butt.
 *
 * For us this means instead of taking the first origin...we take the best...
 */

/*
 * Initialize it so far as we've seen no prefixes, and are still looking
 * for route entries...
 */
	best_prefix = 0;		/* 0 bits is not very specific */
	done = FALSE;			/* not done finding route: entries */

	rp = (char *)reply;		/* initialize main pointer to buffer */
	origin[0]='\0';			/* initialize returned string */
	reply[MAXREPLYLEN-1]='\0';

	rp = (char *)strstr(rp,tmp2);	/* Find route: in the string */
	while (rp != 0) {		/* If there is such a thing... */
					/*  find it again later */
           pp = (char *)strstr(rp,tmp3);	/* Find / in the route entry */
           if (pp == 0) {		/* No prefix... */
              prefix = 0;		/* So we bias it out of here */
	   } else {
	      prefix = atoi(pp+1);	/* convert to decimal*/
           }

 	   if (prefix >= best_prefix) {	/* it's equal to or better */
	      i = (char *)strstr(pp,tmp);	/* find origin: delimiter */
              if (i != 0) {			/* it's nice if there is one */
	         i += strlen(DATA_DELIMITER);	/* skip delimiter... */
	         i++;				/* and the colon... */
	         while (*i == ' ') i++;		/* skip spaces */
		 /* i now points to start of origin AS string */
	         j = i;				/* terminate... */
		 while (*j >= '0') j++;
	         if (prefix > best_prefix) {
	            strcpy(origin,"/");		/* put a slash in */
	            best_prefix = prefix;		/* update best */
	         } else {
	            strcat(origin,"/");		/* put a mutiple as separator*/
	         }
	         strncpy(tmp4,i,(j-i));		/* copy new origin */
		 tmp4[j-i] = '\0';		/* null terminate it */
	         if (!(strstr(origin,tmp4))) {	/* if it's not a dup */
	            strncat(origin,i,(j-i));	/*  stick it in */
	         } else {
                    if (prefix == best_prefix) 	/* Otherwise remove slash */
                       origin[strlen(origin)-1] = '\0';
                 } /* end if not a dup */
	      } /* end if origin found */
	   } /* endif  prefix > best_prefix */
	   rp = (char *)strstr(rp+1,tmp2);	/* Find route: in the string */
	} /* end while */
	/*
	 * Go home... 
	 */
	close(s);
	if (best_prefix != 0) {			/* did we get anything? */
	   return((char *)&origin[1]);		/* strip off leading slash */
	} else {
	   return(0);
	}	   
}




short getshort(ptr)
char *ptr;
{
    union {
	short retval;
	char ch[2];
    } foo;

    foo.ch[0] = (*ptr & 0xff);
    foo.ch[1] = (*(ptr+1) & 0xff);

    return (foo.retval);
}

char *doresolve (name)
char *name;
{
  int query=QUERY;
  int qtype=T_SOA;
  int qclass=C_IN;
  unsigned char buf[256];
  char *ans;
  int blen, alen, got;
  int anssiz, i;
  short shrt;
  HEADER *h;
  char *contact_ptr;
  int ptr;

  anssiz = 512;
  ans = (char *)malloc(anssiz);
  if (!ans) {
    return(0);
  }

  blen = res_mkquery(query,name,qclass,qtype,NULL,0,NULL,(u_char *)buf,sizeof(buf));
  if (blen < 0) {
    return (0);
  }

  alen = res_send((unsigned char *)buf,blen,(unsigned char *)ans,anssiz);
  if (alen == -1) {
    return (0);
  }
 
  if (alen < 12) {
    return (0);
  }

  h = (HEADER *)ans;
  
  h->id = ntohs(h->id);
  h->qdcount = ntohs(h->qdcount);
  h->ancount = ntohs(h->ancount);
  h->nscount = ntohs(h->nscount);
  h->arcount = ntohs(h->arcount);

  if (h->ancount == 0) return(0);

  ptr = 12;	/* point at first question field */
  for (i=0; i< (int)h->qdcount && ptr<alen; i++) {
    ptr=doqd((unsigned char *)ans,ptr);
  }

  for (i=0; i< (int)h->ancount && ptr<alen; i++) {
    ptr=dorr((unsigned char *)ans,ptr,&contact_ptr);
  }

  return (contact_ptr);
}


doqd(ans,off)
unsigned char *ans;
int off;
{
    char name[256];

    name[0]=0;
    off = doname(ans,off,name);
    off = dotype(ans,off);
    off = doclass(ans,off);
    return (off);
}


dorr(ans,off,contact_ptr)
unsigned char *ans;
int off;
char **contact_ptr;
{
    int class, typ;
    char name[256];

    name[0]=0;
    off = doname(ans,off,name);
    typ = ntohs(getshort(ans+off));
    off = dotype(ans,off);
    class = ntohs(getshort(ans+off));
    off = doclass(ans,off);
    off = dottl(ans,off);
    off = dordata(ans,off,class,typ,name,contact_ptr);
    return(off);
}

doname(ans,off,name)
int off;
unsigned char *ans;
char *name;
{
    int newoff, i;
    char tmp[50];

	/* redirect? */
    if ((*(ans+off) & 0xc0) == 0xc0) {
	newoff = getshort(ans+off);
	newoff = 0x3fff & ntohs(newoff);
	doname(ans,newoff,name);
	return (off+2);
    }
	/* end of string */
    if (*(ans+off) == 0) {
	strcat(name," ");
	return(off+1);
    }
	/* token */
    for (i=1; i<=(int)*(ans+off); i++)
	tmp[i-1]=ans[off+i];
    tmp[i-1] = '.';
    tmp[i]=0;
    strcat(name,tmp);
    return (doname(ans,off+1+(*(ans+off)), name));
}


dotype(ans,off)
int off;
unsigned char *ans;
{
    return(off+2);
}


doclass(ans,off)
int off;
unsigned char *ans;
{
    return(off+2);
}


dottl(ans,off)
int off;
unsigned char *ans;
{
    return(off+4);
}


dordata(ans,off,class,typ,fname,contact_ptr)
unsigned char *ans;
int off,class,typ;
char *fname;
char **contact_ptr;
{
    int len = ntohs(getshort(ans+off));
    int retval = off+len+2;
    int i,j;
    static char name[256];

    off += 2;
    switch (typ) {
	case T_SOA:
	    name[0]=0;
	    off = doname(ans,off,name);
	    name[0]=0;
	    off = doname(ans,off,name);
	    *contact_ptr=name;
	    return (0);
	default:
	    return (0);
    }
}

/*
	The VMS command line interface, DCL, uppercases all unquoted input.
	By default this program can be executed via
		$ mc location:traceroute options

	However, since this program needs case sensitivity for the various 
	options to work, it is defined as an alias ``symbol'' with open 
	quotes:
		$ traceroute == "location:traceroute """

	Unfortunately, this has the side effects of making argc=2, where 
	argv[0]	is the image location and name, and argv[1] is the entire 
	option line.  Thus for VMS we need to break the options up...

	Note that if the symbol VMS_CLD is defined, then this is not used
	at all, but rather the VMS Command Language Definition facility
	is used.

*/

void AbortIfNull (ThePointer)                                    
char *ThePointer;                                                
{                                                                
        if (ThePointer == NULL) {                                
                Fprintf(stderr, "bad flags on that switch!%s",terminator);
                exit(666);                                       
        };                                                       
}                                                                


#ifdef __vms		

			
fixargs(a,b,c)
int *a;			/* argc */
char **b;		/* argv */
char **c;		/* av */
{	
	char *ptr, *space;
	/* Set the image name */
	c[0] = b[0];

	/* Initialize pointers */
	*a = 1;
	ptr = b[1];
	if (*ptr == ' ') ptr++;		/* eliminate first space */

	/* Delineate all strings ending with space */
	while ((space = strchr(ptr,' ')) != 0) {
	   *space = '\0';
	   c[(*a)++] = ptr;
	   ptr = space + 1;
	}
	
	/* Transfer last one - ending with null */
	c[*a] = ptr;

	/* Update argc */
	(*a)++;
}

#endif /* vms */
