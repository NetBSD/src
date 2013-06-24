/* $NetBSD: explicit_memset.c,v 1.1 2013/06/24 04:21:19 riastradh Exp $ */

#if !defined(_KERNEL) && !defined(_STANDALONE)
#include <string.h>
#define explicit_memset __explicit_memset
#define explicit_memset_impl __explicit_memset_impl
#else
#include <lib/libkern/libkern.h>
#endif

/*
 * The use of a volatile pointer guarantees that the compiler
 * will not optimise the call away.
 */
void *(* volatile explicit_memset_impl)(void *, int, size_t) = memset;

void
explicit_memset(void *b, int c, size_t len)
{

	(*explicit_memset_impl)(b, c, len);
}
