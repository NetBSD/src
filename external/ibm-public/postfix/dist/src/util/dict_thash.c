/*	$NetBSD: dict_thash.c,v 1.1.1.3.10.1 2017/04/21 16:52:53 bouyer Exp $	*/

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
#include <iostuff.h>
#include <vstring.h>
#include <stringops.h>
#include <readlline.h>
#include <dict.h>
#include <dict_ht.h>
#include <dict_thash.h>

/* Application-specific. */

#define STR	vstring_str
#define LEN	VSTRING_LEN

/* dict_thash_open - open flat text data base */

DICT   *dict_thash_open(const char *path, int open_flags, int dict_flags)
{
    DICT   *dict;
    VSTREAM *fp = 0;			/* DICT_THASH_OPEN_RETURN() */
    struct stat st;
    time_t  before;
    time_t  after;
    VSTRING *line_buffer = 0;		/* DICT_THASH_OPEN_RETURN() */
    int     lineno;
    int     last_line;
    char   *key;
    char   *value;

    /*
     * Let the optimizer worry about eliminating redundant code.
     */
#define DICT_THASH_OPEN_RETURN(d) do { \
	DICT *__d = (d); \
	if (fp != 0) \
	    vstream_fclose(fp); \
	if (line_buffer != 0) \
	    vstring_free(line_buffer); \
	return (__d); \
    } while (0)

    /*
     * Sanity checks.
     */
    if (open_flags != O_RDONLY)
	DICT_THASH_OPEN_RETURN(dict_surrogate(DICT_TYPE_THASH, path,
					      open_flags, dict_flags,
				  "%s:%s map requires O_RDONLY access mode",
					      DICT_TYPE_THASH, path));

    /*
     * Read the flat text file into in-memory hash. Read the file again if it
     * may have changed while we were reading.
     */
    for (before = time((time_t *) 0); /* see below */ ; before = after) {
	if ((fp = vstream_fopen(path, open_flags, 0644)) == 0) {
	    DICT_THASH_OPEN_RETURN(dict_surrogate(DICT_TYPE_THASH, path,
						  open_flags, dict_flags,
					     "open database %s: %m", path));
	    }

	/*
	 * Reuse the "internal" dictionary type.
	 */
	dict = dict_open3(DICT_TYPE_HT, path, open_flags, dict_flags);
	dict_type_override(dict, DICT_TYPE_THASH);

	if (line_buffer == 0)
	    line_buffer = vstring_alloc(100);
	last_line = 0;
	while (readllines(line_buffer, fp, &last_line, &lineno)) {

	    /*
	     * First some UTF-8 checks sans casefolding.
	     */
	    if ((dict->flags & DICT_FLAG_UTF8_ACTIVE)
		&& allascii(STR(line_buffer)) == 0
		&& valid_utf8_string(STR(line_buffer), LEN(line_buffer)) == 0) {
		msg_warn("%s, line %d: non-UTF-8 input \"%s\""
			 " -- ignoring this line",
			 VSTREAM_PATH(fp), lineno, STR(line_buffer));
		continue;
	    }

	    /*
	     * Split on the first whitespace character, then trim leading and
	     * trailing whitespace from key and value.
	     */
	    key = STR(line_buffer);
	    value = key + strcspn(key, CHARS_SPACE);
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
	     * Store the value under the key. Handle duplicates
	     * appropriately. XXX Move this into dict_ht, but 1) that map
	     * ignores duplicates by default and we would have to check that
	     * we won't break existing code that depends on such benavior; 2)
	     * by inlining the checks here we can degrade gracefully instead
	     * of terminating with a fatal error. See comment in dict_inline.c.
	     */
	    if (dict->lookup(dict, key) != 0) {
		if (dict_flags & DICT_FLAG_DUP_IGNORE) {
		     /* void */ ;
		} else if (dict_flags & DICT_FLAG_DUP_REPLACE) {
		    dict->update(dict, key, value);
		} else if (dict_flags & DICT_FLAG_DUP_WARN) {
		    msg_warn("%s, line %d: duplicate entry: \"%s\"",
			     path, lineno, key);
		} else {
		    dict->close(dict);
		    DICT_THASH_OPEN_RETURN(dict_surrogate(DICT_TYPE_THASH, path,
						     open_flags, dict_flags,
				     "%s, line %d: duplicate entry: \"%s\"",
							path, lineno, key));
		}
	    } else {
		dict->update(dict, key, value);
	    }
	}

	/*
	 * See if the source file is hot.
	 */
	if (fstat(vstream_fileno(fp), &st) < 0)
	    msg_fatal("fstat %s: %m", path);
	if (vstream_fclose(fp))
	    msg_fatal("read %s: %m", path);
	fp = 0;					/* DICT_THASH_OPEN_RETURN() */
	after = time((time_t *) 0);
	if (st.st_mtime < before - 1 || st.st_mtime > after)
	    break;

	/*
	 * Yes, it is hot. Discard the result and read the file again.
	 */
	dict->close(dict);
	if (msg_verbose > 1)
	    msg_info("pausing to let file %s cool down", path);
	doze(300000);
    }

    dict->owner.uid = st.st_uid;
    dict->owner.status = (st.st_uid != 0);

    DICT_THASH_OPEN_RETURN(DICT_DEBUG (dict));
}
