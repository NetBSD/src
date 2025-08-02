/*	$NetBSD: includes.h,v 1.9.4.1 2025/08/02 05:18:45 perseant Exp $	*/
#include <sys/cdefs.h>
#ifndef __OpenBSD__
#define __bounded__(a, b, c)
#include <sys/types.h>

#include "namespace.h"

#ifdef USE_PAM
#include <security/pam_appl.h>
#endif

void freezero(void *, size_t);
#define explicit_bzero(a, b) explicit_memset((a), 0, (b))
#define timingsafe_bcmp(a, b, c) (!consttime_memequal((a), (b), (c)))
void	*recallocarray(void *, size_t, size_t, size_t);
#endif

