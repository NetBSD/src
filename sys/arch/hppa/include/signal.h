/*	$NetBSD: signal.h,v 1.12 2021/10/28 11:25:02 christos Exp $	*/

/*	$OpenBSD: signal.h,v 1.1 1998/06/23 19:45:27 mickey Exp $	*/

/*
 * Copyright (c) 1994, The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 * 	Utah $Hdr: signal.h 1.3 94/12/16$
 */

#ifndef _HPPA_SIGNAL_H__
#define _HPPA_SIGNAL_H__

/*
 * Machine-dependent signal definitions
 */

#include <sys/featuretest.h>
#include <sys/sigtypes.h>

typedef int sig_atomic_t;

#if defined(_XOPEN_SOURCE) || defined(_NETBSD_SOURCE)
#include <machine/trap.h>	/* codes for SIGILL, SIGFPE */
#endif

#if defined(_LIBC)
#define	__HAVE_STRUCT_SIGCONTEXT
/*
 * Information pushed on stack when a signal is delivered.
 * This is used by the kernel to restore state following
 * execution of the signal handler.  It is also made available
 * to the handler to allow it to restore state properly if
 * a non-standard exit is performed.
 */
struct	sigcontext {
	int	sc_onstack;		/* sigstack state to restore */
	int	__sc_mask13;		/* signal mask to restore (old style) */
	int	sc_sp;			/* sp to restore */
	int	sc_fp;			/* fp to restore */
	int	sc_ap;			/* ap to restore */
	int	sc_pcsqh;		/* pc space queue (head) to restore */
	int	sc_pcoqh;		/* pc offset queue (head) to restore */
	int	sc_pcsqt;		/* pc space queue (tail) to restore */
	int	sc_pcoqt;		/* pc offset queue (tail) to restore */
	int	sc_ps;			/* psl to restore */
	sigset_t sc_mask;		/* signal mask to restore (new style) */
};
#endif

#endif /* _HPPA_SIGNAL_H__ */
