/*	$NetBSD: ip_fil_netbsd.c,v 1.3 2004/03/28 09:01:26 martti Exp $	*/

/*
 * Copyright (C) 1993-2003 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 */
#if !defined(lint)
static const char sccsid[] = "@(#)ip_fil.c	2.41 6/5/96 (C) 1993-2000 Darren Reed";
static const char rcsid[] = "@(#)Id: ip_fil_netbsd.c,v 2.55.2.2 2004/03/22 12:18:08 darrenr Exp";
#endif

#if defined(KERNEL) || defined(_KERNEL)
# undef KERNEL
# undef _KERNEL
# define        KERNEL	1
# define        _KERNEL	1
#endif
#include <sys/param.h>
#if (NetBSD >= 199905) && !defined(IPFILTER_LKM) && defined(_KERNEL)
# include "opt_ipfilter_log.h"
# include "opt_pfil_hooks.h"
# include "opt_ipsec.h"
#endif
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/systm.h>
#if (NetBSD > 199609)
# include <sys/dirent.h>
#else
# include <sys/dir.h>
#endif
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/tcpip.h>
#include <netinet/ip_icmp.h>
#include "netinet/ip_compat.h"
#ifdef USE_INET6
# include <netinet/icmp6.h>
# if (__NetBSD_Version__ > 106009999)
#  include <netinet6/nd6.h>
# endif
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
#include <sys/md5.h>
#include <sys/kernel.h>
extern	int	ip_optcopy __P((struct ip *, struct ip *));

#ifdef IPFILTER_M_IPFILTER
MALLOC_DEFINE(M_IPFILTER, "IP Filter", "IP Filter packet filter data structures");
#endif

#if __NetBSD_Version__ >= 105009999
# define	csuminfo	csum_flags
#endif

extern	struct	protosw	inetsw[];

static	int	(*fr_savep) __P((ip_t *, int, void *, int, struct mbuf **));
static	int	fr_send_ip __P((fr_info_t *, mb_t *, mb_t **));
#ifdef KMUTEX_T
extern  ipfmutex_t	ipf_rw;
extern	ipfrwlock_t	ipf_mutex;
#endif
#ifdef USE_INET6
static int ipfr_fastroute6 __P((struct mbuf *, struct mbuf **,
				fr_info_t *, frdest_t *));
#endif

#if (__NetBSD_Version__ >= 104040000)
# include <sys/callout.h>
struct callout fr_slowtimer_ch;
#endif

#include <sys/conf.h>
#if defined(NETBSD_PF)
# include <net/pfil.h>
/*
 * We provide the fr_checkp name just to minimize changes later.
 */
int (*fr_checkp) __P((ip_t *ip, int hlen, void *ifp, int out, mb_t **mp));
#endif /* NETBSD_PF */

#if (__NetBSD_Version__ >= 106080000) && defined(_KERNEL)
const struct cdevsw ipl_cdevsw = {
	iplopen, iplclose, iplread, nowrite, iplioctl,
	nostop, notty, nopoll, nommap,
};
#endif



#if defined(__NetBSD_Version__) && (__NetBSD_Version__ >= 105110000)
# include <net/pfil.h>

static int fr_check_wrapper(void *, struct mbuf **, struct ifnet *, int );

static int fr_check_wrapper(arg, mp, ifp, dir)
void *arg;
struct mbuf **mp;
struct ifnet *ifp;
int dir;
{
	struct ip *ip = mtod(*mp, struct ip *);
	int rv, hlen = ip->ip_hl << 2;

#if defined(M_CSUM_TCPv4)
	/*
	 * If the packet is out-bound, we can't delay checksums
	 * here.  For in-bound, the checksum has already been
	 * validated.
	 */
	if (dir == PFIL_OUT) {
		if ((*mp)->m_pkthdr.csum_flags & (M_CSUM_TCPv4|M_CSUM_UDPv4)) {
			in_delayed_cksum(*mp);
			(*mp)->m_pkthdr.csum_flags &=
			    ~(M_CSUM_TCPv4|M_CSUM_UDPv4);
		}
	}
#endif /* M_CSUM_TCPv4 */

	/*
	 * We get the packet with all fields in network byte
	 * order.  We expect ip_len and ip_off to be in host
	 * order.  We frob them, call the filter, then frob
	 * them back.
	 *
	 * Note, we don't need to update the checksum, because
	 * it has already been verified.
	 */
	NTOHS(ip->ip_len);
	NTOHS(ip->ip_off);

	rv = fr_check(ip, hlen, ifp, (dir == PFIL_OUT), mp);

	if (rv == 0 && *mp != NULL) {
		ip = mtod(*mp, struct ip *);
		HTONS(ip->ip_len);
		HTONS(ip->ip_off);
	}

	return (rv);
}

# ifdef USE_INET6
#  include <netinet/ip6.h>

static int fr_check_wrapper6(void *, struct mbuf **, struct ifnet *, int );

static int fr_check_wrapper6(arg, mp, ifp, dir)
void *arg;
struct mbuf **mp;
struct ifnet *ifp;
int dir;
{
	
	return (fr_check(mtod(*mp, struct ip *), sizeof(struct ip6_hdr),
	    ifp, (dir == PFIL_OUT), mp));
}
# endif
#endif /* __NetBSD_Version >= 105110000 */


#if	defined(IPFILTER_LKM)
int iplidentify(s)
char *s;
{
	if (strcmp(s, "ipl") == 0)
		return 1;
	return 0;
}
#endif /* IPFILTER_LKM */


/*
 * Try to detect the case when compiling for NetBSD with pseudo-device
 */
#if defined(PFIL_HOOKS)
void
ipfilterattach(count)
int count;
{
# if 0
	if (iplattach() != 0)
		printf("IP Filter failed to attach\n");
# endif
}
#endif


int iplattach()
{
	int s;
#if defined(NETBSD_PF) && (__NetBSD_Version__ >= 104200000)
	int error = 0;
# if defined(__NetBSD_Version__) && (__NetBSD_Version__ >= 105110000)
        struct pfil_head *ph_inet;
#  ifdef USE_INET6
        struct pfil_head *ph_inet6;
#  endif
# endif
#endif

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

#ifdef NETBSD_PF
# if (__NetBSD_Version__ >= 104200000)
#  if __NetBSD_Version__ >= 105110000
	ph_inet = pfil_head_get(PFIL_TYPE_AF, AF_INET);
#   ifdef USE_INET6
	ph_inet6 = pfil_head_get(PFIL_TYPE_AF, AF_INET6);
#   endif
	if (ph_inet == NULL
#   ifdef USE_INET6
	    && ph_inet6 == NULL
#   endif
	   ) {
		printf("pfil_head_get failed\n");
		return ENODEV;
	}

	if (ph_inet != NULL)
		error = pfil_add_hook((void *)fr_check_wrapper, NULL,
				      PFIL_IN|PFIL_OUT, ph_inet);
	else
		error = 0;
# else
	error = pfil_add_hook((void *)fr_check, PFIL_IN|PFIL_OUT,
			      &inetsw[ip_protox[IPPROTO_IP]].pr_pfh);
# endif
	if (error) {
#  ifdef USE_INET6
		goto pfil_error;
#  else
		fr_deinitialise();
		SPL_X(s);
		return error;
#  endif
	}
# else
	pfil_add_hook((void *)fr_check, PFIL_IN|PFIL_OUT);
# endif
# ifdef USE_INET6
#  if __NetBSD_Version__ >= 105110000
	if (ph_inet6 != NULL)
		error = pfil_add_hook((void *)fr_check_wrapper6, NULL,
				      PFIL_IN|PFIL_OUT, ph_inet6);
	else
		error = 0;
	if (error) {
		pfil_remove_hook((void *)fr_check_wrapper6, NULL,
				 PFIL_IN|PFIL_OUT, ph_inet6);
#  else
	error = pfil_add_hook((void *)fr_check, PFIL_IN|PFIL_OUT,
			      &inetsw[ip_protox[IPPROTO_IPV6]].pr_pfh);
	if (error) {
		pfil_remove_hook((void *)fr_check, PFIL_IN|PFIL_OUT,
				 &inetsw[ip_protox[IPPROTO_IP]].pr_pfh);
#  endif
pfil_error:
		fr_deinitialise();
		SPL_X(s);
		return error;
	}
# endif
#endif

	bzero((char *)frcache, sizeof(frcache));
	fr_savep = fr_checkp;
	fr_checkp = fr_check;

	if (fr_control_forwarding & 1)
		ipforwarding = 1;

	SPL_X(s);

#if (__NetBSD_Version__ >= 104010000)
	callout_init(&fr_slowtimer_ch);
	callout_reset(&fr_slowtimer_ch, (hz / IPF_HZ_DIVIDE) * IPF_HZ_MULT,
		     fr_slowtimer, NULL);
#else
	timeout(fr_slowtimer, NULL, (hz / IPF_HZ_DIVIDE) * IPF_HZ_MULT);
#endif
	return 0;
}


/*
 * Disable the filter by removing the hooks from the IP input/output
 * stream.
 */
int ipldetach()
{
	int s;
#if defined(NETBSD_PF) && (__NetBSD_Version__ >= 104200000)
	int error = 0;
# if __NetBSD_Version__ >= 105150000
	struct pfil_head *ph_inet = pfil_head_get(PFIL_TYPE_AF, AF_INET);
#  ifdef USE_INET6
	struct pfil_head *ph_inet6 = pfil_head_get(PFIL_TYPE_AF, AF_INET6);
#  endif
# endif
#endif

	SPL_NET(s);

#if (__NetBSD_Version__ >= 104010000)
	callout_stop(&fr_slowtimer_ch);
#else
	untimeout(fr_slowtimer, NULL);
#endif /* NetBSD */

	fr_checkp = fr_savep;
	(void) frflush(IPL_LOGIPF, FR_INQUE|FR_OUTQUE|FR_INACTIVE);
	(void) frflush(IPL_LOGIPF, FR_INQUE|FR_OUTQUE);

	if (fr_control_forwarding & 2)
		ipforwarding = 0;

#ifdef NETBSD_PF
# if (__NetBSD_Version__ >= 104200000)
#  if __NetBSD_Version__ >= 105110000
	if (ph_inet != NULL)
		error = pfil_remove_hook((void *)fr_check_wrapper, NULL,
					 PFIL_IN|PFIL_OUT, ph_inet);
	else
		error = 0;
#  else
	error = pfil_remove_hook((void *)fr_check, PFIL_IN|PFIL_OUT,
				 &inetsw[ip_protox[IPPROTO_IP]].pr_pfh);
#  endif
	if (error)
		return error;
# else
	pfil_remove_hook((void *)fr_check, PFIL_IN|PFIL_OUT);
# endif
# ifdef USE_INET6
#  if __NetBSD_Version__ >= 105110000
	if (ph_inet6 != NULL)
		error = pfil_remove_hook((void *)fr_check_wrapper6, NULL,
					 PFIL_IN|PFIL_OUT, ph_inet6);
	else
		error = 0;
#  else
	error = pfil_remove_hook((void *)fr_check, PFIL_IN|PFIL_OUT,
				 &inetsw[ip_protox[IPPROTO_IPV6]].pr_pfh);
#  endif
	if (error)
		return error;
# endif
#endif
	fr_deinitialise();

	SPL_X(s);
	return 0;
}


/*
 * Filter ioctl interface.
 */
int iplioctl(dev, cmd, data, mode
#if (NetBSD >= 199511)
, p)
struct proc *p;
#else
)
#endif
dev_t dev;
u_long cmd;
caddr_t data;
int mode;
{
	int s;
	int error = 0, unit = 0, tmp;
	friostat_t fio;

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

	error = fr_ioctlswitch(unit, data, cmd, mode);
	if (error != -1) {
		SPL_X(s);
		return error;
	}
	error = 0;

	switch (cmd)
	{
	case FIONREAD :
#ifdef IPFILTER_LOG
		error = COPYOUT(&iplused[IPL_LOGIPF], (caddr_t)data,
			       sizeof(iplused[IPL_LOGIPF]));
#endif
		break;
	case SIOCFRENB :
		if (!(mode & FWRITE))
			error = EPERM;
		else {
			error = COPYIN(data, &tmp, sizeof(tmp));
			if (error)
				break;
			if (tmp) {
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
		if (!(mode & FWRITE)) {
			error = EPERM;
			break;
		}
	case SIOCIPFGETNEXT :
	case SIOCIPFGET :
		error = fr_ipftune(cmd, data);
		break;
	case SIOCSETFF :
		if (!(mode & FWRITE))
			error = EPERM;
		else
			error = COPYIN(data, &fr_flags, sizeof(fr_flags));
		break;
	case SIOCGETFF :
		error = COPYOUT(&fr_flags, data, sizeof(fr_flags));
		break;
	case SIOCFUNCL :
		error = fr_resolvefunc(data);
		break;
	case SIOCINAFR :
	case SIOCRMAFR :
	case SIOCADAFR :
	case SIOCZRLST :
		if (!(mode & FWRITE))
			error = EPERM;
		else
			error = frrequest(unit, cmd, data, fr_active, 1);
		break;
	case SIOCINIFR :
	case SIOCRMIFR :
	case SIOCADIFR :
		if (!(mode & FWRITE))
			error = EPERM;
		else
			error = frrequest(unit, cmd, data, 1 - fr_active, 1);
		break;
	case SIOCSWAPA :
		if (!(mode & FWRITE))
			error = EPERM;
		else {
			bzero((char *)frcache, sizeof(frcache[0]) * 2);
			*(u_int *)data = fr_active;
			fr_active = 1 - fr_active;
		}
		break;
	case SIOCGETFS :
		fr_getstat(&fio);
		error = fr_outobj(data, &fio, IPFOBJ_IPFSTAT);
		break;
	case	SIOCFRZST :
		if (!(mode & FWRITE))
			error = EPERM;
		else
			error = fr_zerostats(data);
		break;
	case	SIOCIPFFL :
		if (!(mode & FWRITE))
			error = EPERM;
		else {
			error = COPYIN(data, &tmp, sizeof(tmp));
			if (!error) {
				tmp = frflush(unit, tmp);
				error = COPYOUT(&tmp, data, sizeof(tmp));
			}
		}
		break;
	case SIOCSTLCK :
		error = COPYIN(data, &tmp, sizeof(tmp));
		if (error == 0) {
			fr_state_lock = tmp;
			fr_nat_lock = tmp;
			fr_frag_lock = tmp;
			fr_auth_lock = tmp;
		} else
			error = EFAULT;
		break;
#ifdef IPFILTER_LOG
	case	SIOCIPFFB :
		if (!(mode & FWRITE))
			error = EPERM;
		else
			*(int *)data = ipflog_clear(unit);
		break;
#endif /* IPFILTER_LOG */
	case SIOCGFRST :
		error = fr_outobj(data, fr_fragstats(), IPFOBJ_FRAGSTAT);
		break;
	case SIOCFRSYN :
		if (!(mode & FWRITE))
			error = EPERM;
		else {
			frsync();
		}
		break;
	default :
		error = EINVAL;
		break;
	}
	SPL_X(s);
	return error;
}


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


/*
 * routines below for saving IP headers to buffer
 */
int iplopen(dev, flags
#if (NetBSD >= 199511)
, devtype, p)
int devtype;
struct proc *p;
#else
)
#endif
dev_t dev;
int flags;
{
	u_int min = GET_MINOR(dev);

	if (IPL_LOGMAX < min)
		min = ENXIO;
	else
		min = 0;
	return min;
}


int iplclose(dev, flags
#if (NetBSD >= 199511)
, devtype, p)
int devtype;
struct proc *p;
#else
)
#endif
dev_t dev;
int flags;
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
#if (BSD >= 199306)
int iplread(dev, uio, ioflag)
int ioflag;
#else
int iplread(dev, uio)
#endif
dev_t dev;
register struct uio *uio;
{
#ifdef IPFILTER_LOG
	return ipflog_read(GET_MINOR(dev), uio);
#else
	return ENXIO;
#endif
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

	m = m_gethdr(M_DONTWAIT, MT_HEADER);
	if (m == NULL)
		return ENOBUFS;
	if (m == NULL)
		return -1;

	tlen = fin->fin_dlen - (TCP_OFF(tcp) << 2) +
			((tcp->th_flags & TH_SYN) ? 1 : 0) +
			((tcp->th_flags & TH_FIN) ? 1 : 0);

#ifdef USE_INET6
	hlen = (fin->fin_v == 6) ? sizeof(ip6_t) : sizeof(ip_t);
#else
	hlen = sizeof(ip_t);
#endif
	m->m_len = sizeof(*tcp2) + hlen;
	m->m_data += max_linkhdr;
	m->m_pkthdr.len = m->m_len;
	m->m_pkthdr.rcvif = (struct ifnet *)0;
	ip = mtod(m, struct ip *);
#ifdef USE_INET6
	ip6 = (ip6_t *)ip;
#endif
	bzero((char *)ip, sizeof(*tcp2) + hlen);
	tcp2 = (struct tcphdr *)((char *)ip + hlen);

	tcp2->th_sport = tcp->th_dport;
	tcp2->th_dport = tcp->th_sport;
	if (tcp->th_flags & TH_ACK) {
		tcp2->th_seq = tcp->th_ack;
		tcp2->th_flags = TH_RST;
	} else {
		tcp2->th_ack = ntohl(tcp->th_seq);
		tcp2->th_ack += tlen;
		tcp2->th_ack = htonl(tcp2->th_ack);
		tcp2->th_flags = TH_RST|TH_ACK;
	}
	TCP_OFF_A(tcp2, sizeof(*tcp2) >> 2);
#ifdef USE_INET6
	if (fin->fin_v == 6) {
		ip6->ip6_plen = htons(sizeof(struct tcphdr));
		ip6->ip6_nxt = IPPROTO_TCP;
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
	ip_t *ip, *oip;

	ip = mtod(m, ip_t *);

	IP_V_A(ip, fin->fin_v);
	switch (fin->fin_v)
	{
	case 4 :
		oip = fin->fin_ip;
		IP_HL_A(ip, sizeof(*oip) >> 2);
		ip->ip_tos = oip->ip_tos;
		ip->ip_id = fin->fin_ip->ip_id;
		ip->ip_off = ip_mtudisc ? IP_DF : 0;
		ip->ip_ttl = ip_defttl;
		ip->ip_sum = 0;
		break;
#ifdef USE_INET6
	case 6 :
	{
		ip6_t *ip6 = (ip6_t *)ip;

		ip6->ip6_hlim = IPDEFTTL;
	}
#endif
	default :
		return EINVAL;
	}
#ifdef IPSEC
	m->m_pkthdr.rcvif = NULL;
#endif
	return fr_fastroute(m, mpp, fin, NULL);
}


int fr_send_icmp_err(type, fin, dst)
int type;
fr_info_t *fin;
int dst;
{
	int err, hlen, xtra, ohlen, iclen, avail, code;
	struct in_addr dst4;
	struct icmp *icmp;
	struct mbuf *m;
	void *ifp;
#ifdef USE_INET6
	ip6_t *ip6, *ip62;
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

	xtra = 0;
	hlen = 0;
	ohlen = 0;
	avail = 0;
	m = NULL;
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
				return 0;
			}

		avail = MHLEN;
		m = m_gethdr(M_DONTWAIT, MT_HEADER);
		if (m == NULL)
			return ENOBUFS;

		if (dst == 0) {
			if (fr_ifpaddr(4, FRI_NORMAL, ifp,
				       &dst4, NULL) == -1) {
				FREE_MB_T(m);
				return -1;
			}
		} else
			dst4.s_addr = fin->fin_daddr;

		hlen = sizeof(ip_t);
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

		MGETHDR(m, M_DONTWAIT, MT_HEADER);
		if (m == NULL)
			return ENOBUFS;

		MCLGET(m, M_DONTWAIT);
		if (m == NULL)
			return ENOBUFS;
		avail = (m->m_flags & M_EXT) ? MCLBYTES : MHLEN;
		xtra = MIN(fin->fin_plen + sizeof(ip6_t),
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

	iclen = hlen + sizeof(*icmp) + xtra;
	avail -= (max_linkhdr + iclen);
	m->m_data += max_linkhdr;
	m->m_pkthdr.rcvif = (struct ifnet *)0;
	if (xtra > avail)
		xtra = avail;
	iclen += xtra;
	m->m_pkthdr.len = iclen;
	if (avail < 0) {
		m_freem(m);
		return ENOBUFS;
	}
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

	bcopy((char *)fin->fin_ip, (char *)ip2, fin->fin_hlen);

#if defined(M_CSUM_IPv4)
	/*
	 * Clear any in-bound checksum flags for this packet.
	 */
	m->m_pkthdr.csuminfo = 0;
#endif /* __NetBSD__ && M_CSUM_IPv4 */

#ifdef USE_INET6
	ip6 = (ip6_t *)ip;
	if (fin->fin_v == 6) {
		ip62 = (ip6_t *)ip2;
		ip62->ip6_plen = htons(ip62->ip6_plen);

		ip6->ip6_flow = 0;
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

		ip->ip_p = IPPROTO_ICMP;
		ip->ip_src.s_addr = dst4.s_addr;
		ip->ip_dst.s_addr = fin->fin_saddr;

		if (xtra > 0)
			bcopy(fin->fin_dp,
			      (char *)&icmp->icmp_ip + fin->fin_hlen, xtra);
		icmp->icmp_cksum = ipf_cksum((u_short *)icmp,
					     sizeof(*icmp) + 8);
		ip->ip_len = iclen;
		ip->ip_p = IPPROTO_ICMP;
	}
	err = fr_send_ip(fin, m, &m);
	return err;
}


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

#ifdef M_WRITABLE
	/*
	* HOT FIX/KLUDGE:
	*
	* If the mbuf we're about to send is not writable (because of
	* a cluster reference, for example) we'll need to make a copy
	* of it since this routine modifies the contents.
	*
	* If you have non-crappy network hardware that can transmit data
	* from the mbuf, rather than making a copy, this is gonna be a
	* problem.
	*/
	if (M_WRITABLE(m) == 0) {
		if ((m0 = m_dup(m, M_DONTWAIT)) != 0) {
			m_freem(m);
			m = m0;
		} else {
			error = ENOBUFS;
			m_freem(m);
			fr_frouteok[1]++;
		}
	}
#endif

	hlen = fin->fin_hlen;
	ip = mtod(m0, struct ip *);

#if defined(M_CSUM_IPv4)
	/*
	 * Clear any in-bound checksum flags for this packet.
	 */
	m0->m_pkthdr.csuminfo = 0;
#endif /* __NetBSD__ && M_CSUM_IPv4 */

#ifdef USE_INET6
	if (fin->fin_v == 6)
		return ipfr_fastroute6(m0, mpp, fin, fdp);
#endif
	/*
	 * Route packet.
	 */
	ro = &iproute;
	bzero((caddr_t)ro, sizeof (*ro));
	dst = (struct sockaddr_in *)&ro->ro_dst;
	dst->sin_family = AF_INET;

	fr = fin->fin_fr;
	if (fdp)
		ifp = fdp->fd_ifp;
	else {
		ifp = fin->fin_ifp;
		dst->sin_addr = ip->ip_dst;
	}

	if ((ifp == NULL) && (!fr || !(fr->fr_flags & FR_FASTROUTE))) {
		error = -2;
		goto bad;
	}

	/*
	 * In case we're here due to "to <if>" being used with "keep state",
	 * check that we're going in the correct direction.
	 */
	if ((fr != NULL) && (fin->fin_rev != 0)) {
		if ((ifp != NULL) && (fdp == &fr->fr_tif))
			return -1;
		dst->sin_addr = ip->ip_dst;
	} else if (fdp) {
		if (fdp->fd_ip.s_addr) {
			dst->sin_addr = fdp->fd_ip;
			ip->ip_dst = fdp->fd_ip;
		} else
			dst->sin_addr = ip->ip_dst;
	}

	dst->sin_len = sizeof(*dst);
	rtalloc(ro);
	if (!ifp) {
		if (ro->ro_rt == 0 || (ifp = ro->ro_rt->rt_ifp) == 0) {
			if (in_localaddr(ip->ip_dst))
				error = EHOSTUNREACH;
			else
				error = ENETUNREACH;
			goto bad;
		}
		if (ro->ro_rt->rt_flags & RTF_GATEWAY)
			dst = (struct sockaddr_in *)&ro->ro_rt->rt_gateway;
	}
	if (ro->ro_rt)
		ro->ro_rt->rt_use++;

	/*
	 * For input packets which are being "fastrouted", they won't
	 * go back through output filtering and miss their chance to get
	 * NAT'd and counted.
	 */
	fin->fin_ifp = ifp;
	if (fin->fin_out == 0) {
		fin->fin_out = 1;
		(void) fr_acctpkt(fin, NULL);
		fin->fin_fr = NULL;
		if (!fr || !(fr->fr_flags & FR_RETMASK)) {
			u_32_t pass;

			(void) fr_checkstate(fin, &pass);
		}
		(void) fr_checknatout(fin, NULL);
	} else
		ip->ip_sum = 0;
	/*
	 * If small enough for interface, can just send directly.
	 */
	if (ip->ip_len <= ifp->if_mtu) {
		ip->ip_len = htons(ip->ip_len);
		ip->ip_off = htons(ip->ip_off);
#if defined(M_CSUM_IPv4)
		if (ifp->if_capabilities & IFCAP_CSUM_IPv4)
			m->m_pkthdr.csuminfo |= M_CSUM_IPv4;
		else if (ip->ip_sum == 0)
			ip->ip_sum = in_cksum(m, hlen);
#else
		if (!ip->ip_sum)
			ip->ip_sum = in_cksum(m, hlen);
#endif /* M_CSUM_IPv4 */
		error = (*ifp->if_output)(ifp, m, (struct sockaddr *)dst,
					  ro->ro_rt);
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
			m_freem(m);
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
	m_freem(m);
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

	if ((fr != NULL) && (fin->fin_rev != 0)) {
		if ((ifp != NULL) && (fdp == &fr->fr_tif))
			return 0;
	} else if (fdp != NULL) {
		if (IP6_NOTZERO(&fdp->fd_ip6))
			dst6->sin6_addr = fdp->fd_ip6.in6;
	}
	if (ifp == NULL)
		return EHOSTUNREACH;

	/* KAME */
	if (IN6_IS_ADDR_LINKLOCAL(&dst6->sin6_addr))
		dst6->sin6_addr.s6_addr16[1] = htons(ifp->if_index);
	rtalloc((struct route *)ro);

	if ((ifp == NULL) && (ro->ro_rt != NULL))
		ifp = ro->ro_rt->rt_ifp;

	if ((ro->ro_rt == NULL) || (ifp == NULL) ||
	    (ifp != ro->ro_rt->rt_ifp)) {
		error = EHOSTUNREACH;
	} else {
#if (__NetBSD_Version__ >= 106010000)
		struct in6_addr finaldst = fin->fin_dst6;
		int frag;
#endif
		if (ro->ro_rt->rt_flags & RTF_GATEWAY)
			dst6 = (struct sockaddr_in6 *)ro->ro_rt->rt_gateway;
		ro->ro_rt->rt_use++;

#if (__NetBSD_Version__ <= 106009999)
		mtu = nd_ifinfo[ifp->if_index].linkmtu;
#else
		/* Determine path MTU. */
		error = ip6_getpmtu(ro, ro, ifp, &finaldst, &mtu, &frag);
			
#endif
		if ((error == 0) && (m0->m_pkthdr.len <= mtu))
			error = nd6_output(ifp, fin->fin_ifp, m0,
						   dst6, ro->ro_rt);
		else
			error = EMSGSIZE;
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

	ifp = ifptr;
	mask = NULL;

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
#if __NetBSD_Version >= 105190000	/* 1.5T */
	size_t asz;
	

	if (fin->fin_v == 4)
		asz = sizeof(struct in_addr);
	else if (fin->fin_v == 6)
		asz = sizeof(fin->fin_src);
	newiss = tcp_new_iss1((void *)&fin->fin_src, (void *)&fin->fin_dst,
			      fin->fin_sport, fin->fin_dport, asz);
#else
	static int iss_seq_off = 0;
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
#ifdef M_CSUM_TCP_UDP_BAD
	int manual = 0, pflag, cflags, active;
	mb_t *m;

	m = fin->fin_m;
	if (m == NULL) {
		manual = 1;
		goto skipauto;
	}

	switch (fin->fin_p)
	{
	case IPPROTO_UDP :
		pflag = M_CSUM_UDPv4;
		break;
	case IPPROTO_TCP :
		pflag = M_CSUM_TCPv4;
		break;
	default :
		pflag = 0;
		manual = 1;
		break;
	}

	active = ((struct ifnet *)fin->fin_ifp)->if_csum_flags_rx & pflag;
	active |=  M_CSUM_TCP_UDP_BAD | M_CSUM_DATA;
	cflags = m->m_pkthdr.csum_flags & active;

	if (pflag != 0) {
		if (cflags == (pflag | M_CSUM_TCP_UDP_BAD)) {
			fin->fin_flx |= FI_BAD;
		} else if (cflags == (pflag | M_CSUM_DATA)) {
			if ((m->m_pkthdr.csum_data ^ 0xffff) != 0)
				fin->fin_flx |= FI_BAD;
		} else if (cflags == pflag) {
			;
		} else {
			manual = 1;
		}
	}
skipauto:
# ifdef IPFILTER_CKSUM
	if (manual != 0)
		if (fr_checkl4sum(fin) == -1)
			fin->fin_flx |= FI_BAD;
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
# ifdef M_CSUM_TCP_UDP_BAD
	int manual = 0, pflag, cflags, active;
	mb_t *m;

	m = fin->fin_m;

	switch (fin->fin_p)
	{
	case IPPROTO_UDP :
		pflag = M_CSUM_UDPv6;
		break;
	case IPPROTO_TCP :
		pflag = M_CSUM_TCPv6;
		break;
	default :
		pflag = 0;
		manual = 1;
		break;
	}

	active = ((struct ifnet *)fin->fin_ifp)->if_csum_flags_rx & pflag;
	active |=  M_CSUM_TCP_UDP_BAD | M_CSUM_DATA;
	cflags = m->m_pkthdr.csum_flags & active;

	if (pflag != 0) {
		if (cflags == (pflag | M_CSUM_TCP_UDP_BAD)) {
			fin->fin_flx |= FI_BAD;
		} else if (cflags == (pflag | M_CSUM_DATA)) {
			if ((m->m_pkthdr.csum_data ^ 0xffff) != 0)
				fin->fin_flx |= FI_BAD;
		} else if (cflags == pflag) {
			;
		} else {
			manual = 1;
		}
	}
#  ifdef IPFILTER_CKSUM
	if (manual != 0)
		if (fr_checkl6sum(fin) == -1)
			fin->fin_flx |= FI_BAD;
#  endif
# else
#  ifdef IPFILTER_CKSUM
	if (fr_checkl6sum(fin) == -1)
		fin->fin_flx |= FI_BAD;
#  endif
# endif
}
#endif /* USE_INET6 */
