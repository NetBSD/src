/*	$NetBSD: npf_nbuf_test.c,v 1.1.4.4 2013/02/08 19:18:11 riz Exp $	*/

/*
 * NPF nbuf interface test.
 *
 * Public Domain.
 */

#include <sys/types.h>
#include <sys/kmem.h>

#include "npf_impl.h"
#include "npf_test.h"

#define	MBUF_CHAIN_LEN		128

CTASSERT((MBUF_CHAIN_LEN % sizeof(uint32_t)) == 0);

static void
mbuf_consistency_check(nbuf_t *nbuf)
{
	struct mbuf *m = nbuf_head_mbuf(nbuf);

	while (m) {
		assert(m->m_type != MT_FREE);
		m = m->m_next;
	}
}

static char *
parse_nbuf_chain(struct mbuf *m)
{
	const void *dummy_ifp = (void *)0xdeadbeef;
	char *s = kmem_zalloc(MBUF_CHAIN_LEN + 1, KM_SLEEP);
	nbuf_t nbuf;
	void *nptr;
	int n;

	nbuf_init(&nbuf, m, dummy_ifp);

	nptr = nbuf_advance(&nbuf, (random() % 16) + 1, (random() % 16) + 1);
	mbuf_consistency_check(&nbuf);
	assert(nptr != NULL);
	nbuf_reset(&nbuf);

	for (n = 0; ; ) {
		char d[4 + 1];

		nptr = nbuf_ensure_contig(&nbuf, sizeof(uint32_t));
		if (nptr == NULL) {
			break;
		}
		mbuf_consistency_check(&nbuf);
		memcpy(&d, nptr, sizeof(uint32_t));

		d[sizeof(d) - 1] = '\0';
		strcat(s, d);

		if (n + sizeof(uint32_t) == MBUF_CHAIN_LEN) {
			assert(nbuf_advance(&nbuf, sizeof(uint32_t) - 1, 0));
			assert(!nbuf_advance(&nbuf, 1, 0));
			break;
		}
		if (!nbuf_advance(&nbuf, sizeof(uint32_t), 0)) {
			break;
		}
		n += sizeof(uint32_t);
	}
	mbuf_consistency_check(&nbuf);
	return s;
}

static char *
mbuf_getstring(struct mbuf *m)
{
	char *s = kmem_zalloc(MBUF_CHAIN_LEN + 1, KM_SLEEP);
	u_int tlen = 0;

	while (m) {
		int len = m->m_len;
		char *d = m->m_data;
		while (len--) {
			s[tlen++] = *d++;
		}
		m = m->m_next;
	}
	return s;
}

static struct mbuf *
mbuf_alloc_with_off(size_t off, int len)
{
	struct mbuf *m;

	KASSERT(off + len < MLEN);
	m = m_get(M_WAITOK, MT_DATA);
	m->m_data = (char *)m->m_data + off;
	m->m_len = len;
	return m;
}

/*
 * Create an mbuf chain, each of 1 byte size.
 */
static struct mbuf *
mbuf_bytesize(size_t clen)
{
	struct mbuf *m0 = NULL, *m = NULL;
	u_int i, n;

	/* Chain of clen (e.g. 128) mbufs, each storing 1 byte of data. */
	for (i = 0, n = 0; i < clen; i++) {
		/* Range of offset: 0 .. 15. */
		m0 = mbuf_alloc_with_off(n & 0xf, 1);

		/* Fill data with letters from 'a' to 'z'. */
		memset(m0->m_data, 'a' + n, 1);
		n = ('a' + n) < 'z' ? n + 1 : 0;

		/* Next mbuf.. */
		m0->m_next = m;
		m = m0;
	}

	m0 = m_gethdr(M_WAITOK, MT_HEADER);
	m0->m_pkthdr.len = clen;
	m0->m_len = 0;
	m0->m_next = m;
	return m0;
}

/*
 * Generate random amount of mbufs, with random offsets and lengths.
 */
static struct mbuf *
mbuf_random_len(size_t chain_len)
{
	struct mbuf *m0 = NULL, *m = NULL;
	u_int tlen = 0, n = 0;

	while (tlen < chain_len) {
		u_int off, len;
		char *d;

		/* Random offset and length range: 1 .. 16. */
		off = (random() % 16) + 1;
		len = (random() % 16) + 1;

		/* Do not exceed 128 bytes of total length. */
		if (tlen + len > chain_len) {
			len = chain_len - tlen;
		}
		tlen += len;

		/* Consistently fill data with letters from 'a' to 'z'. */
		m0 = mbuf_alloc_with_off(off, len);
		d = m0->m_data;
		while (len--) {
			*d++ = ('a' + n);
			n = ('a' + n) < 'z' ? n + 1 : 0;
		}

		/* Next mbuf.. */
		m0->m_next = m;
		m = m0;
	}
	KASSERT(tlen == chain_len);

	m0 = m_gethdr(M_WAITOK, MT_HEADER);
	m0->m_pkthdr.len = tlen;
	m0->m_next = m;
	m0->m_len = 0;
	return m0;
}

static bool
validate_mbuf_data(bool verbose, char *bufa, char *bufb)
{
	bool ret = (strcmp(bufa, bufb) == 0);

	if (verbose) {
		printf("Buffer A: %s\nBuffer B: %s\n", bufa, bufb);
	}
	kmem_free(bufa, MBUF_CHAIN_LEN + 1);
	kmem_free(bufb, MBUF_CHAIN_LEN + 1);
	return ret;
}

bool
npf_nbuf_test(bool verbose)
{
	struct mbuf *m1, *m2;
	char *bufa, *bufb;
	bool fail = false;

	m1 = mbuf_random_len(MBUF_CHAIN_LEN);
	bufa = mbuf_getstring(m1);
	bufb = parse_nbuf_chain(m1);
	fail |= !validate_mbuf_data(verbose, bufa, bufb);

	m2 = mbuf_bytesize(MBUF_CHAIN_LEN);
	bufa = mbuf_getstring(m2);
	bufb = parse_nbuf_chain(m2);
	fail |= !validate_mbuf_data(verbose, bufa, bufb);

	return !fail;
}
