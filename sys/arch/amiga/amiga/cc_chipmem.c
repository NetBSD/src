/*
 * Copyright (c) 1994 Christian E. Hopps
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
 *      This product includes software developed by Christian E. Hopps.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 *
 *	$Id: cc_chipmem.c,v 1.4 1994/02/01 11:49:59 chopps Exp $
 */
#include "types.h"
#include "cc_chipmem.h"
#include "param.h"

mem_list_t chip;

static void *
allocate (mem_list_t *m, u_long size)
{
    int s = splhigh();				  /* disable everything. */
    void *mem = NULL;
    if (size) {
	dll_node_t *n;

	if (size & ~(CM_BLOCKMASK)) {
	    size = (size & CM_BLOCKMASK) + CM_BLOCKSIZE;
	}
	
	/* walk list of available nodes. */
	for (n = chip.free_list.head; n->next; n = n->next) {
	    mem_node_t *mn = MNODE_FROM_FREE (n);
	    if (size == mn->size) {
		dremove (n);			  /* remove from avail list. */
		n->next = NULL;		  /* mark as removed. */
		n->prev = NULL;		  /* mark as removed. */
		m->free_nodes--;
		m->alloc_nodes++;
		m->total -= mn->size;
		mem = (void *)&mn[1];
		break;
	    } else if (size < mn->size) {
		if ((mn->size - size) <= sizeof (mem_node_t)) {
		    /* our allocation wouldnot leave room for a new node in between. */
		    size = mn->size;		  /* increase size. */
		    dremove (n);		  /* remove from avail list. */
		    n->next = NULL;		  /* mark as removed. */
		    n->prev = NULL;		  /* mark as removed. */
		    m->free_nodes--;
		    m->alloc_nodes++;
		    m->total -= mn->size;
		    mem = (void *)&mn[1];
		    break;
		} else {
		    /* split the node's memory. */
#if 0
		    mem_node_t *new = (mem_node_t *)(MNODES_MEM(mn) + size);
		    new->size = mn->size - (size + sizeof (mem_node_t));
		    mn->size = size;		  /* this node is now exactly size big. */
		    
		    dappend (&mn->node, &new->node); /* add the new node to big list */
		    dappend (&mn->free, &new->free); /* add the new node to free list */
#else
		    mem_node_t *new = mn;
		    new->size -= size + sizeof (mem_node_t);
		    mn = (mem_node_t *)(MNODES_MEM(new) + new->size);
		    mn->size = size;		  /* this node is now exactly size big. */
		    n = &mn->free;

		    dappend (&new->node, &mn->node); /* add the new node to big list */
		    dappend (&new->free, &mn->free); /* add the new node to free list */
#endif

		    dremove (&mn->free);	  /* remove the old node from free list. */
		    n->next = NULL;		  /* mark as removed. */
		    n->prev = NULL;		  /* mark as removed. */
		    
		    m->alloc_nodes++;		  /* increase the number of allocated nodes. */
		    m->total -= (size + sizeof (mem_node_t));
		    mem = (void *)&mn[1];
		    break;
		}
	    }
	}
    }
    splx (s);
    return (mem);
}

static void
deallocate (mem_list_t *m, void *mem)
{
    int s = splhigh();				  /* disable everything. */
    mem_node_t *mn = mem;				  /* point to the memory. */
    mem_node_t *next, *prev;
    int      added = 0;				  /* flag */
    mn--;					  /* now points to the node struct. */

    /* check ahead of us. */
    next = (mem_node_t *)mn->node.next;
    prev = (mem_node_t *)mn->node.prev;
    if (next->node.next && next->free.next) {
	/* if next is: a valid node and a free node. ==> merge */
	dinsert (&next->free, &mn->free);	  /* add onto free list */
	m->free_nodes++;
	
	dremove (&next->node);			  /* remove next from main list. */
	dremove (&next->free);			  /* remove next from free list. */
	m->free_nodes--;
	m->alloc_nodes--;
	m->total += mn->size + sizeof (mem_node_t);  /* add our helpings to the pool. */
	added = 1;
	mn->size += next->size + sizeof (mem_node_t); /* adjust to new size. */
    }
    if (prev->node.prev && prev->free.prev) {
	/* if prev is: a valid node and a free node. ==> merge */
	if (mn->free.next) {			  /* if we are on free list. */
	    dremove (&mn->free);		  /* remove us from free list. */
	    m->free_nodes--;
	}
	dremove (&mn->node);			  /* remove us from main list. */
	m->alloc_nodes--;
	prev->size += mn->size + sizeof (mem_node_t);
	if (added) {
	    m->total += sizeof (mem_node_t);
	} else {
	    m->total += mn->size + sizeof (mem_node_t);  /* add our helpings to the pool. */
	}	    
    } else if (NULL == mn->free.next) {
	/* we still are not on free list and we need to be. */
	while (next->node.next && prev->node.prev) {
	    if (next->free.next) {
		dinsert (&next->free, &mn->free);
		m->free_nodes++;
		    break;
	    }
	    if (prev->free.prev) {
		dappend (&prev->free, &mn->free);
		m->free_nodes++;
		break;
	    }
	    prev = (mem_node_t *)prev->node.prev;
	    next = (mem_node_t *)next->node.next;
	}
	if (NULL == mn->free.next) {
	    if (NULL == next->node.next) {
		/* we are not on list so we can add ourselves to the tail. (we walked to it.) */
		dadd_tail (&m->free_list, &mn->free);
	    } else {
		dadd_head (&m->free_list, &mn->free);
	    }
	    m->free_nodes++;
	}
	m->total += mn->size;		  /* add our helpings to the pool. */
    }
    splx (s);
}

u_long
sizeof_chipmem (void *m)
{
    if (m) {
	mem_node_t *mn = m;
	mn--;
	return (mn->size);
    }
    return (0);
}

void *
alloc_chipmem (u_long size)
{
    u_long *mem;

    return (allocate (&chip, size));
}

void
free_chipmem (void *m)
{
    if (m) {
	deallocate (&chip, m);
    }
}

u_long
avail_chipmem (int largest)
{
    u_long val = 0;
    if (largest) {
	int s = splhigh ();
	dll_node_t *n;

	for (n = chip.free_list.head; n->next; n = n->next) {
	    mem_node_t *mn = MNODE_FROM_FREE (n);
	    if (mn->size > val) {
		val = mn->size;
	    }
	}
	splx (s);
    } else {
	val = chip.total;
    }
    return (val);
}	      



#if ! defined (AMIGA_TEST)
void
cc_init_chipmem (void)
{
    int s = splhigh ();
    mem_node_t *mem;
    extern u_byte *chipmem_end, *chipmem_start;

    chip.size = chipmem_end - (chipmem_start+NBPG);
    chip.memory = (u_byte *)chipmem_steal (chip.size);

    chip.free_nodes = 1;
    chip.alloc_nodes = 0;
    chip.total = chip.size - sizeof (mem_node_t);
    
    mem = (mem_node_t *)chip.memory;
    mem->size = chip.total;

    dinit_list (&chip.node_list);
    dinit_list (&chip.free_list);
    
    dadd_head (&chip.node_list, &mem->node);
    dadd_head (&chip.free_list, &mem->free);
    splx (s);
}

#else /* AMIGA_TEST */
#include <exec/memory.h>
#include <inline/exec.h>

void
cc_init_chipmem (void)
{
    mem_node_t *mem;
    extern u_byte *chipmem_end, *chipmem_start;

    chip.size = 0x64000;			  /* allocate 400k */
    chip.memory = AllocMem (chip.size, MEMF_CHIP);
    if (!chip.memory) {
	panic ("no chip mem");
    }
    chip.free_nodes = 1;
    chip.alloc_nodes = 0;
    chip.total = chip.size - sizeof (mem_node_t);
    
    mem = (mem_node_t *)chip.memory;
    mem->size = chip.total;

    dinit_list (&chip.node_list);
    dinit_list (&chip.free_list);
    
    dadd_head (&chip.node_list, &mem->node);
    dadd_head (&chip.free_list, &mem->free);
}

void
cc_deinit_chipmem (void)
{
    FreeMem (chip.memory, chip.size);
}
#endif /* AMIGA_TEST */
