/* $NetBSD: Lint_Ovfork.c,v 1.3 2024/12/01 16:16:56 rillig Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <unistd.h>

pid_t
vfork(void)
{
	return 0;
}
