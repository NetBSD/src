/*	$NetBSD: altqconf.h,v 1.1.4.3 2002/10/18 02:33:23 nathanw Exp $	*/

#ifdef _KERNEL

#if defined(_KERNEL_OPT)
#include "opt_altq_enabled.h"
#endif

#include <sys/conf.h>

#ifdef ALTQ
#define	NALTQ	1
#else
#define	NALTQ	0
#endif

#endif /* _KERNEL */
