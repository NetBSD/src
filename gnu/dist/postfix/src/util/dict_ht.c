/*++
/* NAME
/*	dict_ht 3
/* SUMMARY
/*	dictionary manager interface to hash tables
/* SYNOPSIS
/*	#include <dict_ht.h>
/*
/*	DICT	*dict_ht_open(name, table, remove)
/*	const char *name;
/*	HTABLE	*table;
/*	void	(*remove)(char *value)
/* DESCRIPTION
/*	dict_ht_open() makes specified hash table accessible via the
/*	generic dictionary operations documented in dict_open(3).
/*	\fIremove\fR specifies an optional callback function
/*	that is called by the hash table manager when the hash table is
/*	removed from the dictionary manager's care. The hash table is not
/*	destroyed when \fIremove\fR is a null pointer.
/* SEE ALSO
/*	dict(3) generic dictionary manager
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

#include "sys_defs.h"

/* Utility library. */

#include "mymalloc.h"
#include "htable.h"
#include "dict.h"
#include "dict_ht.h"

/* Application-specific. */

typedef struct {
    DICT    dict;			/* generic members */
    HTABLE *table;			/* hash table */
    void    (*remove) (char *);		/* callback */
} DICT_HT;

/* dict_ht_lookup - find hash-table entry */

static const char *dict_ht_lookup(DICT *dict, const char *name)
{
    DICT_HT *dict_ht = (DICT_HT *) dict;

    dict_errno = 0;

    return (htable_find(dict_ht->table, name));
}

/* dict_ht_update - add or update hash-table entry */

static void dict_ht_update(DICT *dict, const char *name, const char *value)
{
    DICT_HT *dict_ht = (DICT_HT *) dict;
    HTABLE_INFO *ht;

    if ((ht = htable_locate(dict_ht->table, name)) != 0) {
	myfree(ht->value);
    } else {
	ht = htable_enter(dict_ht->table, name, (char *) 0);
    }
    ht->value = mystrdup(value);
}

/* dict_ht_close - disassociate from hash table */

static void dict_ht_close(DICT *dict)
{
    DICT_HT *dict_ht = (DICT_HT *) dict;

    if (dict_ht->remove)
	htable_free(dict_ht->table, dict_ht->remove);
    dict_free(dict);
}

/* dict_ht_open - create association with hash table */

DICT   *dict_ht_open(const char *name, HTABLE *table, void (*remove) (char *))
{
    DICT_HT *dict_ht;

    dict_ht = (DICT_HT *) dict_alloc(DICT_TYPE_HT, name, sizeof(*dict_ht));
    dict_ht->dict.lookup = dict_ht_lookup;
    dict_ht->dict.update = dict_ht_update;
    dict_ht->dict.close = dict_ht_close;
    dict_ht->table = table;
    dict_ht->remove = remove;
    return (&dict_ht->dict);
}
