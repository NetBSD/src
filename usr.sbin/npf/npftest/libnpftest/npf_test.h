/*	$NetBSD$	*/

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

/* Test interfaces and IP addresses. */
#define	IFNAME_EXT	"npftest0"
#define	IFNAME_INT	"npftest1"
#define	IFNAME_TEST	"npftest2"

#define	LOCAL_IP1	"10.1.1.1"
#define	LOCAL_IP2	"10.1.1.2"
#define	LOCAL_IP3	"10.1.1.3"

/* Note: RFC 5737 compliant addresses. */
#define	PUB_IP1		"192.0.2.1"
#define	PUB_IP2		"192.0.2.2"
#define	PUB_IP3		"192.0.2.3"

#define	REMOTE_IP1	"192.0.2.101"
#define	REMOTE_IP2	"192.0.2.102"
#define	REMOTE_IP3	"192.0.2.103"

#define	LOCAL_IP6	"fd01:203:405:1::1234"
#define	REMOTE_IP6	"2001:db8:fefe::1010"
#define	EXPECTED_IP6	"2001:db8:1:d550::1234"

void		npf_test_init(int (*)(int, const char *, void *),
		    const char *(*)(int, const void *, char *, socklen_t),
		    long (*)(void));
int		npf_test_load(const void *);
ifnet_t *	npf_test_addif(const char *, bool, bool);
ifnet_t *	npf_test_getif(const char *);

int		npf_test_statetrack(const void *, size_t, ifnet_t *,
		    bool, int64_t *);
void		npf_test_conc(bool, unsigned);

struct mbuf *	mbuf_getwithdata(const void *, size_t);
struct mbuf *	mbuf_construct_ether(int);
struct mbuf *	mbuf_construct(int);
struct mbuf *	mbuf_construct6(int);
void *		mbuf_return_hdrs(struct mbuf *, bool, struct ip **);
void *		mbuf_return_hdrs6(struct mbuf *, struct ip6_hdr **);
void		mbuf_icmp_append(struct mbuf *, struct mbuf *);

bool		npf_nbuf_test(bool);
bool		npf_bpf_test(bool);
bool		npf_table_test(bool, void *, size_t);
bool		npf_state_test(bool);

bool		npf_rule_test(bool);
bool		npf_nat_test(bool);

int		npf_inet_pton(int, const char *, void *);
const char *	npf_inet_ntop(int, const void *, char *, socklen_t);

#endif
