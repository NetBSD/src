/*	$NetBSD: keysock.h,v 1.14.16.1 2009/05/13 17:22:50 jym Exp $	*/
/*	$KAME: keysock.h,v 1.8 2000/03/27 05:11:06 sumikawa Exp $	*/

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

#ifndef _NETKEY_KEYSOCK_H_
#define _NETKEY_KEYSOCK_H_

/* statistics for pfkey socket */
#define	PFKEY_STAT_OUT_TOTAL	0	/* # of total calls */
#define	PFKEY_STAT_OUT_BYTES	1	/* total bytecount */
#define	PFKEY_STAT_OUT_MSGTYPE	2	/* message type histogram */
		/* space for 256 counters */
#define	PFKEY_STAT_OUT_INVLEN	258	/* invalid length field */
#define	PFKEY_STAT_OUT_INVVER	259	/* invalid version field */
#define	PFKEY_STAT_OUT_INVMSGTYPE 260	/* invalid message type field */
#define	PFKEY_STAT_OUT_TOOSHORT	261	/* message too short */
#define	PFKEY_STAT_OUT_NOMEM	262	/* memory allocation failure */
#define	PFKEY_STAT_OUT_DUPEXT	263	/* duplicate extension */
#define	PFKEY_STAT_OUT_INVEXTTYPE 264	/* invalid extension type */
#define	PFKEY_STAT_OUT_INVSATYPE 265	/* invalid sa type */
#define	PFKEY_STAT_OUT_INVADDR	266	/* invalid address extension */
#define	PFKEY_STAT_IN_TOTAL	267	/* # of total calls */
#define	PFKEY_STAT_IN_BYTES	268	/* total bytecount */
#define	PFKEY_STAT_IN_MSGTYPE	269	/* message type histogram */
		/* space for 256 counters */
#define	PFKEY_STAT_IN_MSGTARGET	525	/* one/all/registered */
		/* space for 3 counters */
#define	PFKEY_STAT_IN_NOMEM	528	/* memory alloation failure */
#define	PFKEY_STAT_SOCKERR	529	/* # of socket related errors */

#define	PFKEY_NSTATS		530

#define KEY_SENDUP_ONE		0
#define KEY_SENDUP_ALL		1
#define KEY_SENDUP_REGISTERED	2
#define KEY_SENDUP_CANWAIT	4

#ifdef _KERNEL
struct keycb {
	struct rawcb kp_raw;	/* rawcb */
	int kp_promisc;		/* promiscuous mode */
	int kp_registered;	/* registered socket */
	int (*kp_receive) (struct socket *, struct mbuf **, struct uio *,
		struct mbuf **, struct mbuf **, int *);
	struct mbuf *kp_queue;	/* queued mbufs, linked by m_nextpkt */
};

extern int key_output(struct mbuf *, ...);
#ifndef __NetBSD__
extern int key_usrreq(struct socket *,
	int, struct mbuf *, struct mbuf *, struct mbuf *);
#else
extern int key_usrreq(struct socket *,
	int, struct mbuf *, struct mbuf *, struct mbuf *, struct lwp *);
#endif

extern int key_sendup_mbuf(struct socket *, struct mbuf *, int);
#endif /* _KERNEL */

#endif /* !_NETKEY_KEYSOCK_H_ */
