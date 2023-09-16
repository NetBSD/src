/*	$NetBSD: compat_asctime.c,v 1.1 2023/09/16 18:40:26 christos Exp $	*/

/*
 * Written by Jason R. Thorpe <thorpej@NetBSD.org>, October 21, 1997.
 * Public domain.
 */

#include "namespace.h"
#include <sys/cdefs.h>

#define __LIBC12_SOURCE__
#include <time.h>
#include <sys/time.h>
#include <compat/include/time.h>
#include <compat/sys/time.h>

#ifdef __weak_alias
__weak_alias(ctime_r,_ctime_r)
__weak_alias(ctime_rz,_ctime_rz)
#endif

__warn_references(ctime_r,
    "warning: reference to compatibility ctime_r();"
    " include <time.h> for correct reference")
__warn_references(ctime_rz,
    "warning: reference to compatibility ctime_rz();"
    " include <time.h> for correct reference")

#define timeval timeval50
#define timespec timespec50
#define	time_t	int32_t

#include "time/asctime.c"
