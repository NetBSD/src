/*	$NetBSD: hash.c,v 1.9.8.1 2002/06/20 13:36:42 gehenna Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratories.
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
 *	from: @(#)hash.c	8.1 (Berkeley) 6/6/93
 */

#include <sys/param.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"

/*
 * Interned strings are kept in a hash table.  By making each string
 * unique, the program can compare strings by comparing pointers.
 */
struct hashent {
	// XXXLUKEM: a SIMPLEQ might be more appropriate
	TAILQ_ENTRY(hashent) h_next;
	const char *h_name;		/* the string */
	u_int	h_hash;			/* its hash value */
	void	*h_value;		/* other values (for name=value) */
};
struct hashtab {
	size_t	ht_size;		/* size (power of 2) */
	u_int	ht_mask;		/* == ht_size - 1 */
	u_int	ht_used;		/* number of entries used */
	u_int	ht_lim;			/* when to expand */
	TAILQ_HEAD(hashenthead, hashent) *ht_tab;
};

static struct hashtab strings;

static struct hashenthead hefreelist = TAILQ_HEAD_INITIALIZER(hefreelist);

/*
 * HASHFRACTION controls ht_lim, which in turn controls the average chain
 * length.  We allow a few entries, on average, as comparing them is usually
 * cheap (the h_hash values prevent a strcmp).
 */
#define	HASHFRACTION(sz) ((sz) * 3 / 2)

static void			ht_expand(struct hashtab *);
static void			ht_init(struct hashtab *, size_t);
static inline u_int		hash(const char *);
static inline struct hashent   *newhashent(const char *, u_int);

/*
 * Initialize a new hash table.  The size must be a power of 2.
 */
static void
ht_init(struct hashtab *ht, size_t sz)
{
	u_int n;

	ht->ht_tab = emalloc(sz * sizeof (ht->ht_tab[0]));
	ht->ht_size = sz;
	ht->ht_mask = sz - 1;
	for (n = 0; n < sz; n++)
		TAILQ_INIT(&ht->ht_tab[n]);
	ht->ht_used = 0;
	ht->ht_lim = HASHFRACTION(sz);
}

/*
 * Expand an existing hash table.
 */
static void
ht_expand(struct hashtab *ht)
{
	struct hashenthead *h, *oldh;
	struct hashent *p;
	u_int n, i;

	n = ht->ht_size * 2;
	h = emalloc(n * sizeof *h);
	for (i = 0; i < n; i++)
		TAILQ_INIT(&h[i]);
	oldh = ht->ht_tab;
	n--;
	for (i = 0; i < ht->ht_size; i++) {
		while ((p = TAILQ_FIRST(&oldh[i])) != NULL) {
			TAILQ_REMOVE(&oldh[i], p, h_next);
				// XXXLUKEM: really should be TAILQ_INSERT_TAIL
			TAILQ_INSERT_HEAD(&h[p->h_hash & n], p, h_next);
		}
	}
	free(ht->ht_tab);
	ht->ht_tab = h;
	ht->ht_mask = n;
	ht->ht_size = ++n;
	ht->ht_lim = HASHFRACTION(n);
}

/*
 * Make a new hash entry, setting its h_next to NULL.
 * If the free list is not empty, use the first entry from there,
 * otherwise allocate a new entry.
 */
static inline struct hashent *
newhashent(const char *name, u_int h)
{
	struct hashent *hp;

	if (TAILQ_EMPTY(&hefreelist))
		hp = emalloc(sizeof(*hp));
	else {
		hp = TAILQ_FIRST(&hefreelist);
		TAILQ_REMOVE(&hefreelist, hp, h_next);
	}

	hp->h_name = name;
	hp->h_hash = h;
	return (hp);
}

/*
 * Hash a string.
 */
static inline u_int
hash(const char *str)
{
	u_int h;

	for (h = 0; *str;)
		h = (h << 5) + h + *str++;
	return (h);
}

void
initintern(void)
{

	ht_init(&strings, 128);
}

/*
 * Generate a single unique copy of the given string.  We expect this
 * function to be used frequently, so it should be fast.
 */
const char *
intern(const char *s)
{
	struct hashtab *ht;
	struct hashent *hp;
	struct hashenthead *hpp;
	u_int h;
	char *p;

	ht = &strings;
	h = hash(s);
	hpp = &ht->ht_tab[h & ht->ht_mask];
	TAILQ_FOREACH(hp, hpp, h_next) {
		if (hp->h_hash == h && strcmp(hp->h_name, s) == 0)
			return (hp->h_name);
	}
	p = estrdup(s);
	hp = newhashent(p, h);
	TAILQ_INSERT_TAIL(hpp, hp, h_next);
	if (++ht->ht_used > ht->ht_lim)
		ht_expand(ht);
	return (p);
}

struct hashtab *
ht_new(void)
{
	struct hashtab *ht;

	ht = emalloc(sizeof *ht);
	ht_init(ht, 8);
	return (ht);
}

/*
 * Insert and/or replace.
 */
int
ht_insrep(struct hashtab *ht, const char *nam, void *val, int replace)
{
	struct hashent *hp;
	struct hashenthead *hpp;
	u_int h;

	h = hash(nam);
	hpp = &ht->ht_tab[h & ht->ht_mask];
	TAILQ_FOREACH(hp, hpp, h_next) {
		if (hp->h_name == nam) {
			if (replace)
				hp->h_value = val;
			return (1);
		}
	}
	hp = newhashent(nam, h);
	TAILQ_INSERT_TAIL(hpp, hp, h_next);
	hp->h_value = val;
	if (++ht->ht_used > ht->ht_lim)
		ht_expand(ht);
	return (0);
}

/*
 * Remove.
 */
int
ht_remove(struct hashtab *ht, const char *name)
{
	struct hashent *hp;
	struct hashenthead *hpp;
	u_int h;

	h = hash(name);
	hpp = &ht->ht_tab[h & ht->ht_mask];

	TAILQ_FOREACH(hp, hpp, h_next) {
		if (hp->h_name != name)
			continue;
		TAILQ_REMOVE(hpp, hp, h_next);
		TAILQ_INSERT_TAIL(&hefreelist, hp, h_next);
		ht->ht_used--;
		return (0);
	}
	return (1);
}

void *
ht_lookup(struct hashtab *ht, const char *nam)
{
	struct hashent *hp;
	struct hashenthead *hpp;
	u_int h;

	h = hash(nam);
	hpp = &ht->ht_tab[h & ht->ht_mask];
	TAILQ_FOREACH(hp, hpp, h_next)
		if (hp->h_name == nam)
			return (hp->h_value);
	return (NULL);
}

/*
 * first parameter to callback is the entry name from the hash table
 * second parameter is the value from the hash table
 * third argument is passed through from the "arg" parameter to ht_enumerate()
 */

int
ht_enumerate(struct hashtab *ht, ht_callback cbfunc, void *arg)
{
	struct hashent *hp;
	struct hashenthead *hpp;
	u_int i;
	int rval = 0;
	
	for (i = 0; i < ht->ht_size; i++) {
		hpp = &ht->ht_tab[i];
		TAILQ_FOREACH(hp, hpp, h_next)
			rval += (*cbfunc)(hp->h_name, hp->h_value, arg);
	}
	return rval;
}
