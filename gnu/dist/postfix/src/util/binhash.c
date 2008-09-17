/*++
/* NAME
/*	binhash 3
/* SUMMARY
/*	hash table manager
/* SYNOPSIS
/*	#include <binhash.h>
/*
/*	typedef	struct {
/* .in +4
/*		char	*key;
/*		int	key_len;
/*		char	*value;
/*		/* private fields... */
/* .in -4
/*	} BINHASH_INFO;
/*
/*	BINHASH	*binhash_create(size)
/*	int	size;
/*
/*	BINHASH_INFO *binhash_enter(table, key, key_len, value)
/*	BINHASH	*table;
/*	const char *key;
/*	int	key_len;
/*	char	*value;
/*
/*	char	*binhash_find(table, key, key_len)
/*	BINHASH	*table;
/*	const char *key;
/*	int	key_len;
/*
/*	BINHASH_INFO *binhash_locate(table, key, key_len)
/*	BINHASH	*table;
/*	const char *key;
/*	int	key_len;
/*
/*	void	binhash_delete(table, key, key_len, free_fn)
/*	BINHASH	*table;
/*	const char *key;
/*	int	key_len;
/*	void	(*free_fn)(char *);
/*
/*	void	binhash_free(table, free_fn)
/*	BINHASH	*table;
/*	void	(*free_fn)(char *);
/*
/*	void	binhash_walk(table, action, ptr)
/*	BINHASH	*table;
/*	void	(*action)(BINHASH_INFO *info, char *ptr);
/*	char	*ptr;
/*
/*	BINHASH_INFO **binhash_list(table)
/*	BINHASH	*table;
/* DESCRIPTION
/*	This module maintains one or more hash tables. Each table entry
/*	consists of a unique binary-valued lookup key and a generic
/*	character-pointer value.
/*	The tables are automatically resized when they fill up. When the
/*	values to be remembered are not character pointers, proper casts
/*	should be used or the code will not be portable.
/*
/*	binhash_create() creates a table of the specified size and returns a
/*	pointer to the result. The lookup keys are saved with strdup().
/*
/*	binhash_enter() stores a (key, value) pair into the specified table
/*	and returns a pointer to the resulting entry. The code does not
/*	check if an entry with that key already exists: use binhash_locate()
/*	for updating an existing entry. The key is copied; the value is not.
/*
/*	binhash_find() returns the value that was stored under the given key,
/*	or a null pointer if it was not found. In order to distinguish
/*	a null value from a non-existent value, use binhash_locate().
/*
/*	binhash_locate() returns a pointer to the entry that was stored
/*	for the given key, or a null pointer if it was not found.
/*
/*	binhash_delete() removes one entry that was stored under the given key.
/*	If the free_fn argument is not a null pointer, the corresponding
/*	function is called with as argument the value that was stored under
/*	the key.
/*
/*	binhash_free() destroys a hash table, including contents. If the free_fn
/*	argument is not a null pointer, the corresponding function is called
/*	for each table entry, with as argument the value that was stored
/*	with the entry.
/*
/*	binhash_walk() invokes the action function for each table entry, with
/*	a pointer to the entry as its argument. The ptr argument is passed
/*	on to the action function.
/*
/*	binhash_list() returns a null-terminated list of pointers to
/*	all elements in the named table. The list should be passed to
/*	myfree().
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
#include "binhash.h"

/* binhash_hash - hash a string */

static unsigned binhash_hash(const char *key, int len, unsigned size)
{
    unsigned long h = 0;
    unsigned long g;

    /*
     * From the "Dragon" book by Aho, Sethi and Ullman.
     */

    while (len-- > 0) {
	h = (h << 4U) + *key++;
	if ((g = (h & 0xf0000000)) != 0) {
	    h ^= (g >> 24U);
	    h ^= g;
	}
    }
    return (h % size);
}

/* binhash_link - insert element into table */

#define binhash_link(table, elm) { \
    BINHASH_INFO **_h = table->data + binhash_hash(elm->key, elm->key_len, table->size);\
    elm->prev = 0; \
    if ((elm->next = *_h) != 0) \
	(*_h)->prev = elm; \
    *_h = elm; \
    table->used++; \
}

/* binhash_size - allocate and initialize hash table */

static void binhash_size(BINHASH *table, unsigned size)
{
    BINHASH_INFO **h;

    size |= 1;

    table->data = h = (BINHASH_INFO **) mymalloc(size * sizeof(BINHASH_INFO *));
    table->size = size;
    table->used = 0;

    while (size-- > 0)
	*h++ = 0;
}

/* binhash_create - create initial hash table */

BINHASH *binhash_create(int size)
{
    BINHASH *table;

    table = (BINHASH *) mymalloc(sizeof(BINHASH));
    binhash_size(table, size < 13 ? 13 : size);
    return (table);
}

/* binhash_grow - extend existing table */

static void binhash_grow(BINHASH *table)
{
    BINHASH_INFO *ht;
    BINHASH_INFO *next;
    unsigned old_size = table->size;
    BINHASH_INFO **h = table->data;
    BINHASH_INFO **old_entries = h;

    binhash_size(table, 2 * old_size);

    while (old_size-- > 0) {
	for (ht = *h++; ht; ht = next) {
	    next = ht->next;
	    binhash_link(table, ht);
	}
    }
    myfree((char *) old_entries);
}

/* binhash_enter - enter (key, value) pair */

BINHASH_INFO *binhash_enter(BINHASH *table, const char *key, int key_len, char *value)
{
    BINHASH_INFO *ht;

    if (table->used >= table->size)
	binhash_grow(table);
    ht = (BINHASH_INFO *) mymalloc(sizeof(BINHASH_INFO));
    ht->key = mymemdup(key, key_len);
    ht->key_len = key_len;
    ht->value = value;
    binhash_link(table, ht);
    return (ht);
}

/* binhash_find - lookup value */

char   *binhash_find(BINHASH *table, const char *key, int key_len)
{
    BINHASH_INFO *ht;

#define	KEY_EQ(x,y,l) (x[0] == y[0] && memcmp(x,y,l) == 0)

    if (table != 0)
	for (ht = table->data[binhash_hash(key, key_len, table->size)]; ht; ht = ht->next)
	    if (key_len == ht->key_len && KEY_EQ(key, ht->key, key_len))
		return (ht->value);
    return (0);
}

/* binhash_locate - lookup entry */

BINHASH_INFO *binhash_locate(BINHASH *table, const char *key, int key_len)
{
    BINHASH_INFO *ht;

#define	KEY_EQ(x,y,l) (x[0] == y[0] && memcmp(x,y,l) == 0)

    if (table != 0)
	for (ht = table->data[binhash_hash(key, key_len, table->size)]; ht; ht = ht->next)
	    if (key_len == ht->key_len && KEY_EQ(key, ht->key, key_len))
		return (ht);
    return (0);
}

/* binhash_delete - delete one entry */

void    binhash_delete(BINHASH *table, const char *key, int key_len, void (*free_fn) (char *))
{
    if (table != 0) {
	BINHASH_INFO *ht;
	BINHASH_INFO **h = table->data + binhash_hash(key, key_len, table->size);

#define	KEY_EQ(x,y,l) (x[0] == y[0] && memcmp(x,y,l) == 0)

	for (ht = *h; ht; ht = ht->next) {
	    if (key_len == ht->key_len && KEY_EQ(key, ht->key, key_len)) {
		if (ht->next)
		    ht->next->prev = ht->prev;
		if (ht->prev)
		    ht->prev->next = ht->next;
		else
		    *h = ht->next;
		table->used--;
		myfree(ht->key);
		if (free_fn)
		    (*free_fn) (ht->value);
		myfree((char *) ht);
		return;
	    }
	}
	msg_panic("binhash_delete: unknown_key: \"%s\"", key);
    }
}

/* binhash_free - destroy hash table */

void    binhash_free(BINHASH *table, void (*free_fn) (char *))
{
    if (table != 0) {
	unsigned i = table->size;
	BINHASH_INFO *ht;
	BINHASH_INFO *next;
	BINHASH_INFO **h = table->data;

	while (i-- > 0) {
	    for (ht = *h++; ht; ht = next) {
		next = ht->next;
		myfree(ht->key);
		if (free_fn)
		    (*free_fn) (ht->value);
		myfree((char *) ht);
	    }
	}
	myfree((char *) table->data);
	table->data = 0;
	myfree((char *) table);
    }
}

/* binhash_walk - iterate over hash table */

void    binhash_walk(BINHASH *table, void (*action) (BINHASH_INFO *, char *),
		             char *ptr) {
    if (table != 0) {
	unsigned i = table->size;
	BINHASH_INFO **h = table->data;
	BINHASH_INFO *ht;

	while (i-- > 0)
	    for (ht = *h++; ht; ht = ht->next)
		(*action) (ht, ptr);
    }
}

/* binhash_list - list all table members */

BINHASH_INFO **binhash_list(table)
BINHASH *table;
{
    BINHASH_INFO **list;
    BINHASH_INFO *member;
    int     count = 0;
    int     i;

    if (table != 0) {
	list = (BINHASH_INFO **) mymalloc(sizeof(*list) * (table->used + 1));
	for (i = 0; i < table->size; i++)
	    for (member = table->data[i]; member != 0; member = member->next)
		list[count++] = member;
    } else {
	list = (BINHASH_INFO **) mymalloc(sizeof(*list));
    }
    list[count] = 0;
    return (list);
}
