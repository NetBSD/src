/*	$NetBSD: netgroup.h,v 1.1.1.1 2002/06/20 10:30:09 itojun Exp $	*/

#ifndef netgroup_h
#define netgroup_h

int getnetgrent(const char **machinep, const char **userp,
		const char **domainp);

int getnetgrent_r(char **machinep, char **userp, char **domainp,
		  char *buffer, int buflen);

void setnetgrent(const char *netgroup);

void endnetgrent(void);

int innetgr(const char *netgroup, const char *machine,
	    const char *user, const char *domain);

#endif
