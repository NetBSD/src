/*	$NetBSD: ip_pptp_pxy.c,v 1.1.1.1 2004/03/28 08:56:47 martti Exp $	*/

/*
 * Copyright (C) 2002-2003 by Darren Reed
 *
 * Simple PPTP transparent proxy for in-kernel use.  For use with the NAT
 * code.
 *
 * Id: ip_pptp_pxy.c,v 2.10.2.3 2004/03/14 13:11:37 darrenr Exp
 *
 */
#define	IPF_PPTP_PROXY

typedef	struct pptp_pxy {
	ipnat_t		pptp_rule;
	nat_t		*pptp_nat;
	ipstate_t	*pptp_state;
	int		pptp_seencookie;
	u_32_t		pptp_cookie;
} pptp_pxy_t;


int ippr_pptp_init __P((void));
void ippr_pptp_fini __P((void));
int ippr_pptp_new __P((fr_info_t *, ap_session_t *, nat_t *));
void ippr_pptp_del __P((ap_session_t *));
int ippr_pptp_inout __P((fr_info_t *, ap_session_t *, nat_t *));
int ippr_pptp_match __P((fr_info_t *, ap_session_t *, nat_t *));

static	frentry_t	pptpfr;

int	pptp_proxy_init = 0;


/*
 * PPTP application proxy initialization.
 */
int ippr_pptp_init()
{
	bzero((char *)&pptpfr, sizeof(pptpfr));
	pptpfr.fr_ref = 1;
	pptpfr.fr_flags = FR_OUTQUE|FR_PASS|FR_QUICK|FR_KEEPSTATE;
	MUTEX_INIT(&pptpfr.fr_lock, "PPTP proxy rule lock");
	pptp_proxy_init = 1;

	return 0;
}


void ippr_pptp_fini()
{
	if (pptp_proxy_init == 1) {
		MUTEX_DESTROY(&pptpfr.fr_lock);
		pptp_proxy_init = 0;
	}
}


/*
 * Setup for a new PPTP proxy.
 */
int ippr_pptp_new(fin, aps, nat)
fr_info_t *fin;
ap_session_t *aps;
nat_t *nat;
{
	pptp_pxy_t *pptp;
	fr_info_t fi;
	ipnat_t *ipn;
	nat_t *nat2;
	int p, off;
	ip_t *ip;

	ip = fin->fin_ip;
	off = fin->fin_hlen + sizeof(udphdr_t);

	if (nat_outlookup(fin, 0, IPPROTO_GRE, nat->nat_inip,
			  ip->ip_dst) != NULL)
		return -1;

	aps->aps_psiz = sizeof(*pptp);
	KMALLOCS(aps->aps_data, pptp_pxy_t *, sizeof(*pptp));
	if (aps->aps_data == NULL)
		return -1;

	ip = fin->fin_ip;
	pptp = aps->aps_data;
	bzero((char *)pptp, sizeof(*pptp));

	/*
	 * Create NAT rule against which the tunnel/transport mapping is
	 * created.  This is required because the current NAT rule does not
	 * describe GRE but TCP instead.
	 */
	ipn = &pptp->pptp_rule;
	ipn->in_ifps[0] = fin->fin_ifp;
	ipn->in_apr = NULL;
	ipn->in_use = 1;
	ipn->in_hits = 1;
	ipn->in_nip = nat->nat_outip.s_addr;
	ipn->in_ippip = 1;
	ipn->in_inip = nat->nat_inip.s_addr;
	ipn->in_inmsk = 0xffffffff;
	ipn->in_outip = fin->fin_saddr;
	ipn->in_outmsk = nat->nat_outip.s_addr;
	ipn->in_srcip = fin->fin_saddr;
	ipn->in_srcmsk = 0xffffffff;
	ipn->in_redir = NAT_MAP;
	bcopy(nat->nat_ptr->in_ifnames[0], ipn->in_ifnames[0],
	      sizeof(ipn->in_ifnames[0]));
	ipn->in_p = IPPROTO_GRE;

	bcopy((char *)fin, (char *)&fi, sizeof(fi));
	fi.fin_fi.fi_p = IPPROTO_GRE;
	fi.fin_fr = &pptpfr;
	fi.fin_data[0] = 0;
	fi.fin_data[1] = 0;
	p = ip->ip_p;
	ip->ip_p = IPPROTO_GRE;
	fi.fin_flx &= ~FI_TCPUDP;
	fi.fin_flx |= FI_IGNORE;

	nat2 = nat_new(&fi, ipn, &pptp->pptp_nat, 0, NAT_OUTBOUND);
	pptp->pptp_nat = nat2;
	if (nat2 != NULL) {
		(void) nat_proto(&fi, nat2, 0);
		nat_update(&fi, nat2, nat2->nat_ptr);

		fi.fin_data[0] = 0;
		fi.fin_data[1] = 0;
		pptp->pptp_state = fr_addstate(&fi, &pptp->pptp_state, 0);
	}
	ip->ip_p = p;
	return 0;
}


/*
 * For outgoing PPTP packets.  refresh timeouts for NAT & state entries, if
 * we can.  If they have disappeared, recreate them.
 */
int ippr_pptp_inout(fin, aps, nat)
fr_info_t *fin;
ap_session_t *aps;
nat_t *nat;
{
	pptp_pxy_t *pptp;
	fr_info_t fi;
	nat_t *nat2;
	ip_t *ip;
	int p;

	if ((fin->fin_out == 1) && (nat->nat_dir == NAT_INBOUND))
		return 0;

	if ((fin->fin_out == 0) && (nat->nat_dir == NAT_OUTBOUND))
		return 0;

	pptp = aps->aps_data;

	if (pptp != NULL) {
		ip = fin->fin_ip;
		p = ip->ip_p;

		if ((pptp->pptp_nat == NULL) || (pptp->pptp_state == NULL)) {
			bcopy((char *)fin, (char *)&fi, sizeof(fi));
			fi.fin_fi.fi_p = IPPROTO_GRE;
			fi.fin_fr = &pptpfr;
			fi.fin_data[0] = 0;
			fi.fin_data[1] = 0;
			ip->ip_p = IPPROTO_GRE;
			fi.fin_flx &= ~FI_TCPUDP;
			fi.fin_flx |= FI_IGNORE;
		}

		/*
		 * Update NAT timeout/create NAT if missing.
		 */
		if (pptp->pptp_nat != NULL)
			fr_queueback(&pptp->pptp_nat->nat_tqe);
		else {
			nat2 = nat_new(&fi, &pptp->pptp_rule, &pptp->pptp_nat,
				       NAT_SLAVE, nat->nat_dir);
			pptp->pptp_nat = nat2;
			if (nat2 != NULL) {
				(void) nat_proto(&fi, nat2, 0);
				nat_update(&fi, nat2, nat2->nat_ptr);
			}
		}

		/*
		 * Update state timeout/create state if missing.
		 */
		READ_ENTER(&ipf_state);
		if (pptp->pptp_state != NULL) {
			fr_queueback(&pptp->pptp_state->is_sti);
			RWLOCK_EXIT(&ipf_state);
		} else {
			RWLOCK_EXIT(&ipf_state);
			fi.fin_data[0] = 0;
			fi.fin_data[1] = 0;
			pptp->pptp_state = fr_addstate(&fi, &pptp->pptp_state,
						       0);
		}
		ip->ip_p = p;
	}
	return 0;
}


/*
 * clean up after ourselves.
 */
void ippr_pptp_del(aps)
ap_session_t *aps;
{
	pptp_pxy_t *pptp;

	pptp = aps->aps_data;

	if (pptp != NULL) {
		/*
		 * Don't delete it from here, just schedule it to be
		 * deleted ASAP.
		 */
		if (pptp->pptp_nat != NULL) {
			pptp->pptp_nat->nat_age = fr_ticks + 1;
			pptp->pptp_nat->nat_ptr = NULL;
			pptp->pptp_nat->nat_me = NULL;
			fr_queuefront(&pptp->pptp_nat->nat_tqe);
		}

		READ_ENTER(&ipf_state);
		if (pptp->pptp_state != NULL) {
			pptp->pptp_state->is_die = fr_ticks + 1;
			pptp->pptp_state->is_me = NULL;
			fr_queuefront(&pptp->pptp_state->is_sti);
		}
		RWLOCK_EXIT(&ipf_state);

		pptp->pptp_state = NULL;
		pptp->pptp_nat = NULL;
	}
}


int ippr_pptp_match(fin, aps, nat)
fr_info_t *fin;
ap_session_t *aps;
nat_t *nat;
{
	pptp_pxy_t *pptp;
	tcphdr_t *tcp;
	u_32_t cookie;

	pptp = aps->aps_data;
	tcp = fin->fin_dp;

	if ((pptp != NULL) && (fin->fin_dlen - (TCP_OFF(tcp) << 2) >= 8)) {
		u_char *cs;

		cs = (u_char *)tcp + (TCP_OFF(tcp) << 2) + 4;

		if (pptp->pptp_seencookie == 0) {
			pptp->pptp_seencookie = 1;
			pptp->pptp_cookie = (cs[0] << 24) | (cs[1] << 16) |
					    (cs[2] << 8) | cs[3];
		} else {
			cookie = (cs[0] << 24) | (cs[1] << 16) |
				 (cs[2] << 8) | cs[3];
			if (cookie != pptp->pptp_cookie)
				return -1;
		}

	}
	return 0;
}
