/*	$NetBSD: ns_var.h,v 1.13 2004/04/19 00:10:48 matt Exp $	*/

/*-
 * Copyright (c) 1995 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef _KERNEL
extern	long ns_pexseq;
extern	const u_char nsctlerrmap[];

struct socket;
struct nspcb;
struct ifaddr;
struct ifnet;
struct ns_ifaddr;
struct sockaddr_ns;
struct mbuf;
struct ns_addr;
struct route;
struct ifnet_en;
struct in_addr;
struct sockaddr;

/* ns.c */
int ns_control (struct socket *, u_long, caddr_t, struct ifnet *,
		    struct proc *);
void ns_ifscrub (struct ifnet *, struct ns_ifaddr *);
int ns_ifinit (struct ifnet *, struct ns_ifaddr *, struct sockaddr_ns *,
		   int);
struct ns_ifaddr *ns_iaonnetof (struct ns_addr *);
void ns_purgeaddr (struct ifaddr *, struct ifnet *);
void ns_purgeif (struct ifnet *);

/* ns_cksum.c */
u_int16_t ns_cksum (struct mbuf *, int);

/* ns_error.c */
int ns_err_x (int);
void ns_error (struct mbuf *, int, int );
void ns_printhost (struct ns_addr *);
void ns_err_input (struct mbuf *);
u_int32_t nstime (void);
int ns_echo (struct mbuf *);

/* ns_input.c */
void ns_init (void);
void nsintr (void);
void *idp_ctlinput (int, struct sockaddr *, void *);
void idp_forward (struct mbuf *);
int idp_do_route (struct ns_addr *, struct route *);
void idp_undo_route (struct route *);
void ns_watch_output (struct mbuf *, struct ifnet *);

/* ns_ip.c */
struct ifnet_en *nsipattach (void);
int nsipioctl (struct ifnet *, u_long, caddr_t);
void idpip_input (struct mbuf *, ...);
void nsipstart (struct ifnet *);
int nsip_route (struct mbuf *);
int nsip_free (struct ifnet *);
void *nsip_ctlinput (int, struct sockaddr *, void *);

/* ns_output.c */
int ns_output (struct mbuf *, ...);

/* ns_pcb.c */
int ns_pcballoc (struct socket *, struct nspcb *);
int ns_pcbbind (struct nspcb *, struct mbuf *, struct proc *);
int ns_pcbconnect (struct nspcb *, struct mbuf *);
void ns_pcbdisconnect (struct nspcb *);
void ns_pcbdetach (struct nspcb *);
void ns_setsockaddr (struct nspcb *, struct mbuf *);
void ns_setpeeraddr (struct nspcb *, struct mbuf *);
void ns_pcbnotify (struct ns_addr *, int, void (*)(struct nspcb *), long);
void ns_rtchange (struct nspcb *);
struct nspcb *ns_pcblookup (const struct ns_addr *, u_short, int);

#endif
