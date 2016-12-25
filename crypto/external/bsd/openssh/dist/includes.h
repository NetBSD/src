/*	$NetBSD: includes.h,v 1.5 2016/12/25 00:07:47 christos Exp $	*/
#include <sys/cdefs.h>
#ifndef __OpenBSD__
#define __bounded__(a, b, c)
#define explicit_bzero(a, b) explicit_memset((a), 0, (b))
#define timingsafe_bcmp(a, b, c) (!consttime_memequal((a), (b), (c)))
#endif

#include "namespace.h"
