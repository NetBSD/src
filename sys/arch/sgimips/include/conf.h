/*	$NetBSD: conf.h,v 1.1.6.2 2000/11/20 20:23:44 bouyer Exp $	*/

#include <sys/conf.h>

#define mmread mmrw
#define mmwrite mmrw

cdev_decl(mm);
bdev_decl(sw);
cdev_decl(sw);
cdev_decl(scsibus);
cdev_decl(arcs);
