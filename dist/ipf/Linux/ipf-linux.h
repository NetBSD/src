/*	$NetBSD: ipf-linux.h,v 1.1.1.1 2004/03/28 08:56:03 martti Exp $	*/

#ifndef __IPF_LINUX_H__
#define __IPF_LINUX_H__

#include <linux/config.h>
#ifndef CONFIG_NETFILTER
# define CONFIG_NETFILTER
#endif
#include <linux/compatmac.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/socket.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <linux/skbuff.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/icmp.h>
#include <linux/rtnetlink.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_ipv6.h>

struct	tcphdr	{
	__u16	th_sport;
	__u16	th_dport;
	__u32	th_seq;
	__u32	th_ack;
# if defined(__i386__) || defined(__MIPSEL__) || defined(__alpha__) ||\
	defined(__vax__)
	__u8	th_res:4;
	__u8	th_off:4;
#else
	__u8	th_off:4;
	__u8	th_res:4;
#endif
	__u8	th_flags;
	__u16	th_win;
	__u16	th_sum;
	__u16	th_urp;
};

typedef	__u32	tcp_seq;

struct	udphdr	{
	__u16	uh_sport;
	__u16	uh_dport;
	__u16	uh_ulen;
	__u16	uh_sum;
};

struct	ip	{
# if defined(__i386__) || defined(__MIPSEL__) || defined(__alpha__) ||\
	defined(__vax__)
	__u8	ip_hl:4;
	__u8	ip_v:4;
# else
	__u8	ip_v:4;
	__u8	ip_hl:4;
# endif
	__u8	ip_tos;
	__u16	ip_len;
	__u16	ip_id;
	__u16	ip_off;
	__u8	ip_ttl;
	__u8	ip_p;
	__u16	ip_sum;
	struct	in_addr ip_src;
	struct	in_addr ip_dst;
};

/*
 * Structure of an icmp header.
 */
struct icmp {
	__u8	icmp_type;		/* type of message, see below */
	__u8	icmp_code;		/* type sub code */
	__u16	icmp_cksum;		/* ones complement cksum of struct */
	union {
		__u8	ih_pptr;		/* ICMP_PARAMPROB */
		struct	in_addr ih_gwaddr;	/* ICMP_REDIRECT */
		struct	ih_idseq {
			__u16	icd_id;
			__u16	icd_seq;
		} ih_idseq;
		int ih_void;
	} icmp_hun;
# define	icmp_pptr	icmp_hun.ih_pptr
# define	icmp_gwaddr	icmp_hun.ih_gwaddr
# define	icmp_id		icmp_hun.ih_idseq.icd_id
# define	icmp_seq	icmp_hun.ih_idseq.icd_seq
# define	icmp_void	icmp_hun.ih_void
	union {
		struct id_ts {
			__u32	its_otime;
			__u32	its_rtime;
			__u32	its_ttime;
		} id_ts;
		struct id_ip	{
			struct	ip	idi_ip;
			/* options and then 64 bits of data */
		} id_ip;
		u_long	id_mask;
		char	id_data[1];
	} icmp_dun;
# define	icmp_otime	icmp_dun.id_ts.its_otime
# define	icmp_rtime	icmp_dun.id_ts.its_rtime
# define	icmp_ttime	icmp_dun.id_ts.its_ttime
# define	icmp_ip	 icmp_dun.id_ip.idi_ip
# define	icmp_mask	icmp_dun.id_mask
# define	icmp_data	icmp_dun.id_data
};

# ifndef LINUX_IPOVLY
#	define LINUX_IPOVLY
struct ipovly {
	caddr_t ih_next, ih_prev;	/* for protocol sequence q's */
	u_char	ih_x1;			/* (unused) */
	u_char	ih_pr;			/* protocol */
	short	ih_len;		 /* protocol length */
	struct	in_addr ih_src;	 /* source internet address */
	struct	in_addr ih_dst;	 /* destination internet address */
};
# endif

struct	ether_header	{
	__u8	ether_dhost[6];
	__u8	ether_shost[6];
	__u16	ether_type;
};

#include "ip_compat.h"
#include "ip_fil.h"
#include "ip_auth.h"
#include "ip_state.h"
#include "ip_nat.h"
#include "ip_proxy.h"
#include "ip_frag.h"
#ifdef	IPFILTER_LOOKUP
# include "ip_lookup.h"
# include "ip_pool.h"
# include "ip_htable.h"
#endif
#ifdef  IPFILTER_SYNC
# include "netinet/ip_sync.h"
#endif
#ifdef  IPFILTER_SCAN
# include "netinet/ip_scan.h"
#endif
#ifdef IPFILTER_COMPILED
# include "netinet/ip_rules.h"
#endif
#include "ipl.h"

#endif /* __IPF_LINUX_H__ */
