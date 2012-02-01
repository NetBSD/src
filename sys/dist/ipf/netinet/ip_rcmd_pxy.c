/*	$NetBSD: ip_rcmd_pxy.c,v 1.14 2012/02/01 02:21:20 christos Exp $	*/

/*
 * Copyright (C) 2011 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: ip_rcmd_pxy.c,v 1.65.2.2 2012/01/26 05:29:13 darrenr Exp
 *
 * Simple RCMD transparent proxy for in-kernel use.  For use with the NAT
 * code.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: ip_rcmd_pxy.c,v 1.14 2012/02/01 02:21:20 christos Exp $");

#define	IPF_RCMD_PROXY

void ipf_p_rcmd_main_load(void);
void ipf_p_rcmd_main_unload(void);

int ipf_p_rcmd_init(void);
void ipf_p_rcmd_fini(void);
void ipf_p_rcmd_del(ipf_main_softc_t *, ap_session_t *);
int ipf_p_rcmd_new(void *, fr_info_t *, ap_session_t *, nat_t *);
int ipf_p_rcmd_out(void *, fr_info_t *, ap_session_t *, nat_t *);
int ipf_p_rcmd_in(void *, fr_info_t *, ap_session_t *, nat_t *);
u_short ipf_rcmd_atoi(char *);
int ipf_p_rcmd_portmsg(fr_info_t *, ap_session_t *, nat_t *);

static	frentry_t	rcmdfr;

static	int		rcmd_proxy_init = 0;


/*
 * RCMD application proxy initialization.
 */
void
ipf_p_rcmd_main_load()
{
	bzero((char *)&rcmdfr, sizeof(rcmdfr));
	rcmdfr.fr_ref = 1;
	rcmdfr.fr_flags = FR_INQUE|FR_PASS|FR_QUICK|FR_KEEPSTATE;
	MUTEX_INIT(&rcmdfr.fr_lock, "RCMD proxy rule lock");
	rcmd_proxy_init = 1;
}


void
ipf_p_rcmd_main_unload()
{
	if (rcmd_proxy_init == 1) {
		MUTEX_DESTROY(&rcmdfr.fr_lock);
		rcmd_proxy_init = 0;
	}
}


/*
 * Setup for a new RCMD proxy.
 */
int
ipf_p_rcmd_new(arg, fin, aps, nat)
	void *arg;
	fr_info_t *fin;
	ap_session_t *aps;
	nat_t *nat;
{
	tcphdr_t *tcp = (tcphdr_t *)fin->fin_dp;
	rcmdinfo_t *rc;
	ipnat_t *ipn;

	fin = fin;	/* LINT */
	nat = nat;	/* LINT */

	aps->aps_psiz = sizeof(rcmdinfo_t) + nat->nat_ptr->in_namelen + 1;
	KMALLOCS(rc, rcmdinfo_t *, aps->aps_psiz);
	if (rc == NULL) {
		printf("ipf_p_rcmd_new:KMALLOCS(%zu) failed\n", sizeof(*rc));
		return -1;
	}

	aps->aps_data = rc;
	bzero((char *)rc, sizeof(*rc));
	aps->aps_sport = tcp->th_sport;
	aps->aps_dport = tcp->th_dport;

	ipn = &rc->rcmd_rule;
	ipn->in_ifps[0] = nat->nat_ifps[0];
	ipn->in_ifps[1] = nat->nat_ifps[1];
	ipn->in_apr = NULL;
	ipn->in_use = 1;
	ipn->in_hits = 1;
	ipn->in_ippip = 1;

	if ((nat->nat_ptr->in_redir & NAT_REDIRECT) != 0) {
		ipn->in_redir = NAT_MAP;
		ipn->in_snip = ntohl(nat->nat_odstaddr);
		ipn->in_nsrcaddr = nat->nat_odstaddr;
		ipn->in_dnip = ntohl(nat->nat_nsrcaddr);
		ipn->in_ndstaddr = nat->nat_nsrcaddr;
		ipn->in_osrcaddr = nat->nat_ndstaddr;
		ipn->in_odstaddr = nat->nat_osrcaddr;
	} else {
		ipn->in_redir = NAT_REDIRECT;
		ipn->in_snip = ntohl(nat->nat_odstaddr);
		ipn->in_nsrcaddr = nat->nat_odstaddr;
		ipn->in_dnip = ntohl(nat->nat_osrcaddr);
		ipn->in_ndstaddr = nat->nat_osrcaddr;
		ipn->in_osrcaddr = nat->nat_ndstaddr;
		ipn->in_odstaddr = nat->nat_nsrcaddr;
	}

	ipn->in_osrcmsk = 0xffffffff;
	ipn->in_nsrcmsk = 0xffffffff;
	ipn->in_odstmsk = 0xffffffff;
	ipn->in_ndstmsk = 0xffffffff;
	ipn->in_pr[0] = IPPROTO_TCP;
	ipn->in_pr[1] = IPPROTO_TCP;
	MUTEX_INIT(&ipn->in_lock, "rcmd proxy NAT rule");

	ipn->in_namelen = nat->nat_ptr->in_namelen;
	bcopy(nat->nat_ptr->in_names, ipn->in_ifnames, ipn->in_namelen);
	ipn->in_ifnames[0] = nat->nat_ptr->in_ifnames[0];
	ipn->in_ifnames[1] = nat->nat_ptr->in_ifnames[1];

	return 0;
}


void
ipf_p_rcmd_del(softc, aps)
	ipf_main_softc_t *softc;
	ap_session_t *aps;
{
	rcmdinfo_t *rci;

	rci = aps->aps_data;
	if (rci != NULL) {
		MUTEX_DESTROY(&rci->rcmd_rule.in_lock);
	}
}


/*
 * ipf_rcmd_atoi - implement a simple version of atoi
 */
u_short
ipf_rcmd_atoi(ptr)
	char *ptr;
{
	register char *s = ptr, c;
	register u_short i = 0;

	while (((c = *s++) != '\0') && ISDIGIT(c)) {
		i *= 10;
		i += c - '0';
	}
	return i;
}


int
ipf_p_rcmd_portmsg(fin, aps, nat)
	fr_info_t *fin;
	ap_session_t *aps;
	nat_t *nat;
{
	tcphdr_t *tcp, tcph, *tcp2 = &tcph;
	int off, dlen, nflags, direction;
	ipf_main_softc_t *softc;
	char portbuf[8], *s;
	rcmdinfo_t *rc;
	fr_info_t fi;
	u_short sp;
	nat_t *nat2;
	ip_t *ip;
	mb_t *m;

	tcp = (tcphdr_t *)fin->fin_dp;

	m = fin->fin_m;
	ip = fin->fin_ip;
	off = (char *)tcp - (char *)ip + (TCP_OFF(tcp) << 2) + fin->fin_ipoff;

#ifdef __sgi
	dlen = fin->fin_plen - off;
#else
	dlen = MSGDSIZE(m) - off;
#endif
	if (dlen <= 0)
		return 0;

	rc = (rcmdinfo_t *)aps->aps_data;
	if ((rc->rcmd_portseq != 0) &&
	    (tcp->th_seq != rc->rcmd_portseq))
		return 0;

	bzero(portbuf, sizeof(portbuf));
	COPYDATA(m, off, MIN(sizeof(portbuf), dlen), portbuf);

	portbuf[sizeof(portbuf) - 1] = '\0';
	s = portbuf;
	sp = ipf_rcmd_atoi(s);
	if (sp == 0) {
#ifdef IP_RCMD_PROXY_DEBUG
		printf("ipf_p_rcmd_portmsg:sp == 0 dlen %d [%s]\n",
		       dlen, portbuf);
#endif
		return 0;
	}

	if (rc->rcmd_port != 0 && sp != rc->rcmd_port) {
#ifdef IP_RCMD_PROXY_DEBUG
		printf("ipf_p_rcmd_portmsg:sp(%d) != rcmd_port(%d)\n",
		       sp, rc->rcmd_port);
#endif
		return 0;
	}

	rc->rcmd_port = sp;
	rc->rcmd_portseq = tcp->th_seq;

	/*
	 * Initialise the packet info structure so we can search the NAT
	 * table to see if there already is soemthing present that matches
	 * up with what we want to add.
	 */
	bcopy((char *)fin, (char *)&fi, sizeof(fi));
	fi.fin_flx |= FI_IGNORE;
	fi.fin_data[0] = 0;
	fi.fin_data[1] = sp;
	fi.fin_fi.fi_saddr = nat->nat_ndstaddr;
	fi.fin_fi.fi_daddr = nat->nat_nsrcaddr;

	if (nat->nat_dir == NAT_OUTBOUND) {
		nat2 = ipf_nat_outlookup(&fi, NAT_SEARCH|IPN_TCP,
					 nat->nat_pr[1],
					 nat->nat_osrcip, nat->nat_odstip);
	} else {
		nat2 = ipf_nat_inlookup(&fi, NAT_SEARCH|IPN_TCP,
					nat->nat_pr[0],
					nat->nat_osrcip, nat->nat_odstip);
	}

	softc = fin->fin_main_soft;
	if (nat2 == NULL) {
#ifdef USE_MUTEXES
		ipf_nat_softc_t *softn = softc->ipf_nat_soft;
#endif

		/*
		 * Add skeleton NAT entry for connection which will come
		 * back the other way.
		 */
		int slen;

		slen = ip->ip_len;
		ip->ip_len = htons(fin->fin_hlen + sizeof(*tcp));

		/*
		 * Fill out the fake TCP header with a few fields that ipfilter
		 * considers to be important.
		 */
		bzero((char *)tcp2, sizeof(*tcp2));
		tcp2->th_win = htons(8192);
		TCP_OFF_A(tcp2, 5);
		tcp2->th_flags = TH_SYN;

		fi.fin_dp = (char *)tcp2;
		fi.fin_fr = &rcmdfr;
		fi.fin_dlen = sizeof(*tcp2);
		fi.fin_plen = fi.fin_hlen + sizeof(*tcp2);

		if (nat->nat_dir == NAT_OUTBOUND) {
			fi.fin_out = 0;
			direction = NAT_INBOUND;
		} else {
			fi.fin_out = 1;
			direction = NAT_OUTBOUND;
		}
		nflags = SI_W_SPORT;

		fi.fin_flx &= FI_LOWTTL|FI_FRAG|FI_TCPUDP|FI_OPTIONS|FI_IGNORE;

		nflags |= NAT_SLAVE|IPN_TCP;
		MUTEX_ENTER(&softn->ipf_nat_new);
		nat2 = ipf_nat_add(&fi, &rc->rcmd_rule, NULL, nflags,
				   direction);
		MUTEX_EXIT(&softn->ipf_nat_new);

		if (nat2 != NULL) {
			(void) ipf_nat_proto(&fi, nat2, IPN_TCP);
			MUTEX_ENTER(&nat2->nat_lock);
			ipf_nat_update(&fi, nat2);
			MUTEX_EXIT(&nat2->nat_lock);
			fi.fin_ifp = NULL;
			if (nat2->nat_dir == NAT_INBOUND)
				fi.fin_fi.fi_daddr = nat->nat_osrcaddr;
			(void) ipf_state_add(softc, &fi, NULL, SI_W_SPORT);
		}
		ip->ip_len = slen;
	}
	return 0;
}


int
ipf_p_rcmd_out(arg, fin, aps, nat)
	void *arg;
	fr_info_t *fin;
	ap_session_t *aps;
	nat_t *nat;
{
	if (nat->nat_dir == NAT_OUTBOUND)
		return ipf_p_rcmd_portmsg(fin, aps, nat);
	return 0;
}


int
ipf_p_rcmd_in(arg, fin, aps, nat)
	void *arg;
	fr_info_t *fin;
	ap_session_t *aps;
	nat_t *nat;
{
	if (nat->nat_dir == NAT_INBOUND)
		return ipf_p_rcmd_portmsg(fin, aps, nat);
	return 0;
}
