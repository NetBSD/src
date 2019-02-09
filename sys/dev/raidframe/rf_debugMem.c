/*	$NetBSD: rf_debugMem.c,v 1.22 2019/02/09 03:34:00 christos Exp $	*/
/*
 * Copyright (c) 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Daniel Stodolsky, Mark Holland, Jim Zelenka
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/* debugMem.c:  memory usage debugging stuff.
 * Malloc, Calloc, and Free are #defined everywhere
 * to do_malloc, do_calloc, and do_free.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rf_debugMem.c,v 1.22 2019/02/09 03:34:00 christos Exp $");

#include <dev/raidframe/raidframevar.h>

#include "rf_threadstuff.h"
#include "rf_options.h"
#include "rf_debugMem.h"
#include "rf_general.h"
#include "rf_shutdown.h"

#if RF_DEBUG_MEM

static size_t tot_mem_in_use = 0;

/* Hash table of information about memory allocations */
#define RF_MH_TABLESIZE 1000

struct mh_struct {
	void   *address;
	size_t     size;
	const char *file;
	uint32_t     line;
	char    allocated;
	struct mh_struct *next;
};
static struct mh_struct *mh_table[RF_MH_TABLESIZE];
static rf_declare_mutex2(rf_debug_mem_mutex);
static int mh_table_initialized = 0;

static void memory_hash_insert(void *, size_t,  const char *, uint32_t);
static int memory_hash_remove(void *, size_t);

void
rf_record_malloc(void *p, size_t size, const char *file, uint32_t line)
{
	RF_ASSERT(size != 0);

	/* rf_lock_mutex2(rf_debug_mem_mutex); */
	memory_hash_insert(p, size, file, line);
	tot_mem_in_use += size;
	/* rf_unlock_mutex2(rf_debug_mem_mutex); */
	if ((intptr_t)p == rf_memDebugAddress) {
		printf("%s,%d: %s: Debug address allocated\n", file, line,
		    __func__);
	}
}

void
rf_unrecord_malloc(void *p, size_t sz)
{
	size_t     size;

	/* rf_lock_mutex2(rf_debug_mem_mutex); */
	size = memory_hash_remove(p, sz);
	tot_mem_in_use -= size;
	/* rf_unlock_mutex2(rf_debug_mem_mutex); */
	if ((intptr_t) p == rf_memDebugAddress) {
		/* this is really only a flag line for gdb */
		printf("%s: Found debug address\n", __func__);
	}
}

void
rf_print_unfreed(void)
{
	size_t i;
	int foundone = 0;
	struct mh_struct *p;

	for (i = 0; i < RF_MH_TABLESIZE; i++) {
		for (p = mh_table[i]; p; p = p->next) {
			if (!p->allocated)
				continue;
			if (foundone) {
				printf("\n\n:%s: There are unfreed memory"
				    " locations at program shutdown:\n",
				    __func__);
			}
			foundone = 1;
			printf("%s: @%s,%d: addr %p size %zu\n", __func__,
			    p->file, p->line, p->address, p->size);
		}
	}
	if (tot_mem_in_use) {
		printf("%s: %zu total bytes in use\n",
		    __func__, tot_mem_in_use);
	}
}
#endif /* RF_DEBUG_MEM */

#if RF_DEBUG_MEM
static void
rf_ShutdownDebugMem(void *unused)
{
	rf_destroy_mutex2(rf_debug_mem_mutex);
}
#endif

int
rf_ConfigureDebugMem(RF_ShutdownList_t **listp)
{
#if RF_DEBUG_MEM
	size_t     i;

	rf_init_mutex2(rf_debug_mem_mutex, IPL_VM);
	if (rf_memDebug) {
		for (i = 0; i < RF_MH_TABLESIZE; i++)
			mh_table[i] = NULL;
		mh_table_initialized = 1;
	}
	rf_ShutdownCreate(listp, rf_ShutdownDebugMem, NULL);
#endif
	return (0);
}

#if RF_DEBUG_MEM

#define HASHADDR(a) ((size_t)((((uintptr_t)a) >> 3) % RF_MH_TABLESIZE))

static void
memory_hash_insert(void *addr, size_t size, const char *file, uint32_t line)
{
	size_t bucket = (size_t)HASHADDR(addr);
	struct mh_struct *p;

	RF_ASSERT(mh_table_initialized);

	/* search for this address in the hash table */
	for (p = mh_table[bucket]; p && (p->address != addr); p = p->next)
		continue;
	if (!p) {
		p = RF_Malloc(sizeof(*p));
		RF_ASSERT(p);
		p->next = mh_table[bucket];
		mh_table[bucket] = p;
		p->address = addr;
		p->allocated = 0;
	}
	if (p->allocated) {
		printf("%s: @%s,%u: ERROR: Reallocated addr %p without free\n",
		    __func__, file, line, addr);
		printf("%s: last allocated @%s,%u\n",
		    __func__, p->file, p->line);
		RF_ASSERT(0);
	}
	p->size = size;
	p->line = line;
	p->file = file;
	p->allocated = 1;
}

static int
memory_hash_remove(void *addr, size_t sz)
{
	size_t bucket = HASHADDR(addr);
	struct mh_struct *p;

	RF_ASSERT(mh_table_initialized);
	for (p = mh_table[bucket]; p && (p->address != addr); p = p->next)
		continue;
	if (!p) {
		printf("%s: ERROR: Freeing never-allocated address %p\n",
		    __func__, addr);
		RF_PANIC();
	}
	if (!p->allocated) {
		printf("%s: ERROR: Freeing unallocated address %p."
		    " Last allocation @%s,%u\n",
		    __func__, addr, p->file, p->line);
		RF_PANIC();
	}
	if (sz > 0 && p->size != sz) {	/* you can suppress this error by
					 * using a negative value as the size
					 * to free */
		printf("%s: ERROR: Incorrect size (%zu should be %zu) at"
		    " free for address %p. Allocated @%s,%u\n", __func__,
		    sz, p->size, addr, p->file, p->line);
		RF_PANIC();
	}
	p->allocated = 0;
	return p->size;
}
#endif /* RF_DEBUG_MEM */


