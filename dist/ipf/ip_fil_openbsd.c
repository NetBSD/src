/*	$NetBSD: ip_fil_openbsd.c,v 1.1.1.5 2007/04/14 20:17:22 martin Exp $	*/

/*
 * Copyright (C) 1993-2003 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 */
#if !defined(lint)
static const char sccsid[] = "@(#)ip_fil.c	2.41 6/5/96 (C) 1993-2000 Darren Reed";
static const char rcsid[] = "@(#)Id: ip_fil_openbsd.c,v 2.50.2.31 2007/02/08 19:59:52 darrenr Exp";
#endif

#if defined(KERNEL) || defined(_KERNEL)
# undef KERNEL
# undef _KERNEL
# define        KERNEL	1
# define        _KERNEL	1
#endif
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/dirent.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/timeout.h>

#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/udp.h>
#include <netinet/tcpip.h>
#include <netinet/ip_icmp.h>
#include "netinet/ip_compat.h"
#ifdef USE_INET6
# include <netinet/icmp6.h>
#endif
#include "netinet/ip_fil.h"
#include "netinet/ip_nat.h"
#include "netinet/ip_frag.h"
#include "netinet/ip_state.h"
#include "netinet/ip_proxy.h"
#include "netinet/ip_auth.h"
#ifdef	IPFILTER_SYNC
#include "netinet/ip_sync.h"
#endif
#ifdef	IPFILTER_SCAN
#include "netinet/ip_scan.h"
#endif
#include "netinet/ip_pool.h"
#include <sys/kernel.h>
extern	int	ip_optcopy __P((struct ip *, struct ip *));

#ifdef IPFILTER_M_IPFILTER
MALLOC_DEFINE(M_IPFILTER, "IP Filter", "IP Filter packet filter data structures");
#endif


extern	struct	protosw	inetsw[];
extern	int	ipforwarding;
#ifdef	USE_INET6
extern int ip6_getpmtu(struct route_in6 *, struct route_in6 *,
		       struct ifnet *, struct in6_addr *, u_long *);
#endif

static	int	(*fr_savep) __P((ip_t *, int, void *, int, struct mbuf **));
static	int	fr_send_ip __P((fr_info_t *, mb_t *, mb_t **));
#ifdef	USE_INET6
static int ipfr_fastroute6 __P((struct mbuf *, struct mbuf **,
				fr_info_t *, frdest_t *));
#endif

#include <sys/timeout.h>
struct timeout fr_slowtimer_ch;

#include <sys/conf.h>
/*
 * We provide the fr_checkp name just to minimize changes later.
 */
int (*fr_checkp) __P((ip_t *ip, int hlen, void *ifp, int out, mb_t **mp));


#if	defined(IPFILTER_LKM)
int iplidentify(s)
char *s;
{
	if (strcmp(s, "ipl") == 0)
		return 1;
	return 0;
}
#endif /* IPFILTER_LKM */


int ipfattach()
{
	int s;

	SPL_NET(s);
	if ((fr_running > 0) || (fr_checkp == fr_check)) {
		printf("IP Filter: already initialized\n");
		SPL_X(s);
		return EBUSY;
	}

	if (fr_initialise() < 0) {
		SPL_X(s);
		return EIO;
	}

	bzero((char *)frcache, sizeof(frcache));
	fr_savep = fr_checkp;
	fr_checkp = fr_check;

	if (fr_control_forwarding & 1)
		ipforwarding = 1;

	SPL_X(s);
	timeout_set(&fr_slowtimer_ch, fr_slowtimer, NULL);
	timeout_add(&fr_slowtimer_ch, (hz / IPF_HZ_DIVIDE) * IPF_HZ_MULT);
	return 0;
}


/*
 * Disable the filter by removing the hooks from the IP input/output
 * stream.
 */
int ipfdetach()
{
	int s;

	SPL_NET(s);

	timeout_del(&fr_slowtimer_ch);

	fr_deinitialise();

	fr_checkp = fr_savep;
	(void) frflush(IPL_LOGIPF, 0, FR_INQUE|FR_OUTQUE|FR_INACTIVE);
	(void) frflush(IPL_LOGIPF, 0, FR_INQUE|FR_OUTQUE);

	if (fr_control_forwarding & 2)
		ipforwarding = 0;

	SPL_X(s);
	return 0;
}


/*
 * Filter ioctl interface.
 */
int iplioctl(dev, cmd, data, mode , p)
struct proc *p;
dev_t dev;
u_long cmd;
caddr_t data;
int mode;
{
	int error = 0, unit = 0;
	SPL_INT(s);

	if ((securelevel >= 2) && (mode & FWRITE))
		return EPERM;

	unit = GET_MINOR(dev);
	if ((IPL_LOGMAX < unit) || (unit < 0))
		return ENXIO;

	if (fr_running <= 0) {
		if (unit != IPL_LOGIPF)
			return EIO;
		if (cmd != SIOCIPFGETNEXT && cmd != SIOCIPFGET &&
		    cmd != SIOCIPFSET && cmd != SIOCFRENB &&
		    cmd != SIOCGETFS && cmd != SIOCGETFF)
			return EIO;
	}

	SPL_NET(s);

	error = fr_ioctlswitch(unit, data, cmd, mode, p->p_cred->p_ruid, p);
	if (error != -1) {
		SPL_X(s);
		return error;
	}

	SPL_X(s);
	return error;
}


#if 0
void fr_forgetifp(ifp)
void *ifp;
{
	register frentry_t *f;

	WRITE_ENTER(&ipf_mutex);
	for (f = ipacct[0][fr_active]; (f != NULL); f = f->fr_next)
		if (f->fr_ifa == ifp)
			f->fr_ifa = (void *)-1;
	for (f = ipacct[1][fr_active]; (f != NULL); f = f->fr_next)
		if (f->fr_ifa == ifp)
			f->fr_ifa = (void *)-1;
	for (f = ipfilter[0][fr_active]; (f != NULL); f = f->fr_next)
		if (f->fr_ifa == ifp)
			f->fr_ifa = (void *)-1;
	for (f = ipfilter[1][fr_active]; (f != NULL); f = f->fr_next)
		if (f->fr_ifa == ifp)
			f->fr_ifa = (void *)-1;
#ifdef USE_INET6
	for (f = ipacct6[0][fr_active]; (f != NULL); f = f->fr_next)
		if (f->fr_ifa == ifp)
			f->fr_ifa = (void *)-1;
	for (f = ipacct6[1][fr_active]; (f != NULL); f = f->fr_next)
		if (f->fr_ifa == ifp)
			f->fr_ifa = (void *)-1;
	for (f = ipfilter6[0][fr_active]; (f != NULL); f = f->fr_next)
		if (f->fr_ifa == ifp)
			f->fr_ifa = (void *)-1;
	for (f = ipfilter6[1][fr_active]; (f != NULL); f = f->fr_next)
		if (f->fr_ifa == ifp)
			f->fr_ifa = (void *)-1;
#endif
	RWLOCK_EXIT(&ipf_mutex);
	fr_natsync(ifp);
}
#endif


/*
 * routines below for saving IP headers to buffer
 */
int iplopen(dev, flags, devtype, p)
dev_t dev;
int flags;
int devtype;
struct proc *p;
{
	u_int min = GET_MINOR(dev);

	if (IPL_LOGMAX < min)
		min = ENXIO;
	else
		min = 0;
	return min;
}


int iplclose(dev, flags, devtype, p)
dev_t dev;
int flags;
int devtype;
struct proc *p;
{
	u_int	min = GET_MINOR(dev);

	if (IPL_LOGMAX < min)
		min = ENXIO;
	else
		min = 0;
	return min;
}

/*
 * iplread/ipllog
 * both of these must operate with at least splnet() lest they be
 * called during packet processing and cause an inconsistancy to appear in
 * the filter lists.
 */
int iplread(dev, uio, ioflag)
dev_t dev;
register struct uio *uio;
int ioflag;
{

	if (fr_running < 1)
		return EIO;

# ifdef	IPFILTER_SYNC
	if (GET_MINOR(dev) == IPL_LOGSYNC)
		return ipfsync_read(uio);
# endif

#ifdef IPFILTER_LOG
	return ipflog_read(GET_MINOR(dev), uio);
#else
	return ENXIO;
#endif
}


/*
 * iplwrite
 * both of these must operate with at least splnet() lest they be
 * called during packet processing and cause an inconsistancy to appear in
 * the filter lists.
 */
#if (BSD >= 199306)
int iplwrite(dev, uio, ioflag)
int ioflag;
#else
int iplwrite(dev, uio)
#endif
dev_t dev;
register struct uio *uio;
{

	if (fr_running < 1)
		return EIO;

#ifdef	IPFILTER_SYNC
	if (GET_MINOR(dev) == IPL_LOGSYNC)
		return ipfsync_write(uio);
#endif
	return ENXIO;
}


/*
 * fr_send_reset - this could conceivably be a call to tcp_respond(), but that
 * requires a large amount of setting up and isn't any more efficient.
 */
int fr_send_reset(fin)
fr_info_t *fin;
{
	struct tcphdr *tcp, *tcp2;
	int tlen = 0, hlen;
	struct mbuf *m;
#ifdef USE_INET6
	ip6_t *ip6;
#endif
	ip_t *ip;

	tcp = fin->fin_dp;
	if (tcp->th_flags & TH_RST)
		return -1;		/* feedback loop */

#ifndef	IPFILTER_CKSUM
	if (fr_checkl4sum(fin) == -1)
		return -1;
#endif

	tlen = fin->fin_dlen - (TCP_OFF(tcp) << 2) +
			((tcp->th_flags & TH_SYN) ? 1 : 0) +
			((tcp->th_flags & TH_FIN) ? 1 : 0);

#ifdef USE_INET6
	hlen = (fin->fin_v == 6) ? sizeof(ip6_t) : sizeof(ip_t);
#else
	hlen = sizeof(ip_t);
#endif
#ifdef	MGETHDR
	MGETHDR(m, M_DONTWAIT, MT_HEADER);
#else
	MGET(m, M_DONTWAIT, MT_HEADER);
#endif
	if (m == NULL)
		return -1;
	if (sizeof(*tcp2) + hlen > MHLEN) {
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			FREE_MB_T(m);
			return -1;
		}
	}

	m->m_len = sizeof(*tcp2) + hlen;
	m->m_data += max_linkhdr;
	m->m_pkthdr.len = m->m_len;
	m->m_pkthdr.rcvif = (struct ifnet *)0;
	ip = mtod(m, struct ip *);
	bzero((char *)ip, hlen);
#ifdef USE_INET6
	ip6 = (ip6_t *)ip;
#endif
	tcp2 = (struct tcphdr *)((char *)ip + hlen);
	tcp2->th_sport = tcp->th_dport;
	tcp2->th_dport = tcp->th_sport;

	if (tcp->th_flags & TH_ACK) {
		tcp2->th_seq = tcp->th_ack;
		tcp2->th_flags = TH_RST;
		tcp2->th_ack = 0;
	} else {
		tcp2->th_seq = 0;
		tcp2->th_ack = ntohl(tcp->th_seq);
		tcp2->th_ack += tlen;
		tcp2->th_ack = htonl(tcp2->th_ack);
		tcp2->th_flags = TH_RST|TH_ACK;
	}
	TCP_X2_A(tcp2, 0);
	TCP_OFF_A(tcp2, sizeof(*tcp2) >> 2);
	tcp2->th_win = tcp->th_win;
	tcp2->th_sum = 0;
	tcp2->th_urp = 0;

#ifdef USE_INET6
	if (fin->fin_v == 6) {
		ip6->ip6_flow = ((ip6_t *)fin->fin_ip)->ip6_flow;
		ip6->ip6_plen = htons(sizeof(struct tcphdr));
		ip6->ip6_nxt = IPPROTO_TCP;
		ip6->ip6_hlim = 0;
		ip6->ip6_src = fin->fin_dst6;
		ip6->ip6_dst = fin->fin_src6;
		tcp2->th_sum = in6_cksum(m, IPPROTO_TCP,
					 sizeof(*ip6), sizeof(*tcp2));
		return fr_send_ip(fin, m, &m);
	}
#endif
	ip->ip_p = IPPROTO_TCP;
	ip->ip_len = htons(sizeof(struct tcphdr));
	ip->ip_src.s_addr = fin->fin_daddr;
	ip->ip_dst.s_addr = fin->fin_saddr;
	tcp2->th_sum = in_cksum(m, hlen + sizeof(*tcp2));
	ip->ip_len = hlen + sizeof(*tcp2);
	return fr_send_ip(fin, m, &m);
}


static int fr_send_ip(fin, m, mpp)
fr_info_t *fin;
mb_t *m, **mpp;
{
	fr_info_t fnew;
	ip_t *ip, *oip;
	int hlen;

	ip = mtod(m, ip_t *);
	bzero((char *)&fnew, sizeof(fnew));

	IP_V_A(ip, fin->fin_v);
	switch (fin->fin_v)
	{
	case 4 :
		fnew.fin_v = 4;
		oip = fin->fin_ip;
		IP_HL_A(ip, sizeof(*oip) >> 2);
		ip->ip_tos = oip->ip_tos;
		ip->ip_id = fr_nextipid(fin);
		ip->ip_off = ip_mtudisc ? IP_DF : 0;
		ip->ip_ttl = ip_defttl;
		ip->ip_sum = 0;
		hlen = sizeof(*oip);
		break;
#ifdef USE_INET6
	case 6 :
	{
		ip6_t *ip6 = (ip6_t *)ip;

		ip6->ip6_vfc = 0x60;
		ip6->ip6_hlim = IPDEFTTL;

		fnew.fin_v = 6;
		hlen = sizeof(*ip6);
		break;
	}
#endif
	default :
		return EINVAL;
	}
#ifdef IPSEC
	m->m_pkthdr.rcvif = NULL;
#endif

	fnew.fin_ifp = fin->fin_ifp;
	fnew.fin_flx = FI_NOCKSUM;
	fnew.fin_m = m;
	fnew.fin_ip = ip;
	fnew.fin_mp = mpp;
	fnew.fin_hlen = hlen;
	fnew.fin_dp = (char *)ip + hlen;
	(void) fr_makefrip(hlen, ip, &fnew);

	return fr_fastroute(m, mpp, &fnew, NULL);
}


int fr_send_icmp_err(type, fin, dst)
int type;
fr_info_t *fin;
int dst;
{
	int err, hlen, xtra, iclen, ohlen, avail, code;
	struct in_addr dst4;
	struct icmp *icmp;
	struct mbuf *m;
	void *ifp;
#ifdef USE_INET6
	ip6_t *ip6;
	struct in6_addr dst6;
#endif
	ip_t *ip, *ip2;

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
#ifdef MGETHDR
	MGETHDR(m, M_DONTWAIT, MT_HEADER);
#else
	MGET(m, M_DONTWAIT, MT_HEADER);
#endif
	if (m == NULL)
		return -1;
	avail = MHLEN;

	xtra = 0;
	hlen = 0;
	ohlen = 0;
	ifp = fin->fin_ifp;
	if (fin->fin_v == 4) {
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
				FREE_MB_T(m);
				return 0;
			}

		if (dst == 0) {
			if (fr_ifpaddr(4, FRI_NORMAL, ifp,
				       &dst4, NULL) == -1) {
				FREE_MB_T(m);
				return -1;
			}
		} else
			dst4.s_addr = fin->fin_daddr;

		hlen = sizeof(ip_t);
		ohlen = fin->fin_hlen;
		if (fin->fin_hlen < fin->fin_plen)
			xtra = MIN(fin->fin_dlen, 8);
		else
			xtra = 0;
	}

#ifdef USE_INET6
	else if (fin->fin_v == 6) {
		hlen = sizeof(ip6_t);
		ohlen = sizeof(ip6_t);
		type = icmptoicmp6types[type];
		if (type == ICMP6_DST_UNREACH)
			code = icmptoicmp6unreach[code];

		if (hlen + sizeof(*icmp) + max_linkhdr +
		    fin->fin_plen > avail) {
			MCLGET(m, M_DONTWAIT);
			if ((m->m_flags & M_EXT) == 0) {
				FREE_MB_T(m);
				return -1;
			}
			avail = MCLBYTES;
		}
		xtra = MIN(fin->fin_plen,
			   avail - hlen - sizeof(*icmp) - max_linkhdr);
		if (dst == 0) {
			if (fr_ifpaddr(6, FRI_NORMAL, ifp,
				       (struct in_addr *)&dst6, NULL) == -1) {
				FREE_MB_T(m);
				return -1;
			}
		} else
			dst6 = fin->fin_dst6;
	}
#endif
	else {
		FREE_MB_T(m);
		return -1;
	}

	iclen = hlen + sizeof(*icmp);
	avail -= (max_linkhdr + iclen);
	if (avail < 0) {
		FREE_MB_T(m);
		return -1;
	}
	if (xtra > avail)
		xtra = avail;
	iclen += xtra;
	m->m_data += max_linkhdr;
	m->m_pkthdr.rcvif = (struct ifnet *)0;
	m->m_pkthdr.len = iclen;
	m->m_len = iclen;
	ip = mtod(m, ip_t *);
	icmp = (struct icmp *)((char *)ip + hlen);
	ip2 = (ip_t *)&icmp->icmp_ip;
	bzero((char *)ip, iclen);

	icmp->icmp_type = type;
	icmp->icmp_code = fin->fin_icode;
	icmp->icmp_cksum = 0;
#ifdef icmp_nextmtu
	if (type == ICMP_UNREACH &&
	    fin->fin_icode == ICMP_UNREACH_NEEDFRAG && ifp)
		icmp->icmp_nextmtu = htons(((struct ifnet *)ifp)->if_mtu);
#endif

	bcopy((char *)fin->fin_ip, (char *)ip2, ohlen);

#ifdef USE_INET6
	ip6 = (ip6_t *)ip;
	if (fin->fin_v == 6) {
		ip6->ip6_flow = ((ip6_t *)fin->fin_ip)->ip6_flow;
		ip6->ip6_plen = htons(iclen - hlen);
		ip6->ip6_nxt = IPPROTO_ICMPV6;
		ip6->ip6_hlim = 0;
		ip6->ip6_src = dst6;
		ip6->ip6_dst = fin->fin_src6;
		if (xtra > 0)
			bcopy((char *)fin->fin_ip + ohlen,
			      (char *)&icmp->icmp_ip + ohlen, xtra);
		icmp->icmp_cksum = in6_cksum(m, IPPROTO_ICMPV6,
					     sizeof(*ip6), iclen - hlen);
	} else
#endif
	{
		ip2->ip_len = htons(ip2->ip_len);
		ip2->ip_off = htons(ip2->ip_off);
		ip->ip_p = IPPROTO_ICMP;
		ip->ip_src.s_addr = dst4.s_addr;
		ip->ip_dst.s_addr = fin->fin_saddr;

		if (xtra > 0)
			bcopy((char *)fin->fin_ip + ohlen,
			      (char *)&icmp->icmp_ip + ohlen, xtra);
		icmp->icmp_cksum = ipf_cksum((u_short *)icmp,
					     sizeof(*icmp) + 8);
		ip->ip_len = iclen;
		ip->ip_p = IPPROTO_ICMP;
	}
	err = fr_send_ip(fin, m, &m);
	return err;
}


#if !defined(IPFILTER_LKM)
void iplinit __P((void));

void
iplinit()
{
	if (ipfattach() != 0)
		printf("IP Filter failed to attach\n");
	else
		fr_running = 1;
	ip_init();
}
#endif /* ! IPFILTER_LKM */


int fr_fastroute(m0, mpp, fin, fdp)
mb_t *m0, **mpp;
fr_info_t *fin;
frdest_t *fdp;
{
	register struct ip *ip, *mhip;
	register struct mbuf *m = m0;
	register struct route *ro;
	int len, off, error = 0, hlen, code;
	struct ifnet *ifp, *sifp;
	struct sockaddr_in *dst;
	struct route iproute;
	u_short ip_off;
	frentry_t *fr;

#ifdef USE_INET6
	if (fin->fin_v == 6) {
		/*
		 * currently "to <if>" and "to <if>:ip#" are not supported
		 * for IPv6
		 */
		error = ipfr_fastroute6(m, mpp, fin, fdp);
		if ((error != 0) && (*mpp != NULL)) {
			FREE_MB_T(*mpp);
			*mpp = NULL;
		}
		return error;
	}
#endif

	hlen = fin->fin_hlen;
	ip = mtod(m0, struct ip *);

	/*
	 * Route packet.
	 */
	ro = &iproute;
	bzero((caddr_t)ro, sizeof (*ro));
	dst = (struct sockaddr_in *)&ro->ro_dst;
	dst->sin_family = AF_INET;
	dst->sin_addr = ip->ip_dst;

	fr = fin->fin_fr;
	if (fdp != NULL)
		ifp = fdp->fd_ifp;
	else
		ifp = fin->fin_ifp;

	if ((ifp == NULL) && (!fr || !(fr->fr_flags & FR_FASTROUTE))) {
		error = -2;
		goto bad;
	}

	if ((fdp != NULL) && (fdp->fd_ip.s_addr != 0))
		dst->sin_addr = fdp->fd_ip;

	dst->sin_len = sizeof(*dst);
	rtalloc(ro);

	if ((ifp == NULL) && (ro->ro_rt != NULL))
		ifp = ro->ro_rt->rt_ifp;

	if ((ro->ro_rt == NULL) || (ifp == NULL)) {
		if (in_localaddr(ip->ip_dst))
			error = EHOSTUNREACH;
		else
			error = ENETUNREACH;
		goto bad;
	}
	if (ro->ro_rt->rt_flags & RTF_GATEWAY)
		dst = (struct sockaddr_in *)ro->ro_rt->rt_gateway;
	if (ro->ro_rt)
		ro->ro_rt->rt_use++;

	/*
	 * For input packets which are being "fastrouted", they won't
	 * go back through output filtering and miss their chance to get
	 * NAT'd and counted.  Duplicated packets aren't considered to be
	 * part of the normal packet stream, so do not NAT them or pass
	 * them through stateful checking, etc.
	 */
	if ((fdp != &fr->fr_dif) && (fin->fin_out == 0)) {
		sifp = fin->fin_ifp;
		fin->fin_ifp = ifp;
		fin->fin_out = 1;
		(void) fr_acctpkt(fin, NULL);
		fin->fin_fr = NULL;
		if (!fr || !(fr->fr_flags & FR_RETMASK)) {
			u_32_t pass;

			(void) fr_checkstate(fin, &pass);
		}

		switch (fr_checknatout(fin, NULL))
		{
		case 0 :
			break;
		case 1 :
			fr_natderef((nat_t **)&fin->fin_nat);
			ip->ip_sum = 0;
			break;
		case -1 :
			error = -1;
			goto done;
			break;
		}

		fin->fin_ifp = sifp;
		fin->fin_out = 0;
	} else
		ip->ip_sum = 0;
	/*
	 * If small enough for interface, can just send directly.
	 */
	if (ip->ip_len <= ifp->if_mtu) {
		int i = 0;

#ifdef MCLISREFERENCED
		if ((m->m_flags & (M_EXT|M_CLUSTER)) == M_EXT &&
		    MCLISREFERENCED(m))
#else
		if (m->m_flags & M_EXT)
#endif
			i = 1;
		ip->ip_len = htons(ip->ip_len);
		ip->ip_off = htons(ip->ip_off);
		if (!ip->ip_sum)
			ip->ip_sum = in_cksum(m, hlen);
		error = (*ifp->if_output)(ifp, m, (struct sockaddr *)dst,
					  ro->ro_rt);
		if (i) {
			ip->ip_len = ntohs(ip->ip_len);
			ip->ip_off = ntohs(ip->ip_off);
		}
		goto done;
	}
	/*
	 * Too large for interface; fragment if possible.
	 * Must be able to put at least 8 bytes per fragment.
	 */
	ip_off = ip->ip_off;
	if (ip_off & IP_DF) {
		error = EMSGSIZE;
		goto bad;
	}
	len = (ifp->if_mtu - hlen) &~ 7;
	if (len < 8) {
		error = EMSGSIZE;
		goto bad;
	}

    {
	int mhlen, firstlen = len;
	struct mbuf **mnext = &m->m_act;

	/*
	 * Loop through length of segment after first fragment,
	 * make new header and copy data of each part and link onto chain.
	 */
	m0 = m;
	mhlen = sizeof (struct ip);
	for (off = hlen + len; off < ip->ip_len; off += len) {
#ifdef MGETHDR
		MGETHDR(m, M_DONTWAIT, MT_HEADER);
#else
		MGET(m, M_DONTWAIT, MT_HEADER);
#endif
		if (m == 0) {
			m = m0;
			error = ENOBUFS;
			goto bad;
		}
		m->m_data += max_linkhdr;
		mhip = mtod(m, struct ip *);
		bcopy((char *)ip, (char *)mhip, sizeof(*ip));
		if (hlen > sizeof (struct ip)) {
			mhlen = ip_optcopy(ip, mhip) + sizeof (struct ip);
			IP_HL_A(mhip, mhlen >> 2);
		}
		m->m_len = mhlen;
		mhip->ip_off = ((off - hlen) >> 3) + ip_off;
		if (off + len >= ip->ip_len)
			len = ip->ip_len - off;
		else
			mhip->ip_off |= IP_MF;
		mhip->ip_len = htons((u_short)(len + mhlen));
		m->m_next = m_copy(m0, off, len);
		if (m->m_next == 0) {
			error = ENOBUFS;	/* ??? */
			goto sendorfree;
		}
		m->m_pkthdr.len = mhlen + len;
		m->m_pkthdr.rcvif = NULL;
		mhip->ip_off = htons((u_short)mhip->ip_off);
		mhip->ip_sum = 0;
		mhip->ip_sum = in_cksum(m, mhlen);
		*mnext = m;
		mnext = &m->m_act;
	}
	/*
	 * Update first fragment by trimming what's been copied out
	 * and updating header, then send each fragment (in order).
	 */
	m_adj(m0, hlen + firstlen - ip->ip_len);
	ip->ip_len = htons((u_short)(hlen + firstlen));
	ip->ip_off = htons((u_short)IP_MF);
	ip->ip_sum = 0;
	ip->ip_sum = in_cksum(m0, hlen);
sendorfree:
	for (m = m0; m; m = m0) {
		m0 = m->m_act;
		m->m_act = 0;
		if (error == 0)
			error = (*ifp->if_output)(ifp, m,
			    (struct sockaddr *)dst, ro->ro_rt);
		else
			FREE_MB_T(m);
	}
    }	
done:
	if (!error)
		fr_frouteok[0]++;
	else
		fr_frouteok[1]++;

	if (ro->ro_rt) {
		RTFREE(ro->ro_rt);
	}
	*mpp = NULL;
	return 0;
bad:
	if (error == EMSGSIZE) {
		sifp = fin->fin_ifp;
		code = fin->fin_icode;
		fin->fin_icode = ICMP_UNREACH_NEEDFRAG;
		fin->fin_ifp = ifp;
		(void) fr_send_icmp_err(ICMP_UNREACH, fin, 1);
		fin->fin_ifp = sifp;
		fin->fin_icode = code;
	}
	FREE_MB_T(m);
	goto done;
}


#if defined(USE_INET6)
/*
 * This is the IPv6 specific fastroute code.  It doesn't clean up the mbuf's
 * or ensure that it is an IPv6 packet that is being forwarded, those are
 * expected to be done by the called (ipfr_fastroute).
 */
static int ipfr_fastroute6(m0, mpp, fin, fdp)
struct mbuf *m0, **mpp;
fr_info_t *fin;
frdest_t *fdp;
{
	struct route_in6 ip6route;
	struct sockaddr_in6 *dst6;
	struct route_in6 *ro;
	struct ifnet *ifp;
	frentry_t *fr;
#if defined(OpenBSD) && (OpenBSD >= 200211)
	struct route_in6 *ro_pmtu = NULL;
	struct in6_addr finaldst;
	ip6_t *ip6;
#endif
	u_long mtu;
	int error;

	ro = &ip6route;
	fr = fin->fin_fr;
	bzero((caddr_t)ro, sizeof(*ro));
	dst6 = (struct sockaddr_in6 *)&ro->ro_dst;
	dst6->sin6_family = AF_INET6;
	dst6->sin6_len = sizeof(struct sockaddr_in6);
	dst6->sin6_addr = fin->fin_fi.fi_dst.in6;

	if (fdp != NULL)
		ifp = fdp->fd_ifp;
	else
		ifp = fin->fin_ifp;

	if (fdp != NULL) {
		if (IP6_NOTZERO(&fdp->fd_ip6))
			dst6->sin6_addr = fdp->fd_ip6.in6;
	}

	rtalloc((struct route *)ro);

	if ((ifp == NULL) && (ro->ro_rt != NULL))
		ifp = ro->ro_rt->rt_ifp;

	if ((ro->ro_rt == NULL) || (ifp == NULL) ||
	    (ifp != ro->ro_rt->rt_ifp)) {
		error = EHOSTUNREACH;
	} else {
		if (ro->ro_rt->rt_flags & RTF_GATEWAY)
			dst6 = (struct sockaddr_in6 *)ro->ro_rt->rt_gateway;
		ro->ro_rt->rt_use++;

#if defined(OpenBSD) && (OpenBSD >= 200211)
		ip6 = mtod(m0, ip6_t *);
		ro_pmtu = ro;
		finaldst = ip6->ip6_dst;
		error = ip6_getpmtu(ro_pmtu, ro, ifp, &finaldst, &mtu);
		if (error == 0) {
#else
			mtu = nd_ifinfo[ifp->if_index].linkmtu;
#endif
			if (m0->m_pkthdr.len <= mtu) {
				*mpp = NULL;
				error = nd6_output(ifp, fin->fin_ifp, m0,
						   dst6, ro->ro_rt);
			} else {
				error = EMSGSIZE;
			}
#if defined(OpenBSD) && (OpenBSD >= 200211)
		}
#endif
	}

	if (ro->ro_rt != NULL) {
		RTFREE(ro->ro_rt);
	}
	return error;
}
#endif


int fr_verifysrc(fin)
fr_info_t *fin;
{
	struct sockaddr_in *dst;
	struct route iproute;

	bzero((char *)&iproute, sizeof(iproute));
	dst = (struct sockaddr_in *)&iproute.ro_dst;
	dst->sin_len = sizeof(*dst);
	dst->sin_family = AF_INET;
	dst->sin_addr = fin->fin_src;
	rtalloc(&iproute);
	if (iproute.ro_rt == NULL)
		return 0;
	return (fin->fin_ifp == iproute.ro_rt->rt_ifp);
}


/*
 * return the first IP Address associated with an interface
 */
int fr_ifpaddr(v, atype, ifptr, inp, inpmask)
int v, atype;
void *ifptr;
struct in_addr *inp, *inpmask;
{
#ifdef USE_INET6
	struct in6_addr *inp6 = NULL;
#endif
	struct sockaddr *sock, *mask;
	struct sockaddr_in *sin;
	struct ifaddr *ifa;
	struct ifnet *ifp;

	if ((ifptr == NULL) || (ifptr == (void *)-1))
		return -1;

	ifp = ifptr;

	if (v == 4)
		inp->s_addr = 0;
#ifdef USE_INET6
	else if (v == 6)
		bzero((char *)inp, sizeof(struct in6_addr));
#endif
	ifa = ifp->if_addrlist.tqh_first;
	sock = ifa->ifa_addr;
	while (sock != NULL && ifa != NULL) {
		sin = (struct sockaddr_in *)sock;
		if ((v == 4) && (sin->sin_family == AF_INET))
			break;
#ifdef USE_INET6
		if ((v == 6) && (sin->sin_family == AF_INET6)) {
			inp6 = &((struct sockaddr_in6 *)sin)->sin6_addr;
			if (!IN6_IS_ADDR_LINKLOCAL(inp6) &&
			    !IN6_IS_ADDR_LOOPBACK(inp6))
				break;
		}
#endif
		ifa = ifa->ifa_list.tqe_next;
		if (ifa != NULL)
			sock = ifa->ifa_addr;
	}
	if (ifa == NULL || sock == NULL)
		return -1;

	mask = ifa->ifa_netmask;
	if (atype == FRI_BROADCAST)
		sock = ifa->ifa_broadaddr;
	else if (atype == FRI_PEERADDR)
		sock = ifa->ifa_dstaddr;

#ifdef USE_INET6
	if (v == 6)
		return fr_ifpfillv6addr(atype, (struct sockaddr_in6 *)sock,
					(struct sockaddr_in6 *)mask,
					inp, inpmask);
#endif
	return fr_ifpfillv4addr(atype, (struct sockaddr_in *)sock,
				(struct sockaddr_in *)mask, inp, inpmask);
}


u_32_t fr_newisn(fin)
fr_info_t *fin;
{
	u_32_t newiss;
#if OpenBSD > 200012
	newiss = tcp_rndiss_next();
#else
	static iss_seq_off = 0;
	u_char hash[16];
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
#endif
	return newiss;
}


/* ------------------------------------------------------------------------ */
/* Function:    fr_nextipid                                                 */
/* Returns:     int - 0 == success, -1 == error (packet should be droppped) */
/* Parameters:  fin(I) - pointer to packet information                      */
/*                                                                          */
/* Returns the next IPv4 ID to use for this packet.                         */
/* ------------------------------------------------------------------------ */
u_short fr_nextipid(fin)
fr_info_t *fin;
{
	u_short id;

	id = htons(ip_randomid());

	return id;
}


INLINE void fr_checkv4sum(fin)
fr_info_t *fin;
{
#ifdef M_CSUM_TCP_UDP_BAD
	int manual, pflag, cflags, active;
	mb_t *m;

	if ((fin->fin_flx & FI_NOCKSUM) != 0)
		return;

	manual = 0;
	m = fin->fin_m;
	if (m == NULL) {
		manual = 1;
		goto skipauto;
	}

	switch (fin->fin_p)
	{
	case IPPROTO_UDP :
		active = M_TCP_CSUM_IN_OK | M_TCP_CSUM_IN_BAD;
		pflag = M_TCP_CSUM_IN_OK;
		break;
	case IPPROTO_TCP :
		active = M_UDP_CSUM_IN_OK | M_UDP_CSUM_IN_BAD;
		pflag = M_UDP_CSUM_IN_OK;
		break;
	default :
		pflag = 0;
		manual = 1;
		break;
	}

	cflags = m->m_pkthdr.csum & active;

	if (pflag != 0) {
		if (cflags == active)) {
			fin->fin_flx |= FI_BAD
		} else if (cflags == pflag) {
			;
		} else {
			manual = 1;
		}
	}
skipauto:
# ifdef IPFILTER_CKSUM
	if (manual != 0) {
		if (fr_checkl4sum(fin) == -1)
			fin->fin_flx |= FI_BAD;
		else
			m->m_pkthdr.csum |= pflag;
	}
# else
	;
# endif
#else
# ifdef IPFILTER_CKSUM
	if (fr_checkl4sum(fin) == -1)
		fin->fin_flx |= FI_BAD;
# endif
#endif
}


#ifdef USE_INET6
INLINE void fr_checkv6sum(fin)
fr_info_t *fin;
{
	fr_checkv4sum(fin);
}
#endif


size_t mbufchainlen(m0)
struct mbuf *m0;
{
	size_t len;

	if ((m0->m_flags & M_PKTHDR) != 0) {
		len = m0->m_pkthdr.len;
	} else {
		struct mbuf *m;

		for (m = m0, len = 0; m != NULL; m = m->m_next)
			len += m->m_len;
	}
	return len;
}


/* ------------------------------------------------------------------------ */
/* Function:    fr_pullup                                                   */
/* Returns:     NULL == pullup failed, else pointer to protocol header      */
/* Parameters:  m(I)   - pointer to buffer where data packet starts         */
/*              fin(I) - pointer to packet information                      */
/*              len(I) - number of bytes to pullup                          */
/*                                                                          */
/* Attempt to move at least len bytes (from the start of the buffer) into a */
/* single buffer for ease of access.  Operating system native functions are */
/* used to manage buffers - if necessary.  If the entire packet ends up in  */
/* a single buffer, set the FI_COALESCE flag even though fr_coalesce() has  */
/* not been called.  Both fin_ip and fin_dp are updated before exiting _IF_ */
/* and ONLY if the pullup succeeds.                                         */
/*                                                                          */
/* We assume that 'min' is a pointer to a buffer that is part of the chain  */
/* of buffers that starts at *fin->fin_mp.                                  */
/* ------------------------------------------------------------------------ */
void *fr_pullup(min, fin, len)
mb_t *min;
fr_info_t *fin;
int len;
{
	int out = fin->fin_out, dpoff, ipoff;
	mb_t *m = min;
	char *ip;

	if (m == NULL)
		return NULL;

	ip = (char *)fin->fin_ip;
	if ((fin->fin_flx & FI_COALESCE) != 0)
		return ip;

	ipoff = fin->fin_ipoff;
	if (fin->fin_dp != NULL)
		dpoff = (char *)fin->fin_dp - (char *)ip;
	else
		dpoff = 0;

	if (M_LEN(m) < len) {
#ifdef MHLEN
		/*
		 * Assume that M_PKTHDR is set and just work with what is left
		 * rather than check..
		 * Should not make any real difference, anyway.
		 */
		if (len > MHLEN)
#else
		if (len > MLEN)
#endif
		{
#ifdef HAVE_M_PULLDOWN
			if (m_pulldown(m, 0, len, NULL) == NULL)
				m = NULL;
#else
			FREE_MB_T(*fin->fin_mp);
			m = NULL;
#endif
		} else
		{
			m = m_pullup(m, len);
		}
		*fin->fin_mp = m;
		fin->fin_m = m;
		if (m == NULL) {
			ATOMIC_INCL(frstats[out].fr_pull[1]);
			return NULL;
		}
		ip = MTOD(m, char *) + ipoff;
	}

	ATOMIC_INCL(frstats[out].fr_pull[0]);
	fin->fin_ip = (ip_t *)ip;
	if (fin->fin_dp != NULL)
		fin->fin_dp = (char *)fin->fin_ip + dpoff;

	if (len == fin->fin_plen)
		fin->fin_flx |= FI_COALESCE;
	return ip;
}


int ipf_inject(fin, m)
fr_info_t *fin;
mb_t *m;
{
	int error;

	if (fin->fin_out == 0) {
		struct ifqueue *ifq;

		ifq = &ipintrq;

		if (IF_QFULL(ifq)) {
			IF_DROP(ifq);
			FREE_MB_T(m);
			error = ENOBUFS;
		} else {
			IF_ENQUEUE(ifq, m);
			error = 0;
		}
	} else {
		error = ip_output(m, NULL, NULL, IP_FORWARDING, NULL, NULL);
	}

	return error;
}
