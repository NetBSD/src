/*	$NetBSD: getnetgr.c,v 1.4 2001/06/15 17:26:51 tsutsui Exp $	*/

#include <sys/cdefs.h>

#ifdef __weak_alias
#define endnetgrent		_endnetgrent
#define innetgr			_innetgr
#define getnetgrent		_getnetgrent
#define setnetgrent		_setnetgrent
#endif

#include <netgroup.h>

#ifdef __weak_alias
__weak_alias(endnetgrent,_endnetgrent)
__weak_alias(getnetgrent,_getnetgrent)
__weak_alias(innetgr,_innetgr)
__weak_alias(setnetgrent,_setnetgrent)
#endif

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
