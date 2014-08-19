/*	$NetBSD: dict_memcache.c,v 1.1.1.1.6.3 2014/08/19 23:59:42 tls Exp $	*/

/*++
/* NAME
/*	dict_memcache 3
/* SUMMARY
/*	dictionary interface to memcaches
/* SYNOPSIS
/*	#include <dict_memcache.h>
/*
/*	DICT	*dict_memcache_open(name, open_flags, dict_flags)
/*	const char *name;
/*	int	open_flags;
/*	int	dict_flags;
/* DESCRIPTION
/*	dict_memcache_open() opens a memcache, providing
/*	a dictionary interface for Postfix key->value mappings.
/*	The result is a pointer to the installed dictionary.
/*
/*	Configuration parameters are described in memcache_table(5).
/*
/*	Arguments:
/* .IP name
/*	The path to the Postfix memcache configuration file.
/* .IP open_flags
/*	O_RDONLY or O_RDWR. This function ignores flags that don't
/*	specify a read, write or append mode.
/* .IP dict_flags
/*	See dict_open(3).
/* SEE ALSO
/*	dict(3) generic dictionary manager
/* HISTORY
/* .ad
/* .fi
/*	The first memcache client for Postfix was written by Omar
/*	Kilani, and was based on libmemcache.  The current
/*	implementation implements the memcache protocol directly,
/*	and bears no resemblance to earlier work.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>			/* XXX sscanf() */

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <dict.h>
#include <vstring.h>
#include <stringops.h>
#include <auto_clnt.h>
#include <vstream.h>

/* Global library. */

#include <cfg_parser.h>
#include <db_common.h>
#include <memcache_proto.h>

/* Application-specific. */

#include <dict_memcache.h>

 /*
  * Structure of one memcache dictionary handle.
  */
typedef struct {
    DICT    dict;			/* parent class */
    CFG_PARSER *parser;			/* common parameter parser */
    void   *dbc_ctxt;			/* db_common context */
    char   *key_format;			/* query key translation */
    int     timeout;			/* client timeout */
    int     mc_ttl;			/* memcache update expiration */
    int     mc_flags;			/* memcache update flags */
    int     err_pause;			/* delay between errors */
    int     max_tries;			/* number of tries */
    int     max_line;			/* reply line limit */
    int     max_data;			/* reply data limit */
    char   *memcache;			/* memcache server spec */
    AUTO_CLNT *clnt;			/* memcache client stream */
    VSTRING *clnt_buf;			/* memcache client buffer */
    VSTRING *key_buf;			/* lookup key */
    VSTRING *res_buf;			/* lookup result */
    int     error;			/* memcache dict_errno */
    DICT   *backup;			/* persistent backup */
} DICT_MC;

 /*
  * Memcache option defaults and names.
  */
#define DICT_MC_DEF_HOST	"localhost"
#define DICT_MC_DEF_PORT	"11211"
#define DICT_MC_DEF_MEMCACHE	"inet:" DICT_MC_DEF_HOST ":" DICT_MC_DEF_PORT
#define DICT_MC_DEF_KEY_FMT	"%s"
#define DICT_MC_DEF_MC_TTL	3600
#define DICT_MC_DEF_MC_TIMEOUT	2
#define DICT_MC_DEF_MC_FLAGS	0
#define DICT_MC_DEF_MAX_TRY	2
#define DICT_MC_DEF_MAX_LINE	1024
#define DICT_MC_DEF_MAX_DATA	10240
#define DICT_MC_DEF_ERR_PAUSE	1

#define DICT_MC_NAME_MEMCACHE	"memcache"
#define DICT_MC_NAME_BACKUP	"backup"
#define DICT_MC_NAME_KEY_FMT	"key_format"
#define DICT_MC_NAME_MC_TTL	"ttl"
#define DICT_MC_NAME_MC_TIMEOUT	"timeout"
#define DICT_MC_NAME_MC_FLAGS	"flags"
#define DICT_MC_NAME_MAX_TRY	"max_try"
#define DICT_MC_NAME_MAX_LINE	"line_size_limit"
#define DICT_MC_NAME_MAX_DATA	"data_size_limit"
#define DICT_MC_NAME_ERR_PAUSE	"retry_pause"

 /*
  * SLMs.
  */
#define STR(x)	vstring_str(x)
#define LEN(x)	VSTRING_LEN(x)

/*#define msg_verbose 1*/

/* dict_memcache_set - set memcache key/value */

static int dict_memcache_set(DICT_MC *dict_mc, const char *value, int ttl)
{
    VSTREAM *fp;
    int     count;
    int     data_len = strlen(value);

    /*
     * Return a permanent error if we can't store this data. This results in
     * loss of information.
     */
    if (data_len > dict_mc->max_data) {
	msg_warn("database %s:%s: data for key %s is too long (%s=%d) "
		 "-- not stored", DICT_TYPE_MEMCACHE, dict_mc->dict.name,
		 STR(dict_mc->key_buf), DICT_MC_NAME_MAX_DATA,
		 dict_mc->max_data);
	/* Not stored! */
	DICT_ERR_VAL_RETURN(dict_mc, DICT_ERR_NONE, DICT_STAT_FAIL);
    }
    for (count = 0; count < dict_mc->max_tries; count++) {
	if (count > 0)
	    sleep(dict_mc->err_pause);
	if ((fp = auto_clnt_access(dict_mc->clnt)) == 0) {
	    break;
	} else if (memcache_printf(fp, "set %s %d %d %ld",
		STR(dict_mc->key_buf), dict_mc->mc_flags, ttl, data_len) < 0
		   || memcache_fwrite(fp, value, strlen(value)) < 0
		   || memcache_get(fp, dict_mc->clnt_buf,
				   dict_mc->max_line) < 0) {
	    if (count > 0)
		msg_warn(errno ? "database %s:%s: I/O error: %m" :
			 "database %s:%s: I/O error",
			 DICT_TYPE_MEMCACHE, dict_mc->dict.name);
	} else if (strcmp(STR(dict_mc->clnt_buf), "STORED") != 0) {
	    if (count > 0)
		msg_warn("database %s:%s: update failed: %.30s",
			 DICT_TYPE_MEMCACHE, dict_mc->dict.name,
			 STR(dict_mc->clnt_buf));
	} else {
	    /* Victory! */
	    DICT_ERR_VAL_RETURN(dict_mc, DICT_ERR_NONE, DICT_STAT_SUCCESS);
	}
	auto_clnt_recover(dict_mc->clnt);
    }
    DICT_ERR_VAL_RETURN(dict_mc, DICT_ERR_RETRY, DICT_STAT_ERROR);
}

/* dict_memcache_get - get memcache key/value */

static const char *dict_memcache_get(DICT_MC *dict_mc)
{
    VSTREAM *fp;
    long    todo;
    int     count;

    for (count = 0; count < dict_mc->max_tries; count++) {
	if (count > 0)
	    sleep(dict_mc->err_pause);
	if ((fp = auto_clnt_access(dict_mc->clnt)) == 0) {
	    break;
	} else if (memcache_printf(fp, "get %s", STR(dict_mc->key_buf)) < 0
	    || memcache_get(fp, dict_mc->clnt_buf, dict_mc->max_line) < 0) {
	    if (count > 0)
		msg_warn(errno ? "database %s:%s: I/O error: %m" :
			 "database %s:%s: I/O error",
			 DICT_TYPE_MEMCACHE, dict_mc->dict.name);
	} else if (strcmp(STR(dict_mc->clnt_buf), "END") == 0) {
	    /* Not found. */
	    DICT_ERR_VAL_RETURN(dict_mc, DICT_ERR_NONE, (char *) 0);
	} else if (sscanf(STR(dict_mc->clnt_buf),
			  "VALUE %*s %*s %ld", &todo) != 1
		   || todo < 0 || todo > dict_mc->max_data) {
	    if (count > 0)
		msg_warn("%s: unexpected memcache server reply: %.30s",
			 dict_mc->dict.name, STR(dict_mc->clnt_buf));
	} else if (memcache_fread(fp, dict_mc->res_buf, todo) < 0) {
	    if (count > 0)
		msg_warn("%s: EOF receiving memcache server reply",
			 dict_mc->dict.name);
	} else {
	    /* Victory! */
	    if (memcache_get(fp, dict_mc->clnt_buf, dict_mc->max_line) < 0
		|| strcmp(STR(dict_mc->clnt_buf), "END") != 0)
		auto_clnt_recover(dict_mc->clnt);
	    DICT_ERR_VAL_RETURN(dict_mc, DICT_ERR_NONE, STR(dict_mc->res_buf));
	}
	auto_clnt_recover(dict_mc->clnt);
    }
    DICT_ERR_VAL_RETURN(dict_mc, DICT_ERR_RETRY, (char *) 0);
}

/* dict_memcache_del - delete memcache key/value */

static int dict_memcache_del(DICT_MC *dict_mc)
{
    VSTREAM *fp;
    int     count;

    for (count = 0; count < dict_mc->max_tries; count++) {
	if (count > 0)
	    sleep(dict_mc->err_pause);
	if ((fp = auto_clnt_access(dict_mc->clnt)) == 0) {
	    break;
	} else if (memcache_printf(fp, "delete %s", STR(dict_mc->key_buf)) < 0
	    || memcache_get(fp, dict_mc->clnt_buf, dict_mc->max_line) < 0) {
	    if (count > 0)
		msg_warn(errno ? "database %s:%s: I/O error: %m" :
			 "database %s:%s: I/O error",
			 DICT_TYPE_MEMCACHE, dict_mc->dict.name);
	} else if (strcmp(STR(dict_mc->clnt_buf), "DELETED") == 0) {
	    /* Victory! */
	    DICT_ERR_VAL_RETURN(dict_mc, DICT_ERR_NONE, DICT_STAT_SUCCESS);
	} else if (strcmp(STR(dict_mc->clnt_buf), "NOT_FOUND") == 0) {
	    /* Not found! */
	    DICT_ERR_VAL_RETURN(dict_mc, DICT_ERR_NONE, DICT_STAT_FAIL);
	} else {
	    if (count > 0)
		msg_warn("database %s:%s: delete failed: %.30s",
			 DICT_TYPE_MEMCACHE, dict_mc->dict.name,
			 STR(dict_mc->clnt_buf));
	}
	auto_clnt_recover(dict_mc->clnt);
    }
    DICT_ERR_VAL_RETURN(dict_mc, DICT_ERR_RETRY, DICT_STAT_ERROR);
}

/* dict_memcache_prepare_key - prepare lookup key */

static int dict_memcache_prepare_key(DICT_MC *dict_mc, const char *name)
{

    /*
     * Optionally case-fold the search string.
     */
    if (dict_mc->dict.flags & DICT_FLAG_FOLD_FIX) {
	if (dict_mc->dict.fold_buf == 0)
	    dict_mc->dict.fold_buf = vstring_alloc(10);
	vstring_strcpy(dict_mc->dict.fold_buf, name);
	name = lowercase(STR(dict_mc->dict.fold_buf));
    }

    /*
     * Optionally expand the query key format.
     */
#define DICT_MC_NO_KEY		(0)
#define DICT_MC_NO_QUOTING	((void (*)(DICT *, const char *, VSTRING *)) 0)

    if (dict_mc->key_format != 0
	&& strcmp(dict_mc->key_format, DICT_MC_DEF_KEY_FMT) != 0) {
	VSTRING_RESET(dict_mc->key_buf);
	if (db_common_expand(dict_mc->dbc_ctxt, dict_mc->key_format,
			     name, DICT_MC_NO_KEY, dict_mc->key_buf,
			     DICT_MC_NO_QUOTING) == 0)
	    return (0);
    } else {
	vstring_strcpy(dict_mc->key_buf, name);
    }

    /*
     * The length indicates whether the expansion is empty or not.
     */
    return (LEN(dict_mc->key_buf));
}

/* dict_memcache_valid_key - validate key */

static int dict_memcache_valid_key(DICT_MC *dict_mc,
				           const char *name,
				           const char *operation,
				        void (*log_func) (const char *,...))
{
    unsigned char *cp;
    int     rc;

#define DICT_MC_SKIP(why) do { \
	if (msg_verbose || log_func != msg_info) \
	    log_func("%s: skipping %s for name \"%s\": %s", \
		     dict_mc->dict.name, operation, name, (why)); \
	DICT_ERR_VAL_RETURN(dict_mc, DICT_ERR_NONE, 0); \
    } while (0)

    if (*name == 0)
	DICT_MC_SKIP("empty lookup key");
    if ((rc = db_common_check_domain(dict_mc->dbc_ctxt, name)) == 0)
	DICT_MC_SKIP("domain mismatch");
    if (rc < 0)
	DICT_ERR_VAL_RETURN(dict_mc, rc, 0);
    if (dict_memcache_prepare_key(dict_mc, name) == 0)
	DICT_MC_SKIP("empty lookup key expansion");
    for (cp = (unsigned char *) STR(dict_mc->key_buf); *cp; cp++)
	if (isascii(*cp) && isspace(*cp))
	    DICT_MC_SKIP("name contains space");

    DICT_ERR_VAL_RETURN(dict_mc, DICT_ERR_NONE, 1);
}

/* dict_memcache_update - update memcache */

static int dict_memcache_update(DICT *dict, const char *name,
				        const char *value)
{
    const char *myname = "dict_memcache_update";
    DICT_MC *dict_mc = (DICT_MC *) dict;
    DICT   *backup = dict_mc->backup;
    int     upd_res;

    /*
     * Skip updates with an inapplicable key, noisily. This results in loss
     * of information.
     */
    if (dict_memcache_valid_key(dict_mc, name, "update", msg_warn) == 0)
	DICT_ERR_VAL_RETURN(dict, dict_mc->error, DICT_STAT_FAIL);

    /*
     * Update the memcache first.
     */
    upd_res = dict_memcache_set(dict_mc, value, dict_mc->mc_ttl);
    dict->error = dict_mc->error;

    /*
     * Update the backup database last.
     */
    if (backup) {
	upd_res = backup->update(backup, name, value);
	dict->error = backup->error;
    }
    if (msg_verbose)
	msg_info("%s: %s: update key \"%s\"(%s) => \"%s\" %s",
		 myname, dict_mc->dict.name, name, STR(dict_mc->key_buf),
		 value, dict_mc->error ? "(memcache error)" : (backup
		       && backup->error) ? "(backup error)" : "(no error)");

    return (upd_res);
}

/* dict_memcache_lookup - lookup memcache */

static const char *dict_memcache_lookup(DICT *dict, const char *name)
{
    const char *myname = "dict_memcache_lookup";
    DICT_MC *dict_mc = (DICT_MC *) dict;
    DICT   *backup = dict_mc->backup;
    const char *retval;

    /*
     * Skip lookups with an inapplicable key, silently. This is just asking
     * for information that cannot exist.
     */
    if (dict_memcache_valid_key(dict_mc, name, "lookup", msg_info) == 0)
	DICT_ERR_VAL_RETURN(dict, dict_mc->error, (char *) 0);

    /*
     * Search the memcache first.
     */
    retval = dict_memcache_get(dict_mc);
    dict->error = dict_mc->error;

    /*
     * Search the backup database last. Update the memcache if the data is
     * found.
     */
    if (backup) {
	backup->error = 0;
	if (retval == 0) {
	    retval = backup->lookup(backup, name);
	    dict->error = backup->error;
	    /* Update the cache. */
	    if (retval != 0)
		dict_memcache_set(dict_mc, retval, dict_mc->mc_ttl);
	}
    }
    if (msg_verbose)
	msg_info("%s: %s: key \"%s\"(%s) => %s",
		 myname, dict_mc->dict.name, name, STR(dict_mc->key_buf),
		 retval ? retval : dict_mc->error ? "(memcache error)" :
	      (backup && backup->error) ? "(backup error)" : "(not found)");

    return (retval);
}

/* dict_memcache_delete - delete memcache entry */

static int dict_memcache_delete(DICT *dict, const char *name)
{
    const char *myname = "dict_memcache_delete";
    DICT_MC *dict_mc = (DICT_MC *) dict;
    DICT   *backup = dict_mc->backup;
    int     del_res;

    /*
     * Skip lookups with an inapplicable key, noisily. This is just deleting
     * information that cannot exist.
     */
    if (dict_memcache_valid_key(dict_mc, name, "delete", msg_info) == 0)
	DICT_ERR_VAL_RETURN(dict, dict_mc->error, dict_mc->error ?
			    DICT_STAT_ERROR : DICT_STAT_FAIL);

    /*
     * Update the memcache first.
     */
    del_res = dict_memcache_del(dict_mc);
    dict->error = dict_mc->error;

    /*
     * Update the persistent database last.
     */
    if (backup) {
	del_res = backup->delete(backup, name);
	dict->error = backup->error;
    }
    if (msg_verbose)
	msg_info("%s: %s: delete key \"%s\"(%s) => %s",
		 myname, dict_mc->dict.name, name, STR(dict_mc->key_buf),
		 dict_mc->error ? "(memcache error)" : (backup
		       && backup->error) ? "(backup error)" : "(no error)");

    return (del_res);
}

/* dict_memcache_sequence - first/next lookup */

static int dict_memcache_sequence(DICT *dict, int function, const char **key,
				          const char **value)
{
    const char *myname = "dict_memcache_sequence";
    DICT_MC *dict_mc = (DICT_MC *) dict;
    DICT   *backup = dict_mc->backup;
    int     seq_res;

    if (backup == 0) {
	msg_warn("database %s:%s: first/next support requires backup database",
		 DICT_TYPE_MEMCACHE, dict_mc->dict.name);
	DICT_ERR_VAL_RETURN(dict, DICT_ERR_NONE, DICT_STAT_FAIL);
    } else {
	seq_res = backup->sequence(backup, function, key, value);
	if (msg_verbose)
	    msg_info("%s: %s: key \"%s\" => %s",
		     myname, dict_mc->dict.name, *key ? *key : "(not found)",
		     *value ? *value : backup->error ? "(backup error)" :
		     "(not found)");
	DICT_ERR_VAL_RETURN(dict, backup->error, seq_res);
    }
}

/* dict_memcache_close - close memcache */

static void dict_memcache_close(DICT *dict)
{
    DICT_MC *dict_mc = (DICT_MC *) dict;

    cfg_parser_free(dict_mc->parser);
    db_common_free_ctx(dict_mc->dbc_ctxt);
    if (dict_mc->key_format)
	myfree(dict_mc->key_format);
    myfree(dict_mc->memcache);
    auto_clnt_free(dict_mc->clnt);
    vstring_free(dict_mc->clnt_buf);
    vstring_free(dict_mc->key_buf);
    vstring_free(dict_mc->res_buf);
    if (dict->fold_buf)
	vstring_free(dict->fold_buf);
    if (dict_mc->backup)
	dict_close(dict_mc->backup);
    dict_free(dict);
}

/* dict_memcache_open - open memcache */

DICT   *dict_memcache_open(const char *name, int open_flags, int dict_flags)
{
    DICT_MC *dict_mc;
    char   *backup;
    CFG_PARSER *parser;

    /*
     * Sanity checks.
     */
    if (dict_flags & DICT_FLAG_NO_UNAUTH)
	return (dict_surrogate(DICT_TYPE_MEMCACHE, name, open_flags, dict_flags,
		     "%s:%s map is not allowed for security-sensitive data",
			       DICT_TYPE_MEMCACHE, name));
    open_flags &= (O_RDONLY | O_RDWR | O_WRONLY | O_APPEND);
    if (open_flags != O_RDONLY && open_flags != O_RDWR)
	return (dict_surrogate(DICT_TYPE_MEMCACHE, name, open_flags, dict_flags,
			"%s:%s map requires O_RDONLY or O_RDWR access mode",
			       DICT_TYPE_MEMCACHE, name));

    /*
     * Open the configuration file.
     */
    if ((parser = cfg_parser_alloc(name)) == 0)
	return (dict_surrogate(DICT_TYPE_MEMCACHE, name, open_flags, dict_flags,
			       "open %s: %m", name));

    /*
     * Create the dictionary object.
     */
    dict_mc = (DICT_MC *) dict_alloc(DICT_TYPE_MEMCACHE, name,
				     sizeof(*dict_mc));
    dict_mc->dict.lookup = dict_memcache_lookup;
    if (open_flags == O_RDWR) {
	dict_mc->dict.update = dict_memcache_update;
	dict_mc->dict.delete = dict_memcache_delete;
    }
    dict_mc->dict.sequence = dict_memcache_sequence;
    dict_mc->dict.close = dict_memcache_close;
    dict_mc->dict.flags = dict_flags;
    dict_mc->key_buf = vstring_alloc(10);
    dict_mc->res_buf = vstring_alloc(10);

    /*
     * Parse the configuration file.
     */
    dict_mc->parser = parser;
    dict_mc->key_format = cfg_get_str(dict_mc->parser, DICT_MC_NAME_KEY_FMT,
				      DICT_MC_DEF_KEY_FMT, 0, 0);
    dict_mc->timeout = cfg_get_int(dict_mc->parser, DICT_MC_NAME_MC_TIMEOUT,
				   DICT_MC_DEF_MC_TIMEOUT, 0, 0);
    dict_mc->mc_ttl = cfg_get_int(dict_mc->parser, DICT_MC_NAME_MC_TTL,
				  DICT_MC_DEF_MC_TTL, 0, 0);
    dict_mc->mc_flags = cfg_get_int(dict_mc->parser, DICT_MC_NAME_MC_FLAGS,
				    DICT_MC_DEF_MC_FLAGS, 0, 0);
    dict_mc->err_pause = cfg_get_int(dict_mc->parser, DICT_MC_NAME_ERR_PAUSE,
				     DICT_MC_DEF_ERR_PAUSE, 1, 0);
    dict_mc->max_tries = cfg_get_int(dict_mc->parser, DICT_MC_NAME_MAX_TRY,
				     DICT_MC_DEF_MAX_TRY, 1, 0);
    dict_mc->max_line = cfg_get_int(dict_mc->parser, DICT_MC_NAME_MAX_LINE,
				    DICT_MC_DEF_MAX_LINE, 1, 0);
    dict_mc->max_data = cfg_get_int(dict_mc->parser, DICT_MC_NAME_MAX_DATA,
				    DICT_MC_DEF_MAX_DATA, 1, 0);
    dict_mc->memcache = cfg_get_str(dict_mc->parser, DICT_MC_NAME_MEMCACHE,
				    DICT_MC_DEF_MEMCACHE, 0, 0);

    /*
     * Initialize the memcache client.
     */
    dict_mc->clnt = auto_clnt_create(dict_mc->memcache, dict_mc->timeout, 0, 0);
    dict_mc->clnt_buf = vstring_alloc(100);

    /*
     * Open the optional backup database.
     */
    backup = cfg_get_str(dict_mc->parser, DICT_MC_NAME_BACKUP,
			 (char *) 0, 0, 0);
    if (backup) {
	dict_mc->backup = dict_open(backup, open_flags, dict_flags);
	myfree(backup);
    } else
	dict_mc->backup = 0;

    /*
     * Parse templates and common database parameters. Maps that use
     * substring keys should only be used with the full input key.
     */
    dict_mc->dbc_ctxt = 0;
    db_common_parse(&dict_mc->dict, &dict_mc->dbc_ctxt,
		    dict_mc->key_format, 1);
    db_common_parse_domain(dict_mc->parser, dict_mc->dbc_ctxt);
    if (db_common_dict_partial(dict_mc->dbc_ctxt))
	/* Breaks recipient delimiters */
	dict_mc->dict.flags |= DICT_FLAG_PATTERN;
    else
	dict_mc->dict.flags |= DICT_FLAG_FIXED;

    dict_mc->dict.flags |= DICT_FLAG_MULTI_WRITER;

    return (&dict_mc->dict);
}
