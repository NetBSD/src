/*	$NetBSD: altqconf.h,v 1.1 2000/12/14 23:50:43 thorpej Exp $	*/

#if defined(_KERNEL) && !defined(_LKM)
#include "opt_altq_enabled.h"

#include <sys/conf.h>

#ifdef ALTQ
#define	NALTQ	1
#else
#define	NALTQ	0
#endif

cdev_decl(altq);

#define	cdev_altq_init(x,y)	cdev__oci_init(x,y)
#endif /* _KERNEL && ! _LKM */
