/*	$NetBSD: krpc_subr.c,v 1.4.2.2 1994/08/12 06:41:58 mycroft Exp $	*/

/*
 * Copyright (c) 1994 Gordon Ross, Adam Glass 
 * Copyright (c) 1992 Regents of the University of California.
 * All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
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
 *	California, Lawrence Berkeley Laboratory and its contributors.
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
 * partially based on:
 *      libnetboot/rpc.c
 *               @(#) Header: rpc.c,v 1.12 93/09/28 08:31:56 leres Exp  (LBL)
 */

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/systm.h>
#include <sys/reboot.h>

#include <net/if.h>
#include <netinet/in.h>

#include <nfs/rpcv2.h>

/*
 * Kernel support for Sun RPC
 *
 * Used currently for bootstrapping in nfs diskless configurations.
 * 
 * Note: will not work on variable-sized rpc args/results.
 *       implicit size-limit of an mbuf.
 */
 
#define	PMAPPORT		111
#define	PMAPPROG		100000
#define	PMAPVERS		2
#define	PMAPPROC_GETPORT	3

/*
 * Generic RPC headers
 */

struct auth_info {
	int	rp_atype;		/* auth type */
	u_long	rp_alen;		/* auth length */
};

struct rpc_call {
	u_long	rp_xid;			/* request transaction id */
	int 	rp_direction;	        /* call direction (0) */
	u_long	rp_rpcvers;		/* rpc version (2) */
	u_long	rp_prog;		/* program */
	u_long	rp_vers;		/* version */
	u_long	rp_proc;		/* procedure */
	struct	auth_info rp_auth;
	struct	auth_info rp_verf;
};

struct rpc_reply {
	u_long	rp_xid;			/* request transaction id */
	int	rp_direction;		/* call direction (1) */
	int	rp_astatus;		/* accept status (0: accepted) */
	union {
		u_long	rpu_errno;
		struct {
			struct auth_info rp_auth;
			u_long	rp_rstatus;		/* reply status */
		} rpu_ok;
	} rp_u;
};

#define MIN_REPLY_HDR 16	/* xid, dir, astat, errno */

/*
 * What is the longest we will wait before re-sending a request?
 * Note this is also the frequency of "RPC timeout" messages.
 * The re-send loop count sup linearly to this maximum, so the
 * first complaint will happen after (1+2+3+4+5)=15 seconds.
 */
#define	MAX_RESEND_DELAY 5	/* seconds */

/*
 * Call portmap to lookup a port number for a particular rpc program
 * Returns non-zero error on failure.
 */
int
krpc_portmap(sa,  prog, vers, portp)
	struct sockaddr *sa;		/* server address */
	u_long prog, vers;	/* host order */
	u_short *portp;		/* network order */
{
	struct sdata {
		u_long	prog;		/* call program */
		u_long	vers;		/* call version */
		u_long	proto;		/* call protocol */
		u_long	port;		/* call port (unused) */
	} *sdata;
	struct rdata {
		u_short pad;
		u_short port;
	} *rdata;
	struct mbuf *m;
	int error;

	/* The portmapper port is fixed. */
	if (prog == PMAPPROG) {
		*portp = htons(PMAPPORT);
		return 0;
	}

	m = m_gethdr(M_WAIT, MT_DATA);
	if (m == NULL)
		return ENOBUFS;
	m->m_len = sizeof(*sdata);
	m->m_pkthdr.len = m->m_len;
	sdata = mtod(m, struct sdata *);

	/* Do the RPC to get it. */
	sdata->prog = htonl(prog);
	sdata->vers = htonl(vers);
	sdata->proto = htonl(IPPROTO_UDP);
	sdata->port = 0;

	error = krpc_call(sa, PMAPPROG, PMAPVERS,
					  PMAPPROC_GETPORT, &m);
	if (error) 
		return error;

	rdata = mtod(m, struct rdata *);
	*portp = rdata->port;

	m_freem(m);
	return 0;
}

/*
 * Do a remote procedure call (RPC) and wait for its reply.
 */
int
krpc_call(sa, prog, vers, func, data)
	struct sockaddr *sa;
	u_long prog, vers, func;
	struct mbuf **data;	/* input/output */
{
	struct socket *so;
	struct sockaddr_in *sin;
	struct timeval *tv;
	struct mbuf *m, *nam, *mhead;
	struct rpc_call *call;
	struct rpc_reply *reply;
	struct uio auio;
	int error, rcvflg, timo, secs, len;
	static u_long xid = ~0xFF;
	u_short tport;

	/*
	 * Validate address family.
	 * Sorry, this is INET specific...
	 */
	if (sa->sa_family != AF_INET)
		return (EAFNOSUPPORT);

	/* Free at end if not null. */
	nam = mhead = NULL;

	/*
	 * Create socket and set its recieve timeout.
	 */
	if ((error = socreate(AF_INET, &so, SOCK_DGRAM, 0)))
		goto out;

	m = m_get(M_WAIT, MT_SOOPTS);
	if (m == NULL) {
		error = ENOBUFS;
		goto out;
	}
	tv = mtod(m, struct timeval *);
	m->m_len = sizeof(*tv);
	tv->tv_sec = 1;
	tv->tv_usec = 0;
	if ((error = sosetopt(so, SOL_SOCKET, SO_RCVTIMEO, m)))
		goto out;

	/*
	 * Bind the local endpoint to a reserved port,
	 * because some NFS servers refuse requests from
	 * non-reserved (non-privileged) ports.
	 */
	m = m_getclr(M_WAIT, MT_SONAME);
	sin = mtod(m, struct sockaddr_in *);
	sin->sin_len = m->m_len = sizeof(*sin);
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = INADDR_ANY;
	tport = IPPORT_RESERVED;
	do {
		tport--;
		sin->sin_port = htons(tport);
		error = sobind(so, m);
	} while (error == EADDRINUSE &&
			 tport > IPPORT_RESERVED / 2);
	m_freem(m);
	if (error) {
		printf("bind failed\n");
		goto out;
	}

	/*
	 * Setup socket address for the server.
	 */
	nam = m_get(M_WAIT, MT_SONAME);
	if (nam == NULL) {
		error = ENOBUFS;
		goto out;
	}
	sin = mtod(nam, struct sockaddr_in *);
	bcopy((caddr_t)sa, (caddr_t)sin, (nam->m_len = sa->sa_len));

	/*
	 * Set the port number that the request will use.
	 */
	if ((error = krpc_portmap(sa, prog, vers, &sin->sin_port)))
		goto out;

	/*
	 * Prepend RPC message header.
	 */
	m = *data;
	*data = NULL;
#ifdef	DIAGNOSTIC
	if ((m->m_flags & M_PKTHDR) == 0)
		panic("krpc_call: send data w/o pkthdr");
	if (m->m_pkthdr.len < m->m_len)
		panic("krpc_call: pkthdr.len not set");
#endif
	mhead = m_prepend(m, sizeof(*call), M_WAIT);
	if (mhead == NULL) {
		error = ENOBUFS;
		goto out;
	}
	mhead->m_pkthdr.len += sizeof(*call);
	mhead->m_pkthdr.rcvif = NULL;

	/*
	 * Fill in the RPC header
	 */
	call = mtod(mhead, struct rpc_call *);
	bzero((caddr_t)call, sizeof(*call));
	call->rp_xid = ++xid;	/* no need to put in network order */
	/* call->rp_direction = 0; */
	call->rp_rpcvers = htonl(2);
	call->rp_prog = htonl(prog);
	call->rp_vers = htonl(vers);
	call->rp_proc = htonl(func);
	/* call->rp_auth = 0; */
	/* call->rp_verf = 0; */

	/*
	 * Send it, repeatedly, until a reply is received,
	 * but delay each re-send by an increasing amount.
	 * If the delay hits the maximum, start complaining.
	 */
	timo = 0;
	for (;;) {
		/* Send RPC request (or re-send). */
		m = m_copym(mhead, 0, M_COPYALL, M_WAIT);
		if (m == NULL) {
			error = ENOBUFS;
			goto out;
		}
		error = sosend(so, nam, NULL, m, NULL, 0);
		if (error) {
			printf("krpc_call: sosend: %d\n", error);
			goto out;
		}
		m = NULL;

		/* Determine new timeout. */
		if (timo < MAX_RESEND_DELAY)
			timo++;
		else
			printf("RPC timeout for server 0x%x\n",
			       ntohl(sin->sin_addr.s_addr));

		/*
		 * Wait for up to timo seconds for a reply.
		 * The socket receive timeout was set to 1 second.
		 */
		secs = timo;
		while (secs > 0) {
			auio.uio_resid = len = 1<<16;
			rcvflg = 0;
			error = soreceive(so, NULL, &auio, &m, NULL, &rcvflg);
			if (error == EWOULDBLOCK) {
				secs--;
				continue;
			}
			if (error)
				goto out;
			len -= auio.uio_resid;

			/* Is the reply complete and the right one? */
			if (len < MIN_REPLY_HDR) {
				m_freem(m);
				continue;
			}
			if (m->m_len < MIN_REPLY_HDR) {
				m = m_pullup(m, MIN_REPLY_HDR);
				if (!m)
					continue;
			}
			reply = mtod(m, struct rpc_reply *);
			if ((reply->rp_direction == htonl(RPC_REPLY)) &&
				(reply->rp_xid == xid))
				goto gotreply;	/* break two levels */
		} /* while secs */
	} /* forever send/receive */

	error = ETIMEDOUT;
	goto out;

 gotreply:

	/*
	 * Pull as much as we can into first mbuf, to make
	 * result buffer contiguous.  Note that if the entire
	 * result won't fit into one mbuf, you're out of luck.
	 * XXX - Should not rely on making the entire reply
	 * contiguous (fix callers instead). -gwr
	 */
#ifdef	DIAGNOSTIC
	if ((m->m_flags & M_PKTHDR) == 0)
		panic("krpc_call: received pkt w/o header?");
#endif
	len = m->m_pkthdr.len;
	if (m->m_len < len) {
		m = m_pullup(m, len);
		if (m == NULL) {
			error = ENOBUFS;
			goto out;
		}
	}
	reply = mtod(m, struct rpc_reply *);

	/*
	 * Check RPC acceptance and status.
	 */
	if (reply->rp_astatus != 0) {
		error = ntohl(reply->rp_u.rpu_errno);
		printf("rpc denied, error=%d\n", error);
		m_freem(m);
		goto out;
	}
	if ((error = ntohl(reply->rp_u.rpu_ok.rp_rstatus)) != 0) {
		printf("rpc status=%d\n", error);
		m_freem(m);
		goto out;
	}

	/*
	 * Strip RPC header
	 */
	len = sizeof(*reply);
	if (reply->rp_u.rpu_ok.rp_auth.rp_atype != 0) {
		len += ntohl(reply->rp_u.rpu_ok.rp_auth.rp_alen);
		len = (len + 3) & ~3; /* XXX? */
	}
	m_adj(m, len);

	/* result */
	*data = m;

 out:
	if (nam) m_freem(nam);
	if (mhead) m_freem(mhead);
	soclose(so);
	return error;
}
