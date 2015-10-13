/*	$KAME: sctp6_var.h,v 1.7 2004/08/17 04:06:22 itojun Exp $	*/
/*	$NetBSD: sctp6_var.h,v 1.1 2015/10/13 21:28:35 rjs Exp $	*/

/*
 * Copyright (c) 2001, 2002, 2004 Cisco Systems, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Cisco Systems, Inc.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CISCO SYSTEMS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL CISCO SYSTEMS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef _NETINET6_SCTP6_VAR_H_
#define _NETINET6_SCTP6_VAR_H_

#if defined(_KERNEL)

extern const struct pr_usrreqs sctp6_usrreqs;

int	sctp6_ctloutput(int, struct socket *, int, int, struct mbuf **);
int	sctp6_usrreq(struct socket *, int, struct mbuf *, struct mbuf *,
			   struct mbuf *, struct lwp *);
int	sctp6_input(struct mbuf **, int *, int);
int	sctp6_output(struct sctp_inpcb *, struct mbuf *, struct sockaddr *,
	struct mbuf *, struct lwp *);
void *	sctp6_ctlinput(int, const struct sockaddr *, void *);

#endif /* _KERNEL */
#endif
