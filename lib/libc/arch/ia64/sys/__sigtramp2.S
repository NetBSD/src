/*	$NetBSD: __sigtramp2.S,v 1.2 2020/10/15 05:50:14 skrll Exp $	*/

#include "SYS.h"

/* XXX:	 Fix this when we look at signal handling */

ENTRY(__sigtramp_siginfo_2, 2)
	/* XXX:	 get pointer to ucontext */
	/* setcontext() ie. CALLSYS_NOERROR(setcontext)  */
	/* get error code */
	CALLSYS_NOERROR(exit) /* exit */
END(__sigtramp_siginfo_2)
