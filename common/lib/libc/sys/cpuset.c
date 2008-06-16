/*	$NetBSD: cpuset.c,v 1.7 2008/06/16 13:02:08 christos Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _STANDALONE
#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: cpuset.c,v 1.7 2008/06/16 13:02:08 christos Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/param.h>
#include <sys/sched.h>
#ifdef _KERNEL
#include <sys/kmem.h>
#include <lib/libkern/libkern.h>
#include <sys/atomic.h>
#else
#include <string.h>
#include <stdlib.h>
#include <sys/sysctl.h>
#endif

#define	CPUSET_SHIFT	5
#define	CPUSET_MASK	31
#define CPUSET_SIZE(nc)	((nc) > 32 ? ((nc) >> CPUSET_SHIFT) : 1)

struct _cpuset {
	size_t		size;
	unsigned int	nused;
	struct _cpuset *next;
	uint32_t	bits[0];
};

size_t
/*ARGSUSED*/
_cpuset_size(const cpuset_t *c)
{
	return sizeof(struct {
	    size_t size;
	    unsigned int nused;
	    struct _cpuset *next;
	    uint32_t bits[c->size];
	});
}

void
_cpuset_zero(cpuset_t *c)
{
#ifdef _KERNEL
	KASSERT(c->nused == 1);
#endif
	(void)memset(c->bits, 0, c->size * sizeof(c->bits[0]));
}

int
_cpuset_isset(const cpuset_t *c, cpuid_t i)
{
	const int j = i >> CPUSET_SHIFT;

	if (j >= c->size || j < 0)
		return -1;
	return ((1 << (i & CPUSET_MASK)) & c->bits[j]) != 0;
}

int
_cpuset_set(cpuset_t *c, cpuid_t i)
{
	const int j = i >> CPUSET_SHIFT;

	if (j >= c->size || j < 0)
		return -1;
	c->bits[j] |= 1 << (i & CPUSET_MASK);
	return 0;
}

int
_cpuset_clr(cpuset_t *c, cpuid_t i)
{
	const int j = i >> CPUSET_SHIFT;

	if (j >= c->size || j < 0)
		return -1;
	c->bits[j] &= ~(1 << (i & CPUSET_MASK));
	return 0;
}

cpuset_t *
_cpuset_create(void)
{
	cpuset_t s, *c;
#ifdef _KERNEL
	s.size = CPUSET_SIZE(MAXCPUS);
	c = kmem_zalloc(_cpuset_size(&s), KM_SLEEP);
#else
	static int mib[2] = { CTL_HW, HW_NCPU };
	size_t len;
	int nc;
	if (sysctl(mib, __arraycount(mib), &nc, &len, NULL, 0) == -1)
		return NULL;
	s.size = CPUSET_SIZE((unsigned int)nc);
	c = calloc(1, _cpuset_size(&s));
#endif
	if (c != NULL) {
		c->next = NULL;
		c->nused = 1;
		c->size = s.size;
	}
	return c;
}

void
_cpuset_destroy(cpuset_t *c)
{
#ifdef _KERNEL
	while (c) {
		KASSERT(c->nused == 0);
		kmem_free(c, _cpuset_size(c));
		c = c->next;
	}
#else
	free(c);
#endif
}

#ifdef _KERNEL
size_t
kcpuset_nused(const cpuset_t *c)
{
	return c->nused;
}

void
kcpuset_copy(cpuset_t *d, const cpuset_t *s)
{

	KASSERT(d->size == s->size);
	KASSERT(d->nused == 1);
	(void)memcpy(d->bits, s->bits, d->size * sizeof(d->bits[0]));
}

void
kcpuset_use(cpuset_t *c)
{

	atomic_inc_uint(&c->nused);
}

void
kcpuset_unuse(cpuset_t *c, cpuset_t **lst)
{

	if (atomic_dec_uint_nv(&c->nused) != 0)
		return;
	KASSERT(c->nused > 0);
	KASSERT(c->next == NULL);
	if (lst == NULL) {
		_cpuset_destroy(c);
		return;
	}
	c->next = *lst;
	*lst = c;
}
#endif
#endif
