/*	$NetBSD: if_laggproto.h,v 1.17 2022/05/24 20:50:20 andvar Exp $	*/

/*
 * Copyright (c) 2021 Internet Initiative Japan Inc.
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

#ifndef _NET_LAGG_IF_LAGGPROTO_H_
#define _NET_LAGG_IF_LAGGPROTO_H_

struct lagg_softc;
struct lagg_proto_softc;

#define LAGG_MAX_PORTS	32
#define LAGG_PORT_PRIO	0x8000U

enum lagg_work_state {
	LAGG_WORK_IDLE,
	LAGG_WORK_ENQUEUED,
	LAGG_WORK_STOPPING
};
struct lagg_work {
	struct work	 lw_cookie;
	void		(*lw_func)(struct lagg_work *, void *);
	void		*lw_arg;
	int		 lw_state;
};

static inline void
lagg_work_set(struct lagg_work *w,
    void (*func)(struct lagg_work *, void *), void *arg)
{

	w->lw_func = func;
	w->lw_arg = arg;
}

struct workqueue *
		lagg_workq_create(const char *, pri_t, int, int);
void		lagg_workq_destroy(struct workqueue *);
void		lagg_workq_add(struct workqueue *, struct lagg_work *);
void		lagg_workq_wait(struct workqueue *, struct lagg_work *);

struct lagg_port {
	struct psref_target	 lp_psref;
	struct ifnet		*lp_ifp;	/* physical interface */
	struct lagg_softc	*lp_softc;	/* parent lagg */
	void			*lp_proto_ctx;
	bool			 lp_ifdetaching;
	bool			 lp_promisc;
	void			*lp_linkstate_hook;
	void			*lp_ifdetach_hook;

	uint32_t		 lp_prio;	/* port priority */
	uint32_t		 lp_flags;	/* port flags */

	u_char			 lp_iftype;
	uint8_t			 lp_lladdr[ETHER_ADDR_LEN];
	int			 lp_eccapenable;
	uint64_t		 lp_ifcapenable;
	uint64_t		 lp_mtu;

	int			(*lp_ioctl)(struct ifnet *, u_long, void *);
	void			(*lp_input)(struct ifnet *, struct mbuf *);
	int			(*lp_output)(struct ifnet *, struct mbuf *,
				    const struct sockaddr *,
				    const struct rtentry *);

	SIMPLEQ_ENTRY(lagg_port)
				 lp_entry;
};

struct lagg_proto {
	lagg_proto	 pr_num;
	void		(*pr_init)(void);
	void		(*pr_fini)(void);
	int		(*pr_attach)(struct lagg_softc *,
			    struct lagg_proto_softc **);
	void		(*pr_detach)(struct lagg_proto_softc *);
	int		(*pr_up)(struct lagg_proto_softc *);
	void		(*pr_down)(struct lagg_proto_softc *);
	int		(*pr_transmit)(struct lagg_proto_softc *,
			    struct mbuf *);
	struct mbuf *	(*pr_input)(struct lagg_proto_softc *,
			    struct lagg_port *, struct mbuf *);
	int		(*pr_allocport)(struct lagg_proto_softc *,
			    struct lagg_port *);
	void		(*pr_freeport)(struct lagg_proto_softc *,
			    struct lagg_port *);
	void		(*pr_startport)(struct lagg_proto_softc *,
			    struct lagg_port *);
	void		(*pr_stopport)(struct lagg_proto_softc *,
			    struct lagg_port *);
	void		(*pr_protostat)(struct lagg_proto_softc *,
			    struct laggreqproto *);
	void		(*pr_portstat)(struct lagg_proto_softc *,
			    struct lagg_port *, struct laggreqport *);
	void		(*pr_linkstate)(struct lagg_proto_softc *,
			    struct lagg_port *);
	int		(*pr_ioctl)(struct lagg_proto_softc *,
			    struct laggreqproto *);
};

struct lagg_variant {
	lagg_proto	 lv_proto;
	struct lagg_proto_softc
			*lv_psc;

	struct psref_target	lv_psref;
};

struct lagg_mc_entry {
	LIST_ENTRY(lagg_mc_entry)
				 mc_entry;
	struct ether_multi	*mc_enm;
	struct sockaddr_storage	 mc_addr;
};

struct lagg_vlantag {
	uint16_t	 lvt_vtag;
	TAILQ_ENTRY(lagg_vlantag)
			 lvt_entry;
};

struct lagg_softc {
	kmutex_t		 sc_lock;
	struct ifmedia		 sc_media;
	u_char			 sc_iftype;

	/* interface link-layer address */
	uint8_t			 sc_lladdr[ETHER_ADDR_LEN];
	/* generated random lladdr */
	uint8_t			 sc_lladdr_rand[ETHER_ADDR_LEN];

	LIST_HEAD(, lagg_mc_entry)
				 sc_mclist;
	TAILQ_HEAD(, lagg_vlantag)
				 sc_vtags;
	pserialize_t		 sc_psz;
	struct lagg_variant	*sc_var;
	SIMPLEQ_HEAD(, lagg_port)
				 sc_ports;
	size_t			 sc_nports;
	char			 sc_evgroup[16];
	struct evcnt		 sc_novar;

	struct sysctllog	*sc_sysctllog;
	const struct sysctlnode	*sc_sysctlnode;
	bool			 sc_hash_mac;
	bool			 sc_hash_ipaddr;
	bool			 sc_hash_ip6addr;
	bool			 sc_hash_tcp;
	bool			 sc_hash_udp;

	/*
	 * storage size of sc_if is a variable-length,
	 * should be the last
	 */
	struct ifnet		 sc_if;
};

/*
 * Locking notes:
 * - sc_lock(LAGG_LOCK()) is an adaptive mutex and protects items
 *   of struct lagg_softc
 * - a lock in struct lagg_proto_softc, for example LACP_LOCK(), is
 *   an adaptive mutex and protects member contained in the struct
 * - sc_var is protected by both pselialize (sc_psz) and psref (lv_psref)
 *    - Updates of sc_var is serialized by sc_lock
 * - Items in sc_ports is protected by both psref (lp_psref) and
 *   pserialize contained in struct lagg_proto_softc
 *   - details are described in if_laggport.c and if_lagg_lacp.c
 *   - Updates of items in sc_ports are serialized by sc_lock
 * - an instance referenced by lp_proto_ctx in struct lagg_port is
 *   protected by a lock in struct lagg_proto_softc
 *
 * Locking order:
 * - IFNET_LOCK(sc_if) -> LAGG_LOCK -> ETHER_LOCK(sc_if) -> a lock in
 *   struct lagg_port_softc
 * - IFNET_LOCK(sc_if) -> LAGG_LOCK -> IFNET_LOCK(lp_ifp)
 * - IFNET_LOCK(lp_ifp) -> a lock in struct lagg_proto_softc
 * - Currently, there is no combination of following locks
 *   - IFNET_LOCK(lp_ifp) and ETHER_LOCK(sc_if)
 */
#define LAGG_LOCK(_sc)		mutex_enter(&(_sc)->sc_lock)
#define LAGG_UNLOCK(_sc)	mutex_exit(&(_sc)->sc_lock)
#define LAGG_LOCKED(_sc)	mutex_owned(&(_sc)->sc_lock)
#define LAGG_CLLADDR(_sc)	CLLADDR((_sc)->sc_if.if_sadl)

#define LAGG_PORTS_FOREACH(_sc, _lp)	\
    SIMPLEQ_FOREACH((_lp), &(_sc)->sc_ports, lp_entry)
#define LAGG_PORTS_FIRST(_sc)	SIMPLEQ_FIRST(&(_sc)->sc_ports)
#define LAGG_PORTS_EMPTY(_sc)	SIMPLEQ_EMPTY(&(_sc)->sc_ports)
#define LAGG_PORT_IOCTL(_lp, _cmd, _data)	\
	(_lp)->lp_ioctl == NULL ? ENOTTY :	\
	(_lp)->lp_ioctl((_lp)->lp_ifp, (_cmd), (_data))

static inline const void *
lagg_m_extract(struct mbuf *m, size_t off, size_t reqlen, void *buf)
{
	ssize_t len;
	const void *rv;

	KASSERT(ISSET(m->m_flags, M_PKTHDR));
	len = off + reqlen;

	if (m->m_pkthdr.len < len) {
		return NULL;
	}

	if (m->m_len >= len) {
		rv = mtod(m, uint8_t *) + off;
	} else {
		m_copydata(m, off, reqlen, buf);
		rv = buf;
	}

	return rv;
}

static inline int
lagg_port_xmit(struct lagg_port *lp, struct mbuf *m)
{

	return if_transmit_lock(lp->lp_ifp, m);
}

static inline bool
lagg_portactive(struct lagg_port *lp)
{
	struct ifnet *ifp;

	ifp = lp->lp_ifp;

	if (ifp->if_link_state != LINK_STATE_DOWN &&
	    ISSET(ifp->if_flags, IFF_UP)) {
		return true;
	}

	return false;
}

static inline bool
lagg_debug_enable(struct lagg_softc *sc)
{
	if (__predict_false(ISSET(sc->sc_if.if_flags, IFF_DEBUG)))
		return true;

	return false;
}

#define LAGG_LOG(_sc, _lvl, _fmt, _arg...) do {		\
	if ((_lvl) == LOG_DEBUG && 			\
	    !lagg_debug_enable(_sc))			\
		break;					\
							\
	log((_lvl), "%s: ", (_sc)->sc_if.if_xname);	\
	addlog((_fmt), ##_arg);				\
} while(0)

void		lagg_port_getref(struct lagg_port *, struct psref *);
void		lagg_port_putref(struct lagg_port *, struct psref *);
void		lagg_output(struct lagg_softc *,
		    struct lagg_port *, struct mbuf *);
uint32_t	lagg_hashmbuf(struct lagg_softc *, struct mbuf *);

void		lagg_common_detach(struct lagg_proto_softc *);
int		lagg_common_allocport(struct lagg_proto_softc *,
		    struct lagg_port *);
void		lagg_common_freeport(struct lagg_proto_softc *,
		    struct lagg_port *);
void		lagg_common_startport(struct lagg_proto_softc *,
		    struct lagg_port *);
void		lagg_common_stopport(struct lagg_proto_softc *,
		    struct lagg_port *);
void		lagg_common_linkstate(struct lagg_proto_softc *,
		    struct lagg_port *);

int		lagg_none_attach(struct lagg_softc *,
		    struct lagg_proto_softc **);

int		lagg_fail_attach(struct lagg_softc *,
		    struct lagg_proto_softc **);
int		lagg_fail_transmit(struct lagg_proto_softc *, struct mbuf *);
struct mbuf *	lagg_fail_input(struct lagg_proto_softc *, struct lagg_port *,
		   struct mbuf *);
void		lagg_fail_portstat(struct lagg_proto_softc *,
		    struct lagg_port *, struct laggreqport *);
int		lagg_fail_ioctl(struct lagg_proto_softc *,
		    struct laggreqproto *);

int		lagg_lb_attach(struct lagg_softc *, struct lagg_proto_softc **);
void		lagg_lb_startport(struct lagg_proto_softc *,
		    struct lagg_port *);
void		lagg_lb_stopport(struct lagg_proto_softc *, struct lagg_port *);
int		lagg_lb_transmit(struct lagg_proto_softc *, struct mbuf *);
struct mbuf *	lagg_lb_input(struct lagg_proto_softc *, struct lagg_port *,
		    struct mbuf *);
void		lagg_lb_portstat(struct lagg_proto_softc *,
		    struct lagg_port *, struct laggreqport *);

int		lacp_attach(struct lagg_softc *, struct lagg_proto_softc **);
void		lacp_detach(struct lagg_proto_softc *);
int		lacp_up(struct lagg_proto_softc *);
void		lacp_down(struct lagg_proto_softc *);
int		lacp_transmit(struct lagg_proto_softc *, struct mbuf *);
struct mbuf *	lacp_input(struct lagg_proto_softc *, struct lagg_port *,
		    struct mbuf *);
int		lacp_allocport(struct lagg_proto_softc *, struct lagg_port *);
void		lacp_freeport(struct lagg_proto_softc *, struct lagg_port *);
void		lacp_startport(struct lagg_proto_softc *, struct lagg_port *);
void		lacp_stopport(struct lagg_proto_softc *, struct lagg_port *);
void		lacp_protostat(struct lagg_proto_softc *,
		    struct laggreqproto *);
void		lacp_portstat(struct lagg_proto_softc *, struct lagg_port *,
		    struct laggreqport *);
void		lacp_linkstate_ifnet_locked(struct lagg_proto_softc *, struct lagg_port *);
int		lacp_ioctl(struct lagg_proto_softc *, struct laggreqproto *);
#endif
