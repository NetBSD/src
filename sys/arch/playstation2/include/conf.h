/*	$NetBSD: conf.h,v 1.1 2001/10/16 15:38:43 uch Exp $	*/

#include <sys/conf.h>

#define mmread mmrw
#define mmwrite mmrw

cdev_decl(mm);
