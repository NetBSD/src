/*	$NetBSD: altqconf.h,v 1.1.4.1 2001/06/21 18:12:50 nathanw Exp $	*/

#if defined(_KERNEL_OPT)
#include "opt_altq_enabled.h"

#include <sys/conf.h>

#ifdef ALTQ
#define	NALTQ	1
#else
#define	NALTQ	0
#endif

cdev_decl(altq);

#define	cdev_altq_init(x,y)	cdev__oci_init(x,y)
#endif /* _KERNEL_OPT */
