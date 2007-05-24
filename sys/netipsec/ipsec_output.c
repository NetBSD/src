/*	$NetBSD: ipsec_output.c,v 1.17.2.1 2007/05/24 19:13:12 pavel Exp $	*/

/*-
 * Copyright (c) 2002, 2003 Sam Leffler, Errno Consulting
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
 * $FreeBSD: /repoman/r/ncvs/src/sys/netipsec/ipsec_output.c,v 1.3.2.2 2003/03/28 20:32:53 sam Exp $
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ipsec_output.c,v 1.17.2.1 2007/05/24 19:13:12 pavel Exp $");

/*
 * IPsec output processing.
 */
#include "opt_inet.h"
#ifdef __FreeBSD__
#include "opt_inet6.h"
#endif
#include "opt_ipsec.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/syslog.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/in_var.h>
#include <netinet/ip_ecn.h>
#ifdef INET6
#  ifdef __FreeBSD__
#  include <netinet6/ip6_ecn.h>
#  endif
#endif

#include <netinet/ip6.h>
#ifdef INET6
#include <netinet6/ip6_var.h>
#endif
#include <netinet/in_pcb.h>
#ifdef INET6
#include <netinet/icmp6.h>
#endif

#include <netipsec/ipsec.h>
#include <netipsec/ipsec_var.h>
#ifdef INET6
#include <netipsec/ipsec6.h>
#endif
#include <netipsec/ah_var.h>
#include <netipsec/esp_var.h>
#include <netipsec/ipcomp_var.h>

#include <netipsec/xform.h>

#include <netipsec/key.h>
#include <netipsec/keydb.h>
#include <netipsec/key_debug.h>
#include <netipsec/ipsec_osdep.h>

#include <net/net_osdep.h>		/* ovbcopy() in ipsec6_encapsulate() */

int
ipsec_process_done(struct mbuf *m, struct ipsecrequest *isr)
{
	struct tdb_ident *tdbi;
	struct m_tag *mtag;
	struct secasvar *sav;
	struct secasindex *saidx;
	int error;

	IPSEC_SPLASSERT_SOFTNET("ipsec_process_done");

	IPSEC_ASSERT(m != NULL, ("ipsec_process_done: null mbuf"));
	IPSEC_ASSERT(isr != NULL, ("ipsec_process_done: null ISR"));
	sav = isr->sav;
	IPSEC_ASSERT(sav != NULL, ("ipsec_process_done: null SA"));
	IPSEC_ASSERT(sav->sah != NULL, ("ipsec_process_done: null SAH"));

	saidx = &sav->sah->saidx;
	switch (saidx->dst.sa.sa_family) {
#ifdef INET
	case AF_INET:
		/* Fix the header length, for AH processing. */
		mtod(m, struct ip *)->ip_len = htons(m->m_pkthdr.len);
		break;
#endif /* INET */
#ifdef INET6
	case AF_INET6:
		/* Fix the header length, for AH processing. */
		if (m->m_pkthdr.len < sizeof (struct ip6_hdr)) {
			error = ENXIO;
			goto bad;
		}
		if (m->m_pkthdr.len - sizeof (struct ip6_hdr) > IPV6_MAXPACKET) {
			/* No jumbogram support. */
			error = ENXIO;	/*?*/
			goto bad;
		}
		mtod(m, struct ip6_hdr *)->ip6_plen =
			htons(m->m_pkthdr.len - sizeof(struct ip6_hdr));
		break;
#endif /* INET6 */
	default:
		DPRINTF(("ipsec_process_done: unknown protocol family %u\n",
		    saidx->dst.sa.sa_family));
		error = ENXIO;
		goto bad;
	}

	/*
	 * Add a record of what we've done or what needs to be done to the
	 * packet.
	 */
	mtag = m_tag_get(PACKET_TAG_IPSEC_OUT_DONE,
			sizeof(struct tdb_ident), M_NOWAIT);
	if (mtag == NULL) {
		DPRINTF(("ipsec_process_done: could not get packet tag\n"));
		error = ENOMEM;
		goto bad;
	}

	tdbi = (struct tdb_ident *)(mtag + 1);
	tdbi->dst = saidx->dst;
	tdbi->proto = saidx->proto;
	tdbi->spi = sav->spi;
	m_tag_prepend(m, mtag);

	/*
	 * If there's another (bundled) SA to apply, do so.
	 * Note that this puts a burden on the kernel stack size.
	 * If this is a problem we'll need to introduce a queue
	 * to set the packet on so we can unwind the stack before
	 * doing further processing.
	 */
	if (isr->next) {
		newipsecstat.ips_out_bundlesa++;
        switch ( saidx->dst.sa.sa_family ) {
#ifdef INET
        case AF_INET:
			return ipsec4_process_packet(m, isr->next, 0,0);
#endif /* INET */
#ifdef INET6
		case AF_INET6:
        	return ipsec6_process_packet(m,isr->next);
#endif /* INET6 */
		default :
			DPRINTF(("ipsec_process_done: unknown protocol family %u\n",
                               saidx->dst.sa.sa_family));
			error = ENXIO;
			goto bad;
        }
	}

	/*
	 * We're done with IPsec processing, transmit the packet using the
	 * appropriate network protocol (IP or IPv6). SPD lookup will be
	 * performed again there.
	 */
	switch (saidx->dst.sa.sa_family) {
#ifdef INET
	struct ip *ip;
	case AF_INET:
		ip = mtod(m, struct ip *);
#ifdef __FreeBSD__
		/* FreeBSD ip_output() expects ip_len, ip_off in host endian */
		ip->ip_len = ntohs(ip->ip_len);
		ip->ip_off = ntohs(ip->ip_off);
#endif /* __FreeBSD_ */
		return ip_output(m, NULL, NULL, IP_RAWOUTPUT,
		    (struct ip_moptions *)NULL, (struct socket *)NULL);

#endif /* INET */
#ifdef INET6
	case AF_INET6:
		/*
		 * We don't need massage, IPv6 header fields are always in
		 * net endian.
		 */
		return ip6_output(m, NULL, NULL, 0, NULL, NULL, NULL);
#endif /* INET6 */
	}
	panic("ipsec_process_done");
bad:
	m_freem(m);
	KEY_FREESAV(&sav);
	return (error);
}

static struct ipsecrequest *
ipsec_nextisr(
	struct mbuf *m,
	struct ipsecrequest *isr,
	int af,
	struct secasindex *saidx,
	int *error
)
{
#define IPSEC_OSTAT(x,y,z) (isr->saidx.proto == IPPROTO_ESP ? (x)++ : \
			    isr->saidx.proto == IPPROTO_AH ? (y)++ : (z)++)
	struct secasvar *sav;

	IPSEC_SPLASSERT_SOFTNET("ipsec_nextisr");
	IPSEC_ASSERT(af == AF_INET || af == AF_INET6,
		("ipsec_nextisr: invalid address family %u", af));
again:
	/*
	 * Craft SA index to search for proper SA.  Note that
	 * we only fillin unspecified SA peers for transport
	 * mode; for tunnel mode they must already be filled in.
	 */
	*saidx = isr->saidx;
	if (isr->saidx.mode == IPSEC_MODE_TRANSPORT) {
		/* Fillin unspecified SA peers only for transport mode */
		if (af == AF_INET) {
			struct sockaddr_in *sin;
			struct ip *ip = mtod(m, struct ip *);

			if (saidx->src.sa.sa_len == 0) {
				sin = &saidx->src.sin;
				sin->sin_len = sizeof(*sin);
				sin->sin_family = AF_INET;
				sin->sin_port = IPSEC_PORT_ANY;
				sin->sin_addr = ip->ip_src;
			}
			if (saidx->dst.sa.sa_len == 0) {
				sin = &saidx->dst.sin;
				sin->sin_len = sizeof(*sin);
				sin->sin_family = AF_INET;
				sin->sin_port = IPSEC_PORT_ANY;
				sin->sin_addr = ip->ip_dst;
			}
		} else {
			struct sockaddr_in6 *sin6;
			struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);

			if (saidx->src.sin6.sin6_len == 0) {
				sin6 = (struct sockaddr_in6 *)&saidx->src;
				sin6->sin6_len = sizeof(*sin6);
				sin6->sin6_family = AF_INET6;
				sin6->sin6_port = IPSEC_PORT_ANY;
				sin6->sin6_addr = ip6->ip6_src;
				if (IN6_IS_SCOPE_LINKLOCAL(&ip6->ip6_src)) {
					/* fix scope id for comparing SPD */
					sin6->sin6_addr.s6_addr16[1] = 0;
					sin6->sin6_scope_id =
					    ntohs(ip6->ip6_src.s6_addr16[1]);
				}
			}
			if (saidx->dst.sin6.sin6_len == 0) {
				sin6 = (struct sockaddr_in6 *)&saidx->dst;
				sin6->sin6_len = sizeof(*sin6);
				sin6->sin6_family = AF_INET6;
				sin6->sin6_port = IPSEC_PORT_ANY;
				sin6->sin6_addr = ip6->ip6_dst;
				if (IN6_IS_SCOPE_LINKLOCAL(&ip6->ip6_dst)) {
					/* fix scope id for comparing SPD */
					sin6->sin6_addr.s6_addr16[1] = 0;
					sin6->sin6_scope_id =
					    ntohs(ip6->ip6_dst.s6_addr16[1]);
				}
			}
		}
	}

	/*
	 * Lookup SA and validate it.
	 */
	*error = key_checkrequest(isr, saidx);
	if (*error != 0) {
		/*
		 * IPsec processing is required, but no SA found.
		 * I assume that key_acquire() had been called
		 * to get/establish the SA. Here I discard
		 * this packet because it is responsibility for
		 * upper layer to retransmit the packet.
		 */
		newipsecstat.ips_out_nosa++;
		goto bad;
	}
	sav = isr->sav;
	if (sav == NULL) {		/* XXX valid return */
		IPSEC_ASSERT(ipsec_get_reqlevel(isr) == IPSEC_LEVEL_USE,
			("ipsec_nextisr: no SA found, but required; level %u",
			ipsec_get_reqlevel(isr)));
		isr = isr->next;
		if (isr == NULL) {
			/*XXXstatistic??*/
			*error = EINVAL;		/*XXX*/
			return isr;
		}
		goto again;
	}

	/*
	 * Check system global policy controls.
	 */
	if ((isr->saidx.proto == IPPROTO_ESP && !esp_enable) ||
	    (isr->saidx.proto == IPPROTO_AH && !ah_enable) ||
	    (isr->saidx.proto == IPPROTO_IPCOMP && !ipcomp_enable)) {
		DPRINTF(("ipsec_nextisr: IPsec outbound packet dropped due"
			" to policy (check your sysctls)\n"));
		IPSEC_OSTAT(espstat.esps_pdrops, ahstat.ahs_pdrops,
		    ipcompstat.ipcomps_pdrops);
		*error = EHOSTUNREACH;
		goto bad;
	}

	/*
	 * Sanity check the SA contents for the caller
	 * before they invoke the xform output method.
	 */
	if (sav->tdb_xform == NULL) {
		DPRINTF(("ipsec_nextisr: no transform for SA\n"));
		IPSEC_OSTAT(espstat.esps_noxform, ahstat.ahs_noxform,
		    ipcompstat.ipcomps_noxform);
		*error = EHOSTUNREACH;
		goto bad;
	}
	return isr;
bad:
	IPSEC_ASSERT(*error != 0, ("ipsec_nextisr: error return w/ no error code"));
	return NULL;
#undef IPSEC_OSTAT
}

#ifdef INET
/*
 * IPsec output logic for IPv4.
 */
int
ipsec4_process_packet(
    struct mbuf *m,
    struct ipsecrequest *isr,
    int flags,
    int tunalready
)
{
	struct secasindex saidx;
	struct secasvar *sav;
	struct ip *ip;
	int s, error, i, off;

	IPSEC_ASSERT(m != NULL, ("ipsec4_process_packet: null mbuf"));
	IPSEC_ASSERT(isr != NULL, ("ipsec4_process_packet: null isr"));

	s = splsoftnet();			/* insure SA contents don't change */

	isr = ipsec_nextisr(m, isr, AF_INET, &saidx, &error);
	if (isr == NULL)
		goto bad;

	sav = isr->sav;
	if (!tunalready) {
		union sockaddr_union *dst = &sav->sah->saidx.dst;
		int setdf;

		/*
		 * Collect IP_DF state from the outer header.
		 */
		if (dst->sa.sa_family == AF_INET) {
			if (m->m_len < sizeof (struct ip) &&
			    (m = m_pullup(m, sizeof (struct ip))) == NULL) {
				error = ENOBUFS;
				goto bad;
			}
			ip = mtod(m, struct ip *);
			/* Honor system-wide control of how to handle IP_DF */
			switch (ip4_ipsec_dfbit) {
			case 0:			/* clear in outer header */
			case 1:			/* set in outer header */
				setdf = ip4_ipsec_dfbit;
				break;
			default:		/* propagate to outer header */
				setdf = ip->ip_off;
#ifndef __FreeBSD__
		/* On FreeBSD, ip_off and ip_len assumed in host endian. */
				setdf = ntohs(setdf);
#endif
				setdf = htons(setdf & IP_DF);
				break;
			}
		} else {
			ip = NULL;		/* keep compiler happy */
			setdf = 0;
		}
		/* Do the appropriate encapsulation, if necessary */
		if (isr->saidx.mode == IPSEC_MODE_TUNNEL || /* Tunnel requ'd */
		    dst->sa.sa_family != AF_INET ||	    /* PF mismatch */
#if 0
		    (sav->flags & SADB_X_SAFLAGS_TUNNEL) || /* Tunnel requ'd */
		    sav->tdb_xform->xf_type == XF_IP4 ||    /* ditto */
#endif
		    (dst->sa.sa_family == AF_INET &&	    /* Proxy */
		     dst->sin.sin_addr.s_addr != INADDR_ANY &&
		     dst->sin.sin_addr.s_addr != ip->ip_dst.s_addr)) {
			struct mbuf *mp;

			/* Fix IPv4 header checksum and length */
			if (m->m_len < sizeof (struct ip) &&
			    (m = m_pullup(m, sizeof (struct ip))) == NULL) {
				error = ENOBUFS;
				goto bad;
			}
			ip = mtod(m, struct ip *);
			ip->ip_len = htons(m->m_pkthdr.len);
			ip->ip_sum = 0;
#ifdef _IP_VHL
			if (ip->ip_vhl == IP_VHL_BORING)
				ip->ip_sum = in_cksum_hdr(ip);
			else
				ip->ip_sum = in_cksum(m,
					_IP_VHL_HL(ip->ip_vhl) << 2);
#else
			ip->ip_sum = in_cksum(m, ip->ip_hl << 2);
#endif

			/* Encapsulate the packet */
			error = ipip_output(m, isr, &mp, 0, 0);
			if (mp == NULL && !error) {
				/* Should never happen. */
				DPRINTF(("ipsec4_process_packet: ipip_output "
					"returns no mbuf and no error!"));
				error = EFAULT;
			}
			if (error) {
				if (mp) {
					/* XXX: Should never happen! */
					m_freem(mp);
				}
				m = NULL; /* ipip_output() already freed it */
				goto bad;
			}
			m = mp, mp = NULL;
			/*
			 * ipip_output clears IP_DF in the new header.  If
			 * we need to propagate IP_DF from the outer header,
			 * then we have to do it here.
			 *
			 * XXX shouldn't assume what ipip_output does.
			 */
			if (dst->sa.sa_family == AF_INET && setdf) {
				if (m->m_len < sizeof (struct ip) &&
				    (m = m_pullup(m, sizeof (struct ip))) == NULL) {
					error = ENOBUFS;
					goto bad;
				}
				ip = mtod(m, struct ip *);
				ip->ip_off = ntohs(ip->ip_off);
				ip->ip_off |= IP_DF;
				ip->ip_off = htons(ip->ip_off);
			}
		}
	}

	/*
	 * Dispatch to the appropriate IPsec transform logic.  The
	 * packet will be returned for transmission after crypto
	 * processing, etc. are completed.  For encapsulation we
	 * bypass this call because of the explicit call done above
	 * (necessary to deal with IP_DF handling for IPv4).
	 *
	 * NB: m & sav are ``passed to caller'' who's reponsible for
	 *     for reclaiming their resources.
	 */
	if (sav->tdb_xform->xf_type != XF_IP4) {
		ip = mtod(m, struct ip *);
		i = ip->ip_hl << 2;
		off = offsetof(struct ip, ip_p);
		error = (*sav->tdb_xform->xf_output)(m, isr, NULL, i, off);
	} else {
		error = ipsec_process_done(m, isr);
	}
	splx(s);
	return error;
bad:
	splx(s);
	if (m)
		m_freem(m);
	return error;
}
#endif

#ifdef INET6
int
ipsec6_process_packet(
	struct mbuf *m,
 	struct ipsecrequest *isr
    )
{
	struct secasindex saidx;
	struct secasvar *sav;
	struct ip6_hdr *ip6;
	int s, error, i, off; 

	IPSEC_ASSERT(m != NULL, ("ipsec6_process_packet: null mbuf"));
	IPSEC_ASSERT(isr != NULL, ("ipsec6_process_packet: null isr"));

	s = splsoftnet();   /* insure SA contents don't change */
	isr = ipsec_nextisr(m, isr, AF_INET6, &saidx, &error);
	if (isr == NULL) {
		// XXX Should we send a notification ?
		goto bad;
	}

	sav = isr->sav;
	if (sav->tdb_xform->xf_type != XF_IP4) {
		i = sizeof(struct ip6_hdr);
		off = offsetof(struct ip6_hdr, ip6_nxt);
		error = (*sav->tdb_xform->xf_output)(m, isr, NULL, i, off);
       } else {
		union sockaddr_union *dst = &sav->sah->saidx.dst;

        ip6 = mtod(m, struct ip6_hdr *);

		/* Do the appropriate encapsulation, if necessary */
		if (isr->saidx.mode == IPSEC_MODE_TUNNEL || /* Tunnel requ'd */
               dst->sa.sa_family != AF_INET6 ||        /* PF mismatch */
            ((dst->sa.sa_family == AF_INET6) &&
                (!IN6_IS_ADDR_UNSPECIFIED(&dst->sin6.sin6_addr)) &&
                (!IN6_ARE_ADDR_EQUAL(&dst->sin6.sin6_addr,
                &ip6->ip6_dst))) 
            )
		{
			struct mbuf *mp;
            /* Fix IPv6 header payload length. */
            if (m->m_len < sizeof(struct ip6_hdr))
                if ((m = m_pullup(m,sizeof(struct ip6_hdr))) == NULL)
                   return ENOBUFS;

            if (m->m_pkthdr.len - sizeof(*ip6) > IPV6_MAXPACKET) {
                /* No jumbogram support. */
                m_freem(m);
                return ENXIO;   /*XXX*/
            }
            ip6 = mtod(m, struct ip6_hdr *);
            ip6->ip6_plen = htons(m->m_pkthdr.len - sizeof(*ip6));

			/* Encapsulate the packet */
			error = ipip_output(m, isr, &mp, 0, 0);
			if (mp == NULL && !error) {
				/* Should never happen. */
				DPRINTF(("ipsec6_process_packet: ipip_output "
						 "returns no mbuf and no error!"));
				 error = EFAULT;
			}

			if (error) {
				if (mp) {
					/* XXX: Should never happen! */
					m_freem(mp);
				}
				m = NULL; /* ipip_output() already freed it */
				goto bad;
			}
    
			m = mp;
			mp = NULL;
		}
	
		error = ipsec_process_done(m,isr);
		}
		splx(s);
		return error;
bad:
	splx(s);
	if (m)
		m_freem(m);
	return error;
}
#endif /*INET6*/
