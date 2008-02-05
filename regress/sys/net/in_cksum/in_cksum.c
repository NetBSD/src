/*	$NetBSD: in_cksum.c,v 1.3 2008/02/05 10:00:17 yamt Exp $	*/
/*-
 * Copyright (c) 2008 Joerg Sonnenberger <joerg@NetBSD.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: in_cksum.c,v 1.3 2008/02/05 10:00:17 yamt Exp $");

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/resource.h>
#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int	portable_cpu_in_cksum(struct mbuf*, int, int, uint32_t);
int	cpu_in_cksum(struct mbuf*, int, int, uint32_t);

static bool	random_aligned;

static void
free_mbuf_chain(struct mbuf *m)
{
	struct mbuf *next;

	if (m == NULL)
		return;

	next = m->m_next;
	free(m);
	free_mbuf_chain(next);
}

static struct mbuf *
allocate_mbuf_chain(char **lens)
{
	int len, off;
	struct mbuf *m;

	if (*lens == NULL)
		return NULL;

	len = atoi(*lens);
	off = random_aligned ? rand() % 64 : 0;

	m = malloc(sizeof(struct m_hdr) + len + off);
	if (m == NULL)
		err(EXIT_FAILURE, "malloc failed");

	m->m_data = (char *)m + sizeof(struct m_hdr) + off;
	m->m_len = len;

	m->m_next = allocate_mbuf_chain(lens + 1);

	return m;
}

static void
randomise_mbuf_chain(struct mbuf *m)
{
	int i, data, len;

	for (i = 0; i < m->m_len; i += sizeof(int)) {
		data = rand();
		if (i + sizeof(int) < m->m_len)
			len = sizeof(int);
		else
			len = m->m_len - i;
		memcpy(m->m_data + i, &data, len);
	}
	if (m->m_next)
		randomise_mbuf_chain(m->m_next);
}

static int
mbuf_len(struct mbuf *m)
{
	return m == NULL ? 0 : m->m_len + mbuf_len(m->m_next);
}

int	in_cksum_portable(struct mbuf *, int);
int	in_cksum(struct mbuf *, int);

int
main(int argc, char **argv)
{
	struct rusage res;
	struct timeval tv, old_tv;
	int loops, old_sum, new_sum, off, len;
	long i, iterations;
	uint32_t init_sum;
	struct mbuf *m;

	if (argc < 4)
		err(1, "%s loops unalign iterations ...", argv[0]);

	loops = atoi(argv[1]);
	random_aligned = atoi(argv[2]);
	iterations = atol(argv[3]);

	for (; loops; --loops) {
		if ((m = allocate_mbuf_chain(argv + 4)) == NULL)
			continue;
		randomise_mbuf_chain(m);
		init_sum = rand();
		len = mbuf_len(m);

		/* force one loop over all data */
		if (loops == 1)
			off = 0;
		else
			off = len ? rand() % len : 0;

		len -= off;
		old_sum = portable_cpu_in_cksum(m, len, off, init_sum);
		new_sum = cpu_in_cksum(m, len, off, init_sum);
		if (old_sum != new_sum)
			errx(1, "comparision failed: %x %x", old_sum, new_sum);

		if (iterations == 0)
			continue;

		getrusage(RUSAGE_SELF, &res);
		tv = res.ru_utime;
		for (i = iterations; i; --i)
			(void)portable_cpu_in_cksum(m, len, off, init_sum);
		getrusage(RUSAGE_SELF, &res);
		timersub(&res.ru_utime, &tv, &old_tv);
		printf("portable version: %ld.%06ld\n", (long)old_tv.tv_sec, (long)old_tv.tv_usec);

		getrusage(RUSAGE_SELF, &res);
		tv = res.ru_utime;
		for (i = iterations; i; --i)
			(void)cpu_in_cksum(m, len, off, init_sum);
		getrusage(RUSAGE_SELF, &res);
		timersub(&res.ru_utime, &tv, &tv);
		printf("test version:     %ld.%06ld\n", (long)tv.tv_sec, (long)tv.tv_usec);
		printf("relative time:    %3.f%%\n",
		    100 * ((float)tv.tv_sec * 1e6 + tv.tv_usec) /
		    ((float)old_tv.tv_sec * 1e6 + old_tv.tv_usec + 1));

		free_mbuf_chain(m);
	}

	return 0;
}
