/*	$NetBSD: includes.h,v 1.6.4.1 2017/12/04 10:55:18 snj Exp $	*/
#include <sys/cdefs.h>
#ifndef __OpenBSD__
#define __bounded__(a, b, c)
#define explicit_bzero(a, b) explicit_memset((a), 0, (b))
#define timingsafe_bcmp(a, b, c) (!consttime_memequal((a), (b), (c)))
#include <sys/types.h>
void	*recallocarray(void *, size_t, size_t, size_t);
#endif

#include "namespace.h"
