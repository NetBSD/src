/*	$NetBSD: getnetgr.c,v 1.1.1.1 1995/10/08 23:08:48 gwr Exp $	*/

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
