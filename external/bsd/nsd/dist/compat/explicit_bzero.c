/*	$OpenBSD: explicit_bzero.c,v 1.4 2015/08/31 02:53:57 guenther Exp $ */
/*
 * Public domain.
 * Written by Matthew Dempsky.
 */
#include "config.h"
#include <string.h>

#ifdef HAVE_ATTR_WEAK
__attribute__((weak)) void
#else
void
#endif
__explicit_bzero_hook(void *ATTR_UNUSED(buf), size_t ATTR_UNUSED(len))
{
}

void
explicit_bzero(void *buf, size_t len)
{
	memset(buf, 0, len);
	__explicit_bzero_hook(buf, len);
}
