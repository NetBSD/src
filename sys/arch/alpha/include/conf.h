/* $NetBSD: conf.h,v 1.4 2000/04/10 01:19:13 chs Exp $ */

#include <sys/conf.h>
#include <machine/cpuconf.h>

bdev_decl(fd);
cdev_decl(fd);

cdev_decl(a12dc);
cdev_decl(scc);
cdev_decl(zs);
cdev_decl(prom);			/* XXX */
