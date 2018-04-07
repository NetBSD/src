/*	$NetBSD: includes.h,v 1.7.2.1 2018/04/07 04:11:48 pgoyette Exp $	*/
#include <sys/cdefs.h>
#ifndef __OpenBSD__
#define __bounded__(a, b, c)
#include <sys/types.h>
void freezero(void *, size_t);
#define explicit_bzero(a, b) explicit_memset((a), 0, (b))
#define timingsafe_bcmp(a, b, c) (!consttime_memequal((a), (b), (c)))
#include <sys/types.h>
void	*recallocarray(void *, size_t, size_t, size_t);
#endif

#include "namespace.h"
