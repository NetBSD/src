/*	$NetBSD: conf.h,v 1.1 2000/06/14 15:39:57 soren Exp $	*/

#include <sys/conf.h>

#define mmread mmrw
#define mmwrite mmrw

cdev_decl(mm);
bdev_decl(sw);
cdev_decl(sw);
cdev_decl(scsibus);
cdev_decl(arcs);
