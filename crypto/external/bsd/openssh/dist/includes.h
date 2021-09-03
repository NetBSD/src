/*	$NetBSD: includes.h,v 1.9 2021/09/03 10:30:33 christos Exp $	*/
#include <sys/cdefs.h>
#ifndef __OpenBSD__
#define __bounded__(a, b, c)
#include <sys/types.h>

#include "namespace.h"

void freezero(void *, size_t);
#define explicit_bzero(a, b) explicit_memset((a), 0, (b))
#define timingsafe_bcmp(a, b, c) (!consttime_memequal((a), (b), (c)))
void	*recallocarray(void *, size_t, size_t, size_t);
#endif

