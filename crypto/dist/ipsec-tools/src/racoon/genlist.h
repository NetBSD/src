/*	$NetBSD: genlist.h,v 1.4 2006/09/09 16:22:09 manu Exp $	*/

/* Id: genlist.h,v 1.2 2004/07/12 20:43:50 ludvigm Exp */

/*
 * Copyright (C) 2004 SuSE Linux AG, Nuernberg, Germany.
 * Contributed by: Michal Ludvig <mludvig@suse.cz>, SUSE Labs
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _GENLIST_H
#define _GENLIST_H

#include <sys/queue.h>

/* See the bottom of genlist.c for example use. */

/* This declares 'struct genlist' */
TAILQ_HEAD(genlist, genlist_entry);

/* This is where the data are actually stored. */
struct genlist_entry {
	void *data;
	TAILQ_ENTRY(genlist_entry) chain;
};

/* This function returns an initialized list head. */
struct genlist *genlist_init (void);

/* Insert an entry at the beginning/end og the list. */
struct genlist_entry *genlist_insert (struct genlist *head, void *data);
struct genlist_entry *genlist_append (struct genlist *head, void *data);

/* Create a function with this prototype for use with genlist_foreach().
 * See genlist_foreach() description below for details. */
typedef void *(genlist_func_t)(void *entry, void *arg);

/* Traverse the list and call 'func' for each entry.  As long as func() returns
 * NULL the list traversal continues, once it returns non-NULL (usually the
 * 'entry' arg), the list traversal exits and the return value is returned
 * further from genlist_foreach(). Optional 'arg' may be passed to func(), e.g.
 * for some lookup purposes, etc. */
void *genlist_foreach (struct genlist *head, genlist_func_t func, void *arg);

/* Get first entry in list if head is not NULL, otherwise get next
 * entry based on saved position in list from previous call as stored in buf.
 * If buf is NULL no position is saved */
void *genlist_next (struct genlist *head, struct genlist_entry **buf);

/* Create a function with this prototype for use with genlist_free()
 * to free any storage associated with genlist_entry.data */
typedef void (genlist_freedata_t)(void *entry);

/* Free all storage associated with list at head using func to free any
 * alloc()d data in data field of genlist_entry */
void genlist_free (struct genlist *head, genlist_freedata_t func);

#endif /* _GENLIST_H */
