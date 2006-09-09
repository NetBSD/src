/*	$NetBSD: genlist.c,v 1.4 2006/09/09 16:22:09 manu Exp $	*/

/* Id: genlist.c,v 1.2 2004/07/12 20:43:50 ludvigm Exp */

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

#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>

#include "genlist.h"

struct genlist *
genlist_init (void)
{
	struct genlist *new = calloc(sizeof(struct genlist), 1);
	TAILQ_INIT(new);
	return new;
}

struct genlist_entry *
genlist_insert (struct genlist *head, void *data)
{
	struct genlist_entry *entry = calloc(sizeof(struct genlist_entry), 1);
	entry->data = data;
	TAILQ_INSERT_HEAD(head, entry, chain);
	return entry;
}

struct genlist_entry *
genlist_append (struct genlist *head, void *data)
{
	struct genlist_entry *entry = calloc(sizeof(struct genlist_entry), 1);
	entry->data = data;
	TAILQ_INSERT_TAIL(head, entry, chain);
	return entry;
}

void *
genlist_foreach (struct genlist *head, genlist_func_t func, void *arg)
{
	struct genlist_entry *p;
	void *ret = NULL;
	TAILQ_FOREACH(p, head, chain) {
		ret = (*func)(p->data, arg);
		if (ret)
			break;
	}

	return ret;
}

void *
genlist_next (struct genlist *head, struct genlist_entry **buf)
{
	struct genlist_entry *p;

	if (head)
		p = TAILQ_FIRST(head);
	else
		p = (buf && *buf) ? TAILQ_NEXT(*buf, chain) : NULL;
	if (buf)
		*buf = p;
	return (p ? p->data : NULL);
}

void
genlist_free (struct genlist *head, genlist_freedata_t func)
{
	struct genlist_entry *p;

	while ((p = TAILQ_LAST(head, genlist)) != NULL) {
		TAILQ_REMOVE(head, p, chain);
		if (func)
			func(p->data);
		free(p);
	}
	free(head);
}


#if 0
/* Here comes the example... */
struct conf {
	struct genlist	*l1, *l2;
};

void *
print_entry(void *entry, void *arg)
{
	if (!entry)
		return NULL;
	printf("%s\n", (char *)entry);
	return NULL;
}

void
dump_list(struct genlist *head)
{
	genlist_foreach(head, print_entry, NULL);
}

void
free_data(void *data)
{
	printf ("removing %s\n", (char *)data);
}

int main()
{
	struct conf *cf;
	char *cp;
	struct genlist_entry *gpb;

	cf = calloc(sizeof(struct conf), 1);
	cf->l1 = genlist_init();
	cf->l2 = genlist_init();
	
	genlist_insert(cf->l1, "Ahoj");
	genlist_insert(cf->l1, "Cau");
	genlist_insert(cf->l1, "Nazdar");
	genlist_insert(cf->l1, "Te buch");

	genlist_append(cf->l2, "Curak");
	genlist_append(cf->l2, "Kozy");
	genlist_append(cf->l2, "Pica");
	genlist_append(cf->l2, "Prdel");

	printf("List 2\n");
	dump_list(cf->l2);
	printf("\nList 1\n");
	dump_list(cf->l1);

	printf("\nList 2 - using genlist_next()\n");
	for (cp = genlist_next (cf->l2, &gpb); cp; cp = genlist_next (0, &gpb))
	    printf("%s\n", cp);

	printf("\nFreeing List 1\n");
	/* the data here isn't actually alloc'd so we would really call
	 * genlist_free (cf->l1, 0);    but to illustrate the idea */
	genlist_free (cf->l1, free_data);
	cf->l1 = 0;

	return 0;
}
#endif
