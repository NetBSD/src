/* $NetBSD: ipsec.c,v 1.157 2018/04/19 08:27:38 maxv Exp $ */
/* $FreeBSD: ipsec.c,v 1.2.2.2 2003/07/01 01:38:13 sam Exp $ */
/* $KAME: ipsec.c,v 1.103 2001/05/24 07:14:18 sakane Exp $ */

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *	may be used to endorse or promote products derived from this software
 *	without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: ipsec.c,v 1.157 2018/04/19 08:27:38 maxv Exp $");

/*
 * IPsec controller part.
 */

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
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/sysctl.h>
#include <sys/proc.h>
#include <sys/kauth.h>
#include <sys/cpu.h>
#include <sys/kmem.h>
#include <sys/pserialize.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/in_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip_private.h>

#include <netinet/ip6.h>
#ifdef INET6
#include <netinet6/ip6_var.h>
#endif
#include <netinet/in_pcb.h>
#ifdef INET6
#include <netinet6/in6_pcb.h>
#include <netinet/icmp6.h>
#endif

#include <netipsec/ipsec.h>
#include <netipsec/ipsec_var.h>
#include <netipsec/ipsec_private.h>
#ifdef INET6
#include <netipsec/ipsec6.h>
#endif
#include <netipsec/ah_var.h>
#include <netipsec/esp_var.h>
#include <netipsec/ipcomp.h>		/*XXX*/
#include <netipsec/ipcomp_var.h>

#include <netipsec/key.h>
#include <netipsec/keydb.h>
#include <netipsec/key_debug.h>

#include <netipsec/xform.h>

int ipsec_used = 0;
int ipsec_enabled = 1;

#ifdef IPSEC_DEBUG
int ipsec_debug = 1;

/*
 * When set to 1, IPsec will send packets with the same sequence number.
 * This allows to verify if the other side has proper replay attacks detection.
 */
int ipsec_replay = 0;

/*
 * When set 1, IPsec will send packets with corrupted HMAC.
 * This allows to verify if the other side properly detects modified packets.
 */
int ipsec_integrity = 0;
#else
int ipsec_debug = 0;
#endif

percpu_t *ipsecstat_percpu;

int ip4_ah_offsetmask = 0;	/* maybe IP_DF? */
int ip4_ipsec_dfbit = 2;	/* DF bit on encap. 0: clear 1: set 2: copy */
int ip4_esp_trans_deflev = IPSEC_LEVEL_USE;
int ip4_esp_net_deflev = IPSEC_LEVEL_USE;
int ip4_ah_trans_deflev = IPSEC_LEVEL_USE;
int ip4_ah_net_deflev = IPSEC_LEVEL_USE;
struct secpolicy ip4_def_policy;
int ip4_ipsec_ecn = 0;		/* ECN ignore(-1)/forbidden(0)/allowed(1) */

u_int ipsec_spdgen = 1;		/* SPD generation # */

static struct secpolicy ipsec_dummy_sp __read_mostly = {
	.state		= IPSEC_SPSTATE_ALIVE,
	/* If ENTRUST, the dummy SP never be used. See ipsec_getpolicybysock. */
	.policy		= IPSEC_POLICY_ENTRUST,
};

static struct secpolicy *ipsec_checkpcbcache(struct mbuf *,
    struct inpcbpolicy *, int);
static int ipsec_fillpcbcache(struct inpcbpolicy *, struct mbuf *,
    struct secpolicy *, int);
static int ipsec_invalpcbcache(struct inpcbpolicy *, int);

/*
 * Crypto support requirements:
 *
 *  1	require hardware support
 * -1	require software support
 *  0	take anything
 */
int crypto_support = 0;

static struct secpolicy *ipsec_getpolicybysock(struct mbuf *, u_int,
    struct inpcb_hdr *, int *);

#ifdef INET6
int ip6_esp_trans_deflev = IPSEC_LEVEL_USE;
int ip6_esp_net_deflev = IPSEC_LEVEL_USE;
int ip6_ah_trans_deflev = IPSEC_LEVEL_USE;
int ip6_ah_net_deflev = IPSEC_LEVEL_USE;
struct secpolicy ip6_def_policy;
int ip6_ipsec_ecn = 0;		/* ECN ignore(-1)/forbidden(0)/allowed(1) */
#endif

static int ipsec_setspidx_inpcb(struct mbuf *, void *);
static int ipsec_setspidx(struct mbuf *, struct secpolicyindex *, int);
static void ipsec4_get_ulp(struct mbuf *m, struct secpolicyindex *, int);
static int ipsec4_setspidx_ipaddr(struct mbuf *, struct secpolicyindex *);
#ifdef INET6
static void ipsec6_get_ulp(struct mbuf *m, struct secpolicyindex *, int);
static int ipsec6_setspidx_ipaddr(struct mbuf *, struct secpolicyindex *);
#endif
static void ipsec_delpcbpolicy(struct inpcbpolicy *);
static void ipsec_destroy_policy(struct secpolicy *);
static int ipsec_sp_reject(const struct secpolicy *, const struct mbuf *);
static void vshiftl(unsigned char *, int, int);
static size_t ipsec_sp_hdrsiz(const struct secpolicy *, const struct mbuf *);

/*
 * Try to validate and use cached policy on a PCB.
 */
static struct secpolicy *
ipsec_checkpcbcache(struct mbuf *m, struct inpcbpolicy *pcbsp, int dir)
{
	struct secpolicyindex spidx;
	struct secpolicy *sp = NULL;
	int s;

	KASSERT(IPSEC_DIR_IS_VALID(dir));
	KASSERT(pcbsp != NULL);
	KASSERT(dir < __arraycount(pcbsp->sp_cache));
	KASSERT(inph_locked(pcbsp->sp_inph));

	/*
	 * Checking the generation and sp->state and taking a reference to an SP
	 * must be in a critical section of pserialize. See key_unlink_sp.
	 */
	s = pserialize_read_enter();
	/* SPD table change invalidate all the caches. */
	if (ipsec_spdgen != pcbsp->sp_cache[dir].cachegen) {
		ipsec_invalpcbcache(pcbsp, dir);
		goto out;
	}
	sp = pcbsp->sp_cache[dir].cachesp;
	if (sp == NULL)
		goto out;
	if (sp->state != IPSEC_SPSTATE_ALIVE) {
		sp = NULL;
		ipsec_invalpcbcache(pcbsp, dir);
		goto out;
	}
	if ((pcbsp->sp_cacheflags & IPSEC_PCBSP_CONNECTED) == 0) {
		/* NB: assume ipsec_setspidx never sleep */
		if (ipsec_setspidx(m, &spidx, 1) != 0) {
			sp = NULL;
			goto out;
		}

		/*
		 * We have to make an exact match here since the cached rule
		 * might have lower priority than a rule that would otherwise
		 * have matched the packet.
		 */
		if (memcmp(&pcbsp->sp_cache[dir].cacheidx, &spidx,
		    sizeof(spidx))) {
			sp = NULL;
			goto out;
		}
	} else {
		/*
		 * The pcb is connected, and the L4 code is sure that:
		 * - outgoing side uses inp_[lf]addr
		 * - incoming side looks up policy after inpcb lookup
		 * and address pair is know to be stable.  We do not need
		 * to generate spidx again, nor check the address match again.
		 *
		 * For IPv4/v6 SOCK_STREAM sockets, this assumptions holds
		 * and there are calls to ipsec_pcbconn() from in_pcbconnect().
		 */
	}

	sp->lastused = time_second;
	KEY_SP_REF(sp);
	KEYDEBUG_PRINTF(KEYDEBUG_IPSEC_STAMP,
	    "DP cause refcnt++:%d SP:%p\n",
	    key_sp_refcnt(sp), pcbsp->sp_cache[dir].cachesp);
out:
	pserialize_read_exit(s);
	return sp;
}

static int
ipsec_fillpcbcache(struct inpcbpolicy *pcbsp, struct mbuf *m,
    struct secpolicy *sp, int dir)
{

	KASSERT(IPSEC_DIR_IS_INOROUT(dir));
	KASSERT(dir < __arraycount(pcbsp->sp_cache));
	KASSERT(inph_locked(pcbsp->sp_inph));

	pcbsp->sp_cache[dir].cachesp = NULL;
	pcbsp->sp_cache[dir].cachehint = IPSEC_PCBHINT_UNKNOWN;
	if (ipsec_setspidx(m, &pcbsp->sp_cache[dir].cacheidx, 1) != 0) {
		return EINVAL;
	}
	pcbsp->sp_cache[dir].cachesp = sp;
	if (pcbsp->sp_cache[dir].cachesp) {
		/*
		 * If the PCB is connected, we can remember a hint to
		 * possibly short-circuit IPsec processing in other places.
		 */
		if (pcbsp->sp_cacheflags & IPSEC_PCBSP_CONNECTED) {
			switch (pcbsp->sp_cache[dir].cachesp->policy) {
			case IPSEC_POLICY_NONE:
			case IPSEC_POLICY_BYPASS:
				pcbsp->sp_cache[dir].cachehint =
				    IPSEC_PCBHINT_NO;
				break;
			default:
				pcbsp->sp_cache[dir].cachehint =
				    IPSEC_PCBHINT_YES;
			}
		}
	}
	pcbsp->sp_cache[dir].cachegen = ipsec_spdgen;

	return 0;
}

static int
ipsec_invalpcbcache(struct inpcbpolicy *pcbsp, int dir)
{
	int i;

	KASSERT(inph_locked(pcbsp->sp_inph));

	for (i = IPSEC_DIR_INBOUND; i <= IPSEC_DIR_OUTBOUND; i++) {
		if (dir != IPSEC_DIR_ANY && i != dir)
			continue;
		pcbsp->sp_cache[i].cachesp = NULL;
		pcbsp->sp_cache[i].cachehint = IPSEC_PCBHINT_UNKNOWN;
		pcbsp->sp_cache[i].cachegen = 0;
		memset(&pcbsp->sp_cache[i].cacheidx, 0,
		    sizeof(pcbsp->sp_cache[i].cacheidx));
	}
	return 0;
}

void
ipsec_pcbconn(struct inpcbpolicy *pcbsp)
{

	KASSERT(inph_locked(pcbsp->sp_inph));

	pcbsp->sp_cacheflags |= IPSEC_PCBSP_CONNECTED;
	ipsec_invalpcbcache(pcbsp, IPSEC_DIR_ANY);
}

void
ipsec_pcbdisconn(struct inpcbpolicy *pcbsp)
{

	KASSERT(inph_locked(pcbsp->sp_inph));

	pcbsp->sp_cacheflags &= ~IPSEC_PCBSP_CONNECTED;
	ipsec_invalpcbcache(pcbsp, IPSEC_DIR_ANY);
}

void
ipsec_invalpcbcacheall(void)
{

	if (ipsec_spdgen == UINT_MAX)
		ipsec_spdgen = 1;
	else
		ipsec_spdgen++;
}

/*
 * Return a held reference to the default SP.
 */
static struct secpolicy *
key_get_default_sp(int af, const char *where, int tag)
{
	struct secpolicy *sp;

	KEYDEBUG_PRINTF(KEYDEBUG_IPSEC_STAMP, "DP from %s:%u\n", where, tag);

	switch(af) {
	case AF_INET:
		sp = &ip4_def_policy;
		break;
#ifdef INET6
	case AF_INET6:
		sp = &ip6_def_policy;
		break;
#endif
	default:
		KEYDEBUG_PRINTF(KEYDEBUG_IPSEC_STAMP,
		    "unexpected protocol family %u\n", af);
		return NULL;
	}

	if (sp->policy != IPSEC_POLICY_DISCARD &&
	    sp->policy != IPSEC_POLICY_NONE) {
		IPSECLOG(LOG_INFO, "fixed system default policy: %d->%d\n",
		    sp->policy, IPSEC_POLICY_NONE);
		sp->policy = IPSEC_POLICY_NONE;
	}
	KEY_SP_REF(sp);

	KEYDEBUG_PRINTF(KEYDEBUG_IPSEC_STAMP, "DP returns SP:%p (%u)\n",
	    sp, key_sp_refcnt(sp));
	return sp;
}

#define	KEY_GET_DEFAULT_SP(af) \
	key_get_default_sp((af), __func__, __LINE__)

/*
 * For OUTBOUND packet having a socket. Searching SPD for packet,
 * and return a pointer to SP.
 * OUT:	NULL:	no appropriate SP found, the following value is set to error.
 *		0	: bypass
 *		EACCES	: discard packet.
 *		ENOENT	: ipsec_acquire() in progress, maybe.
 *		others	: error occurred.
 *	others:	a pointer to SP
 *
 * NOTE: IPv6 mapped address concern is implemented here.
 */
static struct secpolicy *
ipsec_getpolicybysock(struct mbuf *m, u_int dir, struct inpcb_hdr *inph,
    int *error)
{
	struct inpcbpolicy *pcbsp = NULL;
	struct secpolicy *currsp = NULL;	/* policy on socket */
	struct secpolicy *sp;
	int af;

	KASSERT(m != NULL);
	KASSERT(inph != NULL);
	KASSERT(error != NULL);
	KASSERTMSG(IPSEC_DIR_IS_INOROUT(dir), "invalid direction %u", dir);

	KASSERT(inph->inph_socket != NULL);
	KASSERT(inph_locked(inph));

	/* XXX FIXME inpcb/in6pcb vs socket*/
	af = inph->inph_af;
	KASSERTMSG(af == AF_INET || af == AF_INET6,
	    "unexpected protocol family %u", af);

	KASSERT(inph->inph_sp != NULL);
	/* If we have a cached entry, and if it is still valid, use it. */
	IPSEC_STATINC(IPSEC_STAT_SPDCACHELOOKUP);
	currsp = ipsec_checkpcbcache(m, inph->inph_sp, dir);
	if (currsp) {
		*error = 0;
		return currsp;
	}
	IPSEC_STATINC(IPSEC_STAT_SPDCACHEMISS);

	switch (af) {
	case AF_INET:
#if defined(INET6)
	case AF_INET6:
#endif
		*error = ipsec_setspidx_inpcb(m, inph);
		pcbsp = inph->inph_sp;
		break;
	default:
		*error = EPFNOSUPPORT;
		break;
	}
	if (*error)
		return NULL;

	KASSERT(pcbsp != NULL);
	switch (dir) {
	case IPSEC_DIR_INBOUND:
		currsp = pcbsp->sp_in;
		break;
	case IPSEC_DIR_OUTBOUND:
		currsp = pcbsp->sp_out;
		break;
	}
	KASSERT(currsp != NULL);

	if (pcbsp->priv) {	/* when privileged socket */
		switch (currsp->policy) {
		case IPSEC_POLICY_BYPASS:
		case IPSEC_POLICY_IPSEC:
			KEY_SP_REF(currsp);
			sp = currsp;
			break;

		case IPSEC_POLICY_ENTRUST:
			/* look for a policy in SPD */
			sp = KEY_LOOKUP_SP_BYSPIDX(&currsp->spidx, dir);
			if (sp == NULL)		/* no SP found */
				sp = KEY_GET_DEFAULT_SP(af);
			break;

		default:
			IPSECLOG(LOG_ERR, "Invalid policy for PCB %d\n",
			    currsp->policy);
			*error = EINVAL;
			return NULL;
		}
	} else {				/* unpriv, SPD has policy */
		sp = KEY_LOOKUP_SP_BYSPIDX(&currsp->spidx, dir);
		if (sp == NULL) {		/* no SP found */
			switch (currsp->policy) {
			case IPSEC_POLICY_BYPASS:
				IPSECLOG(LOG_ERR, "Illegal policy for "
				    "non-priviliged defined %d\n",
				    currsp->policy);
				*error = EINVAL;
				return NULL;

			case IPSEC_POLICY_ENTRUST:
				sp = KEY_GET_DEFAULT_SP(af);
				break;

			case IPSEC_POLICY_IPSEC:
				KEY_SP_REF(currsp);
				sp = currsp;
				break;

			default:
				IPSECLOG(LOG_ERR, "Invalid policy for "
				    "PCB %d\n", currsp->policy);
				*error = EINVAL;
				return NULL;
			}
		}
	}
	KASSERTMSG(sp != NULL, "null SP (priv %u policy %u", pcbsp->priv,
	    currsp->policy);
	KEYDEBUG_PRINTF(KEYDEBUG_IPSEC_STAMP,
	    "DP (priv %u policy %u) allocates SP:%p (refcnt %u)\n",
	    pcbsp->priv, currsp->policy, sp, key_sp_refcnt(sp));
	ipsec_fillpcbcache(pcbsp, m, sp, dir);
	return sp;
}

/*
 * For FORWARDING packet or OUTBOUND without a socket. Searching SPD for packet,
 * and return a pointer to SP.
 * OUT:	positive: a pointer to the entry for security policy leaf matched.
 *	NULL:	no appropriate SP found, the following value is set to error.
 *		0	: bypass
 *		EACCES	: discard packet.
 *		ENOENT	: ipsec_acquire() in progress, maybe.
 *		others	: error occurred.
 */
static struct secpolicy *
ipsec_getpolicybyaddr(struct mbuf *m, u_int dir, int flag, int *error)
{
	struct secpolicyindex spidx;
	struct secpolicy *sp;

	KASSERT(m != NULL);
	KASSERT(error != NULL);
	KASSERTMSG(IPSEC_DIR_IS_INOROUT(dir), "invalid direction %u", dir);

	sp = NULL;

	/* Make an index to look for a policy. */
	*error = ipsec_setspidx(m, &spidx, (flag & IP_FORWARDING) ? 0 : 1);
	if (*error != 0) {
		IPSECLOG(LOG_DEBUG, "setpidx failed, dir %u flag %u\n", dir, flag);
		memset(&spidx, 0, sizeof(spidx));
		return NULL;
	}

	spidx.dir = dir;

	if (key_havesp(dir)) {
		sp = KEY_LOOKUP_SP_BYSPIDX(&spidx, dir);
	}
	if (sp == NULL) {
		/* no SP found, use system default */
		sp = KEY_GET_DEFAULT_SP(spidx.dst.sa.sa_family);
	}

	KASSERT(sp != NULL);
	return sp;
}

static struct secpolicy *
ipsec_checkpolicy(struct mbuf *m, u_int dir, u_int flag, int *error,
    void *inp)
{
	struct secpolicy *sp;

	*error = 0;

	if (inp == NULL) {
		sp = ipsec_getpolicybyaddr(m, dir, flag, error);
	} else {
		struct inpcb_hdr *inph = (struct inpcb_hdr *)inp;
		KASSERT(inph->inph_socket != NULL);
		sp = ipsec_getpolicybysock(m, dir, inph, error);
	}
	if (sp == NULL) {
		KASSERTMSG(*error != 0, "getpolicy failed w/o error");
		IPSEC_STATINC(IPSEC_STAT_OUT_INVAL);
		return NULL;
	}
	KASSERTMSG(*error == 0, "sp w/ error set to %u", *error);

	switch (sp->policy) {
	case IPSEC_POLICY_ENTRUST:
	default:
		printf("%s: invalid policy %u\n", __func__, sp->policy);
		/* fall thru... */
	case IPSEC_POLICY_DISCARD:
		IPSEC_STATINC(IPSEC_STAT_OUT_POLVIO);
		*error = -EINVAL;	/* packet is discarded by caller */
		break;
	case IPSEC_POLICY_BYPASS:
	case IPSEC_POLICY_NONE:
		KEY_SP_UNREF(&sp);
		sp = NULL;		/* NB: force NULL result */
		break;
	case IPSEC_POLICY_IPSEC:
		KASSERT(sp->req != NULL);
		break;
	}

	if (*error != 0) {
		KEY_SP_UNREF(&sp);
		sp = NULL;
		IPSECLOG(LOG_DEBUG, "done, error %d\n", *error);
	}

	return sp;
}

int
ipsec4_output(struct mbuf *m, struct inpcb *inp, int flags,
    u_long *mtu, bool *natt_frag, bool *done)
{
	struct secpolicy *sp = NULL;
	u_long _mtu = 0;
	int error, s;

	/*
	 * Check the security policy (SP) for the packet and, if required,
	 * do IPsec-related processing.  There are two cases here; the first
	 * time a packet is sent through it will be untagged and handled by
	 * ipsec_checkpolicy().  If the packet is resubmitted to ip_output
	 * (e.g. after AH, ESP, etc. processing), there will be a tag to
	 * bypass the lookup and related policy checking.
	 */
	if (ipsec_outdone(m)) {
		return 0;
	}
	s = splsoftnet();
	if (inp && ipsec_pcb_skip_ipsec(inp->inp_sp, IPSEC_DIR_OUTBOUND)) {
		splx(s);
		return 0;
	}
	sp = ipsec_checkpolicy(m, IPSEC_DIR_OUTBOUND, flags, &error, inp);

	/*
	 * There are four return cases:
	 *	sp != NULL                    apply IPsec policy
	 *	sp == NULL, error == 0        no IPsec handling needed
	 *	sp == NULL, error == -EINVAL  discard packet w/o error
	 *	sp == NULL, error != 0        discard packet, report error
	 */
	if (sp == NULL) {
		splx(s);
		if (error) {
			/*
			 * Hack: -EINVAL is used to signal that a packet
			 * should be silently discarded.  This is typically
			 * because we asked key management for an SA and
			 * it was delayed (e.g. kicked up to IKE).
			 */
			if (error == -EINVAL)
				error = 0;
			m_freem(m);
			*done = true;
			return error;
		}
		/* No IPsec processing for this packet. */
		return 0;
	}

	/*
	 * Do delayed checksums now because we send before
	 * this is done in the normal processing path.
	 */
	if (m->m_pkthdr.csum_flags & (M_CSUM_TCPv4|M_CSUM_UDPv4)) {
		in_delayed_cksum(m);
		m->m_pkthdr.csum_flags &= ~(M_CSUM_TCPv4|M_CSUM_UDPv4);
	}

	error = ipsec4_process_packet(m, sp->req, &_mtu);
	if (error == 0 && _mtu != 0) {
		/*
		 * NAT-T ESP fragmentation: do not do IPSec processing
		 * now, we will do it on each fragmented packet.
		 */
		*mtu = _mtu;
		*natt_frag = true;
		KEY_SP_UNREF(&sp);
		splx(s);
		return 0;
	}

	/*
	 * Preserve KAME behaviour: ENOENT can be returned
	 * when an SA acquire is in progress.  Don't propagate
	 * this to user-level; it confuses applications.
	 *
	 * XXX this will go away when the SADB is redone.
	 */
	if (error == ENOENT)
		error = 0;
	KEY_SP_UNREF(&sp);
	splx(s);
	*done = true;
	return error;
}

int
ipsec4_input(struct mbuf *m, int flags)
{
	struct secpolicy *sp;
	int error, s;

	s = splsoftnet();
	error = ipsec_in_reject(m, NULL);
	splx(s);
	if (error) {
		return EINVAL;
	}

	if (flags == 0) {
		/* We are done. */
		return 0;
	}

	/*
	 * Peek at the outbound SP for this packet to determine if
	 * it is a Fast Forward candidate.
	 */
	s = splsoftnet();
	sp = ipsec_checkpolicy(m, IPSEC_DIR_OUTBOUND, flags, &error, NULL);
	if (sp != NULL) {
		m->m_flags &= ~M_CANFASTFWD;
		KEY_SP_UNREF(&sp);
	}
	splx(s);
	return 0;
}

/*
 * If the packet is routed over IPsec tunnel, tell the originator the
 * tunnel MTU.
 *     tunnel MTU = if MTU - sizeof(IP) - ESP/AH hdrsiz
 *
 * XXX: Quick hack!!!
 *
 * XXX: And what if the MTU goes negative?
 */
int
ipsec4_forward(struct mbuf *m, int *destmtu)
{
	struct secpolicy *sp;
	size_t ipsechdr;
	int error;

	sp = ipsec_getpolicybyaddr(m, IPSEC_DIR_OUTBOUND, IP_FORWARDING,
	    &error);
	if (sp == NULL) {
		return EINVAL;
	}

	/* Count IPsec header size. */
	ipsechdr = ipsec_sp_hdrsiz(sp, m);

	/*
	 * Find the correct route for outer IPv4 header, compute tunnel MTU.
	 */
	if (sp->req) {
		struct secasvar *sav;

		sav = ipsec_lookup_sa(sp->req, m);
		if (sav != NULL) {
			struct route *ro;
			struct rtentry *rt;

			ro = &sav->sah->sa_route;
			rt = rtcache_validate(ro);
			if (rt && rt->rt_ifp) {
				*destmtu = rt->rt_rmx.rmx_mtu ?
				    rt->rt_rmx.rmx_mtu : rt->rt_ifp->if_mtu;
				*destmtu -= ipsechdr;
			}
			rtcache_unref(rt, ro);
			KEY_SA_UNREF(&sav);
		}
	}
	KEY_SP_UNREF(&sp);
	return 0;
}

static int
ipsec_setspidx_inpcb(struct mbuf *m, void *pcb)
{
	struct inpcb_hdr *inph = (struct inpcb_hdr *)pcb;
	int error;

	KASSERT(inph != NULL);
	KASSERT(inph->inph_sp != NULL);
	KASSERT(inph->inph_sp->sp_out != NULL);
	KASSERT(inph->inph_sp->sp_in != NULL);

	error = ipsec_setspidx(m, &inph->inph_sp->sp_in->spidx, 1);
	if (error == 0) {
		inph->inph_sp->sp_in->spidx.dir = IPSEC_DIR_INBOUND;
		inph->inph_sp->sp_out->spidx = inph->inph_sp->sp_in->spidx;
		inph->inph_sp->sp_out->spidx.dir = IPSEC_DIR_OUTBOUND;
	} else {
		memset(&inph->inph_sp->sp_in->spidx, 0,
		    sizeof(inph->inph_sp->sp_in->spidx));
		memset(&inph->inph_sp->sp_out->spidx, 0,
		    sizeof(inph->inph_sp->sp_out->spidx));
	}
	return error;
}

/*
 * configure security policy index (src/dst/proto/sport/dport)
 * by looking at the content of mbuf.
 * the caller is responsible for error recovery (like clearing up spidx).
 */
static int
ipsec_setspidx(struct mbuf *m, struct secpolicyindex *spidx, int needport)
{
	struct ip *ip = NULL;
	struct ip ipbuf;
	u_int v;
	struct mbuf *n;
	int len;
	int error;

	KASSERT(m != NULL);

	/*
	 * validate m->m_pkthdr.len.  we see incorrect length if we
	 * mistakenly call this function with inconsistent mbuf chain
	 * (like 4.4BSD tcp/udp processing).
	 *
	 * XXX XXX XXX: We should remove this.
	 */
	len = 0;
	for (n = m; n; n = n->m_next)
		len += n->m_len;
	if (m->m_pkthdr.len != len) {
		KEYDEBUG_PRINTF(KEYDEBUG_IPSEC_DUMP,
		    "total of m_len(%d) != pkthdr.len(%d), ignored.\n",
		    len, m->m_pkthdr.len);
		KASSERTMSG(0, "impossible");
		return EINVAL;
	}

	if (m->m_pkthdr.len < sizeof(struct ip)) {
		KEYDEBUG_PRINTF(KEYDEBUG_IPSEC_DUMP,
		    "pkthdr.len(%d) < sizeof(struct ip), ignored.\n",
		    m->m_pkthdr.len);
		return EINVAL;
	}

	if (m->m_len >= sizeof(*ip)) {
		ip = mtod(m, struct ip *);
	} else {
		m_copydata(m, 0, sizeof(ipbuf), &ipbuf);
		ip = &ipbuf;
	}
	v = ip->ip_v;
	switch (v) {
	case 4:
		error = ipsec4_setspidx_ipaddr(m, spidx);
		if (error)
			return error;
		ipsec4_get_ulp(m, spidx, needport);
		return 0;
#ifdef INET6
	case 6:
		if (m->m_pkthdr.len < sizeof(struct ip6_hdr)) {
			KEYDEBUG_PRINTF(KEYDEBUG_IPSEC_DUMP,
			    "pkthdr.len(%d) < sizeof(struct ip6_hdr), "
			    "ignored.\n", m->m_pkthdr.len);
			return EINVAL;
		}
		error = ipsec6_setspidx_ipaddr(m, spidx);
		if (error)
			return error;
		ipsec6_get_ulp(m, spidx, needport);
		return 0;
#endif
	default:
		KEYDEBUG_PRINTF(KEYDEBUG_IPSEC_DUMP,
		    "unknown IP version %u, ignored.\n", v);
		return EINVAL;
	}
}

static void
ipsec4_get_ulp(struct mbuf *m, struct secpolicyindex *spidx, int needport)
{
	u_int8_t nxt;
	int off;

	KASSERT(m != NULL);
	KASSERTMSG(m->m_pkthdr.len >= sizeof(struct ip), "packet too short");

	/* NB: ip_input() flips it into host endian XXX need more checking */
	if (m->m_len >= sizeof(struct ip)) {
		struct ip *ip = mtod(m, struct ip *);
		if (ip->ip_off & htons(IP_MF | IP_OFFMASK))
			goto done;
		off = ip->ip_hl << 2;
		nxt = ip->ip_p;
	} else {
		struct ip ih;

		m_copydata(m, 0, sizeof(struct ip), &ih);
		if (ih.ip_off & htons(IP_MF | IP_OFFMASK))
			goto done;
		off = ih.ip_hl << 2;
		nxt = ih.ip_p;
	}

	while (off < m->m_pkthdr.len) {
		struct ip6_ext ip6e;
		struct tcphdr th;
		struct udphdr uh;
		struct icmp icmph;

		switch (nxt) {
		case IPPROTO_TCP:
			spidx->ul_proto = nxt;
			if (!needport)
				goto done_proto;
			if (off + sizeof(struct tcphdr) > m->m_pkthdr.len)
				goto done;
			m_copydata(m, off, sizeof(th), &th);
			spidx->src.sin.sin_port = th.th_sport;
			spidx->dst.sin.sin_port = th.th_dport;
			return;
		case IPPROTO_UDP:
			spidx->ul_proto = nxt;
			if (!needport)
				goto done_proto;
			if (off + sizeof(struct udphdr) > m->m_pkthdr.len)
				goto done;
			m_copydata(m, off, sizeof(uh), &uh);
			spidx->src.sin.sin_port = uh.uh_sport;
			spidx->dst.sin.sin_port = uh.uh_dport;
			return;
		case IPPROTO_AH:
			if (off + sizeof(ip6e) > m->m_pkthdr.len)
				goto done;
			/* XXX sigh, this works but is totally bogus */
			m_copydata(m, off, sizeof(ip6e), &ip6e);
			off += (ip6e.ip6e_len + 2) << 2;
			nxt = ip6e.ip6e_nxt;
			break;
		case IPPROTO_ICMP:
			spidx->ul_proto = nxt;
			if (off + sizeof(struct icmp) > m->m_pkthdr.len)
				goto done;
			m_copydata(m, off, sizeof(icmph), &icmph);
			((struct sockaddr_in *)&spidx->src)->sin_port =
			    htons((uint16_t)icmph.icmp_type);
			((struct sockaddr_in *)&spidx->dst)->sin_port =
			    htons((uint16_t)icmph.icmp_code);
			return;
		default:
			/* XXX intermediate headers??? */
			spidx->ul_proto = nxt;
			goto done_proto;
		}
	}
done:
	spidx->ul_proto = IPSEC_ULPROTO_ANY;
done_proto:
	spidx->src.sin.sin_port = IPSEC_PORT_ANY;
	spidx->dst.sin.sin_port = IPSEC_PORT_ANY;
}

static int
ipsec4_setspidx_ipaddr(struct mbuf *m, struct secpolicyindex *spidx)
{
	static const struct sockaddr_in template = {
		sizeof(struct sockaddr_in),
		AF_INET,
		0, { 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 }
	};

	spidx->src.sin = template;
	spidx->dst.sin = template;

	if (m->m_len < sizeof(struct ip)) {
		m_copydata(m, offsetof(struct ip, ip_src),
		    sizeof(struct in_addr), &spidx->src.sin.sin_addr);
		m_copydata(m, offsetof(struct ip, ip_dst),
		    sizeof(struct in_addr), &spidx->dst.sin.sin_addr);
	} else {
		struct ip *ip = mtod(m, struct ip *);
		spidx->src.sin.sin_addr = ip->ip_src;
		spidx->dst.sin.sin_addr = ip->ip_dst;
	}

	spidx->prefs = sizeof(struct in_addr) << 3;
	spidx->prefd = sizeof(struct in_addr) << 3;

	return 0;
}

#ifdef INET6
static void
ipsec6_get_ulp(struct mbuf *m, struct secpolicyindex *spidx, int needport)
{
	int off, nxt;
	struct tcphdr th;
	struct udphdr uh;
	struct icmp6_hdr icmph;

	KASSERT(m != NULL);

	if (KEYDEBUG_ON(KEYDEBUG_IPSEC_DUMP)) {
		kdebug_mbuf(__func__, m);
	}

	/* set default */
	spidx->ul_proto = IPSEC_ULPROTO_ANY;
	((struct sockaddr_in6 *)&spidx->src)->sin6_port = IPSEC_PORT_ANY;
	((struct sockaddr_in6 *)&spidx->dst)->sin6_port = IPSEC_PORT_ANY;

	nxt = -1;
	off = ip6_lasthdr(m, 0, IPPROTO_IPV6, &nxt);
	if (off < 0 || m->m_pkthdr.len < off)
		return;

	switch (nxt) {
	case IPPROTO_TCP:
		spidx->ul_proto = nxt;
		if (!needport)
			break;
		if (off + sizeof(struct tcphdr) > m->m_pkthdr.len)
			break;
		m_copydata(m, off, sizeof(th), &th);
		((struct sockaddr_in6 *)&spidx->src)->sin6_port = th.th_sport;
		((struct sockaddr_in6 *)&spidx->dst)->sin6_port = th.th_dport;
		break;
	case IPPROTO_UDP:
		spidx->ul_proto = nxt;
		if (!needport)
			break;
		if (off + sizeof(struct udphdr) > m->m_pkthdr.len)
			break;
		m_copydata(m, off, sizeof(uh), &uh);
		((struct sockaddr_in6 *)&spidx->src)->sin6_port = uh.uh_sport;
		((struct sockaddr_in6 *)&spidx->dst)->sin6_port = uh.uh_dport;
		break;
	case IPPROTO_ICMPV6:
		spidx->ul_proto = nxt;
		if (off + sizeof(struct icmp6_hdr) > m->m_pkthdr.len)
			break;
		m_copydata(m, off, sizeof(icmph), &icmph);
		((struct sockaddr_in6 *)&spidx->src)->sin6_port =
		    htons((uint16_t)icmph.icmp6_type);
		((struct sockaddr_in6 *)&spidx->dst)->sin6_port =
		    htons((uint16_t)icmph.icmp6_code);
		break;
	default:
		/* XXX intermediate headers??? */
		spidx->ul_proto = nxt;
		break;
	}
}

static int
ipsec6_setspidx_ipaddr(struct mbuf *m, struct secpolicyindex *spidx)
{
	struct ip6_hdr *ip6 = NULL;
	struct ip6_hdr ip6buf;
	struct sockaddr_in6 *sin6;

	if (m->m_len >= sizeof(*ip6)) {
		ip6 = mtod(m, struct ip6_hdr *);
	} else {
		m_copydata(m, 0, sizeof(ip6buf), &ip6buf);
		ip6 = &ip6buf;
	}

	sin6 = (struct sockaddr_in6 *)&spidx->src;
	memset(sin6, 0, sizeof(*sin6));
	sin6->sin6_family = AF_INET6;
	sin6->sin6_len = sizeof(struct sockaddr_in6);
	memcpy(&sin6->sin6_addr, &ip6->ip6_src, sizeof(ip6->ip6_src));
	if (IN6_IS_SCOPE_LINKLOCAL(&ip6->ip6_src)) {
		sin6->sin6_addr.s6_addr16[1] = 0;
		sin6->sin6_scope_id = ntohs(ip6->ip6_src.s6_addr16[1]);
	}
	spidx->prefs = sizeof(struct in6_addr) << 3;

	sin6 = (struct sockaddr_in6 *)&spidx->dst;
	memset(sin6, 0, sizeof(*sin6));
	sin6->sin6_family = AF_INET6;
	sin6->sin6_len = sizeof(struct sockaddr_in6);
	memcpy(&sin6->sin6_addr, &ip6->ip6_dst, sizeof(ip6->ip6_dst));
	if (IN6_IS_SCOPE_LINKLOCAL(&ip6->ip6_dst)) {
		sin6->sin6_addr.s6_addr16[1] = 0;
		sin6->sin6_scope_id = ntohs(ip6->ip6_dst.s6_addr16[1]);
	}
	spidx->prefd = sizeof(struct in6_addr) << 3;

	return 0;
}
#endif

static void
ipsec_delpcbpolicy(struct inpcbpolicy *p)
{

	kmem_intr_free(p, sizeof(*p));
}

/* initialize policy in PCB */
int
ipsec_init_policy(struct socket *so, struct inpcbpolicy **policy)
{
	struct inpcbpolicy *new;

	KASSERT(so != NULL);
	KASSERT(policy != NULL);

	new = kmem_intr_zalloc(sizeof(*new), KM_NOSLEEP);
	if (new == NULL) {
		IPSECLOG(LOG_DEBUG, "No more memory.\n");
		return ENOBUFS;
	}

	if (IPSEC_PRIVILEGED_SO(so))
		new->priv = 1;
	else
		new->priv = 0;

	/*
	 * Set dummy SPs. Actual SPs will be allocated later if needed.
	 */
	new->sp_in = &ipsec_dummy_sp;
	new->sp_out = &ipsec_dummy_sp;

	*policy = new;

	return 0;
}

static void
ipsec_destroy_policy(struct secpolicy *sp)
{

	if (sp == &ipsec_dummy_sp) {
		; /* It's dummy. No need to free it. */
	} else {
		/*
		 * We cannot destroy here because it can be called in
		 * softint. So mark the SP as DEAD and let the timer
		 * destroy it. See key_timehandler_spd.
		 */
		sp->state = IPSEC_SPSTATE_DEAD;
	}
}

int
ipsec_set_policy(void *inp, int optname, const void *request, size_t len,
    kauth_cred_t cred)
{
	struct inpcb_hdr *inph = (struct inpcb_hdr *)inp;
	const struct sadb_x_policy *xpl;
	struct secpolicy *newsp, *oldsp;
	struct secpolicy **policy;
	int error;

	KASSERT(!cpu_softintr_p());
	KASSERT(inph != NULL);
	KASSERT(inph_locked(inph));
	KASSERT(request != NULL);

	if (len < sizeof(*xpl))
		return EINVAL;
	xpl = (const struct sadb_x_policy *)request;

	KASSERT(inph->inph_sp != NULL);

	/* select direction */
	switch (xpl->sadb_x_policy_dir) {
	case IPSEC_DIR_INBOUND:
		policy = &inph->inph_sp->sp_in;
		break;
	case IPSEC_DIR_OUTBOUND:
		policy = &inph->inph_sp->sp_out;
		break;
	default:
		IPSECLOG(LOG_ERR, "invalid direction=%u\n",
		    xpl->sadb_x_policy_dir);
		return EINVAL;
	}

	/* sanity check. */
	if (policy == NULL || *policy == NULL)
		return EINVAL;

	if (KEYDEBUG_ON(KEYDEBUG_IPSEC_DUMP)) {
		kdebug_sadb_xpolicy("set passed policy", request);
	}

	/* check policy type */
	/* ipsec_set_policy() accepts IPSEC, ENTRUST and BYPASS. */
	if (xpl->sadb_x_policy_type == IPSEC_POLICY_DISCARD ||
	    xpl->sadb_x_policy_type == IPSEC_POLICY_NONE)
		return EINVAL;

	/* check privileged socket */
	if (xpl->sadb_x_policy_type == IPSEC_POLICY_BYPASS) {
		error = kauth_authorize_network(cred, KAUTH_NETWORK_IPSEC,
		    KAUTH_REQ_NETWORK_IPSEC_BYPASS, NULL, NULL, NULL);
		if (error)
			return error;
	}

	/* allocation new SP entry */
	if ((newsp = key_msg2sp(xpl, len, &error)) == NULL)
		return error;

	key_init_sp(newsp);
	newsp->created = time_uptime;
	/* Insert the global list for SPs for sockets */
	key_socksplist_add(newsp);

	/* clear old SP and set new SP */
	oldsp = *policy;
	*policy = newsp;
	ipsec_destroy_policy(oldsp);

	if (KEYDEBUG_ON(KEYDEBUG_IPSEC_DUMP)) {
		printf("%s: new policy\n", __func__);
		kdebug_secpolicy(newsp);
	}

	return 0;
}

int
ipsec_get_policy(void *inp, const void *request, size_t len,
    struct mbuf **mp)
{
	struct inpcb_hdr *inph = (struct inpcb_hdr *)inp;
	const struct sadb_x_policy *xpl;
	struct secpolicy *policy;

	/* sanity check. */
	if (inph == NULL || request == NULL || mp == NULL)
		return EINVAL;
	KASSERT(inph->inph_sp != NULL);
	if (len < sizeof(*xpl))
		return EINVAL;
	xpl = (const struct sadb_x_policy *)request;

	/* select direction */
	switch (xpl->sadb_x_policy_dir) {
	case IPSEC_DIR_INBOUND:
		policy = inph->inph_sp->sp_in;
		break;
	case IPSEC_DIR_OUTBOUND:
		policy = inph->inph_sp->sp_out;
		break;
	default:
		IPSECLOG(LOG_ERR, "invalid direction=%u\n",
		    xpl->sadb_x_policy_dir);
		return EINVAL;
	}

	if (policy == NULL)
		return EINVAL;

	*mp = key_sp2msg(policy, M_NOWAIT);
	if (!*mp) {
		IPSECLOG(LOG_DEBUG, "No more memory.\n");
		return ENOBUFS;
	}

	if (KEYDEBUG_ON(KEYDEBUG_IPSEC_DUMP)) {
		kdebug_mbuf(__func__, *mp);
	}

	return 0;
}

int
ipsec_delete_pcbpolicy(void *inp)
{
	struct inpcb_hdr *inph = (struct inpcb_hdr *)inp;

	KASSERT(inph != NULL);

	if (inph->inph_sp == NULL)
		return 0;

	if (inph->inph_sp->sp_in != NULL)
		ipsec_destroy_policy(inph->inph_sp->sp_in);

	if (inph->inph_sp->sp_out != NULL)
		ipsec_destroy_policy(inph->inph_sp->sp_out);

	ipsec_invalpcbcache(inph->inph_sp, IPSEC_DIR_ANY);

	ipsec_delpcbpolicy(inph->inph_sp);
	inph->inph_sp = NULL;

	return 0;
}

/*
 * Return the current level (either IPSEC_LEVEL_USE or IPSEC_LEVEL_REQUIRE).
 */
u_int
ipsec_get_reqlevel(const struct ipsecrequest *isr)
{
	u_int level = 0;
	u_int esp_trans_deflev, esp_net_deflev;
	u_int ah_trans_deflev, ah_net_deflev;

	KASSERT(isr != NULL);
	KASSERT(isr->sp != NULL);
	KASSERTMSG(
	    isr->sp->spidx.src.sa.sa_family == isr->sp->spidx.dst.sa.sa_family,
	    "af family mismatch, src %u, dst %u",
	    isr->sp->spidx.src.sa.sa_family, isr->sp->spidx.dst.sa.sa_family);

/* XXX note that we have ipseclog() expanded here - code sync issue */
#define IPSEC_CHECK_DEFAULT(lev)					\
    (((lev) != IPSEC_LEVEL_USE && (lev) != IPSEC_LEVEL_REQUIRE		\
    && (lev) != IPSEC_LEVEL_UNIQUE) ?					\
	(ipsec_debug ? log(LOG_INFO, "fixed system default level " #lev \
	":%d->%d\n", (lev), IPSEC_LEVEL_REQUIRE) : (void)0),		\
	(lev) = IPSEC_LEVEL_REQUIRE, (lev)				\
    : (lev))

	/* set default level */
	switch (((struct sockaddr *)&isr->sp->spidx.src)->sa_family) {
#ifdef INET
	case AF_INET:
		esp_trans_deflev = IPSEC_CHECK_DEFAULT(ip4_esp_trans_deflev);
		esp_net_deflev = IPSEC_CHECK_DEFAULT(ip4_esp_net_deflev);
		ah_trans_deflev = IPSEC_CHECK_DEFAULT(ip4_ah_trans_deflev);
		ah_net_deflev = IPSEC_CHECK_DEFAULT(ip4_ah_net_deflev);
		break;
#endif
#ifdef INET6
	case AF_INET6:
		esp_trans_deflev = IPSEC_CHECK_DEFAULT(ip6_esp_trans_deflev);
		esp_net_deflev = IPSEC_CHECK_DEFAULT(ip6_esp_net_deflev);
		ah_trans_deflev = IPSEC_CHECK_DEFAULT(ip6_ah_trans_deflev);
		ah_net_deflev = IPSEC_CHECK_DEFAULT(ip6_ah_net_deflev);
		break;
#endif
	default:
		panic("%s: unknown af %u", __func__,
		    isr->sp->spidx.src.sa.sa_family);
	}

#undef IPSEC_CHECK_DEFAULT

	/* set level */
	switch (isr->level) {
	case IPSEC_LEVEL_DEFAULT:
		switch (isr->saidx.proto) {
		case IPPROTO_ESP:
			if (isr->saidx.mode == IPSEC_MODE_TUNNEL)
				level = esp_net_deflev;
			else
				level = esp_trans_deflev;
			break;
		case IPPROTO_AH:
			if (isr->saidx.mode == IPSEC_MODE_TUNNEL)
				level = ah_net_deflev;
			else
				level = ah_trans_deflev;
			break;
		case IPPROTO_IPCOMP:
			/*
			 * we don't really care, as IPcomp document says that
			 * we shouldn't compress small packets
			 */
			level = IPSEC_LEVEL_USE;
			break;
		default:
			panic("%s: Illegal protocol defined %u", __func__,
			    isr->saidx.proto);
		}
		break;

	case IPSEC_LEVEL_USE:
	case IPSEC_LEVEL_REQUIRE:
		level = isr->level;
		break;
	case IPSEC_LEVEL_UNIQUE:
		level = IPSEC_LEVEL_REQUIRE;
		break;

	default:
		panic("%s: Illegal IPsec level %u", __func__, isr->level);
	}

	return level;
}

/*
 * Check security policy requirements against the actual packet contents.
 *
 * If the SP requires an IPsec packet, and the packet was neither AH nor ESP,
 * then kick it.
 */
static int
ipsec_sp_reject(const struct secpolicy *sp, const struct mbuf *m)
{
	struct ipsecrequest *isr;

	if (KEYDEBUG_ON(KEYDEBUG_IPSEC_DATA)) {
		printf("%s: using SP\n", __func__);
		kdebug_secpolicy(sp);
	}

	/* check policy */
	switch (sp->policy) {
	case IPSEC_POLICY_DISCARD:
		return 1;
	case IPSEC_POLICY_BYPASS:
	case IPSEC_POLICY_NONE:
		return 0;
	}

	KASSERTMSG(sp->policy == IPSEC_POLICY_IPSEC,
	    "invalid policy %u", sp->policy);

	/* XXX should compare policy against ipsec header history */

	for (isr = sp->req; isr != NULL; isr = isr->next) {
		if (ipsec_get_reqlevel(isr) != IPSEC_LEVEL_REQUIRE)
			continue;
		switch (isr->saidx.proto) {
		case IPPROTO_ESP:
			if ((m->m_flags & M_DECRYPTED) == 0) {
				KEYDEBUG_PRINTF(KEYDEBUG_IPSEC_DUMP,
				    "ESP m_flags:%x\n", m->m_flags);
				return 1;
			}
			break;
		case IPPROTO_AH:
			if ((m->m_flags & M_AUTHIPHDR) == 0) {
				KEYDEBUG_PRINTF(KEYDEBUG_IPSEC_DUMP,
				    "AH m_flags:%x\n", m->m_flags);
				return 1;
			}
			break;
		case IPPROTO_IPCOMP:
			/*
			 * We don't really care, as IPcomp document
			 * says that we shouldn't compress small
			 * packets, IPComp policy should always be
			 * treated as being in "use" level.
			 */
			break;
		}
	}

	return 0;
}

/*
 * Check security policy requirements.
 */
int
ipsec_in_reject(struct mbuf *m, void *inp)
{
	struct inpcb_hdr *inph = (struct inpcb_hdr *)inp;
	struct secpolicy *sp;
	int error;
	int result;

	KASSERT(m != NULL);

	if (inph == NULL)
		sp = ipsec_getpolicybyaddr(m, IPSEC_DIR_INBOUND,
		    IP_FORWARDING, &error);
	else
		sp = ipsec_getpolicybysock(m, IPSEC_DIR_INBOUND,
		    inph, &error);

	if (sp != NULL) {
		result = ipsec_sp_reject(sp, m);
		if (result)
			IPSEC_STATINC(IPSEC_STAT_IN_POLVIO);
		KEY_SP_UNREF(&sp);
	} else {
		result = 0;
	}
	return result;
}

/*
 * Compute the byte size to be occupied by the IPsec header. If it is
 * tunneled, it includes the size of outer IP header.
 */
static size_t
ipsec_sp_hdrsiz(const struct secpolicy *sp, const struct mbuf *m)
{
	struct ipsecrequest *isr;
	size_t siz;

	if (KEYDEBUG_ON(KEYDEBUG_IPSEC_DATA)) {
		printf("%s: using SP\n", __func__);
		kdebug_secpolicy(sp);
	}

	switch (sp->policy) {
	case IPSEC_POLICY_DISCARD:
	case IPSEC_POLICY_BYPASS:
	case IPSEC_POLICY_NONE:
		return 0;
	}

	KASSERTMSG(sp->policy == IPSEC_POLICY_IPSEC,
	    "invalid policy %u", sp->policy);

	siz = 0;
	for (isr = sp->req; isr != NULL; isr = isr->next) {
		size_t clen = 0;
		struct secasvar *sav;

		switch (isr->saidx.proto) {
		case IPPROTO_ESP:
			sav = ipsec_lookup_sa(isr, m);
			if (sav != NULL) {
				clen = esp_hdrsiz(sav);
				KEY_SA_UNREF(&sav);
			} else
				clen = esp_hdrsiz(NULL);
			break;
		case IPPROTO_AH:
			sav = ipsec_lookup_sa(isr, m);
			if (sav != NULL) {
				clen = ah_hdrsiz(sav);
				KEY_SA_UNREF(&sav);
			} else
				clen = ah_hdrsiz(NULL);
			break;
		case IPPROTO_IPCOMP:
			clen = sizeof(struct ipcomp);
			break;
		}

		if (isr->saidx.mode == IPSEC_MODE_TUNNEL) {
			switch (isr->saidx.dst.sa.sa_family) {
			case AF_INET:
				clen += sizeof(struct ip);
				break;
#ifdef INET6
			case AF_INET6:
				clen += sizeof(struct ip6_hdr);
				break;
#endif
			default:
				IPSECLOG(LOG_ERR, "unknown AF %d in "
				    "IPsec tunnel SA\n",
				    ((const struct sockaddr *)&isr->saidx.dst)
				    ->sa_family);
				break;
			}
		}
		siz += clen;
	}

	return siz;
}

size_t
ipsec_hdrsiz(struct mbuf *m, u_int dir, void *inp)
{
	struct inpcb_hdr *inph = (struct inpcb_hdr *)inp;
	struct secpolicy *sp;
	int error;
	size_t size;

	KASSERT(m != NULL);
	KASSERTMSG(inph == NULL || inph->inph_socket != NULL,
	    "socket w/o inpcb");

	if (inph == NULL)
		sp = ipsec_getpolicybyaddr(m, dir, IP_FORWARDING, &error);
	else
		sp = ipsec_getpolicybysock(m, dir, inph, &error);

	if (sp != NULL) {
		size = ipsec_sp_hdrsiz(sp, m);
		KEYDEBUG_PRINTF(KEYDEBUG_IPSEC_DATA, "size:%zu.\n", size);
		KEY_SP_UNREF(&sp);
	} else {
		size = 0;
	}

	return size;
}

/*
 * Check the variable replay window.
 * ipsec_chkreplay() performs replay check before ICV verification.
 * ipsec_updatereplay() updates replay bitmap.  This must be called after
 * ICV verification (it also performs replay check, which is usually done
 * beforehand).
 * 0 (zero) is returned if packet disallowed, 1 if packet permitted.
 *
 * based on RFC 2401.
 */
int
ipsec_chkreplay(u_int32_t seq, const struct secasvar *sav)
{
	const struct secreplay *replay;
	u_int32_t diff;
	int fr;
	u_int32_t wsizeb;	/* constant: bits of window size */
	int frlast;		/* constant: last frame */

	IPSEC_SPLASSERT_SOFTNET(__func__);

	KASSERT(sav != NULL);
	KASSERT(sav->replay != NULL);

	replay = sav->replay;

	if (replay->wsize == 0)
		return 1;	/* no need to check replay. */

	/* constant */
	frlast = replay->wsize - 1;
	wsizeb = replay->wsize << 3;

	/* sequence number of 0 is invalid */
	if (seq == 0)
		return 0;

	/* first time is always okay */
	if (replay->count == 0)
		return 1;

	if (seq > replay->lastseq) {
		/* larger sequences are okay */
		return 1;
	} else {
		/* seq is equal or less than lastseq. */
		diff = replay->lastseq - seq;

		/* over range to check, i.e. too old or wrapped */
		if (diff >= wsizeb)
			return 0;

		fr = frlast - diff / 8;

		/* this packet already seen ? */
		if ((replay->bitmap)[fr] & (1 << (diff % 8)))
			return 0;

		/* out of order but good */
		return 1;
	}
}

/*
 * check replay counter whether to update or not.
 * OUT:	0:	OK
 *	1:	NG
 */
int
ipsec_updatereplay(u_int32_t seq, const struct secasvar *sav)
{
	struct secreplay *replay;
	u_int32_t diff;
	int fr;
	u_int32_t wsizeb;	/* constant: bits of window size */
	int frlast;		/* constant: last frame */

	IPSEC_SPLASSERT_SOFTNET(__func__);

	KASSERT(sav != NULL);
	KASSERT(sav->replay != NULL);

	replay = sav->replay;

	if (replay->wsize == 0)
		goto ok;	/* no need to check replay. */

	/* constant */
	frlast = replay->wsize - 1;
	wsizeb = replay->wsize << 3;

	/* sequence number of 0 is invalid */
	if (seq == 0)
		return 1;

	/* first time */
	if (replay->count == 0) {
		replay->lastseq = seq;
		memset(replay->bitmap, 0, replay->wsize);
		(replay->bitmap)[frlast] = 1;
		goto ok;
	}

	if (seq > replay->lastseq) {
		/* seq is larger than lastseq. */
		diff = seq - replay->lastseq;

		/* new larger sequence number */
		if (diff < wsizeb) {
			/* In window */
			/* set bit for this packet */
			vshiftl(replay->bitmap, diff, replay->wsize);
			(replay->bitmap)[frlast] |= 1;
		} else {
			/* this packet has a "way larger" */
			memset(replay->bitmap, 0, replay->wsize);
			(replay->bitmap)[frlast] = 1;
		}
		replay->lastseq = seq;

		/* larger is good */
	} else {
		/* seq is equal or less than lastseq. */
		diff = replay->lastseq - seq;

		/* over range to check, i.e. too old or wrapped */
		if (diff >= wsizeb)
			return 1;

		fr = frlast - diff / 8;

		/* this packet already seen ? */
		if ((replay->bitmap)[fr] & (1 << (diff % 8)))
			return 1;

		/* mark as seen */
		(replay->bitmap)[fr] |= (1 << (diff % 8));

		/* out of order but good */
	}

ok:
	if (replay->count == ~0) {
		char buf[IPSEC_LOGSASTRLEN];

		/* set overflow flag */
		replay->overflow++;

		/* don't increment, no more packets accepted */
		if ((sav->flags & SADB_X_EXT_CYCSEQ) == 0)
			return 1;

		IPSECLOG(LOG_WARNING, "replay counter made %d cycle. %s\n",
		    replay->overflow, ipsec_logsastr(sav, buf, sizeof(buf)));
	}

	replay->count++;

	return 0;
}

/*
 * shift variable length buffer to left.
 * IN:	bitmap: pointer to the buffer
 *	nbit:	the number of to shift.
 *	wsize:	buffer size (bytes).
 */
static void
vshiftl(unsigned char *bitmap, int nbit, int wsize)
{
	int s, j, i;
	unsigned char over;

	for (j = 0; j < nbit; j += 8) {
		s = (nbit - j < 8) ? (nbit - j): 8;
		bitmap[0] <<= s;
		for (i = 1; i < wsize; i++) {
			over = (bitmap[i] >> (8 - s));
			bitmap[i] <<= s;
			bitmap[i-1] |= over;
		}
	}

	return;
}

/* Return a printable string for the address. */
const char *
ipsec_address(const union sockaddr_union *sa, char *buf, size_t size)
{
	switch (sa->sa.sa_family) {
	case AF_INET:
		in_print(buf, size, &sa->sin.sin_addr);
		return buf;
#if INET6
	case AF_INET6:
		in6_print(buf, size, &sa->sin6.sin6_addr);
		return buf;
#endif
	default:
		return "(unknown address family)";
	}
}

const char *
ipsec_logsastr(const struct secasvar *sav, char *buf, size_t size)
{
	const struct secasindex *saidx = &sav->sah->saidx;
	char sbuf[IPSEC_ADDRSTRLEN], dbuf[IPSEC_ADDRSTRLEN];

	KASSERTMSG(saidx->src.sa.sa_family == saidx->dst.sa.sa_family,
	    "af family mismatch, src %u, dst %u",
	    saidx->src.sa.sa_family, saidx->dst.sa.sa_family);

	snprintf(buf, size, "SA(SPI=%u src=%s dst=%s)",
	    (u_int32_t)ntohl(sav->spi),
	    ipsec_address(&saidx->src, sbuf, sizeof(sbuf)),
	    ipsec_address(&saidx->dst, dbuf, sizeof(dbuf)));

	return buf;
}

void
ipsec_dumpmbuf(struct mbuf *m)
{
	int totlen;
	int i;
	u_char *p;

	totlen = 0;
	printf("---\n");
	while (m) {
		p = mtod(m, u_char *);
		for (i = 0; i < m->m_len; i++) {
			printf("%02x ", p[i]);
			totlen++;
			if (totlen % 16 == 0)
				printf("\n");
		}
		m = m->m_next;
	}
	if (totlen % 16 != 0)
		printf("\n");
	printf("---\n");
}

#ifdef INET6
struct secpolicy *
ipsec6_check_policy(struct mbuf *m, struct in6pcb *in6p, int flags,
    int *needipsecp, int *errorp)
{
	struct secpolicy *sp = NULL;
	int s;
	int error = 0;
	int needipsec = 0;

	if (ipsec_outdone(m)) {
		goto skippolicycheck;
	}
	s = splsoftnet();
	if (in6p && ipsec_pcb_skip_ipsec(in6p->in6p_sp, IPSEC_DIR_OUTBOUND)) {
		splx(s);
		goto skippolicycheck;
	}
	sp = ipsec_checkpolicy(m, IPSEC_DIR_OUTBOUND, flags, &error, in6p);
	splx(s);

	/*
	 * There are four return cases:
	 *	sp != NULL                    apply IPsec policy
	 *	sp == NULL, error == 0        no IPsec handling needed
	 *	sp == NULL, error == -EINVAL  discard packet w/o error
	 *	sp == NULL, error != 0        discard packet, report error
	 */
	if (sp == NULL) {
		needipsec = 0;
	} else {
		needipsec = 1;
	}

skippolicycheck:
	*errorp = error;
	*needipsecp = needipsec;
	return sp;
}

int
ipsec6_input(struct mbuf *m)
{
	int s, error;

	s = splsoftnet();
	error = ipsec_in_reject(m, NULL);
	splx(s);
	if (error) {
		return EINVAL;
	}

	return 0;
}
#endif /* INET6 */

/*
 * -----------------------------------------------------------------------------
 */

/* XXX this stuff doesn't belong here... */

static struct xformsw *xforms = NULL;

/*
 * Register a transform; typically at system startup.
 */
void
xform_register(struct xformsw *xsp)
{
	xsp->xf_next = xforms;
	xforms = xsp;
}

/*
 * Initialize transform support in an sav.
 */
int
xform_init(struct secasvar *sav, int xftype)
{
	struct xformsw *xsp;

	if (sav->tdb_xform != NULL)	/* previously initialized */
		return 0;
	for (xsp = xforms; xsp; xsp = xsp->xf_next)
		if (xsp->xf_type == xftype)
			return (*xsp->xf_init)(sav, xsp);

	IPSECLOG(LOG_DEBUG, "no match for xform type %d\n", xftype);
	return EINVAL;
}

void
nat_t_ports_get(struct mbuf *m, u_int16_t *dport, u_int16_t *sport)
{
	struct m_tag *tag;

	if ((tag = m_tag_find(m, PACKET_TAG_IPSEC_NAT_T_PORTS, NULL))) {
		*sport = ((u_int16_t *)(tag + 1))[0];
		*dport = ((u_int16_t *)(tag + 1))[1];
	} else
		*sport = *dport = 0;
}

/*
 * XXXJRT This should be done as a protosw init call.
 */
void
ipsec_attach(void)
{

	ipsec_output_init();

	ipsecstat_percpu = percpu_alloc(sizeof(uint64_t) * IPSEC_NSTATS);

	sysctl_net_inet_ipsec_setup(NULL);
#ifdef INET6
	sysctl_net_inet6_ipsec6_setup(NULL);
#endif

	ah_attach();
	esp_attach();
	ipcomp_attach();
	ipe4_attach();
#ifdef TCP_SIGNATURE
	tcpsignature_attach();
#endif
}
