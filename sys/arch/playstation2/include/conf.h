/*	$NetBSD: conf.h,v 1.1.6.2 2002/01/10 19:47:36 thorpej Exp $	*/

#include <sys/conf.h>

#define mmread mmrw
#define mmwrite mmrw

cdev_decl(mm);
