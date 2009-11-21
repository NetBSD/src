/* $NetBSD: stubs.c,v 1.10 2009/11/21 20:32:17 rmind Exp $ */
/*
 * stubs.c -- functions I haven't written yet
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: stubs.c,v 1.10 2009/11/21 20:32:17 rmind Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

int
suibyte(void *base, int c)
{
	panic("suibyte not implemented");
}

int
suisword(void *base, short c)
{
	panic("suisword not implemented");
}

int
suiword(void *base, long c)
{
	panic("suiword not implemented");
}

int
fuibyte(const void *base)
{
	panic("fuibyte not implemented");
}

int
fuisword(const void *base)
{
	panic("fuisword not implemented");
}

long
fuiword(const void *base)
{
	panic("fuiword not implemented");
}
