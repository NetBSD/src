/* $NetBSD: explicit_bzero.c,v 1.1.4.2 2012/10/30 18:46:14 yamt Exp $ */

#if !defined(_KERNEL) && !defined(_STANDALONE)
#include <string.h>
#define explicit_bzero __explicit_bzero
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
explicit_bzero(void *b, size_t len)
{

	(*explicit_memset_impl)(b, 0, len);
}
