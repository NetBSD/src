/*	$NetBSD: kern_malloc.c,v 1.40 1999/01/22 07:55:49 chs Exp $	*/

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1987, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *
 *	@(#)kern_malloc.c	8.4 (Berkeley) 5/20/95
 */

#include "opt_lockdebug.h"
#include "opt_uvm.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/map.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>

#if defined(UVM)
#include <uvm/uvm_extern.h>

static struct vm_map kmem_map_store;
vm_map_t kmem_map = NULL;
#endif

#include "opt_kmemstats.h"
#include "opt_malloclog.h"

struct kmembuckets bucket[MINBUCKET + 16];
struct kmemstats kmemstats[M_LAST];
struct kmemusage *kmemusage;
char *kmembase, *kmemlimit;
const char *memname[] = INITKMEMNAMES;

#ifdef MALLOCLOG
#ifndef MALLOCLOGSIZE
#define	MALLOCLOGSIZE	100000
#endif

struct malloclog {
	void *addr;
	long size;
	int type;
	int action;
	const char *file;
	long line;
} malloclog[MALLOCLOGSIZE];

long	malloclogptr;

static void domlog __P((void *a, long size, int type, int action,
	const char *file, long line));
static void hitmlog __P((void *a));

static void
domlog(a, size, type, action, file, line)
	void *a;
	long size;
	int type;
	int action;
	const char *file;
	long line;
{

	malloclog[malloclogptr].addr = a;
	malloclog[malloclogptr].size = size;
	malloclog[malloclogptr].type = type;
	malloclog[malloclogptr].action = action;
	malloclog[malloclogptr].file = file;
	malloclog[malloclogptr].line = line;
	malloclogptr++;
	if (malloclogptr >= MALLOCLOGSIZE)
		malloclogptr = 0;
}

static void
hitmlog(a)
	void *a;
{
	struct malloclog *lp;
	long l;

#define	PRT \
	if (malloclog[l].addr == a && malloclog[l].action) { \
		lp = &malloclog[l]; \
		printf("malloc log entry %ld:\n", l); \
		printf("\taddr = %p\n", lp->addr); \
		printf("\tsize = %ld\n", lp->size); \
		printf("\ttype = %s\n", memname[lp->type]); \
		printf("\taction = %s\n", lp->action == 1 ? "alloc" : "free"); \
		printf("\tfile = %s\n", lp->file); \
		printf("\tline = %ld\n", lp->line); \
	}

	for (l = malloclogptr; l < MALLOCLOGSIZE; l++)
		PRT

	for (l = 0; l < malloclogptr; l++)
		PRT
}
#endif /* MALLOCLOG */

#ifdef DIAGNOSTIC
/*
 * This structure provides a set of masks to catch unaligned frees.
 */
long addrmask[] = { 0,
	0x00000001, 0x00000003, 0x00000007, 0x0000000f,
	0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff,
	0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff,
	0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff,
};

/*
 * The WEIRD_ADDR is used as known text to copy into free objects so
 * that modifications after frees can be detected.
 */
#define WEIRD_ADDR	((unsigned) 0xdeadbeef)
#define MAX_COPY	32

/*
 * Normally the freelist structure is used only to hold the list pointer
 * for free objects.  However, when running with diagnostics, the first
 * 8 bytes of the structure is unused except for diagnostic information,
 * and the free list pointer is at offst 8 in the structure.  Since the
 * first 8 bytes is the portion of the structure most often modified, this
 * helps to detect memory reuse problems and avoid free list corruption.
 */
struct freelist {
	int32_t	spare0;
	int16_t	type;
	int16_t	spare1;
	caddr_t	next;
};
#else /* !DIAGNOSTIC */
struct freelist {
	caddr_t	next;
};
#endif /* DIAGNOSTIC */

/*
 * Allocate a block of memory
 */
#ifdef MALLOCLOG
void *
_malloc(size, type, flags, file, line)
	unsigned long size;
	int type, flags;
	const char *file;
	long line;
#else
void *
malloc(size, type, flags)
	unsigned long size;
	int type, flags;
#endif /* MALLOCLOG */
{
	register struct kmembuckets *kbp;
	register struct kmemusage *kup;
	register struct freelist *freep;
	long indx, npg, allocsize;
	int s;
	caddr_t va, cp, savedlist;
#ifdef DIAGNOSTIC
	int32_t *end, *lp;
	int copysize;
	const char *savedtype;
#endif
#ifdef LOCKDEBUG
	extern int simplelockrecurse;
#endif
#ifdef KMEMSTATS
	register struct kmemstats *ksp = &kmemstats[type];

	if (((unsigned long)type) > M_LAST)
		panic("malloc - bogus type");
#endif
	indx = BUCKETINDX(size);
	kbp = &bucket[indx];
	s = splimp();
#ifdef KMEMSTATS
	while (ksp->ks_memuse >= ksp->ks_limit) {
		if (flags & M_NOWAIT) {
			splx(s);
			return ((void *) NULL);
		}
		if (ksp->ks_limblocks < 65535)
			ksp->ks_limblocks++;
		tsleep((caddr_t)ksp, PSWP+2, memname[type], 0);
	}
	ksp->ks_size |= 1 << indx;
#endif
#ifdef DIAGNOSTIC
	copysize = 1 << indx < MAX_COPY ? 1 << indx : MAX_COPY;
#endif
#ifdef LOCKDEBUG
	if (flags & M_NOWAIT)
		simplelockrecurse++;
#endif
	if (kbp->kb_next == NULL) {
		kbp->kb_last = NULL;
		if (size > MAXALLOCSAVE)
			allocsize = roundup(size, CLBYTES);
		else
			allocsize = 1 << indx;
		npg = clrnd(btoc(allocsize));
#if defined(UVM)
		va = (caddr_t) uvm_km_kmemalloc(kmem_map, uvmexp.kmem_object,
				(vsize_t)ctob(npg), 
				(flags & M_NOWAIT) ? UVM_KMF_NOWAIT : 0);
#else
		va = (caddr_t) kmem_malloc(kmem_map, (vsize_t)ctob(npg),
					   !(flags & M_NOWAIT));
#endif
		if (va == NULL) {
			/*
			 * Kmem_malloc() can return NULL, even if it can
			 * wait, if there is no map space avaiable, because
			 * it can't fix that problem.  Neither can we,
			 * right now.  (We should release pages which
			 * are completely free and which are in buckets
			 * with too many free elements.)
			 */
			if ((flags & M_NOWAIT) == 0)
				panic("malloc: out of space in kmem_map");
#ifdef LOCKDEBUG
			simplelockrecurse--;
#endif
			splx(s);
			return ((void *) NULL);
		}
#ifdef KMEMSTATS
		kbp->kb_total += kbp->kb_elmpercl;
#endif
		kup = btokup(va);
		kup->ku_indx = indx;
		if (allocsize > MAXALLOCSAVE) {
			if (npg > 65535)
				panic("malloc: allocation too large");
			kup->ku_pagecnt = npg;
#ifdef KMEMSTATS
			ksp->ks_memuse += allocsize;
#endif
			goto out;
		}
#ifdef KMEMSTATS
		kup->ku_freecnt = kbp->kb_elmpercl;
		kbp->kb_totalfree += kbp->kb_elmpercl;
#endif
		/*
		 * Just in case we blocked while allocating memory,
		 * and someone else also allocated memory for this
		 * bucket, don't assume the list is still empty.
		 */
		savedlist = kbp->kb_next;
		kbp->kb_next = cp = va + (npg * NBPG) - allocsize;
		for (;;) {
			freep = (struct freelist *)cp;
#ifdef DIAGNOSTIC
			/*
			 * Copy in known text to detect modification
			 * after freeing.
			 */
			end = (int32_t *)&cp[copysize];
			for (lp = (int32_t *)cp; lp < end; lp++)
				*lp = WEIRD_ADDR;
			freep->type = M_FREE;
#endif /* DIAGNOSTIC */
			if (cp <= va)
				break;
			cp -= allocsize;
			freep->next = cp;
		}
		freep->next = savedlist;
		if (kbp->kb_last == NULL)
			kbp->kb_last = (caddr_t)freep;
	}
	va = kbp->kb_next;
	kbp->kb_next = ((struct freelist *)va)->next;
#ifdef DIAGNOSTIC
	freep = (struct freelist *)va;
	savedtype = (unsigned)freep->type < M_LAST ?
		memname[freep->type] : "???";
#if defined(UVM)
	if (kbp->kb_next) {
		int rv;
		vaddr_t addr = (vaddr_t)kbp->kb_next;

		vm_map_lock_read(kmem_map);
		rv = uvm_map_checkprot(kmem_map, addr,
				       addr + sizeof(struct freelist),
				       VM_PROT_WRITE);
		vm_map_unlock_read(kmem_map);

		if (!rv)
#else
	if (kbp->kb_next &&
	    !kernacc(kbp->kb_next, sizeof(struct freelist), 0)) 
#endif
								{
		printf(
		    "%s %ld of object %p size %ld %s %s (invalid addr %p)\n",
		    "Data modified on freelist: word", 
		    (long)((int32_t *)&kbp->kb_next - (int32_t *)kbp),
		    va, size, "previous type", savedtype, kbp->kb_next);
#ifdef MALLOCLOG
		hitmlog(va);
#endif
		kbp->kb_next = NULL;
#if defined(UVM)
		}
#endif
	}

	/* Fill the fields that we've used with WEIRD_ADDR */
#if BYTE_ORDER == BIG_ENDIAN
	freep->type = WEIRD_ADDR >> 16;
#endif
#if BYTE_ORDER == LITTLE_ENDIAN
	freep->type = (short)WEIRD_ADDR;
#endif
	end = (int32_t *)&freep->next +
	    (sizeof(freep->next) / sizeof(int32_t));
	for (lp = (int32_t *)&freep->next; lp < end; lp++)
		*lp = WEIRD_ADDR;

	/* and check that the data hasn't been modified. */
	end = (int32_t *)&va[copysize];
	for (lp = (int32_t *)va; lp < end; lp++) {
		if (*lp == WEIRD_ADDR)
			continue;
		printf("%s %ld of object %p size %ld %s %s (0x%x != 0x%x)\n",
		    "Data modified on freelist: word",
		    (long)(lp - (int32_t *)va), va, size, "previous type",
		    savedtype, *lp, WEIRD_ADDR);
#ifdef MALLOCLOG
		hitmlog(va);
#endif
		break;
	}

	freep->spare0 = 0;
#endif /* DIAGNOSTIC */
#ifdef KMEMSTATS
	kup = btokup(va);
	if (kup->ku_indx != indx)
		panic("malloc: wrong bucket");
	if (kup->ku_freecnt == 0)
		panic("malloc: lost data");
	kup->ku_freecnt--;
	kbp->kb_totalfree--;
	ksp->ks_memuse += 1 << indx;
out:
	kbp->kb_calls++;
	ksp->ks_inuse++;
	ksp->ks_calls++;
	if (ksp->ks_memuse > ksp->ks_maxused)
		ksp->ks_maxused = ksp->ks_memuse;
#else
out:
#endif
#ifdef MALLOCLOG
	domlog(va, size, type, 1, file, line);
#endif
	splx(s);
#ifdef LOCKDEBUG
	if (flags & M_NOWAIT)
		simplelockrecurse--;
#endif
	return ((void *) va);
}

/*
 * Free a block of memory allocated by malloc.
 */
#ifdef MALLOCLOG
void
_free(addr, type, file, line)
	void *addr;
	int type;
	const char *file;
	long line;
#else
void
free(addr, type)
	void *addr;
	int type;
#endif /* MALLOCLOG */
{
	register struct kmembuckets *kbp;
	register struct kmemusage *kup;
	register struct freelist *freep;
	long size;
	int s;
#ifdef DIAGNOSTIC
	caddr_t cp;
	int32_t *end, *lp;
	long alloc, copysize;
#endif
#ifdef KMEMSTATS
	register struct kmemstats *ksp = &kmemstats[type];
#endif

	kup = btokup(addr);
	size = 1 << kup->ku_indx;
	kbp = &bucket[kup->ku_indx];
	s = splimp();
#ifdef MALLOCLOG
	domlog(addr, 0, type, 2, file, line);
#endif
#ifdef DIAGNOSTIC
	/*
	 * Check for returns of data that do not point to the
	 * beginning of the allocation.
	 */
	if (size > NBPG * CLSIZE)
		alloc = addrmask[BUCKETINDX(NBPG * CLSIZE)];
	else
		alloc = addrmask[kup->ku_indx];
	if (((u_long)addr & alloc) != 0)
		panic("free: unaligned addr %p, size %ld, type %s, mask %ld\n",
			addr, size, memname[type], alloc);
#endif /* DIAGNOSTIC */
	if (size > MAXALLOCSAVE) {
#if defined(UVM)
		uvm_km_free(kmem_map, (vaddr_t)addr, ctob(kup->ku_pagecnt));
#else
		kmem_free(kmem_map, (vaddr_t)addr, ctob(kup->ku_pagecnt));
#endif
#ifdef KMEMSTATS
		size = kup->ku_pagecnt << PGSHIFT;
		ksp->ks_memuse -= size;
		kup->ku_indx = 0;
		kup->ku_pagecnt = 0;
		if (ksp->ks_memuse + size >= ksp->ks_limit &&
		    ksp->ks_memuse < ksp->ks_limit)
			wakeup((caddr_t)ksp);
		ksp->ks_inuse--;
		kbp->kb_total -= 1;
#endif
		splx(s);
		return;
	}
	freep = (struct freelist *)addr;
#ifdef DIAGNOSTIC
	/*
	 * Check for multiple frees. Use a quick check to see if
	 * it looks free before laboriously searching the freelist.
	 */
	if (freep->spare0 == WEIRD_ADDR) {
		for (cp = kbp->kb_next; cp;
		    cp = ((struct freelist *)cp)->next) {
			if (addr != cp)
				continue;
			printf("multiply freed item %p\n", addr);
#ifdef MALLOCLOG
			hitmlog(addr);
#endif
			panic("free: duplicated free");
		}
	}
#ifdef LOCKDEBUG
	/*
	 * Check if we're freeing a locked simple lock.
	 */
	simple_lock_freecheck(addr, (char *)addr + size);
#endif
	/*
	 * Copy in known text to detect modification after freeing
	 * and to make it look free. Also, save the type being freed
	 * so we can list likely culprit if modification is detected
	 * when the object is reallocated.
	 */
	copysize = size < MAX_COPY ? size : MAX_COPY;
	end = (int32_t *)&((caddr_t)addr)[copysize];
	for (lp = (int32_t *)addr; lp < end; lp++)
		*lp = WEIRD_ADDR;
	freep->type = type;
#endif /* DIAGNOSTIC */
#ifdef KMEMSTATS
	kup->ku_freecnt++;
	if (kup->ku_freecnt >= kbp->kb_elmpercl) {
		if (kup->ku_freecnt > kbp->kb_elmpercl)
			panic("free: multiple frees");
		else if (kbp->kb_totalfree > kbp->kb_highwat)
			kbp->kb_couldfree++;
	}
	kbp->kb_totalfree++;
	ksp->ks_memuse -= size;
	if (ksp->ks_memuse + size >= ksp->ks_limit &&
	    ksp->ks_memuse < ksp->ks_limit)
		wakeup((caddr_t)ksp);
	ksp->ks_inuse--;
#endif
	if (kbp->kb_next == NULL)
		kbp->kb_next = addr;
	else
		((struct freelist *)kbp->kb_last)->next = addr;
	freep->next = NULL;
	kbp->kb_last = addr;
	splx(s);
}

/*
 * Change the size of a block of memory.
 */
void *
realloc(curaddr, newsize, type, flags)
	void *curaddr;
	unsigned long newsize;
	int type, flags;
{
	register struct kmemusage *kup;
	long cursize;
	void *newaddr;
#ifdef DIAGNOSTIC
	long alloc;
#endif

	/*
	 * Realloc() with a NULL pointer is the same as malloc().
	 */
	if (curaddr == NULL)
		return (malloc(newsize, type, flags));

	/*
	 * Realloc() with zero size is the same as free().
	 */
	if (newsize == 0) {
		free(curaddr, type);
		return (NULL);
	}

	/*
	 * Find out how large the old allocation was (and do some
	 * sanity checking).
	 */
	kup = btokup(curaddr);
	cursize = 1 << kup->ku_indx;

#ifdef DIAGNOSTIC
	/*
	 * Check for returns of data that do not point to the
	 * beginning of the allocation.
	 */
	if (cursize > NBPG * CLSIZE)
		alloc = addrmask[BUCKETINDX(NBPG * CLSIZE)];
	else
		alloc = addrmask[kup->ku_indx];
	if (((u_long)curaddr & alloc) != 0)
		panic("realloc: unaligned addr %p, size %ld, type %s, mask %ld\n",
			curaddr, cursize, memname[type], alloc);
#endif /* DIAGNOSTIC */

	if (cursize > MAXALLOCSAVE)
		cursize = ctob(kup->ku_pagecnt);

	/*
	 * If we already actually have as much as they want, we're done.
	 */
	if (newsize <= cursize)
		return (curaddr);

	/*
	 * Can't satisfy the allocation with the existing block.
	 * Allocate a new one and copy the data.
	 */
	newaddr = malloc(newsize, type, flags);
	if (newaddr == NULL) {
		/*
		 * Malloc() failed, because flags included M_NOWAIT.
		 * Return NULL to indicate that failure.  The old
		 * pointer is still valid.
		 */
		return NULL;
	}
	memcpy(newaddr, curaddr, cursize);

	/*
	 * We were successful: free the old allocation and return
	 * the new one.
	 */
	free(curaddr, type);
	return (newaddr);
}

/*
 * Initialize the kernel memory allocator
 */
void
kmeminit()
{
#ifdef KMEMSTATS
	register long indx;
#endif
	int npg;

#if	((MAXALLOCSAVE & (MAXALLOCSAVE - 1)) != 0)
		ERROR!_kmeminit:_MAXALLOCSAVE_not_power_of_2
#endif
#if	(MAXALLOCSAVE > MINALLOCSIZE * 32768)
		ERROR!_kmeminit:_MAXALLOCSAVE_too_big
#endif
#if	(MAXALLOCSAVE < CLBYTES)
		ERROR!_kmeminit:_MAXALLOCSAVE_too_small
#endif

	if (sizeof(struct freelist) > (1 << MINBUCKET))
		panic("minbucket too small/struct freelist too big");

	npg = VM_KMEM_SIZE/ NBPG;
#if defined(UVM)
	kmemusage = (struct kmemusage *) uvm_km_zalloc(kernel_map,
		(vsize_t)(npg * sizeof(struct kmemusage)));
	kmem_map = uvm_km_suballoc(kernel_map, (vaddr_t *)&kmembase,
		(vaddr_t *)&kmemlimit, (vsize_t)(npg * NBPG), 
			FALSE, FALSE, &kmem_map_store);
#else
	kmemusage = (struct kmemusage *) kmem_alloc(kernel_map,
		(vsize_t)(npg * sizeof(struct kmemusage)));
	kmem_map = kmem_suballoc(kernel_map, (vaddr_t *)&kmembase,
		(vaddr_t *)&kmemlimit, (vsize_t)(npg * NBPG), FALSE);
#endif
#ifdef KMEMSTATS
	for (indx = 0; indx < MINBUCKET + 16; indx++) {
		if (1 << indx >= CLBYTES)
			bucket[indx].kb_elmpercl = 1;
		else
			bucket[indx].kb_elmpercl = CLBYTES / (1 << indx);
		bucket[indx].kb_highwat = 5 * bucket[indx].kb_elmpercl;
	}
	for (indx = 0; indx < M_LAST; indx++)
		kmemstats[indx].ks_limit = npg * NBPG * 6 / 10;
#endif
}

#ifdef DDB
#include <ddb/db_output.h>

/*
 * Dump kmem statistics from ddb.
 *
 * usage: call dump_kmemstats
 */
void	dump_kmemstats __P((void));

void
dump_kmemstats()
{
#ifdef KMEMSTATS
	const char *name;
	int i;

	for (i = 0; i < M_LAST; i++) {
		name = memname[i] ? memname[i] : "";

		db_printf("%2d %s%.*s %ld\n", i, name,
		    (int)(20 - strlen(name)), "                    ",
		    kmemstats[i].ks_memuse);
	}
#else
	db_printf("Kmem stats are not being collected.\n");
#endif /* KMEMSTATS */
}
#endif /* DDB */
