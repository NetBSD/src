/*	$NetBSD: getnetgr.c,v 1.3 1999/03/13 19:08:44 sommerfe Exp $	*/

#include <netgroup.h>

/*
 * Just stub these out, so it looks like
 * we are not in any any netgroups.
 */

void
endnetgrent()
{
}

void
setnetgrent(ng)
	const char	*ng;
{
}

int
getnetgrent(host, user, domain)
	const char	**host;
	const char	**user;
	const char	**domain;
{
	return 0;
}

int
innetgr(grp, host, user, domain)
	const char	*grp, *host, *user, *domain;
{
	return 0;
}
