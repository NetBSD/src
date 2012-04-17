/*	$NetBSD: ip_pool.h,v 1.1.1.4.2.1 2012/04/17 00:02:24 yamt Exp $	*/

/*
 * Copyright (C) 1993-2001, 2003 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: ip_pool.h,v 2.26.2.6 2007/10/10 09:51:43 darrenr Exp
 */

#ifndef	__IP_POOL_H__
#define	__IP_POOL_H__

#if defined(_KERNEL) && !defined(__osf__) && !defined(__hpux) && \
    !defined(linux) && !defined(sun) && !defined(AIX)
# include <net/radix.h>
extern void rn_freehead(struct radix_node_head *);
# define FreeS(p, z)		KFREES(p, z)
extern int max_keylen;
#else
# if defined(__osf__) || defined(__hpux) || defined(sun)
#  include "radix_ipf_local.h"
#  define radix_mask ipf_radix_mask
#  define radix_node ipf_radix_node
#  define radix_node_head ipf_radix_node_head
# else
#  include "radix_ipf.h"
# endif
#endif
#include "netinet/ip_lookup.h"

#define	IP_POOL_NOMATCH		0
#define	IP_POOL_POSITIVE	1

typedef	struct ip_pool_node {
	struct	radix_node	ipn_nodes[2];
	addrfamily_t		ipn_addr;
	addrfamily_t		ipn_mask;
	int			ipn_info;
	int			ipn_ref;
char			ipn_name[FR_GROUPLEN];
u_long			ipn_hits;
	struct ip_pool_node	*ipn_next, **ipn_pnext;
} ip_pool_node_t;


typedef	struct ip_pool_s {
	struct ip_pool_s	*ipo_next;
	struct ip_pool_s	**ipo_pnext;
	struct radix_node_head	*ipo_head;
	ip_pool_node_t	*ipo_list;
	u_long		ipo_hits;
	int		ipo_unit;
	int		ipo_flags;
	int		ipo_ref;
	char		ipo_name[FR_GROUPLEN];
} ip_pool_t;

#define	IPOOL_DELETE	0x01
#define	IPOOL_ANON	0x02


typedef	struct	ip_pool_stat	{
	u_long		ipls_pools;
	u_long		ipls_tables;
	u_long		ipls_nodes;
	ip_pool_t	*ipls_list[IPL_LOGSIZE];
} ip_pool_stat_t;


extern	ip_pool_stat_t	ipoolstat;
extern	ip_pool_t	*ip_pool_list[IPL_LOGSIZE];

extern	int	ip_pool_search(void *, int, void *);
extern	int	ip_pool_init(void);
extern	void	ip_pool_fini(void);
extern	int	ip_pool_create(iplookupop_t *);
extern	int	ip_pool_insert(ip_pool_t *, i6addr_t *, i6addr_t *, int);
extern	int	ip_pool_remove(ip_pool_t *, ip_pool_node_t *);
extern	int	ip_pool_destroy(int, char *);
extern	void	ip_pool_free(ip_pool_t *);
extern	void	ip_pool_deref(ip_pool_t *);
extern	void	ip_pool_node_deref(ip_pool_node_t *);
extern	void	*ip_pool_find(int, char *);
extern	ip_pool_node_t *ip_pool_findeq(ip_pool_t *,
					  addrfamily_t *, addrfamily_t *);
extern	int	ip_pool_flush(iplookupflush_t *);
extern	int	ip_pool_statistics(iplookupop_t *);
extern	int	ip_pool_getnext(ipftoken_t *, ipflookupiter_t *);
extern	void	ip_pool_iterderef(u_int, int, void *);

#endif /* __IP_POOL_H__ */
