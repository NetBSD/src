/* $NetBSD: Lint_Ovfork.c,v 1.2.2.1 2025/08/02 05:54:35 perseant Exp $ */

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
