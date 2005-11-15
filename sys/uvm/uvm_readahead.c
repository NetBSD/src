/*	$NetBSD: uvm_readahead.c,v 1.1.2.3 2005/11/15 11:28:39 yamt Exp $	*/

/*-
 * Copyright (c)2003, 2005 YAMAMOTO Takashi,
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uvm_readahead.c,v 1.1.2.3 2005/11/15 11:28:39 yamt Exp $");

#include <sys/param.h>
#include <sys/pool.h>
#include <sys/fcntl.h>	/* POSIX_FADV_* */

#include <uvm/uvm.h>
#include <uvm/uvm_readahead.h>

struct uvm_ractx {
	int ra_flags;
#define	RA_VALID	1
	int ra_advice;
	off_t ra_winstart;
	size_t ra_winsize;
	off_t ra_next;
};

/*
 * XXX tune
 * XXX should consider the amount of memory in the system
 */

#define	RA_WINSIZE_INIT	MAXPHYS
#define	RA_WINSIZE_MAX	(MAXPHYS * 8)
#define	RA_WINSIZE_SEQENTIAL	RA_WINSIZE_MAX
#define	RA_MINSIZE	(MAXPHYS * 2)

static off_t ra_startio(struct uvm_object *, off_t, size_t);
static struct uvm_ractx *ra_allocctx(void);
static void ra_freectx(struct uvm_ractx *);

POOL_INIT(ractx_pool, sizeof(struct uvm_ractx), 0, 0, 0, "ractx",
    &pool_allocator_nointr);

static struct uvm_ractx *
ra_allocctx(void)
{

	return pool_get(&ractx_pool, PR_NOWAIT);
}

static void
ra_freectx(struct uvm_ractx *ra)
{

	pool_put(&ractx_pool, ra);
}

static off_t
ra_startio(struct uvm_object *uobj, off_t off, size_t sz)
{
	const off_t endoff = off + sz;

#if defined(READAHEAD_DEBUG)
	printf("%s: uobj=%p, off=%" PRIu64 ", endoff=%" PRIu64 "\n",
	    __func__, uobj, off, endoff);
#endif /* defined(READAHEAD_DEBUG) */
	off = trunc_page(off);
	while (off < endoff) {
		const size_t chunksize = MAXPHYS;
		int error;
		size_t donebytes;
		int npages;
		int orignpages;
		size_t bytelen;

		KASSERT((chunksize & (chunksize - 1)) == 0);
		KASSERT((off & PAGE_MASK) == 0);
		bytelen = ((off + chunksize) & -(off_t)chunksize) - off;
#if defined(READAHEAD_DEBUG)
		printf("%s: off=%" PRIu64 ", bytelen=%zu\n",
		    __func__, off, bytelen);
#endif /* defined(READAHEAD_DEBUG) */
		KASSERT((bytelen & PAGE_MASK) == 0);
		npages = orignpages = bytelen >> PAGE_SHIFT;
		KASSERT(npages != 0);
		simple_lock(&uobj->vmobjlock);
		error = (*uobj->pgops->pgo_get)(uobj, off, NULL,
		    &npages, 0, VM_PROT_READ, 0, 0);
		if (error) {
#if defined(READAHEAD_DEBUG)
			if (error != EINVAL) {
				printf("%s: error=%d\n", __func__, error);
			}
#endif /* defined(READAHEAD_DEBUG) */
			break;
		}
		KASSERT(orignpages != npages);
		donebytes = orignpages << PAGE_SHIFT;
		off += donebytes;
	}

	return off;
}

/* ------------------------------------------------------------ */

struct uvm_ractx *
uvm_ra_allocctx(int advice)
{
	struct uvm_ractx *ra;

	KASSERT(advice == POSIX_FADV_NORMAL ||
	    advice == POSIX_FADV_SEQUENTIAL ||
	    advice == POSIX_FADV_RANDOM);

	ra = ra_allocctx();
	if (ra != NULL) {
		ra->ra_flags = 0;
		ra->ra_winstart = 0;
		ra->ra_advice = advice;
	}

	return ra;
}

void
uvm_ra_freectx(struct uvm_ractx *ra)
{

	KASSERT(ra != NULL);
	ra_freectx(ra);
}

void
uvm_ra_request(struct uvm_ractx *ra, struct uvm_object *uobj,
    off_t reqoff, size_t reqsize)
{

	if (ra == NULL) {
		return;
	}

	switch (ra->ra_advice) {
	case POSIX_FADV_NORMAL:
		break;

	case POSIX_FADV_RANDOM:
		return;

	case POSIX_FADV_SEQUENTIAL:
		if (reqoff <= ra->ra_winstart) {
			ra->ra_next = reqoff;
		}
		ra->ra_winsize = RA_WINSIZE_SEQENTIAL;
		goto do_readahead;

	default:
#if defined(DIAGNOSTIC)
		panic("%s: unknown advice %d", __func__, ra->ra_advice);
#endif /* defined(DIAGNOSTIC) */
		break;
	}

	if ((ra->ra_flags & RA_VALID) == 0) {
initialize:
		ra->ra_winstart = ra->ra_next = reqoff + reqsize;
		ra->ra_winsize = RA_WINSIZE_INIT;
		ra->ra_flags |= RA_VALID;
		return;
	}

	if (reqoff < ra->ra_winstart ||
	    ra->ra_winstart + ra->ra_winsize < reqoff) {

		/*
		 * miss
		 */

		goto initialize;
	}

	/*
	 * hit
	 */

do_readahead:
	if (reqoff > ra->ra_next) {
		ra->ra_next = reqoff;
	}

	if (reqoff + ra->ra_winsize > ra->ra_next) {
		off_t raoff = MAX(reqoff, ra->ra_next);
		size_t rasize = reqoff + ra->ra_winsize - ra->ra_next;

		if (rasize >= RA_MINSIZE) {
			ra->ra_next = ra_startio(uobj, raoff, rasize);
		}
	}

	/*
	 * update window
	 */

	ra->ra_winstart = reqoff + reqsize;
	ra->ra_winsize = MIN(RA_WINSIZE_MAX, ra->ra_winsize + reqsize);
}
