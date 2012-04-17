/*	$NetBSD: dict.c,v 1.1.1.3.4.1 2012/04/17 00:04:34 yamt Exp $	*/

/*++
/* NAME
/*	dict 3
/* SUMMARY
/*	dictionary manager
/* SYNOPSIS
/*	#include <dict.h>
/*
/*	extern int dict_unknown_allowed;
/*	extern int dict_errno;
/*
/*	void	dict_register(dict_name, dict_info)
/*	const char *dict_name;
/*	DICT	*dict_info;
/*
/*	DICT	*dict_handle(dict_name)
/*	const char *dict_name;
/*
/*	void	dict_unregister(dict_name)
/*	const char *dict_name;
/*
/*	void	dict_update(dict_name, member, value)
/*	const char *dict_name;
/*	const char *member;
/*	const char *value;
/*
/*	const char *dict_lookup(dict_name, member)
/*	const char *dict_name;
/*	const char *member;
/*
/*	int	dict_delete(dict_name, member)
/*	const char *dict_name;
/*	const char *member;
/*
/*	int	dict_sequence(dict_name, func, member, value)
/*	const char *dict_name;
/*	int	func;
/*	const char **member;
/*	const char **value;
/*
/*	const char *dict_eval(dict_name, string, int recursive)
/*	const char *dict_name;
/*	const char *string;
/*	int	recursive;
/*
/*	int	dict_walk(action, context)
/*	void	(*action)(dict_name, dict_handle, context)
/*	char	*context;
/*
/*	const char *dict_changed_name()
/* AUXILIARY FUNCTIONS
/*	void	dict_load_file(dict_name, path)
/*	const char *dict_name;
/*	const char *path;
/*
/*	void	dict_load_fp(dict_name, fp)
/*	const char *dict_name;
/*	VSTREAM	*fp;
/*
/*	const char *dict_flags_str(dict_flags)
/*	int	dict_flags;
/* DESCRIPTION
/*	This module maintains a collection of name-value dictionaries.
/*	Each dictionary has its own name and has its own methods to read
/*	or update members. Examples of dictionaries that can be accessed
/*	in this manner are the global UNIX-style process environment,
/*	hash tables, NIS maps, DBM files, and so on. Dictionary values
/*	are not limited to strings but can be arbitrary objects as long
/*	as they can be represented by character pointers.
/* FEATURES
/* .fi
/* .ad
/*	Notable features of this module are:
/* .IP "macro expansion (string-valued dictionaries only)"
/*	Macros of the form $\fIname\fR can be expanded to the current
/*	value of \fIname\fR. The forms $(\fIname\fR) and ${\fIname\fR} are
/*	also supported.
/* .IP "unknown names"
/*	References to unknown dictionary or dictionary member names either
/*	default to an empty dictionary or null pointer value, or cause a
/*	run-time error. The behavior is controlled by the global
/*	\fIdict_unknown_allowed\fR boolean variable.
/* .PP
/*	dict_register() adds a new dictionary, including access methods,
/*	to the list of known dictionaries, or increments the reference
/*	count for an existing (name, dictionary) pair.  Otherwise, it is
/*	an error to pass an existing name (this would cause a memory leak).
/*
/*	dict_handle() returns the generic dictionary handle of the
/*	named dictionary, or a null pointer when the named dictionary
/*	is not found.
/*
/*	dict_unregister() decrements the reference count of the named
/*	dictionary. When the reference count reaches zero, dict_unregister()
/*	breaks the (name, dictionary) association and executes the
/*	dictionary's optional \fIremove\fR method.
/*
/*	dict_update() updates the value of the named dictionary member.
/*	The dictionary member and the named dictionary are instantiated
/*	on the fly.
/*
/*	dict_lookup() returns the value of the named member (i.e. without
/*	expanding macros in the member value).  The \fIdict_name\fR argument
/*	specifies the dictionary to search. The result is a null pointer
/*	when no value is found, otherwise the result is owned by the
/*	underlying dictionary method. Make a copy if the result is to be
/*	modified, or if the result is to survive multiple dict_lookup() calls.
/*
/*	dict_delete() removes the named member from the named dictionary.
/*	The result value is zero when the member was found.
/*
/*	dict_sequence() steps through the named dictionary and returns
/*	keys and values in some implementation-defined order. The func
/*	argument is DICT_SEQ_FUN_FIRST to set the cursor to the first
/*	entry or DICT_SEQ_FUN_NEXT to select the next entry. The result
/*	is owned by the underlying dictionary method. Make a copy if the
/*	result is to be modified, or if the result is to survive multiple
/*	dict_sequence() calls. The result value is zero when a member
/*	was found.
/*
/*	dict_eval() expands macro references in the specified string.
/*	The result is owned by the dictionary manager. Make a copy if the
/*	result is to survive multiple dict_eval() calls. When the
/*	\fIrecursive\fR argument is non-zero, macro references in macro
/*	lookup results are expanded recursively.
/*
/*	dict_walk() iterates over all registered dictionaries in some
/*	arbitrary order, and invokes the specified action routine with
/*	as arguments:
/* .IP "const char *dict_name"
/*	Dictionary name.
/* .IP "DICT *dict_handle"
/*	Generic dictionary handle.
/* .IP "char *context"
/*	Application context from the caller.
/* .PP
/*	dict_changed_name() returns non-zero when any dictionary needs to
/*	be re-opened because it has changed or because it was unlinked.
/*	A non-zero result is the name of a changed dictionary.
/*
/*	dict_load_file() reads name-value entries from the named file.
/*	Lines that begin with whitespace are concatenated to the preceding
/*	line (the newline is deleted).
/*	Each entry is stored in the dictionary named by \fIdict_name\fR.
/*
/*	dict_load_fp() reads name-value entries from an open stream.
/*	It has the same semantics as the dict_load_file() function.
/*
/*	dict_flags_str() returns a printable representation of the
/*	specified dictionary flags. The result is overwritten upon
/*	each call.
/* SEE ALSO
/*	htable(3)
/* BUGS
/* DIAGNOSTICS
/*	Fatal errors: out of memory, reference to unknown name,
/*	malformed macro name.
/*
/*	The lookup routines may set the \fIdict_errno\fR variable when
/*	they were unable to find the requested result. The lookup
/*	routines must reset \fIdict_errno\fR before each lookup operation.
/*	\fIdict_errno\fR can have the following values:
/* .IP DICT_ERR_RETRY
/*	The dictionary was temporarily unavailable. This can happen
/*	with network-based services.
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

/* System libraries. */

#include "sys_defs.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

/* Utility library. */

#include "msg.h"
#include "htable.h"
#include "mymalloc.h"
#include "vstream.h"
#include "vstring.h"
#include "readlline.h"
#include "mac_expand.h"
#include "stringops.h"
#include "iostuff.h"
#include "name_mask.h"
#include "dict.h"
#include "dict_ht.h"

 /*
  * By default, use a sane default for an unknown name.
  */
int     dict_unknown_allowed = 1;
int     dict_errno = 0;

static HTABLE *dict_table;

 /*
  * Each (name, dictionary) instance has a reference count. The count is part
  * of the name, not the dictionary. The same dictionary may be registered
  * under multiple names. The structure below keeps track of instances and
  * reference counts.
  */
typedef struct {
    DICT   *dict;
    int     refcount;
} DICT_NODE;

#define dict_node(dict) \
	(dict_table ? (DICT_NODE *) htable_find(dict_table, dict) : 0)

#define STR(x)	vstring_str(x)

/* dict_register - make association with dictionary */

void    dict_register(const char *dict_name, DICT *dict_info)
{
    const char *myname = "dict_register";
    DICT_NODE *node;

    if (dict_table == 0)
	dict_table = htable_create(0);
    if ((node = dict_node(dict_name)) == 0) {
	node = (DICT_NODE *) mymalloc(sizeof(*node));
	node->dict = dict_info;
	node->refcount = 0;
	htable_enter(dict_table, dict_name, (char *) node);
    } else if (dict_info != node->dict)
	msg_fatal("%s: dictionary name exists: %s", myname, dict_name);
    node->refcount++;
    if (msg_verbose > 1)
	msg_info("%s: %s %d", myname, dict_name, node->refcount);
}

/* dict_handle - locate generic dictionary handle */

DICT   *dict_handle(const char *dict_name)
{
    DICT_NODE *node;

    return ((node = dict_node(dict_name)) != 0 ? node->dict : 0);
}

/* dict_node_free - dict_unregister() callback */

static void dict_node_free(char *ptr)
{
    DICT_NODE *node = (DICT_NODE *) ptr;
    DICT   *dict = node->dict;

    if (dict->close)
	dict->close(dict);
    myfree((char *) node);
}

/* dict_unregister - break association with named dictionary */

void    dict_unregister(const char *dict_name)
{
    const char *myname = "dict_unregister";
    DICT_NODE *node;

    if ((node = dict_node(dict_name)) == 0)
	msg_panic("non-existing dictionary: %s", dict_name);
    if (msg_verbose > 1)
	msg_info("%s: %s %d", myname, dict_name, node->refcount);
    if (--(node->refcount) == 0)
	htable_delete(dict_table, dict_name, dict_node_free);
}

/* dict_update - replace or add dictionary entry */

void    dict_update(const char *dict_name, const char *member, const char *value)
{
    const char *myname = "dict_update";
    DICT_NODE *node;
    DICT   *dict;

    if ((node = dict_node(dict_name)) == 0) {
	if (dict_unknown_allowed == 0)
	    msg_fatal("%s: unknown dictionary: %s", myname, dict_name);
	dict = dict_ht_open(dict_name, O_CREAT | O_RDWR, 0);
	dict_register(dict_name, dict);
    } else
	dict = node->dict;
    if (msg_verbose > 1)
	msg_info("%s: %s = %s", myname, member, value);
    dict->update(dict, member, value);
}

/* dict_lookup - look up dictionary entry */

const char *dict_lookup(const char *dict_name, const char *member)
{
    const char *myname = "dict_lookup";
    DICT_NODE *node;
    DICT   *dict;
    const char *ret = 0;

    if ((node = dict_node(dict_name)) == 0) {
	if (dict_unknown_allowed == 0)
	    msg_fatal("%s: unknown dictionary: %s", myname, dict_name);
    } else {
	dict = node->dict;
	ret = dict->lookup(dict, member);
	if (ret == 0 && dict_unknown_allowed == 0)
	    msg_fatal("dictionary %s: unknown member: %s", dict_name, member);
    }
    if (msg_verbose > 1)
	msg_info("%s: %s = %s", myname, member, ret ? ret : "(notfound)");
    return (ret);
}

/* dict_delete - delete dictionary entry */

int     dict_delete(const char *dict_name, const char *member)
{
    const char *myname = "dict_delete";
    DICT_NODE *node;
    DICT   *dict;
    int     result;

    if ((node = dict_node(dict_name)) == 0) {
	if (dict_unknown_allowed == 0)
	    msg_fatal("%s: unknown dictionary: %s", myname, dict_name);
	dict = dict_ht_open(dict_name, O_CREAT | O_RDWR, 0);
	dict_register(dict_name, dict);
    } else
	dict = node->dict;
    if (msg_verbose > 1)
	msg_info("%s: delete %s", myname, member);
    if ((result = dict->delete(dict, member)) != 0 && dict_unknown_allowed == 0)
	msg_fatal("%s: dictionary %s: unknown member: %s",
		  myname, dict_name, member);
    return (result);
}

/* dict_sequence - traverse dictionary */

int     dict_sequence(const char *dict_name, const int func,
		              const char **member, const char **value)
{
    const char *myname = "dict_sequence";
    DICT_NODE *node;
    DICT   *dict;

    if ((node = dict_node(dict_name)) == 0) {
	if (dict_unknown_allowed == 0)
	    msg_fatal("%s: unknown dictionary: %s", myname, dict_name);
	dict = dict_ht_open(dict_name, O_CREAT | O_RDWR, 0);
	dict_register(dict_name, dict);
    } else
	dict = node->dict;
    if (msg_verbose > 1)
	msg_info("%s: sequence func %d", myname, func);
    return (dict->sequence(dict, func, member, value));
}

/* dict_load_file - read entries from text file */

void    dict_load_file(const char *dict_name, const char *path)
{
    VSTREAM *fp;
    struct stat st;
    time_t  before;
    time_t  after;

    /*
     * Read the file again if it is hot. This may result in reading a partial
     * parameter name when a file changes in the middle of a read.
     */
    for (before = time((time_t *) 0); /* see below */ ; before = after) {
	if ((fp = vstream_fopen(path, O_RDONLY, 0)) == 0)
	    msg_fatal("open %s: %m", path);
	dict_load_fp(dict_name, fp);
	if (fstat(vstream_fileno(fp), &st) < 0)
	    msg_fatal("fstat %s: %m", path);
	if (vstream_ferror(fp) || vstream_fclose(fp))
	    msg_fatal("read %s: %m", path);
	after = time((time_t *) 0);
	if (st.st_mtime < before - 1 || st.st_mtime > after)
	    break;
	if (msg_verbose > 1)
	    msg_info("pausing to let %s cool down", path);
	doze(300000);
    }
}

/* dict_load_fp - read entries from open stream */

void    dict_load_fp(const char *dict_name, VSTREAM *fp)
{
    VSTRING *buf;
    char   *member;
    char   *val;
    int     lineno;
    const char *err;

    buf = vstring_alloc(100);
    lineno = 0;

    while (readlline(buf, fp, &lineno)) {
	if ((err = split_nameval(STR(buf), &member, &val)) != 0)
	    msg_fatal("%s, line %d: %s: \"%s\"",
		      VSTREAM_PATH(fp), lineno, err, STR(buf));
	dict_update(dict_name, member, val);
    }
    vstring_free(buf);
}

/* dict_eval_lookup - macro parser call-back routine */

static const char *dict_eval_lookup(const char *key, int unused_type,
				            char *dict_name)
{
    const char *pp;

    /*
     * XXX how would one recover?
     */
    if ((pp = dict_lookup(dict_name, key)) == 0 && dict_errno != 0)
	msg_fatal("dictionary %s: lookup %s: temporary error", dict_name, key);

    return (pp);
}

/* dict_eval - expand embedded dictionary references */

const char *dict_eval(const char *dict_name, const char *value, int recursive)
{
    const char *myname = "dict_eval";
    static VSTRING *buf;
    int     status;

    /*
     * Initialize.
     */
    if (buf == 0)
	buf = vstring_alloc(10);

    /*
     * Expand macros, possibly recursively.
     */
#define DONT_FILTER (char *) 0

    status = mac_expand(buf, value,
			recursive ? MAC_EXP_FLAG_RECURSE : MAC_EXP_FLAG_NONE,
			DONT_FILTER, dict_eval_lookup, (char *) dict_name);
    if (status & MAC_PARSE_ERROR)
	msg_fatal("dictionary %s: macro processing error", dict_name);
    if (msg_verbose > 1) {
	if (strcmp(value, STR(buf)) != 0)
	    msg_info("%s: expand %s -> %s", myname, value, STR(buf));
	else
	    msg_info("%s: const  %s", myname, value);
    }
    return (STR(buf));
}

/* dict_walk - iterate over all dictionaries in arbitrary order */

void    dict_walk(DICT_WALK_ACTION action, char *ptr)
{
    HTABLE_INFO **ht_info_list;
    HTABLE_INFO **ht;
    HTABLE_INFO *h;

    ht_info_list = htable_list(dict_table);
    for (ht = ht_info_list; (h = *ht) != 0; ht++)
	action(h->key, (DICT *) h->value, ptr);
    myfree((char *) ht_info_list);
}

/* dict_changed_name - see if any dictionary has changed */

const char *dict_changed_name(void)
{
    const char *myname = "dict_changed_name";
    struct stat st;
    HTABLE_INFO **ht_info_list;
    HTABLE_INFO **ht;
    HTABLE_INFO *h;
    const char *status;
    DICT   *dict;

    ht_info_list = htable_list(dict_table);
    for (status = 0, ht = ht_info_list; status == 0 && (h = *ht) != 0; ht++) {
	dict = ((DICT_NODE *) h->value)->dict;
	if (dict->stat_fd < 0)			/* not file-based */
	    continue;
	if (dict->mtime == 0)			/* not bloody likely */
	    msg_warn("%s: table %s: null time stamp", myname, h->key);
	if (fstat(dict->stat_fd, &st) < 0)
	    msg_fatal("%s: fstat: %m", myname);
	if (st.st_mtime != dict->mtime || st.st_nlink == 0)
	    status = h->key;
    }
    myfree((char *) ht_info_list);
    return (status);
}

/* dict_changed - backwards compatibility */

int     dict_changed(void)
{
    return (dict_changed_name() != 0);
}

 /*
  * Mapping between flag names and flag values.
  */
static const NAME_MASK dict_mask[] = {
    "warn_dup", (1 << 0),		/* if file, warn about dups */
    "ignore_dup", (1 << 1),		/* if file, ignore dups */
    "try0null", (1 << 2),		/* do not append 0 to key/value */
    "try1null", (1 << 3),		/* append 0 to key/value */
    "fixed", (1 << 4),			/* fixed key map */
    "pattern", (1 << 5),		/* keys are patterns */
    "lock", (1 << 6),			/* lock before access */
    "replace", (1 << 7),		/* if file, replace dups */
    "sync_update", (1 << 8),		/* if file, sync updates */
    "debug", (1 << 9),			/* log access */
    "no_regsub", (1 << 11),		/* disallow regexp substitution */
    "no_proxy", (1 << 12),		/* disallow proxy mapping */
    "no_unauth", (1 << 13),		/* disallow unauthenticated data */
    "fold_fix", (1 << 14),		/* case-fold with fixed-case key map */
    "fold_mul", (1 << 15),		/* case-fold with multi-case key map */
    "open_lock", (1 << 16),		/* permanent lock upon open */
    0,
};

/* dict_flags_str - convert mask to string for debugging purposes */

const char *dict_flags_str(int dict_flags)
{
    static VSTRING *buf = 0;

    if (buf == 0)
	buf = vstring_alloc(1);

    return (str_name_mask_opt(buf, "dictionary flags", dict_mask, dict_flags,
			      NAME_MASK_RETURN | NAME_MASK_PIPE));
}
