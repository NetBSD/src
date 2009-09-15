/*	$NetBSD: ctable.c,v 1.1.1.1.2.2 2009/09/15 06:03:54 snj Exp $	*/

/*++
/* NAME
/*	ctable 3
/* SUMMARY
/*	cache manager
/* SYNOPSIS
/*	#include <ctable.h>
/*
/*	CTABLE	*ctable_create(limit, create, delete, context)
/*	int	limit;
/*	void	*(*create)(const char *key, void *context);
/*	void	(*delete)(void *value, void *context);
/*	void	*context;
/*
/*	const void *ctable_locate(cache, key)
/*	CTABLE	*cache;
/*	const char *key;
/*
/*	void	ctable_free(cache)
/*	CTABLE	*cache;
/*
/*	void	ctable_walk(cache, action)
/*	CTABLE	*cache;
/*	void	(*action)(const char *key, const void *value);
/* DESCRIPTION
/*	This module maintains multiple caches. Cache items are purged
/*	automatically when the number of items exceeds a configurable
/*	limit. Caches never shrink. Each cache entry consists of a
/*	string-valued lookup key and a generic data pointer value.
/*
/*	ctable_create() creates a cache with the specified size limit, and
/*	returns a pointer to the result. The create and delete arguments
/*	specify pointers to call-back functions that create a value, given
/*	a key, and delete a given value, respectively. The context argument
/*	is passed on to the call-back routines.
/*
/*	ctable_locate() looks up or generates the value that corresponds to
/*	the specified key, and returns that value.
/*
/*	ctable_free() destroys the specified cache, including its contents.
/*
/*	ctable_walk() iterates over all elements in the cache, and invokes
/*	the action function for each cache element with the corresponding
/*	key and value as arguments. This function is useful mainly for
/*	cache performance debugging.
/* DIAGNOSTICS
/*	Fatal errors: out of memory. Panic: interface violation.
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

/* System library. */

#include <sys_defs.h>
#include <stdlib.h>
#include <stddef.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <ring.h>
#include <htable.h>
#include <ctable.h>

 /*
  * Cache entries are kept in most-recently used order. We use a hash table
  * to quickly locate cache entries.
  */
#define	CTABLE_ENTRY struct ctable_entry

struct ctable_entry {
    RING    ring;			/* MRU linkage */
    const char *key;			/* lookup key */
    void   *value;			/* corresponding value */
};

#define RING_TO_CTABLE_ENTRY(ring_ptr)	\
	RING_TO_APPL(ring_ptr, CTABLE_ENTRY, ring)
#define RING_PTR_OF(x)	(&((x)->ring))

struct ctable {
    HTABLE *table;			/* table with key, ctable_entry pairs */
    unsigned limit;			/* max nr of entries */
    unsigned used;			/* current nr of entries */
    CTABLE_CREATE_FN create;		/* constructor */
    CTABLE_DELETE_FN delete;		/* destructor */
    RING    ring;			/* MRU linkage */
    void   *context;			/* application context */
};

#define CTABLE_MIN_SIZE	5

/* ctable_create - create empty cache */

CTABLE *ctable_create(int limit, CTABLE_CREATE_FN create,
		              CTABLE_DELETE_FN delete, void *context)
{
    CTABLE *cache = (CTABLE *) mymalloc(sizeof(CTABLE));
    const char *myname = "ctable_create";

    if (limit < 1)
	msg_panic("%s: bad cache limit: %d", myname, limit);

    cache->table = htable_create(limit);
    cache->limit = (limit < CTABLE_MIN_SIZE ? CTABLE_MIN_SIZE : limit);
    cache->used = 0;
    cache->create = create;
    cache->delete = delete;
    ring_init(RING_PTR_OF(cache));
    cache->context = context;
    return (cache);
}

/* ctable_locate - look up or create cache item */

const void *ctable_locate(CTABLE *cache, const char *key)
{
    const char *myname = "ctable_locate";
    CTABLE_ENTRY *entry;

    /*
     * If the entry is not in the cache, make sure there is room for a new
     * entry and install it at the front of the MRU chain. Otherwise, move
     * the entry to the front of the MRU chain if it is not already there.
     * All this means that the cache never shrinks.
     */
    if ((entry = (CTABLE_ENTRY *) htable_find(cache->table, key)) == 0) {
	if (cache->used >= cache->limit) {
	    entry = RING_TO_CTABLE_ENTRY(ring_pred(RING_PTR_OF(cache)));
	    if (msg_verbose)
		msg_info("%s: purge entry key %s", myname, entry->key);
	    ring_detach(RING_PTR_OF(entry));
	    cache->delete(entry->value, cache->context);
	    htable_delete(cache->table, entry->key, (void (*) (char *)) 0);
	} else {
	    entry = (CTABLE_ENTRY *) mymalloc(sizeof(CTABLE_ENTRY));
	    cache->used++;
	}
	entry->value = cache->create(key, cache->context);
	entry->key = htable_enter(cache->table, key, (char *) entry)->key;
	ring_append(RING_PTR_OF(cache), RING_PTR_OF(entry));
	if (msg_verbose)
	    msg_info("%s: install entry key %s", myname, entry->key);
    } else if (entry == RING_TO_CTABLE_ENTRY(ring_succ(RING_PTR_OF(cache)))) {
	if (msg_verbose)
	    msg_info("%s: leave existing entry key %s", myname, entry->key);
    } else {
	ring_detach(RING_PTR_OF(entry));
	ring_append(RING_PTR_OF(cache), RING_PTR_OF(entry));
	if (msg_verbose)
	    msg_info("%s: move existing entry key %s", myname, entry->key);
    }
    return (entry->value);
}

static CTABLE *ctable_free_cache;

/* ctable_free_callback - callback function */

static void ctable_free_callback(char *ptr)
{
    CTABLE_ENTRY *entry = (CTABLE_ENTRY *) ptr;

    ctable_free_cache->delete(entry->value, ctable_free_cache->context);
    myfree((char *) entry);
}

/* ctable_free - destroy cache and contents */

void    ctable_free(CTABLE *cache)
{
    CTABLE *saved_cache = ctable_free_cache;

    /*
     * XXX the hash table does not pass application context so we have to
     * store it in a global variable.
     */
    ctable_free_cache = cache;
    htable_free(cache->table, ctable_free_callback);
    myfree((char *) cache);
    ctable_free_cache = saved_cache;
}

/* ctable_walk - iterate over all cache entries */

void    ctable_walk(CTABLE *cache, void (*action) (const char *, const void *))
{
    RING   *entry = RING_PTR_OF(cache);

    /* Walking down the MRU chain is less work than using ht_walk(). */

    while ((entry = ring_succ(entry)) != RING_PTR_OF(cache))
	action((RING_TO_CTABLE_ENTRY(entry)->key),
	       (RING_TO_CTABLE_ENTRY(entry)->value));
}

#ifdef TEST

 /*
  * Proof-of-concept test program. Read keys from stdin, ask for values not
  * in cache.
  */
#include <unistd.h>
#include <vstream.h>
#include <vstring.h>
#include <vstring_vstream.h>
#include <msg_vstream.h>

#define STR(x)	vstring_str(x)

static void *ask(const char *key, void *context)
{
    VSTRING *data_buf = (VSTRING *) context;

    vstream_printf("ask: %s = ", key);
    vstream_fflush(VSTREAM_OUT);
    if (vstring_get_nonl(data_buf, VSTREAM_IN) == VSTREAM_EOF)
	vstream_longjmp(VSTREAM_IN, 1);
    if (!isatty(0)) {
	vstream_printf("%s\n", STR(data_buf));
	vstream_fflush(VSTREAM_OUT);
    }
    return (mystrdup(STR(data_buf)));
}

static void drop(void *data, void *unused_context)
{
    myfree(data);
}

int     main(int unused_argc, char **argv)
{
    VSTRING *key_buf;
    VSTRING *data_buf;
    CTABLE *cache;
    const char *value;

    msg_vstream_init(argv[0], VSTREAM_ERR);
    key_buf = vstring_alloc(100);
    data_buf = vstring_alloc(100);
    cache = ctable_create(1, ask, drop, (void *) data_buf);
    msg_verbose = 1;
    vstream_control(VSTREAM_IN, VSTREAM_CTL_EXCEPT, VSTREAM_CTL_END);

    if (vstream_setjmp(VSTREAM_IN) == 0) {
	for (;;) {
	    vstream_printf("key = ");
	    vstream_fflush(VSTREAM_OUT);
	    if (vstring_get_nonl(key_buf, VSTREAM_IN) == VSTREAM_EOF)
		vstream_longjmp(VSTREAM_IN, 1);
	    if (!isatty(0)) {
		vstream_printf("%s\n", STR(key_buf));
		vstream_fflush(VSTREAM_OUT);
	    }
	    value = ctable_locate(cache, STR(key_buf));
	    vstream_printf("result: %s\n", value);
	}
    }
    ctable_free(cache);
    vstring_free(key_buf);
    vstring_free(data_buf);
    return (0);
}

#endif
