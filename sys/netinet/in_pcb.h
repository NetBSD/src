/*	$NetBSD: in_pcb.h,v 1.38 2004/04/21 17:49:46 itojun Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)in_pcb.h	8.1 (Berkeley) 6/10/93
 */

#ifndef _NETINET_IN_PCB_H_
#define _NETINET_IN_PCB_H_

#include <sys/queue.h>
#include <netinet/in_pcb_hdr.h>

/*
 * Common structure pcb for internet protocol implementation.
 * Here are stored pointers to local and foreign host table
 * entries, local and foreign socket numbers, and pointers
 * up (to a socket structure) and down (to a protocol-specific)
 * control block.
 */
struct inpcb {
	struct inpcb_hdr inp_head;
#define inp_hash	inp_head.inph_hash
#define inp_queue	inp_head.inph_queue
#define inp_af		inp_head.inph_af
#define inp_ppcb	inp_head.inph_ppcb
#define inp_state	inp_head.inph_state
#define inp_socket	inp_head.inph_socket
#define inp_table	inp_head.inph_table
#define inp_sp		inp_head.inph_sp
	struct	  route inp_route;	/* placeholder for routing entry */
	u_int16_t inp_fport;		/* foreign port */
	u_int16_t inp_lport;		/* local port */
	int	  inp_flags;		/* generic IP/datagram flags */
	struct	  ip inp_ip;		/* header prototype; should have more */
	struct	  mbuf *inp_options;	/* IP options */
	struct	  ip_moptions *inp_moptions; /* IP multicast options */
	int	  inp_errormtu;		/* MTU of last xmit status = EMSGSIZE */
};

#define	inp_faddr	inp_ip.ip_dst
#define	inp_laddr	inp_ip.ip_src

/* flags in inp_flags: */
#define	INP_RECVOPTS		0x01	/* receive incoming IP options */
#define	INP_RECVRETOPTS		0x02	/* receive IP options for reply */
#define	INP_RECVDSTADDR		0x04	/* receive IP dst address */
#define	INP_HDRINCL		0x08	/* user supplies entire IP header */
#define	INP_HIGHPORT		0x10	/* (unused; FreeBSD compat) */
#define	INP_LOWPORT		0x20	/* user wants "low" port binding */
#define	INP_ANONPORT		0x40	/* port chosen for user */
#define	INP_RECVIF		0x80	/* receive incoming interface */
#define	INP_CONTROLOPTS		(INP_RECVOPTS|INP_RECVRETOPTS|INP_RECVDSTADDR|\
				INP_RECVIF)

#define	sotoinpcb(so)		((struct inpcb *)(so)->so_pcb)

#ifdef _KERNEL
void	in_losing(struct inpcb *);
int	in_pcballoc(struct socket *, void *);
int	in_pcbbind(void *, struct mbuf *, struct proc *);
int	in_pcbconnect(void *, struct mbuf *);
void	in_pcbdetach(void *);
void	in_pcbdisconnect(void *);
void	in_pcbinit(struct inpcbtable *, int, int);
struct inpcb *
	in_pcblookup_port(struct inpcbtable *,
	    struct in_addr, u_int, int);
struct inpcb *
	in_pcblookup_bind(struct inpcbtable *,
	    struct in_addr, u_int);
struct inpcb *
	in_pcblookup_connect(struct inpcbtable *,
	    struct in_addr, u_int, struct in_addr, u_int);
int	in_pcbnotify(struct inpcbtable *, struct in_addr, u_int,
	    struct in_addr, u_int, int, void (*)(struct inpcb *, int));
void	in_pcbnotifyall(struct inpcbtable *, struct in_addr, int,
	    void (*)(struct inpcb *, int));
void	in_pcbpurgeif0(struct inpcbtable *, struct ifnet *);
void	in_pcbpurgeif(struct inpcbtable *, struct ifnet *);
void	in_pcbstate(struct inpcb *, int);
void	in_rtchange(struct inpcb *, int);
void	in_setpeeraddr(struct inpcb *, struct mbuf *);
void	in_setsockaddr(struct inpcb *, struct mbuf *);
struct rtentry *
	in_pcbrtentry(struct inpcb *);
extern struct sockaddr_in *in_selectsrc(struct sockaddr_in *,
	struct route *, int, struct ip_moptions *, int *);
#endif

#endif /* _NETINET_IN_PCB_H_ */
