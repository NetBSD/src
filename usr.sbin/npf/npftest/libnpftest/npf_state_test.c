/*	$NetBSD: npf_state_test.c,v 1.3.4.3 2013/01/23 00:06:44 yamt Exp $	*/

/*
 * NPF state tracking test.
 *
 * Public Domain.
 */

#include <sys/types.h>
#include <sys/kmem.h>

#include "npf_impl.h"
#include "npf_test.h"

typedef struct {
	int		tcpfl;		/* TCP flags. */
	int		tlen;		/* TCP data length. */
	uint32_t	seq;		/* SEQ number. */
	uint32_t	ack;		/* ACK number. */
	uint32_t	win;		/* TCP Window. */
	int		flags;		/* Direction et al. */
} tcp_meta_t;

#define	S	TH_SYN
#define	A	TH_ACK
#define	F	TH_FIN
#define	OUT	0x1
#define	IN	0x2
#define	ERR	0x4
#define	CLEAR	.flags = 0

static const tcp_meta_t packet_sequence[] = {
	/*
	 *	TCP data	SEQ	ACK		WIN
	 */

	/* Out of order ACK. */
	{ S,	0,		9999,	0,		4096,	OUT	},
	{ S|A,	0,		9,	10000,		2048,	IN	},
	{ A,	0,		10000,	10,		4096,	OUT	},
	/* --- */
	{ A,	0,		10,	10000,		2048,	IN	},
	{ A,	1000,		10000,	10,		4096,	OUT	},
	{ A,	1000,		11000,	10,		4096,	OUT	},
	{ A,	0,		10,	12000,		2048,	IN	},
	{ A,	0,		10,	13000,		2048,	IN	},
	{ A,	1000,		12000,	10,		4096,	OUT	},
	{ A,	0,		10,	11000,		1048,	IN	},
	/* --- */
	{ A,	1000,		14000,	10,		4096,	OUT	},
	{ A,	0,		10,	13000,		2048,	IN	},
	{ CLEAR },

	/* Retransmission after out of order ACK and missing ACK. */
	{ S,	0,		9999,	0,		1000,	OUT	},
	{ S|A,	0,		9,	10000,		4000,	IN	},
	{ A,	0,		10000,	10,		1000,	OUT	},
	/* --- */
	{ A,	1000,		10000,	10,		1000,	OUT	},
	{ A,	0,		10,	11000,		4000,	IN	},
	{ A,	1000,		11000,	10,		1000,	OUT	},
	{ A,	1000,		12000,	10,		1000,	OUT	},
	{ A,	1000,		13000,	10,		1000,	OUT	},
	{ A,	1000,		14000,	10,		1000,	OUT	},
	/* --- Assume the first was delayed; second was lost after us. */
	{ A,	0,		10,	15000,		4000,	IN	},
	{ A,	0,		10,	15000,		2000,	IN	},
	/* --- */
	{ A,	1000,		12000,	10,		1000,	OUT	},
	{ CLEAR },

	/* FIN exchange with retransmit. */
	{ S,	0,		999,	0,		1000,	OUT	},
	{ S|A,	0,		9,	1000,		2000,	IN	},
	{ A,	0,		1000,	10,		1000,	OUT	},
	/* --- */
	{ F,	0,		10,	1000,		2000,	IN	},
	{ F,	0,		1000,	10,		1000,	OUT	},
	{ A,	0,		1000,	11,		1000,	OUT	},
	/* --- */
	{ F,	0,		1000,	11,		1000,	OUT	},
	{ F,	0,		1000,	11,		1000,	OUT	},
	{ A,	0,		11,	1001,		2000,	OUT	},
	{ CLEAR },

	/* Out of window. */
	{ S,	0,		9,	0,		8760,	OUT	},
	{ S|A,	0,		9999,	10,		1000,	IN	},
	{ A,	0,		10,	10000,		8760,	OUT	},
	/* --- */
	{ A,	1460,		10000,	10,		1000,	IN	},
	{ A,	1460,		11460,	10,		1000,	IN	},
	{ A,	0,		10,	12920,		8760,	OUT	},
	{ A,	1460,		12920,	10,		1000,	IN	},
	{ A,	0,		10,	14380,		8760,	OUT	},
	{ A,	1460,		17300,	10,		1000,	IN	},
	{ A,	0,		10,	14380,		8760,	OUT	},
	{ A,	1460,		18760,	10,		1000,	IN	},
	{ A,	0,		10,	14380,		8760,	OUT	},
	{ A,	1460,		20220,	10,		1000,	IN	},
	{ A,	0,		10,	14380,		8760,	OUT	},
	{ A,	1460,		21680,	10,		1000,	IN	},
	{ A,	0,		10,	14380,		8760,	OUT	},
	/* --- */
	{ A,	1460,		14380,	10,		1000,	IN	},
	{ A,	1460,		23140,	10,		1000,	IN|ERR	},
	{ CLEAR },

};

#undef S
#undef A
#undef F

static struct mbuf *
construct_packet(const tcp_meta_t *p)
{
	struct mbuf *m = mbuf_construct(IPPROTO_TCP);
	struct ip *ip;
	struct tcphdr *th;

	th = mbuf_return_hdrs(m, false, &ip);

	/* Imitate TCP payload, set TCP sequence numbers, flags and window. */
	ip->ip_len = htons(sizeof(struct ip) + sizeof(struct tcphdr) + p->tlen);
	th->th_seq = htonl(p->seq);
	th->th_ack = htonl(p->ack);
	th->th_flags = p->tcpfl;
	th->th_win = htons(p->win);
	return m;
}

static bool
process_packet(const int i, npf_state_t *nst, bool *snew)
{
	const tcp_meta_t *p = &packet_sequence[i];
	npf_cache_t npc = { .npc_info = 0 };
	nbuf_t nbuf;
	int ret;

	if (p->flags == 0) {
		npf_state_destroy(nst);
		*snew = true;
		return true;
	}

	const void *dummy_ifp = (void *)0xdeadbeef;
	nbuf_init(&nbuf, construct_packet(p), dummy_ifp);
	ret = npf_cache_all(&npc, &nbuf);
	KASSERT((ret & NPC_IPFRAG) == 0);

	if (*snew) {
		ret = npf_state_init(&npc, &nbuf, nst);
		KASSERT(ret == true);
		*snew = false;
	}
	ret = npf_state_inspect(&npc, &nbuf, nst, p->flags == OUT);
	m_freem(nbuf.nb_mbuf);

	return ret ? true : (p->flags & ERR) != 0;
}

bool
npf_state_test(bool verbose)
{
	npf_state_t nst;
	bool snew = true;
	bool ok = true;

	for (u_int i = 0; i < __arraycount(packet_sequence); i++) {
		if (process_packet(i, &nst, &snew)) {
			continue;
		}
		if (verbose) {
			printf("Failed on packet %d, state dump:\n", i);
			npf_state_dump(&nst);
		}
		ok = false;
	}
	return ok;
}
