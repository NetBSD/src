/*
 * Copyright (c) 1993 Christian E. Hopps
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Christian E. Hopps.
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
 */
#if ! defined (_CC_CHIPMEM_H)
#define _CC_CHIPMEM_H

#include "cc_types.h"
#include "dlists.h"

typedef struct mem_node {
    dll_node_t node;				  /* allways set. */
    dll_node_t free;				  /* only set when nodes are available */
    u_long   size;				  /* indicates size of memory following node. */
} mem_node_t;

typedef struct mem_list {
    dll_list_t free_list;
    dll_list_t node_list;
    u_long   free_nodes;
    u_long   alloc_nodes;
    u_long   total;				  /* total free. */
    u_long   size;				  /* size of it all. */
    u_byte  *memory;				  /* all the memory. */
} mem_list_t;

#define CM_BLOCKSIZE 0x4
#define CM_BLOCKMASK (~(CM_BLOCKSIZE - 1))

#define MNODE_FROM_FREE(n) ((mem_node_t *)(((u_long)n) - offsetof (mem_node_t, free)))
#define MNODES_MEM(mn) ((u_byte *)(&mn[1]))

#if ! defined (AMIGA_TEST)
extern caddr_t CHIPMEMADDR;
#define PREP_DMA_MEM(mem) (void *)((caddr_t)mem - CHIPMEMADDR)
#else
#define PREP_DMA_MEM(mem) (mem)
#endif

/* allocate chip memory.  returns NULL if not enough available. */
void * alloc_chipmem (u_long size);

/* the memory to free or NULL. */
void free_chipmem (void *m);

/* returns total available or returns largest if ``largest'' not 0 */
u_long avail_chipmem (int largest);

/* returns the size of the given block of memory. */
u_long sizeof_chipmem (void *m);

void cc_init_chipmem (void);

#if defined (AMIGA_TEST)
void cc_deinit_chipmem (void);
#endif

#if ! defined (NULL)
#define NULL ((void *)0U)
#endif

#endif /* _CC_CHIPMEM_H */
