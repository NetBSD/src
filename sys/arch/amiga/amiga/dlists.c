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

#include "dlists.h"

void
dinit_list (dll_list_t *list)
{
    list->head = (dll_node_t *)&list->tail;
    list->tail = NULL;
    list->tailprev = (dll_node_t *)&list->head;
}

void
dadd_head (dll_list_t *list, dll_node_t *node)
{
    dll_node_t *next_node;

    next_node = list->head;
    list->head = node;

    /* new head node */
    node->next = next_node;
    node->prev = (dll_node_t *)&list->head;

    /* old head node */
    next_node->prev = node;
}

void
dadd_tail (dll_list_t *list, dll_node_t *node)
{
    dll_node_t *prev_node;
    
    prev_node = list->tailprev;
    list->tailprev = node;
    
    /* new tail node */
    node->next = (dll_node_t *)&list->tail;
    node->prev = prev_node;
    
    /* old tail node */
    prev_node->next = node;
    return;
}

dll_node_t *
dremove_head (dll_list_t *list)
{
    dll_node_t *next_node, *head_node;
    
    if(IS_DLIST_EMPTY(list)) {
	return(NULL);
    }

    head_node = list->head;
    next_node = head_node->next;

    next_node->prev = (dll_node_t *)&list->head;
    list->head = next_node;
    
    return(head_node);
}

dll_node_t *
dremove_tail (dll_list_t *list)
{
    dll_node_t *rem_node, *new_node;
    
    if( IS_DLIST_EMPTY(list) ) {
	return(NULL);
    }
    
    rem_node = list->tailprev;
    new_node = rem_node->prev;
    
    list->tailprev = new_node;
    new_node->next = (dll_node_t *)&list->tail;
    
    return(rem_node);
}

void
dremove (dll_node_t *node)
{
    dll_node_t *prev, *next;
    
    next = node->next;
    prev = node->prev;
    
    prev->next = next;
    next->prev = prev;
}

void
dappend (dll_node_t *inlist_node, dll_node_t *append_node)
{
    dadd_head((dll_list_t *)inlist_node, append_node);
}

void
dinsert (dll_node_t *inlist_node, dll_node_t *node)
{
    dadd_head((dll_list_t *)inlist_node->prev, node);
}

