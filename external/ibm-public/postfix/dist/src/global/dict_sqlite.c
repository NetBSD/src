/*	$NetBSD: dict_sqlite.c,v 1.1.1.2.2.1 2013/02/25 00:27:17 tls Exp $	*/

/*++
/* NAME
/*	dict_sqlite 3
/* SUMMARY
/*	dictionary manager interface to SQLite3 databases
/* SYNOPSIS
/*	#include <dict_sqlite.h>
/*
/*	DICT	*dict_sqlite_open(name, open_flags, dict_flags)
/*	const char *name;
/*	int	open_flags;
/*	int	dict_flags;
/* DESCRIPTION
/*	dict_sqlite_open() creates a dictionary of type 'sqlite'.
/*	This dictionary is an interface for the postfix key->value
/*	mappings to SQLite.  The result is a pointer to the installed
/*	dictionary.
/* .PP
/*	Arguments:
/* .IP name
/*	Either the path to the SQLite configuration file (if it
/*	starts with '/' or '.'), or the prefix which will be used
/*	to obtain main.cf configuration parameters for this search.
/*
/*	In the first case, the configuration parameters below are
/*	specified in the file as \fIname\fR=\fIvalue\fR pairs.
/*
/*	In the second case, the configuration parameters are prefixed
/*	with the value of \fIname\fR and an underscore, and they
/*	are specified in main.cf.  For example, if this value is
/*	\fIsqlitecon\fR, the parameters would look like
/*	\fIsqlitecon_dbpath\fR, \fIsqlitecon_query\fR, and so on.
/* .IP open_flags
/*	Must be O_RDONLY.
/* .IP dict_flags
/*	See dict_open(3).
/* .PP
/*	Configuration parameters:
/* .IP dbpath
/*	Path to SQLite database
/* .IP query
/*	Query template. Before the query is actually issued, variable
/*	substitutions are performed. See sqlite_table(5) for details.
/* .IP result_format
/*	The format used to expand results from queries.  Substitutions
/*	are performed as described in sqlite_table(5). Defaults to
/*	returning the lookup result unchanged.
/* .IP expansion_limit
/*	Limit (if any) on the total number of lookup result values.
/*	Lookups which exceed the limit fail with dict->error=DICT_ERR_RETRY.
/*	Note that each non-empty (and non-NULL) column of a
/*	multi-column result row counts as one result.
/* .IP "select_field, where_field, additional_conditions"
/*	Legacy query interface.
/* SEE ALSO
/*	dict(3) generic dictionary manager
/* AUTHOR(S)
/*	Axel Steiner
/*	ast@treibsand.com
/*
/*	Adopted and updated by:
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <string.h>

#ifdef HAS_SQLITE
#include <sqlite3.h>

#if !defined(SQLITE_VERSION_NUMBER) || (SQLITE_VERSION_NUMBER < 3005004)
#define sqlite3_prepare_v2 sqlite3_prepare
#endif

/* Utility library. */

#include <msg.h>
#include <dict.h>
#include <vstring.h>
#include <stringops.h>
#include <mymalloc.h>

/* Global library. */

#include <cfg_parser.h>
#include <db_common.h>

/* Application-specific. */

#include <dict_sqlite.h>

typedef struct {
    DICT    dict;			/* generic member */
    CFG_PARSER *parser;			/* common parameter parser */
    sqlite3 *db;			/* sqlite handle */
    char   *query;			/* db_common_expand() query */
    char   *result_format;		/* db_common_expand() result_format */
    void   *ctx;			/* db_common_parse() context */
    char   *dbpath;			/* dbpath config attribute */
    int     expansion_limit;		/* expansion_limit config attribute */
} DICT_SQLITE;

/* dict_sqlite_quote - escape SQL metacharacters in input string */

static void dict_sqlite_quote(DICT *dict, const char *raw_text, VSTRING *result)
{
    char   *quoted_text;

    quoted_text = sqlite3_mprintf("%q", raw_text);
    /* Fix 20100616 */
    if (quoted_text == 0)
	msg_fatal("dict_sqlite_quote: out of memory");
    vstring_strcat(result, quoted_text);
    sqlite3_free(quoted_text);
}

/* dict_sqlite_close - close the database */

static void dict_sqlite_close(DICT *dict)
{
    const char *myname = "dict_sqlite_close";
    DICT_SQLITE *dict_sqlite = (DICT_SQLITE *) dict;

    if (msg_verbose)
	msg_info("%s: %s", myname, dict_sqlite->parser->name);

    if (sqlite3_close(dict_sqlite->db) != SQLITE_OK)
	msg_fatal("%s: close %s failed", myname, dict_sqlite->parser->name);
    cfg_parser_free(dict_sqlite->parser);
    myfree(dict_sqlite->dbpath);
    myfree(dict_sqlite->query);
    myfree(dict_sqlite->result_format);
    if (dict_sqlite->ctx)
	db_common_free_ctx(dict_sqlite->ctx);
    if (dict->fold_buf)
	vstring_free(dict->fold_buf);
    dict_free(dict);
}

/* dict_sqlite_lookup - find database entry */

static const char *dict_sqlite_lookup(DICT *dict, const char *name)
{
    const char *myname = "dict_sqlite_lookup";
    DICT_SQLITE *dict_sqlite = (DICT_SQLITE *) dict;
    sqlite3_stmt *sql_stmt;
    const char *query_remainder;
    static VSTRING *query;
    static VSTRING *result;
    const char *retval;
    int     expansion = 0;
    int     status;
    int     domain_rc;

    /*
     * In case of return without lookup (skipped key, etc.).
     */
    dict->error = 0;

    /*
     * Don't frustrate future attempts to make Postfix UTF-8 transparent.
     */
    if (!valid_utf_8(name, strlen(name))) {
	if (msg_verbose)
	    msg_info("%s: %s: Skipping lookup of non-UTF-8 key '%s'",
		     myname, dict_sqlite->parser->name, name);
	return (0);
    }

    /*
     * Optionally fold the key. Folding may be enabled on on-the-fly.
     */
    if (dict->flags & DICT_FLAG_FOLD_FIX) {
	if (dict->fold_buf == 0)
	    dict->fold_buf = vstring_alloc(100);
	vstring_strcpy(dict->fold_buf, name);
	name = lowercase(vstring_str(dict->fold_buf));
    }

    /*
     * Apply the optional domain filter for email address lookups.
     */
    if ((domain_rc = db_common_check_domain(dict_sqlite->ctx, name)) == 0) {
	if (msg_verbose)
	    msg_info("%s: %s: Skipping lookup of '%s'",
		     myname, dict_sqlite->parser->name, name);
	return (0);
    }
    if (domain_rc < 0)
	DICT_ERR_VAL_RETURN(dict, domain_rc, (char *) 0);

    /*
     * Expand the query and query the database.
     */
#define INIT_VSTR(buf, len) do { \
	if (buf == 0) \
		buf = vstring_alloc(len); \
	VSTRING_RESET(buf); \
	VSTRING_TERMINATE(buf); \
    } while (0)

    INIT_VSTR(query, 10);

    if (!db_common_expand(dict_sqlite->ctx, dict_sqlite->query,
			  name, 0, query, dict_sqlite_quote))
	return (0);

    if (msg_verbose)
	msg_info("%s: %s: Searching with query %s",
		 myname, dict_sqlite->parser->name, vstring_str(query));

    if (sqlite3_prepare_v2(dict_sqlite->db, vstring_str(query), -1,
			   &sql_stmt, &query_remainder) != SQLITE_OK)
	msg_fatal("%s: %s: SQL prepare failed: %s\n",
		  myname, dict_sqlite->parser->name,
		  sqlite3_errmsg(dict_sqlite->db));

    if (*query_remainder && msg_verbose)
	msg_info("%s: %s: Ignoring text at end of query: %s",
		 myname, dict_sqlite->parser->name, query_remainder);

    /*
     * Retrieve and expand the result(s).
     */
    INIT_VSTR(result, 10);
    while ((status = sqlite3_step(sql_stmt)) != SQLITE_DONE) {
	if (status == SQLITE_ROW) {
	    if (db_common_expand(dict_sqlite->ctx, dict_sqlite->result_format,
				 (char *) sqlite3_column_text(sql_stmt, 0),
				 name, result, 0)
		&& dict_sqlite->expansion_limit > 0
		&& ++expansion > dict_sqlite->expansion_limit) {
		msg_warn("%s: %s: Expansion limit exceeded for key '%s'",
			 myname, dict_sqlite->parser->name, name);
		dict->error = DICT_ERR_RETRY;
		break;
	    }
	}
	/* Fix 20100616 */
	else {
	    msg_warn("%s: %s: SQL step failed for query '%s': %s\n",
		     myname, dict_sqlite->parser->name,
		     vstring_str(query), sqlite3_errmsg(dict_sqlite->db));
	    dict->error = DICT_ERR_RETRY;
	    break;
	}
    }

    /*
     * Clean up.
     */
    if (sqlite3_finalize(sql_stmt))
	msg_fatal("%s: %s: SQL finalize failed for query '%s': %s\n",
		  myname, dict_sqlite->parser->name,
		  vstring_str(query), sqlite3_errmsg(dict_sqlite->db));

    return ((dict->error == 0 && *(retval = vstring_str(result)) != 0) ?
	    retval : 0);
}

/* sqlite_parse_config - parse sqlite configuration file */

static void sqlite_parse_config(DICT_SQLITE *dict_sqlite, const char *sqlitecf)
{
    VSTRING *buf;

    /*
     * Parse the primary configuration parameters, and emulate the legacy
     * query interface if necessary. This simplifies migration from one SQL
     * database type to another.
     */
    dict_sqlite->dbpath = cfg_get_str(dict_sqlite->parser, "dbpath", "", 1, 0);
    dict_sqlite->query = cfg_get_str(dict_sqlite->parser, "query", NULL, 0, 0);
    if (dict_sqlite->query == 0) {
	buf = vstring_alloc(100);
	db_common_sql_build_query(buf, dict_sqlite->parser);
	dict_sqlite->query = vstring_export(buf);
    }
    dict_sqlite->result_format =
	cfg_get_str(dict_sqlite->parser, "result_format", "%s", 1, 0);
    dict_sqlite->expansion_limit =
	cfg_get_int(dict_sqlite->parser, "expansion_limit", 0, 0, 0);

    /*
     * Parse the query / result templates and the optional domain filter.
     */
    dict_sqlite->ctx = 0;
    (void) db_common_parse(&dict_sqlite->dict, &dict_sqlite->ctx,
			   dict_sqlite->query, 1);
    (void) db_common_parse(0, &dict_sqlite->ctx, dict_sqlite->result_format, 0);
    db_common_parse_domain(dict_sqlite->parser, dict_sqlite->ctx);

    /*
     * Maps that use substring keys should only be used with the full input
     * key.
     */
    if (db_common_dict_partial(dict_sqlite->ctx))
	dict_sqlite->dict.flags |= DICT_FLAG_PATTERN;
    else
	dict_sqlite->dict.flags |= DICT_FLAG_FIXED;
}

/* dict_sqlite_open - open sqlite database */

DICT   *dict_sqlite_open(const char *name, int open_flags, int dict_flags)
{
    DICT_SQLITE *dict_sqlite;
    CFG_PARSER *parser;

    /*
     * Sanity checks.
     */
    if (open_flags != O_RDONLY)
	return (dict_surrogate(DICT_TYPE_SQLITE, name, open_flags, dict_flags,
			       "%s:%s map requires O_RDONLY access mode",
			       DICT_TYPE_SQLITE, name));

    /*
     * Open the configuration file.
     */
    if ((parser = cfg_parser_alloc(name)) == 0)
	return (dict_surrogate(DICT_TYPE_SQLITE, name, open_flags, dict_flags,
			       "open %s: %m", name));

    dict_sqlite = (DICT_SQLITE *) dict_alloc(DICT_TYPE_SQLITE, name,
					     sizeof(DICT_SQLITE));
    dict_sqlite->dict.lookup = dict_sqlite_lookup;
    dict_sqlite->dict.close = dict_sqlite_close;
    dict_sqlite->dict.flags = dict_flags;

    dict_sqlite->parser = parser;
    sqlite_parse_config(dict_sqlite, name);

    if (sqlite3_open(dict_sqlite->dbpath, &dict_sqlite->db))
	msg_fatal("%s:%s: Can't open database: %s\n",
		  DICT_TYPE_SQLITE, name, sqlite3_errmsg(dict_sqlite->db));

    dict_sqlite->dict.owner = cfg_get_owner(dict_sqlite->parser);

    return (DICT_DEBUG (&dict_sqlite->dict));
}

#endif
