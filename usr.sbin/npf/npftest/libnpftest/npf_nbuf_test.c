/*	$NetBSD: npf_nbuf_test.c,v 1.1 2012/04/14 21:57:29 rmind Exp $	*/

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

static char *
parse_nbuf_chain(void *nbuf, void *n_ptr)
{
	char *s = kmem_zalloc(MBUF_CHAIN_LEN + 1, KM_SLEEP);
	int n, error;

	for (n = 0; ; ) {
		char d[4 + 1];

		error = nbuf_fetch_datum(nbuf, n_ptr, sizeof(uint32_t), d);
		if (error) {
			return NULL;
		}
		d[sizeof(d) - 1] = '\0';
		strcat(s, d);

		if (n + sizeof(uint32_t) == MBUF_CHAIN_LEN) {
			assert(nbuf_advance(&nbuf, n_ptr,
			    sizeof(uint32_t) - 1));
			break;
		}
		n_ptr = nbuf_advance(&nbuf, n_ptr, sizeof(uint32_t));
		if (n_ptr == NULL) {
			return NULL;
		}
		n += sizeof(uint32_t);
	}
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

	m = kmem_zalloc(sizeof(struct mbuf) + off + len, KM_SLEEP);
	m->m_data = (char *)m + sizeof(struct mbuf) + off;
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
		n = ('a' + n) != 'z' ? n + 1 : 0;

		/* Next mbuf.. */
		m0->m_next = m;
		m = m0;
	}
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
			n = ('a' + n) != 'z' ? n + 1 : 0;
		}

		/* Next mbuf.. */
		m0->m_next = m;
		m = m0;
	}
	assert(tlen == chain_len);
	return m0;
}

static bool
validate_mbuf_data(struct mbuf *m, bool verbose, char *bufa, char *bufb)
{
	bool ret = (strcmp(bufa, bufb) == 0);

	if (verbose) {
		printf("Buffer A: %s\nBuffer B: %s\n", bufa, bufb);
	}
	/* XXX free m */
	kmem_free(bufa, MBUF_CHAIN_LEN + 1);
	kmem_free(bufb, MBUF_CHAIN_LEN + 1);
	return ret;
}

bool
npf_nbuf_test(bool verbose)
{
	struct mbuf *m1, *m2;
	char *bufa, *bufb;

	m1 = mbuf_random_len(MBUF_CHAIN_LEN);
	bufa = mbuf_getstring(m1);
	bufb = parse_nbuf_chain(m1, m1->m_data);
	if (!validate_mbuf_data(m1, verbose, bufa, bufb)) {
		return false;
	}

	m2 = mbuf_bytesize(MBUF_CHAIN_LEN);
	bufa = mbuf_getstring(m2);
	bufb = parse_nbuf_chain(m2, m2->m_data);
	if (!validate_mbuf_data(m2, verbose, bufa, bufb)) {
		return false;
	}

	return true;
}
