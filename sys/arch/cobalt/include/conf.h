/*	$NetBSD: conf.h,v 1.1 2000/03/19 23:07:46 soren Exp $	*/

#include <sys/conf.h>

#define mmread mmrw
#define mmwrite mmrw

cdev_decl(mm);
