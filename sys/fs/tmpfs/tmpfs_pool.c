/*	$NetBSD: tmpfs_pool.c,v 1.6.4.1 2007/03/24 14:55:59 yamt Exp $	*/

/*
 * Copyright (c) 2005, 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julio M. Merino Vidal, developed as part of Google's Summer of Code
 * 2005 program.
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
__KERNEL_RCSID(0, "$NetBSD: tmpfs_pool.c,v 1.6.4.1 2007/03/24 14:55:59 yamt Exp $");

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

/*
 * tmpfs provides a custom pool allocator mostly to exactly keep track of
 * how many memory is used for each file system instance.  These pools are
 * never shared across multiple mount points for the reasons described
 * below:
 *
 * - It is very easy to control how many memory is associated with a
 *   given file system.  tmpfs provides a custom pool allocator that
 *   controls memory usage according to previously specified usage
 *   limits, by simply increasing or decreasing a counter when pages
 *   are allocated or released, respectively.
 *
 *   If the pools were shared, we could easily end up with unaccounted
 *   memory, thus taking incorrect decisions on the amount of memory
 *   use.  As an example to prove this point, consider two mounted
 *   instances of tmpfs, one mounted on A and another one on B. Assume
 *   that each memory page can hold up to four directory entries and
 *   that, for each entry you create on A, you create three on B
 *   afterwards.  After doing this, each memory page will be holding an
 *   entry from A and three for B.  If you sum up all the space taken by
 *   the total amount of allocated entries, rounded up to a page
 *   boundary, that number will match the number of allocated pages, so
 *   everything is fine.
 *
 *   Now suppose we unmount B.  Given that the file system has to
 *   disappear, we have to delete all the directory entries attached to
 *   it.  But the problem is that freeing those entries will not release
 *   any memory page.  Instead, each page will be filled up to a 25%,
 *   and the rest, a 75%, will be lost.  Not lost in a strict term,
 *   because the memory can be reused by new entries, but lost in the
 *   sense that it is not accounted by any file system.  Despite A will
 *   think it is using an amount 'X' of memory, it will be really using
 *   fourth times that number, thus causing mistakes when it comes to
 *   decide if there is more free space for that specific instance of
 *   tmpfs.
 *
 * - The number of page faults and cache misses is reduced given that all
 *   entries of a given file system are stored in less pages.  Note that
 *   this is true because it is common to allocate and/or access many
 *   entries at once on a specific file system.
 *
 *   Following the example given above, listing a directory on file system
 *   A could result, in the worst case scenario, in fourth times more page
 *   faults if we shared the pools.
 */
struct pool_allocator tmpfs_pool_allocator = {
	.pa_alloc = tmpfs_pool_page_alloc,
	.pa_free = tmpfs_pool_page_free,
};

/* --------------------------------------------------------------------- */

/*
 * Initializes the pool pointed to by tpp and associates it to the mount
 * point tmp.  The size of its elements is set to size.  Its wait channel
 * is derived from the string given in what and the mount point given in
 * 'tmp', which should result in a unique string among all existing pools.
 */
void
tmpfs_pool_init(struct tmpfs_pool *tpp, size_t size, const char *what,
    struct tmpfs_mount *tmp)
{
	int cnt;

	cnt = snprintf(tpp->tp_name, sizeof(tpp->tp_name),
	    "%s_pool_%p", what, tmp);
	KASSERT(cnt < sizeof(tpp->tp_name));

	pool_init(&tpp->tp_pool, size, 0, 0, 0, tpp->tp_name,
	    &tmpfs_pool_allocator, IPL_NONE);
	tpp->tp_mount = tmp;
}

/* --------------------------------------------------------------------- */

/*
 * Destroys the pool pointed to by 'tpp'.
 */
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

/*
 * Initialize the string pool pointed to by 'tsp' and attach it to the
 * 'tmp' mount point.
 */
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

/*
 * Destroy the given string pool.
 */
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

/*
 * Allocate a new string with a minimum length of len from the 'tsp'
 * string pool.  The pool can return a bigger string, but the caller must
 * not make any assumptions about the real object size.
 */
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

/*
 * Destroy the str string, which was allocated from the 'tsp' string pool
 * with a length of 'len'.  The length must match the one given during
 * allocation.
 */
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
