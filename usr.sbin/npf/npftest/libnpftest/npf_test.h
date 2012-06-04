/*	$NetBSD: npf_test.h,v 1.3 2012/06/04 00:28:34 rmind Exp $	*/

/*
 * Public Domain.
 */

#ifndef _LIB_NPF_TEST_H_
#define _LIB_NPF_TEST_H_

#include <sys/types.h>
#include <sys/mbuf.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet6/in6.h>

#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/ethertypes.h>

int		npf_test_load(const void *);
int		npf_test_handlepkt(const void *, size_t, unsigned,
		    bool, int64_t *);

struct mbuf *	mbuf_getwithdata(const void *, size_t);
struct mbuf *	mbuf_construct_ether(int);
struct mbuf *	mbuf_construct(int);
void *		mbuf_return_hdrs(struct mbuf *, bool, struct ip **);
void		mbuf_icmp_append(struct mbuf *, struct mbuf *);

bool		npf_nbuf_test(bool);
bool		npf_processor_test(bool);
bool		npf_table_test(bool);
bool		npf_state_test(bool);

#endif
