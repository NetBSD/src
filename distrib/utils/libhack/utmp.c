/*	$NetBSD: utmp.c,v 1.5 2003/07/26 17:07:37 salo Exp $	*/

/*
 * Written by Gordon W. Ross <gwr@NetBSD.org>
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

void
loginx(ut)
	const struct utmpx *ut;
{
}

int
logout(line)
	const char *line;
{
	return(0);
}

int
logoutx(line, status, type)
	const char *line;
	int status, type;
{
	return(0);
}

void
logwtmp(line, name, host)
	const char *line, *name, *host;
{
}

void
logwtmpx(line, name, host, status, type)
	const char *line, *name, *host;
	int status, type;
{
}
