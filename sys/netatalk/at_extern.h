/*	$NetBSD: at_extern.h,v 1.2 1997/04/02 21:47:01 christos Exp $	*/

/*
 * Copyright (c) 1990,1994 Regents of The University of Michigan.
 * All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation, and that the name of The University
 * of Michigan not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. This software is supplied as is without expressed or
 * implied warranties of any kind.
 *
 * This product includes software developed by the University of
 * California, Berkeley and its contributors.
 *
 *	Research Systems Unix Group
 *	The University of Michigan
 *	c/o Wesley Craig
 *	535 W. William Street
 *	Ann Arbor, Michigan
 *	+1-313-764-2278
 *	netatalk@umich.edu
 */

#ifndef _NETATALK_AT_EXTERN_H_
#define _NETATALK_AT_EXTERN_H_

struct ifnet;
struct mbuf;
struct sockaddr_at;
struct proc;
struct at_ifaddr;
struct route;
struct socket;

void	atintr		__P((void));
void	aarpprobe	__P((void *));
int	aarpresolve	__P((struct ifnet *, struct mbuf *,
    struct sockaddr_at *, u_char *));
void	aarpinput	__P((struct ifnet *, struct mbuf *));
int	at_broadcast	__P((struct sockaddr_at  *));
void	aarp_clean	__P((void));
int	at_control	__P((u_long, caddr_t, struct ifnet *, struct proc *));
u_int16_t
	at_cksum	__P((struct mbuf *, int));
int	ddp_usrreq	__P((struct socket *, int, struct mbuf *, struct mbuf *,
    struct mbuf *, struct proc *));
void	ddp_init	__P((void ));
struct ifaddr *
	at_ifawithnet	__P((struct sockaddr_at *, struct ifaddr *));
int	ddp_output	__P((struct mbuf *, ...));
struct ddpcb  *
	ddp_search	__P((struct sockaddr_at *, struct sockaddr_at *,
    struct at_ifaddr *));
int     ddp_route	__P((struct mbuf *, struct route *));


#endif /* _NETATALK_AT_EXTERN_H_ */
