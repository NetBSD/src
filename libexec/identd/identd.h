/* $NetBSD: identd.h,v 1.11 2016/12/10 05:43:11 christos Exp $ */

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
#define csatosin(sa)	((const struct sockaddr_in *)(sa))
#define csatosin6(sa)	((const struct sockaddr_in6 *)(sa))
#define in_hosteq(s,t)	((s).s_addr == (t).s_addr)

void maybe_syslog(int, const char *, ...) __sysloglike(2, 3);

#ifdef WITH_PF
int pf_natlookup(const struct sockaddr_storage *, struct sockaddr_storage *,
    in_port_t *);
#endif

#ifdef WITH_IPF
int ipf_natlookup(const struct sockaddr_storage *, struct sockaddr_storage *,
    in_port_t *);
#endif

#ifdef WITH_NPF
int npf_natlookup(const struct sockaddr_storage *, struct sockaddr_storage *,
    in_port_t *);
#endif

#endif /* !_IDENTD_H_ */
