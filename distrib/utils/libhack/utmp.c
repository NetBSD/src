/*	$NetBSD: utmp.c,v 1.3 1999/06/21 02:32:20 danw Exp $	*/

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
	const struct utmp *ut;
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
