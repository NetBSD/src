/*
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
 *   $Id: rpc.h,v 1.1 1994/05/08 16:11:36 brezak Exp $
 */

/* XXX defines we can't easily get from system includes */
#define	PMAPPORT		111
#define	PMAPPROG		100000
#define	PMAPVERS		2
#define	PMAPPROC_GETPORT	3

#define	RPC_MSG_VERSION		2
#define MSG_ACCEPTED		0
#define CALL			0
#define REPLY			1


/* Null rpc auth info */
struct auth_info {
	int	rp_atype;		/* zero (really AUTH_NULL) */
	u_long	rp_alen;		/* zero (size of auth struct) */
};

/* Generic rpc call header */
struct rpc_call {
	u_long	rp_xid;			/* request transaction id */
	int	rp_direction;		/* call direction */
	u_long	rp_rpcvers;		/* rpc version (2) */
	u_long	rp_prog;		/* program */
	u_long	rp_vers;		/* version */
	u_long	rp_proc;		/* procedure */
	struct	auth_info rp_auth;	/* AUTH_NULL */
	struct	auth_info rp_verf;	/* AUTH_NULL */
};

/* Generic rpc reply header */
struct rpc_reply {
	u_long	rp_xid;			/* request transaction id */
	int	rp_direction;		/* call direction */
	int	rp_stat;		/* accept status */
	u_long	rp_prog;		/* program (unused) */
	u_long	rp_vers;		/* version (unused) */
	u_long	rp_proc;		/* procedure (unused) */
};

/* RPC functions: */
int	callrpc __P((struct iodesc *d, u_long prog, u_long ver, u_long op,
	    void *sdata, int slen, void *rdata, int rlen));
u_short	getport __P((struct iodesc *d, u_long prog, u_long vers));

