/*	$NetBSD: hash.c,v 1.38 2020/10/03 23:16:28 rillig Exp $	*/

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
MAKE_RCSID("$NetBSD: hash.c,v 1.38 2020/10/03 23:16:28 rillig Exp $");

/*
 * Forward references to local procedures that are used before they're
 * defined:
 */

static void RebuildTable(Hash_Table *);

/*
 * The following defines the ratio of # entries to # buckets
 * at which we rebuild the table to make it larger.
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

/* Sets up the hash table. */
void
Hash_InitTable(Hash_Table *t)
{
	size_t n = 16, i;
	struct Hash_Entry **hp;

	t->numEntries = 0;
	t->maxchain = 0;
	t->bucketsSize = n;
	t->bucketsMask = n - 1;
	t->buckets = hp = bmake_malloc(sizeof(*hp) * n);
	for (i = 0; i < n; i++)
		hp[i] = NULL;
}

/* Removes everything from the hash table and frees up the memory space it
 * occupied (except for the space in the Hash_Table structure). */
void
Hash_DeleteTable(Hash_Table *t)
{
	struct Hash_Entry **hp, *h, *nexth = NULL;
	int i;

	for (hp = t->buckets, i = t->bucketsSize; --i >= 0;) {
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
Hash_Entry *
Hash_FindEntry(Hash_Table *t, const char *key)
{
	Hash_Entry *e;
	unsigned h;
	int chainlen;

	if (t == NULL || t->buckets == NULL)
	    return NULL;

	h = hash(key, NULL);
	chainlen = 0;
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

void *
Hash_FindValue(Hash_Table *t, const char *key)
{
    Hash_Entry *he = Hash_FindEntry(t, key);
    return he != NULL ? he->value : NULL;
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
Hash_Entry *
Hash_CreateEntry(Hash_Table *t, const char *key, Boolean *newPtr)
{
	Hash_Entry *e;
	unsigned h;
	size_t keylen;
	int chainlen;
	struct Hash_Entry **hp;

	/*
	 * Hash the key.  As a side effect, save the length (strlen) of the
	 * key in case we need to create the entry.
	 */
	h = hash(key, &keylen);
	chainlen = 0;
#ifdef DEBUG_HASH_LOOKUP
	DEBUG4(HASH, "%s: %p h=%x key=%s\n", __func__, t, h, key);
#endif
	for (e = t->buckets[h & t->bucketsMask]; e != NULL; e = e->next) {
		chainlen++;
		if (e->namehash == h && strcmp(e->name, key) == 0) {
			if (newPtr != NULL)
				*newPtr = FALSE;
			break;
		}
	}
	if (chainlen > t->maxchain)
		t->maxchain = chainlen;
	if (e)
		return e;

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
Hash_DeleteEntry(Hash_Table *t, Hash_Entry *e)
{
	Hash_Entry **hp, *p;

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

/* Sets things up for enumerating all entries in the hash table.
 *
 * Input:
 *	t		Table to be searched.
 *	searchPtr	Area in which to keep state about search.
 *
 * Results:
 *	The return value is the address of the first entry in
 *	the hash table, or NULL if the table is empty.
 */
Hash_Entry *
Hash_EnumFirst(Hash_Table *t, Hash_Search *searchPtr)
{
	searchPtr->table = t;
	searchPtr->nextBucket = 0;
	searchPtr->entry = NULL;
	return Hash_EnumNext(searchPtr);
}

/* Returns the next entry in the hash table, or NULL if the end of the table
 * is reached.
 *
 * Input:
 *	searchPtr	Area used to keep state about search.
 */
Hash_Entry *
Hash_EnumNext(Hash_Search *searchPtr)
{
	Hash_Entry *e;
	Hash_Table *t = searchPtr->table;

	/*
	 * The entry field points to the most recently returned
	 * entry, or is NULL if we are starting up.  If not NULL, we have
	 * to start at the next one in the chain.
	 */
	e = searchPtr->entry;
	if (e != NULL)
		e = e->next;
	/*
	 * If the chain ran out, or if we are starting up, we need to
	 * find the next nonempty chain.
	 */
	while (e == NULL) {
		if (searchPtr->nextBucket >= t->bucketsSize)
			return NULL;
		e = t->buckets[searchPtr->nextBucket++];
	}
	searchPtr->entry = e;
	return e;
}

/* Makes a new hash table that is larger than the old one. The entire hash
 * table is moved, so any bucket numbers from the old table become invalid. */
static void
RebuildTable(Hash_Table *t)
{
	Hash_Entry *e, *next = NULL, **hp, **xp;
	int i, mask;
	Hash_Entry **oldhp;
	int oldsize;

	oldhp = t->buckets;
	oldsize = i = t->bucketsSize;
	i <<= 1;
	t->bucketsSize = i;
	t->bucketsMask = mask = i - 1;
	t->buckets = hp = bmake_malloc(sizeof(*hp) * i);
	while (--i >= 0)
		*hp++ = NULL;
	for (hp = oldhp, i = oldsize; --i >= 0;) {
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

void
Hash_ForEach(Hash_Table *t, void (*action)(void *, void *), void *data)
{
	Hash_Search search;
	Hash_Entry *e;

	for (e = Hash_EnumFirst(t, &search);
	     e != NULL;
	     e = Hash_EnumNext(&search))
		action(Hash_GetValue(e), data);
}

void
Hash_DebugStats(Hash_Table *t, const char *name)
{
    DEBUG4(HASH, "Hash_Table %s: size=%d numEntries=%d maxchain=%d\n",
	   name, t->bucketsSize, t->numEntries, t->maxchain);
}
