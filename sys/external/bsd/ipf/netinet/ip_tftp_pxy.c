/*	$NetBSD: ip_tftp_pxy.c,v 1.2.4.2 2012/04/17 00:08:17 yamt Exp $	*/

/*
 * Copyright (C) 2010 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: ip_tftp_pxy.c,v 2.1.2.2 2012/01/26 05:29:13 darrenr Exp
 */

#define IPF_TFTP_PROXY

void ipf_p_tftp_main_load(void);
void ipf_p_tftp_main_unload(void);
int ipf_p_tftp_new(void *, fr_info_t *, ap_session_t *, nat_t *);
int ipf_p_tftp_out(void *, fr_info_t *, ap_session_t *, nat_t *);
int ipf_p_tftp_in(void *, fr_info_t *, ap_session_t *, nat_t *);
int ipf_p_tftp_client(fr_info_t *, ap_session_t *, nat_t *);
int ipf_p_tftp_server(fr_info_t *, ap_session_t *, nat_t *);
int ipf_p_tftp_backchannel(fr_info_t *, ap_session_t *, nat_t *);

static	frentry_t	tftpfr;

int	tftp_proxy_init = 0;

typedef struct tftpinfo {
	nat_t	*ti_datanat;
	ipstate_t	*ti_datastate;
	int	ti_lastcmd;
	int	ti_nextblk;
	int	ti_lastblk;
	int	ti_lasterror;
	char	ti_filename[80];
} tftpinfo_t;

#define	TFTP_CMD_READ	1
#define	TFTP_CMD_WRITE	2
#define	TFTP_CMD_DATA	3
#define	TFTP_CMD_ACK	4
#define	TFTP_CMD_ERROR	5


/*
 * TFTP application proxy initialization.
 */
void
ipf_p_tftp_main_load(void)
{

	bzero((char *)&tftpfr, sizeof(tftpfr));
	tftpfr.fr_ref = 1;
	tftpfr.fr_flags = FR_INQUE|FR_PASS|FR_QUICK|FR_KEEPSTATE;
	MUTEX_INIT(&tftpfr.fr_lock, "TFTP proxy rule lock");
	tftp_proxy_init = 1;
}


void
ipf_p_tftp_main_unload(void)
{

	if (tftp_proxy_init == 1) {
		MUTEX_DESTROY(&tftpfr.fr_lock);
		tftp_proxy_init = 0;
	}
}


int
ipf_p_tftp_out(void *arg, fr_info_t *fin, ap_session_t *aps, nat_t *nat)
{

	if (nat->nat_dir == NAT_OUTBOUND)
		return ipf_p_tftp_client(fin, aps, nat);
	return ipf_p_tftp_server(fin, aps, nat);
}


int
ipf_p_tftp_in(void *arg, fr_info_t *fin, ap_session_t *aps, nat_t *nat)
{

	if (nat->nat_dir == NAT_INBOUND)
		return ipf_p_tftp_client(fin, aps, nat);
	return ipf_p_tftp_server(fin, aps, nat);
}


int
ipf_p_tftp_new(void *arg, fr_info_t *fin, ap_session_t *aps, nat_t *nat)
{
	udphdr_t *udp;
	tftpinfo_t *ti;

	KMALLOC(ti, tftpinfo_t *);
	if (ti == NULL)
		return -1;

	aps->aps_data = ti;
	aps->aps_psiz = sizeof(*ti);
	ti->ti_lastcmd = 0;

	nat = nat;	/* LINT */
	fin = fin;	/* LINT */

	udp = (udphdr_t *)fin->fin_dp;
	aps->aps_sport = udp->uh_sport;
	aps->aps_dport = udp->uh_dport;
	return 0;
}


/*
 * Setup for a new TFTP proxy.
 */
int
ipf_p_tftp_backchannel(fr_info_t *fin, ap_session_t *aps, nat_t *nat)
{
	ipf_main_softc_t *softc = fin->fin_main_soft;
#ifdef USE_MUTEXES
	ipf_nat_softc_t *softn = softc->ipf_nat_soft;
#endif
	struct in_addr swip,swip2;
	tftpinfo_t *ti;
	udphdr_t *udp;
	fr_info_t fi;
	nat_t *nat2;

	ti = aps->aps_data;
	udp = (udphdr_t *)fin->fin_dp;
	/*
	 * Add skeleton NAT entry for connection which will come back the
	 * other way.
	 */
	bcopy((char *)fin, (char *)&fi, sizeof(fi));
	fi.fin_flx |= FI_IGNORE;
	fi.fin_data[1] = 0;
	if (nat->nat_dir == NAT_OUTBOUND)
		nat2 = ipf_nat_outlookup(&fi, NAT_SEARCH|IPN_UDP,
					 nat->nat_pr[0], nat->nat_osrcip,
					 nat->nat_odstip);
	else
		nat2 = ipf_nat_inlookup(&fi, NAT_SEARCH|IPN_UDP,
					nat->nat_pr[0], nat->nat_nsrcip,
					nat->nat_odstip);
	if (nat2 == NULL) {
		u_short slen;
		int nflags;
		ip_t *ip;

		ip = fin->fin_ip;
		slen = ip->ip_len;
		ip->ip_len = htons(fin->fin_hlen + sizeof(*udp));
		bzero((char *)udp, sizeof(*udp));
		udp->uh_sport = htons(fi.fin_data[0]);
		udp->uh_dport = 0; /* XXX - don't specify remote port */
		udp->uh_ulen = 0;
		udp->uh_sum = 0;
		fi.fin_dp = (char *)udp;
		fi.fin_fr = &tftpfr;
		fi.fin_dlen = sizeof(*udp);
		fi.fin_plen = fi.fin_hlen + sizeof(*udp);
		fi.fin_flx &= FI_LOWTTL|FI_FRAG|FI_TCPUDP|FI_OPTIONS|FI_IGNORE;
		nflags = NAT_SLAVE|IPN_UDP|SI_W_DPORT;

		swip = ip->ip_src;
		swip2 = ip->ip_dst;
		fi.fin_fi.fi_saddr = nat->nat_osrcaddr;
		ip->ip_src = nat->nat_osrcip;
		fi.fin_fi.fi_daddr = nat->nat_odstaddr;
		ip->ip_dst = nat->nat_odstip;

		if (nat->nat_dir == NAT_INBOUND)
			nflags |= NAT_NOTRULEPORT;

		MUTEX_ENTER(&softn->ipf_nat_new);
		nat2 = ipf_nat_add(&fi, nat->nat_ptr, &ti->ti_datanat,
				   nflags, nat->nat_dir);
		MUTEX_EXIT(&softn->ipf_nat_new);
		if (nat2 != NULL) {
			(void) ipf_nat_proto(&fi, nat2, IPN_UDP);
			ipf_nat_update(&fi, nat2);
			fi.fin_ifp = NULL;
			if (ipf_state_add(softc, &fi, &ti->ti_datastate,
					  SI_W_DPORT) != 0) {
				ipf_nat_setpending(softc, nat2);
			}
		}
		ip->ip_len = slen;
		ip->ip_src = swip;
		ip->ip_dst = swip2;
		return 0;
	}
	return -1;
}


int
ipf_p_tftp_client(fr_info_t *fin, ap_session_t *aps, nat_t *nat)
{
	u_char *msg, *s, *t;
	tftpinfo_t *ti;
	u_short opcode;
	udphdr_t *udp;
	int len;

	if (fin->fin_dlen < 4)
		return 0;

	ti = aps->aps_data;
	msg = fin->fin_dp;
	msg += sizeof(udphdr_t);
	opcode = (msg[0] << 8) | msg[1];

	switch (opcode)
	{
	case TFTP_CMD_READ :
	case TFTP_CMD_WRITE :
		if (fin->fin_out != 0)
			return -1;
		len = fin->fin_dlen - sizeof(*udp) - 2;
		if (len > sizeof(ti->ti_filename) - 1)
			len = sizeof(ti->ti_filename) - 1;
		s = msg + 2;
		for (t = (u_char *)ti->ti_filename; (len > 0); len--, s++) {
			*t++ = *s;
			if (*s == '\0')
				break;
		}
		break;
	default :
		return -1;
	}

	ti = aps->aps_data;
	ti->ti_lastcmd = opcode;
	return 0;
}


int
ipf_p_tftp_server(fr_info_t *fin, ap_session_t *aps, nat_t *nat)
{
	tftpinfo_t *ti;
	u_short opcode;
	u_short arg;
	u_char *msg;

	if (fin->fin_dlen < 4)
		return 0;

	ti = aps->aps_data;
	msg = fin->fin_dp;
	msg += sizeof(udphdr_t);
	arg = (msg[2] << 8) | msg[3];
	opcode = (msg[0] << 8) | msg[1];

	switch (opcode)
	{
	case TFTP_CMD_ACK :
		/* This proxy should not see any ACKS for DATA blocks */
		if (fin->fin_out != 1)
			return -1;
		if ((arg == 0) &&
		    (ti->ti_lastcmd == TFTP_CMD_READ ||
		     ti->ti_lastcmd == TFTP_CMD_WRITE))
			ipf_p_tftp_backchannel(fin, aps, nat);
		ti->ti_lastblk = arg;
		break;
	case TFTP_CMD_ERROR :
		if (fin->fin_out != 1)
			return -1;
		ti->ti_lasterror = arg;
		break;
	default :
		return -1;
	}

	ti->ti_lastcmd = opcode;
	return 0;
}
