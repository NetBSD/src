/* $NetBSD: rmalloc.c,v 1.1.14.3 2006/12/30 20:45:21 yamt Exp $ */

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
