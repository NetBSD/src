/*	$NetBSD: conf.h,v 1.1.6.2 2000/11/20 20:07:03 bouyer Exp $	*/

#include <sys/conf.h>

#define mmread mmrw
#define mmwrite mmrw

cdev_decl(mm);
