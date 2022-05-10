/* $NetBSD: if_pppoe.c,v 1.180 2022/05/10 09:05:03 knakahara Exp $ */

/*
 * Copyright (c) 2002, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Martin Husemann <martin@NetBSD.org>.
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
__KERNEL_RCSID(0, "$NetBSD: if_pppoe.c,v 1.180 2022/05/10 09:05:03 knakahara Exp $");

#ifdef _KERNEL_OPT
#include "pppoe.h"
#include "opt_pppoe.h"
#include "opt_net_mpsafe.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/atomic.h>
#include <sys/callout.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/kauth.h>
#include <sys/intr.h>
#include <sys/socketvar.h>
#include <sys/device.h>
#include <sys/module.h>
#include <sys/sysctl.h>
#include <sys/rwlock.h>
#include <sys/mutex.h>
#include <sys/psref.h>
#include <sys/cprng.h>
#include <sys/workqueue.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/if_ether.h>
#include <net/if_sppp.h>
#include <net/if_spppvar.h>
#include <net/if_pppoe.h>
#include <net/if_dl.h>

#include <net/bpf.h>

#include "ioconf.h"

#ifdef NET_MPSAFE
#define PPPOE_MPSAFE	1
#endif

#ifndef PPPOE_DEQUEUE_MAXLEN
#define PPPOE_DEQUEUE_MAXLEN	IFQ_MAXLEN
#endif

struct pppoehdr {
	uint8_t vertype;
	uint8_t code;
	uint16_t session;
	uint16_t plen;
} __packed;

struct pppoetag {
	uint16_t tag;
	uint16_t len;
} __packed;

#define	PPPOE_HEADERLEN	sizeof(struct pppoehdr)
#define	PPPOE_OVERHEAD	(PPPOE_HEADERLEN + 2)
#define	PPPOE_VERTYPE	0x11	/* VER=1, TYPE = 1 */

#define	PPPOE_TAG_EOL		0x0000		/* end of list */
#define	PPPOE_TAG_SNAME		0x0101		/* service name */
#define	PPPOE_TAG_ACNAME	0x0102		/* access concentrator name */
#define	PPPOE_TAG_HUNIQUE	0x0103		/* host unique */
#define	PPPOE_TAG_ACCOOKIE	0x0104		/* AC cookie */
#define	PPPOE_TAG_VENDOR	0x0105		/* vendor specific */
#define	PPPOE_TAG_RELAYSID	0x0110		/* relay session id */
#define	PPPOE_TAG_MAX_PAYLOAD	0x0120		/* max payload */
#define	PPPOE_TAG_SNAME_ERR	0x0201		/* service name error */
#define	PPPOE_TAG_ACSYS_ERR	0x0202		/* AC system error */
#define	PPPOE_TAG_GENERIC_ERR	0x0203		/* generic error */

#define	PPPOE_CODE_PADI		0x09		/* Active Discovery Initiation */
#define	PPPOE_CODE_PADO		0x07		/* Active Discovery Offer */
#define	PPPOE_CODE_PADR		0x19		/* Active Discovery Request */
#define	PPPOE_CODE_PADS		0x65		/* Active Discovery Session confirmation */
#define	PPPOE_CODE_PADT		0xA7		/* Active Discovery Terminate */

/* two byte PPP protocol discriminator, then IP data */
#define	PPPOE_MAXMTU	(ETHERMTU - PPPOE_OVERHEAD)

/* Add a 16 bit unsigned value to a buffer pointed to by PTR */
#define	PPPOE_ADD_16(PTR, VAL)			\
		*(PTR)++ = (VAL) / 256;		\
		*(PTR)++ = (VAL) % 256

/* Add a complete PPPoE header to the buffer pointed to by PTR */
#define	PPPOE_ADD_HEADER(PTR, CODE, SESS, LEN)	\
		*(PTR)++ = PPPOE_VERTYPE;	\
		*(PTR)++ = (CODE);		\
		PPPOE_ADD_16(PTR, SESS);	\
		PPPOE_ADD_16(PTR, LEN)

#define	PPPOE_DISC_TIMEOUT	(hz*5)	/* base for quick timeout calculation */
#define	PPPOE_SLOW_RETRY	(hz*60)	/* persistent retry interval */
#define	PPPOE_RECON_FAST	(hz*15)	/* first retry after auth failure */
#define	PPPOE_RECON_IMMEDIATE	(hz/10)	/* "no delay" reconnect */
#define	PPPOE_RECON_PADTRCVD	(hz*5)	/* reconnect delay after PADT received */
#define	PPPOE_DISC_MAXPADI	4	/* retry PADI four times (quickly) */
#define	PPPOE_DISC_MAXPADR	2	/* retry PADR twice */

#ifdef PPPOE_SERVER
/* from if_spppsubr.c */
#define	IFF_PASSIVE	IFF_LINK0	/* wait passively for connection */
#endif

#define PPPOE_LOCK(_sc, _op)	rw_enter(&(_sc)->sc_lock, (_op))
#define PPPOE_UNLOCK(_sc)	rw_exit(&(_sc)->sc_lock)
#define PPPOE_WLOCKED(_sc)	rw_write_held(&(_sc)->sc_lock)

#ifdef PPPOE_MPSAFE
#define DECLARE_SPLNET_VARIABLE
#define ACQUIRE_SPLNET()	do { } while (0)
#define RELEASE_SPLNET()	do { } while (0)
#else
#define DECLARE_SPLNET_VARIABLE	int __s
#define ACQUIRE_SPLNET()	do {					\
					__s = splnet();			\
				} while (0)
#define RELEASE_SPLNET()	do {					\
					splx(__s);			\
				} while (0)
#endif

#ifdef PPPOE_DEBUG
#define DPRINTF(_sc, _fmt, _arg...)	pppoe_printf((_sc), (_fmt), ##_arg)
#else
#define DPRINTF(_sc, _fmt, _arg...)	__nothing
#endif

struct pppoe_softc {
	struct sppp sc_sppp;		/* contains a struct ifnet as first element */
	LIST_ENTRY(pppoe_softc) sc_list;
	struct ifnet *sc_eth_if;	/* ethernet interface we are using */

	uint64_t sc_id;			/* id of this softc, our hunique */
	int sc_state;			/* discovery phase or session connected */
	struct ether_addr sc_dest;	/* hardware address of concentrator */
	uint16_t sc_session;		/* PPPoE session id */

	char *sc_service_name;		/* if != NULL: requested name of service */
	char *sc_concentrator_name;	/* if != NULL: requested concentrator id */
	uint8_t *sc_ac_cookie;		/* content of AC cookie we must echo back */
	size_t sc_ac_cookie_len;	/* length of cookie data */
	uint8_t *sc_relay_sid;		/* content of relay SID we must echo back */
	size_t sc_relay_sid_len;	/* length of relay SID data */
#ifdef PPPOE_SERVER
	uint8_t *sc_hunique;		/* content of host unique we must echo back */
	size_t sc_hunique_len;		/* length of host unique */
#endif
	callout_t sc_timeout;		/* timeout while not in session state */
	struct workqueue *sc_timeout_wq;	/* workqueue for timeout */
	struct work sc_timeout_wk;
	u_int sc_timeout_scheduled;
	int sc_padi_retried;		/* number of PADI retries already done */
	int sc_padr_retried;		/* number of PADR retries already done */
	krwlock_t sc_lock;	/* lock of sc_state, sc_session, and sc_eth_if */
	bool sc_detaching;
};

/* incoming traffic will be queued here */
struct ifqueue ppoediscinq = { .ifq_maxlen = IFQ_MAXLEN };
struct ifqueue ppoeinq = { .ifq_maxlen = IFQ_MAXLEN };

void *pppoe_softintr = NULL;
static void pppoe_softintr_handler(void *);

extern int sppp_ioctl(struct ifnet *, unsigned long, void *);

/* input routines */
static void pppoeintr(void);
static void pppoe_disc_input(struct mbuf *);
static void pppoe_dispatch_disc_pkt(struct mbuf *, int);
static void pppoe_data_input(struct mbuf *);
static void pppoe_enqueue(struct ifqueue *, struct mbuf *);

/* management routines */
static int pppoe_connect(struct pppoe_softc *);
static int pppoe_disconnect(struct pppoe_softc *);
static void pppoe_abort_connect(struct pppoe_softc *);
static int pppoe_ioctl(struct ifnet *, unsigned long, void *);
static void pppoe_tls(struct sppp *);
static void pppoe_tlf(struct sppp *);
static void pppoe_start(struct ifnet *);
#ifdef PPPOE_MPSAFE
static int pppoe_transmit(struct ifnet *, struct mbuf *);
#endif
static void pppoe_clear_softc(struct pppoe_softc *, const char *);
static void pppoe_printf(struct pppoe_softc *, const char *, ...);

/* internal timeout handling */
static void pppoe_timeout_co(void *);
static void pppoe_timeout_co_halt(void *);
static void pppoe_timeout_wk(struct work *, void *);
static void pppoe_timeout(struct pppoe_softc *);

/* sending actual protocol controll packets */
static int pppoe_send_padi(struct pppoe_softc *);
static int pppoe_send_padr(struct pppoe_softc *);
#ifdef PPPOE_SERVER
static int pppoe_send_pado(struct pppoe_softc *);
static int pppoe_send_pads(struct pppoe_softc *);
#endif
static int pppoe_send_padt(struct ifnet *, u_int, const uint8_t *);

/* raw output */
static int pppoe_output(struct pppoe_softc *, struct mbuf *);

/* internal helper functions */
static struct pppoe_softc * pppoe_find_softc_by_session(u_int, struct ifnet *,
    krw_t);
static struct pppoe_softc * pppoe_find_softc_by_hunique(uint8_t *, size_t,
    struct ifnet *, krw_t);
static struct mbuf *pppoe_get_mbuf(size_t len);

static void pppoe_ifattach_hook(void *, unsigned long, void *);

static LIST_HEAD(pppoe_softc_head, pppoe_softc) pppoe_softc_list;
static krwlock_t pppoe_softc_list_lock;

static int	pppoe_clone_create(struct if_clone *, int);
static int	pppoe_clone_destroy(struct ifnet *);

static bool	pppoe_term_unknown = false;
static int	pppoe_term_unknown_pps = 1;

static struct sysctllog	*pppoe_sysctl_clog;
static void sysctl_net_pppoe_setup(struct sysctllog **);

static struct if_clone pppoe_cloner =
    IF_CLONE_INITIALIZER("pppoe", pppoe_clone_create, pppoe_clone_destroy);

/* ARGSUSED */
void
pppoeattach(int count)
{

	/*
	 * Nothing to do here, initialization is handled by the
	 * module initialization code in pppoeinit() below).
	 */
}

static void
pppoeinit(void)
{

	LIST_INIT(&pppoe_softc_list);
	rw_init(&pppoe_softc_list_lock);
	if_clone_attach(&pppoe_cloner);

	pppoe_softintr = softint_establish(SOFTINT_MPSAFE|SOFTINT_NET,
	    pppoe_softintr_handler, NULL);
	sysctl_net_pppoe_setup(&pppoe_sysctl_clog);

	IFQ_LOCK_INIT(&ppoediscinq);
	IFQ_LOCK_INIT(&ppoeinq);
}

static int
pppoedetach(void)
{
	int error = 0;

	rw_enter(&pppoe_softc_list_lock, RW_READER);
	if (!LIST_EMPTY(&pppoe_softc_list)) {
		rw_exit(&pppoe_softc_list_lock);
		error = EBUSY;
	}

	if (error == 0) {
		if_clone_detach(&pppoe_cloner);
		softint_disestablish(pppoe_softintr);
		/* Remove our sysctl sub-tree */
		sysctl_teardown(&pppoe_sysctl_clog);
	}

	return error;
}

static void
pppoe_softc_genid(uint64_t *id)
{
	struct pppoe_softc *sc;
	uint64_t rndid;

	rw_enter(&pppoe_softc_list_lock, RW_READER);

	while (1) {
		rndid = cprng_strong64();

		sc = NULL;
		LIST_FOREACH(sc, &pppoe_softc_list, sc_list) {
			if (sc->sc_id == rndid)
				break;
		}
		if (sc == NULL) {
			break;
		}
	}

	rw_exit(&pppoe_softc_list_lock);
	*id = rndid;
}

static int
pppoe_clone_create(struct if_clone *ifc, int unit)
{
	struct pppoe_softc *sc;
	struct ifnet *ifp;
	int rv;

	sc = kmem_zalloc(sizeof(*sc), KM_SLEEP);
	ifp = &sc->sc_sppp.pp_if;

	rw_init(&sc->sc_lock);
	pppoe_softc_genid(&sc->sc_id);
	/* changed to real address later */
	memcpy(&sc->sc_dest, etherbroadcastaddr, sizeof(sc->sc_dest));

	if_initname(ifp, "pppoe", unit);
	ifp->if_softc = sc;
	ifp->if_mtu = PPPOE_MAXMTU;
	ifp->if_flags = IFF_SIMPLEX|IFF_POINTOPOINT|IFF_MULTICAST;
#ifdef PPPOE_MPSAFE
	ifp->if_extflags = IFEF_MPSAFE;
#endif
	ifp->if_type = IFT_PPP;
	ifp->if_hdrlen = sizeof(struct ether_header) + PPPOE_HEADERLEN;
	ifp->if_dlt = DLT_PPP_ETHER;
	ifp->if_ioctl = pppoe_ioctl;
	ifp->if_start = pppoe_start;
#ifdef PPPOE_MPSAFE
	ifp->if_transmit = pppoe_transmit;
#endif
	IFQ_SET_MAXLEN(&ifp->if_snd, IFQ_MAXLEN);
	IFQ_SET_READY(&ifp->if_snd);

	sc->sc_sppp.pp_tls = pppoe_tls;
	sc->sc_sppp.pp_tlf = pppoe_tlf;
	sc->sc_sppp.pp_flags |= PP_KEEPALIVE |	/* use LCP keepalive */
				PP_NOFRAMING;	/* no serial encapsulation */
	sc->sc_sppp.pp_framebytes = PPPOE_HEADERLEN;	/* framing added to ppp packets */

	rv = workqueue_create(&sc->sc_timeout_wq,
	    ifp->if_xname, pppoe_timeout_wk, sc,
	    PRI_SOFTNET, IPL_SOFTNET, 0);
	if (rv != 0)
		goto destroy_sclock;

	callout_init(&sc->sc_timeout, CALLOUT_MPSAFE);
	callout_setfunc(&sc->sc_timeout, pppoe_timeout_co, sc);

	if_initialize(ifp);

	ifp->if_percpuq = if_percpuq_create(ifp);

	rw_enter(&pppoe_softc_list_lock, RW_READER);
	if (LIST_EMPTY(&pppoe_softc_list)) {
		pfil_add_ihook(pppoe_ifattach_hook, NULL, PFIL_IFNET, if_pfil);
	}
	LIST_INSERT_HEAD(&pppoe_softc_list, sc, sc_list);
	rw_exit(&pppoe_softc_list_lock);

	sppp_attach(ifp);
	bpf_attach(ifp, DLT_PPP_ETHER, 0);
	if_register(ifp);

	return 0;

destroy_sclock:
	rw_destroy(&sc->sc_lock);
	kmem_free(sc, sizeof(*sc));

	return rv;
}

static int
pppoe_clone_destroy(struct ifnet *ifp)
{
	struct pppoe_softc * sc = ifp->if_softc;

	PPPOE_LOCK(sc, RW_WRITER);
	/* stop ioctls */
	sc->sc_detaching = true;

	if (ifp->if_flags & IFF_RUNNING) {
		pppoe_clear_softc(sc, "destroy interface");
		sc->sc_eth_if = NULL;
	}
	PPPOE_UNLOCK(sc);

	rw_enter(&pppoe_softc_list_lock, RW_WRITER);
	LIST_REMOVE(sc, sc_list);

	if (LIST_EMPTY(&pppoe_softc_list)) {
		pfil_remove_ihook(pppoe_ifattach_hook, NULL, PFIL_IFNET, if_pfil);
	}
	rw_exit(&pppoe_softc_list_lock);

	bpf_detach(ifp);
	sppp_detach(&sc->sc_sppp.pp_if);
	if_detach(ifp);

	callout_setfunc(&sc->sc_timeout, pppoe_timeout_co_halt, sc);

	workqueue_wait(sc->sc_timeout_wq, &sc->sc_timeout_wk);
	workqueue_destroy(sc->sc_timeout_wq);

	callout_halt(&sc->sc_timeout, NULL);
	callout_destroy(&sc->sc_timeout);

#ifdef PPPOE_SERVER
	if (sc->sc_hunique) {
		free(sc->sc_hunique, M_DEVBUF);
		sc->sc_hunique = NULL;
		sc->sc_hunique_len = 0;
	}
#endif
	if (sc->sc_concentrator_name)
		free(sc->sc_concentrator_name, M_DEVBUF);
	if (sc->sc_service_name)
		free(sc->sc_service_name, M_DEVBUF);
	if (sc->sc_ac_cookie)
		free(sc->sc_ac_cookie, M_DEVBUF);
	if (sc->sc_relay_sid)
		free(sc->sc_relay_sid, M_DEVBUF);

	rw_destroy(&sc->sc_lock);

	kmem_free(sc, sizeof(*sc));

	return 0;
}

static void
pppoe_printf(struct pppoe_softc *sc, const char *fmt, ...)
{
	va_list ap;
	bool pppoe_debug;

#ifdef PPPOE_DEBUG
	pppoe_debug = true;
#else
	pppoe_debug = false;
#endif

	if (sc == NULL) {
		if (!pppoe_debug)
			return;

		printf("pppoe: ");
	} else {
		if (!ISSET(sc->sc_sppp.pp_if.if_flags, IFF_DEBUG))
			return;

		printf("%s: ", sc->sc_sppp.pp_if.if_xname);
	}

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

/*
 * Find the interface handling the specified session.
 * Note: O(number of sessions open), this is a client-side only, mean
 * and lean implementation, so number of open sessions typically should
 * be 1.
 */
static struct pppoe_softc *
pppoe_find_softc_by_session(u_int session, struct ifnet *rcvif, krw_t lock)
{
	struct pppoe_softc *sc = NULL;

	if (session == 0)
		return NULL;
	rw_enter(&pppoe_softc_list_lock, RW_READER);
	LIST_FOREACH(sc, &pppoe_softc_list, sc_list) {
		PPPOE_LOCK(sc, lock);
		if ( sc->sc_state == PPPOE_STATE_SESSION
		    && sc->sc_session == session
		    && sc->sc_eth_if == rcvif)
			break;

		PPPOE_UNLOCK(sc);
	}
	rw_exit(&pppoe_softc_list_lock);
	return sc;
}

/* Check host unique token passed and return appropriate softc pointer,
 * or NULL if token is bogus. */
static struct pppoe_softc *
pppoe_find_softc_by_hunique(uint8_t *token, size_t len,
    struct ifnet *rcvif, krw_t lock)
{
	struct pppoe_softc *sc;
	uint64_t t;

	CTASSERT(sizeof(t) == sizeof(sc->sc_id));

	rw_enter(&pppoe_softc_list_lock, RW_READER);
	if (LIST_EMPTY(&pppoe_softc_list)) {
		rw_exit(&pppoe_softc_list_lock);
		return NULL;
	}

	if (len != sizeof(sc->sc_id)) {
		rw_exit(&pppoe_softc_list_lock);
		return NULL;
	}
	memcpy(&t, token, len);

	LIST_FOREACH(sc, &pppoe_softc_list, sc_list) {
		PPPOE_LOCK(sc, lock);
		if (sc->sc_id == t && sc->sc_eth_if != NULL) {
			break;
		}
		PPPOE_UNLOCK(sc);
	}
	rw_exit(&pppoe_softc_list_lock);

	if (sc == NULL) {
		pppoe_printf(NULL, "alien host unique tag"
		    ", no session found\n");
		return NULL;
	}

	/* should be safe to access *sc now */
	if (sc->sc_state < PPPOE_STATE_PADI_SENT || sc->sc_state >= PPPOE_STATE_SESSION) {
		pppoe_printf(sc, "host unique tag found"
		    ", but it belongs to a connection in state %d\n",
		    sc->sc_state);
		PPPOE_UNLOCK(sc);
		return NULL;
	}
	if (sc->sc_eth_if != rcvif) {
		pppoe_printf(sc, "wrong interface, not accepting host unique\n");
		PPPOE_UNLOCK(sc);
		return NULL;
	}
	return sc;
}

static void
pppoe_softintr_handler(void *dummy)
{
	/* called at splsoftnet() */
	pppoeintr();
}

/* called at appropriate protection level */
static void
pppoeintr(void)
{
	struct mbuf *m;
	int i;

	SOFTNET_LOCK_UNLESS_NET_MPSAFE();

	for (i = 0; i < PPPOE_DEQUEUE_MAXLEN; i++) {
		IFQ_LOCK(&ppoediscinq);
		IF_DEQUEUE(&ppoediscinq, m);
		IFQ_UNLOCK(&ppoediscinq);
		if (m == NULL)
			break;
		pppoe_disc_input(m);
	}

	for (i = 0; i < PPPOE_DEQUEUE_MAXLEN; i++) {
		IFQ_LOCK(&ppoeinq);
		IF_DEQUEUE(&ppoeinq, m);
		IFQ_UNLOCK(&ppoeinq);
		if (m == NULL)
			break;
		pppoe_data_input(m);
	}

#if PPPOE_DEQUEUE_MAXLEN < IFQ_MAXLEN
	if (!IF_IS_EMPTY(&ppoediscinq) || !IF_IS_EMPTY(&ppoeinq))
		softint_schedule(pppoe_softintr);
#endif

	SOFTNET_UNLOCK_UNLESS_NET_MPSAFE();
}

/* analyze and handle a single received packet while not in session state */
static void
pppoe_dispatch_disc_pkt(struct mbuf *m, int off)
{
	uint16_t tag, len;
	uint16_t session, plen;
	struct pppoe_softc *sc;
	const char *err_msg;
	char *error;
	size_t dlen;
	uint8_t *ac_cookie;
	size_t ac_cookie_len;
	uint8_t *relay_sid;
	size_t relay_sid_len;
	uint8_t *hunique;
	size_t hunique_len;
	struct pppoehdr *ph;
	struct pppoetag *pt;
	struct mbuf *n;
	int noff, err, errortag;
	struct ether_header *eh;
	struct ifnet *rcvif;
	struct psref psref;

	if (m->m_len < sizeof(*eh)) {
		m = m_pullup(m, sizeof(*eh));
		if (m == NULL)
			goto done;
	}
	eh = mtod(m, struct ether_header *);
	off += sizeof(*eh);

	if (m->m_pkthdr.len - off < PPPOE_HEADERLEN) {
		goto done;
	}

	M_REGION_GET(ph, struct pppoehdr *, m, off, sizeof(*ph));
	if (ph == NULL) {
		goto done;
	}
	if (ph->vertype != PPPOE_VERTYPE) {
		goto done;
	}

	ac_cookie = NULL;
	ac_cookie_len = 0;
	relay_sid = NULL;
	relay_sid_len = 0;
	hunique = NULL;
	hunique_len = 0;

	session = ntohs(ph->session);
	plen = ntohs(ph->plen);
	off += sizeof(*ph);

	if (plen + off > m->m_pkthdr.len) {
		goto done;
	}
	m_adj(m, off + plen - m->m_pkthdr.len);	/* ignore trailing garbage */

	tag = 0;
	len = 0;
	sc = NULL;
	err_msg = NULL;
	errortag = 0;
	while (off + sizeof(*pt) <= m->m_pkthdr.len) {
		M_REGION_GET(pt, struct pppoetag *, m, off, sizeof(*pt));
		if (pt == NULL) {
			goto done;
		}

		tag = ntohs(pt->tag);
		len = ntohs(pt->len);
		if (off + len + sizeof(*pt) > m->m_pkthdr.len) {
			goto done;
		}
		switch (tag) {
		case PPPOE_TAG_EOL:
			goto breakbreak;
		case PPPOE_TAG_SNAME:
			break;	/* ignored */
		case PPPOE_TAG_ACNAME:
			if (len > 0) {
				dlen = 4 * len + 1;
				error = malloc(dlen, M_TEMP, M_NOWAIT);
				if (error == NULL)
					break;

				n = m_pulldown(m, off + sizeof(*pt), len,
				    &noff);
				if (!n) {
					m = NULL;
					free(error, M_TEMP);
					goto done;
				}

				strnvisx(error, dlen,
				    mtod(n, char*) + noff, len,
				    VIS_SAFE | VIS_OCTAL);
				pppoe_printf(NULL, "connected to %s\n", error);
				free(error, M_TEMP);
			}
			break;	/* ignored */
		case PPPOE_TAG_HUNIQUE:
			if (hunique == NULL) {
				n = m_pulldown(m, off + sizeof(*pt), len,
				    &noff);
				if (!n) {
					m = NULL;
					err_msg = "TAG HUNIQUE ERROR";
					break;
				}

				hunique = mtod(n, uint8_t *) + noff;
				hunique_len = len;
			}
			break;
		case PPPOE_TAG_ACCOOKIE:
			if (ac_cookie == NULL) {
				n = m_pulldown(m, off + sizeof(*pt), len,
				    &noff);
				if (!n) {
					err_msg = "TAG ACCOOKIE ERROR";
					m = NULL;
					break;
				}
				ac_cookie = mtod(n, char *) + noff;
				ac_cookie_len = len;
			}
			break;
		case PPPOE_TAG_RELAYSID:
			if (relay_sid == NULL) {
				n = m_pulldown(m, off + sizeof(*pt), len,
				    &noff);
				if (!n) {
					err_msg = "TAG RELAYSID ERROR";
					m = NULL;
					break;
				}
				relay_sid = mtod(n, char *) + noff;
				relay_sid_len = len;
			}
			break;
		case PPPOE_TAG_SNAME_ERR:
			err_msg = "SERVICE NAME ERROR";
			errortag = 1;
			break;
		case PPPOE_TAG_ACSYS_ERR:
			err_msg = "AC SYSTEM ERROR";
			errortag = 1;
			break;
		case PPPOE_TAG_GENERIC_ERR:
			err_msg = "GENERIC ERROR";
			errortag = 1;
			break;
		}
		if (err_msg) {
			error = NULL;
			if (errortag && len) {
				dlen = 4 * len + 1;
				error = malloc(dlen, M_TEMP,
				    M_NOWAIT|M_ZERO);
				n = m_pulldown(m, off + sizeof(*pt), len,
				    &noff);
				if (!n) {
					m = NULL;
				} else if (error) {
					strnvisx(error, dlen,
					    mtod(n, char*) + noff, len,
					    VIS_SAFE | VIS_OCTAL);
				}
			}
			if (error) {
				pppoe_printf(NULL, "%s: %s\n", err_msg, error);
				free(error, M_TEMP);
			} else
				pppoe_printf(NULL, "%s\n", err_msg);
			if (errortag || m == NULL)
				goto done;
		}
		off += sizeof(*pt) + len;
	}
breakbreak:;

	switch (ph->code) {
	case PPPOE_CODE_PADI:
#ifdef PPPOE_SERVER
		/*
		 * got service name, concentrator name, and/or host unique.
		 * ignore if we have no interfaces with IFF_PASSIVE|IFF_UP.
		 */
		rw_enter(&pppoe_softc_list_lock, RW_READER);
		if (LIST_EMPTY(&pppoe_softc_list)) {
			rw_exit(&pppoe_softc_list_lock);
			goto done;
		}

		LIST_FOREACH(sc, &pppoe_softc_list, sc_list) {
			PPPOE_LOCK(sc, RW_WRITER);
			if (!(sc->sc_sppp.pp_if.if_flags & IFF_UP)) {
				PPPOE_UNLOCK(sc);
				continue;
			}
			if (!(sc->sc_sppp.pp_if.if_flags & IFF_PASSIVE)) {
				PPPOE_UNLOCK(sc);
				continue;
			}

			if (sc->sc_state == PPPOE_STATE_INITIAL)
				break;

			PPPOE_UNLOCK(sc);
		}
		rw_exit(&pppoe_softc_list_lock);

		if (sc == NULL) {
			goto done;
		}

		if (hunique) {
			if (sc->sc_hunique)
				free(sc->sc_hunique, M_DEVBUF);
			sc->sc_hunique = malloc(hunique_len, M_DEVBUF,
			    M_DONTWAIT);
			if (sc->sc_hunique == NULL) {
				PPPOE_UNLOCK(sc);
				goto done;
			}
			sc->sc_hunique_len = hunique_len;
			memcpy(sc->sc_hunique, hunique, hunique_len);
		}
		memcpy(&sc->sc_dest, eh->ether_shost, sizeof sc->sc_dest);
		sc->sc_state = PPPOE_STATE_PADO_SENT;
		pppoe_send_pado(sc);
		PPPOE_UNLOCK(sc);
		break;
#endif /* PPPOE_SERVER */

	case PPPOE_CODE_PADR:
#ifdef PPPOE_SERVER
		/*
		 * get sc from ac_cookie if IFF_PASSIVE
		 */
		if (ac_cookie == NULL) {
			goto done;
		}

		rcvif = m_get_rcvif_psref(m, &psref);
		if (__predict_true(rcvif != NULL)) {
			sc = pppoe_find_softc_by_hunique(ac_cookie,
			    ac_cookie_len, rcvif, RW_WRITER);
		}
		m_put_rcvif_psref(rcvif, &psref);
		if (sc == NULL) {
			/* be quiet if there is not a single pppoe instance */
			rw_enter(&pppoe_softc_list_lock, RW_READER);
			if (!LIST_EMPTY(&pppoe_softc_list)) {
				pppoe_printf(NULL, "received PADR"
				    " but could not find request for it\n");
			}
			rw_exit(&pppoe_softc_list_lock);
			goto done;
		}

		if (sc->sc_state != PPPOE_STATE_PADO_SENT) {
			pppoe_printf(sc, "received unexpected PADR\n");
			PPPOE_UNLOCK(sc);
			goto done;
		}

		if (hunique) {
			if (sc->sc_hunique)
				free(sc->sc_hunique, M_DEVBUF);
			sc->sc_hunique = malloc(hunique_len, M_DEVBUF,
			    M_DONTWAIT);
			if (sc->sc_hunique == NULL) {
				PPPOE_UNLOCK(sc);
				goto done;
			}
			sc->sc_hunique_len = hunique_len;
			memcpy(sc->sc_hunique, hunique, hunique_len);
		}
		pppoe_send_pads(sc);
		sc->sc_state = PPPOE_STATE_SESSION;
		PPPOE_UNLOCK(sc);

		sc->sc_sppp.pp_up(&sc->sc_sppp);
		break;
#else
		/* ignore, we are no access concentrator */
		goto done;
#endif /* PPPOE_SERVER */

	case PPPOE_CODE_PADO:
		rcvif = m_get_rcvif_psref(m, &psref);
		if (__predict_false(rcvif == NULL))
			goto done;

		if (hunique != NULL) {
			sc = pppoe_find_softc_by_hunique(hunique,
			    hunique_len, rcvif, RW_WRITER);
		}

		m_put_rcvif_psref(rcvif, &psref);

		if (sc == NULL) {
			/* be quiet if there is not a single pppoe instance */
			rw_enter(&pppoe_softc_list_lock, RW_READER);
			if (!LIST_EMPTY(&pppoe_softc_list)) {
				pppoe_printf(NULL, "received PADO"
				    " but could not find request for it\n");
			}
			rw_exit(&pppoe_softc_list_lock);
			goto done;
		}

		if (sc->sc_state != PPPOE_STATE_PADI_SENT) {
			pppoe_printf(sc, "received unexpected PADO\n");
			PPPOE_UNLOCK(sc);
			goto done;
		}

		if (ac_cookie) {
			if (sc->sc_ac_cookie)
				free(sc->sc_ac_cookie, M_DEVBUF);
			sc->sc_ac_cookie = malloc(ac_cookie_len, M_DEVBUF,
			    M_DONTWAIT);
			if (sc->sc_ac_cookie == NULL) {
				pppoe_printf(sc, "FATAL: could not allocate memory "
				    "for AC cookie\n");
				sc->sc_ac_cookie_len = 0;
				PPPOE_UNLOCK(sc);
				goto done;
			}
			sc->sc_ac_cookie_len = ac_cookie_len;
			memcpy(sc->sc_ac_cookie, ac_cookie, ac_cookie_len);
		} else if (sc->sc_ac_cookie) {
			free(sc->sc_ac_cookie, M_DEVBUF);
			sc->sc_ac_cookie = NULL;
			sc->sc_ac_cookie_len = 0;
		}
		if (relay_sid) {
			if (sc->sc_relay_sid)
				free(sc->sc_relay_sid, M_DEVBUF);
			sc->sc_relay_sid = malloc(relay_sid_len, M_DEVBUF,
			    M_DONTWAIT);
			if (sc->sc_relay_sid == NULL) {
				pppoe_printf(sc, "FATAL: could not allocate memory "
				    "for relay SID\n");
				sc->sc_relay_sid_len = 0;
				PPPOE_UNLOCK(sc);
				goto done;
			}
			sc->sc_relay_sid_len = relay_sid_len;
			memcpy(sc->sc_relay_sid, relay_sid, relay_sid_len);
		} else if (sc->sc_relay_sid) {
			free(sc->sc_relay_sid, M_DEVBUF);
			sc->sc_relay_sid = NULL;
			sc->sc_relay_sid_len = 0;
		}
		memcpy(&sc->sc_dest, eh->ether_shost, sizeof sc->sc_dest);
		callout_stop(&sc->sc_timeout);
		sc->sc_padr_retried = 0;
		sc->sc_state = PPPOE_STATE_PADR_SENT;
		if ((err = pppoe_send_padr(sc)) != 0) {
			pppoe_printf(sc,
			    "failed to send PADR, error=%d\n", err);
		}
		callout_schedule(&sc->sc_timeout,
		    PPPOE_DISC_TIMEOUT * (1 + sc->sc_padr_retried));

		PPPOE_UNLOCK(sc);
		break;

	case PPPOE_CODE_PADS:
		rcvif = m_get_rcvif_psref(m, &psref);
		if (__predict_false(rcvif == NULL))
			goto done;

		if (hunique != NULL) {
			sc = pppoe_find_softc_by_hunique(hunique,
			    hunique_len, rcvif, RW_WRITER);
		}

		m_put_rcvif_psref(rcvif, &psref);

		if (sc == NULL)
			goto done;

		if (memcmp(&sc->sc_dest, eh->ether_shost,
		    sizeof sc->sc_dest) != 0) {
			PPPOE_UNLOCK(sc);
			goto done;
		}

		sc->sc_session = session;
		callout_stop(&sc->sc_timeout);
		pppoe_printf(sc, "session 0x%x connected\n", session);
		sc->sc_state = PPPOE_STATE_SESSION;
		PPPOE_UNLOCK(sc);

		sc->sc_sppp.pp_up(&sc->sc_sppp);	/* notify upper layers */
		break;

	case PPPOE_CODE_PADT:
		rcvif = m_get_rcvif_psref(m, &psref);
		if (__predict_false(rcvif == NULL))
			goto done;

		sc = pppoe_find_softc_by_session(session, rcvif,
		    RW_WRITER);

		m_put_rcvif_psref(rcvif, &psref);

		if (sc == NULL)
			goto done;

		if (memcmp(&sc->sc_dest, eh->ether_shost,
		    sizeof sc->sc_dest) != 0) {
			PPPOE_UNLOCK(sc);
			goto done;
		}

		pppoe_clear_softc(sc, "received PADT");
		if (sc->sc_sppp.pp_if.if_flags & IFF_RUNNING) {
			pppoe_printf(sc, "wait for reconnect\n");
			callout_schedule(&sc->sc_timeout,
			    PPPOE_RECON_PADTRCVD);
		}
		PPPOE_UNLOCK(sc);
		break;

	default:
		rcvif = m_get_rcvif_psref(m, &psref);
		if (__predict_false(rcvif == NULL))
			goto done;

		if (hunique != NULL) {
			sc = pppoe_find_softc_by_hunique(hunique,
			    hunique_len, rcvif, RW_READER);
		}

		m_put_rcvif_psref(rcvif, &psref);

		pppoe_printf(sc, "unknown code (0x%04x) session = 0x%04x\n",
		    ph->code, session);
		if (sc == NULL)
			goto done;
		PPPOE_UNLOCK(sc);
		break;
	}

done:
	if (m)
		m_freem(m);
	return;
}

static void
pppoe_disc_input(struct mbuf *m)
{
	KASSERT(m->m_flags & M_PKTHDR);

	/*
	 * Avoid error messages if there is not a single PPPoE instance.
	 */
	rw_enter(&pppoe_softc_list_lock, RW_READER);
	if (!LIST_EMPTY(&pppoe_softc_list)) {
		rw_exit(&pppoe_softc_list_lock);
		pppoe_dispatch_disc_pkt(m, 0);
	} else {
		rw_exit(&pppoe_softc_list_lock);
		m_freem(m);
	}
}

static bool
pppoe_is_my_frame(uint8_t *dhost, struct ifnet *rcvif)
{

	if (memcmp(CLLADDR(rcvif->if_sadl), dhost, ETHER_ADDR_LEN) == 0)
		return true;

	return false;
}

static void
pppoe_data_input(struct mbuf *m)
{
	uint16_t session, plen;
	struct pppoe_softc *sc;
	struct pppoehdr *ph;
	struct ifnet *rcvif;
	struct psref psref;
	uint8_t shost[ETHER_ADDR_LEN];
	uint8_t dhost[ETHER_ADDR_LEN];
	bool term_unknown = pppoe_term_unknown;

	KASSERT(m->m_flags & M_PKTHDR);

	/*
	 * Avoid error messages if there is not a single PPPoE instance.
	 */
	rw_enter(&pppoe_softc_list_lock, RW_READER);
	if (LIST_EMPTY(&pppoe_softc_list)) {
		rw_exit(&pppoe_softc_list_lock);
		goto drop;
	}
	rw_exit(&pppoe_softc_list_lock);

	if (term_unknown) {
		memcpy(shost, mtod(m, struct ether_header*)->ether_shost,
		    ETHER_ADDR_LEN);
		memcpy(dhost, mtod(m, struct ether_header*)->ether_dhost,
		    ETHER_ADDR_LEN);
	}
	m_adj(m, sizeof(struct ether_header));
	if (m->m_pkthdr.len <= PPPOE_HEADERLEN) {
		goto drop;
	}

	if (m->m_len < sizeof(*ph)) {
		m = m_pullup(m, sizeof(*ph));
		if (m == NULL) {
			return;
		}
	}
	ph = mtod(m, struct pppoehdr *);

	if (ph->vertype != PPPOE_VERTYPE) {
		goto drop;
	}
	if (ph->code != 0) {
		goto drop;
	}

	session = ntohs(ph->session);
	rcvif = m_get_rcvif_psref(m, &psref);
	if (__predict_false(rcvif == NULL))
		goto drop;
	sc = pppoe_find_softc_by_session(session, rcvif, RW_READER);
	if (sc == NULL) {
		if (term_unknown) {
			static struct timeval lasttime = {0, 0};
			static int curpps = 0;
			/*
			 * avoid to send wrong PADT which is response from
			 * session stage packets for other hosts when parent
			 * ethernet is promiscuous mode.
			 */
			if (pppoe_is_my_frame(dhost, rcvif) &&
			    ppsratecheck(&lasttime, &curpps,
				pppoe_term_unknown_pps)) {
				pppoe_printf(NULL, "input for unknown session %#x, "
				    "sending PADT\n", session);
				pppoe_send_padt(rcvif, session, shost);
			}
		}
		m_put_rcvif_psref(rcvif, &psref);
		goto drop;
	}

	m_put_rcvif_psref(rcvif, &psref);

	plen = ntohs(ph->plen);

	bpf_mtap(&sc->sc_sppp.pp_if, m, BPF_D_IN);

	m_adj(m, PPPOE_HEADERLEN);

#ifdef PPPOE_DEBUG
	{
		struct mbuf *p;

		printf("%s: pkthdr.len=%d, pppoe.len=%d",
		    sc->sc_sppp.pp_if.if_xname, m->m_pkthdr.len, plen);
		p = m;
		while (p) {
			printf(" l=%d", p->m_len);
			p = p->m_next;
		}
		printf("\n");
	}
#endif
	PPPOE_UNLOCK(sc);

	if (m->m_pkthdr.len < plen)
		goto drop;

	/* ignore trailing garbage */
	m_adj(m, plen - m->m_pkthdr.len);
	/*
	 * Fix incoming interface pointer (not the raw ethernet interface
	 * anymore)
	 */
	m_set_rcvif(m, &sc->sc_sppp.pp_if);

	/* pass packet up and account for it */
	if_statinc(&sc->sc_sppp.pp_if, if_ipackets);
	sppp_input(&sc->sc_sppp.pp_if, m);
	return;

drop:
	m_freem(m);
}

static int
pppoe_output(struct pppoe_softc *sc, struct mbuf *m)
{
	struct sockaddr dst;
	struct ether_header *eh;
	uint16_t etype;

	if (sc->sc_eth_if == NULL) {
		m_freem(m);
		return EIO;
	}

	memset(&dst, 0, sizeof dst);
	dst.sa_family = AF_UNSPEC;
	eh = (struct ether_header*)&dst.sa_data;
	etype = sc->sc_state == PPPOE_STATE_SESSION
	    ? ETHERTYPE_PPPOE : ETHERTYPE_PPPOEDISC;
	eh->ether_type = htons(etype);
	memcpy(&eh->ether_dhost, &sc->sc_dest, sizeof sc->sc_dest);

	DPRINTF(sc, "(%x) state=%d, session=0x%x output -> %s, len=%d\n",
	    etype, sc->sc_state, sc->sc_session,
	    ether_sprintf((const unsigned char *)&sc->sc_dest), m->m_pkthdr.len);

	m->m_flags &= ~(M_BCAST|M_MCAST);
	if_statinc(&sc->sc_sppp.pp_if, if_opackets);
	return if_output_lock(sc->sc_eth_if, sc->sc_eth_if, m, &dst, NULL);
}

static int
pppoe_parm_cpyinstr(struct pppoe_softc *sc,
    char **dst, const void *src, size_t len)
{
	int error = 0;
	char *next = NULL;
	size_t bufsiz, cpysiz, strsiz;

	bufsiz = len + 1;

	if (src == NULL)
		goto out;

	bufsiz = len + 1;
	next = malloc(bufsiz, M_DEVBUF, M_WAITOK);
	if (next == NULL)
		return ENOMEM;

	error = copyinstr(src, next, bufsiz, &cpysiz);
	if (error != 0)
		goto fail;
	if (cpysiz != bufsiz) {
		error = EINVAL;
		goto fail;
	}

	strsiz = strnlen(next, bufsiz);
	if (strsiz == bufsiz) {
		error = EINVAL;
		goto fail;
	}

out:
	PPPOE_LOCK(sc, RW_WRITER);
	if (*dst != NULL)
		free(*dst, M_DEVBUF);
	*dst = next;
	next = NULL;
	PPPOE_UNLOCK(sc);
fail:
	if (next != NULL)
		free(next, M_DEVBUF);

	return error;
}

static int
pppoe_ioctl(struct ifnet *ifp, unsigned long cmd, void *data)
{
	struct lwp *l = curlwp;	/* XXX */
	struct pppoe_softc *sc = (struct pppoe_softc*)ifp;
	struct ifreq *ifr = data;
	int error = 0;

	switch (cmd) {
	case PPPOESETPARMS:
	{
		struct pppoediscparms *parms = (struct pppoediscparms*)data;
		if (kauth_authorize_network(l->l_cred, KAUTH_NETWORK_INTERFACE,
		    KAUTH_REQ_NETWORK_INTERFACE_SETPRIV, ifp, (void *)cmd,
		    NULL) != 0)
			return EPERM;
		if (parms->eth_ifname[0] != 0) {
			struct ifnet *eth_if;

			PPPOE_LOCK(sc, RW_WRITER);
			if (sc->sc_detaching) {
				PPPOE_UNLOCK(sc);
				return ENXIO;
			}
			eth_if = ifunit(parms->eth_ifname);
			if (eth_if == NULL || eth_if->if_dlt != DLT_EN10MB) {
				sc->sc_eth_if = NULL;
				PPPOE_UNLOCK(sc);
				return ENXIO;
			}

			if (sc->sc_sppp.pp_if.if_mtu !=
			    eth_if->if_mtu - PPPOE_OVERHEAD) {
				sc->sc_sppp.pp_if.if_mtu = eth_if->if_mtu -
				    PPPOE_OVERHEAD;
			}
			sc->sc_eth_if = eth_if;
			PPPOE_UNLOCK(sc);
		}

		error = pppoe_parm_cpyinstr(sc, &sc->sc_concentrator_name,
		    parms->ac_name, parms->ac_name_len);
		if (error != 0)
			return error;

		error = pppoe_parm_cpyinstr(sc, &sc->sc_service_name,
		    parms->service_name, parms->service_name_len);
		if (error != 0)
			return error;
		return 0;
	}
	break;
	case PPPOEGETPARMS:
	{
		struct pppoediscparms *parms = (struct pppoediscparms*)data;
		memset(parms, 0, sizeof *parms);
		PPPOE_LOCK(sc, RW_READER);
		if (sc->sc_eth_if)
			strlcpy(parms->ifname, sc->sc_eth_if->if_xname,
			    sizeof(parms->ifname));
		PPPOE_UNLOCK(sc);
		return 0;
	}
	break;
	case PPPOEGETSESSION:
	{
		struct pppoeconnectionstate *state = (struct pppoeconnectionstate*)data;
		PPPOE_LOCK(sc, RW_READER);
		state->state = sc->sc_state;
		state->session_id = sc->sc_session;
		state->padi_retry_no = sc->sc_padi_retried;
		state->padr_retry_no = sc->sc_padr_retried;
		PPPOE_UNLOCK(sc);
		return 0;
	}
	break;
	case SIOCSIFFLAGS:
		/*
		 * Prevent running re-establishment timers overriding
		 * administrators choice.
		 */
		PPPOE_LOCK(sc, RW_WRITER);
		if (sc->sc_detaching) {
			PPPOE_UNLOCK(sc);
			return ENXIO;
		}

		if ((ifr->ifr_flags & IFF_UP) == 0
		     && sc->sc_state < PPPOE_STATE_SESSION) {
			callout_stop(&sc->sc_timeout);
			sc->sc_state = PPPOE_STATE_INITIAL;
			sc->sc_padi_retried = 0;
			sc->sc_padr_retried = 0;
			memcpy(&sc->sc_dest, etherbroadcastaddr,
			    sizeof(sc->sc_dest));
		}

		PPPOE_UNLOCK(sc);

		error = sppp_ioctl(ifp, cmd, data);

		return error;
	case SIOCSIFMTU:
		if (ifr->ifr_mtu > (sc->sc_eth_if == NULL ?
		    PPPOE_MAXMTU : (sc->sc_eth_if->if_mtu - PPPOE_OVERHEAD))) {
			return EINVAL;
		}
		/*FALLTHROUGH*/
	default:
		return sppp_ioctl(ifp, cmd, data);
	}
	return 0;
}

/*
 * Allocate a mbuf/cluster with space to store the given data length
 * of payload, leaving space for prepending an ethernet header
 * in front.
 */
static struct mbuf *
pppoe_get_mbuf(size_t len)
{
	struct mbuf *m;

	if (len + sizeof(struct ether_header) > MCLBYTES)
		return NULL;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return NULL;
	if (len + sizeof(struct ether_header) > MHLEN) {
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_free(m);
			return NULL;
		}
	}
	m->m_data += sizeof(struct ether_header);
	m->m_len = len;
	m->m_pkthdr.len = len;
	m_reset_rcvif(m);

	return m;
}

static int
pppoe_send_padi(struct pppoe_softc *sc)
{
	struct mbuf *m0;
	int len, l1 = 0, l2 = 0;
	uint8_t *p;

	if (sc->sc_state > PPPOE_STATE_PADI_SENT)
		panic("pppoe_send_padi in state %d", sc->sc_state);

	/* Compute packet length. */
	len = sizeof(struct pppoetag);
	if (sc->sc_service_name != NULL) {
		l1 = strlen(sc->sc_service_name);
		len += l1;
	}
	if (sc->sc_concentrator_name != NULL) {
		l2 = strlen(sc->sc_concentrator_name);
		len += sizeof(struct pppoetag) + l2;
	}
	len += sizeof(struct pppoetag) + sizeof(sc->sc_id);
	if (sc->sc_sppp.pp_if.if_mtu > PPPOE_MAXMTU) {
		len += sizeof(struct pppoetag) + 2;
	}

	/* Allocate packet. */
	m0 = pppoe_get_mbuf(len + PPPOE_HEADERLEN);
	if (m0 == NULL)
		return ENOBUFS;

	/* Fill in packet. */
	p = mtod(m0, uint8_t *);
	PPPOE_ADD_HEADER(p, PPPOE_CODE_PADI, 0, len);
	PPPOE_ADD_16(p, PPPOE_TAG_SNAME);
	if (sc->sc_service_name != NULL) {
		PPPOE_ADD_16(p, l1);
		memcpy(p, sc->sc_service_name, l1);
		p += l1;
	} else {
		PPPOE_ADD_16(p, 0);
	}
	if (sc->sc_concentrator_name != NULL) {
		PPPOE_ADD_16(p, PPPOE_TAG_ACNAME);
		PPPOE_ADD_16(p, l2);
		memcpy(p, sc->sc_concentrator_name, l2);
		p += l2;
	}
	PPPOE_ADD_16(p, PPPOE_TAG_HUNIQUE);
	PPPOE_ADD_16(p, sizeof(sc->sc_id));
	memcpy(p, &sc->sc_id, sizeof(sc->sc_id));
	p += sizeof(sc->sc_id);

	if (sc->sc_sppp.pp_if.if_mtu > PPPOE_MAXMTU) {
		PPPOE_ADD_16(p, PPPOE_TAG_MAX_PAYLOAD);
		PPPOE_ADD_16(p, 2);
		PPPOE_ADD_16(p, (uint16_t)sc->sc_sppp.pp_if.if_mtu);
	}

#ifdef PPPOE_DEBUG
	if (p - mtod(m0, uint8_t *) != len + PPPOE_HEADERLEN)
		panic("pppoe_send_padi: garbled output len, should be %ld, is %ld",
		    (long)(len + PPPOE_HEADERLEN), (long)(p - mtod(m0, uint8_t *)));
#endif

	/* Send packet. */
	return pppoe_output(sc, m0);
}

static void
pppoe_timeout_co(void *arg)
{
	struct pppoe_softc *sc = (struct pppoe_softc *)arg;

	if (atomic_swap_uint(&sc->sc_timeout_scheduled, 1) != 0)
		return;

	workqueue_enqueue(sc->sc_timeout_wq, &sc->sc_timeout_wk, NULL);
}

static void
pppoe_timeout_co_halt(void *unused __unused)
{

	/* do nothing to halt callout safely */
}

static void
pppoe_timeout_wk(struct work *wk __unused, void *arg)
{
	struct pppoe_softc *sc = (struct pppoe_softc *)arg;

	atomic_swap_uint(&sc->sc_timeout_scheduled, 0);
	pppoe_timeout(sc);
}

static void
pppoe_timeout(struct pppoe_softc *sc)
{
	int retry_wait, err;
	DECLARE_SPLNET_VARIABLE;

	pppoe_printf(sc, "timeout\n");

	PPPOE_LOCK(sc, RW_WRITER);
	switch (sc->sc_state) {
	case PPPOE_STATE_INITIAL:
		/* delayed connect from pppoe_tls() */
		if (!sc->sc_detaching)
			pppoe_connect(sc);
		break;
	case PPPOE_STATE_PADI_SENT:
		/*
		 * We have two basic ways of retrying:
		 *  - Quick retry mode: try a few times in short sequence
		 *  - Slow retry mode: we already had a connection successfully
		 *    established and will try infinitely (without user
		 *    intervention)
		 * We only enter slow retry mode if IFF_LINK1 (aka autodial)
		 * is not set.
		 */

		/* initialize for quick retry mode */
		retry_wait = PPPOE_DISC_TIMEOUT * (1 + sc->sc_padi_retried);

		ACQUIRE_SPLNET();
		sc->sc_padi_retried++;
		if (sc->sc_padi_retried >= PPPOE_DISC_MAXPADI) {
			if ((sc->sc_sppp.pp_if.if_flags & IFF_LINK1) == 0) {
				/* slow retry mode */
				retry_wait = PPPOE_SLOW_RETRY;
			} else {
				pppoe_abort_connect(sc);
				RELEASE_SPLNET();
				PPPOE_UNLOCK(sc);
				return;
			}
		}
		if ((err = pppoe_send_padi(sc)) != 0) {
			sc->sc_padi_retried--;
			pppoe_printf(sc,
			    "failed to transmit PADI, error=%d\n", err);
		}
		callout_schedule(&sc->sc_timeout,retry_wait);
		RELEASE_SPLNET();
		break;

	case PPPOE_STATE_PADR_SENT:
		ACQUIRE_SPLNET();
		sc->sc_padr_retried++;
		if (sc->sc_padr_retried >= PPPOE_DISC_MAXPADR) {
			memcpy(&sc->sc_dest, etherbroadcastaddr,
			    sizeof(sc->sc_dest));
			sc->sc_state = PPPOE_STATE_PADI_SENT;
			sc->sc_padi_retried = 0;
			sc->sc_padr_retried = 0;
			if ((err = pppoe_send_padi(sc)) != 0) {
				pppoe_printf(sc,
				    "failed to send PADI, error=%d\n", err);
			}
			callout_schedule(&sc->sc_timeout,
			    PPPOE_DISC_TIMEOUT * (1 + sc->sc_padi_retried));
			RELEASE_SPLNET();
			PPPOE_UNLOCK(sc);
			return;
		}
		if ((err = pppoe_send_padr(sc)) != 0) {
			sc->sc_padr_retried--;
			pppoe_printf(sc,"failed to send PADR, error=%d", err);
		}
		callout_schedule(&sc->sc_timeout,
		    PPPOE_DISC_TIMEOUT * (1 + sc->sc_padr_retried));
		RELEASE_SPLNET();
		break;
	case PPPOE_STATE_CLOSING:
		pppoe_disconnect(sc);
		break;
	default:
		PPPOE_UNLOCK(sc);
		return;	/* all done, work in peace */
	}
	PPPOE_UNLOCK(sc);
}

/* Start a connection (i.e. initiate discovery phase) */
static int
pppoe_connect(struct pppoe_softc *sc)
{
	int err;
	DECLARE_SPLNET_VARIABLE;

	KASSERT(PPPOE_WLOCKED(sc));

	if (sc->sc_state != PPPOE_STATE_INITIAL)
		return EBUSY;

#ifdef PPPOE_SERVER
	/* wait PADI if IFF_PASSIVE */
	if ((sc->sc_sppp.pp_if.if_flags & IFF_PASSIVE))
		return 0;
#endif
	ACQUIRE_SPLNET();
	/* save state, in case we fail to send PADI */
	sc->sc_state = PPPOE_STATE_PADI_SENT;
	sc->sc_padi_retried = 0;
	sc->sc_padr_retried = 0;
	err = pppoe_send_padi(sc);
	if (err != 0)
		pppoe_printf(sc, "failed to send PADI, error=%d\n", err);
	callout_schedule(&sc->sc_timeout, PPPOE_DISC_TIMEOUT);
	RELEASE_SPLNET();
	return err;
}

/* disconnect */
static int
pppoe_disconnect(struct pppoe_softc *sc)
{
	int err;
	DECLARE_SPLNET_VARIABLE;

	KASSERT(PPPOE_WLOCKED(sc));

	ACQUIRE_SPLNET();

	if (sc->sc_state < PPPOE_STATE_SESSION)
		err = EBUSY;
	else {
		pppoe_printf(sc, "disconnecting\n");
		err = pppoe_send_padt(sc->sc_eth_if, sc->sc_session,
		    (const uint8_t *)&sc->sc_dest);
	}

	/* cleanup softc */
	sc->sc_state = PPPOE_STATE_INITIAL;

	memcpy(&sc->sc_dest, etherbroadcastaddr, sizeof(sc->sc_dest));
	if (sc->sc_ac_cookie) {
		free(sc->sc_ac_cookie, M_DEVBUF);
		sc->sc_ac_cookie = NULL;
	}
	sc->sc_ac_cookie_len = 0;
	if (sc->sc_relay_sid) {
		free(sc->sc_relay_sid, M_DEVBUF);
		sc->sc_relay_sid = NULL;
	}
	sc->sc_relay_sid_len = 0;
#ifdef PPPOE_SERVER
	if (sc->sc_hunique) {
		free(sc->sc_hunique, M_DEVBUF);
		sc->sc_hunique = NULL;
	}
	sc->sc_hunique_len = 0;
#endif
	sc->sc_session = 0;

	PPPOE_UNLOCK(sc);

	/* notify upper layer */
	sc->sc_sppp.pp_down(&sc->sc_sppp);

	PPPOE_LOCK(sc, RW_WRITER);

	RELEASE_SPLNET();
	return err;
}

/* Connection attempt aborted */
static void
pppoe_abort_connect(struct pppoe_softc *sc)
{
	KASSERT(PPPOE_WLOCKED(sc));

	pppoe_printf(sc, "could not establish connection\n");
	sc->sc_state = PPPOE_STATE_CLOSING;

	PPPOE_UNLOCK(sc);

	/* notify upper layer */
	sc->sc_sppp.pp_down(&sc->sc_sppp);

	PPPOE_LOCK(sc, RW_WRITER);

	/* clear connection state */
	memcpy(&sc->sc_dest, etherbroadcastaddr, sizeof(sc->sc_dest));
	sc->sc_state = PPPOE_STATE_INITIAL;
}

static int
pppoe_send_padr(struct pppoe_softc *sc)
{
	struct mbuf *m0;
	uint8_t *p;
	size_t len, l1 = 0;

	if (sc->sc_state != PPPOE_STATE_PADR_SENT)
		return EIO;

	/* Compute packet length. */
	len = sizeof(struct pppoetag);
	if (sc->sc_service_name != NULL) {
		l1 = strlen(sc->sc_service_name);
		len += l1;
	}
	if (sc->sc_ac_cookie_len > 0) {
		len += sizeof(struct pppoetag) + sc->sc_ac_cookie_len;
	}
	if (sc->sc_relay_sid_len > 0) {
		len += sizeof(struct pppoetag) + sc->sc_relay_sid_len;
	}
	len += sizeof(struct pppoetag) + sizeof(sc->sc_id);
	if (sc->sc_sppp.pp_if.if_mtu > PPPOE_MAXMTU) {
		len += sizeof(struct pppoetag) + 2;
	}

	/* Allocate packet. */
	m0 = pppoe_get_mbuf(len + PPPOE_HEADERLEN);
	if (m0 == NULL)
		return ENOBUFS;

	/* Fill in packet. */
	p = mtod(m0, uint8_t *);
	PPPOE_ADD_HEADER(p, PPPOE_CODE_PADR, 0, len);
	PPPOE_ADD_16(p, PPPOE_TAG_SNAME);
	if (sc->sc_service_name != NULL) {
		PPPOE_ADD_16(p, l1);
		memcpy(p, sc->sc_service_name, l1);
		p += l1;
	} else {
		PPPOE_ADD_16(p, 0);
	}
	if (sc->sc_ac_cookie_len > 0) {
		PPPOE_ADD_16(p, PPPOE_TAG_ACCOOKIE);
		PPPOE_ADD_16(p, sc->sc_ac_cookie_len);
		memcpy(p, sc->sc_ac_cookie, sc->sc_ac_cookie_len);
		p += sc->sc_ac_cookie_len;
	}
	if (sc->sc_relay_sid_len > 0) {
		PPPOE_ADD_16(p, PPPOE_TAG_RELAYSID);
		PPPOE_ADD_16(p, sc->sc_relay_sid_len);
		memcpy(p, sc->sc_relay_sid, sc->sc_relay_sid_len);
		p += sc->sc_relay_sid_len;
	}
	PPPOE_ADD_16(p, PPPOE_TAG_HUNIQUE);
	PPPOE_ADD_16(p, sizeof(sc->sc_id));
	memcpy(p, &sc->sc_id, sizeof(sc->sc_id));
	p += sizeof(sc->sc_id);

	if (sc->sc_sppp.pp_if.if_mtu > PPPOE_MAXMTU) {
		PPPOE_ADD_16(p, PPPOE_TAG_MAX_PAYLOAD);
		PPPOE_ADD_16(p, 2);
		PPPOE_ADD_16(p, (uint16_t)sc->sc_sppp.pp_if.if_mtu);
	}

#ifdef PPPOE_DEBUG
	if (p - mtod(m0, uint8_t *) != len + PPPOE_HEADERLEN)
		panic("pppoe_send_padr: garbled output len, should be %ld, is %ld",
			(long)(len + PPPOE_HEADERLEN), (long)(p - mtod(m0, uint8_t *)));
#endif

	/* Send packet. */
	return pppoe_output(sc, m0);
}

/* send a PADT packet */
static int
pppoe_send_padt(struct ifnet *outgoing_if, u_int session, const uint8_t *dest)
{
	struct ether_header *eh;
	struct sockaddr dst;
	struct mbuf *m0;
	uint8_t *p;

	m0 = pppoe_get_mbuf(PPPOE_HEADERLEN);
	if (!m0)
		return EIO;
	p = mtod(m0, uint8_t *);
	PPPOE_ADD_HEADER(p, PPPOE_CODE_PADT, session, 0);

	memset(&dst, 0, sizeof dst);
	dst.sa_family = AF_UNSPEC;
	eh = (struct ether_header*)&dst.sa_data;
	eh->ether_type = htons(ETHERTYPE_PPPOEDISC);
	memcpy(&eh->ether_dhost, dest, ETHER_ADDR_LEN);

	m0->m_flags &= ~(M_BCAST|M_MCAST);
	return if_output_lock(outgoing_if, outgoing_if, m0, &dst, NULL);
}

#ifdef PPPOE_SERVER
static int
pppoe_send_pado(struct pppoe_softc *sc)
{
	struct mbuf *m0;
	uint8_t *p;
	size_t len;

	if (sc->sc_state != PPPOE_STATE_PADO_SENT)
		return EIO;

	/* Include AC cookie. */
	len = sizeof(struct pppoetag) + sizeof(sc->sc_id);
	/* Include hunique. */
	len += sizeof(struct pppoetag) + sc->sc_hunique_len;

	m0 = pppoe_get_mbuf(len + PPPOE_HEADERLEN);
	if (!m0)
		return EIO;
	p = mtod(m0, uint8_t *);

	PPPOE_ADD_HEADER(p, PPPOE_CODE_PADO, 0, len);
	PPPOE_ADD_16(p, PPPOE_TAG_ACCOOKIE);
	PPPOE_ADD_16(p, sizeof(sc->sc_id));
	memcpy(p, &sc->sc_id, sizeof(sc->sc_id));
	p += sizeof(sc->sc_id);
	PPPOE_ADD_16(p, PPPOE_TAG_HUNIQUE);
	PPPOE_ADD_16(p, sc->sc_hunique_len);
	memcpy(p, sc->sc_hunique, sc->sc_hunique_len);
	return pppoe_output(sc, m0);
}

static int
pppoe_send_pads(struct pppoe_softc *sc)
{
	struct bintime bt;
	struct mbuf *m0;
	uint8_t *p;
	size_t len, l1 = 0;	/* XXX: gcc */

	KASSERT(PPPOE_WLOCKED(sc));

	if (sc->sc_state != PPPOE_STATE_PADO_SENT)
		return EIO;

	getbinuptime(&bt);
	sc->sc_session = bt.sec % 0xff + 1;

	/* Include service name. */
	len = sizeof(struct pppoetag);
	if (sc->sc_service_name != NULL) {
		l1 = strlen(sc->sc_service_name);
		len += l1;
	}
	/* Include hunique. */
	len += sizeof(struct pppoetag) + sc->sc_hunique_len;

	m0 = pppoe_get_mbuf(len + PPPOE_HEADERLEN);
	if (!m0)
		return ENOBUFS;
	p = mtod(m0, uint8_t *);

	PPPOE_ADD_HEADER(p, PPPOE_CODE_PADS, sc->sc_session, len);
	PPPOE_ADD_16(p, PPPOE_TAG_SNAME);
	if (sc->sc_service_name != NULL) {
		PPPOE_ADD_16(p, l1);
		memcpy(p, sc->sc_service_name, l1);
		p += l1;
	} else {
		PPPOE_ADD_16(p, 0);
	}
	PPPOE_ADD_16(p, PPPOE_TAG_HUNIQUE);
	PPPOE_ADD_16(p, sc->sc_hunique_len);
	memcpy(p, sc->sc_hunique, sc->sc_hunique_len);
	return pppoe_output(sc, m0);
}
#endif

static void
pppoe_tls(struct sppp *sp)
{
	struct pppoe_softc *sc = (void *)sp;
	int wtime;

	PPPOE_LOCK(sc, RW_READER);

	if (sc->sc_state != PPPOE_STATE_INITIAL) {
		PPPOE_UNLOCK(sc);
		return;
	}

	if (sc->sc_sppp.pp_phase == SPPP_PHASE_ESTABLISH &&
	    sc->sc_sppp.pp_auth_failures > 0) {
		/*
		 * Delay trying to reconnect a bit more - the peer
		 * might have failed to contact its radius server.
		 */
		wtime = PPPOE_RECON_FAST * sc->sc_sppp.pp_auth_failures;
		if (wtime > PPPOE_SLOW_RETRY)
			wtime = PPPOE_SLOW_RETRY;
	} else {
		wtime = PPPOE_RECON_IMMEDIATE;
	}
	callout_schedule(&sc->sc_timeout, wtime);

	PPPOE_UNLOCK(sc);
}

static void
pppoe_tlf(struct sppp *sp)
{
	struct pppoe_softc *sc = (void *)sp;

	PPPOE_LOCK(sc, RW_WRITER);

	if (sc->sc_state < PPPOE_STATE_SESSION) {
		callout_stop(&sc->sc_timeout);
		sc->sc_state = PPPOE_STATE_INITIAL;
		sc->sc_padi_retried = 0;
		sc->sc_padr_retried = 0;
		memcpy(&sc->sc_dest, etherbroadcastaddr,
		    sizeof(sc->sc_dest));
		PPPOE_UNLOCK(sc);
		return;
	}

	/*
	 * Do not call pppoe_disconnect here, the upper layer state
	 * machine gets confused by this. We must return from this
	 * function and defer disconnecting to the timeout handler.
	 */
	sc->sc_state = PPPOE_STATE_CLOSING;

	callout_schedule(&sc->sc_timeout, hz/50);

	PPPOE_UNLOCK(sc);
}

static void
pppoe_start(struct ifnet *ifp)
{
	struct pppoe_softc *sc = (void *)ifp;
	struct mbuf *m;
	uint8_t *p;
	size_t len;

	if (sppp_isempty(ifp))
		return;

	/* are we ready to process data yet? */
	PPPOE_LOCK(sc, RW_READER);
	if (sc->sc_state < PPPOE_STATE_SESSION) {
		sppp_flush(&sc->sc_sppp.pp_if);
		PPPOE_UNLOCK(sc);
		return;
	}

	while ((m = sppp_dequeue(ifp)) != NULL) {
		len = m->m_pkthdr.len;
		M_PREPEND(m, PPPOE_HEADERLEN, M_DONTWAIT);
		if (m == NULL) {
			if_statinc(ifp, if_oerrors);
			continue;
		}
		p = mtod(m, uint8_t *);
		PPPOE_ADD_HEADER(p, 0, sc->sc_session, len);

		bpf_mtap(&sc->sc_sppp.pp_if, m, BPF_D_OUT);

		pppoe_output(sc, m);
	}
	PPPOE_UNLOCK(sc);
}

#ifdef PPPOE_MPSAFE
static int
pppoe_transmit(struct ifnet *ifp, struct mbuf *m)
{
	struct pppoe_softc *sc = (void *)ifp;
	uint8_t *p;
	size_t len;

	if (m == NULL)
		return EINVAL;

	/* are we ready to process data yet? */
	PPPOE_LOCK(sc, RW_READER);
	if (sc->sc_state < PPPOE_STATE_SESSION) {
		PPPOE_UNLOCK(sc);
		m_freem(m);
		return ENOBUFS;
	}

	len = m->m_pkthdr.len;
	M_PREPEND(m, PPPOE_HEADERLEN, M_DONTWAIT);
	if (m == NULL) {
		PPPOE_UNLOCK(sc);
		if_statinc(ifp, if_oerrors);
		return ENETDOWN;
	}
	p = mtod(m, uint8_t *);
	PPPOE_ADD_HEADER(p, 0, sc->sc_session, len);

	bpf_mtap(&sc->sc_sppp.pp_if, m, BPF_D_OUT);

	pppoe_output(sc, m);
	PPPOE_UNLOCK(sc);
	return 0;
}
#endif /* PPPOE_MPSAFE */

static void
pppoe_ifattach_hook(void *arg, unsigned long cmd, void *arg2)
{
	struct ifnet *ifp = arg2;
	struct pppoe_softc *sc;
	DECLARE_SPLNET_VARIABLE;

	if (cmd != PFIL_IFNET_DETACH)
		return;

	ACQUIRE_SPLNET();
	rw_enter(&pppoe_softc_list_lock, RW_READER);
	LIST_FOREACH(sc, &pppoe_softc_list, sc_list) {
		PPPOE_LOCK(sc, RW_WRITER);
		if (sc->sc_eth_if != ifp) {
			PPPOE_UNLOCK(sc);
			continue;
		}
		if (sc->sc_sppp.pp_if.if_flags & IFF_UP) {
			sc->sc_sppp.pp_if.if_flags &= ~(IFF_UP|IFF_RUNNING);
			pppoe_printf(sc,
			    "ethernet interface detached, going down\n");
		}
		sc->sc_eth_if = NULL;
		pppoe_clear_softc(sc, "ethernet interface detached");
		PPPOE_UNLOCK(sc);
	}
	rw_exit(&pppoe_softc_list_lock);
	RELEASE_SPLNET();
}

static void
pppoe_clear_softc(struct pppoe_softc *sc, const char *message)
{
	KASSERT(PPPOE_WLOCKED(sc));

	/* stop timer */
	callout_stop(&sc->sc_timeout);
	pppoe_printf(sc, "session 0x%x terminated, %s\n",
	    sc->sc_session, message);

	/* fix our state */
	sc->sc_state = PPPOE_STATE_INITIAL;

	PPPOE_UNLOCK(sc);

	/* signal upper layer */
	sc->sc_sppp.pp_down(&sc->sc_sppp);

	PPPOE_LOCK(sc, RW_WRITER);

	/* clean up softc */
	memcpy(&sc->sc_dest, etherbroadcastaddr, sizeof(sc->sc_dest));
	if (sc->sc_ac_cookie) {
		free(sc->sc_ac_cookie, M_DEVBUF);
		sc->sc_ac_cookie = NULL;
	}
	if (sc->sc_relay_sid) {
		free(sc->sc_relay_sid, M_DEVBUF);
		sc->sc_relay_sid = NULL;
	}
	sc->sc_ac_cookie_len = 0;
	sc->sc_session = 0;
}

static void
pppoe_enqueue(struct ifqueue *inq, struct mbuf *m)
{
	if (m->m_flags & M_PROMISC) {
		m_freem(m);
		return;
	}

#ifndef PPPOE_SERVER
	if (m->m_flags & (M_MCAST | M_BCAST)) {
		m_freem(m);
		return;
	}
#endif

	IFQ_LOCK(inq);
	if (IF_QFULL(inq)) {
		IF_DROP(inq);
		IFQ_UNLOCK(inq);
		m_freem(m);
	} else {
		IF_ENQUEUE(inq, m);
		IFQ_UNLOCK(inq);
		softint_schedule(pppoe_softintr);
	}
	return;
}

void
pppoe_input(struct ifnet *ifp, struct mbuf *m)
{
	pppoe_enqueue(&ppoeinq, m);
	return;
}

void
pppoedisc_input(struct ifnet *ifp, struct mbuf *m)
{
	pppoe_enqueue(&ppoediscinq, m);
	return;
}

static void
sysctl_net_pppoe_setup(struct sysctllog **clog)
{
	const struct sysctlnode *node = NULL;
	extern pktq_rps_hash_func_t sppp_pktq_rps_hash_p;

	sppp_pktq_rps_hash_p = pktq_rps_hash_default;

	sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "pppoe",
	    SYSCTL_DESCR("PPPOE protocol"),
	    NULL, 0, NULL, 0,
	    CTL_NET, CTL_CREATE, CTL_EOL);

	if (node == NULL)
		return;

	sysctl_createv(clog, 0, &node, NULL,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_BOOL, "term_unknown",
	    SYSCTL_DESCR("Terminate unknown sessions"),
	    NULL, 0, &pppoe_term_unknown, sizeof(pppoe_term_unknown),
	    CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &node, NULL,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
	    CTLTYPE_STRING, "rps_hash",
	    SYSCTL_DESCR("Interface rps hash function control"),
	    sysctl_pktq_rps_hash_handler, 0, (void *)&sppp_pktq_rps_hash_p,
	    PKTQ_RPS_HASH_NAME_LEN,
	    CTL_CREATE, CTL_EOL);
}

/*
 * Module infrastructure
 */
#include "if_module.h"

IF_MODULE(MODULE_CLASS_DRIVER, pppoe, "sppp_subr")
