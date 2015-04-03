/*	$NetBSD: includes.h,v 1.4 2015/04/03 23:58:19 christos Exp $	*/
#include <sys/cdefs.h>
#ifndef __OpenBSD__
#define __bounded__(a, b, c)
#define explicit_bzero(a, b) explicit_memset((a), 0, (b))
#define timingsafe_bcmp(a, b, c) (!consttime_memequal((a), (b), (c)))
#endif

#include "namespace.h"
