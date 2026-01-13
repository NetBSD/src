/*	$NetBSD: xmalloc.c,v 1.25 2026/01/13 05:26:55 skrll Exp $	*/

/*
 * Copyright 1996 John D. Polstra.
 * Copyright 1996 Matt Thomas <matt@3am-software.com>
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by John Polstra.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1983 Regents of the University of California.
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
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
/*static char *sccsid = "from: @(#)malloc.c	5.11 (Berkeley) 2/23/91";*/
#endif /* LIBC_SCCS and not lint */

/*
 * malloc.c (Caltech) 2/21/82
 * Chris Kingsley, kingsley@cit-20.
 *
 * This is a very fast storage allocator.  It allocates blocks of a small
 * number of different sizes, and keeps free lists of each size.  Blocks that
 * don't exactly fit are passed up to the next larger size.  In this
 * implementation, the available sizes are 2^n-4 (or 2^n-10) bytes long.
 * This is designed for use in a virtual memory environment.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: xmalloc.c,v 1.25 2026/01/13 05:26:55 skrll Exp $");
#endif /* not lint */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "rtld.h"

/*
 * Pre-allocate mmap'ed pages
 */
#define	NPOOLPAGES	(32 * 1024 / pagesz)
static char 		*pagepool_start, *pagepool_end;
#define PAGEPOOL_SIZE	(size_t)(pagepool_end - pagepool_start)

/*
 * The overhead on a block is at least 4 bytes.  When free, this space
 * contains a pointer to the next free block, and the bottom two bits must
 * be zero.  When in use, the first byte is set to MAGIC, and the second
 * byte is the size index.  The remaining bytes are for alignment.
 */
union	overhead {
	union	overhead *ov_next;	/* when free */
	struct {
		uint16_t	ovu_index;	/* bucket # */
		uint8_t		ovu_magic;	/* magic number */
	} ovu;
#define	ov_magic	ovu.ovu_magic
#define	ov_index	ovu.ovu_index
};

static void morecore(size_t);
static int morepages(int);
static void *imalloc(size_t);

#define	MAGIC		0xef		/* magic # on accounting info */
#define	AMAGIC		0xdf		/* magic # for aligned alloc */


/*
 * nextf[i] is the pointer to the next free block of size
 * (FIRST_BUCKET_SIZE << i).  The overhead information precedes the data
 * area returned to the user.
 */
#define	FIRST_BUCKET_SHIFT	3
#define	FIRST_BUCKET_SIZE	(1U << FIRST_BUCKET_SHIFT)
#define	NBUCKETS 30
static	union overhead *nextf[NBUCKETS];

static	size_t pagesz;			/* page size */
static	size_t pageshift;		/* page size shift */

#ifdef MSTATS
/*
 * nmalloc[i] is the difference between the number of mallocs and frees
 * for a given block size.
 */
static	u_int nmalloc[NBUCKETS];
#endif

#if defined(MALLOC_DEBUG)
#define	ASSERT(p)   if (!(p)) botch("p")
static void
botch(const char *s)
{
	xwarnx("\r\nassertion botched: %s\r\n", s);
	abort();
}
#else
#define	ASSERT(p)
#endif

#define TRACE()	xprintf("TRACE %s:%d\n", __FILE__, __LINE__)

static void *
cp2op(void *cp)
{
	return (((caddr_t)cp - sizeof(union overhead)));
}

static void *
imalloc(size_t nbytes)
{
	union overhead *op;
	size_t bucket;
	size_t amt;

	/*
	 * First time malloc is called, setup page size.
	 */
	if (pagesz == 0) {
		size_t n, m;
		pagesz = n = _rtld_pagesz;
		pageshift = ffs(pagesz) - 1;
		if (morepages(NPOOLPAGES) == 0)
			return NULL;
		op = (union overhead *)(pagepool_start);
		m = sizeof (*op) - (((char *)op - (char *)NULL) & (n - 1));
		if (n < m)
			n += pagesz - m;
		else
			n -= m;
		if (n) {
			pagepool_start += n;
		}
	}

	/*
	 * Convert amount of memory requested into closest block size
	 * stored in hash buckets which satisfies request.
	 * Account for space used per block for accounting.
	 */
	amt = FIRST_BUCKET_SIZE;
	bucket = 0;
	while (nbytes > amt - sizeof(*op)) {
		amt <<= 1;
		bucket++;
		if (amt == 0 || bucket >= NBUCKETS)
			return (NULL);
	}
	/*
	 * If nothing in hash bucket right now,
	 * request more memory from the system.
	 */
	if ((op = nextf[bucket]) == NULL) {
		morecore(bucket);
		if ((op = nextf[bucket]) == NULL)
			return (NULL);
	}
	/* remove from linked list */
	nextf[bucket] = op->ov_next;
	op->ov_magic = MAGIC;
	op->ov_index = bucket;
#ifdef MSTATS
	nmalloc[bucket]++;
#endif
	return ((char *)(op + 1));
}

void *
xmalloc_aligned(size_t size, size_t align, size_t offset)
{
	void *mem;
	union overhead *ov;
	uintptr_t x;

	if (align < FIRST_BUCKET_SIZE)
		align = FIRST_BUCKET_SIZE;
	offset &= align - 1;
	mem = imalloc(size + align + offset + sizeof(union overhead));
	if (mem == NULL)
		return (NULL);
	x = roundup2((uintptr_t)mem + sizeof(union overhead), align);
	x += offset;
	ov = cp2op((void *)x);
	ov->ov_magic = AMAGIC;
	ov->ov_index = x - (uintptr_t)mem + sizeof(union overhead);

	return ((void *)x);
}

/*
 * Allocate more memory to the indicated bucket.
 */
static void
morecore(size_t bucket)
{
	union overhead *op;
	size_t sz;		/* size of desired block */
	size_t amt;		/* amount to allocate */
	size_t nblks;		/* how many blocks we get */

	sz = FIRST_BUCKET_SIZE << bucket;
	if (sz < pagesz) {
		amt = pagesz;
		nblks = amt >> (bucket + FIRST_BUCKET_SHIFT);
	} else {
		amt = sz;
		nblks = 1;
	}
	if (amt > PAGEPOOL_SIZE)
		if (morepages((amt >> pageshift) + NPOOLPAGES) == 0)
			return;
	op = (union overhead *)pagepool_start;
	pagepool_start += amt;

	/*
	 * Add new memory allocated to that on
	 * free list for this hash bucket.
	 */
	nextf[bucket] = op;
	while (--nblks > 0) {
		op->ov_next = (union overhead *)((caddr_t)op + sz);
		op = (union overhead *)((caddr_t)op + sz);
	}
}

void
xfree(void *cp)
{
	union overhead *op, *opx;
	size_t size;

	if (cp == NULL)
		return;
	opx = cp2op(cp);

	op = opx->ov_magic == AMAGIC ? (void *)((caddr_t)cp - opx->ov_index) :
	    opx;
	ASSERT(op->ov_magic == MAGIC);		/* make sure it was in use */
	if (op->ov_magic != MAGIC)
		return;				/* sanity */
	size = op->ov_index;
	ASSERT(size < NBUCKETS);
	op->ov_next = nextf[size];		/* also clobbers ov_magic */
	nextf[size] = op;
#ifdef MSTATS
	nmalloc[size]--;
#endif
}

static void *
irealloc(void *cp, size_t nbytes)
{
	size_t onb;
	size_t i;
	union overhead *op;
	char *res;

	if (cp == NULL)
		return (imalloc(nbytes));
	op = (union overhead *)((caddr_t)cp - sizeof (union overhead));
	if (op->ov_magic != MAGIC) {
		static const char *err_str =
		    "memory corruption or double free in realloc\n";
		extern char *__progname;
		write(STDERR_FILENO, __progname, strlen(__progname));
		write(STDERR_FILENO, err_str, strlen(err_str));
		abort();
	}

	i = op->ov_index;
	onb = 1 << (i + FIRST_BUCKET_SHIFT);
	if (onb < pagesz)
		onb -= sizeof (*op);
	else
		onb += pagesz - sizeof (*op);
	/* avoid the copy if same size block */
	if (i) {
		i = 1 << (i + 2);
		if (i < pagesz)
			i -= sizeof (*op);
		else
			i += pagesz - sizeof (*op);
	}
	if (nbytes <= onb && nbytes > i) {
		return(cp);
	}
	if ((res = imalloc(nbytes)) == NULL)
		return (NULL);
	if (cp != res) {	/* common optimization if "compacting" */
		memcpy(res, cp, (nbytes < onb) ? nbytes : onb);
		xfree(cp);
	}
	return (res);
}

#ifdef MSTATS
/*
 * mstats - print out statistics about malloc
 *
 * Prints two lines of numbers, one showing the length of the free list
 * for each size category, the second showing the number of mallocs -
 * frees for each size category.
 */
void
mstats(char *s)
{
	int i, j;
	union overhead *p;
	int totfree = 0,
	totused = 0;

	xprintf("Memory allocation statistics %s\nfree:\t", s);
	for (i = 0; i < NBUCKETS; i++) {
		for (j = 0, p = nextf[i]; p; p = p->ov_next, j++)
			;
		xprintf(" %d", j);
		totfree += j * (1 << (i + FIRST_BUCKET_SHIFT));
	}
	xprintf("\nused:\t");
	for (i = 0; i < NBUCKETS; i++) {
		xprintf(" %d", nmalloc[i]);
		totused += nmalloc[i] * (1 << (i + FIRST_BUCKET_SHIFT));
	}
	xprintf("\n\tTotal in use: %d, total free: %d\n",
	    totused, totfree);
}
#endif


static int
morepages(int n)
{
	int	fd = -1;
	int	offset;

#ifdef NEED_DEV_ZERO
	fd = open("/dev/zero", O_RDWR, 0);
	if (fd == -1)
		xerr(1, "/dev/zero");
#endif

	if (PAGEPOOL_SIZE > pagesz) {
		caddr_t	addr = (caddr_t)
			(((long)pagepool_start + pagesz - 1) & ~(pagesz - 1));
		if (munmap(addr, pagepool_end - addr) != 0)
			xwarn("morepages: munmap %p", addr);
	}

	offset = (long)pagepool_start - ((long)pagepool_start & ~(pagesz - 1));

	if ((pagepool_start = mmap(0, n * pagesz,
			PROT_READ|PROT_WRITE,
			MAP_ANON|MAP_PRIVATE, fd, 0)) == (caddr_t)-1) {
		xprintf("Cannot map anonymous memory");
		return 0;
	}
	pagepool_end = pagepool_start + n * pagesz;
	pagepool_start += offset;

#ifdef NEED_DEV_ZERO
	close(fd);
#endif
	return n;
}

void *
xcalloc(size_t size)
{

	return memset(xmalloc(size), 0, size);
}

void *
xmalloc(size_t size)
{
	void *p = imalloc(size);

	if (p == NULL)
		xerr(1, "%s", xstrerror(errno));
	return p;
}

void *
xrealloc(void *p, size_t size)
{
	p = irealloc(p, size);

	if (p == NULL)
		xerr(1, "%s", xstrerror(errno));
	return p;
}

char *
xstrdup(const char *str)
{
	size_t len;
	char *copy;

	len = strlen(str) + 1;
	copy = xmalloc(len);
	memcpy(copy, str, len);
	return (copy);
}
