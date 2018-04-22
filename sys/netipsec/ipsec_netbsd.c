/*	$NetBSD: ipsec_netbsd.c,v 1.53 2018/04/22 10:25:40 maxv Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: ipsec_netbsd.c,v 1.53 2018/04/22 10:25:40 maxv Exp $");

#if defined(_KERNEL_OPT)
#include "opt_inet.h"
#include "opt_ipsec.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
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
#include <sys/cpu.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/ip_ecn.h>
#include <netinet/ip_icmp.h>

#include <netipsec/ipsec.h>
#include <netipsec/ipsec_var.h>
#include <netipsec/ipsec_private.h>
#include <netipsec/key.h>
#include <netipsec/keydb.h>
#include <netipsec/key_debug.h>
#include <netipsec/ah.h>
#include <netipsec/ah_var.h>
#include <netipsec/esp.h>
#include <netipsec/esp_var.h>
#include <netipsec/ipip_var.h>
#include <netipsec/ipcomp_var.h>

#ifdef INET6
#include <netipsec/ipsec6.h>
#include <netinet6/ip6protosw.h>
#include <netinet/icmp6.h>
#endif

#include <netipsec/key.h>

/* assumes that ip header and ah header are contiguous on mbuf */
void *
ah4_ctlinput(int cmd, const struct sockaddr *sa, void *v)
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

	if (cmd == PRC_MSGSIZE && ip_mtudisc && ip && ip->ip_v == 4) {
		/*
		 * Check to see if we have a valid SA corresponding to
		 * the address in the ICMP message payload.
		 */
		ah = (struct ah *)((char *)ip + (ip->ip_hl << 2));
		sav = KEY_LOOKUP_SA((const union sockaddr_union *)sa,
		    IPPROTO_AH, ah->ah_spi, 0, 0);

		if (sav) {
			if (SADB_SASTATE_USABLE_P(sav)) {
				/*
				 * Now that we've validated that we are actually
				 * communicating with the host indicated in the
				 * ICMP message, locate the ICMP header,
				 * recalculate the new MTU, and create the
				 * corresponding routing entry.
				 */
				icp = (struct icmp *)((char *)ip -
				    offsetof(struct icmp, icmp_ip));
				icmp_mtudisc(icp, ip->ip_dst);
			}
			KEY_SA_UNREF(&sav);
		}
	}
	return NULL;
}

/* assumes that ip header and esp header are contiguous on mbuf */
void *
esp4_ctlinput(int cmd, const struct sockaddr *sa, void *v)
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

	if (cmd == PRC_MSGSIZE && ip_mtudisc && ip && ip->ip_v == 4) {
		/*
		 * Check to see if we have a valid SA corresponding to
		 * the address in the ICMP message payload.
		 */
		esp = (struct esp *)((char *)ip + (ip->ip_hl << 2));
		sav = KEY_LOOKUP_SA((const union sockaddr_union *)sa,
		    IPPROTO_ESP, esp->esp_spi, 0, 0);

		if (sav) {
			if (SADB_SASTATE_USABLE_P(sav)) {
				/*
				 * Now that we've validated that we are actually
				 * communicating with the host indicated in the
				 * ICMP message, locate the ICMP header,
				 * recalculate the new MTU, and create the
				 * corresponding routing entry.
				 */
				icp = (struct icmp *)((char *)ip -
				    offsetof(struct icmp, icmp_ip));
				icmp_mtudisc(icp, ip->ip_dst);
			}
			KEY_SA_UNREF(&sav);
		}
	}
	return NULL;
}

#ifdef INET6
void *
ah6_ctlinput(int cmd, const struct sockaddr *sa, void *d)
{
	const struct newah *ahp;
	struct newah ah;
	struct secasvar *sav;
	struct ip6_hdr *ip6;
	struct mbuf *m;
	struct ip6ctlparam *ip6cp = NULL;
	int off;

	if (sa->sa_family != AF_INET6 ||
	    sa->sa_len != sizeof(struct sockaddr_in6))
		return NULL;
	if ((unsigned)cmd >= PRC_NCMDS)
		return NULL;

	/* if the parameter is from icmp6, decode it. */
	if (d != NULL) {
		ip6cp = (struct ip6ctlparam *)d;
		m = ip6cp->ip6c_m;
		ip6 = ip6cp->ip6c_ip6;
		off = ip6cp->ip6c_off;
	} else {
		m = NULL;
		ip6 = NULL;
		off = 0;
	}

	if (ip6) {
		/* check if we can safely examine src and dst ports */
		if (m->m_pkthdr.len < off + sizeof(ah))
			return NULL;

		if (m->m_len < off + sizeof(ah)) {
			/*
			 * this should be rare case,
			 * so we compromise on this copy...
			 */
			m_copydata(m, off, sizeof(ah), &ah);
			ahp = &ah;
		} else
			ahp = (struct newah *)(mtod(m, char *) + off);

		if (cmd == PRC_MSGSIZE) {
			int valid = 0;

			/*
			 * Check to see if we have a valid SA corresponding
			 * to the address in the ICMP message payload.
			 */
			sav = KEY_LOOKUP_SA((const union sockaddr_union *)sa,
			    IPPROTO_AH, ahp->ah_spi, 0, 0);

			if (sav) {
				if (SADB_SASTATE_USABLE_P(sav))
					valid++;
				KEY_SA_UNREF(&sav);
			}

			/* XXX Further validation? */

			/*
			 * Depending on the value of "valid" and routing
			 * table size (mtudisc_{hi,lo}wat), we will:
			 * - recalculate the new MTU and create the
			 *   corresponding routing entry, or
			 * - ignore the MTU change notification.
			 */
			icmp6_mtudisc_update((struct ip6ctlparam *)d, valid);
		}

		/* we normally notify single pcb here */
	} else {
		/* we normally notify any pcb here */
	}
	return NULL;
}

void *
esp6_ctlinput(int cmd, const struct sockaddr *sa, void *d)
{
	const struct newesp *espp;
	struct newesp esp;
	struct ip6ctlparam *ip6cp = NULL, ip6cp1;
	struct secasvar *sav;
	struct ip6_hdr *ip6;
	struct mbuf *m;
	int off;

	if (sa->sa_family != AF_INET6 ||
	    sa->sa_len != sizeof(struct sockaddr_in6))
		return NULL;
	if ((unsigned)cmd >= PRC_NCMDS)
		return NULL;

	/* if the parameter is from icmp6, decode it. */
	if (d != NULL) {
		ip6cp = (struct ip6ctlparam *)d;
		m = ip6cp->ip6c_m;
		ip6 = ip6cp->ip6c_ip6;
		off = ip6cp->ip6c_off;
	} else {
		m = NULL;
		ip6 = NULL;
		off = 0;
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
		memset(&ip6cp1, 0, sizeof(ip6cp1));
		ip6cp1.ip6c_src = ip6cp->ip6c_src;
		pfctlinput2(cmd, sa, &ip6cp1);

		/* check if we can safely examine src and dst ports */
		if (m->m_pkthdr.len < off + sizeof(esp))
			return NULL;

		if (m->m_len < off + sizeof(esp)) {
			/*
			 * this should be rare case,
			 * so we compromise on this copy...
			 */
			m_copydata(m, off, sizeof(esp), &esp);
			espp = &esp;
		} else
			espp = (struct newesp *)(mtod(m, char *) + off);

		if (cmd == PRC_MSGSIZE) {
			int valid = 0;

			/*
			 * Check to see if we have a valid SA corresponding to
			 * the address in the ICMP message payload.
			 */

			sav = KEY_LOOKUP_SA((const union sockaddr_union *)sa,
			    IPPROTO_ESP, espp->esp_spi, 0, 0);

			if (sav) {
				if (SADB_SASTATE_USABLE_P(sav))
					valid++;
				KEY_SA_UNREF(&sav);
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
	return NULL;
}
#endif /* INET6 */

static int
sysctl_ipsec(SYSCTLFN_ARGS)
{
	int error, t;
	struct sysctlnode node;

	node = *rnode;
	t = *(int *)rnode->sysctl_data;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	switch (rnode->sysctl_num) {
	case IPSECCTL_DEF_ESP_TRANSLEV:
	case IPSECCTL_DEF_ESP_NETLEV:
	case IPSECCTL_DEF_AH_TRANSLEV:
	case IPSECCTL_DEF_AH_NETLEV:
		if (t != IPSEC_LEVEL_USE &&
		    t != IPSEC_LEVEL_REQUIRE)
			return EINVAL;
		ipsec_invalpcbcacheall();
		break;
	case IPSECCTL_DEF_POLICY:
		if (t != IPSEC_POLICY_DISCARD &&
		    t != IPSEC_POLICY_NONE)
			return EINVAL;
		ipsec_invalpcbcacheall();
		break;
	default:
		return EINVAL;
	}

	*(int *)rnode->sysctl_data = t;

	return 0;
}

#ifdef IPSEC_DEBUG
static int
sysctl_ipsec_test(SYSCTLFN_ARGS)
{
	int t, error;
	struct sysctlnode node;

	node = *rnode;
	t = *(int *)rnode->sysctl_data;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (t < 0 || t > 1)
		return EINVAL;

	if (rnode->sysctl_data == &ipsec_replay)
		printf("ipsec: Anti-Replay service %s\n",
		    (t == 1) ? "deactivated" : "activated");
	else if (rnode->sysctl_data == &ipsec_integrity)
		 printf("ipsec: HMAC corruption %s\n",
		     (t == 0) ? "deactivated" : "activated");

	*(int *)rnode->sysctl_data = t;

	return 0;
}
#endif

static int
sysctl_net_inet_ipsec_stats(SYSCTLFN_ARGS)
{

	return (NETSTAT_SYSCTL(ipsecstat_percpu, IPSEC_NSTATS));
}

static int
sysctl_net_inet_ah_stats(SYSCTLFN_ARGS)
{

	return (NETSTAT_SYSCTL(ahstat_percpu, AH_NSTATS));
}

static int
sysctl_net_inet_esp_stats(SYSCTLFN_ARGS)
{

	return (NETSTAT_SYSCTL(espstat_percpu, ESP_NSTATS));
}

static int
sysctl_net_inet_ipcomp_stats(SYSCTLFN_ARGS)
{

	return (NETSTAT_SYSCTL(ipcompstat_percpu, IPCOMP_NSTATS));
}

static int
sysctl_net_inet_ipip_stats(SYSCTLFN_ARGS)
{

	return (NETSTAT_SYSCTL(ipipstat_percpu, IPIP_NSTATS));
}

static int
sysctl_net_ipsec_enabled(SYSCTLFN_ARGS)
{
	int newenabled, error;
	struct sysctlnode node;
	node = *rnode;
	node.sysctl_data = &newenabled;

	newenabled = ipsec_enabled;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	switch (newenabled) {
	case 0:
		if (key_get_used())
			return EBUSY;
		/*FALLTHROUGH*/
	case 1:
	case 2:
		ipsec_enabled = newenabled;
		key_update_used();
		return 0;
	default:
		return EINVAL;
	}
}

/* XXX will need a different oid at parent */
void
sysctl_net_inet_ipsec_setup(struct sysctllog **clog)
{
	const struct sysctlnode *_ipsec;
	int ipproto_ipsec;

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "inet", NULL,
		       NULL, 0, NULL, 0,
		       CTL_NET, PF_INET, CTL_EOL);

	/*
	 * in numerical order:
	 *
	 * net.inet.ipip:	CTL_NET.PF_INET.IPPROTO_IPIP
	 * net.inet.esp:	CTL_NET.PF_INET.IPPROTO_ESP
	 * net.inet.ah:		CTL_NET.PF_INET.IPPROTO_AH
	 * net.inet.ipcomp:	CTL_NET.PF_INET.IPPROTO_IPCOMP
	 * net.inet.ipsec:	CTL_NET.PF_INET.CTL_CREATE
	 *
	 * this creates separate trees by name, but maintains that the
	 * ipsec name leads to all the old leaves.
	 */

	/* create net.inet.ipip */
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "ipip", NULL,
		       NULL, 0, NULL, 0,
		       CTL_NET, PF_INET, IPPROTO_IPIP, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READONLY,
		       CTLTYPE_STRUCT, "ipip_stats", NULL,
		       sysctl_net_inet_ipip_stats, 0, NULL, 0,
		       CTL_NET, PF_INET, IPPROTO_IPIP,
		       CTL_CREATE, CTL_EOL);

	/* create net.inet.esp subtree under IPPROTO_ESP */
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "esp", NULL,
		       NULL, 0, NULL, 0,
		       CTL_NET, PF_INET, IPPROTO_ESP, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READONLY,
		       CTLTYPE_STRUCT, "esp_stats", NULL,
		       sysctl_net_inet_esp_stats, 0, NULL, 0,
		       CTL_NET, PF_INET, IPPROTO_ESP,
		       CTL_CREATE, CTL_EOL);

	/* create net.inet.ah subtree under IPPROTO_AH */
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "ah", NULL,
		       NULL, 0, NULL, 0,
		       CTL_NET, PF_INET, IPPROTO_AH, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READONLY,
		       CTLTYPE_STRUCT, "ah_stats", NULL,
		       sysctl_net_inet_ah_stats, 0, NULL, 0,
		       CTL_NET, PF_INET, IPPROTO_AH,
		       CTL_CREATE, CTL_EOL);

	/* create net.inet.ipcomp */
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "ipcomp", NULL,
		       NULL, 0, NULL, 0,
		       CTL_NET, PF_INET, IPPROTO_IPCOMP, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READONLY,
		       CTLTYPE_STRUCT, "ipcomp_stats", NULL,
		       sysctl_net_inet_ipcomp_stats, 0, NULL, 0,
		       CTL_NET, PF_INET, IPPROTO_IPCOMP,
		       CTL_CREATE, CTL_EOL);

	/* create net.inet.ipsec subtree under dynamic oid */
	sysctl_createv(clog, 0, NULL, &_ipsec,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "ipsec", NULL,
		       NULL, 0, NULL, 0,
		       CTL_NET, PF_INET, CTL_CREATE, CTL_EOL);
	ipproto_ipsec = (_ipsec != NULL) ? _ipsec->sysctl_num : 0;

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "def_policy", NULL,
		       sysctl_ipsec, 0, &ip4_def_policy.policy, 0,
		       CTL_NET, PF_INET, ipproto_ipsec,
		       IPSECCTL_DEF_POLICY, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "esp_trans_deflev", NULL,
		       sysctl_ipsec, 0, &ip4_esp_trans_deflev, 0,
		       CTL_NET, PF_INET, ipproto_ipsec,
		       IPSECCTL_DEF_ESP_TRANSLEV, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "esp_net_deflev", NULL,
		       sysctl_ipsec, 0, &ip4_esp_net_deflev, 0,
		       CTL_NET, PF_INET, ipproto_ipsec,
		       IPSECCTL_DEF_ESP_NETLEV, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "ah_trans_deflev", NULL,
		       sysctl_ipsec, 0, &ip4_ah_trans_deflev, 0,
		       CTL_NET, PF_INET, ipproto_ipsec,
		       IPSECCTL_DEF_AH_TRANSLEV, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "ah_net_deflev", NULL,
		       sysctl_ipsec, 0, &ip4_ah_net_deflev, 0,
		       CTL_NET, PF_INET, ipproto_ipsec,
		       IPSECCTL_DEF_AH_NETLEV, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "ah_cleartos", NULL,
		       NULL, 0, &ip4_ah_cleartos, 0,
		       CTL_NET, PF_INET, ipproto_ipsec,
		       IPSECCTL_AH_CLEARTOS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "ah_offsetmask", NULL,
		       NULL, 0, &ip4_ah_offsetmask, 0,
		       CTL_NET, PF_INET, ipproto_ipsec,
		       IPSECCTL_AH_OFFSETMASK, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "dfbit", NULL,
		       NULL, 0, &ip4_ipsec_dfbit, 0,
		       CTL_NET, PF_INET, ipproto_ipsec,
		       IPSECCTL_DFBIT, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "ecn", NULL,
		       NULL, 0, &ip4_ipsec_ecn, 0,
		       CTL_NET, PF_INET, ipproto_ipsec,
		       IPSECCTL_ECN, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "debug", NULL,
		       NULL, 0, &ipsec_debug, 0,
		       CTL_NET, PF_INET, ipproto_ipsec,
		       IPSECCTL_DEBUG, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "ipip_spoofcheck", NULL,
		       NULL, 0, &ipip_spoofcheck, 0,
		       CTL_NET, PF_INET, ipproto_ipsec,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READONLY,
		       CTLTYPE_STRUCT, "ipsecstats", NULL,
		       sysctl_net_inet_ipsec_stats, 0, NULL, 0,
		       CTL_NET, PF_INET, ipproto_ipsec,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "enabled",
		       SYSCTL_DESCR("Enable IPSec processing"),
		       sysctl_net_ipsec_enabled, 0, NULL, 0,
		       CTL_NET, PF_INET, ipproto_ipsec,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READONLY,
		       CTLTYPE_INT, "used",
		       SYSCTL_DESCR("Is IPSec active?"),
		       NULL, 0, &ipsec_used, 0,
		       CTL_NET, PF_INET, ipproto_ipsec,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "ah_enable", NULL,
		       NULL, 0, &ah_enable, 0,
		       CTL_NET, PF_INET, ipproto_ipsec,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "esp_enable", NULL,
		       NULL, 0, &esp_enable, 0,
		       CTL_NET, PF_INET, ipproto_ipsec,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "ipcomp_enable", NULL,
		       NULL, 0, &ipcomp_enable, 0,
		       CTL_NET, PF_INET, ipproto_ipsec,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "crypto_support", NULL,
		       NULL, 0, &crypto_support, 0,
		       CTL_NET, PF_INET, ipproto_ipsec,
		       CTL_CREATE, CTL_EOL);

#ifdef IPSEC_DEBUG
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "test_replay",
		       SYSCTL_DESCR("Emulate replay attack"),
		       sysctl_ipsec_test, 0, &ipsec_replay, 0,
		       CTL_NET, PF_INET, ipproto_ipsec,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "test_integrity",
		       SYSCTL_DESCR("Emulate man-in-the-middle attack"),
		       sysctl_ipsec_test, 0, &ipsec_integrity, 0,
		       CTL_NET, PF_INET, ipproto_ipsec,
		       CTL_CREATE, CTL_EOL);
#endif
}

#ifdef INET6
void
sysctl_net_inet6_ipsec6_setup(struct sysctllog **clog)
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "inet6", NULL,
		       NULL, 0, NULL, 0,
		       CTL_NET, PF_INET6, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "ipsec6",
		       SYSCTL_DESCR("IPv6 related IPSec settings"),
		       NULL, 0, NULL, 0,
		       CTL_NET, PF_INET6, IPPROTO_AH, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_STRUCT, "stats",
		       SYSCTL_DESCR("IPSec statistics and counters"),
		       sysctl_net_inet_ipsec_stats, 0, NULL, 0,
		       CTL_NET, PF_INET6, IPPROTO_AH,
		       IPSECCTL_STATS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "def_policy",
		       SYSCTL_DESCR("Default action for non-IPSec packets"),
		       sysctl_ipsec, 0, (void *)&ip6_def_policy, 0,
		       CTL_NET, PF_INET6, IPPROTO_AH,
		       IPSECCTL_DEF_POLICY, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "esp_trans_deflev",
		       SYSCTL_DESCR("Default required security level for "
				    "transport mode traffic"),
		       sysctl_ipsec, 0, &ip6_esp_trans_deflev, 0,
		       CTL_NET, PF_INET6, IPPROTO_AH,
		       IPSECCTL_DEF_ESP_TRANSLEV, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "esp_net_deflev",
		       SYSCTL_DESCR("Default required security level for "
				    "tunneled traffic"),
		       sysctl_ipsec, 0, &ip6_esp_net_deflev, 0,
		       CTL_NET, PF_INET6, IPPROTO_AH,
		       IPSECCTL_DEF_ESP_NETLEV, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "ah_trans_deflev",
		       SYSCTL_DESCR("Default required security level for "
				    "transport mode headers"),
		       sysctl_ipsec, 0, &ip6_ah_trans_deflev, 0,
		       CTL_NET, PF_INET6, IPPROTO_AH,
		       IPSECCTL_DEF_AH_TRANSLEV, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "ah_net_deflev",
		       SYSCTL_DESCR("Default required security level for "
				    "tunneled headers"),
		       sysctl_ipsec, 0, &ip6_ah_net_deflev, 0,
		       CTL_NET, PF_INET6, IPPROTO_AH,
		       IPSECCTL_DEF_AH_NETLEV, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "ecn",
		       SYSCTL_DESCR("Behavior of ECN for tunneled traffic"),
		       NULL, 0, &ip6_ipsec_ecn, 0,
		       CTL_NET, PF_INET6, IPPROTO_AH,
		       IPSECCTL_ECN, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "debug",
		       SYSCTL_DESCR("Enable IPSec debugging output"),
		       NULL, 0, &ipsec_debug, 0,
		       CTL_NET, PF_INET6, IPPROTO_AH,
		       IPSECCTL_DEBUG, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "enabled",
		       SYSCTL_DESCR("Enable IPSec processing"),
		       sysctl_net_ipsec_enabled, 0, NULL, 0,
		       CTL_NET, PF_INET6, IPPROTO_AH,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READONLY,
		       CTLTYPE_INT, "used",
		       SYSCTL_DESCR("Is IPSec active?"),
		       NULL, 0, &ipsec_used, 0,
		       CTL_NET, PF_INET6, IPPROTO_AH,
		       CTL_CREATE, CTL_EOL);
}
#endif /* INET6 */
