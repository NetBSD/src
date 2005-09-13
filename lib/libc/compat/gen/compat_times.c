/*	$NetBSD: compat_times.c,v 1.1 2005/09/13 01:44:09 christos Exp $	*/

/*
 * Ben Harris, 2002.
 * This file is in the Public Domain.
 */

#define __LIBC12_SOURCE__
#include "namespace.h"
#include <sys/cdefs.h>
#include <time.h>
#include <compat/include/time.h>
#include <sys/times.h>
#include <compat/sys/times.h>

#ifdef __weak_alias
__weak_alias(times,_times)
#endif

__warn_references(times,
    "warning: reference to compatibility times(); include <sys/times.h> for correct reference")


#include "gen/times.c"
