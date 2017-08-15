/* $NetBSD: explicit_memset.c,v 1.1.2.1 2017/08/15 04:53:01 snj Exp $ */

/*
 * Written by Matthias Drochner <drochner@NetBSD.org>.
 * Public domain.
 */

#include "netbsd_ssh_compat.h"

#if !defined(_KERNEL) && !defined(_STANDALONE)
#include "namespace.h"
#include <string.h>
#if 0 /* XXXMRG netbsd-6 ssh */
#ifdef __weak_alias
__weak_alias(explicit_memset,_explicit_memset)
#endif
#endif
#define explicit_memset_impl __explicit_memset_impl
#else
#include <lib/libkern/libkern.h>
#endif

/*
 * The use of a volatile pointer guarantees that the compiler
 * will not optimise the call away.
 */
void *(* volatile explicit_memset_impl)(void *, int, size_t) = memset;

void *
explicit_memset(void *b, int c, size_t len)
{

	return (*explicit_memset_impl)(b, c, len);
}
