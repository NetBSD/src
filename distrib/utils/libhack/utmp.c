/*	$NetBSD: utmp.c,v 1.1 1999/05/19 03:54:40 gwr Exp $	*/

/*
 * Written by Gordon W. Ross <gwr@netbsd.org>
 * Public domain.
 */

/* Simplified (do nothing:) */
#include <sys/types.h>
#include <utmp.h>

void
login(ut)
	struct utmp *ut;
{
}

int
logout(line)
	const char *line;
{
}


void
logwtmp(line, name, host)
	const char *line, *name, *host;
{
}
