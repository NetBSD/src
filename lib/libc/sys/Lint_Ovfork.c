/* $NetBSD: Lint_Ovfork.c,v 1.2.6.1 2000/06/23 16:18:07 minoura Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#if 0
#include <unistd.h>
#else
#include <sys/cdefs.h>
#include <sys/types.h>
pid_t vfork __P((void));
#endif

/*ARGSUSED*/
pid_t
vfork()
{
	return (0);
}
