/*	$NetBSD: utmp.c,v 1.2 1999/05/27 05:41:15 gwr Exp $	*/

/*
 * Written by Gordon W. Ross <gwr@netbsd.org>
 * Public domain.
 */

/* Simplified (do nothing:) */
#include <sys/types.h>
#include <utmp.h>
#include <util.h>

void
login(ut)
	struct utmp *ut;
{
}

int
logout(line)
	const char *line;
{
	return(0);
}


void
logwtmp(line, name, host)
	const char *line, *name, *host;
{
}
