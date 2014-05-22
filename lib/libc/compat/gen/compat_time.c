/*	$NetBSD: compat_time.c,v 1.2.8.1 2014/05/22 11:36:51 yamt Exp $	*/

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
__weak_alias(time,_time)
#endif

__warn_references(time,
    "warning: reference to compatibility time();"
    " include <time.h> for correct reference")

#define timeval timeval50
#define time_t int32_t

#define gettimeofday __compat_gettimeofday

#include "gen/time.c"
