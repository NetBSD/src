/*	$NetBSD: altqconf.h,v 1.2.2.1 2002/10/10 18:30:08 jdolecek Exp $	*/

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
