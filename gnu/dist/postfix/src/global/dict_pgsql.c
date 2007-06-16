/*	$NetBSD: dict_pgsql.c,v 1.1.1.5.4.1 2007/06/16 16:59:55 snj Exp $	*/

/*++
/* NAME
/*	dict_pgsql 3
/* SUMMARY
/*	dictionary manager interface to PostgreSQL databases
/* SYNOPSIS
/*	#include <dict_pgsql.h>
/*
/*	DICT	*dict_pgsql_open(name, open_flags, dict_flags)
/*	const char *name;
/*	int	open_flags;
/*	int	dict_flags;
/* DESCRIPTION
/*	dict_pgsql_open() creates a dictionary of type 'pgsql'.  This
/*	dictionary is an interface for the postfix key->value mappings
/*	to pgsql.  The result is a pointer to the installed dictionary,
/*	or a null pointer in case of problems.
/*
/*	The pgsql dictionary can manage multiple connections to
/*	different sql servers for the same database.  It assumes that
/*	the underlying data on each server is identical (mirrored) and
/*	maintains one connection at any given time.  If any connection
/*	fails,  any other available ones will be opened and used.
/*	The intent of this feature is to eliminate a single point of
/*	failure for mail systems that would otherwise rely on a single
/*	pgsql server.
/* .PP
/*	Arguments:
/* .IP name
/*	Either the path to the PostgreSQL configuration file (if it
/*	starts with '/' or '.'), or the prefix which will be used to
/*	obtain main.cf configuration parameters for this search.
/*
/*	In the first case, the configuration parameters below are
/*	specified in the file as \fIname\fR=\fBvalue\fR pairs.
/*
/*	In the second case, the configuration parameters are
/*	prefixed with the value of \fIname\fR and an underscore,
/*	and they are specified in main.cf.  For example, if this
/*	value is \fIpgsqlsource\fR, the parameters would look like
/*	\fIpgsqlsource_user\fR, \fIpgsqlsource_table\fR, and so on.
/* .IP other_name
/*	reference for outside use.
/* .IP open_flags
/*	Must be O_RDONLY.
/* .IP dict_flags
/*	See dict_open(3).
/*
/* .PP
/*	Configuration parameters:
/*
/*	The parameters encode a number of pieces of information:
/*	username, password, databasename, table, select_field,
/*	where_field, and hosts:
/* .IP \fIuser\fR
/*	Username for connecting to the database.
/* .IP \fIpassword\fR
/*	Password for the above.
/* .IP \fIdbname\fR
/*	Name of the database.
/* .IP \fIquery\fR
/*	Query template. If not defined a default query template is constructed
/*	from the legacy \fIselect_function\fR or failing that the \fItable\fR,
/*	\fIselect_field\fR, \fIwhere_field\fR, and \fIadditional_conditions\fR
/*	parameters. Before the query is issues, variable substitutions are
/*	performed. See pgsql_table(5).
/* .IP \fIdomain\fR
/*	List of domains the queries should be restricted to.  If
/*	specified, only FQDN addresses whose domain parts matching this
/*	list will be queried against the SQL database.  Lookups for
/*	partial addresses are also supressed.  This can significantly
/*	reduce the query load on the server.
/* .IP \fIresult_format\fR
/*	The format used to expand results from queries.  Substitutions
/*	are performed as described in pgsql_table(5). Defaults to returning
/*	the lookup result unchanged.
/* .IP expansion_limit
/*	Limit (if any) on the total number of lookup result values. Lookups which
/*	exceed the limit fail with dict_errno=DICT_ERR_RETRY. Note that each
/*	non-empty (and non-NULL) column of a multi-column result row counts as
/*	one result.
/* .IP \fIselect_function\fR
/*	When \fIquery\fR is not defined, the function to be used instead of
/*	the default query based on the legacy \fItable\fR, \fIselect_field\fR,
/*	\fIwhere_field\fR, and \fIadditional_conditions\fR parameters.
/* .IP \fItable\fR
/*	When \fIquery\fR and \fIselect_function\fR are not defined, the name of the
/*	FROM table used to construct the default query template, see pgsql_table(5).
/* .IP \fIselect_field\fR
/*	When \fIquery\fR and \fIselect_function\fR are not defined, the name of the
/*	SELECT field used to construct the default query template, see pgsql_table(5).
/* .IP \fIwhere_field\fR
/*	When \fIquery\fR and \fIselect_function\fR are not defined, the name of the
/*	WHERE field used to construct the default query template, see pgsql_table(5).
/* .IP \fIadditional_conditions\fR
/*	When \fIquery\fR and \fIselect_function\fR are not defined, the name of the
/*	additional text to add to the WHERE field in the default query template (this
/*	usually begins with "and") see pgsql_table(5).
/* .IP \fIhosts\fR
/*	List of hosts to connect to.
/* .PP
/*	For example, if you want the map to reference databases of
/*	the name "your_db" and execute a query like this: select
/*	forw_addr from aliases where alias like '<some username>'
/*	against any database called "postfix_info" located on hosts
/*	host1.some.domain and host2.some.domain, logging in as user
/*	"postfix" and password "passwd" then the configuration file
/*	should read:
/* .PP
/*	\fIuser\fR = \fBpostfix\fR
/* .br
/*	\fIpassword\fR = \fBpasswd\fR
/* .br
/*	\fIdbname\fR = \fBpostfix_info\fR
/* .br
/*	\fItable\fR = \fBaliases\fR
/* .br
/*	\fIselect_field\fR = \fBforw_addr\fR
/* .br
/*	\fIwhere_field\fR = \fBalias\fR
/* .br
/*	\fIhosts\fR = \fBhost1.some.domain\fR \fBhost2.some.domain\fR
/* .PP
/* SEE ALSO
/*	dict(3) generic dictionary manager
/* AUTHOR(S)
/*	Aaron Sethman
/*	androsyn@ratbox.org
/*
/*	Based upon dict_mysql.c by
/*
/*	Scott Cotton
/*	IC Group, Inc.
/*	scott@icgroup.com
/*
/*	Joshua Marcus
/*	IC Group, Inc.
/*	josh@icgroup.com
/*--*/

/* System library. */

#include "sys_defs.h"

#ifdef HAS_PGSQL
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <time.h>

#include <postgres_ext.h>
#include <libpq-fe.h>

/* Utility library. */

#include "dict.h"
#include "msg.h"
#include "mymalloc.h"
#include "argv.h"
#include "vstring.h"
#include "split_at.h"
#include "find_inet.h"
#include "myrand.h"
#include "events.h"
#include "stringops.h"

/* Global library. */

#include "cfg_parser.h"
#include "db_common.h"

/* Application-specific. */

#include "dict_pgsql.h"

#define STATACTIVE			(1<<0)
#define STATFAIL			(1<<1)
#define STATUNTRIED			(1<<2)

#define TYPEUNIX			(1<<0)
#define TYPEINET			(1<<1)

#define RETRY_CONN_MAX			100
#define RETRY_CONN_INTV			60		/* 1 minute */
#define IDLE_CONN_INTV			60		/* 1 minute */

typedef struct {
    PGconn *db;
    char   *hostname;
    char   *name;
    char   *port;
    unsigned type;			/* TYPEUNIX | TYPEINET */
    unsigned stat;			/* STATUNTRIED | STATFAIL | STATCUR */
    time_t  ts;				/* used for attempting reconnection */
} HOST;

typedef struct {
    int     len_hosts;			/* number of hosts */
    HOST  **db_hosts;			/* hosts on which databases reside */
} PLPGSQL;

typedef struct {
    DICT    dict;
    CFG_PARSER *parser;
    char   *query;
    char   *result_format;
    void   *ctx;
    int     expansion_limit;
    char   *username;
    char   *password;
    char   *dbname;
    char   *table;
    ARGV   *hosts;
    PLPGSQL *pldb;
    HOST   *active_host;
} DICT_PGSQL;


/* Just makes things a little easier for me.. */
#define PGSQL_RES PGresult

/* internal function declarations */
static PLPGSQL *plpgsql_init(ARGV *);
static PGSQL_RES *plpgsql_query(DICT_PGSQL *, const char *, VSTRING *, char *,
				char *, char *);
static void plpgsql_dealloc(PLPGSQL *);
static void plpgsql_close_host(HOST *);
static void plpgsql_down_host(HOST *);
static void plpgsql_connect_single(HOST *, char *, char *, char *);
static const char *dict_pgsql_lookup(DICT *, const char *);
DICT   *dict_pgsql_open(const char *, int, int);
static void dict_pgsql_close(DICT *);
static HOST *host_init(const char *);

/* dict_pgsql_quote - escape SQL metacharacters in input string */

static void dict_pgsql_quote(DICT *dict, const char *name, VSTRING *result)
{
    DICT_PGSQL *dict_pgsql = (DICT_PGSQL *) dict;
    HOST  *active_host = dict_pgsql->active_host;
    char  *myname = "dict_pgsql_quote";
    size_t len = strlen(name);
    size_t buflen = 2*len + 1;
    int err = 1;

    if (active_host == 0)
	msg_panic("%s: bogus dict_pgsql->active_host", myname);

    /*
     * We won't get arithmetic overflows in 2*len + 1, because Postfix
     * input keys have reasonable size limits, better safe than sorry.
     */
    if (buflen <= len)
	msg_panic("%s: arithmetic overflow in 2*%lu+1", 
		  myname, (unsigned long) len);

    /*
     * XXX Workaround: stop further processing when PQescapeStringConn()
     * (below) fails. A more proper fix requires invasive changes, not
     * suitable for a stable release.
     */
    if (active_host->stat == STATFAIL)
	return;

    /*
     * Escape the input string, using PQescapeStringConn(), because
     * the older PQescapeString() is not safe anymore, as stated by the
     * documentation.
     *
     * From current libpq (8.1.4) documentation:
     *
     * PQescapeStringConn writes an escaped version of the from string 
     * to the to buffer, escaping special characters so that they cannot
     * cause any harm, and adding a terminating zero byte.
     *
     * ...
     *
     * The parameter from points to the first character of the string 
     * that is to be escaped, and the length parameter gives the number 
     * of bytes in this string. A terminating zero byte is not required,
     * and should not be counted in length.
     *
     * ...
     *
     * (The parameter) to shall point to a buffer that is able to hold 
     * at least one more byte than twice the value of length, otherwise
     * the behavior is undefined.
     *
     * ...
     *
     * If the error parameter is not NULL, then *error is set to zero on
     * success, nonzero on error ... The output string is still generated
     * on error, but it can be expected that the server will reject it as
     * malformed. On error, a suitable message is stored in the conn 
     * object, whether or not error is NULL.
     */
    VSTRING_SPACE(result, buflen);
    PQescapeStringConn(active_host->db, vstring_end(result), name, len, &err);
    if (err == 0) {
	VSTRING_SKIP(result);
    } else {
	/* 
	 * PQescapeStringConn() failed. According to the docs, we still 
	 * have a valid, null-terminated output string, but we need not
	 * rely on this behavior.
	 */
	msg_warn("dict pgsql: (host %s) cannot escape input string: %s", 
		 active_host->hostname, PQerrorMessage(active_host->db));
	active_host->stat = STATFAIL;
	VSTRING_TERMINATE(result);
    }
}

/* dict_pgsql_lookup - find database entry */

static const char *dict_pgsql_lookup(DICT *dict, const char *name)
{
    const char *myname = "dict_pgsql_lookup";
    PGSQL_RES *query_res;
    DICT_PGSQL *dict_pgsql;
    PLPGSQL *pldb;
    static VSTRING *query;
    static VSTRING *result;
    int     i;
    int     j;
    int     numrows;
    int     numcols;
    int     expansion;
    const char *r;
   
    dict_pgsql = (DICT_PGSQL *) dict;
    pldb = dict_pgsql->pldb;

#define INIT_VSTR(buf, len) do { \
	if (buf == 0) \
	    buf = vstring_alloc(len); \
	VSTRING_RESET(buf); \
	VSTRING_TERMINATE(buf); \
    } while (0)

    INIT_VSTR(query, 10);
    INIT_VSTR(result, 10);

    dict_errno = 0;

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
     * If there is a domain list for this map, then only search for
     * addresses in domains on the list. This can significantly reduce
     * the load on the server.
     */
    if (db_common_check_domain(dict_pgsql->ctx, name) == 0) {
        if (msg_verbose)
	    msg_info("%s: Skipping lookup of '%s'", myname, name);
        return (0);
    }

    /*
     * Suppress the actual lookup if the expansion is empty.
     * 
     * This initial expansion is outside the context of any
     * specific host connection, we just want to check the
     * key pre-requisites, so when quoting happens separately
     * for each connection, we don't bother with quoting...
     */
    if (!db_common_expand(dict_pgsql->ctx, dict_pgsql->query,
			  name, 0, query, 0))
	return (0);
    
    /* do the query - set dict_errno & cleanup if there's an error */
    if ((query_res = plpgsql_query(dict_pgsql, name, query,
				   dict_pgsql->dbname,
				   dict_pgsql->username,
				   dict_pgsql->password)) == 0) {
	dict_errno = DICT_ERR_RETRY;
	return 0;
    }
    
    numrows = PQntuples(query_res);
    if (msg_verbose)
	msg_info("%s: retrieved %d rows", myname, numrows);
    if (numrows == 0) {
	PQclear(query_res);
	return 0;
    }
    numcols = PQnfields(query_res);

    for (expansion = i = 0; i < numrows && dict_errno == 0; i++) {
	for (j = 0; j < numcols; j++) {
	    r = PQgetvalue(query_res, i, j);
	    if (db_common_expand(dict_pgsql->ctx, dict_pgsql->result_format,
	    			 r, name, result, 0)
		&& dict_pgsql->expansion_limit > 0
		&& ++expansion > dict_pgsql->expansion_limit) {
		msg_warn("%s: %s: Expansion limit exceeded for key: '%s'",
			 myname, dict_pgsql->parser->name, name);
		dict_errno = DICT_ERR_RETRY;
		break;
	    }
	}
    }
    PQclear(query_res);
    r = vstring_str(result);
    return ((dict_errno == 0 && *r) ? r : 0);
}

/* dict_pgsql_check_stat - check the status of a host */

static int dict_pgsql_check_stat(HOST *host, unsigned stat, unsigned type,
				 time_t t)
{
    if ((host->stat & stat) && (!type || host->type & type)) {
	/* try not to hammer the dead hosts too often */
	if (host->stat == STATFAIL && host->ts > 0 && host->ts >= t)
	    return 0;
	return 1;
    }
    return 0;
}

/* dict_pgsql_find_host - find a host with the given status */

static HOST *dict_pgsql_find_host(PLPGSQL *PLDB, unsigned stat, unsigned type)
{
    time_t  t;
    int     count = 0;
    int     idx;
    int     i;

    t = time((time_t *) 0);
    for (i = 0; i < PLDB->len_hosts; i++) {
	if (dict_pgsql_check_stat(PLDB->db_hosts[i], stat, type, t))
	    count++;
    }

    if (count) {
	idx = (count > 1) ?
	    1 + count * (double) myrand() / (1.0 + RAND_MAX) : 1;

	for (i = 0; i < PLDB->len_hosts; i++) {
	    if (dict_pgsql_check_stat(PLDB->db_hosts[i], stat, type, t) &&
		--idx == 0)
		return PLDB->db_hosts[i];
	}
    }
    return 0;
}

/* dict_pgsql_get_active - get an active connection */

static HOST *dict_pgsql_get_active(PLPGSQL *PLDB, char *dbname,
				   char *username, char *password)
{
    const char *myname = "dict_pgsql_get_active";
    HOST   *host;
    int     count = RETRY_CONN_MAX;

    /* try the active connections first; prefer the ones to UNIX sockets */
    if ((host = dict_pgsql_find_host(PLDB, STATACTIVE, TYPEUNIX)) != NULL ||
	(host = dict_pgsql_find_host(PLDB, STATACTIVE, TYPEINET)) != NULL) {
	if (msg_verbose)
	    msg_info("%s: found active connection to host %s", myname,
		     host->hostname);
	return host;
    }

    /*
     * Try the remaining hosts.
     * "count" is a safety net, in case the loop takes more than
     * RETRY_CONN_INTV and the dead hosts are no longer skipped.
     */
    while (--count > 0 &&
	   ((host = dict_pgsql_find_host(PLDB, STATUNTRIED | STATFAIL,
					TYPEUNIX)) != NULL ||
	   (host = dict_pgsql_find_host(PLDB, STATUNTRIED | STATFAIL,
					TYPEINET)) != NULL)) {
	if (msg_verbose)
	    msg_info("%s: attempting to connect to host %s", myname,
		     host->hostname);
	plpgsql_connect_single(host, dbname, username, password);
	if (host->stat == STATACTIVE)
	    return host;
    }

    /* bad news... */
    return 0;
}

/* dict_pgsql_event - callback: close idle connections */

static void dict_pgsql_event(int unused_event, char *context)
{
    HOST   *host = (HOST *) context;

    if (host->db)
	plpgsql_close_host(host);
}

/*
 * plpgsql_query - process a PostgreSQL query.  Return PGSQL_RES* on success.
 *			On failure, log failure and try other db instances.
 *			on failure of all db instances, return 0;
 *			close unnecessary active connections
 */

static PGSQL_RES *plpgsql_query(DICT_PGSQL *dict_pgsql,
				        const char *name,
				        VSTRING *query,
				        char *dbname,
				        char *username,
				        char *password)
{
    PLPGSQL *PLDB = dict_pgsql->pldb;
    HOST   *host;
    PGSQL_RES *res = 0;
    ExecStatusType status;

    while ((host = dict_pgsql_get_active(PLDB, dbname, username, password)) != NULL) {
	/*
	 * The active host is used to escape strings in the
	 * context of the active connection's character encoding.
	 */
	dict_pgsql->active_host = host;
	VSTRING_RESET(query);
	VSTRING_TERMINATE(query);
	db_common_expand(dict_pgsql->ctx, dict_pgsql->query,
			 name, 0, query, dict_pgsql_quote);
	dict_pgsql->active_host = 0;

	/* Check for potential dict_pgsql_quote() failure. */
	if (host->stat == STATFAIL) {
	    plpgsql_down_host(host);
	    continue;
	}

	/* 
	 * Submit a command to the server. Be paranoid when processing
	 * the result set: try to enumerate every successful case, and
	 * reject everything else.
	 *
	 * From PostgreSQL 8.1.4 docs: (PQexec) returns a PGresult
	 * pointer or possibly a null pointer. A non-null pointer will
	 * generally be returned except in out-of-memory conditions or
	 * serious errors such as inability to send the command to the
	 * server.
	 */
	if ((res = PQexec(host->db, vstring_str(query))) != 0) {
	    /*
	     * XXX Because non-null result pointer does not imply success,
	     * we need to check the command's result status.
	     *
	     * Section 28.3.1: A result of status PGRES_NONFATAL_ERROR
	     * will never be returned directly by PQexec or other query
	     * execution functions; results of this kind are instead 
	     * passed to the notice processor.
	     *
	     * PGRES_EMPTY_QUERY is being sent by the server when the
	     * query string is empty. The sanity-checking done by
	     * the Postfix infrastructure makes this case impossible,
	     * so we need not handle this situation explicitly.
	     */
	    switch ((status = PQresultStatus(res))) {
	    case PGRES_TUPLES_OK:
	    case PGRES_COMMAND_OK:
		/* Success. */
		if (msg_verbose)
		    msg_info("dict_pgsql: successful query from host %s", 
			     host->hostname);
		event_request_timer(dict_pgsql_event, (char *) host, 
				    IDLE_CONN_INTV);
		return (res);
	    case PGRES_FATAL_ERROR:
		msg_warn("pgsql query failed: fatal error from host %s: %s",
			 host->hostname, PQresultErrorMessage(res));
		break;
	    case PGRES_BAD_RESPONSE:
		msg_warn("pgsql query failed: protocol error, host %s",
			 host->hostname);
		break;
	    default:
		msg_warn("pgsql query failed: unknown code 0x%lx from host %s",
			 (unsigned long) status, host->hostname);
		break;
	    }
	} else {
	    /* 
	     * This driver treats null pointers like fatal, non-null
	     * result pointer errors, as suggested by the PostgreSQL 
	     * 8.1.4 documentation.
	     */
	    msg_warn("pgsql query failed: fatal error from host %s: %s",
		     host->hostname, PQerrorMessage(host->db));
	}

	/* 
	 * XXX An error occurred. Clean up memory and skip this connection.
	 */
	if (res != 0)
	    PQclear(res);
	plpgsql_down_host(host);
    }

    return (0);
}

/*
 * plpgsql_connect_single -
 * used to reconnect to a single database when one is down or none is
 * connected yet. Log all errors and set the stat field of host accordingly
 */
static void plpgsql_connect_single(HOST *host, char *dbname, char *username, char *password)
{
    if ((host->db = PQsetdbLogin(host->name, host->port, NULL, NULL,
				 dbname, username, password)) == NULL
	|| PQstatus(host->db) != CONNECTION_OK) {
	msg_warn("connect to pgsql server %s: %s",
		 host->hostname, PQerrorMessage(host->db));
	plpgsql_down_host(host);
	return;
    }

    if (msg_verbose)
	msg_info("dict_pgsql: successful connection to host %s",
		 host->hostname);

    /*
     * XXX Postfix does not send multi-byte characters. The following
     * piece of code is an explicit statement of this fact, and the 
     * database server should not accept multi-byte information after
     * this point.
     */
    if (PQsetClientEncoding(host->db, "LATIN1") != 0) {
	msg_warn("dict_pgsql: cannot set the encoding to LATIN1, skipping %s",
		 host->hostname);
	plpgsql_down_host(host);
	return;
    }

    /* Success. */
    host->stat = STATACTIVE;
}

/* plpgsql_close_host - close an established PostgreSQL connection */

static void plpgsql_close_host(HOST *host)
{
    if (host->db)
	PQfinish(host->db);
    host->db = 0;
    host->stat = STATUNTRIED;
}

/*
 * plpgsql_down_host - close a failed connection AND set a "stay away from
 * this host" timer.
 */
static void plpgsql_down_host(HOST *host)
{
    if (host->db)
	PQfinish(host->db);
    host->db = 0;
    host->ts = time((time_t *) 0) + RETRY_CONN_INTV;
    host->stat = STATFAIL;
    event_cancel_timer(dict_pgsql_event, (char *) host);
}

/* pgsql_parse_config - parse pgsql configuration file */

static void pgsql_parse_config(DICT_PGSQL *dict_pgsql, const char *pgsqlcf)
{
    const char *myname = "pgsql_parse_config";
    CFG_PARSER *p;
    char   *hosts;
    VSTRING *query;
    char   *select_function;

    p = dict_pgsql->parser = cfg_parser_alloc(pgsqlcf);
    dict_pgsql->username = cfg_get_str(p, "user", "", 0, 0);
    dict_pgsql->password = cfg_get_str(p, "password", "", 0, 0);
    dict_pgsql->dbname = cfg_get_str(p, "dbname", "", 1, 0);
    dict_pgsql->result_format = cfg_get_str(p, "result_format", "%s", 1, 0);
    /*
     * XXX: The default should be non-zero for safety, but that is not
     * backwards compatible.
     */
    dict_pgsql->expansion_limit = cfg_get_int(dict_pgsql->parser,
					      "expansion_limit", 0, 0, 0);

    if ((dict_pgsql->query = cfg_get_str(p, "query", 0, 0, 0)) == 0) {
         /*
          * No query specified -- fallback to building it from components
          * ( old style "select %s from %s where %s" )
          */
	query = vstring_alloc(64);
	select_function = cfg_get_str(p, "select_function", 0, 0, 0);
	if (select_function != 0) {
	    vstring_sprintf(query, "SELECT %s('%%s')", select_function);
	    myfree(select_function);
	} else
            db_common_sql_build_query(query, p);
	dict_pgsql->query = vstring_export(query);
    }

    /*
     * Must parse all templates before we can use db_common_expand()
     */
    dict_pgsql->ctx = 0;
    (void) db_common_parse(&dict_pgsql->dict, &dict_pgsql->ctx,
			   dict_pgsql->query, 1);
    (void) db_common_parse(0, &dict_pgsql->ctx, dict_pgsql->result_format, 0);
    db_common_parse_domain(p, dict_pgsql->ctx);

    /*
     * Maps that use substring keys should only be used with the full
     * input key.
     */
    if (db_common_dict_partial(dict_pgsql->ctx))
	dict_pgsql->dict.flags |= DICT_FLAG_PATTERN;
    else
	dict_pgsql->dict.flags |= DICT_FLAG_FIXED;
    if (dict_pgsql->dict.flags & DICT_FLAG_FOLD_FIX)
	dict_pgsql->dict.fold_buf = vstring_alloc(10);

    hosts = cfg_get_str(p, "hosts", "", 0, 0);

    dict_pgsql->hosts = argv_split(hosts, " ,\t\r\n");
    if (dict_pgsql->hosts->argc == 0) {
	argv_add(dict_pgsql->hosts, "localhost", ARGV_END);
	argv_terminate(dict_pgsql->hosts);
	if (msg_verbose)
	    msg_info("%s: %s: no hostnames specified, defaulting to '%s'",
		     myname, pgsqlcf, dict_pgsql->hosts->argv[0]);
    }
    myfree(hosts);
}

/* dict_pgsql_open - open PGSQL data base */

DICT   *dict_pgsql_open(const char *name, int open_flags, int dict_flags)
{
    DICT_PGSQL *dict_pgsql;

    if (open_flags != O_RDONLY)
	msg_fatal("%s:%s map requires O_RDONLY access mode",
		  DICT_TYPE_PGSQL, name);

    dict_pgsql = (DICT_PGSQL *) dict_alloc(DICT_TYPE_PGSQL, name,
					   sizeof(DICT_PGSQL));
    dict_pgsql->dict.lookup = dict_pgsql_lookup;
    dict_pgsql->dict.close = dict_pgsql_close;
    dict_pgsql->dict.flags = dict_flags;
    pgsql_parse_config(dict_pgsql, name);
    dict_pgsql->active_host = 0;
    dict_pgsql->pldb = plpgsql_init(dict_pgsql->hosts);
    if (dict_pgsql->pldb == NULL)
	msg_fatal("couldn't intialize pldb!\n");
    return &dict_pgsql->dict;
}

/* plpgsql_init - initalize a PGSQL database */

static PLPGSQL *plpgsql_init(ARGV *hosts)
{
    PLPGSQL *PLDB;
    int     i;

    PLDB = (PLPGSQL *) mymalloc(sizeof(PLPGSQL));
    PLDB->len_hosts = hosts->argc;
    PLDB->db_hosts = (HOST **) mymalloc(sizeof(HOST *) * hosts->argc);
    for (i = 0; i < hosts->argc; i++)
	PLDB->db_hosts[i] = host_init(hosts->argv[i]);

    return PLDB;
}


/* host_init - initialize HOST structure */

static HOST *host_init(const char *hostname)
{
    const char *myname = "pgsql host_init";
    HOST   *host = (HOST *) mymalloc(sizeof(HOST));
    const char *d = hostname;

    host->db = 0;
    host->hostname = mystrdup(hostname);
    host->stat = STATUNTRIED;
    host->ts = 0;

    /*
     * Ad-hoc parsing code. Expect "unix:pathname" or "inet:host:port", where
     * both "inet:" and ":port" are optional.
     */
    if (strncmp(d, "unix:", 5) == 0 || strncmp(d, "inet:", 5) == 0)
	d += 5;
    host->name = mystrdup(d);
    host->port = split_at_right(host->name, ':');

    /* This is how PgSQL distinguishes between UNIX and INET: */
    if (host->name[0] && host->name[0] != '/')
	host->type = TYPEINET;
    else
	host->type = TYPEUNIX;

    if (msg_verbose > 1)
	msg_info("%s: host=%s, port=%s, type=%s", myname, host->name,
		 host->port ? host->port : "",
		 host->type == TYPEUNIX ? "unix" : "inet");
    return host;
}

/* dict_pgsql_close - close PGSQL data base */

static void dict_pgsql_close(DICT *dict)
{
    DICT_PGSQL *dict_pgsql = (DICT_PGSQL *) dict;

    plpgsql_dealloc(dict_pgsql->pldb);
    cfg_parser_free(dict_pgsql->parser);
    myfree(dict_pgsql->username);
    myfree(dict_pgsql->password);
    myfree(dict_pgsql->dbname);
    myfree(dict_pgsql->query);
    myfree(dict_pgsql->result_format);
    if (dict_pgsql->hosts)
    	argv_free(dict_pgsql->hosts);
    if (dict_pgsql->ctx)
	db_common_free_ctx(dict_pgsql->ctx);
    if (dict->fold_buf)
	vstring_free(dict->fold_buf);
    dict_free(dict);
}

/* plpgsql_dealloc - free memory associated with PLPGSQL close databases */

static void plpgsql_dealloc(PLPGSQL *PLDB)
{
    int     i;

    for (i = 0; i < PLDB->len_hosts; i++) {
	event_cancel_timer(dict_pgsql_event, (char *) (PLDB->db_hosts[i]));
	if (PLDB->db_hosts[i]->db)
	    PQfinish(PLDB->db_hosts[i]->db);
	myfree(PLDB->db_hosts[i]->hostname);
	myfree(PLDB->db_hosts[i]->name);
	myfree((char *) PLDB->db_hosts[i]);
    }
    myfree((char *) PLDB->db_hosts);
    myfree((char *) (PLDB));
}

#endif
