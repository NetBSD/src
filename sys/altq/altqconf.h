/*	$NetBSD: altqconf.h,v 1.2.14.1 2002/05/16 12:29:27 gehenna Exp $	*/

#if defined(_KERNEL_OPT)
#include "opt_altq_enabled.h"

#include <sys/conf.h>

#ifdef ALTQ
#define	NALTQ	1
#else
#define	NALTQ	0
#endif

#endif /* _KERNEL_OPT */
