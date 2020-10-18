/*	$NetBSD: hash.c,v 1.46 2020/10/18 12:36:43 rillig Exp $	*/

/*
 * Copyright (c) 1988, 1989, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Adam de Boor.
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
 */

/*
 * Copyright (c) 1988, 1989 by Adam de Boor
 * Copyright (c) 1989 by Berkeley Softworks
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Adam de Boor.
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

/*
 * This module contains routines to manipulate a hash table.
 * See hash.h for a definition of the structure of the hash
 * table.  Hash tables grow automatically as the amount of
 * information increases.
 */

#include "make.h"

/*	"@(#)hash.c	8.1 (Berkeley) 6/6/93"	*/
MAKE_RCSID("$NetBSD: hash.c,v 1.46 2020/10/18 12:36:43 rillig Exp $");

/*
 * The ratio of # entries to # buckets at which we rebuild the table to
 * make it larger.
 */
#define rebuildLimit 3

/* This hash function matches Gosling's emacs. */
static unsigned int
hash(const char *key, size_t *out_keylen)
{
	unsigned h = 0;
	const char *p = key;
	while (*p != '\0')
		h = (h << 5) - h + (unsigned char)*p++;
	if (out_keylen != NULL)
		*out_keylen = (size_t)(p - key);
	return h;
}

static HashEntry *
HashTable_Find(HashTable *t, unsigned int h, const char *key)
{
	HashEntry *e;
	unsigned int chainlen = 0;

#ifdef DEBUG_HASH_LOOKUP
	DEBUG4(HASH, "%s: %p h=%x key=%s\n", __func__, t, h, key);
#endif

	for (e = t->buckets[h & t->bucketsMask]; e != NULL; e = e->next) {
		chainlen++;
		if (e->namehash == h && strcmp(e->name, key) == 0)
			break;
	}

	if (chainlen > t->maxchain)
		t->maxchain = chainlen;

	return e;
}

/* Sets up the hash table. */
void
Hash_InitTable(HashTable *t)
{
	unsigned int n = 16, i;
	HashEntry **hp;

	t->numEntries = 0;
	t->maxchain = 0;
	t->bucketsSize = n;
	t->bucketsMask = n - 1;
	t->buckets = hp = bmake_malloc(sizeof(*hp) * n);
	for (i = 0; i < n; i++)
		hp[i] = NULL;
}

/* Removes everything from the hash table and frees up the memory space it
 * occupied (except for the space in the HashTable structure). */
void
Hash_DeleteTable(HashTable *t)
{
	HashEntry **hp, *h, *nexth = NULL;
	int i;

	for (hp = t->buckets, i = (int)t->bucketsSize; --i >= 0;) {
		for (h = *hp++; h != NULL; h = nexth) {
			nexth = h->next;
			free(h);
		}
	}
	free(t->buckets);

	/*
	 * Set up the hash table to cause memory faults on any future access
	 * attempts until re-initialization.
	 */
	t->buckets = NULL;
}

/* Searches the hash table for an entry corresponding to the key.
 *
 * Input:
 *	t		Hash table to search.
 *	key		A hash key.
 *
 * Results:
 *	Returns a pointer to the entry for key, or NULL if the table contains
 *	no entry for the key.
 */
HashEntry *
Hash_FindEntry(HashTable *t, const char *key)
{
	unsigned int h = hash(key, NULL);
	return HashTable_Find(t, h, key);
}

void *
Hash_FindValue(HashTable *t, const char *key)
{
	HashEntry *he = Hash_FindEntry(t, key);
	return he != NULL ? he->value : NULL;
}

/* Makes a new hash table that is larger than the old one. The entire hash
 * table is moved, so any bucket numbers from the old table become invalid. */
static void
RebuildTable(HashTable *t)
{
	HashEntry *e, *next = NULL, **hp, **xp;
	int i;
	unsigned int mask, oldsize, newsize;
	HashEntry **oldhp;

	oldhp = t->buckets;
	oldsize = t->bucketsSize;
	newsize = oldsize << 1;
	t->bucketsSize = (unsigned int)newsize;
	t->bucketsMask = mask = newsize - 1;
	t->buckets = hp = bmake_malloc(sizeof(*hp) * newsize);
	i = (int)newsize;
	while (--i >= 0)
		*hp++ = NULL;
	for (hp = oldhp, i = (int)oldsize; --i >= 0;) {
		for (e = *hp++; e != NULL; e = next) {
			next = e->next;
			xp = &t->buckets[e->namehash & mask];
			e->next = *xp;
			*xp = e;
		}
	}
	free(oldhp);
	DEBUG5(HASH, "%s: %p size=%d entries=%d maxchain=%d\n",
	       __func__, t, t->bucketsSize, t->numEntries, t->maxchain);
	t->maxchain = 0;
}

/* Searches the hash table for an entry corresponding to the key.
 * If no entry is found, then one is created.
 *
 * Input:
 *	t		Hash table to search.
 *	key		A hash key.
 *	newPtr		Filled with TRUE if new entry created,
 *			FALSE otherwise.
 */
HashEntry *
Hash_CreateEntry(HashTable *t, const char *key, Boolean *newPtr)
{
	HashEntry *e;
	unsigned h;
	size_t keylen;
	HashEntry **hp;

	h = hash(key, &keylen);
	e = HashTable_Find(t, h, key);
	if (e) {
		if (newPtr != NULL)
			*newPtr = FALSE;
		return e;
	}

	/*
	 * The desired entry isn't there.  Before allocating a new entry,
	 * expand the table if necessary (and this changes the resulting
	 * bucket chain).
	 */
	if (t->numEntries >= rebuildLimit * t->bucketsSize)
		RebuildTable(t);

	e = bmake_malloc(sizeof(*e) + keylen);
	hp = &t->buckets[h & t->bucketsMask];
	e->next = *hp;
	*hp = e;
	Hash_SetValue(e, NULL);
	e->namehash = h;
	memcpy(e->name, key, keylen + 1);
	t->numEntries++;

	if (newPtr != NULL)
		*newPtr = TRUE;
	return e;
}

/* Delete the given hash table entry and free memory associated with it. */
void
Hash_DeleteEntry(HashTable *t, HashEntry *e)
{
	HashEntry **hp, *p;

	for (hp = &t->buckets[e->namehash & t->bucketsMask];
	     (p = *hp) != NULL; hp = &p->next) {
		if (p == e) {
			*hp = p->next;
			free(p);
			t->numEntries--;
			return;
		}
	}
	abort();
}

/* Set things up for iterating over all entries in the hash table. */
void
HashIter_Init(HashIter *hi, HashTable *t)
{
	hi->table = t;
	hi->nextBucket = 0;
	hi->entry = NULL;
}

/* Return the next entry in the hash table, or NULL if the end of the table
 * is reached. */
HashEntry *
HashIter_Next(HashIter *hi)
{
	HashEntry *e;
	HashTable *t = hi->table;

	/*
	 * The entry field points to the most recently returned
	 * entry, or is NULL if we are starting up.  If not NULL, we have
	 * to start at the next one in the chain.
	 */
	e = hi->entry;
	if (e != NULL)
		e = e->next;
	/*
	 * If the chain ran out, or if we are starting up, we need to
	 * find the next nonempty chain.
	 */
	while (e == NULL) {
		if (hi->nextBucket >= t->bucketsSize)
			return NULL;
		e = t->buckets[hi->nextBucket++];
	}
	hi->entry = e;
	return e;
}

void
Hash_DebugStats(HashTable *t, const char *name)
{
	DEBUG4(HASH, "HashTable %s: size=%u numEntries=%u maxchain=%u\n",
	       name, t->bucketsSize, t->numEntries, t->maxchain);
}
