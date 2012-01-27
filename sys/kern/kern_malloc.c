/*	$NetBSD: kern_malloc.c,v 1.134 2012/01/27 19:48:40 para Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: kern_malloc.c,v 1.134 2012/01/27 19:48:40 para Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/kmem.h>
#include <sys/systm.h>
#include <sys/debug.h>
#include <sys/mutex.h>
#include <sys/lockdebug.h>

#include <uvm/uvm_extern.h>

#include "opt_kmemstats.h"
#include "opt_malloclog.h"
#include "opt_malloc_debug.h"

struct kmembuckets kmembuckets[MINBUCKET + 16];
struct kmemusage *kmemusage;
struct malloc_type *kmemstatistics;

kmutex_t malloc_lock;

extern void *kmem_intr_alloc(size_t, km_flag_t);
extern void *kmem_intr_zalloc(size_t, km_flag_t);
extern void kmem_intr_free(void *, size_t);

struct malloc_header {
	size_t mh_size;
};

/*
 * Allocate a block of memory
 */
#ifdef MALLOCLOG
void *
_kern_malloc(unsigned long size, struct malloc_type *ksp, int flags,
    const char *file, long line)
#else
void *
kern_malloc(unsigned long size, struct malloc_type *ksp, int flags)
#endif /* MALLOCLOG */
{
	struct malloc_header *mh;
	int kmflags = ((flags & M_NOWAIT) != 0
	    ? KM_NOSLEEP : KM_SLEEP);
	size_t allocsize = sizeof(struct malloc_header) + size;
	void *p;

	p = kmem_intr_alloc(allocsize, kmflags);
	if (p == NULL)
		return NULL;

	if ((flags & M_ZERO) != 0) {
		memset(p, 0, allocsize);
	}
	mh = (void *)p;
	mh->mh_size = allocsize;

	return mh + 1;
}

/*
 * Free a block of memory allocated by malloc.
 */
#ifdef MALLOCLOG
void
_kern_free(void *addr, struct malloc_type *ksp, const char *file, long line)
#else
void
kern_free(void *addr, struct malloc_type *ksp)
#endif /* MALLOCLOG */
{
	struct malloc_header *mh;

	mh = addr;
	mh--;

	kmem_intr_free(mh, mh->mh_size);
}

/*
 * Change the size of a block of memory.
 */
void *
kern_realloc(void *curaddr, unsigned long newsize, struct malloc_type *ksp,
    int flags)
{
	struct malloc_header *mh;
	unsigned long cursize;
	void *newaddr;

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
	if ((flags & M_NOWAIT) == 0) {
		ASSERT_SLEEPABLE();
	}
#endif

	mh = curaddr;
	mh--;

	cursize = mh->mh_size;

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
 * Add a malloc type to the system.
 */
void
malloc_type_attach(struct malloc_type *type)
{

	if (type->ks_magic != M_MAGIC)
		panic("malloc_type_attach: bad magic");

#ifdef DIAGNOSTIC
	{
		struct malloc_type *ksp;
		for (ksp = kmemstatistics; ksp != NULL; ksp = ksp->ks_next) {
			if (ksp == type)
				panic("%s: `%s' already on list", __func__,
				    type->ks_shortdesc);
		}
	}
#endif

#ifdef KMEMSTATS
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
	mutex_spin_enter(&malloc_lock);
	type->ks_limit = limit;
	mutex_spin_exit(&malloc_lock);
#endif
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

	mutex_init(&malloc_lock, MUTEX_DEFAULT, IPL_VM);

#ifdef KMEMSTATS
	for (indx = 0; indx < MINBUCKET + 16; indx++) {
		if (1 << indx >= PAGE_SIZE)
			kmembuckets[indx].kb_elmpercl = 1;
		else
			kmembuckets[indx].kb_elmpercl = PAGE_SIZE / (1 << indx);
		kmembuckets[indx].kb_highwat =
			5 * kmembuckets[indx].kb_elmpercl;
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


#if 0
/*
 * Diagnostic messages about "Data modified on
 * freelist" indicate a memory corruption, but
 * they do not help tracking it down.
 * This function can be called at various places
 * to sanity check malloc's freelist and discover
 * where does the corruption take place.
 */
int
freelist_sanitycheck(void) {
	int i,j;
	struct kmembuckets *kbp;
	struct freelist *freep;
	int rv = 0;

	for (i = MINBUCKET; i <= MINBUCKET + 15; i++) {
		kbp = &kmembuckets[i];
		freep = (struct freelist *)kbp->kb_next;
		j = 0;
		while(freep) {
			vm_map_lock(kmem_map);
			rv = uvm_map_checkprot(kmem_map, (vaddr_t)freep,
			    (vaddr_t)freep + sizeof(struct freelist),
			    VM_PROT_WRITE);
			vm_map_unlock(kmem_map);

			if ((rv == 0) || (*(int *)freep != WEIRD_ADDR)) {
				printf("bucket %i, chunck %d at %p modified\n",
				    i, j, freep);
				return 1;
			}
			freep = (struct freelist *)freep->next;
			j++;
		}
	}

	return 0;
}
#endif
