/*	$NetBSD: getif.h,v 1.2 1998/01/09 08:09:09 perry Exp $	*/

/* getif.h */

#ifdef	__STDC__
extern struct ifreq *getif(int, struct in_addr *);
#else
extern struct ifreq *getif();
#endif
