/*	$NetBSD: can_pcb.h,v 1.2.10.2 2017/12/03 11:39:03 jdolecek Exp $	*/

/*-
 * Copyright (c) 2003, 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Robert Swindells and Manuel Bouyer
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#ifndef _NETCAN_CAN_PCB_H_
#define _NETCAN_CAN_PCB_H_

#include <sys/queue.h>

/*
 * Common structure pcb for can protocol implementation.
 * Here are stored pointers to local and foreign host table
 * entries, local and foreign socket numbers, and pointers
 * up (to a socket structure) and down (to a protocol-specific)
 * control block.
 */
struct canpcbpolicy;


struct canpcb {
	LIST_ENTRY(canpcb) canp_hash;
	LIST_ENTRY(canpcb) canp_lhash;
	TAILQ_ENTRY(canpcb) canp_queue;
	kmutex_t 	canp_mtx;	/* protect states and refcount */
	int		canp_state;
	int		canp_flags;
	struct		socket *canp_socket;	/* back pointer to socket */
	struct		ifnet *canp_ifp; /* interface this socket is bound to */

	struct		canpcbtable *canp_table;
	struct		can_filter *canp_filters; /* filter array */
	int		canp_nfilters; /* size of canp_filters */

	int		canp_refcount;
};

LIST_HEAD(canpcbhead, canpcb);

TAILQ_HEAD(canpcbqueue, canpcb);

struct canpcbtable {
	struct	canpcbqueue canpt_queue;
	struct	canpcbhead *canpt_bindhashtbl;
	struct	canpcbhead *canpt_connecthashtbl;
	u_long	canpt_bindhash;
	u_long	canpt_connecthash;
};

/* states in canp_state: */
#define	CANP_DETACHED		0
#define	CANP_ATTACHED		1
#define	CANP_BOUND		2
#define	CANP_CONNECTED		3

/* flags in canp_flags: */
#define CANP_NO_LOOPBACK	0x0001 /* local loopback disabled */
#define CANP_RECEIVE_OWN	0x0002 /* receive own message */


#define	sotocanpcb(so)		((struct canpcb *)(so)->so_pcb)

#ifdef _KERNEL
void	can_losing(struct canpcb *);
int	can_pcballoc (struct socket *, void *);
int	can_pcbbind(void *, struct sockaddr_can *, struct lwp *);
int	can_pcbconnect(void *, struct sockaddr_can *);
void	can_pcbdetach(void *);
void	can_pcbdisconnect(void *);
void	can_pcbinit(struct canpcbtable *, int, int);
int	can_pcbnotify(struct canpcbtable *, u_int32_t,
	    u_int32_t, int, void (*)(struct canpcb *, int));
void	can_pcbnotifyall(struct canpcbtable *, u_int32_t, int,
	    void (*)(struct canpcb *, int));
void	can_pcbpurgeif0(struct canpcbtable *, struct ifnet *);
void	can_pcbpurgeif(struct canpcbtable *, struct ifnet *);
void	can_pcbstate(struct canpcb *, int);
void	can_setsockaddr(struct canpcb *, struct sockaddr_can *);
int	can_pcbsetfilter(struct canpcb *, struct can_filter *, int);
bool	can_pcbfilter(struct canpcb *, struct mbuf *);

/* refcount management */
void	canp_ref(struct canpcb *);
void	canp_unref(struct canpcb *);
#endif

#endif /* _NETCAN_CAN_PCB_H_ */
