/*	$NetBSD: ip_h323_pxy.c,v 1.11.12.1 2012/04/17 00:08:13 yamt Exp $	*/

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
__KERNEL_RCSID(1, "$NetBSD: ip_h323_pxy.c,v 1.11.12.1 2012/04/17 00:08:13 yamt Exp $");

#define IPF_H323_PROXY

int 
ippr_h323_init(void);
void 
ippr_h323_fini(void);
int 
ippr_h323_new(fr_info_t *, ap_session_t *, nat_t *);
void
ippr_h323_del(ap_session_t *);
int 
ippr_h323_out(fr_info_t *, ap_session_t *, nat_t *);
int 
ippr_h323_in(fr_info_t *, ap_session_t *, nat_t *);

int 
ippr_h245_new(fr_info_t *, ap_session_t *, nat_t *);
int 
ippr_h245_out(fr_info_t *, ap_session_t *, nat_t *);
int 
ippr_h245_in(fr_info_t *, ap_session_t *, nat_t *);

static	frentry_t	h323_fr;

int	h323_proxy_init = 0;

static int
find_port(int, void *, int datlen, int *, u_short *);


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
int
ippr_h323_init(void)
{
	bzero((char *)&h323_fr, sizeof(h323_fr));
	h323_fr.fr_ref = 1;
	h323_fr.fr_flags = FR_INQUE|FR_PASS|FR_QUICK|FR_KEEPSTATE;
	MUTEX_INIT(&h323_fr.fr_lock, "H323 proxy rule lock");
	h323_proxy_init = 1;

	return 0;
}


void
ippr_h323_fini(void)
{
	if (h323_proxy_init == 1) {
		MUTEX_DESTROY(&h323_fr.fr_lock);
		h323_proxy_init = 0;
	}
}


int
ippr_h323_new(fr_info_t *fin, ap_session_t *aps, nat_t *nat)
{
	fin = fin;	/* LINT */
	nat = nat;	/* LINT */

	aps->aps_data = NULL;
	aps->aps_psiz = 0;

	return 0;
}


void
ippr_h323_del(ap_session_t *aps)
{
	int i;
	ipnat_t *ipn;

	if (aps->aps_data) {
		for (i = 0, ipn = aps->aps_data;
		     i < (aps->aps_psiz / sizeof(ipnat_t));
		     i++, ipn = (ipnat_t *)((char *)ipn + sizeof(*ipn)))
		{
			/*
			 * Check the comment in ippr_h323_in() function,
			 * just above fr_nat_ioctl() call.
			 * We are lucky here because this function is not
			 * called with ipf_nat locked.
			 */
			if (fr_nat_ioctl((void *)ipn, SIOCRMNAT, NAT_SYSSPACE|
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
ippr_h323_in(fr_info_t *fin, ap_session_t *aps, nat_t *nat)
{
	int ipaddr, off, datlen;
	unsigned short port;
	tcphdr_t *tcp;
	void *data;
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
		KMALLOCS(newarray, char *, aps->aps_psiz + sizeof(*ipn));
		if (newarray == NULL) {
			return -1;
		}
		ipn = (ipnat_t *)&newarray[aps->aps_psiz];
		bcopy((void *)nat->nat_ptr, (void *)ipn, sizeof(ipnat_t));
		(void) strncpy(ipn->in_plabel, "h245", APR_LABELLEN);

		ipn->in_inip = nat->nat_inip.s_addr;
		ipn->in_inmsk = 0xffffffff;
		ipn->in_dport = htons(port);
		/*
		 * we got a problem here. we need to call fr_nat_ioctl() to add
		 * the h245 proxy rule, but since we already hold (READ locked)
		 * the nat table rwlock (ipf_nat), if we go into fr_nat_ioctl(),
		 * it will try to WRITE lock it. This will causing dead lock
		 * on RTP.
		 *
		 * The quick & dirty solution here is release the read lock,
		 * call fr_nat_ioctl() and re-lock it.
		 * A (maybe better) solution is do a UPGRADE(), and instead
		 * of calling fr_nat_ioctl(), we add the nat rule ourself.
		 */
		RWLOCK_EXIT(&ipf_nat);
		if (fr_nat_ioctl((void *)ipn, SIOCADNAT,
				 NAT_SYSSPACE|FWRITE, 0, NULL) == -1) {
			READ_ENTER(&ipf_nat);
			return -1;
		}
		READ_ENTER(&ipf_nat);
		if (aps->aps_data != NULL && aps->aps_psiz > 0) {
			bcopy(aps->aps_data, newarray, aps->aps_psiz);
			KFREES(aps->aps_data, aps->aps_psiz);
		}
		aps->aps_data = newarray;
		aps->aps_psiz += sizeof(*ipn);
	}
	return 0;
}


int
ippr_h245_new(fr_info_t *fin, ap_session_t *aps, nat_t *nat)
{
	fin = fin;	/* LINT */
	nat = nat;	/* LINT */

	aps->aps_data = NULL;
	aps->aps_psiz = 0;
	return 0;
}


int
ippr_h245_out(fr_info_t *fin, ap_session_t *aps, nat_t *nat)
{
	int ipaddr, off, datlen;
	tcphdr_t *tcp;
	u_short port;
	void *data;
	ip_t *ip;

	aps = aps;	/* LINT */

	ip = fin->fin_ip;
	tcp = (tcphdr_t *)fin->fin_dp;
	ipaddr = nat->nat_inip.s_addr;
	data = (char *)tcp + (TCP_OFF(tcp) << 2);
	datlen = fin->fin_dlen - (TCP_OFF(tcp) << 2);
	if (find_port(ipaddr, data, datlen, &off, &port) == 0) {
		fr_info_t fi;
		nat_t     *nat2;

/*		port = htons(port); */
		nat2 = nat_outlookup(fin, IPN_UDP, IPPROTO_UDP,
				    ip->ip_src, ip->ip_dst);
		if (nat2 == NULL) {
			struct ip newip;
			udphdr_t udp;

			bcopy((void *)ip, (void *)&newip, sizeof(newip));
			newip.ip_len = fin->fin_hlen + sizeof(udp);
			newip.ip_p = IPPROTO_UDP;
			newip.ip_src = nat->nat_inip;

			bzero((char *)&udp, sizeof(udp));
			udp.uh_sport = port;

			memcpy(&fi, fin, sizeof(fi));
			fi.fin_fi.fi_p = IPPROTO_UDP;
			fi.fin_data[0] = port;
			fi.fin_data[1] = 0;
			fi.fin_dp = (char *)&udp;

			MUTEX_ENTER(&ipf_nat_new);
			nat2 = nat_new(&fi, nat->nat_ptr, NULL,
				       NAT_SLAVE|IPN_UDP|SI_W_DPORT,
				       NAT_OUTBOUND);
			MUTEX_EXIT(&ipf_nat_new);
			if (nat2 != NULL) {
				(void) nat_proto(&fi, nat2, IPN_UDP);
				MUTEX_ENTER(&nat2->nat_lock);
				nat_update(&fi, nat2);
				MUTEX_EXIT(&nat2->nat_lock);

				nat2->nat_ptr->in_hits++;
#ifdef	IPFILTER_LOG
				nat_log(nat2, (u_int)(nat->nat_ptr->in_redir));
#endif
				memcpy((char *)data + off, &ip->ip_src.s_addr, 4);
				memcpy((char *)data + off + 4, &nat2->nat_outport, 2);
			}
		}
	}
	return 0;
}
