/* $NetBSD: conf.h,v 1.3 1998/03/24 05:17:14 thorpej Exp $ */

#include <sys/conf.h>
#include <machine/cpuconf.h>

cdev_decl(a12dc);
cdev_decl(scc);
cdev_decl(zs);
cdev_decl(prom);			/* XXX */
