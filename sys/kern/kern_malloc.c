/*	$NetBSD: kern_malloc.c,v 1.81 2003/08/07 16:31:46 agc Exp $	*/

/*
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
 *
 *	@(#)kern_malloc.c	8.4 (Berkeley) 5/20/95
 */

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_malloc.c,v 1.81 2003/08/07 16:31:46 agc Exp $");

#include "opt_lockdebug.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

static struct vm_map kmem_map_store;
struct vm_map *kmem_map = NULL;

#include "opt_kmempages.h"

#ifdef NKMEMCLUSTERS
#error NKMEMCLUSTERS is obsolete; remove it from your kernel config file and use NKMEMPAGES instead or let the kernel auto-size
#endif

/*
 * Default number of pages in kmem_map.  We attempt to calculate this
 * at run-time, but allow it to be either patched or set in the kernel
 * config file.
 */
#ifndef NKMEMPAGES
#define	NKMEMPAGES	0
#endif
int	nkmempages = NKMEMPAGES;

/*
 * Defaults for lower- and upper-bounds for the kmem_map page count.
 * Can be overridden by kernel config options.
 */
#ifndef	NKMEMPAGES_MIN
#define	NKMEMPAGES_MIN	NKMEMPAGES_MIN_DEFAULT
#endif

#ifndef NKMEMPAGES_MAX
#define	NKMEMPAGES_MAX	NKMEMPAGES_MAX_DEFAULT
#endif

#include "opt_kmemstats.h"
#include "opt_malloclog.h"
#include "opt_malloc_debug.h"

struct kmembuckets bucket[MINBUCKET + 16];
struct kmemusage *kmemusage;
char *kmembase, *kmemlimit;

struct malloc_type *kmemstatistics;

#ifdef MALLOCLOG
#ifndef MALLOCLOGSIZE
#define	MALLOCLOGSIZE	100000
#endif

struct malloclog {
	void *addr;
	long size;
	struct malloc_type *type;
	int action;
	const char *file;
	long line;
} malloclog[MALLOCLOGSIZE];

long	malloclogptr;

static void
domlog(void *a, long size, struct malloc_type *type, int action,
    const char *file, long line)
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
hitmlog(void *a)
{
	struct malloclog *lp;
	long l;

#define	PRT do { \
	if (malloclog[l].addr == a && malloclog[l].action) { \
		lp = &malloclog[l]; \
		printf("malloc log entry %ld:\n", l); \
		printf("\taddr = %p\n", lp->addr); \
		printf("\tsize = %ld\n", lp->size); \
		printf("\ttype = %s\n", lp->type->ks_shortdesc); \
		printf("\taction = %s\n", lp->action == 1 ? "alloc" : "free"); \
		printf("\tfile = %s\n", lp->file); \
		printf("\tline = %ld\n", lp->line); \
	} \
} while (/* CONSTCOND */0)

	for (l = malloclogptr; l < MALLOCLOGSIZE; l++)
		PRT;

	for (l = 0; l < malloclogptr; l++)
		PRT;
}
#endif /* MALLOCLOG */

#ifdef DIAGNOSTIC
/*
 * This structure provides a set of masks to catch unaligned frees.
 */
const long addrmask[] = { 0,
	0x00000001, 0x00000003, 0x00000007, 0x0000000f,
	0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff,
	0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff,
	0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff,
};

/*
 * The WEIRD_ADDR is used as known text to copy into free objects so
 * that modifications after frees can be detected.
 */
#define	WEIRD_ADDR	((uint32_t) 0xdeadbeef)
#ifdef DEBUG
#define	MAX_COPY	PAGE_SIZE
#else
#define	MAX_COPY	32
#endif

/*
 * Normally the freelist structure is used only to hold the list pointer
 * for free objects.  However, when running with diagnostics, the first
 * 8/16 bytes of the structure is unused except for diagnostic information,
 * and the free list pointer is at offset 8/16 in the structure.  Since the
 * first 8 bytes is the portion of the structure most often modified, this
 * helps to detect memory reuse problems and avoid free list corruption.
 */
struct freelist {
	uint32_t spare0;
#ifdef _LP64
	uint32_t spare1;		/* explicit padding */
#endif
	struct malloc_type *type;
	caddr_t	next;
};
#else /* !DIAGNOSTIC */
struct freelist {
	caddr_t	next;
};
#endif /* DIAGNOSTIC */

/*
 * The following are standard, build-in malloc types are are not
 * specific to any one subsystem.
 */
MALLOC_DEFINE(M_DEVBUF, "devbuf", "device driver memory");
MALLOC_DEFINE(M_DMAMAP, "DMA map", "bus_dma(9) structures");
MALLOC_DEFINE(M_FREE, "free", "should be on free list");
MALLOC_DEFINE(M_PCB, "pcb", "protocol control block");
MALLOC_DEFINE(M_SOFTINTR, "softintr", "Softinterrupt structures");
MALLOC_DEFINE(M_TEMP, "temp", "misc. temporary data buffers");

/* XXX These should all be elsewhere. */
MALLOC_DEFINE(M_RTABLE, "routetbl", "routing tables");
MALLOC_DEFINE(M_FTABLE, "fragtbl", "fragment reassembly header");
MALLOC_DEFINE(M_UFSMNT, "UFS mount", "UFS mount structure");
MALLOC_DEFINE(M_NETADDR, "Export Host", "Export host address structure");
MALLOC_DEFINE(M_IPMOPTS, "ip_moptions", "internet multicast options");
MALLOC_DEFINE(M_IPMADDR, "in_multi", "internet multicast address");
MALLOC_DEFINE(M_MRTABLE, "mrt", "multicast routing tables");
MALLOC_DEFINE(M_1394DATA, "1394data", "IEEE 1394 data buffers");

struct simplelock malloc_slock = SIMPLELOCK_INITIALIZER;

/*
 * Allocate a block of memory
 */
#ifdef MALLOCLOG
void *
_malloc(unsigned long size, struct malloc_type *ksp, int flags,
    const char *file, long line)
#else
void *
malloc(unsigned long size, struct malloc_type *ksp, int flags)
#endif /* MALLOCLOG */
{
	struct kmembuckets *kbp;
	struct kmemusage *kup;
	struct freelist *freep;
	long indx, npg, allocsize;
	int s;
	caddr_t va, cp, savedlist;
#ifdef DIAGNOSTIC
	uint32_t *end, *lp;
	int copysize;
	const char *savedtype;
#endif

#ifdef LOCKDEBUG
	if ((flags & M_NOWAIT) == 0)
		simple_lock_only_held(NULL, "malloc");
#endif
#ifdef MALLOC_DEBUG
	if (debug_malloc(size, ksp, flags, (void **) &va))
		return ((void *) va);
#endif
	indx = BUCKETINDX(size);
	kbp = &bucket[indx];
	s = splvm();
	simple_lock(&malloc_slock);
#ifdef KMEMSTATS
	while (ksp->ks_memuse >= ksp->ks_limit) {
		if (flags & M_NOWAIT) {
			simple_unlock(&malloc_slock);
			splx(s);
			return ((void *) NULL);
		}
		if (ksp->ks_limblocks < 65535)
			ksp->ks_limblocks++;
		ltsleep((caddr_t)ksp, PSWP+2, ksp->ks_shortdesc, 0,
			&malloc_slock);
	}
	ksp->ks_size |= 1 << indx;
#endif
#ifdef DIAGNOSTIC
	copysize = 1 << indx < MAX_COPY ? 1 << indx : MAX_COPY;
#endif
	if (kbp->kb_next == NULL) {
		kbp->kb_last = NULL;
		if (size > MAXALLOCSAVE)
			allocsize = round_page(size);
		else
			allocsize = 1 << indx;
		npg = btoc(allocsize);
		simple_unlock(&malloc_slock);
		va = (caddr_t) uvm_km_kmemalloc(kmem_map, NULL,
		    (vsize_t)ctob(npg),
		    ((flags & M_NOWAIT) ? UVM_KMF_NOWAIT : 0) |
		    ((flags & M_CANFAIL) ? UVM_KMF_CANFAIL : 0));
		if (__predict_false(va == NULL)) {
			/*
			 * Kmem_malloc() can return NULL, even if it can
			 * wait, if there is no map space avaiable, because
			 * it can't fix that problem.  Neither can we,
			 * right now.  (We should release pages which
			 * are completely free and which are in buckets
			 * with too many free elements.)
			 */
			if ((flags & (M_NOWAIT|M_CANFAIL)) == 0)
				panic("malloc: out of space in kmem_map");
			splx(s);
			return (NULL);
		}
		simple_lock(&malloc_slock);
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
		kbp->kb_next = cp = va + (npg << PAGE_SHIFT) - allocsize;
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
	/* XXX potential to get garbage pointer here. */
	savedtype = freep->type->ks_shortdesc;
	if (kbp->kb_next) {
		int rv;
		vaddr_t addr = (vaddr_t)kbp->kb_next;

		vm_map_lock(kmem_map);
		rv = uvm_map_checkprot(kmem_map, addr,
		    addr + sizeof(struct freelist), VM_PROT_WRITE);
		vm_map_unlock(kmem_map);

		if (__predict_false(rv == 0)) {
			printf("Data modified on freelist: "
			    "word %ld of object %p size %ld previous type %s "
			    "(invalid addr %p)\n",
			    (long)((int32_t *)&kbp->kb_next - (int32_t *)kbp),
			    va, size, "foo", kbp->kb_next);
#ifdef MALLOCLOG
			hitmlog(va);
#endif
			kbp->kb_next = NULL;
		}
	}

	/* Fill the fields that we've used with WEIRD_ADDR */
#ifdef _LP64
	freep->type = (struct malloc_type *)
	    (WEIRD_ADDR | (((u_long) WEIRD_ADDR) << 32));
#else
	freep->type = (struct malloc_type *) WEIRD_ADDR;
#endif
	end = (int32_t *)&freep->next +
	    (sizeof(freep->next) / sizeof(int32_t));
	for (lp = (int32_t *)&freep->next; lp < end; lp++)
		*lp = WEIRD_ADDR;

	/* and check that the data hasn't been modified. */
	end = (uint32_t *)&va[copysize];
	for (lp = (int32_t *)va; lp < end; lp++) {
		if (__predict_true(*lp == WEIRD_ADDR))
			continue;
		printf("Data modified on freelist: "
		    "word %ld of object %p size %ld previous type %s "
		    "(0x%x != 0x%x)\n",
		    (long)(lp - (uint32_t *)va), va, size,
		    "bar", *lp, WEIRD_ADDR);
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
	domlog(va, size, ksp, 1, file, line);
#endif
	simple_unlock(&malloc_slock);
	splx(s);
	if ((flags & M_ZERO) != 0)
		memset(va, 0, size);
	return ((void *) va);
}

/*
 * Free a block of memory allocated by malloc.
 */
#ifdef MALLOCLOG
void
_free(void *addr, struct malloc_type *ksp, const char *file, long line)
#else
void
free(void *addr, struct malloc_type *ksp)
#endif /* MALLOCLOG */
{
	struct kmembuckets *kbp;
	struct kmemusage *kup;
	struct freelist *freep;
	long size;
	int s;
#ifdef DIAGNOSTIC
	caddr_t cp;
	int32_t *end, *lp;
	long alloc, copysize;
#endif

#ifdef MALLOC_DEBUG
	if (debug_free(addr, ksp))
		return;
#endif

#ifdef DIAGNOSTIC
	/*
	 * Ensure that we're free'ing something that we could
	 * have allocated in the first place.  That is, check
	 * to see that the address is within kmem_map.
	 */
	if (__predict_false((vaddr_t)addr < kmem_map->header.start ||
	    (vaddr_t)addr >= kmem_map->header.end))
		panic("free: addr %p not within kmem_map", addr);
#endif

	kup = btokup(addr);
	size = 1 << kup->ku_indx;
	kbp = &bucket[kup->ku_indx];
	s = splvm();
	simple_lock(&malloc_slock);
#ifdef MALLOCLOG
	domlog(addr, 0, ksp, 2, file, line);
#endif
#ifdef DIAGNOSTIC
	/*
	 * Check for returns of data that do not point to the
	 * beginning of the allocation.
	 */
	if (size > PAGE_SIZE)
		alloc = addrmask[BUCKETINDX(PAGE_SIZE)];
	else
		alloc = addrmask[kup->ku_indx];
	if (((u_long)addr & alloc) != 0)
		panic("free: unaligned addr %p, size %ld, type %s, mask %ld",
		    addr, size, ksp->ks_shortdesc, alloc);
#endif /* DIAGNOSTIC */
	if (size > MAXALLOCSAVE) {
		uvm_km_free(kmem_map, (vaddr_t)addr, ctob(kup->ku_pagecnt));
#ifdef KMEMSTATS
		size = kup->ku_pagecnt << PGSHIFT;
		ksp->ks_memuse -= size;
		kup->ku_indx = 0;
		kup->ku_pagecnt = 0;
		if (ksp->ks_memuse + size >= ksp->ks_limit &&
		    ksp->ks_memuse < ksp->ks_limit)
			wakeup((caddr_t)ksp);
#ifdef DIAGNOSTIC
		if (ksp->ks_inuse == 0)
			panic("free 1: inuse 0, probable double free");
#endif
		ksp->ks_inuse--;
		kbp->kb_total -= 1;
#endif
		simple_unlock(&malloc_slock);
		splx(s);
		return;
	}
	freep = (struct freelist *)addr;
#ifdef DIAGNOSTIC
	/*
	 * Check for multiple frees. Use a quick check to see if
	 * it looks free before laboriously searching the freelist.
	 */
	if (__predict_false(freep->spare0 == WEIRD_ADDR)) {
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
	freep->type = ksp;
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
#ifdef DIAGNOSTIC
	if (ksp->ks_inuse == 0)
		panic("free 2: inuse 0, probable double free");
#endif
	ksp->ks_inuse--;
#endif
	if (kbp->kb_next == NULL)
		kbp->kb_next = addr;
	else
		((struct freelist *)kbp->kb_last)->next = addr;
	freep->next = NULL;
	kbp->kb_last = addr;
	simple_unlock(&malloc_slock);
	splx(s);
}

/*
 * Change the size of a block of memory.
 */
void *
realloc(void *curaddr, unsigned long newsize, struct malloc_type *ksp,
    int flags)
{
	struct kmemusage *kup;
	unsigned long cursize;
	void *newaddr;
#ifdef DIAGNOSTIC
	long alloc;
#endif

	/*
	 * realloc() with a NULL pointer is the same as malloc().
	 */
	if (curaddr == NULL)
		return (malloc(newsize, ksp, flags));

	/*
	 * realloc() with zero size is the same as free().
	 */
	if (newsize == 0) {
		free(curaddr, ksp);
		return (NULL);
	}

#ifdef LOCKDEBUG
	if ((flags & M_NOWAIT) == 0)
		simple_lock_only_held(NULL, "realloc");
#endif

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
	if (cursize > PAGE_SIZE)
		alloc = addrmask[BUCKETINDX(PAGE_SIZE)];
	else
		alloc = addrmask[kup->ku_indx];
	if (((u_long)curaddr & alloc) != 0)
		panic("realloc: "
		    "unaligned addr %p, size %ld, type %s, mask %ld\n",
		    curaddr, cursize, ksp->ks_shortdesc, alloc);
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
	newaddr = malloc(newsize, ksp, flags);
	if (__predict_false(newaddr == NULL)) {
		/*
		 * malloc() failed, because flags included M_NOWAIT.
		 * Return NULL to indicate that failure.  The old
		 * pointer is still valid.
		 */
		return (NULL);
	}
	memcpy(newaddr, curaddr, cursize);

	/*
	 * We were successful: free the old allocation and return
	 * the new one.
	 */
	free(curaddr, ksp);
	return (newaddr);
}

/*
 * Roundup size to the actual allocation size.
 */
unsigned long
malloc_roundup(unsigned long size)
{

	if (size > MAXALLOCSAVE)
		return (roundup(size, PAGE_SIZE));
	else
		return (1 << BUCKETINDX(size));
}

/*
 * Add a malloc type to the system.
 */
void
malloc_type_attach(struct malloc_type *type)
{

	if (nkmempages == 0)
		panic("malloc_type_attach: nkmempages == 0");

	if (type->ks_magic != M_MAGIC)
		panic("malloc_type_attach: bad magic");

#ifdef DIAGNOSTIC
	{
		struct malloc_type *ksp;
		for (ksp = kmemstatistics; ksp != NULL; ksp = ksp->ks_next) {
			if (ksp == type)
				panic("malloc_type_attach: already on list");
		}
	}
#endif

#ifdef KMEMSTATS
	if (type->ks_limit == 0)
		type->ks_limit = ((u_long)nkmempages << PAGE_SHIFT) * 6U / 10U;
#else
	type->ks_limit = 0;
#endif

	type->ks_next = kmemstatistics;
	kmemstatistics = type;
}

/*
 * Remove a malloc type from the system..
 */
void
malloc_type_detach(struct malloc_type *type)
{
	struct malloc_type *ksp;

#ifdef DIAGNOSTIC
	if (type->ks_magic != M_MAGIC)
		panic("malloc_type_detach: bad magic");
#endif

	if (type == kmemstatistics)
		kmemstatistics = type->ks_next;
	else {
		for (ksp = kmemstatistics; ksp->ks_next != NULL;
		     ksp = ksp->ks_next) {
			if (ksp->ks_next == type) {
				ksp->ks_next = type->ks_next;
				break;
			}
		}
#ifdef DIAGNOSTIC
		if (ksp->ks_next == NULL)
			panic("malloc_type_detach: not on list");
#endif
	}
	type->ks_next = NULL;
}

/*
 * Set the limit on a malloc type.
 */
void
malloc_type_setlimit(struct malloc_type *type, u_long limit)
{
#ifdef KMEMSTATS
	int s;

	s = splvm();
	type->ks_limit = limit;
	splx(s);
#endif
}

/*
 * Compute the number of pages that kmem_map will map, that is,
 * the size of the kernel malloc arena.
 */
void
kmeminit_nkmempages(void)
{
	int npages;

	if (nkmempages != 0) {
		/*
		 * It's already been set (by us being here before, or
		 * by patching or kernel config options), bail out now.
		 */
		return;
	}

	/*
	 * We use the following (simple) formula:
	 *
	 *	- Starting point is physical memory / 4.
	 *
	 *	- Clamp it down to NKMEMPAGES_MAX.
	 *
	 *	- Round it up to NKMEMPAGES_MIN.
	 */
	npages = physmem / 4;

	if (npages > NKMEMPAGES_MAX)
		npages = NKMEMPAGES_MAX;

	if (npages < NKMEMPAGES_MIN)
		npages = NKMEMPAGES_MIN;

	nkmempages = npages;
}

/*
 * Initialize the kernel memory allocator
 */
void
kmeminit(void)
{
	__link_set_decl(malloc_types, struct malloc_type);
	struct malloc_type * const *ksp;
#ifdef KMEMSTATS
	long indx;
#endif

#if	((MAXALLOCSAVE & (MAXALLOCSAVE - 1)) != 0)
		ERROR!_kmeminit:_MAXALLOCSAVE_not_power_of_2
#endif
#if	(MAXALLOCSAVE > MINALLOCSIZE * 32768)
		ERROR!_kmeminit:_MAXALLOCSAVE_too_big
#endif
#if	(MAXALLOCSAVE < NBPG)
		ERROR!_kmeminit:_MAXALLOCSAVE_too_small
#endif

	if (sizeof(struct freelist) > (1 << MINBUCKET))
		panic("minbucket too small/struct freelist too big");

	/*
	 * Compute the number of kmem_map pages, if we have not
	 * done so already.
	 */
	kmeminit_nkmempages();

	kmemusage = (struct kmemusage *) uvm_km_zalloc(kernel_map,
	    (vsize_t)(nkmempages * sizeof(struct kmemusage)));
	kmem_map = uvm_km_suballoc(kernel_map, (void *)&kmembase,
	    (void *)&kmemlimit, (vsize_t)(nkmempages << PAGE_SHIFT), 
	    VM_MAP_INTRSAFE, FALSE, &kmem_map_store);
#ifdef KMEMSTATS
	for (indx = 0; indx < MINBUCKET + 16; indx++) {
		if (1 << indx >= PAGE_SIZE)
			bucket[indx].kb_elmpercl = 1;
		else
			bucket[indx].kb_elmpercl = PAGE_SIZE / (1 << indx);
		bucket[indx].kb_highwat = 5 * bucket[indx].kb_elmpercl;
	}
#endif

	/* Attach all of the statically-linked malloc types. */
	__link_set_foreach(ksp, malloc_types)
		malloc_type_attach(*ksp);

#ifdef MALLOC_DEBUG
	debug_malloc_init();
#endif
}

#ifdef DDB
#include <ddb/db_output.h>

/*
 * Dump kmem statistics from ddb.
 *
 * usage: call dump_kmemstats
 */
void	dump_kmemstats(void);

void
dump_kmemstats(void)
{
#ifdef KMEMSTATS
	struct malloc_type *ksp;

	for (ksp = kmemstatistics; ksp != NULL; ksp = ksp->ks_next) {
		if (ksp->ks_memuse == 0)
			continue;
		db_printf("%s%.*s %ld\n", ksp->ks_shortdesc,
		    (int)(20 - strlen(ksp->ks_shortdesc)),
		    "                    ",
		    ksp->ks_memuse);
	}
#else
	db_printf("Kmem stats are not being collected.\n");
#endif /* KMEMSTATS */
}
#endif /* DDB */
