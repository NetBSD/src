/* $NetBSD: Lint___vfork14.c,v 1.5 2024/12/01 16:16:57 rillig Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#define __LIBC12_SOURCE__
#include <unistd.h>
#include <compat/include/unistd.h>

pid_t
__vfork14(void)
{
	return 0;
}
