/*
 * dstore.c - BSDI global storage for lsof
 */


/*
 * Copyright 1994 Purdue Research Foundation, West Lafayette, Indiana
 * 47907.  All rights reserved.
 *
 * Written by Victor A. Abell
 *
 * This software is not subject to any license of the American Telephone
 * and Telegraph Company or the Regents of the University of California.
 *
 * Permission is granted to anyone to use this software for any purpose on
 * any computer system, and to alter it and redistribute it freely, subject
 * to the following restrictions:
 *
 * 1. Neither the authors nor Purdue University are responsible for any
 *    consequences of the use of this software.
 *
 * 2. The origin of this software must not be misrepresented, either by
 *    explicit claim or by omission.  Credit to the authors and Purdue
 *    University must appear in documentation and sources.
 *
 * 3. Altered versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 4. This notice may not be removed or altered.
 */

#ifndef lint
static char copyright[] =
"@(#) Copyright 1994 Purdue Research Foundation.\nAll rights reserved.\n";
static char *rcsid = "$Id$";
#endif


#include "lsof.h"


KA_T Cfp;			/* current file's file struct pointer kernel
				 * address */


/*
 * Drive_Nl -- table to drive the building of Nl[] via build_Nl()
 *             (See lsof.h and misc.c.)
 */

struct drive_Nl Drive_Nl[] = {

#if	BSDIV<20100
	{ X_NCACHE,	"_nchhead"	},
	{ X_NCSIZE,	"_numcache"	},
#else	/* BSDIV>=20100 */
	{ X_NCACHE,	"_nchashtbl"	},
	{ X_NCSIZE,	"_nchash"	},
#endif	/* BSDIV<20100 */

	{ "",		""		},
	{ NULL,		NULL		}
};

kvm_t *Kd;			/* kvm descriptor */
KA_T Kpa;			/* kernel proc struct address */

struct l_vfs *Lvfs = NULL;	/* local vfs structure table */

int Np = 0;			/* number of kernel processes */
struct kinfo_proc *P = NULL;	/* local process table copy */

#if	defined(HASFSTRUCT)
/*
 * Pff_tab[] - table for printing file flags
 */

struct pff_tab Pff_tab[] = {
	{ (long)FREAD,		FF_READ		},
	{ (long)FWRITE,		FF_WRITE	},
	{ (long)FNONBLOCK,	FF_NBLOCK	},
	{ (long)FNDELAY,	FF_NDELAY	},
	{ (long)FAPPEND,	FF_APPEND	},
	{ (long)FASYNC,		FF_ASYNC	},
	{ (long)FFSYNC,		FF_FSYNC	},
	{ (long)FMARK,		FF_MARK		},
	{ (long)FHASLOCK,	FF_HASLOCK	},
	{ (long)FDEFER,		FF_DEFER	},
	{ (long)O_SHLOCK,	FF_SHLOCK	},
	{ (long)O_EXLOCK,	FF_EXLOCK	},
	{ (long)0,		NULL 		}
};


/*
 * Pof_tab[] - table for print process open file flags
 */

struct pff_tab Pof_tab[] = {

	{ (long)UF_EXCLOSE,	POF_CLOEXEC	},
	{ (long)UF_MAPPED,	POF_MAPPED	},
	{ (long)0,		NULL		}
};
#endif	/* defined(HASFSTRUCT) */

int pgshift = 0;		/* kernel's page shift */
