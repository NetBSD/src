/*	$NetBSD: includes.h,v 1.8.4.1 2023/12/25 12:31:04 martin Exp $	*/
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

