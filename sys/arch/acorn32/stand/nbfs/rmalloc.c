/* $NetBSD: rmalloc.c,v 1.2 2006/07/13 16:48:18 bjh21 Exp $ */

#include <lib/libsa/stand.h>
#include <riscoscalls.h>

void *
alloc(size_t size)
{
	void *ret;

	if (xosmodule_alloc(size, &ret) != NULL)
		return NULL;
	return ret;
}

void dealloc(void *ptr, size_t size)
{

	xosmodule_free(ptr);
}
