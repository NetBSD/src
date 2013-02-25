/*	$NetBSD: dict_ht.c,v 1.1.1.3.10.1 2013/02/25 00:27:31 tls Exp $	*/

/*++
/* NAME
/*	dict_ht 3
/* SUMMARY
/*	dictionary manager interface to hash tables
/* SYNOPSIS
/*	#include <dict_ht.h>
/*
/*	DICT	*dict_ht_open(name, open_flags, dict_flags)
/*	const char *name;
/*	int	open_flags;
/*	int	dict_flags;
/* DESCRIPTION
/*	dict_ht_open() creates a memory-resident hash table and
/*	makes it accessible via the generic dictionary operations
/*	documented in dict_open(3).  The open_flags argument is
/*	ignored.
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
#include "stringops.h"
#include "vstring.h"

/* Application-specific. */

typedef struct {
    DICT    dict;			/* generic members */
    HTABLE *table;			/* hash table */
} DICT_HT;

/* dict_ht_lookup - find hash-table entry */

static const char *dict_ht_lookup(DICT *dict, const char *name)
{
    DICT_HT *dict_ht = (DICT_HT *) dict;

    /*
     * Optionally fold the key.
     */
    if (dict->flags & DICT_FLAG_FOLD_FIX) {
	if (dict->fold_buf == 0)
	    dict->fold_buf = vstring_alloc(10);
	vstring_strcpy(dict->fold_buf, name);
	name = lowercase(vstring_str(dict->fold_buf));
    }
    DICT_ERR_VAL_RETURN(dict, DICT_ERR_NONE, htable_find(dict_ht->table, name));
}

/* dict_ht_update - add or update hash-table entry */

static int dict_ht_update(DICT *dict, const char *name, const char *value)
{
    DICT_HT *dict_ht = (DICT_HT *) dict;
    HTABLE_INFO *ht;
    char   *saved_value = mystrdup(value);

    /*
     * Optionally fold the key.
     */
    if (dict->flags & DICT_FLAG_FOLD_FIX) {
	if (dict->fold_buf == 0)
	    dict->fold_buf = vstring_alloc(10);
	vstring_strcpy(dict->fold_buf, name);
	name = lowercase(vstring_str(dict->fold_buf));
    }
    if ((ht = htable_locate(dict_ht->table, name)) != 0) {
	myfree(ht->value);
    } else {
	ht = htable_enter(dict_ht->table, name, (char *) 0);
    }
    ht->value = saved_value;
    DICT_ERR_VAL_RETURN(dict, DICT_ERR_NONE, DICT_STAT_SUCCESS);
}

/* dict_ht_sequence - first/next iterator */

static int dict_ht_sequence(DICT *dict, int how, const char **name,
			            const char **value)
{
    DICT_HT *dict_ht = (DICT_HT *) dict;
    HTABLE_INFO *ht;

    ht = htable_sequence(dict_ht->table,
			 how == DICT_SEQ_FUN_FIRST ? HTABLE_SEQ_FIRST :
			 how == DICT_SEQ_FUN_NEXT ? HTABLE_SEQ_NEXT :
			 HTABLE_SEQ_STOP);
    if (ht != 0) {
	*name = ht->key;
	*value = ht->value;
	DICT_ERR_VAL_RETURN(dict, DICT_ERR_NONE, DICT_STAT_SUCCESS);
    } else {
	*name = 0;
	*value = 0;
	DICT_ERR_VAL_RETURN(dict, DICT_ERR_NONE, DICT_STAT_FAIL);
    }
}

/* dict_ht_close - disassociate from hash table */

static void dict_ht_close(DICT *dict)
{
    DICT_HT *dict_ht = (DICT_HT *) dict;

    htable_free(dict_ht->table, myfree);
    if (dict_ht->dict.fold_buf)
	vstring_free(dict_ht->dict.fold_buf);
    dict_free(dict);
}

/* dict_ht_open - create association with hash table */

DICT   *dict_ht_open(const char *name, int unused_open_flags, int dict_flags)
{
    DICT_HT *dict_ht;

    dict_ht = (DICT_HT *) dict_alloc(DICT_TYPE_HT, name, sizeof(*dict_ht));
    dict_ht->dict.lookup = dict_ht_lookup;
    dict_ht->dict.update = dict_ht_update;
    dict_ht->dict.sequence = dict_ht_sequence;
    dict_ht->dict.close = dict_ht_close;
    dict_ht->dict.flags = dict_flags | DICT_FLAG_FIXED;
    if (dict_flags & DICT_FLAG_FOLD_FIX)
	dict_ht->dict.fold_buf = vstring_alloc(10);
    dict_ht->table = htable_create(0);
    dict_ht->dict.owner.status = DICT_OWNER_TRUSTED;
    return (&dict_ht->dict);
}
