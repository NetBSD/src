/*	$NetBSD: hash.c,v 1.24 2022/05/20 21:18:55 rillig Exp $	*/

/*
 * Copyright (c) 1994, 1995 Jochen Pohl
 * All Rights Reserved.
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
 *      This product includes software developed by Jochen Pohl for
 *	The NetBSD Project.
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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if defined(__RCSID)
__RCSID("$NetBSD: hash.c,v 1.24 2022/05/20 21:18:55 rillig Exp $");
#endif

/*
 * XXX Really need a generalized hash table package
 */

#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "lint2.h"

/* pointer to hash table, initialized in inithash() */
static	hte_t	**htab;

/*
 * Initialize hash table.
 */
hte_t **
htab_new(void)
{
	return xcalloc(HSHSIZ2, sizeof(*htab_new()));
}

/*
 * Compute hash value from a string.
 */
static unsigned int
hash(const char *s)
{
	unsigned int v;
	const char *p;

	v = 0;
	for (p = s; *p != '\0'; p++) {
		v = (v << 4) + (unsigned char)*p;
		v ^= v >> 28;
	}
	return v % HSHSIZ2;
}

/*
 * Look for a hash table entry. If no hash table entry for the
 * given name exists and mknew is set, create a new one.
 */
hte_t *
_hsearch(hte_t **table, const char *s, bool mknew)
{
	unsigned int h;
	hte_t	*hte;

	if (table == NULL)
		table = htab;

	h = hash(s);
	for (hte = table[h]; hte != NULL; hte = hte->h_link) {
		if (strcmp(hte->h_name, s) == 0)
			break;
	}

	if (hte != NULL || !mknew)
		return hte;

	/* create a new hte */
	hte = xmalloc(sizeof(*hte));
	hte->h_name = xstrdup(s);
	hte->h_used = false;
	hte->h_def = false;
	hte->h_static = false;
	hte->h_syms = NULL;
	hte->h_lsym = &hte->h_syms;
	hte->h_calls = NULL;
	hte->h_lcall = &hte->h_calls;
	hte->h_usyms = NULL;
	hte->h_lusym = &hte->h_usyms;
	hte->h_link = table[h];
	hte->h_hte = NULL;
	table[h] = hte;

	return hte;
}

struct hte_list {
	hte_t **items;
	size_t len;
	size_t cap;
};

static void
hte_list_add(struct hte_list *list, hte_t *item)
{
	if (list->len >= list->cap) {
		list->cap = list->cap == 0 ? 1024 : 2 * list->cap;
		list->items = xrealloc(list->items,
		    sizeof(list->items[0]) * list->cap);
	}
	list->items[list->len++] = item;
}

static int
hte_by_name(const void *va, const void *vb)
{
	const hte_t *a = *((const hte_t *const *)va);
	const hte_t *b = *((const hte_t *const *)vb);

	return strcmp(a->h_name, b->h_name);
}

void
symtab_init(void)
{
	htab = htab_new();
}

/*
 * Call the action for each name in the hash table.
 */
void
symtab_forall(void (*action)(hte_t *))
{
	int	i;
	hte_t	*hte;
	hte_t	**table = htab;

	for (i = 0; i < HSHSIZ2; i++) {
		for (hte = table[i]; hte != NULL; hte = hte->h_link)
			action(hte);
	}
}

/* Run the action for each name in the symbol table, in alphabetic order. */
void
symtab_forall_sorted(void (*action)(hte_t *))
{
	hte_t *hte;
	struct hte_list sorted = { NULL, 0, 0 };
	size_t i;
	hte_t **table = htab;

	for (i = 0; i < HSHSIZ2; i++)
		for (hte = table[i]; hte != NULL; hte = hte->h_link)
			hte_list_add(&sorted, hte);

	qsort(sorted.items, sorted.len, sizeof(sorted.items[0]), hte_by_name);

	for (i = 0; i < sorted.len; i++)
		action(sorted.items[i]);

	free(sorted.items);
}

/*
 * Free all contents of the hash table that this module allocated.
 */
void
_destroyhash(hte_t **table)
{
	int	i;
	hte_t	*hte, *nexthte;

	if (table == NULL)
		err(1, "_destroyhash called on main hash table");

	for (i = 0; i < HSHSIZ2; i++) {
		for (hte = table[i]; hte != NULL; hte = nexthte) {
			free(__UNCONST(hte->h_name));
			nexthte = hte->h_link;
			free(hte);
		}
	}
	free(table);
}
