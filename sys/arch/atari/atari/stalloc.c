/*	$NetBSD: stalloc.c,v 1.20 2023/01/24 07:57:20 mlelstv Exp $	*/

/*
 * Copyright (c) 1995 Leo Weppelman (Atari modifications)
 * Copyright (c) 1994 Christian E. Hopps (allocator stuff)
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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: stalloc.c,v 1.20 2023/01/24 07:57:20 mlelstv Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/queue.h>

#include <atari/atari/stalloc.h>

/*
 * St-mem allocator.
 */
/*
 * From atari_init.c
 */
extern u_long st_pool_size, st_pool_virt, st_pool_phys;

#define	PHYS_ADDR(virt)	((u_long)(virt) - st_pool_virt + st_pool_phys)

static TAILQ_HEAD(stlist, mem_node) st_list;
static TAILQ_HEAD(freelist, mem_node) free_list;
u_long   stmem_total;		/* total free.		*/

void
init_stmem(void)
{
	int s;
	struct mem_node *mem;

	s = splhigh();
	stmem_total = st_pool_size - sizeof(*mem);

	mem = (struct mem_node *)st_pool_virt;
	mem->size = st_pool_size - sizeof(*mem);

	TAILQ_INIT(&st_list);
	TAILQ_INIT(&free_list);

	TAILQ_INSERT_HEAD(&st_list, mem, link);
	TAILQ_INSERT_HEAD(&free_list, mem, free_link);
	splx(s);
}

void *
alloc_stmem(u_long size, void **phys_addr)
{
	struct mem_node *mn, *new, *bfit;
	int s;

	if (size == 0)
		return NULL;

	s = splhigh();

	if ((size & ~(ST_BLOCKMASK)) != 0)
		size = (size & ST_BLOCKMASK) + ST_BLOCKSIZE;

	/*
	 * walk list of available nodes, finding the best-fit.
	 */
	bfit = NULL;
	TAILQ_FOREACH(mn, &free_list, free_link) {
		if (size <= mn->size) {
			if ((bfit != NULL) &&
			    (bfit->size < mn->size))
				continue;
			bfit = mn;
		}
	}
	if (bfit != NULL)
		mn = bfit;
	if (mn == NULL) {
		printf("St-mem pool exhausted, binpatch 'st_pool_size'"
			"to get more\n");
		splx(s);
		return NULL;
	}

	if ((mn->size - size) <= sizeof(*mn)) {
		/*
		 * our allocation would not leave room
		 * for a new node in between.
		 */
		TAILQ_REMOVE(&free_list, mn, free_link);
		mn->type = MNODE_USED;
		size = mn->size;	 /* increase size. (or same) */
		stmem_total -= mn->size;
		splx(s);
		*phys_addr = (void*)PHYS_ADDR(&mn[1]);
		return (void *)&mn[1];
	}

	/*
	 * split the node's memory.
	 */
	new = mn;
	new->size -= size + sizeof(*mn);
	mn = (struct mem_node *)(MNODES_MEM(new) + new->size);
	mn->size = size;

	/*
	 * add split node to node list
	 * and mark as not on free list
	 */
	TAILQ_INSERT_AFTER(&st_list, new, mn, link);
	mn->type = MNODE_USED;

	stmem_total -= size + sizeof(*mn);
	splx(s);
	*phys_addr = (void*)PHYS_ADDR(&mn[1]);
	return (void *)&mn[1];
}

void
free_stmem(void *mem)
{
	struct mem_node *mn, *next, *prev;
	int s;

	if (mem == NULL)
		return;

	s = splhigh();
	mn = (struct mem_node *)mem - 1;
	next = TAILQ_NEXT(mn, link);
	prev = TAILQ_PREV(mn, stlist, link);

	/*
	 * check ahead of us.
	 */
	if (next != NULL && next->type == MNODE_FREE) {
		/*
		 * if next is: a valid node and a free node. ==> merge
		 */
		TAILQ_INSERT_BEFORE(next, mn, free_link);
		mn->type = MNODE_FREE;
		TAILQ_REMOVE(&st_list, next, link);
		TAILQ_REMOVE(&free_list, next, free_link);
		stmem_total += mn->size + sizeof(*mn);
		mn->size += next->size + sizeof(*mn);
	}
	if (prev != NULL && prev->type == MNODE_FREE) {
		/*
		 * if prev is: a valid node and a free node. ==> merge
		 */
		if (mn->type != MNODE_FREE)
			stmem_total += mn->size + sizeof(*mn);
		else {
			/* already on free list */
			TAILQ_REMOVE(&free_list, mn, free_link);
			mn->type = MNODE_USED;
			stmem_total += sizeof(*mn);
		}
		TAILQ_REMOVE(&st_list, mn, link);
		prev->size += mn->size + sizeof(*mn);
	} else if (mn->type != MNODE_FREE) {
		/*
		 * we still are not on free list and we need to be.
		 * <-- | -->
		 */
		while (next != NULL && prev != NULL) {
			if (next->type == MNODE_FREE) {
				TAILQ_INSERT_BEFORE(next, mn, free_link);
				mn->type = MNODE_FREE;
				break;
			}
			if (prev->type == MNODE_FREE) {
				TAILQ_INSERT_AFTER(&free_list, prev, mn,
				    free_link);
				mn->type = MNODE_FREE;
				break;
			}
			prev = TAILQ_PREV(prev, stlist, link);
			next = TAILQ_NEXT(next, link);
		}
		if (mn->type != MNODE_FREE) {
			if (next == NULL) {
				/*
				 * we are not on list so we can add
				 * ourselves to the tail. (we walked to it.)
				 */
				TAILQ_INSERT_TAIL(&free_list,mn,free_link);
			} else {
				TAILQ_INSERT_HEAD(&free_list,mn,free_link);
			}
			mn->type = MNODE_FREE;
		}
		stmem_total += mn->size;/* add our helpings to the pool. */
	}
	splx(s);
}
