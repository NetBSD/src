/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
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

#if HAVE_JEMALLOC > 100
#include <malloc.h>

void *__je_mallocx(size_t, int);
void *
mallocx(size_t l, int f)
{
	return __je_mallocx(l, f);
}

void *__je_rallocx(void *, size_t, int);
void *
rallocx(void *p, size_t l, int f)
{
	return __je_rallocx(p, l, f);
}

size_t __je_xallocx(void *, size_t, size_t, int);
size_t 
xallocx(void *p, size_t l, size_t s, int f)
{
	return __je_xallocx(p, l, s, f);
}

size_t __je_sallocx(const void *, int);
size_t
sallocx(const void *p, int f)
{
	return __je_sallocx(p, f);
}

void __je_dallocx(void *, int);
void 
dallocx(void *p, int f)
{
	__je_dallocx(p, f);
}

void __je_sdallocx(void *, size_t, int);
void 
sdallocx(void *p, size_t l, int f)
{
	__je_sdallocx(p, l, f);
}

size_t __je_nallocx(size_t, int);
size_t
nallocx(size_t l, int f)
{
	return __je_nallocx(l, f);
}

int __je_mallctl(const char *, void *, size_t *, void *, size_t);
int
mallctl(const char *n, void *p, size_t *s, void *v, size_t l)
{
	return __je_mallctl(n, p, s, v, l);
}

int __je_mallctlnametomib(const char *, size_t *, size_t *);
int
mallctlnametomib(const char *n, size_t *l, size_t *s)
{
	return __je_mallctlnametomib(n, l, s);
}

int __je_mallctlbymib(const size_t *, size_t, void *, size_t *, void *, size_t);
int
mallctlbymib(const size_t *sp, size_t s, void *p , size_t *dp, void *r ,
    size_t l)
{
	return __je_mallctlbymib(sp, s, p , dp, r, l);
}

void __je_malloc_stats_print(void (*)(void *, const char *), void *,
    const char *);
void
malloc_stats_print(void (*f)(void *, const char *), void *p, const char *n)
{
	__je_malloc_stats_print(f, p, n);
}

size_t __je_malloc_usable_size(const void *);
size_t
malloc_usable_size(const void *p)
{
    return __je_malloc_usable_size(p);
}
void (*__je_malloc_message_get(void))(void *, const char *);
void (*
malloc_message_get(void))(void *, const char *)
{
	return __je_malloc_message_get();
}
	
void __je_malloc_message_set(void (*)(void *, const char *));
void
malloc_message_set(void (*m)(void *, const char *))
{
	__je_malloc_message_set(m);
}

const char *__je_malloc_conf_get(void);
const char *
malloc_conf_get(void)
{
	return __je_malloc_conf_get();
}

void __je_malloc_conf_set(const char *);
void malloc_conf_set(const char *m)
{
	__je_malloc_conf_set(m);
}
#endif /* HAVE_JEMALLOC > 100 */
