/*	$NetBSD: conf.h,v 1.1.4.2 2000/06/22 17:03:13 minoura Exp $	*/

#include <sys/conf.h>

#define mmread mmrw
#define mmwrite mmrw

cdev_decl(mm);
bdev_decl(sw);
cdev_decl(sw);
cdev_decl(scsibus);
cdev_decl(arcs);
