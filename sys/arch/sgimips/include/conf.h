/*	$NetBSD: conf.h,v 1.2 2001/07/08 20:30:13 thorpej Exp $	*/

#include <sys/conf.h>

#define mmread mmrw
#define mmwrite mmrw

cdev_decl(mm);
bdev_decl(sw);
cdev_decl(sw);
cdev_decl(scsibus);
