/* $NetBSD: conf.h,v 1.3.14.1 2000/11/20 19:56:48 bouyer Exp $ */

#include <sys/conf.h>
#include <machine/cpuconf.h>

bdev_decl(fd);
cdev_decl(fd);

cdev_decl(a12dc);
cdev_decl(scc);
cdev_decl(zs);
cdev_decl(prom);			/* XXX */
