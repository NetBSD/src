/* $NetBSD: rmalloc.c,v 1.1.12.1 2006/07/13 17:48:43 gdamore Exp $ */

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
