/*	$NetBSD: ns_var.h,v 1.12 2001/06/19 07:37:17 jdolecek Exp $	*/

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
extern	u_char nsctlerrmap[];

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
int ns_control __P((struct socket *, u_long, caddr_t, struct ifnet *,
		    struct proc *));
void ns_ifscrub __P((struct ifnet *, struct ns_ifaddr *));
int ns_ifinit __P((struct ifnet *, struct ns_ifaddr *, struct sockaddr_ns *,
		   int));
struct ns_ifaddr *ns_iaonnetof __P((struct ns_addr *));
void ns_purgeaddr __P((struct ifaddr *, struct ifnet *));
void ns_purgeif __P((struct ifnet *));

/* ns_cksum.c */
u_int16_t ns_cksum __P((struct mbuf *, int));

/* ns_error.c */
int ns_err_x __P((int));
void ns_error __P((struct mbuf *, int, int ));
void ns_printhost __P((struct ns_addr *));
void ns_err_input __P((struct mbuf *));
u_int32_t nstime __P((void));
int ns_echo __P((struct mbuf *));

/* ns_input.c */
void ns_init __P((void));
void nsintr __P((void));
void *idp_ctlinput __P((int, struct sockaddr *, void *));
void idp_forward __P((struct mbuf *));
int idp_do_route __P((struct ns_addr *, struct route *));
void idp_undo_route __P((struct route *));
void ns_watch_output __P((struct mbuf *, struct ifnet *));

/* ns_ip.c */
struct ifnet_en *nsipattach __P((void));
int nsipioctl __P((struct ifnet *, u_long, caddr_t));
void idpip_input __P((struct mbuf *, ...));
void nsipstart __P((struct ifnet *));
int nsip_route __P((struct mbuf *));
int nsip_free __P((struct ifnet *));
void *nsip_ctlinput __P((int, struct sockaddr *, void *));

/* ns_output.c */
int ns_output __P((struct mbuf *, ...));

/* ns_pcb.c */
int ns_pcballoc __P((struct socket *, struct nspcb *));
int ns_pcbbind __P((struct nspcb *, struct mbuf *, struct proc *));
int ns_pcbconnect __P((struct nspcb *, struct mbuf *));
void ns_pcbdisconnect __P((struct nspcb *));
void ns_pcbdetach __P((struct nspcb *));
void ns_setsockaddr __P((struct nspcb *, struct mbuf *));
void ns_setpeeraddr __P((struct nspcb *, struct mbuf *));
void ns_pcbnotify __P((struct ns_addr *, int, void (*)(struct nspcb *), long));
void ns_rtchange __P((struct nspcb *));
struct nspcb *ns_pcblookup __P((const struct ns_addr *, u_short, int));

#endif
