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
 * Copyright (c) 1982, 1986, 1989, 1993
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)udp_var.h	8.1 (Berkeley) 6/10/93
 */

#ifndef _NETINET6_UDP6_VAR_H_
#define _NETINET6_UDP6_VAR_H_

#if defined(__FreeBSD__) || defined(__NetBSD__)
/*
 * UDP Kernel structures and variables.
 */
struct	udp6stat {
				/* input statistics: */
	u_long	udp6s_ipackets;		/* total input packets */
	u_long	udp6s_hdrops;		/* packet shorter than header */
	u_long	udp6s_badsum;		/* checksum error */
	u_long	udp6s_nosum;		/* no checksum */
	u_long	udp6s_badlen;		/* data length larger than packet */
	u_long	udp6s_noport;		/* no socket on port */
	u_long	udp6s_noportmcast;	/* of above, arrived as broadcast */
	u_long	udp6s_fullsock;		/* not delivered, input socket full */
	u_long	udp6ps_pcbcachemiss;	/* input packets missing pcb cache */
				/* output statistics: */
	u_long	udp6s_opackets;		/* total output packets */
};

/*
 * Names for UDP sysctl objects
 */
#define UDP6CTL_SENDMAX		1	/* default send buffer */
#define UDP6CTL_RECVSPACE	2	/* default recv buffer */
#define UDP6CTL_MAXID		3

#define UDP6CTL_NAMES { \
	{ 0, 0 }, \
	{ "sendmax", CTLTYPE_INT }, \
	{ "recvspace", CTLTYPE_INT }, \
}

#define UDP6CTL_VARS { \
	0, \
	&udp6_sendspace, \
	&udp6_recvspace, \
}
#endif /*__FreeBSD__||__NetBSD__*/
#ifdef __bsdi__
/*
 * UDP Kernel structures and variables.
 */
struct	udp6stat {
				/* input statistics: */
	u_quad_t udp6s_ipackets;	/* total input packets */
	u_quad_t udp6s_hdrops;		/* packet shorter than header */
	u_quad_t udp6s_badsum;		/* checksum error */
	u_quad_t udp6s_nosum;		/* no checksum */
	u_quad_t udp6s_badlen;		/* data length larger than packet */
	u_quad_t udp6s_noport;		/* no socket on port */
	u_quad_t udp6s_noportmcast;	/* of above, arrived as broadcast */
	u_quad_t udp6s_fullsock;	/* not delivered, input socket full */
	u_quad_t udp6ps_pcbcachemiss;	/* input packets missing pcb cache */
				/* output statistics: */
	u_quad_t udp6s_opackets;		/* total output packets */
};

/*
 * Names for UDP sysctl objects
 */
#define UDP6CTL_SENDMAX		2	/* default send buffer */
#define UDP6CTL_RECVSPACE	3	/* default recv buffer */
#define UDP6CTL_STATS		4	/* statistics */
#define UDP6CTL_MAXID		5

#define UDP6CTL_NAMES { \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ "sendmax", CTLTYPE_INT }, \
	{ "recvspace", CTLTYPE_INT }, \
	{ 0, 0 }, \
}

#define UDP6CTL_VARS { \
	0, \
	0, \
	&udp6_sendspace, \
	&udp6_recvspace, \
	0, \
}
#endif /*__bsdi__*/

#ifdef _KERNEL
struct	in6pcb udb6;
struct	udp6stat udp6stat;

void	udp6_ctlinput __P((int, struct sockaddr *, struct ip6_hdr *,
			   struct mbuf *, int));
void	udp6_init __P((void));
int	udp6_input __P((struct mbuf **, int *, int));
int	udp6_output __P((struct in6pcb *,
			 struct mbuf *, struct mbuf *, struct mbuf *));
int	udp6_sysctl __P((int *, u_int, void *, size_t *, void *, size_t));
int	udp6_usrreq __P((struct socket *,
			 int, struct mbuf *, struct mbuf *, struct mbuf *, struct proc *));
#endif /* _KERNEL */

#endif /*_NETINET6_UDP6_VAR_H_*/
