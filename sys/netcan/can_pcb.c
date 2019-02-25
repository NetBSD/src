/*	$NetBSD: can_pcb.c,v 1.7 2019/02/25 06:49:44 maxv Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: can_pcb.c,v 1.7 2019/02/25 06:49:44 maxv Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/kmem.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/pool.h>
#include <sys/proc.h>

#include <net/if.h>
#include <net/route.h>

#include <netcan/can.h>
#include <netcan/can_var.h>
#include <netcan/can_pcb.h>

#define	CANPCBHASH_BIND(table, ifindex) \
	&(table)->canpt_bindhashtbl[ \
	    (ifindex) & (table)->canpt_bindhash]
#define	CANPCBHASH_CONNECT(table, ifindex) \
	&(table)->canpt_connecthashtbl[ \
	    (ifindex) & (table)->canpt_bindhash]

struct pool canpcb_pool;

void
can_pcbinit(struct canpcbtable *table, int bindhashsize, int connecthashsize)
{
	static int canpcb_pool_initialized;

	if (canpcb_pool_initialized == 0) {
		pool_init(&canpcb_pool, sizeof(struct canpcb), 0, 0, 0,
		    "canpcbpl", NULL, IPL_SOFTNET);
		canpcb_pool_initialized = 1;
	}

	TAILQ_INIT(&table->canpt_queue);
	table->canpt_bindhashtbl = hashinit(bindhashsize, HASH_LIST, true,
	    &table->canpt_bindhash);
	table->canpt_connecthashtbl = hashinit(connecthashsize, HASH_LIST,
	    true, &table->canpt_connecthash);
}

int
can_pcballoc(struct socket *so, void *v)
{
	struct canpcbtable *table = v;
	struct canpcb *canp;
	struct can_filter *can_init_filter;
	int s;

	can_init_filter = kmem_alloc(sizeof(struct can_filter), KM_NOSLEEP);
	if (can_init_filter == NULL)
		return (ENOBUFS);
	can_init_filter->can_id = 0;
	can_init_filter->can_mask = 0; /* accept all by default */

	s = splnet();
	canp = pool_get(&canpcb_pool, PR_NOWAIT);
	splx(s);
	if (canp == NULL) {
		kmem_free(can_init_filter, sizeof(struct can_filter));
		return (ENOBUFS);
	}
	memset(canp, 0, sizeof(*canp));
	canp->canp_table = table;
	canp->canp_socket = so;
	canp->canp_filters = can_init_filter;
	canp->canp_nfilters = 1;
	mutex_init(&canp->canp_mtx, MUTEX_DEFAULT, IPL_NET);
	canp->canp_refcount = 1;

	so->so_pcb = canp;
	mutex_enter(&canp->canp_mtx);
	TAILQ_INSERT_HEAD(&table->canpt_queue, canp, canp_queue);
	can_pcbstate(canp, CANP_ATTACHED);
	mutex_exit(&canp->canp_mtx);
	return (0);
}

int
can_pcbbind(void *v, struct sockaddr_can *scan, struct lwp *l)
{
	struct canpcb *canp = v;

	if (scan->can_family != AF_CAN)
		return (EAFNOSUPPORT);
	if (scan->can_len != sizeof(*scan))
		return EINVAL;
	mutex_enter(&canp->canp_mtx);
	if (scan->can_ifindex != 0) {
		canp->canp_ifp = if_byindex(scan->can_ifindex);
		if (canp->canp_ifp == NULL ||
		    canp->canp_ifp->if_dlt != DLT_CAN_SOCKETCAN) {
			canp->canp_ifp = NULL;
			mutex_exit(&canp->canp_mtx);
			return (EADDRNOTAVAIL);
		}
		soisconnected(canp->canp_socket);
	} else {
		canp->canp_ifp = NULL;
		canp->canp_socket->so_state &= ~SS_ISCONNECTED;	/* XXX */
	}
	can_pcbstate(canp, CANP_BOUND);
	mutex_exit(&canp->canp_mtx);
	return 0;
}

/*
 * Connect from a socket to a specified address.
 */
int
can_pcbconnect(void *v, struct sockaddr_can *scan)
{
#if 0
	struct canpcb *canp = v;
	struct sockaddr_can *ifaddr = NULL;
	int error;
#endif

	if (scan->can_family != AF_CAN)
		return (EAFNOSUPPORT);
	if (scan->can_len != sizeof(*scan))
		return EINVAL;
#if 0
	mutex_enter(&canp->canp_mtx);
	memcpy(&canp->canp_dst, scan, sizeof(struct sockaddr_can));
	can_pcbstate(canp, CANP_CONNECTED);
	mutex_exit(&canp->canp_mtx);
	return 0;
#endif
	return EOPNOTSUPP;
}

void
can_pcbdisconnect(void *v)
{
	struct canpcb *canp = v;

	mutex_enter(&canp->canp_mtx);
	can_pcbstate(canp, CANP_DETACHED);
	mutex_exit(&canp->canp_mtx);
	if (canp->canp_socket->so_state & SS_NOFDREF)
		can_pcbdetach(canp);
}

void
can_pcbdetach(void *v)
{
	struct canpcb *canp = v;
	struct socket *so = canp->canp_socket;

	KASSERT(mutex_owned(softnet_lock));
	so->so_pcb = NULL;
	mutex_enter(&canp->canp_mtx);
	can_pcbstate(canp, CANP_DETACHED);
	can_pcbsetfilter(canp, NULL, 0);
	mutex_exit(&canp->canp_mtx);
	TAILQ_REMOVE(&canp->canp_table->canpt_queue, canp, canp_queue);
	sofree(so); /* sofree drops the softnet_lock */
	canp_unref(canp);
	mutex_enter(softnet_lock);
}

void
canp_ref(struct canpcb *canp)
{
	KASSERT(mutex_owned(&canp->canp_mtx));
	canp->canp_refcount++;
}

void
canp_unref(struct canpcb *canp)
{
	mutex_enter(&canp->canp_mtx);
	canp->canp_refcount--;
	KASSERT(canp->canp_refcount >= 0);
	if (canp->canp_refcount > 0) {
		mutex_exit(&canp->canp_mtx);
		return;
	}
	mutex_exit(&canp->canp_mtx);
	mutex_destroy(&canp->canp_mtx);
	pool_put(&canpcb_pool, canp);
}

void
can_setsockaddr(struct canpcb *canp, struct sockaddr_can *scan)
{

	mutex_enter(&canp->canp_mtx);
	memset(scan, 0, sizeof (*scan));
	scan->can_family = AF_CAN;
	scan->can_len = sizeof(*scan);
	if (canp->canp_ifp) 
		scan->can_ifindex = canp->canp_ifp->if_index;
	else
		scan->can_ifindex = 0;
	mutex_exit(&canp->canp_mtx);
}

int
can_pcbsetfilter(struct canpcb *canp, struct can_filter *fp, int nfilters)
{

	struct can_filter *newf;
	KASSERT(mutex_owned(&canp->canp_mtx));

	if (nfilters > 0) {
		newf =
		    kmem_alloc(sizeof(struct can_filter) * nfilters, KM_SLEEP);
		memcpy(newf, fp, sizeof(struct can_filter) * nfilters);
	} else {
		newf = NULL;
	}
	if (canp->canp_filters != NULL) {
		kmem_free(canp->canp_filters,
		    sizeof(struct can_filter) * canp->canp_nfilters);
	}
	canp->canp_filters = newf;
	canp->canp_nfilters = nfilters;
	return 0;
}



#if 0
/*
 * Pass some notification to all connections of a protocol
 * associated with address dst.  The local address and/or port numbers
 * may be specified to limit the search.  The "usual action" will be
 * taken, depending on the ctlinput cmd.  The caller must filter any
 * cmds that are uninteresting (e.g., no error in the map).
 * Call the protocol specific routine (if any) to report
 * any errors for each matching socket.
 *
 * Must be called at splsoftnet.
 */
int
can_pcbnotify(struct canpcbtable *table, u_int32_t faddr, u_int32_t laddr,
    int errno, void (*notify)(struct canpcb *, int))
{
	struct canpcbhead *head;
	struct canpcb *canp, *ncanp;
	int nmatch;

	if (faddr == 0 || notify == 0)
		return (0);

	nmatch = 0;
	head = CANPCBHASH_CONNECT(table, faddr, laddr);
	for (canp = LIST_FIRST(head); canp != NULL; canp = ncanp) {
		ncanp = LIST_NEXT(canp, canp_hash);
		if (canp->canp_faddr == faddr &&
		    canp->canp_laddr == laddr) {
			(*notify)(canp, errno);
			nmatch++;
		}
	}
	return (nmatch);
}

void
can_pcbnotifyall(struct canpcbtable *table, u_int32_t faddr, int errno,
    void (*notify)(struct canpcb *, int))
{
	struct canpcb *canp, *ncanp;

	if (faddr == 0 || notify == 0)
		return;

	TAILQ_FOREACH_SAFE(canp, &table->canpt_queue, canp_queue, ncanp) {
		if (canp->canp_faddr == faddr)
			(*notify)(canp, errno);
	}
}
#endif

#if 0
void
can_pcbpurgeif0(struct canpcbtable *table, struct ifnet *ifp)
{
	struct canpcb *canp, *ncanp;
	struct ip_moptions *imo;
	int i, gap;

}

void
can_pcbpurgeif(struct canpcbtable *table, struct ifnet *ifp)
{
	struct canpcb *canp, *ncanp;

	for (canp = CIRCLEQ_FIRST(&table->canpt_queue);
	    canp != (void *)&table->canpt_queue;
	    canp = ncanp) {
		ncanp = CIRCLEQ_NEXT(canp, canp_queue);
	}
}
#endif



void
can_pcbstate(struct canpcb *canp, int state)
{
	int ifindex = canp->canp_ifp ? canp->canp_ifp->if_index : 0;
	KASSERT(mutex_owned(&canp->canp_mtx));

	if (canp->canp_state > CANP_ATTACHED)
		LIST_REMOVE(canp, canp_hash);

	switch (state) {
	case CANP_BOUND:
		LIST_INSERT_HEAD(CANPCBHASH_BIND(canp->canp_table,
		    ifindex), canp, canp_hash);
		break;
	case CANP_CONNECTED:
		LIST_INSERT_HEAD(CANPCBHASH_CONNECT(canp->canp_table,
		    ifindex), canp, canp_hash);
		break;
	}

	canp->canp_state = state;
}

/*
 * check mbuf against socket accept filter.
 * returns true if mbuf is accepted, false otherwise
 */
bool
can_pcbfilter(struct canpcb *canp, struct mbuf *m)
{
	int i;
	struct can_frame *fmp;
	struct can_filter *fip;

	KASSERT(mutex_owned(&canp->canp_mtx));
	KASSERT((m->m_flags & M_PKTHDR) != 0);
	KASSERT(m->m_len == m->m_pkthdr.len);

	fmp = mtod(m, struct can_frame *);
	for (i = 0; i < canp->canp_nfilters; i++) {
		fip = &canp->canp_filters[i];
		if ((fmp->can_id & fip->can_mask) == fip->can_id)
			return true;
	}
	/* no match */
	return false;
}
