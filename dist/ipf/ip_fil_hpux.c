/*	$NetBSD: ip_fil_hpux.c,v 1.1.1.1 2004/03/28 08:55:32 martti Exp $	*/

/*
 * Copyright (C) 1993-2001 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 */
#if !defined(lint)
static const char sccsid[] = "%W% %G% (C) 1993-2000 Darren Reed";
static const char rcsid[] = "@(#)Id: ip_fil_hpux.c,v 2.45.2.2 2004/03/22 12:18:07 darrenr Exp";
#endif

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/cmn_err.h>
#include <sys/kernel.h>
#include <sys/callout.h>
#include <net/if.h>
#include <net/af.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/tcpip.h>
#include <netinet/ip_icmp.h>
#include "ip_compat.h"
#ifdef	USE_INET6
# include <netinet/icmp6.h>
#endif
#include "ip_fil.h"
#include "ip_state.h"
#include "ip_nat.h"
#include "ip_frag.h"
#include "ip_auth.h"

#include "md5.h"

/*
 * From Solaris <inet/ip.h>, except HP-UX uses int.
 */
typedef struct ipparam_s {
	int	ip_param_min;
	int	ip_param_max;
	int	ip_param_value;
	char	*ip_param_name;
} ipparam_t;
extern	ipparam_t	*ip_param_arr;

#undef	IPFDEBUG
extern	fr_flags, fr_active;
extern	struct	callout	*fr_timer_id;

static	int	fr_send_ip(fr_info_t *, mblk_t *);
ipfmutex_t	ipl_mutex, ipf_authmx, ipf_rw, ipf_stinsert;
ipfmutex_t	ipf_nat_new, ipf_natio, ipf_timeoutlock;
ipfrwlock_t	ipf_mutex, ipf_global, ipf_ipidfrag;
ipfrwlock_t	ipf_frag, ipf_state, ipf_nat, ipf_natfrag, ipf_auth;
int		*ip_ttl_ptr;
int		*ip_mtudisc;
int		*ip_forwarding;


int ipldetach()
{

	if (fr_control_forwarding & 2)
		ip_forwarding = 0;
#ifdef	IPFDEBUG
	cmn_err(CE_CONT, "ipldetach()\n");
#endif
	fr_deinitialise();

	(void) frflush(IPL_LOGIPF, FR_INQUE|FR_OUTQUE|FR_INACTIVE);
	(void) frflush(IPL_LOGIPF, FR_INQUE|FR_OUTQUE);

	RW_DESTROY(&ipf_ipidfrag);
	RW_DESTROY(&ipf_mutex);
	/* NOTE: This lock is acquired in ipf_detach */
	RWLOCK_EXIT(&ipf_global);
	RW_DESTROY(&ipf_global);

	MUTEX_DESTROY(&ipf_timeoutlock);
	MUTEX_DESTROY(&ipf_rw);

	return 0;
}


int iplattach __P((void))
{
	int i;

#ifdef	IPFDEBUG
	cmn_err(CE_CONT, "iplattach()\n");
#endif
	bzero((char *)frcache, sizeof(frcache));
	MUTEX_INIT(&ipf_rw, "ipf_rw");
	MUTEX_INIT(&ipf_timeoutlock, "ipf_timeoutlock");
	RWLOCK_INIT(&ipf_global, "ipf filter load/unload mutex");
	RWLOCK_INIT(&ipf_mutex, "ipf filter rwlock");
	RWLOCK_INIT(&ipf_ipidfrag, "ipf IP NAT-Frag rwlock");

	if (fr_initialise() < 0)
		return -1;

	/*
	 * XXX - There is no terminator for this array, so it is not possible
	 * to tell if what we are looking for is missing and go off the end
	 * of the array.
	 */
	for (i = 0; ; i++) {
		if (!strcmp(ip_param_arr[i].ip_param_name, "ip_def_ttl")) {
			ip_ttl_ptr = &ip_param_arr[i].ip_param_value;
		} else if (!strcmp(ip_param_arr[i].ip_param_name,
			    "ip_pmtu_strategy")) {
			ip_mtudisc = &ip_param_arr[i].ip_param_value;
		} else if (!strcmp(ip_param_arr[i].ip_param_name,
			    "ip_forwarding")) {
			ip_forwarding = &ip_param_arr[i].ip_param_value;
		}

		if (ip_mtudisc != NULL && ip_ttl_ptr != NULL &&
		    ip_forwarding != NULL)
			break;
	}

#ifdef	IPFDEBUG
	cmn_err(CE_CONT, "iplattach() - success!\n");
#endif
	if (fr_control_forwarding & 1)
		*ip_forwarding = 1;
	return 0;
}


/*
 * Filter ioctl interface.
 */
int iplioctl(dev, cmd, data, flags)
dev_t dev;
int cmd;
caddr_t data;
int flags;
{
	int error = 0, tmp;
	friostat_t fio;
	u_int enable;
	minor_t unit;

#ifdef	IPFDEBUG
	cmn_err(CE_CONT, "iplioctl(%x,%x,%x,%x)\n",
		dev, cmd, data, flags);
#endif

	unit = getminor(dev);
	if (IPL_LOGMAX < unit)
		return ENXIO;

	if (fr_running <= 0) {
		if (unit != IPL_LOGIPF)
			return EIO;
		if (cmd != SIOCIPFGETNEXT && cmd != SIOCIPFGET &&
		    cmd != SIOCIPFSET && cmd != SIOCFRENB &&
		    cmd != SIOCGETFS && cmd != SIOCGETFF)
			return EIO;
	}

	READ_ENTER(&ipf_global);

	error = fr_ioctlswitch(unit, data, cmd, flags);
	if (error != -1) {
		RWLOCK_EXIT(&ipf_global);
		return error;
	}
	error = 0;

	switch (cmd)
	{
	case SIOCFRENB :
		if (!(flags & FWRITE))
			error = EPERM;
		else {
			error = BCOPYIN(data, &enable, sizeof(enable));
			if (enable) {
				if (fr_running > 0)
					error = 0;
				else
					error = iplattach();
				if (error == 0)
					fr_running = 1;
				else
					(void) ipldetach();
			} else {
				error = ipldetach();
				if (error == 0)
					fr_running = -1;
			}
		}
		break;
	case SIOCIPFSET :
		if (!(flags & FWRITE)) {
			error = EPERM;
			break;
		}
	case SIOCIPFGETNEXT :
	case SIOCIPFGET :
		error = fr_ipftune(cmd, data);
		break;
	case SIOCSETFF :
		if (!(flags & FWRITE))
			error = EPERM;
		else {
			WRITE_ENTER(&ipf_mutex);
			error = BCOPYIN(data, &fr_flags, sizeof(fr_flags));
			RWLOCK_EXIT(&ipf_mutex);
		}
		break;
	case SIOCGETFF :
		error = BCOPYOUT(&fr_flags, data, sizeof(fr_flags));
		break;
	case SIOCFUNCL :
		error = fr_resolvefunc(data);
		break;
	case SIOCINAFR :
	case SIOCRMAFR :
	case SIOCADAFR :
	case SIOCZRLST :
		if (!(flags & FWRITE))
			error = EPERM;
		else
			error = frrequest(unit, cmd, (caddr_t)data,
					  fr_active, 1);
		break;
	case SIOCINIFR :
	case SIOCRMIFR :
	case SIOCADIFR :
		if (!(flags & FWRITE))
			error = EPERM;
		else
			error = frrequest(unit, cmd, (caddr_t)data,
					  1 - fr_active, 1);
		break;
	case SIOCSWAPA :
		if (!(flags & FWRITE))
			error = EPERM;
		else {
			WRITE_ENTER(&ipf_mutex);
			bzero((char *)frcache, sizeof(frcache[0]) * 2);
			error = BCOPYOUT(&fr_active, data, sizeof(fr_active));
			if (error != 0)
				error = EFAULT;
			else
				fr_active = 1 - fr_active;
			RWLOCK_EXIT(&ipf_mutex);
		}
		break;
	case SIOCGETFS :
		READ_ENTER(&ipf_mutex);
		fr_getstat(&fio);
		RWLOCK_EXIT(&ipf_mutex);
		error = fr_outobj(data, &fio, IPFOBJ_IPFSTAT);
		break;
	case SIOCFRZST :
		if (!(flags & FWRITE))
			error = EPERM;
		else
			error = fr_zerostats((caddr_t)data);
		break;
	case	SIOCIPFFL :
		if (!(flags & FWRITE))
			error = EPERM;
		else {
			error = BCOPYIN(data, &tmp, sizeof(tmp));
			if (!error) {
				tmp = frflush(unit, tmp);
				error = BCOPYOUT(&tmp, data, sizeof(tmp));
			}
		}
		break;
	case SIOCSTLCK :
		error = BCOPYIN(data, &tmp, sizeof(tmp));
		if (error == 0) {
			fr_state_lock = tmp;
			fr_nat_lock = tmp;
			fr_frag_lock = tmp;
			fr_auth_lock = tmp;
		} else
			error = EFAULT;
	break;
#ifdef	IPFILTER_LOG
	case	SIOCIPFFB :
		if (!(flags & FWRITE))
			error = EPERM;
		else {
			tmp = ipflog_clear(unit);
			error = BCOPYOUT(&tmp, data, sizeof(tmp));
		}
		break;
#endif /* IPFILTER_LOG */
	case SIOCFRSYN :
		if (!(flags & FWRITE))
			error = EPERM;
		else
			error = ipfsync();
		break;
	case SIOCGFRST :
		error = fr_outobj(data, fr_fragstats(), IPFOBJ_FRAGSTAT);
		break;
	case FIONREAD :
#ifdef	IPFILTER_LOG
		tmp = (int)iplused[IPL_LOGIPF];

		error = BCOPYOUT(&tmp, data, sizeof(tmp));
#endif
		break;
	default :
		error = EINVAL;
		break;
	}
	RWLOCK_EXIT(&ipf_global);
	return error;
}


void *get_unit(name, v)
char	*name;
int	v;
{
	size_t len = strlen(name) + 1;	/* includes \0 */
	qif_t *qf;
	int sap;

	if (v == 4)
		sap = 0x0800;
	else if (v == 6)
		return NULL;
	spinlock(&pfil_rw);
	qf = qif_iflookup(name, sap);
	spinunlock(&pfil_rw);
	return qf;
}


/*
 * routines below for saving IP headers to buffer
 */
int iplopen(dev, flag, dummy, mode)
dev_t dev;
int flag;
intptr_t dummy;
int mode;
{
	minor_t min = getminor(dev);

#ifdef  IPFDEBUG
	cmn_err(CE_CONT, "iplopen(%x,%x,%x,%x)\n", dev, flag, dummy, mode);
#endif
	min = (IPL_LOGMAX < min) ? ENXIO : 0;
	return min;
}


int iplclose(dev, flag, mode)
dev_t dev;
int flag;
int mode;
{
	minor_t min = getminor(dev);

#ifdef  IPFDEBUG
	cmn_err(CE_CONT, "iplclose(%x,%x,%x)\n", dev, flag, mode);
#endif
	min = (IPL_LOGMAX < min) ? ENXIO : 0;
	return min;
}


#ifdef IPFILTER_LOG
/*
 * iplread/ipllog
 * both of these must operate with at least splnet() lest they be
 * called during packet processing and cause an inconsistancy to appear in
 * the filter lists.
 */
int iplread(dev, uio)
dev_t dev;
register struct uio *uio;
{
#ifdef	IPFDEBUG
	cmn_err(CE_CONT, "iplread(%x,%x)\n", dev, uio);
#endif
	return ipflog_read(getminor(dev), uio);
}
#endif /* IPFILTER_LOG */


#if 0
/*
 * iplread/ipllog
 * both of these must operate with at least splnet() lest they be
 * called during packet processing and cause an inconsistancy to appear in
 * the filter lists.
 */
int iplwrite(dev, uio, cp)
dev_t dev;
register struct uio *uio;
cred_t *cp;
{
#ifdef	IPFDEBUG
	cmn_err(CE_CONT, "iplwrite(%x,%x,%x)\n", dev, uio, cp);
#endif
	if (getminor(dev) != IPL_LOGSYNC)
		return ENXIO;
	return ipfsync_write(uio);
}
#endif /* IPFILTER_SYNC */


/*
 * fr_send_reset - this could conceivably be a call to tcp_respond(), but that
 * requires a large amount of setting up and isn't any more efficient.
 */
int fr_send_reset(fin)
fr_info_t *fin;
{
	tcphdr_t *tcp, *tcp2;
	int tlen, hlen;
	mblk_t *m;
#ifdef	USE_INET6
	ip6_t *ip6;
#endif
	ip_t *ip;

	tcp = fin->fin_dp;
	if (tcp->th_flags & TH_RST)
		return -1;

#ifndef	IPFILTER_CKSUM
	if (fr_checkl4sum(fin) == -1)
		return -1;
#endif

	tlen = (tcp->th_flags & (TH_SYN|TH_FIN)) ? 1 : 0;
#ifdef	USE_INET6
	if (fin->fin_v == 6)
		hlen = sizeof(ip6_t);
	else
#endif
		hlen = sizeof(ip_t);
	hlen += sizeof(*tcp2);
	if ((m = (mblk_t *)allocb(hlen + 16, BPRI_HI)) == NULL)
		return -1;

	m->b_rptr += 16;
	MTYPE(m) = M_DATA;
	m->b_wptr = m->b_rptr + hlen;
	bzero((char *)m->b_rptr, hlen);
	tcp2 = (struct tcphdr *)(m->b_rptr + hlen - sizeof(*tcp2));
	tcp2->th_dport = tcp->th_sport;
	tcp2->th_sport = tcp->th_dport;
	if (tcp->th_flags & TH_ACK) {
		tcp2->th_seq = tcp->th_ack;
		tcp2->th_flags = TH_RST;
	} else {
		tcp2->th_ack = ntohl(tcp->th_seq);
		tcp2->th_ack += tlen;
		tcp2->th_ack = htonl(tcp2->th_ack);
		tcp2->th_flags = TH_RST|TH_ACK;
	}
	tcp2->th_off = sizeof(struct tcphdr) >> 2;

	/*
	 * This is to get around a bug in the Solaris 2.4/2.5 TCP checksum
	 * computation that is done by their put routine.
	 */
#ifdef	USE_INET6
	if (fin->fin_v == 6) {
		ip6 = (ip6_t *)m->b_rptr;
		ip6->ip6_src = fin->fin_dst6;
		ip6->ip6_dst = fin->fin_src6;
		ip6->ip6_plen = htons(sizeof(*tcp));
		ip6->ip6_nxt = IPPROTO_TCP;
	} else
#endif
	{
		ip = (ip_t *)m->b_rptr;
		ip->ip_src.s_addr = fin->fin_daddr;
		ip->ip_dst.s_addr = fin->fin_saddr;
		ip->ip_id = fr_nextipid(fin);
		ip->ip_hl = sizeof(*ip) >> 2;
		ip->ip_p = IPPROTO_TCP;
		ip->ip_len = htons(sizeof(*ip) + sizeof(*tcp));
		ip->ip_tos = fin->fin_ip->ip_tos;
		tcp2->th_sum = fr_cksum(m, ip, IPPROTO_TCP, tcp2);
	}
	return fr_send_ip(fin, m);
}


static int fr_send_ip(fr_info_t *fin, mblk_t *m)
{
	int i;

	RWLOCK_EXIT(&ipf_global);
#ifdef	USE_INET6
	if (fin->fin_v == 6) {
		ip6_t *ip6;

		ip6 = (ip6_t *)m->b_rptr;
		ip6->ip6_flow = 0;
		ip6->ip6_vfc = 0x60;
		ip6->ip6_hlim = 127;
	} else
#endif
	{
		ip_t *ip;

		ip = (ip_t *)m->b_rptr;
		ip->ip_v = IPVERSION;
		ip->ip_ttl = *ip_ttl_ptr;
		ip->ip_sum = ipf_cksum((u_short *)ip, sizeof(*ip));
		ip->ip_off = htons(*ip_mtudisc == 1 ? IP_DF : 0);
	}
	i = pfil_sendbuf(m);
	READ_ENTER(&ipf_global);
	return i;
}


int fr_send_icmp_err(type, fin, dst)
int type;
fr_info_t *fin;
int dst;
{
	struct in_addr dst4;
	struct icmp *icmp;
	mblk_t *m, *mb;
	int hlen, code;
	qpktinfo_t *qpi;
	u_short sz;
#ifdef	USE_INET6
	ip6_t *ip6;
#endif
	ip_t *ip;

	if ((type < 0) || (type > ICMP_MAXTYPE))
		return -1;

	code = fin->fin_icode;
#ifdef USE_INET6
	if ((code < 0) || (code > sizeof(icmptoicmp6unreach)/sizeof(int)))
		return -1;
#endif

#ifndef	IPFILTER_CKSUM
	if (fr_checkl4sum(fin) == -1)
		return -1;
#endif

	qpi = fin->fin_qpi;
	mb = fin->fin_qfm;

#ifdef	USE_INET6
	if (fin->fin_v == 6) {
		sz = sizeof(ip6_t);
		sz += MIN(mb->b_wptr - mb->b_rptr, 512);
		hlen = sizeof(ip6_t);
		type = icmptoicmp6types[type];
		if (type == ICMP6_DST_UNREACH)
			code = icmptoicmp6unreach[code];
	} else
#endif
	{
		if ((fin->fin_p == IPPROTO_ICMP) &&
		    !(fin->fin_flx & FI_SHORT))
			switch (ntohs(fin->fin_data[0]) >> 8)
			{
			case ICMP_ECHO :
			case ICMP_TSTAMP :
			case ICMP_IREQ :
			case ICMP_MASKREQ :
				break;
			default :
				return 0;
			}

		sz = sizeof(ip_t) * 2;
		sz += 8;		/* 64 bits of data */
		hlen = sizeof(ip_t);
	}

	sz += offsetof(struct icmp, icmp_ip);
	if ((m = (mblk_t *)allocb((size_t)sz + 16, BPRI_HI)) == NULL)
		return -1;
	MTYPE(m) = M_DATA;
	m->b_rptr += 16;
	m->b_wptr = m->b_rptr + sz;
	bzero((char *)m->b_rptr, (size_t)sz);
	icmp = (struct icmp *)(m->b_rptr + hlen);
	icmp->icmp_type = type;
	icmp->icmp_code = code;

#ifdef	USE_INET6
	if (fin->fin_v == 6) {
		struct in6_addr dst6;
		int csz;

		if (dst == 0) {
			if (fr_ifpaddr(6, FRI_NORMAL, qpi->qpi_real,
				       (struct in_addr *)&dst6, NULL) == -1) {
				FREE_MB_T(m);
				return -1;
			}
		} else
			dst6 = fin->fin_dst6;

		csz = sz;
		sz -= sizeof(ip6_t);
		ip6 = (ip6_t *)m->b_rptr;
		ip6->ip6_plen = htons((u_short)sz);
		ip6->ip6_nxt = IPPROTO_ICMPV6;
		ip6->ip6_src = dst6;
		ip6->ip6_dst = fin->fin_src6;
		sz -= offsetof(struct icmp, icmp_ip);
		bcopy((char *)mb->b_rptr, (char *)&icmp->icmp_ip, sz);
		icmp->icmp_cksum = csz - sizeof(ip6_t);
	} else
#endif
	{
		ip = (ip_t *)m->b_rptr;
		ip->ip_hl = sizeof(*ip) >> 2;
		ip->ip_p = IPPROTO_ICMP;
		ip->ip_id = fin->fin_ip->ip_id;
		ip->ip_tos = fin->fin_ip->ip_tos;
		ip->ip_len = htons((u_short)sz);
		if (dst == 0) {
			if (fr_ifpaddr(4, FRI_NORMAL, qpi->qpi_real,
				       &dst4, NULL) == -1) {
				FREE_MB_T(m);
				return -1;
			}
		} else
			dst4 = fin->fin_dst;
		ip->ip_src = dst4;
		ip->ip_dst = fin->fin_src;
		bcopy((char *)fin->fin_ip, (char *)&icmp->icmp_ip,
		      sizeof(*fin->fin_ip));
		bcopy((char *)fin->fin_ip + fin->fin_hlen,
		      (char *)&icmp->icmp_ip + sizeof(*fin->fin_ip), 8);
		icmp->icmp_cksum = ipf_cksum((u_short *)icmp,
					     sz - sizeof(ip_t));
	}

	/*
	 * Need to exit out of these so we don't recursively call rw_enter
	 * from fr_qout.
	 */
	return fr_send_ip(fin, m);
}


/*
 * return the first IP Address associated with an interface
 */
int fr_ifpaddr(v, atype, qifptr, inp, inpmask)
int v, atype;
void *qifptr;
struct in_addr *inp, *inpmask;
{
#ifdef	USE_INET6
	struct sockaddr_in6 sin6, mask6;
#endif
	struct sockaddr_in sin, mask;
	qif_t *qif = qifptr;

#ifdef	USE_INET6
	if (v == 6) {
		return ENOTSUP;
	}
#endif

	switch (atype)
	{
	case FRI_BROADCAST :
		sin.sin_addr.s_addr = QF_V4_BROADCAST(qif);
		break;
	case FRI_PEERADDR :
		sin.sin_addr.s_addr = QF_V4_PEERADDR(qif);
		break;
	default :
		sin.sin_addr.s_addr = QF_V4_ADDR(qif);
		break;
	}
	mask.sin_addr.s_addr = QF_V4_NETMASK(qif);

	return fr_ifpfillv4addr(atype, &sin, &mask, inp, inpmask);
}


#ifdef	IPL_SELECT
extern	iplog_select_t	iplog_ss[IPL_LOGMAX+1];
extern	int		selwait;

/*
 * iplog_input_ready and ipflog_select are both submissions from HP.
 */
void iplog_input_ready(unit)
minor_t unit;
{
	if (iplog_ss[unit].read_waiter) {
		selwakeup(iplog_ss[unit].read_waiter,
			  iplog_ss[unit].state & READ_COLLISION);
		iplog_ss[unit].read_waiter = 0;
		iplog_ss[unit].state &= READ_COLLISION;
	}
}


int iplselect(unit, flag)
minor_t unit;
int flag;
{
	kthread_t * t;

	MUTEX_ENTER(&ipl_mutex);
	switch (flag)
	{
	case FREAD:
		if (iplused[unit]) {
			MUTEX_EXIT(&ipl_mutex);
			return 1;
		}
		if ((t = iplog_ss[unit].read_waiter) &&
# if HPUXREV >= 1111
		    waiting_in_select(t)
# else
		    (kt_wchan(t) == (caddr_t)&selwait)
# endif
		    ) {
			iplog_ss[unit].state |= READ_COLLISION;
		} else {
			iplog_ss[unit].read_waiter = u.u_kthreadp;
		}
		break;
	}
	MUTEX_EXIT(&ipl_mutex);
	return 0;
}
#endif


u_32_t fr_newisn(fin)
fr_info_t *fin;
{
	static iss_seq_off = 0;
	u_char hash[16];
	u_32_t newiss;
	MD5_CTX ctx;

	/*
	 * Compute the base value of the ISS.  It is a hash
	 * of (saddr, sport, daddr, dport, secret).
	 */
	MD5Init(&ctx);

	MD5Update(&ctx, (u_char *) &fin->fin_fi.fi_src,
		  sizeof(fin->fin_fi.fi_src));
	MD5Update(&ctx, (u_char *) &fin->fin_fi.fi_dst,
		  sizeof(fin->fin_fi.fi_dst));
	MD5Update(&ctx, (u_char *) &fin->fin_dat, sizeof(fin->fin_dat));

	MD5Update(&ctx, ipf_iss_secret, sizeof(ipf_iss_secret));

	MD5Final(hash, &ctx);

	memcpy(&newiss, hash, sizeof(newiss));

	/*
	 * Now increment our "timer", and add it in to
	 * the computed value.
	 *
	 * XXX Use `addin'?
	 * XXX TCP_ISSINCR too large to use?
	 */
	iss_seq_off += 0x00010000;
	newiss += iss_seq_off;
	return newiss;
}


/* ------------------------------------------------------------------------ */
/* Function:    fr_nextipid                                                 */
/* Returns:     int - 0 == success, -1 == error (packet should be droppped) */
/* Parameters:  fin(I) - pointer to packet information                      */
/*                                                                          */
/* Returns the next IPv4 ID to use for this packet.                         */
/* ------------------------------------------------------------------------ */
INLINE u_short fr_nextipid(fin)
fr_info_t *fin;
{
	static u_short ipid = 0;
	u_short id;

	MUTEX_ENTER(&ipf_rw);
	id = ipid++;
	MUTEX_EXIT(&ipf_rw);

	return id;
}


INLINE void fr_checkv4sum(fin)
fr_info_t *fin;
{
#ifdef IPFILTER_CKSUM
	if (fr_checkl4sum(fin) == -1)
		fin->fin_flx |= FI_BAD;
#endif
}


#ifdef USE_INET6
INLINE void fr_checkv6sum(fin)
fr_info_t *fin;
{
# ifdef IPFILTER_CKSUM
	if (fr_checkl6sum(fin) == -1)
		fin->fin_flx |= FI_BAD;
# endif
}
#endif /* USE_INET6 */
