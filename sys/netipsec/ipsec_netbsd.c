/*	$NetBSD: ipsec_netbsd.c,v 1.2 2003/09/12 11:20:58 itojun Exp $	*/
/*	$KAME: esp_input.c,v 1.60 2001/09/04 08:43:19 itojun Exp $	*/
/*	$KAME: ah_input.c,v 1.64 2001/09/04 08:43:19 itojun Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ipsec_netbsd.c,v 1.2 2003/09/12 11:20:58 itojun Exp $");

#include "opt_inet.h"
#include "opt_ipsec.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>

#include <net/if.h>
#include <net/route.h>
#include <net/netisr.h>
#include <machine/cpu.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/ip_ecn.h>
#include <netinet/ip_icmp.h>

#ifdef IPSEC
#include <netkey/key.h>
#include <netkey/keydb.h>
#include <netkey/key_debug.h>
#endif

#ifdef FAST_IPSEC
#include <netipsec/ipsec.h>
#include <netkey/key.h>
#include <netkey/keydb.h>
#include <netkey/key_debug.h>
#include <netipsec/ah_var.h>
#endif


#include <machine/stdarg.h>



#include <netkey/key.h>

/* assumes that ip header and ah header are contiguous on mbuf */
void *
ah4_ctlinput(cmd, sa, v)
	int cmd;
	struct sockaddr *sa;
	void *v;
{
	struct ip *ip = v;
	struct ah *ah;
	struct icmp *icp;
	struct secasvar *sav;

	if (sa->sa_family != AF_INET ||
	    sa->sa_len != sizeof(struct sockaddr_in))
		return NULL;
	if ((unsigned)cmd >= PRC_NCMDS)
		return NULL;
#ifndef notyet
	(void) ip; (void) ah; (void) icp; (void) sav;
#else
	if (cmd == PRC_MSGSIZE && ip_mtudisc && ip && ip->ip_v == 4) {
		/*
		 * Check to see if we have a valid SA corresponding to
		 * the address in the ICMP message payload.
		 */
		ah = (struct ah *)((caddr_t)ip + (ip->ip_hl << 2));
		if ((sav = key_allocsa(AF_INET,
				       (caddr_t) &ip->ip_src,
				       (caddr_t) &ip->ip_dst,
				       IPPROTO_AH, ah->ah_spi)) == NULL)
			return NULL;
		if (sav->state != SADB_SASTATE_MATURE &&
		    sav->state != SADB_SASTATE_DYING) {
			key_freesav(sav);
			return NULL;
		}

		/* XXX Further validation? */

		key_freesav(sav);

		/*
		 * Now that we've validated that we are actually communicating
		 * with the host indicated in the ICMP message, locate the
		 * ICMP header, recalculate the new MTU, and create the
		 * corresponding routing entry.
		 */
		icp = (struct icmp *)((caddr_t)ip -
		    offsetof(struct icmp, icmp_ip));
		icmp_mtudisc(icp, ip->ip_dst);

		return NULL;
	}
#endif

	return NULL;
}

/* assumes that ip header and esp header are contiguous on mbuf */
void *
esp4_ctlinput(cmd, sa, v)
	int cmd;
	struct sockaddr *sa;
	void *v;
{
	struct ip *ip = v;
	struct esp *esp;
	struct icmp *icp;
	struct secasvar *sav;

	if (sa->sa_family != AF_INET ||
	    sa->sa_len != sizeof(struct sockaddr_in))
		return NULL;
	if ((unsigned)cmd >= PRC_NCMDS)
		return NULL;
#ifndef notyet
	(void) ip; (void) esp; (void) icp; (void) sav;
#else
	if (cmd == PRC_MSGSIZE && ip_mtudisc && ip && ip->ip_v == 4) {
		/*
		 * Check to see if we have a valid SA corresponding to
		 * the address in the ICMP message payload.
		 */
		esp = (struct esp *)((caddr_t)ip + (ip->ip_hl << 2));
		if ((sav = key_allocsa(AF_INET,
				       (caddr_t) &ip->ip_src,
				       (caddr_t) &ip->ip_dst,
				       IPPROTO_ESP, esp->esp_spi)) == NULL)
			return NULL;
		if (sav->state != SADB_SASTATE_MATURE &&
		    sav->state != SADB_SASTATE_DYING) {
			key_freesav(sav);
			return NULL;
		}

		/* XXX Further validation? */

		key_freesav(sav);

		/*
		 * Now that we've validated that we are actually communicating
		 * with the host indicated in the ICMP message, locate the
		 * ICMP header, recalculate the new MTU, and create the
		 * corresponding routing entry.
		 */
		icp = (struct icmp *)((caddr_t)ip -
		    offsetof(struct icmp, icmp_ip));
		icmp_mtudisc(icp, ip->ip_dst);

		return NULL;
	}
#endif

	return NULL;
}

#ifdef INET6
void
esp6_ctlinput(cmd, sa, d)
	int cmd;
	struct sockaddr *sa;
	void *d;
{
	const struct newesp *espp;
	struct newesp esp;
	struct ip6ctlparam *ip6cp = NULL, ip6cp1;
	struct secasvar *sav;
	struct ip6_hdr *ip6;
	struct mbuf *m;
	int off;
	struct sockaddr_in6 *sa6_src, *sa6_dst;

	if (sa->sa_family != AF_INET6 ||
	    sa->sa_len != sizeof(struct sockaddr_in6))
		return;
	if ((unsigned)cmd >= PRC_NCMDS)
		return;

	/* if the parameter is from icmp6, decode it. */
	if (d != NULL) {
		ip6cp = (struct ip6ctlparam *)d;
		m = ip6cp->ip6c_m;
		ip6 = ip6cp->ip6c_ip6;
		off = ip6cp->ip6c_off;
	} else {
		m = NULL;
		ip6 = NULL;
	}

	if (ip6) {
		/*
		 * Notify the error to all possible sockets via pfctlinput2.
		 * Since the upper layer information (such as protocol type,
		 * source and destination ports) is embedded in the encrypted
		 * data and might have been cut, we can't directly call
		 * an upper layer ctlinput function. However, the pcbnotify
		 * function will consider source and destination addresses
		 * as well as the flow info value, and may be able to find
		 * some PCB that should be notified.
		 * Although pfctlinput2 will call esp6_ctlinput(), there is
		 * no possibility of an infinite loop of function calls,
		 * because we don't pass the inner IPv6 header.
		 */
		bzero(&ip6cp1, sizeof(ip6cp1));
		ip6cp1.ip6c_src = ip6cp->ip6c_src;
		pfctlinput2(cmd, sa, (void *)&ip6cp1);

		/*
		 * Then go to special cases that need ESP header information.
		 * XXX: We assume that when ip6 is non NULL,
		 * M and OFF are valid.
		 */

		/* check if we can safely examine src and dst ports */
		if (m->m_pkthdr.len < off + sizeof(esp))
			return;

		if (m->m_len < off + sizeof(esp)) {
			/*
			 * this should be rare case,
			 * so we compromise on this copy...
			 */
			m_copydata(m, off, sizeof(esp), (caddr_t)&esp);
			espp = &esp;
		} else
			espp = (struct newesp*)(mtod(m, caddr_t) + off);

		if (cmd == PRC_MSGSIZE) {
			int valid = 0;

			/*
			 * Check to see if we have a valid SA corresponding to
			 * the address in the ICMP message payload.
			 */
			sa6_src = ip6cp->ip6c_src;
			sa6_dst = (struct sockaddr_in6 *)sa;
			sav = key_allocsa(AF_INET6,
					  (caddr_t)&sa6_src->sin6_addr,
					  (caddr_t)&sa6_dst->sin6_addr,
					  IPPROTO_ESP, espp->esp_spi);
			if (sav) {
				if (sav->state == SADB_SASTATE_MATURE ||
				    sav->state == SADB_SASTATE_DYING)
					valid++;
				key_freesav(sav);
			}

			/* XXX Further validation? */

			/*
			 * Depending on the value of "valid" and routing table
			 * size (mtudisc_{hi,lo}wat), we will:
			 * - recalcurate the new MTU and create the
			 *   corresponding routing entry, or
			 * - ignore the MTU change notification.
			 */
			icmp6_mtudisc_update((struct ip6ctlparam *)d, valid);
		}
	} else {
		/* we normally notify any pcb here */
	}
}
#endif /* INET6 */


/*FIXME: placebo for invalpcbcacheall. Fast-IPsec has no pcb cache? */

void	ipsec_invalpcbcacheall(void);
void
ipsec_invalpcbcacheall(void)
{
}

/* XXX will need a different oid at parent */
int
ipsec_sysctl(name, namelen, oldp, oldlenp, newp, newlen)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
{
	/* All sysctl names at this level are terminal. */
	if (namelen != 1)
		return ENOTDIR;

	/* common sanity checks */
	switch (name[0]) {
	case IPSECCTL_DEF_ESP_TRANSLEV:
	case IPSECCTL_DEF_ESP_NETLEV:
	case IPSECCTL_DEF_AH_TRANSLEV:
	case IPSECCTL_DEF_AH_NETLEV:
		if (newp != NULL && newlen == sizeof(int)) {
			switch (*(int *)newp) {
			case IPSEC_LEVEL_USE:
			case IPSEC_LEVEL_REQUIRE:
				break;
			default:
				return EINVAL;
			}
		}
		break;
	case IPSECCTL_DEF_POLICY:
		if (newp != NULL && newlen == sizeof(int)) {
			switch (*(int *)newp) {
			case IPSEC_POLICY_DISCARD:
			case IPSEC_POLICY_NONE:
				break;
			default:
				return EINVAL;
			}
			ipsec_invalpcbcacheall();
		}
		break;
	}

	switch (name[0]) {
	case IPSECCTL_STATS:
		return sysctl_struct(oldp, oldlenp, newp, newlen,
		    &ipsecstat, sizeof(ipsecstat));
	case IPSECCTL_DEF_POLICY:
		return sysctl_int(oldp, oldlenp, newp, newlen,
		    &ip4_def_policy.policy);
	case IPSECCTL_DEF_ESP_TRANSLEV:
		if (newp != NULL)
			ipsec_invalpcbcacheall();
		return sysctl_int(oldp, oldlenp, newp, newlen,
		    &ip4_esp_trans_deflev);
	case IPSECCTL_DEF_ESP_NETLEV:
		if (newp != NULL)
			ipsec_invalpcbcacheall();
		return sysctl_int(oldp, oldlenp, newp, newlen,
		    &ip4_esp_net_deflev);
	case IPSECCTL_DEF_AH_TRANSLEV:
		if (newp != NULL)
			ipsec_invalpcbcacheall();
		return sysctl_int(oldp, oldlenp, newp, newlen,
		    &ip4_ah_trans_deflev);
	case IPSECCTL_DEF_AH_NETLEV:
		if (newp != NULL)
			ipsec_invalpcbcacheall();
		return sysctl_int(oldp, oldlenp, newp, newlen,
		    &ip4_ah_net_deflev);
	case IPSECCTL_AH_CLEARTOS:
		return sysctl_int(oldp, oldlenp, newp, newlen,
		  &/*ip4_*/ah_cleartos);
	case IPSECCTL_AH_OFFSETMASK:
		return sysctl_int(oldp, oldlenp, newp, newlen,
		    &ip4_ah_offsetmask);
	case IPSECCTL_DFBIT:
		return sysctl_int(oldp, oldlenp, newp, newlen,
		    &ip4_ipsec_dfbit);
	case IPSECCTL_ECN:
		return sysctl_int(oldp, oldlenp, newp, newlen, &ip4_ipsec_ecn);
	case IPSECCTL_DEBUG:
		return sysctl_int(oldp, oldlenp, newp, newlen, &ipsec_debug);
	default:
		return EOPNOTSUPP;
	}
	/* NOTREACHED */
}
