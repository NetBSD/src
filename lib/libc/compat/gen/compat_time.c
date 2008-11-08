/*	$NetBSD: compat_time.c,v 1.1.2.1 2008/11/08 21:49:34 christos Exp $	*/

/*
 * Written by Jason R. Thorpe <thorpej@NetBSD.org>, October 21, 1997.
 * Public domain.
 */

#include "namespace.h"
#include <sys/cdefs.h>

#define __LIBC12_SOURCE__
#define time_t __time_t
#include <time.h>
#include <sys/time.h>
#undef time_t
typedef int32_t time_t;
#include <compat/include/time.h>
#include <compat/sys/time.h>

#ifdef __weak_alias
__weak_alias(time,_time)
#endif

__warn_references(time,
    "warning: reference to compatibility time();"
    " include <time.h> for correct reference")

#define timeval timeval50
#include "gen/time.c"
