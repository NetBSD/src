/* $NetBSD: if_pppoe.c,v 1.4.2.1 2001/09/13 01:16:21 thorpej Exp $ */

/*
 * Copyright (c) 2001 Martin Husemann. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include "pppoe.h"
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/callout.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_types.h>
#include <net/if_ether.h>
#include <net/if_sppp.h>
#include <net/if_pppoe.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#if NPPPOE > 0

#undef PPPOE_DEBUG		/* XXX - remove this or make it an option */
/* #define PPPOE_DEBUG 1 */

#define PPPOE_HEADERLEN	6
#define	PPPOE_VERTYPE	0x11	/* VER=1, TYPE = 1 */

#define	PPPOE_TAG_EOL		0x0000		/* end of list */
#define	PPPOE_TAG_SNAME		0x0101		/* service name */
#define	PPPOE_TAG_ACNAME	0x0102		/* access concentrator name */
#define	PPPOE_TAG_HUNIQUE	0x0103		/* host unique */
#define	PPPOE_TAG_ACCOOKIE	0x0104		/* AC cookie */
#define	PPPOE_TAG_VENDOR	0x0105		/* vendor specific */
#define	PPPOE_TAG_RELAYSID	0x0110		/* relay session id */
#define	PPPOE_TAG_SNAME_ERR	0x0201		/* service name error */
#define	PPPOE_TAG_ACSYS_ERR	0x0202		/* AC system error */
#define	PPPOE_TAG_GENERIC_ERR	0x0203		/* gerneric error */

#define PPPOE_CODE_PADI		0x09		/* Active Discovery Initiation */
#define	PPPOE_CODE_PADO		0x07		/* Active Discovery Offer */
#define	PPPOE_CODE_PADR		0x19		/* Active Discovery Request */
#define	PPPOE_CODE_PADS		0x65		/* Active Discovery Session confirmation */
#define	PPPOE_CODE_PADT		0xA7		/* Active Discovery Terminate */

/* Read a 16 bit unsigned value from a buffer */
#define PPPOE_READ_16(PTR, VAL)				\
		(VAL) = ((PTR)[0] << 8) | (PTR)[1];	\
		(PTR)+=2

/* Add a 16 bit unsigned value to a buffer pointed to by PTR */
#define	PPPOE_ADD_16(PTR, VAL)			\
		*(PTR)++ = (VAL) / 256;		\
		*(PTR)++ = (VAL) % 256

/* Add a complete PPPoE header to the buffer pointed to by PTR */
#define PPPOE_ADD_HEADER(PTR, CODE, SESS, LEN)	\
		*(PTR)++ = PPPOE_VERTYPE;	\
		*(PTR)++ = (CODE);		\
		PPPOE_ADD_16(PTR, SESS);	\
		PPPOE_ADD_16(PTR, LEN)

struct pppoe_softc {
	struct sppp sc_sppp;		/* contains a struct ifnet as first element */
	LIST_ENTRY(pppoe_softc) sc_list;
	struct ifnet *sc_eth_if;	/* ethernet interface we are using */

#define	PPPOE_DISC_TIMEOUT	hz/5
#define PPPOE_DISC_MAXPADI	4	/* retry PADI four times */
#define	PPPOE_DISC_MAXPADR	2	/* retry PADR twice */
	
#define PPPOE_STATE_INITIAL	0
#define PPPOE_STATE_PADI_SENT	1
#define	PPPOE_STATE_PADR_SENT	2
#define	PPPOE_STATE_SESSION	3
#define	PPPOE_STATE_CLOSING	4
	int sc_state;			/* discovery phase or session connected */
	struct ether_addr sc_dest;	/* hardware address of concentrator */
	u_int16_t sc_session;		/* PPPoE session id */

	char *sc_service_name;		/* if != NULL: requested name of service */
	char *sc_concentrator_name;	/* if != NULL: requested concentrator id */
	u_int8_t *sc_ac_cookie;		/* content of AC cookie we must echo back */
	size_t sc_ac_cookie_len;	/* length of cookie data */
	struct callout sc_timeout;	/* timeout while not in session state */
	int sc_padi_retried;		/* number of PADI retries already done */
	int sc_padr_retried;		/* number of PADR retries already done */
};

/* incoming traffic will be queued here */
struct ifqueue ppoediscinq = { NULL };
struct ifqueue ppoeinq = { NULL };

#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
void * pppoe_softintr = NULL;
static void pppoe_softintr_handler(void *);
#else
struct callout pppoe_softintr = CALLOUT_INITIALIZER;
void pppoe_softintr_handler(void*);
#endif

extern int sppp_ioctl(struct ifnet *ifp, unsigned long cmd, void *data);

/* input routines */
static void pppoe_input(void);
static void pppoe_disc_input(struct mbuf *m);
static void pppoe_dispatch_disc_pkt(u_int8_t *p, size_t size, struct ifnet *rcvif, struct ether_header *eh);
static void pppoe_data_input(struct mbuf *m);

/* management routines */
void pppoeattach(int count);
static int pppoe_connect(struct pppoe_softc *sc);
static int pppoe_disconnect(struct pppoe_softc *sc);
static void pppoe_abort_connect(struct pppoe_softc *sc);
static int pppoe_ioctl(struct ifnet *ifp, unsigned long cmd, caddr_t data);
static void pppoe_tls(struct sppp *sp);
static void pppoe_tlf(struct sppp *sp);
static void pppoe_start(struct ifnet *ifp);

/* internal timeout handling */
static void pppoe_timeout(void*);

/* sending actual protocol controll packets */
static int pppoe_send_padi(struct pppoe_softc *sc);
static int pppoe_send_padr(struct pppoe_softc *sc);
static int pppoe_send_padt(struct pppoe_softc *sc);

/* raw output */
static int pppoe_output(struct pppoe_softc *sc, struct mbuf *m);

/* internal helper functions */
static struct pppoe_softc * pppoe_find_softc_by_session(u_int session, struct ifnet *rcvif);
static struct pppoe_softc * pppoe_find_softc_by_hunique(u_int8_t *token, size_t len, struct ifnet *rcvif);

LIST_HEAD(pppoe_softc_head, pppoe_softc) pppoe_softc_list;

int     pppoe_clone_create __P((struct if_clone *, int));
void    pppoe_clone_destroy __P((struct ifnet *));

struct if_clone pppoe_cloner =
    IF_CLONE_INITIALIZER("pppoe", pppoe_clone_create, pppoe_clone_destroy);

/* ARGSUSED */
void
pppoeattach(count)
	int count;
{
	LIST_INIT(&pppoe_softc_list);
	if_clone_attach(&pppoe_cloner);

	ppoediscinq.ifq_maxlen = IFQ_MAXLEN;
	ppoeinq.ifq_maxlen = IFQ_MAXLEN;

#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
	pppoe_softintr = softintr_establish(IPL_SOFTNET, pppoe_softintr_handler, NULL);
#endif
}

int
pppoe_clone_create(ifc, unit)
	struct if_clone *ifc;
	int unit;
{
	struct pppoe_softc *sc;

	sc = malloc(sizeof(struct pppoe_softc), M_DEVBUF, M_WAITOK);
	memset(sc, 0, sizeof(struct pppoe_softc));

	sprintf(sc->sc_sppp.pp_if.if_xname, "pppoe%d", unit);
	sc->sc_sppp.pp_if.if_softc = sc;
	sc->sc_sppp.pp_if.if_mtu = ETHERMTU - PPPOE_HEADERLEN - 2; /* two byte PPP protocol discriminator, then IP data */
	sc->sc_sppp.pp_if.if_flags = IFF_SIMPLEX | IFF_POINTOPOINT
	    | IFF_MULTICAST | IFF_LINK1;	/* auto "dial" */
	sc->sc_sppp.pp_if.if_type = IFT_PPP;
	sc->sc_sppp.pp_if.if_hdrlen = sizeof(struct ether_header)+PPPOE_HEADERLEN;
	sc->sc_sppp.pp_if.if_dlt = DLT_PPP_ETHER;
	sc->sc_sppp.pp_flags |= PP_NOFRAMING;	/* no serial encapsulation */
	sc->sc_sppp.pp_if.if_ioctl = pppoe_ioctl;
	IFQ_SET_MAXLEN(&sc->sc_sppp.pp_if.if_snd, IFQ_MAXLEN);
	IFQ_SET_READY(&sc->sc_sppp.pp_if.if_snd);

	/* changed to real address later */
	memcpy(&sc->sc_dest, etherbroadcastaddr, sizeof(sc->sc_dest));

	callout_init(&sc->sc_timeout);

	sc->sc_sppp.pp_if.if_start = pppoe_start;
	sc->sc_sppp.pp_tls = pppoe_tls;
	sc->sc_sppp.pp_tlf = pppoe_tlf;
	sc->sc_sppp.pp_framebytes = PPPOE_HEADERLEN;	/* framing added to ppp packets */
	
	if_attach(&sc->sc_sppp.pp_if);
	sppp_attach(&sc->sc_sppp.pp_if);

	if_alloc_sadl(&sc->sc_sppp.pp_if);
#if NBPFILTER > 0
	bpfattach(&sc->sc_sppp.pp_if, DLT_PPP_ETHER, 0);
#endif
	LIST_INSERT_HEAD(&pppoe_softc_list, sc, sc_list);
	return 0;
}

void
pppoe_clone_destroy(ifp)
	struct ifnet *ifp;
{
	struct pppoe_softc * sc = ifp->if_softc;

	LIST_REMOVE(sc, sc_list);
#if NBPFILTER > 0
	bpfdetach(ifp);
#endif
	if_detach(ifp);
	free(sc, M_DEVBUF);
}

/*
 * Find the interface handling the specified session.
 * Note: O(number of sessions open), this is a client-side only, mean
 * and lean implementation, so number of open sessions typically should
 * be 1.
 */
static struct pppoe_softc *
pppoe_find_softc_by_session(u_int session, struct ifnet *rcvif)
{
	struct pppoe_softc *sc;

	if (session == 0) return NULL;

	LIST_FOREACH(sc, &pppoe_softc_list, sc_list) {
		if (sc->sc_state == PPPOE_STATE_SESSION
		    && sc->sc_session == session) {
			if (sc->sc_eth_if == rcvif)
				return sc;
			else
				return NULL;
		}
	}
	return NULL;
}

/* Check host unique token passed and return appropriate softc pointer,
 * or NULL if token is bogus. */
static struct pppoe_softc *
pppoe_find_softc_by_hunique(u_int8_t *token, size_t len, struct ifnet *rcvif)
{
	struct pppoe_softc *sc, *t;
	if (len != sizeof sc) return NULL;
	memcpy(&t, token, len);

	LIST_FOREACH(sc, &pppoe_softc_list, sc_list)
		if (sc == t) break;

	if (sc != t) {
#ifdef PPPOE_DEBUG
		printf("pppoe: invalid host unique value\n");
#endif
		return NULL;
	}

	/* should be safe to access *sc now */
	if (sc->sc_state < PPPOE_STATE_PADI_SENT || sc->sc_state >= PPPOE_STATE_SESSION) {
#ifdef PPPOE_DEBUG
		printf("%s: state=%d, not accepting host unique\n",
			sc->sc_sppp.pp_if.if_xname, sc->sc_state);
#endif
		return NULL;
	}
	if (sc->sc_eth_if != rcvif) {
#ifdef PPPOE_DEBUG
		printf("%s: wrong interface, not accepting host unique\n",
			sc->sc_sppp.pp_if.if_xname);
#endif
		return NULL;
	}
	return sc;
}

#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
static void pppoe_softintr_handler(void *dummy)
{
	/* called at splsoftnet() */
	pppoe_input();
}
#else
void pppoe_softintr_handler(void *dummy)
{
	int s = splnet();
	pppoe_input();
	callout_deactivate(&pppoe_softintr);
	splx(s);
}
#endif

/* called at appropriate protection level */
static void
pppoe_input()
{
	struct mbuf *m;
	int s, disc_done, data_done;

	do {
		disc_done = 0;
		data_done = 0;
		for (;;) {
			s = splnet();
			IF_DEQUEUE(&ppoediscinq, m);
			splx(s);
			if (m == NULL) break;
			disc_done = 1;
			pppoe_disc_input(m);
		}

		for (;;) {
			s = splnet();
			IF_DEQUEUE(&ppoeinq, m);
			splx(s);
			if (m == NULL) break;
			data_done = 1;
			pppoe_data_input(m);
		}
	} while (disc_done || data_done);
}

/* analyze and handle a single received packet while not in session state */
static void pppoe_dispatch_disc_pkt(u_int8_t *p, size_t size, struct ifnet *rcvif, struct ether_header *eh)
{
	u_int16_t tag, len;
	u_int8_t vertype, code;
	u_int16_t session, plen;
	struct pppoe_softc *sc;
	const char *err_msg = NULL;
	u_int8_t * ac_cookie;
	size_t ac_cookie_len;

	ac_cookie = NULL;
	ac_cookie_len = 0;
	session = 0;
	if (size <= PPPOE_HEADERLEN) {
		printf("pppoe: packet too short: %ld\n", (long)size);
		return;
	}
	vertype = *p++;
	if (vertype != PPPOE_VERTYPE) {
		printf("pppoe: unknown version/type packet: 0x%x\n", vertype);
		return;
	}
	code = *p++;
	PPPOE_READ_16(p, session);
	PPPOE_READ_16(p, plen);
	size -= PPPOE_HEADERLEN;

	if (plen > size) {
		printf("pppoe: packet content does not fit: data available = %ld, packet size = %ld\n",
			(long)size, (long)plen);
		return;			
	}
	size = plen;	/* ignore trailing garbage */
	tag = 0;
	len = 0;
	sc = NULL;
	while (size > 4) {
		PPPOE_READ_16(p, tag);
		PPPOE_READ_16(p, len);
		if (len > size) {
			printf("pppoe: tag 0x%x len 0x%x is too long\n", tag, len);
			return;
		}
		switch (tag) {
		case PPPOE_TAG_EOL:
			size = 0; break;
		case PPPOE_TAG_SNAME:
			break;	/* ignored */
		case PPPOE_TAG_ACNAME:
			break;	/* ignored */
		case PPPOE_TAG_HUNIQUE:
			if (sc == NULL)
				sc = pppoe_find_softc_by_hunique(p, len, rcvif);
			break;
		case PPPOE_TAG_ACCOOKIE:
			if (ac_cookie == NULL) {
				ac_cookie = p;
				ac_cookie_len = len;
			}
			break;
		case PPPOE_TAG_SNAME_ERR:
			err_msg = "SERVICE NAME ERROR";
			break;
		case PPPOE_TAG_ACSYS_ERR:
			err_msg = "AC SYSTEM ERROR";
			break;
		case PPPOE_TAG_GENERIC_ERR:
			err_msg = "GENERIC ERROR";
			break;
		}
		if (err_msg) {
			printf("%s: %s\n", sc? sc->sc_sppp.pp_if.if_xname : "pppoe",
					err_msg);
			return;
		}
		if (size >= 0) {
			size -= 4 + len;
			if (len > 0)
				p += len;
		}
	}
	switch (code) {
	case PPPOE_CODE_PADI:
	case PPPOE_CODE_PADR:
		/* ignore, we are no access concentrator */
		return;
	case PPPOE_CODE_PADO:
		if (sc == NULL) {
			printf("pppoe: received PADO but could not find request for it\n");
			return;
		}
		if (sc->sc_state != PPPOE_STATE_PADI_SENT) {
			printf("%s: received unexpected PADO\n", sc->sc_sppp.pp_if.if_xname);
			return;
		}
		if (ac_cookie) {
			sc->sc_ac_cookie = malloc(ac_cookie_len, M_DEVBUF, M_DONTWAIT);
			if (sc->sc_ac_cookie == NULL)
				return;
			sc->sc_ac_cookie_len = ac_cookie_len;
			memcpy(sc->sc_ac_cookie, ac_cookie, ac_cookie_len);
		}
		memcpy(&sc->sc_dest, eh->ether_shost, sizeof sc->sc_dest);
		callout_stop(&sc->sc_timeout);
		sc->sc_padr_retried = 0;
		sc->sc_state = PPPOE_STATE_PADR_SENT;
		if (pppoe_send_padr(sc) == 0) 
			callout_reset(&sc->sc_timeout, PPPOE_DISC_TIMEOUT*(1+sc->sc_padr_retried), pppoe_timeout, sc);
		else
			pppoe_abort_connect(sc);
		break;
	case PPPOE_CODE_PADS:
		if (sc == NULL)
			return;
		sc->sc_session = session;
		callout_stop(&sc->sc_timeout);
		printf("%s: session 0x%x connected\n", sc->sc_sppp.pp_if.if_xname, session);
		sc->sc_state = PPPOE_STATE_SESSION;
		sc->sc_sppp.pp_up(&sc->sc_sppp);	/* notify upper layers */
		break;
	case PPPOE_CODE_PADT:
		if (sc == NULL)
			return;
                /* stop timer (we might be about to transmit a PADT ourself) */
                callout_stop(&sc->sc_timeout);
                /* signal upper layer */
                printf("%s: session 0x%x terminated, received PADT\n", sc->sc_sppp.pp_if.if_xname, session);
                /* clean up softc */
                sc->sc_state = PPPOE_STATE_INITIAL;
                memcpy(&sc->sc_dest, etherbroadcastaddr, sizeof(sc->sc_dest));
                if (sc->sc_ac_cookie) {
                        free(sc->sc_ac_cookie, M_MBUF);
                        sc->sc_ac_cookie = NULL;
                }
                sc->sc_ac_cookie_len = 0;
                sc->sc_session = 0;
                sc->sc_sppp.pp_down(&sc->sc_sppp);
                break;
        default:
                printf("%s: unknown code (0x%04x) session = 0x%04x\n",
                    sc? sc->sc_sppp.pp_if.if_xname : "pppoe",
                    code, session);
                break;
        }
}

static void
pppoe_disc_input(struct mbuf *m)
{
        u_int8_t *p;
        struct ether_header *eh;

        eh = mtod(m, struct ether_header*);
        m_adj(m, sizeof(struct ether_header));
        p = mtod(m, u_int8_t*);
        KASSERT(m->m_flags & M_PKTHDR);
        pppoe_dispatch_disc_pkt(p, m->m_len, m->m_pkthdr.rcvif, eh);
        m_free(m);
}

static void
pppoe_data_input(struct mbuf *m)
{
        u_int8_t *p, vertype;
        u_int16_t session, plen, code;
        struct pppoe_softc *sc;

        KASSERT(m->m_flags & M_PKTHDR);

        m_adj(m, sizeof(struct ether_header));
        if (m->m_pkthdr.len <= PPPOE_HEADERLEN) {
                printf("pppoe (data): dropping too short packet: %ld bytes\n", (long)m->m_pkthdr.len);
                goto drop;
        }

        p = mtod(m, u_int8_t*);

        vertype = *p++;
        if (vertype != PPPOE_VERTYPE) {
                printf("pppoe (data): unknown version/type packet: 0x%x\n", vertype);
                goto drop;
        }

        code = *p++;
        if (code != 0)
                goto drop;

        PPPOE_READ_16(p, session);
        sc = pppoe_find_softc_by_session(session, m->m_pkthdr.rcvif);
        if (sc == NULL)
                goto drop;

        PPPOE_READ_16(p, plen);

#if NBPFILTER > 0
        if(sc->sc_sppp.pp_if.if_bpf)
                bpf_mtap(sc->sc_sppp.pp_if.if_bpf, m);
#endif

        m_adj(m, PPPOE_HEADERLEN);

#ifdef PPPOE_DEBUG
        {
                struct mbuf *p;

                printf("%s: pkthdr.len=%d, pppoe.len=%d",
                        sc->sc_sppp.pp_if.if_xname,
                        m->m_pkthdr.len, plen);
                p = m;
                while (p) {
                        printf(" l=%d", p->m_len);
                        p = p->m_next;
                }
                printf("\n");
        }
#endif
        
        if (m->m_pkthdr.len < plen)
                goto drop;

        /* fix incoming interface pointer (not the raw ethernet interface anymore) */
        m->m_pkthdr.rcvif = &sc->sc_sppp.pp_if;

        /* pass packet up */
        sppp_input(&sc->sc_sppp.pp_if, m);
        return;

drop:
        m_free(m);
}

static int
pppoe_output(struct pppoe_softc *sc, struct mbuf *m)
{
        struct sockaddr dst;
        struct ether_header *eh;
        u_int16_t etype;

        if (sc->sc_eth_if == NULL)
                return EIO;

        memset(&dst, 0, sizeof dst);
        dst.sa_family = AF_UNSPEC;
        eh = (struct ether_header*)&dst.sa_data;
        etype = sc->sc_state == PPPOE_STATE_SESSION? ETHERTYPE_PPPOE : ETHERTYPE_PPPOEDISC;
        eh->ether_type = htons(etype);
        memcpy(&eh->ether_dhost, &sc->sc_dest, sizeof sc->sc_dest);

#ifdef PPPOE_DEBUG
        printf("%s (%x) state=%d, session=0x%x output -> %s, len=%d\n",
            sc->sc_sppp.pp_if.if_xname, etype,
            sc->sc_state, sc->sc_session,
	    ether_sprintf((const unsigned char *)&sc->sc_dest), m->m_pkthdr.len);
#endif

	return sc->sc_eth_if->if_output(sc->sc_eth_if, m, &dst, NULL);
}

static int
pppoe_ioctl(struct ifnet *ifp, unsigned long cmd, caddr_t data)
{
	struct proc *p = curproc;	/* XXX */
	struct pppoe_softc *sc = (struct pppoe_softc*)ifp;
	int error = 0;

	switch (cmd) {
	case PPPOESETPARMS:
	{
		struct pppoediscparms *parms = (struct pppoediscparms*)data;
		if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
			return error;
		if (parms->eth_ifname[0] != 0) {
			sc->sc_eth_if = ifunit(parms->eth_ifname);
			if (sc->sc_eth_if == NULL)
				return ENXIO;
		}
		if (parms->ac_name) {
			size_t s;
			char * p = malloc(parms->ac_name_len + 1, M_DEVBUF, M_WAITOK);
			copyinstr(parms->ac_name, p, parms->ac_name_len, &s);
			if (sc->sc_concentrator_name)
				free(sc->sc_concentrator_name, M_DEVBUF);
			sc->sc_concentrator_name = p;
		}
		if (parms->service_name) {
			size_t s;
			char * p = malloc(parms->service_name_len + 1, M_DEVBUF, M_WAITOK);
			copyinstr(parms->service_name, p, parms->service_name_len, &s);
			if (sc->sc_service_name)
				free(sc->sc_service_name, M_DEVBUF);
			sc->sc_service_name = p;
		}
		return 0;
	}
	break;
	case PPPOEGETPARMS:
	{
		struct pppoediscparms *parms = (struct pppoediscparms*)data;
		memset(parms, 0, sizeof *parms);
		if (sc->sc_eth_if)
			strncpy(parms->ifname, sc->sc_eth_if->if_xname, IFNAMSIZ);
		return 0;
	}
	break;
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

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return NULL;
	if (len+sizeof(struct ether_header) > MHLEN) {
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			struct mbuf *n;
			MFREE(m, n);
			return 0;
		}
	}
	m->m_data += sizeof(struct ether_header);
	m->m_len = len;
	m->m_pkthdr.len = len;
	m->m_pkthdr.rcvif = NULL;

	return m;
}

static int
pppoe_send_padi(struct pppoe_softc *sc)
{
	struct mbuf *m0;
	int len, l1, l2;
	u_int8_t *p;

	if (sc->sc_state >PPPOE_STATE_PADI_SENT)
		panic("pppoe_send_padi in state %d", sc->sc_state);

	/* calculate length of frame (excluding ethernet header + pppoe header) */
	len = 2+2+2+2+sizeof sc;	/* service name tag is required, host unique is send too */
	if (sc->sc_service_name != NULL) {
		l1 = strlen(sc->sc_service_name);
		len += l1;
	}
	if (sc->sc_concentrator_name != NULL) {
		l2 = strlen(sc->sc_concentrator_name);
		len += 2+2+l2;
	}

	/* allocate a buffer */
	m0 = pppoe_get_mbuf(len+PPPOE_HEADERLEN);	/* header len + payload len */
	if (!m0) return ENOBUFS;

	/* fill in pkt */
	p = mtod(m0, u_int8_t*);
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
	PPPOE_ADD_16(p, sizeof(sc));
	memcpy(p, &sc, sizeof sc);

#ifdef PPPOE_DEBUG
	p += sizeof sc;
	if (p - mtod(m0, u_int8_t*) != len + PPPOE_HEADERLEN)
		panic("pppoe_send_padi: garbled output len, should be %ld, is %ld",
		    (long)(len + PPPOE_HEADERLEN), (long)(p - mtod(m0, u_int8_t*)));
#endif

	/* send pkt */
	return pppoe_output(sc, m0);
}

static void
pppoe_timeout(void *arg)
{
	int x;
	struct pppoe_softc *sc = (struct pppoe_softc*)arg;

#ifdef PPPOE_DEBUG
	printf("%s: timeout\n", sc->sc_sppp.pp_if.if_xname);
#endif

	switch (sc->sc_state) {
	case PPPOE_STATE_PADI_SENT:
		x = splnet();
		sc->sc_padi_retried++;
		if (sc->sc_padi_retried >= PPPOE_DISC_MAXPADI) {
			pppoe_abort_connect(sc);
			splx(x);
			return;
		}
		if (pppoe_send_padi(sc) == 0)
			callout_reset(&sc->sc_timeout, PPPOE_DISC_TIMEOUT*(1+sc->sc_padi_retried), pppoe_timeout, sc);
		else
			pppoe_abort_connect(sc);
		splx(x);
		break;

	case PPPOE_STATE_PADR_SENT:
		x = splnet();
		sc->sc_padr_retried++;
		if (sc->sc_padr_retried >= PPPOE_DISC_MAXPADR) {
			memcpy(&sc->sc_dest, etherbroadcastaddr, sizeof(sc->sc_dest));
			sc->sc_state = PPPOE_STATE_PADI_SENT;
			sc->sc_padr_retried = 0;
			if (pppoe_send_padi(sc) == 0)
				callout_reset(&sc->sc_timeout, PPPOE_DISC_TIMEOUT*(1+sc->sc_padi_retried), pppoe_timeout, sc);
			else
				pppoe_abort_connect(sc);
			splx(x);
			return;
		}
		if (pppoe_send_padr(sc) == 0) 
			callout_reset(&sc->sc_timeout, PPPOE_DISC_TIMEOUT*(1+sc->sc_padr_retried), pppoe_timeout, sc);
		else
			pppoe_abort_connect(sc);
		splx(x);
		break;
	case PPPOE_STATE_CLOSING:
		x = splnet();
		pppoe_disconnect(sc);
		splx(x);
		break;
	default:
		return;	/* all done, work in peace */
	}
}

/* Start a connection (i.e. initiate discovery phase) */
static int
pppoe_connect(struct pppoe_softc *sc)
{
	int x, err;

	if (sc->sc_state != PPPOE_STATE_INITIAL)
		return EBUSY;

	x = splnet();
	sc->sc_state = PPPOE_STATE_PADI_SENT;
	sc->sc_padr_retried = 0;
	err = pppoe_send_padi(sc);
	if (err == 0)
		callout_reset(&sc->sc_timeout, PPPOE_DISC_TIMEOUT, pppoe_timeout, sc);
	else
		pppoe_abort_connect(sc);
	splx(x);
	return err;
}

/* disconnect */
static int
pppoe_disconnect(struct pppoe_softc *sc)
{
	int err, x;

	x = splnet();

	if (sc->sc_state < PPPOE_STATE_SESSION)
		err = EBUSY;
	else {
		printf("%s: disconnecting\n", sc->sc_sppp.pp_if.if_xname);
		err = pppoe_send_padt(sc);
	}

	/* cleanup softc */
	sc->sc_state = PPPOE_STATE_INITIAL;
	memcpy(&sc->sc_dest, etherbroadcastaddr, sizeof(sc->sc_dest));
	if (sc->sc_ac_cookie) {
		free(sc->sc_ac_cookie, M_MBUF);
		sc->sc_ac_cookie = NULL;
	}
	sc->sc_ac_cookie_len = 0;
	sc->sc_session = 0;

	/* notify upper layer */
	sc->sc_sppp.pp_down(&sc->sc_sppp);

	splx(x);

	return err;
}

/* Connection attempt aborted */
static void
pppoe_abort_connect(struct pppoe_softc *sc)
{
	printf("%s: could not establish connection\n",
		sc->sc_sppp.pp_if.if_xname);
	sc->sc_state = PPPOE_STATE_INITIAL;
	memcpy(&sc->sc_dest, etherbroadcastaddr, sizeof(sc->sc_dest));

	/* notify upper layer */
	sc->sc_sppp.pp_down(&sc->sc_sppp);
}

/* Send a PADR packet */
static int
pppoe_send_padr(struct pppoe_softc *sc)
{
	struct mbuf *m0;
	u_int8_t *p;
	size_t len, l1;

	if (sc->sc_state != PPPOE_STATE_PADR_SENT)
		return EIO;

	len = 2+2+2+2+sizeof(sc);			/* service name, host unique */
	if (sc->sc_service_name != NULL) {		/* service name tag maybe empty */
		l1 = strlen(sc->sc_service_name);
		len += l1;
	}
	if (sc->sc_ac_cookie_len > 0)
		len += 2+2+sc->sc_ac_cookie_len;	/* AC cookie */
	m0 = pppoe_get_mbuf(len+PPPOE_HEADERLEN);
	if (!m0) return ENOBUFS;
	p = mtod(m0, u_int8_t*);
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
	PPPOE_ADD_16(p, PPPOE_TAG_HUNIQUE);
	PPPOE_ADD_16(p, sizeof(sc));
	memcpy(p, &sc, sizeof sc);

#ifdef PPPOE_DEBUG
	p += sizeof sc;
	if (p - mtod(m0, u_int8_t*) != len + PPPOE_HEADERLEN)
		panic("pppoe_send_padr: garbled output len, should be %ld, is %ld",
			(long)(len + PPPOE_HEADERLEN), (long)(p - mtod(m0, u_int8_t*)));
#endif

	return pppoe_output(sc, m0);
}

/* send a PADT packet */
static int
pppoe_send_padt(struct pppoe_softc *sc)
{
	struct mbuf *m0;
	u_int8_t *p;

	if (sc->sc_state < PPPOE_STATE_SESSION)
		return EIO;

#ifdef PPPOE_DEBUG
	printf("%s: sending PADT\n", sc->sc_sppp.pp_if.if_xname);
#endif
	m0 = pppoe_get_mbuf(PPPOE_HEADERLEN);
	if (!m0) return ENOBUFS;
	p = mtod(m0, u_int8_t*);
	PPPOE_ADD_HEADER(p, PPPOE_CODE_PADT, sc->sc_session, 0);
	return pppoe_output(sc, m0);
}

static void
pppoe_tls(struct sppp *sp)
{
	struct pppoe_softc *sc = (void*)sp;
	if (sc->sc_state != PPPOE_STATE_INITIAL)
		return;
	pppoe_connect(sc);
}

static void
pppoe_tlf(struct sppp *sp)
{
	struct pppoe_softc *sc = (void*)sp;
	if (sc->sc_state < PPPOE_STATE_SESSION)
		return;
	/*
	 * Do not call pppoe_disconnect here, the upper layer state
	 * machine gets confused by this. We must return from this
	 * function and defer disconnecting to the timeout handler.
	 */
	sc->sc_state = PPPOE_STATE_CLOSING;
	callout_reset(&sc->sc_timeout, hz/100, pppoe_timeout, sc);
}

static void
pppoe_start(struct ifnet *ifp)
{
	struct pppoe_softc *sc = (void*)ifp;
	struct mbuf *m;
	u_int8_t *p;
	size_t len;

	if (sppp_isempty(ifp))
		return;

	/* are we read to proccess data yet? */
	if (sc->sc_state < PPPOE_STATE_SESSION) {
		sppp_flush(&sc->sc_sppp.pp_if);
		return;
	}

	while ((m = sppp_dequeue(ifp)) != NULL) {
		len = m->m_pkthdr.len;
		M_PREPEND(m, PPPOE_HEADERLEN, M_DONTWAIT);
		if (m == NULL) {
			m_free(m);
			break;
		}
		p = mtod(m, u_int8_t*);
		PPPOE_ADD_HEADER(p, 0, sc->sc_session, len);

#if NBPFILTER > 0
		if(sc->sc_sppp.pp_if.if_bpf)
			bpf_mtap(sc->sc_sppp.pp_if.if_bpf, m);
#endif

		pppoe_output(sc, m);
	}
}

#endif	/* NPPPOE > 0 */
