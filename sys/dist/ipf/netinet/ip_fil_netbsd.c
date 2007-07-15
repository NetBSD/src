/*	$NetBSD: ip_fil_netbsd.c,v 1.31.2.4 2007/07/15 13:27:23 ad Exp $	*/

/*
 * Copyright (C) 1993-2003 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 */
#if !defined(lint)
static const char sccsid[] = "@(#)ip_fil.c	2.41 6/5/96 (C) 1993-2000 Darren Reed";
static const char rcsid[] = "@(#)Id: ip_fil_netbsd.c,v 2.55.2.51 2007/05/31 12:27:35 darrenr Exp";
#endif

#if defined(KERNEL) || defined(_KERNEL)
# undef KERNEL
# undef _KERNEL
# define        KERNEL	1
# define        _KERNEL	1
#endif
#include <sys/param.h>
#if (NetBSD >= 199905) && !defined(IPFILTER_LKM) && defined(_KERNEL)
# if (__NetBSD_Version__ < 399001400)
#  include "opt_ipfilter_log.h"
# else
#  include "opt_ipfilter.h"
# endif
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
#include <sys/poll.h>
#if (__NetBSD_Version__ >= 399002000)
# include <sys/kauth.h>
#endif

#if (__NetBSD_Version__ >= 399002000)
#include <sys/kauth.h>
#endif

#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#if __NetBSD_Version__ >= 105190000	/* 1.5T */
# include <netinet/tcp_timer.h>
# include <netinet/tcp_var.h>
#endif
#include <netinet/udp.h>
#include <netinet/tcpip.h>
#include <netinet/ip_icmp.h>
#include "netinet/ip_compat.h"
#ifdef USE_INET6
# include <netinet/icmp6.h>
# if (__NetBSD_Version__ >= 106000000)
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
#ifdef INET
extern	int	ip_optcopy __P((struct ip *, struct ip *));
#endif

#ifdef IPFILTER_M_IPFILTER
MALLOC_DEFINE(M_IPFILTER, "IP Filter", "IP Filter packet filter data structures");
#endif

#if __NetBSD_Version__ >= 105009999
# define	csuminfo	csum_flags
#endif

#if __NetBSD_Version__ < 200000000
extern	struct	protosw	inetsw[];
#endif

static	int	(*fr_savep) __P((ip_t *, int, void *, int, struct mbuf **));
static	int	fr_send_ip __P((fr_info_t *, mb_t *, mb_t **));
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
# include <sys/select.h>

# if  (__NetBSD_Version__ >= 399001400)
int iplpoll __P((dev_t dev, int events, struct lwp *p));
# else
int iplpoll __P((dev_t dev, int events, struct proc *p));
# endif

struct selinfo ipfselwait[IPL_LOGSIZE];

const struct cdevsw ipl_cdevsw = {
	iplopen, iplclose, iplread, nowrite, iplioctl,
	nostop, notty, iplpoll, nommap,
# if  (__NetBSD_Version__ >= 200000000)
	nokqfilter,
# endif
# ifdef D_OTHER
	D_OTHER,
# endif
};
#endif


#if defined(__NetBSD_Version__) && (__NetBSD_Version__ >= 105110000)
# include <net/pfil.h>

static int fr_check_wrapper(void *, struct mbuf **, struct ifnet *, int );

static int
fr_check_wrapper(void *arg, struct mbuf **mp, struct ifnet *ifp,
    int dir)
{
	struct ip *ip;
	int rv, hlen;

#if __NetBSD_Version__ >= 200080000
	/*
	 * ensure that mbufs are writable beforehand
	 * as it's assumed by ipf code.
	 * XXX inefficient
	 */
	int error = m_makewritable(mp, 0, M_COPYALL, M_DONTWAIT);

	if (error) {
		m_freem(*mp);
		*mp = NULL;
		return error;
	}
#endif
	ip = mtod(*mp, struct ip *);
	hlen = ip->ip_hl << 2;

#ifdef INET
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
#endif /* INET */

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

static int
fr_check_wrapper6(void *arg, struct mbuf **mp, struct ifnet *ifp,
    int dir)
{
#if defined(INET6)
#  if defined(M_CSUM_TCPv6) && (__NetBSD_Version__ > 200000000)
	/*
	 * If the packet is out-bound, we can't delay checksums
	 * here.  For in-bound, the checksum has already been
	 * validated.
	 */
	if (dir == PFIL_OUT) {
		if ((*mp)->m_pkthdr.csum_flags & (M_CSUM_TCPv6|M_CSUM_UDPv6)) {
#   if (__NetBSD_Version__ > 399000600)
			in6_delayed_cksum(*mp);
#   endif
			(*mp)->m_pkthdr.csum_flags &= ~(M_CSUM_TCPv6|
							M_CSUM_UDPv6);
		}
	}
#  endif
#endif /* INET6 */

	return (fr_check(mtod(*mp, struct ip *), sizeof(struct ip6_hdr),
	    ifp, (dir == PFIL_OUT), mp));
}
# endif


# if defined(PFIL_TYPE_IFNET) && defined(PFIL_IFNET)
static int ipf_pfilsync(void *, struct mbuf **, struct ifnet *, int);

static int ipf_pfilsync(void *hdr, struct mbuf **mp,
    struct ifnet *ifp, int dir)
{
	/*
	 * The interface pointer is useless for create (we have nothing to
	 * compare it to) and at detach, the interface name is still in the
	 * list of active NICs (albeit, down, but that's not any real
	 * indicator) and doing ifunit() on the name will still return the
	 * pointer, so it's not much use then, either.
	 */
	frsync(NULL);
	return 0;
}
# endif

#endif /* __NetBSD_Version__ >= 105110000 */


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
ipfilterattach(int count)
{
# if 0
	if (ipfattach() != 0)
		printf("IP Filter failed to attach\n");
# endif
}
#endif


int ipfattach()
{
	int s;
#if defined(NETBSD_PF) && (__NetBSD_Version__ >= 104200000)
	int error = 0;
# if defined(__NetBSD_Version__) && (__NetBSD_Version__ >= 105110000)
        struct pfil_head *ph_inet;
#  ifdef USE_INET6
        struct pfil_head *ph_inet6;
#  endif
#  if defined(PFIL_TYPE_IFNET) && defined(PFIL_IFNET)
        struct pfil_head *ph_ifsync;
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
#   if defined(PFIL_TYPE_IFNET) && defined(PFIL_IFNET)
	ph_ifsync = pfil_head_get(PFIL_TYPE_IFNET, 0);
#   endif

	if (ph_inet == NULL
#   ifdef USE_INET6
	    && ph_inet6 == NULL
#   endif
#   if defined(PFIL_TYPE_IFNET) && defined(PFIL_IFNET)
	    && ph_ifsync == NULL
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
#  else
	error = pfil_add_hook((void *)fr_check, PFIL_IN|PFIL_OUT,
			      &inetsw[ip_protox[IPPROTO_IP]].pr_pfh);
#  endif
	if (error)
		goto pfil_error;
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
		goto pfil_error;
	}
#  else
	error = pfil_add_hook((void *)fr_check, PFIL_IN|PFIL_OUT,
			      &inetsw[ip_protox[IPPROTO_IPV6]].pr_pfh);
	if (error) {
		pfil_remove_hook((void *)fr_check, PFIL_IN|PFIL_OUT,
				 &inetsw[ip_protox[IPPROTO_IP]].pr_pfh);
		goto pfil_error;
	}
#  endif
# endif

# if defined(PFIL_TYPE_IFNET) && defined(PFIL_IFNET)
	if (ph_ifsync != NULL)
		(void) pfil_add_hook((void *)ipf_pfilsync, NULL,
				     PFIL_IFNET, ph_ifsync);
# endif
#endif

	bzero((char *)ipfselwait, sizeof(ipfselwait));
	bzero((char *)frcache, sizeof(frcache));
	fr_savep = fr_checkp;
	fr_checkp = fr_check;

#ifdef INET
	if (fr_control_forwarding & 1)
		ipforwarding = 1;
#endif

	SPL_X(s);

#if (__NetBSD_Version__ >= 104010000)
	callout_init(&fr_slowtimer_ch, 0);
	callout_reset(&fr_slowtimer_ch, (hz / IPF_HZ_DIVIDE) * IPF_HZ_MULT,
		     fr_slowtimer, NULL);
#else
	timeout(fr_slowtimer, NULL, (hz / IPF_HZ_DIVIDE) * IPF_HZ_MULT);
#endif
	return 0;

#if __NetBSD_Version__ >= 105110000
pfil_error:
	fr_deinitialise();
	SPL_X(s);
	return error;
#endif
}


/*
 * Disable the filter by removing the hooks from the IP input/output
 * stream.
 */
int ipfdetach()
{
	int s;
#if defined(NETBSD_PF) && (__NetBSD_Version__ >= 104200000)
	int error = 0;
# if __NetBSD_Version__ >= 105150000
	struct pfil_head *ph_inet = pfil_head_get(PFIL_TYPE_AF, AF_INET);
#  ifdef USE_INET6
	struct pfil_head *ph_inet6 = pfil_head_get(PFIL_TYPE_AF, AF_INET6);
#  endif
#  if defined(PFIL_TYPE_IFNET) && defined(PFIL_IFNET)
	struct pfil_head *ph_ifsync = pfil_head_get(PFIL_TYPE_IFNET, 0);
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
	(void) frflush(IPL_LOGIPF, 0, FR_INQUE|FR_OUTQUE|FR_INACTIVE);
	(void) frflush(IPL_LOGIPF, 0, FR_INQUE|FR_OUTQUE);

#ifdef INET
	if (fr_control_forwarding & 2)
		ipforwarding = 0;
#endif

#ifdef NETBSD_PF
# if (__NetBSD_Version__ >= 104200000)
#  if __NetBSD_Version__ >= 105110000
#   if defined(PFIL_TYPE_IFNET) && defined(PFIL_IFNET)
	(void) pfil_remove_hook((void *)ipf_pfilsync, NULL,
				PFIL_IFNET, ph_ifsync);
#   endif

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
# if  (__NetBSD_Version__ >= 399001400)
struct lwp *p;
#   if (__NetBSD_Version__ >= 399002000)
#     define	UID(l)	kauth_cred_getuid((l)->l_cred)
#  else
#     define	UID(l)	((l)->l_proc->p_cred->p_ruid)
#  endif
# else
struct proc *p;
#  define	UID(p)	((p)->p_cred->p_ruid)
# endif
#else
)
#endif
dev_t dev;
u_long cmd;
#if  (__NetBSD_Version__ >= 499001000)
void *data;
#else
caddr_t data;
#endif
int mode;
{
	int error = 0, unit = 0;
	SPL_INT(s);

#if (__NetBSD_Version__ >= 399002000)
	if ((mode & FWRITE) &&
	    kauth_authorize_network(p->l_cred, KAUTH_NETWORK_FIREWALL,
				    KAUTH_REQ_NETWORK_FIREWALL_FW, NULL,
				    NULL, NULL)) {
		return EPERM;
	}
#else
	if ((securelevel >= 2) && (mode & FWRITE)) {
		return EPERM;
	}
#endif

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

	error = fr_ioctlswitch(unit, data, cmd, mode, UID(p), p);

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
int iplopen(
    dev_t dev,
    int flags,
#if (NetBSD >= 199511)
    int devtype,
#endif
# if  (__NetBSD_Version__ >= 399001400)
    struct lwp *p
# else
    struct proc *p
# endif
)
{
	u_int xmin = GET_MINOR(dev);

	if (IPL_LOGMAX < xmin)
		xmin = ENXIO;
	else
		xmin = 0;
	return xmin;
}


int iplclose(
    dev_t dev,
    int flags,
#if (NetBSD >= 199511)
    int devtype,
#endif
# if  (__NetBSD_Version__ >= 399001400)
    struct lwp *p
# else
    struct proc *p
# endif
)
{
	u_int	xmin = GET_MINOR(dev);

	if (IPL_LOGMAX < xmin)
		xmin = ENXIO;
	else
		xmin = 0;
	return xmin;
}

/*
 * iplread/ipllog
 * both of these must operate with at least splnet() lest they be
 * called during packet processing and cause an inconsistancy to appear in
 * the filter lists.
 */
int iplread(
    dev_t dev,
    struct uio *uio,
#if (BSD >= 199306)
    int ioflag
#endif
)
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
int iplwrite(
    dev_t dev,
    struct uio *uio,
#if (BSD >= 199306)
    int ioflag
#endif
)
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
#ifdef MGETHDR
	MGETHDR(m, M_DONTWAIT, MT_HEADER);
#else
	MGET(m, M_DONTWAIT, MT_HEADER);
#endif
	if (m == NULL)
		return -1;
	if (sizeof(*tcp2) + hlen > MHLEN) {
		MCLGET(m, M_DONTWAIT);
		if (m == NULL)
			return -1;
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
	bzero((char *)ip, sizeof(*tcp2) + hlen);
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
	tcp2->th_x2 = 0;
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
#ifdef INET
	ip->ip_p = IPPROTO_TCP;
	ip->ip_len = htons(sizeof(struct tcphdr));
	ip->ip_src.s_addr = fin->fin_daddr;
	ip->ip_dst.s_addr = fin->fin_saddr;
	tcp2->th_sum = in_cksum(m, hlen + sizeof(*tcp2));
	ip->ip_len = hlen + sizeof(*tcp2);
	return fr_send_ip(fin, m, &m);
#else
	return 0;
#endif
}


static int fr_send_ip(fin, m, mpp)
fr_info_t *fin;
mb_t *m, **mpp;
{
	fr_info_t fnew;
#ifdef INET
	ip_t *oip;
#endif
	ip_t *ip;
	int hlen;

	ip = mtod(m, ip_t *);
	bzero((char *)&fnew, sizeof(fnew));

	IP_V_A(ip, fin->fin_v);
	switch (fin->fin_v)
	{
#ifdef INET
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
#endif
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
			if (m == NULL)
				return -1;
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

	icmp->icmp_type = type;
	icmp->icmp_code = fin->fin_icode;
	icmp->icmp_cksum = 0;
#ifdef icmp_nextmtu
	if (type == ICMP_UNREACH &&
	    fin->fin_icode == ICMP_UNREACH_NEEDFRAG && ifp)
		icmp->icmp_nextmtu = htons(((struct ifnet *)ifp)->if_mtu);
#endif

	bcopy((char *)fin->fin_ip, (char *)ip2, ohlen);

#if defined(M_CSUM_IPv4)
	/*
	 * Clear any in-bound checksum flags for this packet.
	 */
	m->m_pkthdr.csuminfo = 0;
#endif /* __NetBSD__ && M_CSUM_IPv4 */

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


int fr_fastroute(m0, mpp, fin, fdp)
mb_t *m0, **mpp;
fr_info_t *fin;
frdest_t *fdp;
{
	int error = 0;
#ifdef INET
	struct ip *ip, *mhip;
	struct mbuf *m = m0;
	struct route *ro;
	int off, len, hlen, code;
	struct ifnet *ifp, *sifp;
#if __NetBSD_Version__ >= 499001100
	const struct sockaddr *dst;
	union {
		struct sockaddr         dst;
		struct sockaddr_in      dst4;
	} u;
#else
	struct sockaddr_in *dst;
#endif
	struct route iproute;
	u_short ip_off;
	frentry_t *fr;
#endif

	if (fin->fin_v == 6) {
#ifdef USE_INET6
		error = ipfr_fastroute6(m0, mpp, fin, fdp);
#else
		error = EPROTONOSUPPORT;
#endif
		if ((error != 0) && (*mpp != NULL)) {
			FREE_MB_T(*mpp);
			*mpp = NULL;
		}
		return error;
	}
#ifndef INET
	return EPROTONOSUPPORT;
#else

	hlen = fin->fin_hlen;
	ip = mtod(m0, struct ip *);

# if defined(M_CSUM_IPv4)
	/*
	 * Clear any in-bound checksum flags for this packet.
	 */
	m0->m_pkthdr.csuminfo = 0;
# endif /* __NetBSD__ && M_CSUM_IPv4 */

	/*
	 * Route packet.
	 */
	ro = &iproute;
	memset(ro, 0, sizeof(*ro));

	if (fdp != NULL)
		ifp = fdp->fd_ifp;
	else
		ifp = fin->fin_ifp;
	fr = fin->fin_fr;

	if ((ifp == NULL) && (!fr || !(fr->fr_flags & FR_FASTROUTE))) {
		error = -2;
		goto bad;
	}

# if __NetBSD_Version__ >= 499001100
	if ((fdp != NULL) && (fdp->fd_ip.s_addr != 0))
		sockaddr_in_init(&u.dst4, &fdp->fd_ip, 0);
	else
		sockaddr_in_init(&u.dst4, &ip->ip_dst, 0);
	dst = &u.dst;
	rtcache_setdst(ro, dst);
	rtcache_init(ro);
# else
	dst = (struct sockaddr_in *)&ro->ro_dst;
	dst->sin_family = AF_INET;
	dst->sin_addr = ip->ip_dst;

	if ((fdp != NULL) && (fdp->fd_ip.s_addr != 0))
		dst->sin_addr = fdp->fd_ip;
	dst->sin_len = sizeof(*dst);
	rtalloc(ro);
# endif

	if ((ifp == NULL) && (ro->ro_rt != NULL))
		ifp = ro->ro_rt->rt_ifp;

	if ((ro->ro_rt == NULL) || (ifp == NULL)) {
#ifdef INET
		if (in_localaddr(ip->ip_dst))
			error = EHOSTUNREACH;
		else
#endif
			error = ENETUNREACH;
		goto bad;
	}

# if __NetBSD_Version__ >= 499001100
	if (ro->ro_rt->rt_flags & RTF_GATEWAY)
		dst = ro->ro_rt->rt_gateway;
# else
	if (ro->ro_rt->rt_flags & RTF_GATEWAY)
		dst = (struct sockaddr_in *)ro->ro_rt->rt_gateway;
#endif /* __NetBSD_Version__ < 499001100 */

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

			if (fr_checkstate(fin, &pass) != NULL)
				fr_statederef((ipstate_t **)&fin->fin_state);
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

		if (m->m_flags & M_EXT)
			i = 1;

		ip->ip_len = htons(ip->ip_len);
		ip->ip_off = htons(ip->ip_off);
# if defined(M_CSUM_IPv4)
#  if (__NetBSD_Version__ >= 105009999)
		if (ifp->if_csum_flags_tx & M_CSUM_IPv4)
			m->m_pkthdr.csuminfo |= M_CSUM_IPv4;
#  else
		if (ifp->if_capabilities & IFCAP_CSUM_IPv4)
			m->m_pkthdr.csuminfo |= M_CSUM_IPv4;
#  endif /* (__NetBSD_Version__ >= 105009999) */
		else if (ip->ip_sum == 0)
			ip->ip_sum = in_cksum(m, hlen);
# else
		if (!ip->ip_sum)
			ip->ip_sum = in_cksum(m, hlen);
# endif /* M_CSUM_IPv4 */
# if __NetBSD_Version__ >= 499001100
		error = (*ifp->if_output)(ifp, m, dst, ro->ro_rt);
# else
		error = (*ifp->if_output)(ifp, m, (struct sockaddr *)dst,
					  ro->ro_rt);
# endif
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
# ifdef MGETHDR
		MGETHDR(m, M_DONTWAIT, MT_HEADER);
# else
		MGET(m, M_DONTWAIT, MT_HEADER);
# endif
		if (m == 0) {
			m = m0;
			error = ENOBUFS;
			goto bad;
		}
		m->m_data += max_linkhdr;
		mhip = mtod(m, struct ip *);
		bcopy((char *)ip, (char *)mhip, sizeof(*ip));
#ifdef INET
		if (hlen > sizeof (struct ip)) {
			mhlen = ip_optcopy(ip, mhip) + sizeof (struct ip);
			IP_HL_A(mhip, mhlen >> 2);
		}
#endif
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
#ifdef INET
		mhip->ip_sum = in_cksum(m, mhlen);
#endif
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
#ifdef INET
	ip->ip_sum = in_cksum(m0, hlen);
#endif
sendorfree:
	for (m = m0; m; m = m0) {
		m0 = m->m_act;
		m->m_act = 0;
# if __NetBSD_Version__ >= 499001100
		if (error == 0)
			error = (*ifp->if_output)(ifp, m, dst, ro->ro_rt);
		else
			FREE_MB_T(m);
# else
		if (error == 0)
			error = (*ifp->if_output)(ifp, m,
			    (struct sockaddr *)dst, ro->ro_rt);
		else
			FREE_MB_T(m);
# endif
	}
    }
done:
	if (!error)
		fr_frouteok[0]++;
	else
		fr_frouteok[1]++;

# if __NetBSD_Version__ >= 499001100
	rtcache_free(ro);
# else
	if (ro->ro_rt) {
		RTFREE(((struct route *)ro)->ro_rt);
	}
# endif
	*mpp = NULL;
	return error;
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
#endif /* INET */
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
# if __NetBSD_Version__ >= 499001100
	struct route ip6route;
	const struct sockaddr *dst;
	union {
		struct sockaddr         dst;
		struct sockaddr_in6     dst6;
	} u;
	struct route *ro;
# else
	struct route_in6 ip6route;
	struct sockaddr_in6 *dst6;
	struct route_in6 *ro;
# endif
	struct rtentry *rt;
	struct ifnet *ifp;
	frentry_t *fr;
	u_long mtu;
	int error;

	error = 0;
	ro = &ip6route;
	fr = fin->fin_fr;

	if (fdp != NULL)
		ifp = fdp->fd_ifp;
	else
		ifp = fin->fin_ifp;
	memset(ro, 0, sizeof(*ro));
# if __NetBSD_Version__ >= 499001100
	if (fdp != NULL && IP6_NOTZERO(&fdp->fd_ip6))
		sockaddr_in6_init(&u.dst6, &fdp->fd_ip6.in6, 0, 0, 0);
	else
		sockaddr_in6_init(&u.dst6, &fin->fin_fi.fi_dst.in6, 0, 0, 0);
	dst = &u.dst;
	rtcache_setdst(ro, dst);

	rtcache_init(ro);
# else
	dst6 = (struct sockaddr_in6 *)&ro->ro_dst;
	dst6->sin6_family = AF_INET6;
	dst6->sin6_len = sizeof(struct sockaddr_in6);
	dst6->sin6_addr = fin->fin_fi.fi_dst.in6;

	if (fdp != NULL) {
		if (IP6_NOTZERO(&fdp->fd_ip6))
			dst6->sin6_addr = fdp->fd_ip6.in6;
	}
	rtalloc((struct route *)ro);
# endif

	if ((ifp == NULL) && (ro->ro_rt != NULL))
		ifp = ro->ro_rt->rt_ifp;

	if ((ro->ro_rt == NULL) || (ifp == NULL)) {
		error = EHOSTUNREACH;
		goto bad;
	}

	rt = fdp ? NULL : ro->ro_rt;

	/* KAME */
# if __NetBSD_Version__ >= 499001100
	if (IN6_IS_ADDR_LINKLOCAL(&u.dst6.sin6_addr))
		u.dst6.sin6_addr.s6_addr16[1] = htons(ifp->if_index);
# else
	if (IN6_IS_ADDR_LINKLOCAL(&dst6->sin6_addr))
		dst6->sin6_addr.s6_addr16[1] = htons(ifp->if_index);
# endif

	{
# if (__NetBSD_Version__ >= 106010000)
#  if (__NetBSD_Version__ >= 399001400)
		struct in6_ifextra *ife;
#  else
		struct in6_addr finaldst = fin->fin_dst6;
		int frag;
#  endif
# endif
# if __NetBSD_Version__ >= 499001100
		if (ro->ro_rt->rt_flags & RTF_GATEWAY)
			dst = ro->ro_rt->rt_gateway;
# else
		if (ro->ro_rt->rt_flags & RTF_GATEWAY)
			dst6 = (struct sockaddr_in6 *)ro->ro_rt->rt_gateway;
# endif
		ro->ro_rt->rt_use++;

		/* Determine path MTU. */
# if (__NetBSD_Version__ <= 106009999)
		mtu = nd_ifinfo[ifp->if_index].linkmtu;
# else
#  if (__NetBSD_Version__ >= 399001400)
		ife = (struct in6_ifextra *)(ifp)->if_afdata[AF_INET6];
		mtu = ife->nd_ifinfo[ifp->if_index].linkmtu;
#  else
		error = ip6_getpmtu(ro, ro, ifp, &finaldst, &mtu, &frag);
#  endif
# endif
		if ((error == 0) && (m0->m_pkthdr.len <= mtu)) {
			*mpp = NULL;
# if __NetBSD_Version__ >= 499001100
			error = nd6_output(ifp, ifp, m0, satocsin6(dst), rt);
# else
			error = nd6_output(ifp, ifp, m0, dst6, rt);
# endif
		} else {
			error = EMSGSIZE;
		}
	}
bad:
# if __NetBSD_Version__ >= 499001100
	rtcache_free(ro);
# else
	if (ro->ro_rt != NULL) {
		RTFREE(((struct route *)ro)->ro_rt);
	}
# endif
	return error;
}
#endif	/* INET6 */


int fr_verifysrc(fin)
fr_info_t *fin;
{
#if __NetBSD_Version__ >= 499001100
	union {
		struct sockaddr         dst;
		struct sockaddr_in      dst4;
	} u;
#else
	struct sockaddr_in *dst;
#endif
	struct route iproute;
	int rc;

#if __NetBSD_Version__ >= 499001100
	sockaddr_in_init(&u.dst4, &fin->fin_src, 0);
	rtcache_setdst(&iproute, &u.dst);
	rtcache_init(&iproute);
	if (iproute.ro_rt == NULL)
		rc = 0;
	else
		rc = (fin->fin_ifp == iproute.ro_rt->rt_ifp);
	rtcache_free(&iproute);
#else
	dst = (struct sockaddr_in *)&iproute.ro_dst;
	dst->sin_len = sizeof(*dst);
	dst->sin_family = AF_INET;
	dst->sin_addr = fin->fin_src;
	rtalloc(&iproute);
	if (iproute.ro_rt == NULL)
		return 0;
	rc = (fin->fin_ifp == iproute.ro_rt->rt_ifp);
	RTFREE(iproute.ro_rt);
#endif
	return rc;
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
	mask = NULL;

	if (v == 4)
		inp->s_addr = 0;
#ifdef USE_INET6
	else if (v == 6)
		bzero((char *)inp, sizeof(struct in6_addr));
#endif

	ifa = ifp->if_addrlist.tqh_first;
	sock = ifa ? ifa->ifa_addr : NULL;
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
#if __NetBSD_Version__ >= 105190000	/* 1.5T */
	size_t asz;

	if (fin->fin_v == 4)
		asz = sizeof(struct in_addr);
	else if (fin->fin_v == 6)
		asz = sizeof(fin->fin_src);
	else	/* XXX: no way to return error */
		return 0;
#ifdef INET
	return tcp_new_iss1((void *)&fin->fin_src, (void *)&fin->fin_dst,
			    fin->fin_sport, fin->fin_dport, asz, 0);
#else
	return ENOSYS;
#endif
#else
	static int iss_seq_off = 0;
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
#endif
}


/* ------------------------------------------------------------------------ */
/* Function:    fr_nextipid                                                 */
/* Returns:     int - 0 == success, -1 == error (packet should be droppped) */
/* Parameters:  fin(I) - pointer to packet information                      */
/*                                                                          */
/* Returns the next IPv4 ID to use for this packet.                         */
/* ------------------------------------------------------------------------ */
u_short fr_nextipid(fr_info_t *fin)
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
	active |= M_CSUM_TCP_UDP_BAD | M_CSUM_DATA;
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
	int manual, pflag, cflags, active;
	mb_t *m;

	if ((fin->fin_flx & FI_NOCKSUM) != 0)
		return;

	manual = 0;
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
	active |= M_CSUM_TCP_UDP_BAD | M_CSUM_DATA;
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
		if (fr_checkl4sum(fin) == -1)
			fin->fin_flx |= FI_BAD;
#  endif
# else
#  ifdef IPFILTER_CKSUM
	if (fr_checkl4sum(fin) == -1)
		fin->fin_flx |= FI_BAD;
#  endif
# endif
}
#endif /* USE_INET6 */


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
/* We assume that 'xmin' is a pointer to a buffer that is part of the chain */
/* of buffers that starts at *fin->fin_mp.                                  */
/* ------------------------------------------------------------------------ */
void *fr_pullup(xmin, fin, len)
mb_t *xmin;
fr_info_t *fin;
int len;
{
	int out = fin->fin_out, dpoff, ipoff;
	mb_t *m = xmin;
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


int iplpoll(dev, events, p)
dev_t dev;
int events;
#if  (__NetBSD_Version__ >= 399001400)
struct lwp *p;
#else
struct proc *p;
#endif
{
	u_int xmin = GET_MINOR(dev);
	int revents = 0;

	if (IPL_LOGMAX < xmin)
		return ENXIO;

	switch (xmin)
	{
	case IPL_LOGIPF :
	case IPL_LOGNAT :
	case IPL_LOGSTATE :
#ifdef IPFILTER_LOG
		if ((events & (POLLIN | POLLRDNORM)) && ipflog_canread(xmin))
			revents |= events & (POLLIN | POLLRDNORM);
#endif
		break;
	case IPL_LOGAUTH :
		if ((events & (POLLIN | POLLRDNORM)) && fr_auth_waiting())
			revents |= events & (POLLIN | POLLRDNORM);
		break;
	case IPL_LOGSYNC :
#ifdef IPFILTER_SYNC
		if ((events & (POLLIN | POLLRDNORM)) && ipfsync_canread())
			revents |= events & (POLLIN | POLLRDNORM);
		if ((events & (POLLOUT | POLLWRNORM)) && ipfsync_canwrite())
			revents |= events & (POLLOUT | POLLOUTNORM);
#endif
		break;
	case IPL_LOGSCAN :
	case IPL_LOGLOOKUP :
	default :
		break;
	}

	if ((revents == 0) && ((events & (POLLIN|POLLRDNORM)) != 0))
		selrecord(p, &ipfselwait[xmin]);
	return revents;
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
		error = ip_output(m, NULL, NULL, IP_FORWARDING, NULL);
	}

	return error;
}
