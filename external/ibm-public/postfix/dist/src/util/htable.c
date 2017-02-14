/*	$NetBSD: htable.c,v 1.2 2017/02/14 01:16:49 christos Exp $	*/

/*++
/* NAME
/*	htable 3
/* SUMMARY
/*	hash table manager
/* SYNOPSIS
/*	#include <htable.h>
/*
/*	typedef	struct {
/* .in +4
/*		char	*key;
/*		void	*value;
/*		/* private fields... */
/* .in -4
/*	} HTABLE_INFO;
/*
/*	HTABLE	*htable_create(size)
/*	int	size;
/*
/*	HTABLE_INFO *htable_enter(table, key, value)
/*	HTABLE	*table;
/*	const char *key;
/*	void	*value;
/*
/*	char	*htable_find(table, key)
/*	HTABLE	*table;
/*	const char *key;
/*
/*	HTABLE_INFO *htable_locate(table, key)
/*	HTABLE	*table;
/*	const char *key;
/*
/*	void	htable_delete(table, key, free_fn)
/*	HTABLE	*table;
/*	const char *key;
/*	void	(*free_fn)(void *);
/*
/*	void	htable_free(table, free_fn)
/*	HTABLE	*table;
/*	void	(*free_fn)(void *);
/*
/*	void	htable_walk(table, action, ptr)
/*	HTABLE	*table;
/*	void	(*action)(HTABLE_INFO *, void *ptr);
/*	void	*ptr;
/*
/*	HTABLE_INFO **htable_list(table)
/*	HTABLE	*table;
/*
/*	HTABLE_INFO *htable_sequence(table, how)
/*	HTABLE	*table;
/*	int	how;
/* DESCRIPTION
/*	This module maintains one or more hash tables. Each table entry
/*	consists of a unique string-valued lookup key and a generic
/*	character-pointer value.
/*	The tables are automatically resized when they fill up. When the
/*	values to be remembered are not character pointers, proper casts
/*	should be used or the code will not be portable.
/*
/*	htable_create() creates a table of the specified size and returns a
/*	pointer to the result. The lookup keys are saved with mystrdup().
/*	htable_enter() stores a (key, value) pair into the specified table
/*	and returns a pointer to the resulting entry. The code does not
/*	check if an entry with that key already exists: use htable_locate()
/*	for updating an existing entry.
/*
/*	htable_find() returns the value that was stored under the given key,
/*	or a null pointer if it was not found. In order to distinguish
/*	a null value from a non-existent value, use htable_locate().
/*
/*	htable_locate() returns a pointer to the entry that was stored
/*	for the given key, or a null pointer if it was not found.
/*
/*	htable_delete() removes one entry that was stored under the given key.
/*	If the free_fn argument is not a null pointer, the corresponding
/*	function is called with as argument the non-zero value stored under
/*	the key.
/*
/*	htable_free() destroys a hash table, including contents. If the free_fn
/*	argument is not a null pointer, the corresponding function is called
/*	for each table entry, with as argument the non-zero value stored
/*	with the entry.
/*
/*	htable_walk() invokes the action function for each table entry, with
/*	a pointer to the entry as its argument. The ptr argument is passed
/*	on to the action function.
/*
/*	htable_list() returns a null-terminated list of pointers to
/*	all elements in the named table. The list should be passed to
/*	myfree().
/*
/*	htable_sequence() returns the first or next element depending
/*	on the value of the "how" argument.  Specify HTABLE_SEQ_FIRST
/*	to start a new sequence, HTABLE_SEQ_NEXT to continue, and
/*	HTABLE_SEQ_STOP to terminate a sequence early.  The caller
/*	must not delete an element before it is visited.
/* RESTRICTIONS
/*	A callback function should not modify the hash table that is
/*	specified to its caller.
/* DIAGNOSTICS
/*	The following conditions are reported and cause the program to
/*	terminate immediately: memory allocation failure; an attempt
/*	to delete a non-existent entry.
/* SEE ALSO
/*	mymalloc(3) memory management wrapper
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* C library */

#include <sys_defs.h>
#include <string.h>

/* Local stuff */

#include "mymalloc.h"
#include "msg.h"
#include "htable.h"

/* htable_hash - hash a string */

static size_t htable_hash(const char *s, size_t size)
{
    size_t  h = 0;
    size_t  g;

    /*
     * From the "Dragon" book by Aho, Sethi and Ullman.
     */

    while (*s) {
	h = (h << 4U) + *(unsigned const char *) s++;
	if ((g = (h & 0xf0000000)) != 0) {
	    h ^= (g >> 24U);
	    h ^= g;
	}
    }
    return (h % size);
}

/* htable_link - insert element into table */

#define htable_link(table, element) { \
     HTABLE_INFO **_h = table->data + htable_hash(element->key, table->size);\
    element->prev = 0; \
    if ((element->next = *_h) != 0) \
	(*_h)->prev = element; \
    *_h = element; \
    table->used++; \
}

/* htable_size - allocate and initialize hash table */

static void htable_size(HTABLE *table, size_t size)
{
    HTABLE_INFO **h;

    size |= 1;

    table->data = h = (HTABLE_INFO **) mymalloc(size * sizeof(HTABLE_INFO *));
    table->size = size;
    table->used = 0;

    while (size-- > 0)
	*h++ = 0;
}

/* htable_create - create initial hash table */

HTABLE *htable_create(ssize_t size)
{
    HTABLE *table;

    table = (HTABLE *) mymalloc(sizeof(HTABLE));
    htable_size(table, size < 13 ? 13 : size);
    table->seq_bucket = table->seq_element = 0;
    return (table);
}

/* htable_grow - extend existing table */

static void htable_grow(HTABLE *table)
{
    HTABLE_INFO *ht;
    HTABLE_INFO *next;
    size_t  old_size = table->size;
    HTABLE_INFO **h = table->data;
    HTABLE_INFO **old_entries = h;

    htable_size(table, 2 * old_size);

    while (old_size-- > 0) {
	for (ht = *h++; ht; ht = next) {
	    next = ht->next;
	    htable_link(table, ht);
	}
    }
    myfree((void *) old_entries);
}

/* htable_enter - enter (key, value) pair */

HTABLE_INFO *htable_enter(HTABLE *table, const char *key, void *value)
{
    HTABLE_INFO *ht;

    if (table->used >= table->size)
	htable_grow(table);
    ht = (HTABLE_INFO *) mymalloc(sizeof(HTABLE_INFO));
    ht->key = mystrdup(key);
    ht->value = value;
    htable_link(table, ht);
    return (ht);
}

/* htable_find - lookup value */

void   *htable_find(HTABLE *table, const char *key)
{
    HTABLE_INFO *ht;

#define	STREQ(x,y) (x == y || (x[0] == y[0] && strcmp(x,y) == 0))

    if (table)
	for (ht = table->data[htable_hash(key, table->size)]; ht; ht = ht->next)
	    if (STREQ(key, ht->key))
		return (ht->value);
    return (0);
}

/* htable_locate - lookup entry */

HTABLE_INFO *htable_locate(HTABLE *table, const char *key)
{
    HTABLE_INFO *ht;

#define	STREQ(x,y) (x == y || (x[0] == y[0] && strcmp(x,y) == 0))

    if (table)
	for (ht = table->data[htable_hash(key, table->size)]; ht; ht = ht->next)
	    if (STREQ(key, ht->key))
		return (ht);
    return (0);
}

/* htable_delete - delete one entry */

void    htable_delete(HTABLE *table, const char *key, void (*free_fn) (void *))
{
    if (table) {
	HTABLE_INFO *ht;
	HTABLE_INFO **h = table->data + htable_hash(key, table->size);

#define	STREQ(x,y) (x == y || (x[0] == y[0] && strcmp(x,y) == 0))

	for (ht = *h; ht; ht = ht->next) {
	    if (STREQ(key, ht->key)) {
		if (ht->next)
		    ht->next->prev = ht->prev;
		if (ht->prev)
		    ht->prev->next = ht->next;
		else
		    *h = ht->next;
		table->used--;
		myfree(ht->key);
		if (free_fn && ht->value)
		    (*free_fn) (ht->value);
		myfree((void *) ht);
		return;
	    }
	}
	msg_panic("htable_delete: unknown_key: \"%s\"", key);
    }
}

/* htable_free - destroy hash table */

void    htable_free(HTABLE *table, void (*free_fn) (void *))
{
    if (table) {
	ssize_t i = table->size;
	HTABLE_INFO *ht;
	HTABLE_INFO *next;
	HTABLE_INFO **h = table->data;

	while (i-- > 0) {
	    for (ht = *h++; ht; ht = next) {
		next = ht->next;
		myfree(ht->key);
		if (free_fn && ht->value)
		    (*free_fn) (ht->value);
		myfree((void *) ht);
	    }
	}
	myfree((void *) table->data);
	table->data = 0;
	if (table->seq_bucket)
	    myfree((void *) table->seq_bucket);
	table->seq_bucket = 0;
	myfree((void *) table);
    }
}

/* htable_walk - iterate over hash table */

void    htable_walk(HTABLE *table, void (*action) (HTABLE_INFO *, void *),
		            void *ptr) {
    if (table) {
	ssize_t i = table->size;
	HTABLE_INFO **h = table->data;
	HTABLE_INFO *ht;

	while (i-- > 0)
	    for (ht = *h++; ht; ht = ht->next)
		(*action) (ht, ptr);
    }
}

/* htable_list - list all table members */

HTABLE_INFO **htable_list(HTABLE *table)
{
    HTABLE_INFO **list;
    HTABLE_INFO *member;
    ssize_t count = 0;
    ssize_t i;

    if (table != 0) {
	list = (HTABLE_INFO **) mymalloc(sizeof(*list) * (table->used + 1));
	for (i = 0; i < table->size; i++)
	    for (member = table->data[i]; member != 0; member = member->next)
		list[count++] = member;
    } else {
	list = (HTABLE_INFO **) mymalloc(sizeof(*list));
    }
    list[count] = 0;
    return (list);
}

/* htable_sequence - dict(3) compatibility iterator */

HTABLE_INFO *htable_sequence(HTABLE *table, int how)
{
    if (table == 0)
	return (0);

    switch (how) {
    case HTABLE_SEQ_FIRST:			/* start new sequence */
	if (table->seq_bucket)
	    myfree((void *) table->seq_bucket);
	table->seq_bucket = htable_list(table);
	table->seq_element = table->seq_bucket;
	return (*(table->seq_element)++);
    case HTABLE_SEQ_NEXT:			/* next element */
	if (table->seq_element && *table->seq_element)
	    return (*(table->seq_element)++);
	/* FALLTHROUGH */
    default:					/* terminate sequence */
	if (table->seq_bucket) {
	    myfree((void *) table->seq_bucket);
	    table->seq_bucket = table->seq_element = 0;
	}
	return (0);
    }
}

#ifdef TEST
#include <vstring_vstream.h>
#include <myrand.h>

int     main(int unused_argc, char **unused_argv)
{
    VSTRING *buf = vstring_alloc(10);
    ssize_t count = 0;
    HTABLE *hash;
    HTABLE_INFO **ht_info;
    HTABLE_INFO **ht;
    HTABLE_INFO *info;
    ssize_t i;
    ssize_t r;
    int     op;

    /*
     * Load a large number of strings and delete them in a random order.
     */
    hash = htable_create(10);
    while (vstring_get(buf, VSTREAM_IN) != VSTREAM_EOF)
	htable_enter(hash, vstring_str(buf), CAST_INT_TO_VOID_PTR(count++));
    for (i = 0, op = HTABLE_SEQ_FIRST; htable_sequence(hash, op) != 0;
	 i++, op = HTABLE_SEQ_NEXT)
	 /* void */ ;
    if (i != hash->used)
	msg_panic("%ld entries found, but %lu entries exist",
		  (long) i, (unsigned long) hash->used);
    ht_info = htable_list(hash);
    for (i = 0; i < hash->used; i++) {
	r = myrand() % hash->used;
	info = ht_info[i];
	ht_info[i] = ht_info[r];
	ht_info[r] = info;
    }
    for (ht = ht_info; *ht; ht++)
	htable_delete(hash, ht[0]->key, (void (*) (void *)) 0);
    if (hash->used > 0)
	msg_panic("%ld entries not deleted", (long) hash->used);
    myfree((void *) ht_info);
    htable_free(hash, (void (*) (void *)) 0);
    vstring_free(buf);
    return (0);
}

#endif
