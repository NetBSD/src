/*	$NetBSD: ip_fil_linux.c,v 1.1.1.5 2007/04/14 20:17:22 martin Exp $	*/

#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
# define __irq_h        1       /* stop it being included! */
#else
# define _LINUX_TCP_H
#endif
#include <net/ip.h>

#include "ipf-linux.h"

#include <net/checksum.h>
#include <net/route.h>

#include <linux/random.h>
#include <linux/timer.h>
#include <asm/ioctls.h>

extern int sysctl_ip_default_ttl;

static	int	fr_send_ip __P((fr_info_t *, struct sk_buff *, struct sk_buff **));

ipfmutex_t	ipl_mutex, ipf_authmx, ipf_rw, ipf_stinsert;
ipfmutex_t	ipf_nat_new, ipf_natio, ipf_timeoutlock;
ipfrwlock_t	ipf_mutex, ipf_global, ipf_ipidfrag, ipf_frcache, ipf_tokens;
ipfrwlock_t	ipf_frag, ipf_state, ipf_nat, ipf_natfrag, ipf_auth;
struct timer_list	ipf_timer;

static u_int ipf_linux_inout __P((u_int, struct sk_buff **, const struct net_device *, const struct net_device *, int (*okfn)(struct sk_buff *)));

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
static struct nf_hook_ops ipf_hooks[] = {
	{
		.hook		= ipf_linux_inout,
		.owner		= THIS_MODULE,
		.pf		= PF_INET,
		.hooknum	= NF_IP_PRE_ROUTING,
		.priority	= 200,
	},
	{
		.hook		= ipf_linux_inout,
		.owner		= THIS_MODULE,
		.pf		= PF_INET,
		.hooknum	= NF_IP_POST_ROUTING,
		.priority	= 200,
	},
# ifdef USE_INET6
	{
		.hook		= ipf_linux_inout,
		.owner		= THIS_MODULE,
		.pf		= PF_INET6,
		.hooknum	= NF_IP_PRE_ROUTING,
		.priority	= 200,
	},
	{
		.hook		= ipf_linux_inout,
		.owner		= THIS_MODULE,
		.pf		= PF_INET6,
		.hooknum	= NF_IP_POST_ROUTING,
		.priority	= 200,
	}
# endif
};
#else
static struct	nf_hook_ops	ipf_hooks[] = {
	{
		{ NULL, NULL },		/* list */
		ipf_linux_inout,	/* hook */
		PF_INET,		/* pf */
		NF_IP_PRE_ROUTING,	/* hooknum */
		200			/* priority */
	},
	{
		{ NULL, NULL},		/* list */
		ipf_linux_inout,	/* hook */
		PF_INET,		/* pf */
		NF_IP_POST_ROUTING,	/* hooknum */
		200			/* priority */
	},
# ifdef USE_INET6
	{
		{ NULL, NULL },		/* list */
		ipf_linux_inout,	/* hook */
		PF_INET6,		/* pf */
		NF_IP_PRE_ROUTING,	/* hooknum */
		200			/* priority */
	},
	{
		{ NULL, NULL},		/* list */
		ipf_linux_inout,	/* hook */
		PF_INET6,		/* pf */
		NF_IP_POST_ROUTING,	/* hooknum */
		200			/* priority */
	}
# endif
};
#endif


int ipfattach()
{
	int err, i;

	SPL_NET(s);
	if (fr_running > 0) {
		SPL_X(s);
		return -EBUSY;
	}

	bzero((char *)frcache, sizeof(frcache));
	MUTEX_INIT(&ipf_rw, "ipf rw mutex");
	MUTEX_INIT(&ipl_mutex, "ipf log mutex");
	MUTEX_INIT(&ipf_timeoutlock, "ipf timeout lock mutex");
	RWLOCK_INIT(&ipf_global, "ipf global rwlock");
	RWLOCK_INIT(&ipf_mutex, "ipf global mutex rwlock");
	RWLOCK_INIT(&ipf_frcache, "ipf cache mutex rwlock");
	RWLOCK_INIT(&ipf_ipidfrag, "ipf IP NAT-Frag rwlock");
	RWLOCK_INIT(&ipf_tokens, "ipf token rwlock");

	for (i = 0; i < sizeof(ipf_hooks)/sizeof(ipf_hooks[0]); i++) {
		err = nf_register_hook(&ipf_hooks[i]);
		if (err != 0)
			return err;
	}

	if (fr_initialise() < 0) {
		for (i = 0; i < sizeof(ipf_hooks)/sizeof(ipf_hooks[0]); i++)
			nf_unregister_hook(&ipf_hooks[i]);
		SPL_X(s);
		return EIO;
	}

	bzero((char *)frcache, sizeof(frcache));
#ifdef notyet
	if (fr_control_forwarding & 1)
		ipv4_devconf.forwarding = 1;
#endif

	SPL_X(s);
	/* timeout(fr_slowtimer, NULL, (hz / IPF_HZ_DIVIDE) * IPF_HZ_MULT); */
	init_timer(&ipf_timer);
	ipf_timer.function = fr_slowtimer;
	ipf_timer.data = NULL;
	ipf_timer.expires = (HZ / IPF_HZ_DIVIDE) * IPF_HZ_MULT;
	add_timer(&ipf_timer);
	mod_timer(&ipf_timer, HZ/2 + jiffies);
	return 0;
}


int ipfdetach()
{
	int i;

	del_timer(&ipf_timer);

	SPL_NET(s);

	for (i = 0; i < sizeof(ipf_hooks)/sizeof(ipf_hooks[0]); i++)
		nf_unregister_hook(&ipf_hooks[i]);
	/* untimeout(fr_slowtimer, NULL); */

#ifdef notyet
	if (fr_control_forwarding & 2)
		ipv4_devconf.forwarding = 0;
#endif

	fr_deinitialise();

	(void) frflush(IPL_LOGIPF, 0, FR_INQUE|FR_OUTQUE|FR_INACTIVE);
	(void) frflush(IPL_LOGIPF, 0, FR_INQUE|FR_OUTQUE);

	MUTEX_DESTROY(&ipf_timeoutlock);
	MUTEX_DESTROY(&ipl_mutex);
	MUTEX_DESTROY(&ipf_rw);
	RW_DESTROY(&ipf_mutex);
	RW_DESTROY(&ipf_frcache);
	RW_DESTROY(&ipf_global);
	RW_DESTROY(&ipf_tokens);
	RW_DESTROY(&ipf_ipidfrag);

	SPL_X(s);

	return 0;
}


/*
 * Filter ioctl interface.
 */
int ipf_ioctl(struct inode *in, struct file *fp, u_int cmd, u_long arg)
{
	int error = 0, unit = 0;
	caddr_t data;
	mode_t mode;

	unit = MINOR(in->i_rdev);
	if (unit < 0 || unit > IPL_LOGMAX)
		return -ENXIO;

	if (fr_running <= 0) {
		if (unit != IPL_LOGIPF)
			return -EIO;
		if (cmd != SIOCIPFGETNEXT && cmd != SIOCIPFGET &&
		    cmd != SIOCIPFSET && cmd != SIOCFRENB &&
		    cmd != SIOCGETFS && cmd != SIOCGETFF)
			return -EIO;
	}

	mode = fp->f_mode;
	data = (caddr_t)arg;

	error = fr_ioctlswitch(unit, data, cmd, mode, fp->f_uid, fp);
	if (error != -1) {
		SPL_X(s);
		if (error > 0)
			error = -error;
		return error;
	}
	SPL_X(s);

	if (error > 0)
		error = -error;
	return error;
}


u_32_t fr_newisn(fin)
fr_info_t *fin;
{
	u_32_t isn;

	isn = secure_tcp_sequence_number(fin->fin_daddr, fin->fin_saddr,
					 fin->fin_dport, fin->fin_sport);
	return isn;
}


int fr_send_reset(fin)
fr_info_t *fin;
{
	tcphdr_t *tcp, *tcp2;
	int tlen, hlen;
#ifdef	USE_INET6
	ip6_t *ip6;
#endif
	ip_t *ip;
	mb_t *m;

	tcp = fin->fin_dp;
	if (tcp->th_flags & TH_RST)
		return -1;

#ifndef	IPFILTER_CKSUM
	if (fr_checkl4sum(fin) == -1)
		return -1;
#endif
	m = skb_copy(fin->fin_m, GFP_ATOMIC);
	if (m == NULL)
		return -1;

	tlen = (tcp->th_flags & (TH_SYN|TH_FIN)) ? 1 : 0;
#ifdef	USE_INET6
	if (fin->fin_v == 6)
		hlen = sizeof(ip6_t);
	else
#endif
		hlen = sizeof(ip_t);
	hlen += sizeof(*tcp2);
	skb_trim(m, hlen);

	bzero(MTOD(m, char *), hlen);
	ip = MTOD(m, ip_t *);
	bzero((char *)ip, hlen);
	ip->ip_v = fin->fin_v;
	tcp2 = (tcphdr_t *)((char *)ip + hlen - sizeof(*tcp2));
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
	tcp2->th_off = sizeof(*tcp2) >> 2;

#ifdef	USE_INET6
	if (fin->fin_v == 6) {
		ip6 = (ip6_t *)ip;
		ip6->ip6_src = fin->fin_dst6;
		ip6->ip6_dst = fin->fin_src6;
		ip6->ip6_plen = htons(sizeof(*tcp));
		ip6->ip6_nxt = IPPROTO_TCP;
	} else
#endif
	{
		ip->ip_hl = sizeof(*ip) >> 2;
		ip->ip_src.s_addr = fin->fin_daddr;
		ip->ip_dst.s_addr = fin->fin_saddr;
		ip->ip_p = IPPROTO_TCP;
		ip->ip_len = sizeof(*ip) + sizeof(*tcp);
		tcp2->th_sum = fr_cksum(m, ip, IPPROTO_TCP, tcp2, ip->ip_len);
	}
	return fr_send_ip(fin, m, &m);
}


static int fr_send_ip(fin, sk, skp)
fr_info_t *fin;
struct sk_buff *sk, **skp;
{
	fr_info_t fnew;
	ip_t *ip, *oip;
	int hlen;

	ip = MTOD(sk, ip_t *);
	bzero((char *)&fnew, sizeof(fnew));
	oip = fin->fin_ip;

	switch (fin->fin_v)
	{
	case 4 :
		fnew.fin_v = 4;
		ip->ip_hl = sizeof(*oip) >> 2;
		ip->ip_tos = oip->ip_tos;
		ip->ip_id = 0;
		ip->ip_ttl = sysctl_ip_default_ttl;
		ip->ip_sum = 0;
		ip->ip_off = 0x4000;
		hlen = sizeof(*ip);
		break;
	default :
		return EINVAL;
	}

	fnew.fin_ifp = fin->fin_ifp;
	fnew.fin_flx = FI_NOCKSUM;
	fnew.fin_m = sk;
	fnew.fin_ip = ip;
	fnew.fin_mp = skp;
	fnew.fin_hlen = hlen;
	fnew.fin_dp = (char *)ip + hlen;
	(void) fr_makefrip(hlen, ip, &fnew);

	return fr_fastroute(sk, skp, &fnew, NULL);
}


int fr_send_icmp_err(type, fin, isdst)
int type;
fr_info_t *fin;
int isdst;
{
	int hlen, code, leader, dlen;
	struct net_device *ifp;
	struct in_addr dst4;
	struct icmp *icmp;
	u_short sz;
#ifdef	USE_INET6
	ip6_t *ip6;
	mb_t *mb;
#endif
	ip_t *ip;
	mb_t *m, *m0;

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

	m0 = fin->fin_m;
#ifdef USE_INET6
	if (fin->fin_v == 6) {
		sz = sizeof(ip6_t);
		dlen = MIN(M_LEN(m0), 512);
		sz += dlen;
		hlen = sizeof(ip6_t);
		type = icmptoicmp6types[type];
		if (type == ICMP6_DST_UNREACH)
			code = icmptoicmp6unreach[code];
	} else
#endif
	{
		if ((fin->fin_p == IPPROTO_ICMP) && !(fin->fin_flx & FI_SHORT))
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

		sz = sizeof(ip_t) * 2 + 4;
		dlen = 8;		/* 64 bits of data */
		sz += dlen;
		hlen = sizeof(ip_t);
	}

	leader = m0->data - m0->head;
	if ((leader & 15) != 0)
		leader += 16 - (leader & 15);
        m = alloc_skb(sz + leader, GFP_ATOMIC);
        if (m == NULL)
                return -1;
                
        /* Set the data pointer */
        skb_reserve(m, leader);

	bzero(MTOD(m, char *), (size_t)sz);

	m->nh.iph = (struct iphdr *)skb_put(m, hlen);
	ip = (ip_t *)m->nh.iph;
	ip->ip_v = fin->fin_v;

	m->h.icmph = (struct icmphdr *)skb_put(m, hlen + 4 + dlen);
	icmp = (icmphdr_t *)m->h.icmph;
	icmp->icmp_type = type & 0xff;
	icmp->icmp_code = code & 0xff;
#ifdef	icmp_nextmtu
	ifp = fin->fin_ifp;
	if (type == ICMP_UNREACH && (ifp->mtu != 0) &&
	    fin->fin_icode == ICMP_UNREACH_NEEDFRAG)
		icmp->icmp_nextmtu = htons(ifp->mtu);
#endif

#ifdef	USE_INET6
	if (fin->fin_v == 6) {
		struct in6_addr dst6;
		int csz;

		if (isdst == 0) {
			if (fr_ifpaddr(6, FRI_NORMAL, qif->qf_ill,
				       (struct in_addr *)&dst6, NULL) == -1) {
				FREE_MB_T(m);
				return -1;
			}
		} else
			dst6 = fin->fin_dst6;

		csz = sz;
		sz -= sizeof(ip6_t);
		ip6 = (ip6_t *)ip;
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
		ip = MTOD(m, ip_t *);
		ip->ip_hl = sizeof(*ip) >> 2;
		ip->ip_p = IPPROTO_ICMP;
		ip->ip_len = (u_short)sz;
		if (isdst == 0) {
			if (fr_ifpaddr(4, FRI_NORMAL, fin->fin_ifp,
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
		icmp->icmp_ip.ip_len = htons(icmp->icmp_ip.ip_len);
		icmp->icmp_ip.ip_off = htons(icmp->icmp_ip.ip_off);
		bcopy((char *)fin->fin_ip + fin->fin_hlen,
		      (char *)&icmp->icmp_ip + sizeof(*fin->fin_ip), 8);
		icmp->icmp_cksum = ip_compute_csum((u_char *)icmp,
						   sz - sizeof(ip_t));
	}

	/*
	 * Need to exit out of these so we don't recursively call rw_enter
	 * from fr_qout.
	 */
	return fr_send_ip(fin, m, &m);
}


int fr_verifysrc(fin)
fr_info_t *fin;
{
	return 0;
}


void fr_checkv4sum(fin)
fr_info_t *fin;
{
	/*
	 * Linux 2.4.20-8smp (RedHat 9)
	 * Because ip_input() on linux clears the checksum flag in the sk_buff
	 * before calling the netfilter hook, it is not possible to take
	 * advantage of the work already done by the hardware.
	 */
#ifdef IPFILTER_CKSUM
	if (fr_checkl4sum(fin) == -1)
		fin->fin_flx |= FI_BAD;
#endif
}


u_short fr_nextipid(fin)
fr_info_t *fin;
{
#if 1
	static u_short ipid = 0;

	return ipid++;
#else
	ip_t ip;

	__ip_select_ident(&ip, NULL);
	return ip.ip_id;
#endif
}


/*ARGSUSED*/
int fr_fastroute(min, mp, fin, fdp)
mb_t *min, **mp;
fr_info_t *fin;
frdest_t *fdp;
{
	struct net_device *ifp, *sifp;
	struct in_addr dip;
	struct rtable *rt;
	frentry_t *fr;
	int err, sout;
	ip_t *ip;

	rt = NULL;
	fr = fin->fin_fr;
	ip = MTOD(min, ip_t *);
	dip = ip->ip_dst;

	if (fdp != NULL)
		ifp = fdp->fd_ifp;
	else
		ifp = fin->fin_ifp;

	if ((ifp == NULL) && ((fr == NULL) || !(fr->fr_flags & FR_FASTROUTE))) {
		err = ENETUNREACH;
		goto bad;
	}

	if ((fdp != NULL) && (fdp->fd_ip.s_addr))
		dip = fdp->fd_ip;

	switch (fin->fin_v)
	{
	case 4 :
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
		err = ip_route_output(&rt, dip.s_addr, 0,
				       RT_TOS(ip->ip_tos) | RTO_CONN, 0);
#else
		err = 1;
#endif
		if (err != 0 || rt == NULL)
			goto bad;

		if (rt->u.dst.dev == NULL) {
			err = EHOSTUNREACH;
			goto bad;
		}
		break;
	default :
		err = EINVAL;
		goto bad;
	}

	if (fin->fin_out == 0) {
		sifp = fin->fin_ifp;
		sout = fin->fin_out;
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
			err = EINVAL;
			goto bad;
			break;
		}

		fin->fin_ifp = sifp;
		fin->fin_out = sout;
	}
	ip->ip_sum = 0;


	if (min->dst != NULL) {
		dst_release(min->dst);
		min->dst = NULL;
	}

	min->dst = &rt->u.dst;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,21)
	if (min->len > min->dst->pmtu) {
		err = EMSGSIZE;
		goto bad;
	}
#endif

	switch (fin->fin_v)
	{
	case 4 :
		ip->ip_len = htons(ip->ip_len);
		ip->ip_off = htons(ip->ip_off);
		ip->ip_sum = ip_fast_csum((u_char *)ip, ip->ip_hl);

		/*dumpskbuff(min);*/
		NF_HOOK(PF_INET, NF_IP_LOCAL_OUT, min, NULL,
			ifp, ip_finish_output);
		err = 0;
		break;

	default :
		err = EINVAL;
		break;
	}

	if (err == 0)
		return 0;
bad:
	if (min != NULL)
		kfree_skb(min);
	return err;
}


int fr_ifpaddr(v, atype, ifptr, inp, inpmask)
int v, atype;
void *ifptr;
struct in_addr *inp, *inpmask;
{
	struct sockaddr_in sin, sinmask;
	struct in_ifaddr *ifa;
	struct net_device *dev;
	struct in_device *ifp;

	if ((ifptr == NULL) || (ifptr == (void *)-1))
		return -1;

	dev = ifptr;
	ifp = __in_dev_get(dev);

	if (v == 4)
		inp->s_addr = 0;
#ifdef USE_INET6
	else if (v == 6)
		return -1;
#endif

	ifa = ifp->ifa_list;
	while (ifa != NULL) {
		if (ifa->ifa_flags & IFA_F_SECONDARY)
			continue;
		break;
	}

	if (ifa == NULL)
		return -1;

	sin.sin_family = AF_INET;
	sinmask.sin_addr.s_addr = ifa->ifa_mask;
	if (atype == FRI_BROADCAST)
		sin.sin_addr.s_addr = ifa->ifa_broadcast;
	else if (atype == FRI_PEERADDR)
		sin.sin_addr.s_addr = ifa->ifa_address;
	else
		sin.sin_addr.s_addr = ifa->ifa_local;

	return fr_ifpfillv4addr(atype, (struct sockaddr_in *)&sin,
				(struct sockaddr_in *)&sinmask, inp, inpmask);
}


void m_copydata(m, off, len, cp)
mb_t *m;
int off, len;
caddr_t cp;
{
	bcopy(MTOD(m, char *) + off, cp, len);
}


static u_int ipf_linux_inout(hooknum, skbp, inifp, outifp, okfn)
u_int hooknum;
struct sk_buff **skbp;
const struct net_device *inifp, *outifp;
int (*okfn)(struct sk_buff *);
{
	int result, hlen, dir;
	void *ifp;
	ip_t *ip;
	mb_t *sk;

	if (inifp == NULL && outifp != NULL) {
		dir = IPF_OUT;
		ifp = (void *)outifp;
	} else if (inifp != NULL && outifp == NULL) {
		dir = IPF_IN;
		ifp = (void *)inifp;
	} else
		return NF_DROP;

	sk = *skbp;
	ip = MTOD(sk, ip_t *);
	if (ip->ip_v == 4) {
		hlen = ip->ip_hl << 2;
		ip->ip_len = ntohs(ip->ip_len);
		ip->ip_off = ntohs(ip->ip_off);
#ifdef USE_INET6
	} else if (ip->ip_v == 6) {
		hlen = sizeof(ip6_t);
#endif
	} else
		return NF_DROP;
	result = fr_check(ip, hlen, (struct net_device *)ifp, dir, skbp);

	/*
	 * This is kind of not always right...*skbp == NULL might really be
	 * a drop but Linux expects *skbp != NULL for NF_DROP.
	 */
	if (*skbp == NULL)
		return NF_STOLEN;

	if (result != 0)
		return NF_DROP;

	if (ip->ip_v == 4) {
		ip->ip_len = htons(ip->ip_len);
		ip->ip_off = htons(ip->ip_off);
	}
	return NF_ACCEPT;
}


INLINE void ipf_read_enter(rwlk)
ipfrwlock_t *rwlk;
{
#if defined(IPFDEBUG) && !defined(_KERNEL)
	if (rwlk->ipf_magic != 0x97dd8b3a) {
		printk("ipf_read_enter:rwlk %p ipf_magic 0x%x\n",
			rwlk, rwlk->ipf_magic);
		/*
		 * Force a panic.
		 */
		rwlk->ipf_magic = 0;
		*((int *)rwlk->ipf_magic) = 1;
	}
#endif
	read_lock(&rwlk->ipf_lk);
	ATOMIC_INC32(rwlk->ipf_isr);
}


INLINE void ipf_write_enter(rwlk)
ipfrwlock_t *rwlk;
{
#if defined(IPFDEBUG) && !defined(_KERNEL)
	if (rwlk->ipf_magic != 0x97dd8b3a) {
		printk("ipf_write_enter:rwlk %p ipf_magic 0x%x\n",
			rwlk, rwlk->ipf_magic);
		rwlk->ipf_magic = 0;
		*((int *)rwlk->ipf_magic) = 1;
	}
#endif
	write_lock(&rwlk->ipf_lk);
	rwlk->ipf_isw = 1;
}


INLINE void ipf_rw_exit(rwlk)
ipfrwlock_t *rwlk;
{
#if defined(IPFDEBUG) && !defined(_KERNEL)
	if (rwlk->ipf_magic != 0x97dd8b3a) {
		printk("ipf_rw_exit:rwlk %p ipf_magic 0x%x\n",
			rwlk, rwlk->ipf_magic);
		/*
		 * Force a panic.
		 */
		rwlk->ipf_magic = 0;
		*((int *)rwlk->ipf_magic) = 1;
	}
#endif
	if (rwlk->ipf_isw > 0) {
		rwlk->ipf_isw = 0;
		write_unlock(&rwlk->ipf_lk);
	} else if (rwlk->ipf_isr > 0) {
		ATOMIC_DEC32(rwlk->ipf_isr);
		read_unlock(&rwlk->ipf_lk);
	} else {
		panic("rwlk->ipf_isw %d isr %d rwlk %p name [%s]\n",
		      rwlk->ipf_isw, rwlk->ipf_isr, rwlk, rwlk->ipf_lname);
	}
}


/*
 * This is not a perfect solution for a downgrade because we can lose the lock
 * on the object of desire.
 */
INLINE void ipf_rw_downgrade(rwlk)
ipfrwlock_t *rwlk;
{
	ipf_rw_exit(rwlk);
	ipf_read_enter(rwlk);
}


void ipf_rw_init(rwlck, name)
ipfrwlock_t *rwlck;
char *name;
{
	memset(rwlck, 0, sizeof(*rwlck));
	rwlck->ipf_lname = name;
	rwlock_init(&rwlck->ipf_lk);
}


#if 0
void dumpskbuff(sk)
struct sk_buff *sk;
{
	char line[80], *t;
	u_char *s1, *s2;
	int len, i;
	u_char c;

	while (sk != NULL) {
		len = sk->end - sk->data;

		for (s1 = MTOD(sk, char *); len > 0; ) {
			t = line;
			s2 = s1;
			for (i = 0; i < len && i < 16; i++)  {
				c = *s2++;
				sprintf(t, "%02x", c);
				t += 2;
				if (i &1)
					*t++ = ' ';
			}
			*t++ = ' ';
			*t++ = ' ';
			for (i = 0; i < len && i < 16; i++)  {
				c = *s1++;
				*t++ = (c >= 32 && c < 127) ? c : '.';
			}
			*t = '\0';
			printf("%p@%03d %s\n", sk, s1 - MTOD(sk, u_char *), line);
			if (len > 16) {
				len -= 16;
				s1 += 16;
			} else
				len = 0;
		}
		sk = sk->next;
	}
}
#endif


mb_t *m_pullup(m, len)
mb_t *m;
int len;
{
	if (len <= M_LEN(m))
		return m;
	kfree_skb(m);
	return NULL;
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
		m = m_pullup(m, len);
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


/* ------------------------------------------------------------------------ */
/* Function:    fr_slowtimer                                                */
/* Returns:     Nil                                                         */
/* Parameters:  Nil                                                         */
/*                                                                          */
/* Slowly expire held state for fragments.  Timeouts are set * in           */
/* expectation of this being called twice per second.                       */
/* ------------------------------------------------------------------------ */
void fr_slowtimer(long value)
{
	READ_ENTER(&ipf_global);

	ipf_expiretokens();
	fr_fragexpire();
	fr_timeoutstate();
	fr_natexpire();
	fr_authexpire();
	fr_ticks++;
	if (fr_running <= 0)
		goto done;
	mod_timer(&ipf_timer, HZ/2 + jiffies);

done:
	RWLOCK_EXIT(&ipf_global);
}


int ipf_inject(fin, m)
fr_info_t *fin;
mb_t *m;
{
	FREE_MB_T(m);

	fin->fin_m = NULL;
	fin->fin_ip = NULL;

	return EINVAL;
}
