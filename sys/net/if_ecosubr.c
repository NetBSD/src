/*	$NetBSD: if_ecosubr.c,v 1.1 2001/09/10 23:11:06 bjh21 Exp $	*/

/*-
 * Copyright (c) 2001 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "bpfilter.h"

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: if_ecosubr.c,v 1.1 2001/09/10 23:11:06 bjh21 Exp $");

#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/syslog.h>
#include <sys/systm.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_eco.h>
#include <net/if_types.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

/* Default broadcast address */
static const u_int8_t eco_broadcastaddr[] = { 0xff, 0xff };

static int eco_output(struct ifnet *, struct mbuf *, struct sockaddr *,
    struct rtentry *);
static void eco_input(struct ifnet *, struct mbuf *);
static void eco_start(struct ifnet *);
static int eco_ioctl(struct ifnet *, u_long, caddr_t);

static int eco_interestingp(struct ifnet *ifp, struct mbuf *m);
static struct mbuf *eco_immediate(struct ifnet *ifp, struct mbuf *m);
static struct mbuf *eco_ack(struct ifnet *ifp, struct mbuf *m);

void
eco_ifattach(struct ifnet *ifp, const u_int8_t *lla)
{
	struct ecocom *ec = (void *)ifp;

	ifp->if_type = IFT_OTHER;
	ifp->if_addrlen = ECO_ADDR_LEN;
	ifp->if_hdrlen = ECO_HDR_LEN;
	ifp->if_dlt = DLT_ECONET;
	ifp->if_mtu = ECO_MTU;

	ifp->if_output	 = eco_output;
	ifp->if_input	 = eco_input;
	ifp->if_start	 = eco_start;
	ifp->if_ioctl	 = eco_ioctl;

/*	ifp->if_baudrate...; */
	if_alloc_sadl(ifp);
	memcpy(LLADDR(ifp->if_sadl), lla, ifp->if_addrlen);

	/* XXX cast safe? */
	ifp->if_broadcastaddr = (u_int8_t *)eco_broadcastaddr;
	ec->ec_state = ECO_IDLE;
#if NBPFILTER > 0
	bpfattach(ifp, ifp->if_dlt, ECO_HDR_LEN);
#endif
}

#define senderr(e) do {							\
	error = (e);							\
	goto bad;							\
} while (/*CONSTCOND*/0)

static int
eco_output(struct ifnet *ifp, struct mbuf *m0, struct sockaddr *dst,
    struct rtentry *rt0)
{
	struct eco_header ehdr, *eh;
	int error, s;
	struct mbuf *m = m0;
	int hdrcmplt;
	size_t len;
	ALTQ_DECL(struct altq_pktattr pktattr;)

	if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING))
		senderr(ENETDOWN);
	/*
	 * If the queueing discipline needs packet classification,
	 * do it before prepending link headers.
	 */
	IFQ_CLASSIFY(&ifp->if_snd, m, dst->sa_family, &pktattr);

	hdrcmplt = 0;
	switch (dst->sa_family) {
	case pseudo_AF_HDRCMPLT:
		hdrcmplt = 1;
		/* FALLTHROUGH */
	case AF_UNSPEC:
		eh = (struct eco_header *)dst->sa_data;
		ehdr = *eh;
		break;
	default:
		log(LOG_ERR, "%s: can't handle af%d\n", ifp->if_xname,
		    dst->sa_family);
		senderr(EAFNOSUPPORT);
	}

	/*
	 * Add local net header.  If no space in first mbuf,
	 * allocate another.
	 */
	M_PREPEND(m, sizeof (struct eco_header), M_DONTWAIT);
	if (m == 0)
		senderr(ENOBUFS);
	eh = mtod(m, struct eco_header *);
	*eh = ehdr;
	if (!hdrcmplt)
		memcpy(eh->eco_shost, LLADDR(ifp->if_sadl),
		    ECO_ADDR_LEN);

#ifdef PFIL_HOOKS
	if ((error = pfil_run_hooks(&ifp->if_pfil, &m, ifp, PFIL_OUT)) != 0)
		return (error);
	if (m == NULL)
		return (0);
#endif
	
	/*
	 * Queue message on interface, and start output if interface
	 * not yet active.
	 */

	len = m->m_pkthdr.len;
	s = splnet();
	IFQ_ENQUEUE(&ifp->if_snd, m, &pktattr, error);
	if (error) {
		splx(s);
		return (error);
	}
	ifp->if_obytes += len;
	if ((ifp->if_flags & IFF_OACTIVE) == 0)
		(*ifp->if_start)(ifp);
	splx(s);
	return (error);

bad:
	if (m)
		m_freem(m);
	return error;
}

/*
 * Given a scout, decide if we want the rest of the packet.
 */
static int
eco_interestingp(struct ifnet *ifp, struct mbuf *m)
{

	/* Until we get some higher-level protocols, we don't care. */
	return 0;
}

static void
eco_input(struct ifnet *ifp, struct mbuf *m)
{

	/* We should really _do_ something with this packet. */
	m_freem(m);
}

static void
eco_start(struct ifnet *ifp)
{
	struct ecocom *ec = (void *)ifp;
	struct mbuf *m;
	struct eco_header *eh;

	for (;;) {
		IFQ_DEQUEUE(&ifp->if_snd, m);
		if (!m) break;
		if (ec->ec_claimwire(ifp) == 0) {
			eh = mtod(m, struct eco_header *);
			if (eh->eco_port == ECO_PORT_IMMEDIATE) {
				ec->ec_txframe(ifp, m);
				ec->ec_state = ECO_IMMED_SENT;
			} else if (eh->eco_dhost[0] == 255) {
				ec->ec_txframe(ifp, m);
			} else {
				ec->ec_packet = m;
				m = m_copym(m, 0, ECO_HDR_LEN, M_DONTWAIT);
				if (m == NULL) {
					m_freem(ec->ec_packet);
					return;
				}
				ec->ec_txframe(ifp, m);
				ec->ec_state = ECO_SCOUT_SENT;
			}
		} else {
			log(LOG_ERR, "%s: line jammed\n", ifp->if_xname);
			m_freem(m);
		}
	}
}

static int
eco_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct ifreq *ifr = (struct ifreq *)data;
	struct ifaddr *ifa = (struct ifaddr *)data;
	int error = 0;

	switch (cmd) {
	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		switch (ifa->ifa_addr->sa_family) {
		default:
			if ((ifp->if_flags & IFF_RUNNING) == 0)
				error = (*ifp->if_init)(ifp);
			break;
		}
		break;
	case SIOCSIFMTU:
		ifp->if_mtu = ifr->ifr_mtu;

		/* Make sure the device notices the MTU change. */
		if (ifp->if_flags & IFF_UP)
			error = (*ifp->if_init)(ifp);
		break;
	case SIOCSIFFLAGS:
		if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) == IFF_RUNNING) {
			/*
			 * If interface is marked down and it is running,
			 * then stop and disable it.
			 */
			(*ifp->if_stop)(ifp, 1);
		} else if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) == IFF_UP) {
			/*
			 * If interface is marked up and it is stopped, then
			 * start it.
			 */
			error = (*ifp->if_init)(ifp);
		} else if ((ifp->if_flags & IFF_UP) != 0) {
			/*
			 * Reset the interface to pick up changes in any other
			 * flags that affect the hardware state.
			 */
			error = (*ifp->if_init)(ifp);
		}
		break;
	default:
		error = ENOTTY;
	}

	return error;
}

/*
 * Handle a raw Econet frame off the interface.  The interface may be
 * flag-filling for a response.
 *
 * May be called from IPL_NET or IPL_SOFTNET.
 */

struct mbuf *
eco_inputframe(struct ifnet *ifp, struct mbuf *m)
{
	struct ecocom *ec = (void *)ifp;
	struct eco_header *eh;
	struct mbuf *m0;
	struct mbuf *reply;
	int len;

	eh = mtod(m, struct eco_header *);
	switch (ec->ec_state) {
	case ECO_IDLE:
		if (memcmp(eh->eco_dhost, eco_broadcastaddr,
		    ECO_ADDR_LEN) == 0) {
			/* Broadcast */
			/* XXX eco_input(ifp, m) */
			m_freem(m);
		} else if (memcmp(eh->eco_dhost, LLADDR(ifp->if_sadl),
		    ECO_ADDR_LEN) == 0) {
			/* Unicast for us */
			if (eh->eco_port == ECO_PORT_IMMEDIATE)
				return eco_immediate(ifp, m);
			else {
				if (eco_interestingp(ifp, m)) {
					reply = eco_ack(ifp, m);
					if (reply == NULL) {
						m_freem(m);
						return NULL;
					}
					ec->ec_state = ECO_SCOUT_RCVD;
					ec->ec_scout = m;
					return reply;
				} else {
					m_freem(m);
					return NULL;
				}
			}
		} else
			/* Not for us.  Throw it away. */
			m_freem(m);
		break;
	case ECO_SCOUT_RCVD:
		KASSERT(ec->ec_scout != NULL);
		reply = eco_ack(ifp, m);
		/* XXX should check that headers match. */
		/*
		 * Chop off the small header from this frame, and put
		 * the scout (which holds the control byte and port)
		 * in its place.
		 */
		m0 = ec->ec_scout;
		ec->ec_scout = NULL;
		m_adj(m, ECO_SHDR_LEN);
		len = m0->m_pkthdr.len + m->m_pkthdr.len;
		m_cat(m0, m);
		m0->m_pkthdr.len = len;
		ec->ec_state = ECO_DONE;
		eco_input(ifp, m0);
		return reply;
	case ECO_SCOUT_SENT:
		KASSERT(ec->ec_packet != NULL);
		/* XXX should check ack fields for correctness */
		m_freem(m);
		/* Chop out the control and port bytes. */
		m0 = m_copym(ec->ec_packet, 0, ECO_SHDR_LEN, M_DONTWAIT);
		if (m0 == NULL) {
			m_freem(ec->ec_packet);
			return NULL;
		}
		m = m_copypacket(ec->ec_packet, M_DONTWAIT);
		if (m == NULL) {
			m_freem(m0);
			m_freem(ec->ec_packet);
			return NULL;
		}
		m_adj(m, ECO_HDR_LEN);
		len = m0->m_pkthdr.len + m->m_pkthdr.len;
		m_cat(m0, m); /* Doesn't update packet header */
		m0->m_pkthdr.len = len;
		ec->ec_state = ECO_DATA_SENT;
		return m0;
	case ECO_DATA_SENT:
		KASSERT(ec->ec_packet != NULL);
		/* XXX check ack fields? */
		m_freem(m);
		m_freem(ec->ec_packet);
		ec->ec_packet = NULL;
		ec->ec_state = ECO_DONE;
		return NULL;
	default:
		m_freem(m);
		break;
	}
	return NULL;
}

/*
 * Handle an immediate operation, and return the reply, or NULL not to reply.
 * Frees the incoming mbuf.
 */

static struct mbuf *
eco_immediate(struct ifnet *ifp, struct mbuf *m)
{
	struct eco_header *eh, *reh;
	struct mbuf *n;
	static const u_int8_t machinepeek_data[] = { 42, 0, 0, 1 };

	eh = mtod(m, struct eco_header *);
	switch (eh->eco_control) {
	case ECO_CTL_MACHINEPEEK:
		MGETHDR(n, M_DONTWAIT, MT_DATA);
		if (n == NULL)
			goto bad;
		n->m_len = n->m_pkthdr.len = ECO_SHDR_LEN + 4;
		reh = mtod(n, struct eco_header *);
		memcpy(reh->eco_dhost, eh->eco_shost,
		    ECO_ADDR_LEN);
		memcpy(reh->eco_shost, LLADDR(ifp->if_sadl),
		    ECO_ADDR_LEN);
		memcpy(mtod(n, caddr_t) + ECO_SHDR_LEN, machinepeek_data,
		    sizeof(machinepeek_data));
		m_freem(m);
		return n;
	default:
	bad:
		m_freem(m);
		return NULL;
	}
}

/*
 * Generate (and return) an acknowledgement for a frame.  Doesn't free the
 * original frame, since it's probably needed elsewhere.
 */
static struct mbuf *
eco_ack(struct ifnet *ifp, struct mbuf *m)
{
	struct eco_header *eh, *reh;
	struct mbuf *n;

	eh = mtod(m, struct eco_header *);
	MGETHDR(n, M_DONTWAIT, MT_DATA);
	if (n == NULL)
		return NULL;
	n->m_len = n->m_pkthdr.len = ECO_SHDR_LEN;
	reh = mtod(n, struct eco_header *);
	memcpy(reh->eco_dhost, eh->eco_shost, ECO_ADDR_LEN);
	memcpy(reh->eco_shost, LLADDR(ifp->if_sadl), ECO_ADDR_LEN);
	return n;
}

void
eco_inputidle(struct ifnet *ifp)
{
	struct ecocom *ec = (void *)ifp;

	ec->ec_state = ECO_IDLE;
	m_freem(ec->ec_scout);
	ec->ec_scout = NULL;
}

/*
 * Convert Econet address to printable (loggable) representation.
 */
char *
eco_sprintf(const u_int8_t *ea)
{
	static char buf[8];

	if (ea[1] == 0)
		sprintf(buf, "%d", ea[0]);
	else
		sprintf(buf, "%d.%d", ea[1], ea[0]);
	return buf;
}
		    
