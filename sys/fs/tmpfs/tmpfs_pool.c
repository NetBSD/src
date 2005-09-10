/*	$NetBSD: tmpfs_pool.c,v 1.1 2005/09/10 19:20:51 jmmv Exp $	*/

/*
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julio M. Merino Vidal.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * Pool allocator and convenience routines for tmpfs.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tmpfs_pool.c,v 1.1 2005/09/10 19:20:51 jmmv Exp $");

#include <sys/param.h>
#include <sys/pool.h>

#include <uvm/uvm.h>

#include <fs/tmpfs/tmpfs.h>

/* --------------------------------------------------------------------- */

void *	tmpfs_pool_page_alloc(struct pool *, int);
void	tmpfs_pool_page_free(struct pool *, void *);

/* XXX: Will go away when our pool allocator does what it has to do by
 * itself. */
extern void*	pool_page_alloc_nointr(struct pool *, int);
extern void	pool_page_free_nointr(struct pool *, void *);

/* --------------------------------------------------------------------- */

struct pool_allocator tmpfs_pool_allocator = {
	tmpfs_pool_page_alloc,
	tmpfs_pool_page_free,
	0,
};

/* --------------------------------------------------------------------- */

void
tmpfs_pool_init(struct tmpfs_pool *tpp, size_t size, const char *what,
    struct tmpfs_mount *tmp)
{
	char wchan[64];
	int cnt;

	cnt = snprintf(wchan, sizeof(wchan), "%s_pool_%p", what, tmp);
	KASSERT(cnt < sizeof(wchan));

	pool_init((struct pool *)tpp, size, 0, 0, 0, wchan,
	    &tmpfs_pool_allocator);
	tpp->tp_mount = tmp;
}

/* --------------------------------------------------------------------- */

void
tmpfs_pool_destroy(struct tmpfs_pool *tpp)
{

	pool_destroy((struct pool *)tpp);
}

/* --------------------------------------------------------------------- */

void *
tmpfs_pool_page_alloc(struct pool *pp, int flags)
{
	void *page;
	struct tmpfs_pool *tpp;
	struct tmpfs_mount *tmp;

	tpp = (struct tmpfs_pool *)pp;
	tmp = tpp->tp_mount;

	if (TMPFS_PAGES_MAX(tmp) - tmp->tm_pages_used == 0)
		return NULL;

	tmp->tm_pages_used += 1;
	page = pool_page_alloc_nointr(pp, flags);
	KASSERT(page != NULL);

	return page;
}

/* --------------------------------------------------------------------- */

void
tmpfs_pool_page_free(struct pool *pp, void *v)
{
	struct tmpfs_pool *tpp;
	struct tmpfs_mount *tmp;

	tpp = (struct tmpfs_pool *)pp;
	tmp = tpp->tp_mount;

	tmp->tm_pages_used -= 1;
	pool_page_free_nointr(pp, v);
}

/* --------------------------------------------------------------------- */

void
tmpfs_str_pool_init(struct tmpfs_str_pool *tsp, struct tmpfs_mount *tmp)
{

	tmpfs_pool_init(&tsp->tsp_pool_16,   16,   "str", tmp);
	tmpfs_pool_init(&tsp->tsp_pool_32,   32,   "str", tmp);
	tmpfs_pool_init(&tsp->tsp_pool_64,   64,   "str", tmp);
	tmpfs_pool_init(&tsp->tsp_pool_128,  128,  "str", tmp);
	tmpfs_pool_init(&tsp->tsp_pool_256,  256,  "str", tmp);
	tmpfs_pool_init(&tsp->tsp_pool_512,  512,  "str", tmp);
	tmpfs_pool_init(&tsp->tsp_pool_1024, 1024, "str", tmp);
}

/* --------------------------------------------------------------------- */

void
tmpfs_str_pool_destroy(struct tmpfs_str_pool *tsp)
{

	tmpfs_pool_destroy(&tsp->tsp_pool_16);
	tmpfs_pool_destroy(&tsp->tsp_pool_32);
	tmpfs_pool_destroy(&tsp->tsp_pool_64);
	tmpfs_pool_destroy(&tsp->tsp_pool_128);
	tmpfs_pool_destroy(&tsp->tsp_pool_256);
	tmpfs_pool_destroy(&tsp->tsp_pool_512);
	tmpfs_pool_destroy(&tsp->tsp_pool_1024);
}

/* --------------------------------------------------------------------- */

char *
tmpfs_str_pool_get(struct tmpfs_str_pool *tsp, size_t len, int flags)
{
	struct tmpfs_pool *p;

	KASSERT(len <= 1024);

	if      (len <= 16)   p = &tsp->tsp_pool_16;
	else if (len <= 32)   p = &tsp->tsp_pool_32;
	else if (len <= 64)   p = &tsp->tsp_pool_64;
	else if (len <= 128)  p = &tsp->tsp_pool_128;
	else if (len <= 256)  p = &tsp->tsp_pool_256;
	else if (len <= 512)  p = &tsp->tsp_pool_512;
	else if (len <= 1024) p = &tsp->tsp_pool_1024;
	else {
		KASSERT(0);
		p = NULL; /* Silence compiler warnings */
	}

	return (char *)TMPFS_POOL_GET(p, flags);
}

/* --------------------------------------------------------------------- */

void
tmpfs_str_pool_put(struct tmpfs_str_pool *tsp, char *str, size_t len)
{
	struct tmpfs_pool *p;

	KASSERT(len <= 1024);

	if      (len <= 16)   p = &tsp->tsp_pool_16;
	else if (len <= 32)   p = &tsp->tsp_pool_32;
	else if (len <= 64)   p = &tsp->tsp_pool_64;
	else if (len <= 128)  p = &tsp->tsp_pool_128;
	else if (len <= 256)  p = &tsp->tsp_pool_256;
	else if (len <= 512)  p = &tsp->tsp_pool_512;
	else if (len <= 1024) p = &tsp->tsp_pool_1024;
	else {
		KASSERT(0);
		p = NULL; /* Silence compiler warnings */
	}

	TMPFS_POOL_PUT(p, str);
}
