/* $NetBSD: identd.h,v 1.8 2005/04/03 22:15:32 peter Exp $ */

/*
 * identd.h - TCP/IP Ident protocol server.
 *
 * This software is in the public domain.
 * Written by Peter Postma <peter@NetBSD.org>
 */

#ifndef _IDENTD_H_
#define _IDENTD_H_

#define satosin(sa)	((struct sockaddr_in *)(sa))
#define satosin6(sa)	((struct sockaddr_in6 *)(sa))
#define in_hosteq(s,t)	((s).s_addr == (t).s_addr)

void maybe_syslog(int, const char *, ...);

#ifdef WITH_PF
int pf_natlookup(struct sockaddr_storage *, struct sockaddr *, int *);
#endif

#ifdef WITH_IPF
int ipf_natlookup(struct sockaddr_storage *, struct sockaddr *, int *);
#endif

#endif /* !_IDENTD_H_ */
