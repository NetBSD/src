/*	$NetBSD: dict_thash.c,v 1.1.1.2 2013/01/02 18:59:12 tron Exp $	*/

/*++
/* NAME
/*	dict_thash 3
/* SUMMARY
/*	dictionary manager interface to hashed flat text files
/* SYNOPSIS
/*	#include <dict_thash.h>
/*
/*	DICT	*dict_thash_open(path, open_flags, dict_flags)
/*	const char *name;
/*	const char *path;
/*	int	open_flags;
/*	int	dict_flags;
/* DESCRIPTION
/*	dict_thash_open() opens the named flat text file, creates
/*	an in-memory hash table, and makes it available via the
/*	generic interface described in dict_open(3). The input
/*	format is as with postmap(1).
/* DIAGNOSTICS
/*	Fatal errors: cannot open file, out of memory.
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

#include <sys_defs.h>
#include <sys/stat.h>
#include <ctype.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <htable.h>
#include <iostuff.h>
#include <vstring.h>
#include <stringops.h>
#include <readlline.h>
#include <dict.h>
#include <dict_thash.h>
#include <warn_stat.h>

/* Application-specific. */

typedef struct {
    DICT    dict;			/* generic members */
    HTABLE *table;			/* in-memory hash */
    HTABLE_INFO **info;			/* for iterator */
    HTABLE_INFO **cursor;		/* ditto */
} DICT_THASH;

#define STR	vstring_str

/* dict_thash_lookup - find database entry */

static const char *dict_thash_lookup(DICT *dict, const char *name)
{
    DICT_THASH *dict_thash = (DICT_THASH *) dict;
    const char *result = 0;

    /*
     * Optionally fold the key.
     */
    if (dict->flags & DICT_FLAG_FOLD_FIX) {
	if (dict->fold_buf == 0)
	    dict->fold_buf = vstring_alloc(10);
	vstring_strcpy(dict->fold_buf, name);
	name = lowercase(vstring_str(dict->fold_buf));
    }

    /*
     * Look up the value.
     */
    result = htable_find(dict_thash->table, name);

    DICT_ERR_VAL_RETURN(dict, DICT_ERR_NONE, result);
}

/* dict_thash_sequence - traverse the dictionary */

static int dict_thash_sequence(DICT *dict, int function,
			               const char **key, const char **value)
{
    const char *myname = "dict_thash_sequence";
    DICT_THASH *dict_thash = (DICT_THASH *) dict;

    /*
     * Determine and execute the seek function.
     */
    switch (function) {
    case DICT_SEQ_FUN_FIRST:
	if (dict_thash->info == 0)
	    dict_thash->info = htable_list(dict_thash->table);
	dict_thash->cursor = dict_thash->info;
	break;
    case DICT_SEQ_FUN_NEXT:
	if (dict_thash->cursor[0])
	    dict_thash->cursor += 1;
	break;
    default:
	msg_panic("%s: invalid function: %d", myname, function);
    }

    /*
     * Return the entry under the cursor.
     */
    if (dict_thash->cursor[0]) {
	*key = dict_thash->cursor[0]->key;
	*value = dict_thash->cursor[0]->value;
	DICT_ERR_VAL_RETURN(dict, DICT_ERR_NONE, DICT_STAT_SUCCESS);
    } else {
	*key = 0;
	*value = 0;
	DICT_ERR_VAL_RETURN(dict, DICT_ERR_NONE, DICT_STAT_FAIL);
    }
}

/* dict_thash_close - disassociate from data base */

static void dict_thash_close(DICT *dict)
{
    DICT_THASH *dict_thash = (DICT_THASH *) dict;

    htable_free(dict_thash->table, myfree);
    if (dict_thash->info)
	myfree((char *) dict_thash->info);
    if (dict->fold_buf)
	vstring_free(dict->fold_buf);
    dict_free(dict);
}

/* dict_thash_open - open flat text data base */

DICT   *dict_thash_open(const char *path, int open_flags, int dict_flags)
{
    DICT_THASH *dict_thash;
    VSTREAM *fp;
    struct stat st;
    time_t  before;
    time_t  after;
    VSTRING *line_buffer = 0;
    int     lineno;
    char   *key;
    char   *value;
    HTABLE *table;
    HTABLE_INFO *ht;

    /*
     * Sanity checks.
     */
    if (open_flags != O_RDONLY)
	return (dict_surrogate(DICT_TYPE_THASH, path, open_flags, dict_flags,
			       "%s:%s map requires O_RDONLY access mode",
			       DICT_TYPE_THASH, path));

    /*
     * Read the flat text file into in-memory hash. Read the file again if it
     * may have changed while we were reading.
     */
    for (before = time((time_t *) 0); /* see below */ ; before = after) {
	if ((fp = vstream_fopen(path, open_flags, 0644)) == 0) {
	    return (dict_surrogate(DICT_TYPE_THASH, path, open_flags, dict_flags,
				   "open database %s: %m", path));
	}
	if (line_buffer == 0)
	    line_buffer = vstring_alloc(100);
	lineno = 0;
	table = htable_create(13);
	while (readlline(line_buffer, fp, &lineno)) {

	    /*
	     * Split on the first whitespace character, then trim leading and
	     * trailing whitespace from key and value.
	     */
	    key = STR(line_buffer);
	    value = key + strcspn(key, " \t\r\n");
	    if (*value)
		*value++ = 0;
	    while (ISSPACE(*value))
		value++;
	    trimblanks(key, 0)[0] = 0;
	    trimblanks(value, 0)[0] = 0;

	    /*
	     * Enforce the "key whitespace value" format. Disallow missing
	     * keys or missing values.
	     */
	    if (*key == 0 || *value == 0) {
		msg_warn("%s, line %d: expected format: key whitespace value"
			 " -- ignoring this line", path, lineno);
		continue;
	    }
	    if (key[strlen(key) - 1] == ':')
		msg_warn("%s, line %d: record is in \"key: value\" format;"
			 " is this an alias file?", path, lineno);

	    /*
	     * Optionally fold the key.
	     */
	    if (dict_flags & DICT_FLAG_FOLD_FIX)
		lowercase(key);

	    /*
	     * Store the value under the key. Handle duplicates
	     * appropriately.
	     */
	    if ((ht = htable_locate(table, key)) != 0) {
		if (dict_flags & DICT_FLAG_DUP_IGNORE) {
		     /* void */ ;
		} else if (dict_flags & DICT_FLAG_DUP_REPLACE) {
		    myfree(ht->value);
		    ht->value = mystrdup(value);
		} else if (dict_flags & DICT_FLAG_DUP_WARN) {
		    msg_warn("%s, line %d: duplicate entry: \"%s\"",
			     path, lineno, key);
		} else {
		    msg_fatal("%s, line %d: duplicate entry: \"%s\"",
			      path, lineno, key);
		}
	    } else {
		htable_enter(table, key, mystrdup(value));
	    }
	}

	/*
	 * See if the source file is hot.
	 */
	if (fstat(vstream_fileno(fp), &st) < 0)
	    msg_fatal("fstat %s: %m", path);
	if (vstream_fclose(fp))
	    msg_fatal("read %s: %m", path);
	after = time((time_t *) 0);
	if (st.st_mtime < before - 1 || st.st_mtime > after)
	    break;

	/*
	 * Yes, it is hot. Discard the result and read the file again.
	 */
	htable_free(table, myfree);
	if (msg_verbose > 1)
	    msg_info("pausing to let file %s cool down", path);
	doze(300000);
    }
    vstring_free(line_buffer);

    /*
     * Create the in-memory table.
     */
    dict_thash = (DICT_THASH *)
	dict_alloc(DICT_TYPE_THASH, path, sizeof(*dict_thash));
    dict_thash->dict.lookup = dict_thash_lookup;
    dict_thash->dict.sequence = dict_thash_sequence;
    dict_thash->dict.close = dict_thash_close;
    dict_thash->dict.flags = dict_flags | DICT_FLAG_DUP_WARN | DICT_FLAG_FIXED;
    if (dict_flags & DICT_FLAG_FOLD_FIX)
	dict_thash->dict.fold_buf = vstring_alloc(10);
    dict_thash->info = 0;
    dict_thash->table = table;
    dict_thash->dict.owner.uid = st.st_uid;
    dict_thash->dict.owner.status = (st.st_uid != 0);

    return (DICT_DEBUG (&dict_thash->dict));
}
