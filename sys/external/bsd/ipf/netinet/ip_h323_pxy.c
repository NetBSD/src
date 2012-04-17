/*	$NetBSD: ip_h323_pxy.c,v 1.1.4.2 2012/04/17 00:08:16 yamt Exp $	*/

/*
 * Copyright 2001, QNX Software Systems Ltd. All Rights Reserved
 *
 * This source code has been published by QNX Software Systems Ltd. (QSSL).
 * However, any use, reproduction, modification, distribution or transfer of
 * this software, or any software which includes or is based upon any of this
 * code, is only permitted under the terms of the QNX Open Community License
 * version 1.0 (see licensing.qnx.com for details) or as otherwise expressly
 * authorized by a written license agreement from QSSL. For more information,
 * please email licensing@qnx.com.
 *
 * For more details, see QNX_OCL.txt provided with this distribution.
 */

/*
 * Simple H.323 proxy
 *
 *      by xtang@canada.com
 *	ported to ipfilter 3.4.20 by Michael Grant mg-ipf@grant.org
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: ip_h323_pxy.c,v 1.1.4.2 2012/04/17 00:08:16 yamt Exp $");

#define IPF_H323_PROXY

void  ipf_p_h323_main_load(void);
void  ipf_p_h323_main_unload(void);
int  ipf_p_h323_new(void *, fr_info_t *, ap_session_t *, nat_t *);
void ipf_p_h323_del(ipf_main_softc_t *, ap_session_t *);
int  ipf_p_h323_in(void *, fr_info_t *, ap_session_t *, nat_t *);

int  ipf_p_h245_new(void *, fr_info_t *, ap_session_t *, nat_t *);
int  ipf_p_h245_out(void *, fr_info_t *, ap_session_t *, nat_t *);

static	frentry_t	h323_fr;

int	h323_proxy_init = 0;

static int find_port(int, void *, int datlen, int *, u_short *);


static int
find_port(int ipaddr, void *data, int datlen, int *off, unsigned short *port)
{
	u_32_t addr, netaddr;
	u_char *dp;
	int offset;

	if (datlen < 6)
		return -1;

	*port = 0;
	offset = *off;
	dp = (u_char *)data;
	netaddr = ntohl(ipaddr);

	for (offset = 0; offset <= datlen - 6; offset++, dp++) {
		addr = (dp[0] << 24) | (dp[1] << 16) | (dp[2] << 8) | dp[3];
		if (netaddr == addr)
		{
			*port = (*(dp + 4) << 8) | *(dp + 5);
			break;
		}
	}
	*off = offset;
  	return (offset > datlen - 6) ? -1 : 0;
}

/*
 * Initialize local structures.
 */
void
ipf_p_h323_main_load(void)
{
	bzero((char *)&h323_fr, sizeof(h323_fr));
	h323_fr.fr_ref = 1;
	h323_fr.fr_flags = FR_INQUE|FR_PASS|FR_QUICK|FR_KEEPSTATE;
	MUTEX_INIT(&h323_fr.fr_lock, "H323 proxy rule lock");
	h323_proxy_init = 1;
}


void
ipf_p_h323_main_unload(void)
{
	if (h323_proxy_init == 1) {
		MUTEX_DESTROY(&h323_fr.fr_lock);
		h323_proxy_init = 0;
	}
}


int
ipf_p_h323_new(void *arg, fr_info_t *fin, ap_session_t *aps, nat_t *nat)
{
	fin = fin;	/* LINT */
	nat = nat;	/* LINT */

	aps->aps_data = NULL;
	aps->aps_psiz = 0;

	return 0;
}


void
ipf_p_h323_del(ipf_main_softc_t *softc, ap_session_t *aps)
{
	int i;
	ipnat_t *ipn;

	if (aps->aps_data) {
		for (i = 0, ipn = aps->aps_data;
		     i < (aps->aps_psiz / sizeof(ipnat_t));
		     i++, ipn = (ipnat_t *)((char *)ipn + sizeof(*ipn)))
		{
			/*
			 * Check the comment in ipf_p_h323_in() function,
			 * just above ipf_nat_ioctl() call.
			 * We are lucky here because this function is not
			 * called with ipf_nat locked.
			 */
			if (ipf_nat_ioctl(softc, ipn, SIOCRMNAT, NAT_SYSSPACE|
				         NAT_LOCKHELD|FWRITE, 0, NULL) == -1) {
				/*EMPTY*/;
				/* log the error */
			}
		}
		KFREES(aps->aps_data, aps->aps_psiz);
		/* avoid double free */
		aps->aps_data = NULL;
		aps->aps_psiz = 0;
	}
	return;
}


int
ipf_p_h323_in(void *arg, fr_info_t *fin, ap_session_t *aps, nat_t *nat)
{
	ipf_main_softc_t *softc = fin->fin_main_soft;
	int ipaddr, off, datlen;
	unsigned short port;
	void *data;
	tcphdr_t *tcp;
	ip_t *ip;

	ip = fin->fin_ip;
	tcp = (tcphdr_t *)fin->fin_dp;
	ipaddr = ip->ip_src.s_addr;

	data = (char *)tcp + (TCP_OFF(tcp) << 2);
	datlen = fin->fin_dlen - (TCP_OFF(tcp) << 2);
	if (find_port(ipaddr, data, datlen, &off, &port) == 0) {
		ipnat_t *ipn;
		char *newarray;

		/* setup a nat rule to set a h245 proxy on tcp-port "port"
		 * it's like:
		 *   map <if> <inter_ip>/<mask> -> <gate_ip>/<mask> proxy port <port> <port>/tcp
		 */
		ipn = nat->nat_ptr;
		KMALLOCS(newarray, char *, aps->aps_psiz + ipn->in_namelen + 5);
		if (newarray == NULL) {
			return -1;
		}
		ipn = (ipnat_t *)&newarray[aps->aps_psiz];
		bcopy((void *)nat->nat_ptr, (void *)ipn, sizeof(ipnat_t));
		ipn->in_plabel = ipn->in_namelen;
		(void) strncpy(ipn->in_names + ipn->in_namelen, "h245", 4);
		ipn->in_namelen += 4;

		ipn->in_osrcip = nat->nat_osrcip;
		ipn->in_osrcmsk = 0xffffffff;
		ipn->in_odstip = nat->nat_odstip;
		ipn->in_odstmsk = 0xffffffff;
		ipn->in_odport = htons(port);
		MUTEX_INIT(&ipn->in_lock, "h323 proxy NAT rule");
		/*
		 * we got a problem here. we need to call ipf_nat_ioctl() to
		 * add the h245 proxy rule, but since we already hold (READ
		 * locked) the nat table rwlock (ipf_nat), if we go into
		 * ipf_nat_ioctl(), it will try to WRITE lock it. This will
		 * causing dead lock on RTP.
		 *
		 * The quick & dirty solution here is release the read lock,
		 * call ipf_nat_ioctl() and re-lock it.
		 * A (maybe better) solution is do a UPGRADE(), and instead
		 * of calling ipf_nat_ioctl(), we add the nat rule ourself.
		 */
		RWLOCK_EXIT(&softc->ipf_nat);
		if (ipf_nat_ioctl(softc, ipn, SIOCADNAT,
				 NAT_SYSSPACE|FWRITE, 0, NULL) == -1) {
			READ_ENTER(&softc->ipf_nat);
			return -1;
		}
		READ_ENTER(&softc->ipf_nat);
		if (aps->aps_data != NULL && aps->aps_psiz > 0) {
			bcopy(aps->aps_data, newarray, aps->aps_psiz);
			KFREES(aps->aps_data, aps->aps_psiz);
		}
		aps->aps_data = newarray;
		aps->aps_psiz += ipn->in_namelen + 5;
	}
	return 0;
}


int
ipf_p_h245_new(void *arg, fr_info_t *fin, ap_session_t *aps, nat_t *nat)
{
	fin = fin;	/* LINT */
	nat = nat;	/* LINT */

	aps->aps_data = NULL;
	aps->aps_psiz = 0;
	return 0;
}


int
ipf_p_h245_out(void *arg, fr_info_t *fin, ap_session_t *aps, nat_t *nat)
{
	ipf_main_softc_t *softc = fin->fin_main_soft;
#ifdef USE_MUTEXES
	ipf_nat_softc_t *softn = softc->ipf_nat_soft;
#endif
	int ipaddr, off, datlen;
	tcphdr_t *tcp;
	u_short port;
	void *data;
	ip_t *ip;

	aps = aps;	/* LINT */

	ip = fin->fin_ip;
	tcp = (tcphdr_t *)fin->fin_dp;
	ipaddr = nat->nat_osrcaddr;
	data = (char *)tcp + (TCP_OFF(tcp) << 2);
	datlen = fin->fin_plen - fin->fin_hlen - (TCP_OFF(tcp) << 2);
	if (find_port(ipaddr, data, datlen, &off, &port) == 0) {
		fr_info_t fi;
		nat_t     *nat2;

/*		port = htons(port); */
		nat2 = ipf_nat_outlookup(fin, IPN_UDP, IPPROTO_UDP,
					 ip->ip_src, ip->ip_dst);
		if (nat2 == NULL) {
			struct ip newip;
			udphdr_t udp;

			bcopy((void *)ip, (void *)&newip, sizeof(newip));
			newip.ip_len = htons(fin->fin_hlen + sizeof(udp));
			newip.ip_p = IPPROTO_UDP;
			newip.ip_src = nat->nat_osrcip;

			bzero((char *)&udp, sizeof(udp));
			udp.uh_sport = port;

			memcpy(&fi, fin, sizeof(fi));
			fi.fin_fi.fi_p = IPPROTO_UDP;
			fi.fin_data[0] = port;
			fi.fin_data[1] = 0;
			fi.fin_dp = (char *)&udp;

			MUTEX_ENTER(&softn->ipf_nat_new);
			nat2 = ipf_nat_add(&fi, nat->nat_ptr, NULL,
				       NAT_SLAVE|IPN_UDP|SI_W_DPORT,
				       NAT_OUTBOUND);
			MUTEX_EXIT(&softn->ipf_nat_new);
			if (nat2 != NULL) {
				(void) ipf_nat_proto(&fi, nat2, IPN_UDP);
				MUTEX_ENTER(&nat2->nat_lock);
				ipf_nat_update(&fi, nat2);
				MUTEX_EXIT(&nat2->nat_lock);

				nat2->nat_ptr->in_hits++;
#ifdef	IPFILTER_LOG
				ipf_nat_log(softc, softc->ipf_nat_soft, nat2, (u_int)(nat->nat_ptr->in_redir));
#endif
				memcpy((char *)data + off, &ip->ip_src.s_addr, 4);
				memcpy((char *)data + off + 4, &nat2->nat_nsport, 2);
			}
		}
	}
	return 0;
}
