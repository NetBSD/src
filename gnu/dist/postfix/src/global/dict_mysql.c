/*++
/* NAME
/*	dict_mysql 3
/* SUMMARY
/*	dictionary manager interface to MySQL databases
/* SYNOPSIS
/*	#include <dict_mysql.h>
/*
/*	DICT	*dict_mysql_open(name, open_flags, dict_flags)
/*	const char *name;
/*	int	open_flags;
/*	int	dict_flags;
/* DESCRIPTION
/*	dict_mysql_open() creates a dictionary of type 'mysql'.  This
/*	dictionary is an interface for the postfix key->value mappings
/*	to mysql.  The result is a pointer to the installed dictionary,
/*	or a null pointer in case of problems.
/*
/*	The mysql dictionary can manage multiple connections to different
/*	sql servers on different hosts.  It assumes that the underlying data
/*	on each host is identical (mirrored) and maintains one connection
/*	at any given time.  If any connection fails,  any other available
/*	ones will be opened and used.  The intent of this feature is to eliminate
/*	a single point of failure for mail systems that would otherwise rely
/*	on a single mysql server.
/* .PP
/*	Arguments:
/* .IP name
/*	Either the path to the MySQL configuration file (if it starts
/*	with '/' or '.'), or the prefix which will be used to obtain
/*	main.cf configuration parameters for this search.
/*
/*	In the first case, the configuration parameters below are
/*	specified in the file as \fIname\fR=\fBvalue\fR pairs.
/*
/*	In the second case, the configuration parameters are
/*	prefixed with the value of \fIname\fR and an underscore,
/*	and they are specified in main.cf.  For example, if this
/*	value is \fImysqlsource\fR, the parameters would look like
/*	\fImysqlsource_user\fR, \fImysqlsource_table\fR, and so on.
/*
/* .IP other_name
/*	reference for outside use.
/* .IP open_flags
/*	Must be O_RDONLY.
/* .IP dict_flags
/*	See dict_open(3).
/* .PP
/*	Configuration parameters:
/*
/*	The parameters encodes a number of pieces of information:
/*	username, password, databasename, table, select_field,
/*	where_field, and hosts:
/* .IP \fIuser\fR
/* 	Username for connecting to the database.
/* .IP \fIpassword\fR
/*	Password for the above.
/* .IP \fIdbname\fR
/*	Name of the database.
/* .IP \fIdomain\fR
/*      List of domains the queries should be restricted to.  If
/*      specified, only FQDN addresses whose domain parts matching this
/*      list will be queried against the SQL database.  Lookups for
/*      partial addresses are also supressed.  This can significantly
/*      reduce the query load on the server.
/* .IP \fIquery\fR
/*      Query template, before the query is actually issued, variable
/*	substitutions are performed. See mysql_table(5) for details. If
/*	No query is specified, the legacy variables \fItable\fR,
/*	\fIselect_field\fR, \fIwhere_field\fR and \fIadditional_conditions\fR
/*	are used to construct the query template.
/* .IP \fIresult_format\fR
/*      The format used to expand results from queries.  Substitutions
/*      are performed as described in mysql_table(5). Defaults to returning
/*	the lookup result unchanged.
/* .IP expansion_limit
/*	Limit (if any) on the total number of lookup result values. Lookups which
/*	exceed the limit fail with dict_errno=DICT_ERR_RETRY. Note that each
/*	non-empty (and non-NULL) column of a multi-column result row counts as
/*	one result.
/* .IP \fItable\fR
/*	When \fIquery\fR is not set, name of the table used to construct the
/*	query string. This provides compatibility with older releases.
/* .IP \fIselect_field\fR
/*	When \fIquery\fR is not set, name of the result field used to
/*	construct the query string. This provides compatibility with older
/*	releases.
/* .IP \fIwhere_field\fR
/*	When \fIquery\fR is not set, name of the where clause field used to
/*	construct the query string. This provides compatibility with older
/*	releases.
/* .IP \fIadditional_conditions\fR
/*	When \fIquery\fR is not set, additional where clause conditions used
/*	to construct the query string. This provides compatibility with older
/*	releases.
/* .IP \fIhosts\fR
/*	List of hosts to connect to.
/* .PP
/*	For example, if you want the map to reference databases of
/*	the name "your_db" and execute a query like this: select
/*	forw_addr from aliases where alias like '<some username>'
/*	against any database called "vmailer_info" located on hosts
/*	host1.some.domain and host2.some.domain, logging in as user
/*	"vmailer" and password "passwd" then the configuration file
/*	should read:
/* .PP
/*	\fIuser\fR = \fBvmailer\fR
/* .br
/*	\fIpassword\fR = \fBpasswd\fR
/* .br
/*	\fIdbname\fR = \fBvmailer_info\fR
/* .br
/*	\fItable\fR = \fBaliases\fR
/* .br
/*	\fIselect_field\fR = \fBforw_addr\fR
/* .br
/*	\fIwhere_field\fR = \fBalias\fR
/* .br
/*	\fIhosts\fR = \fBhost1.some.domain\fR \fBhost2.some.domain\fR
/* .IP \fIadditional_conditions\fR
/*      Backward compatibility when \fIquery\fR is not set, additional
/*	conditions to the WHERE clause.
/* .IP \fIhosts\fR
/*	List of hosts to connect to.
/* .PP
/*	For example, if you want the map to reference databases of
/*	the name "your_db" and execute a query like this: select
/*	forw_addr from aliases where alias like '<some username>'
/*	against any database called "vmailer_info" located on hosts
/*	host1.some.domain and host2.some.domain, logging in as user
/*	"vmailer" and password "passwd" then the configuration file
/*	should read:
/* .PP
/*	\fIuser\fR = \fBvmailer\fR
/* .br
/*	\fIpassword\fR = \fBpasswd\fR
/* .br
/*	\fIdbname\fR = \fBvmailer_info\fR
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

#ifdef HAS_MYSQL
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <time.h>
#include <mysql.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

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

#include "dict_mysql.h"

/* need some structs to help organize things */
typedef struct {
    MYSQL  *db;
    char   *hostname;
    char   *name;
    unsigned port;
    unsigned type;			/* TYPEUNIX | TYPEINET */
    unsigned stat;			/* STATUNTRIED | STATFAIL | STATCUR */
    time_t  ts;				/* used for attempting reconnection
					 * every so often if a host is down */
} HOST;

typedef struct {
    int     len_hosts;			/* number of hosts */
    HOST  **db_hosts;			/* the hosts on which the databases
					 * reside */
} PLMYSQL;

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
    ARGV   *hosts;
    PLMYSQL *pldb;
#if defined(MYSQL_VERSION_ID) && MYSQL_VERSION_ID >= 40000
    HOST   *active_host;
#endif
} DICT_MYSQL;

#define STATACTIVE			(1<<0)
#define STATFAIL			(1<<1)
#define STATUNTRIED			(1<<2)

#define TYPEUNIX			(1<<0)
#define TYPEINET			(1<<1)

#define RETRY_CONN_MAX			100
#define RETRY_CONN_INTV			60		/* 1 minute */
#define IDLE_CONN_INTV			60		/* 1 minute */

/* internal function declarations */
static PLMYSQL *plmysql_init(ARGV *);
static MYSQL_RES *plmysql_query(DICT_MYSQL *, const char *, VSTRING *, char *,
				char *, char *);
static void plmysql_dealloc(PLMYSQL *);
static void plmysql_close_host(HOST *);
static void plmysql_down_host(HOST *);
static void plmysql_connect_single(HOST *, char *, char *, char *);
static const char *dict_mysql_lookup(DICT *, const char *);
DICT   *dict_mysql_open(const char *, int, int);
static void dict_mysql_close(DICT *);
static void mysql_parse_config(DICT_MYSQL *, const char *);
static HOST *host_init(const char *);

/* dict_mysql_quote - escape SQL metacharacters in input string */

static void dict_mysql_quote(DICT *dict, const char *name, VSTRING *result)
{
    DICT_MYSQL *dict_mysql = (DICT_MYSQL *) dict;
    int len = strlen(name);
    int buflen = 2*len + 1;

    /*
     * We won't get integer overflows in 2*len + 1, because Postfix
     * input keys have reasonable size limits, better safe than sorry.
     */
    if (buflen < len)
	msg_panic("dict_mysql_quote: integer overflow in 2*%d+1", len);
    VSTRING_SPACE(result, buflen);

#if defined(MYSQL_VERSION_ID) && MYSQL_VERSION_ID >= 40000
    if (dict_mysql->active_host)
	mysql_real_escape_string(dict_mysql->active_host->db,
				 vstring_end(result), name, len);
    else
#endif
	mysql_escape_string(vstring_end(result), name, len);

    VSTRING_SKIP(result);
}

/* dict_mysql_lookup - find database entry */

static const char *dict_mysql_lookup(DICT *dict, const char *name)
{
    const char *myname = "dict_mysql_lookup";
    DICT_MYSQL *dict_mysql = (DICT_MYSQL *)dict;
    PLMYSQL *pldb = dict_mysql->pldb;
    MYSQL_RES *query_res;
    MYSQL_ROW row;
    static VSTRING *result;
    static VSTRING *query;
    int     i;
    int     j;
    int     numrows;
    int     expansion;
    const char *r;
    db_quote_callback_t quote_func = dict_mysql_quote;

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
    if (db_common_check_domain(dict_mysql->ctx, name) == 0) {
        if (msg_verbose)
	    msg_info("%s: Skipping lookup of '%s'", myname, name);
        return (0);
    }

#define INIT_VSTR(buf, len) do { \
	if (buf == 0) \
	    buf = vstring_alloc(len); \
	VSTRING_RESET(buf); \
	VSTRING_TERMINATE(buf); \
    } while (0)

    INIT_VSTR(query, 10);

    /*
     * Suppress the lookup if the query expansion is empty
     *
     * This initial expansion is outside the context of any
     * specific host connection, we just want to check the
     * key pre-requisites, so when quoting happens separately
     * for each connection, we don't bother with quoting...
     */
#if defined(MYSQL_VERSION_ID) && MYSQL_VERSION_ID >= 40000
    quote_func = 0;
#endif
    if (!db_common_expand(dict_mysql->ctx, dict_mysql->query,
    			  name, 0, query, quote_func))
        return (0);
    
    /* do the query - set dict_errno & cleanup if there's an error */
    if ((query_res = plmysql_query(dict_mysql, name, query,
				   dict_mysql->dbname,
				   dict_mysql->username,
				   dict_mysql->password)) == 0) {
	dict_errno = DICT_ERR_RETRY;
	return (0);
    }

    numrows = mysql_num_rows(query_res);
    if (msg_verbose)
	msg_info("%s: retrieved %d rows", myname, numrows);
    if (numrows == 0) {
	mysql_free_result(query_res);
	return 0;
    }

    INIT_VSTR(result, 10);

    for (expansion = i = 0; i < numrows && dict_errno == 0; i++) {
	row = mysql_fetch_row(query_res);
	for (j = 0; j < mysql_num_fields(query_res); j++) {
	    if (db_common_expand(dict_mysql->ctx, dict_mysql->result_format,
	    			 row[j], name, result, 0)
		&& dict_mysql->expansion_limit > 0
		&& ++expansion > dict_mysql->expansion_limit) {
		msg_warn("%s: %s: Expansion limit exceeded for key: '%s'",
			 myname, dict_mysql->parser->name, name);
		dict_errno = DICT_ERR_RETRY;
		break;
	    }
	}
    }
    mysql_free_result(query_res);
    r = vstring_str(result);
    return ((dict_errno == 0 && *r) ? r : 0);
}

/* dict_mysql_check_stat - check the status of a host */

static int dict_mysql_check_stat(HOST *host, unsigned stat, unsigned type,
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

/* dict_mysql_find_host - find a host with the given status */

static HOST *dict_mysql_find_host(PLMYSQL *PLDB, unsigned stat, unsigned type)
{
    time_t  t;
    int     count = 0;
    int     idx;
    int     i;

    t = time((time_t *) 0);
    for (i = 0; i < PLDB->len_hosts; i++) {
	if (dict_mysql_check_stat(PLDB->db_hosts[i], stat, type, t))
	    count++;
    }

    if (count) {
	idx = (count > 1) ?
	    1 + count * (double) myrand() / (1.0 + RAND_MAX) : 1;

	for (i = 0; i < PLDB->len_hosts; i++) {
	    if (dict_mysql_check_stat(PLDB->db_hosts[i], stat, type, t) &&
		--idx == 0)
		return PLDB->db_hosts[i];
	}
    }
    return 0;
}

/* dict_mysql_get_active - get an active connection */

static HOST *dict_mysql_get_active(PLMYSQL *PLDB, char *dbname,
				   char *username, char *password)
{
    const char *myname = "dict_mysql_get_active";
    HOST   *host;
    int     count = RETRY_CONN_MAX;

    /* Try the active connections first; prefer the ones to UNIX sockets. */
    if ((host = dict_mysql_find_host(PLDB, STATACTIVE, TYPEUNIX)) != NULL ||
	(host = dict_mysql_find_host(PLDB, STATACTIVE, TYPEINET)) != NULL) {
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
	   ((host = dict_mysql_find_host(PLDB, STATUNTRIED | STATFAIL,
					 TYPEUNIX)) != NULL ||
	   (host = dict_mysql_find_host(PLDB, STATUNTRIED | STATFAIL,
					TYPEINET)) != NULL)) {
	if (msg_verbose)
	    msg_info("%s: attempting to connect to host %s", myname,
		     host->hostname);
	plmysql_connect_single(host, dbname, username, password);
	if (host->stat == STATACTIVE)
	    return host;
    }

    /* bad news... */
    return 0;
}

/* dict_mysql_event - callback: close idle connections */

static void dict_mysql_event(int unused_event, char *context)
{
    HOST   *host = (HOST *) context;

    if (host->db)
	plmysql_close_host(host);
}

/*
 * plmysql_query - process a MySQL query.  Return MYSQL_RES* on success.
 *			On failure, log failure and try other db instances.
 *			on failure of all db instances, return 0;
 *			close unnecessary active connections
 */

static MYSQL_RES *plmysql_query(DICT_MYSQL *dict_mysql,
				        const char *name,
					VSTRING *query,
				        char *dbname,
				        char *username,
				        char *password)
{
    PLMYSQL *PLDB = dict_mysql->pldb;
    HOST   *host;
    MYSQL_RES *res = 0;

    while ((host = dict_mysql_get_active(PLDB, dbname, username, password)) != NULL) {

#if defined(MYSQL_VERSION_ID) && MYSQL_VERSION_ID >= 40000
	/*
	 * The active host is used to escape strings in the
	 * context of the active connection's character encoding.
	 */
	dict_mysql->active_host = host;
	VSTRING_RESET(query);
	VSTRING_TERMINATE(query);
	db_common_expand(dict_mysql->ctx, dict_mysql->query,
			 name, 0, query, dict_mysql_quote);
	dict_mysql->active_host = 0;
#endif

	if (!(mysql_query(host->db, vstring_str(query)))) {
	    if ((res = mysql_store_result(host->db)) == 0) {
		msg_warn("mysql query failed: %s", mysql_error(host->db));
		plmysql_down_host(host);
	    } else {
		if (msg_verbose)
		    msg_info("dict_mysql: successful query from host %s", host->hostname);
		event_request_timer(dict_mysql_event, (char *) host, IDLE_CONN_INTV);
		break;
	    }
	} else {
	    msg_warn("mysql query failed: %s", mysql_error(host->db));
	    plmysql_down_host(host);
	}
    }

    return res;
}

/*
 * plmysql_connect_single -
 * used to reconnect to a single database when one is down or none is
 * connected yet. Log all errors and set the stat field of host accordingly
 */
static void plmysql_connect_single(HOST *host, char *dbname, char *username, char *password)
{
    if ((host->db = mysql_init(NULL)) == NULL)
	msg_fatal("dict_mysql: insufficient memory");
    if (mysql_real_connect(host->db,
			   (host->type == TYPEINET ? host->name : 0),
			   username,
			   password,
			   dbname,
			   host->port,
			   (host->type == TYPEUNIX ? host->name : 0),
			   0)) {
	if (msg_verbose)
	    msg_info("dict_mysql: successful connection to host %s",
		     host->hostname);
	host->stat = STATACTIVE;
    } else {
	msg_warn("connect to mysql server %s: %s",
		 host->hostname, mysql_error(host->db));
	plmysql_down_host(host);
    }
}

/* plmysql_close_host - close an established MySQL connection */
static void plmysql_close_host(HOST *host)
{
    mysql_close(host->db);
    host->db = 0;
    host->stat = STATUNTRIED;
}

/*
 * plmysql_down_host - close a failed connection AND set a "stay away from
 * this host" timer
 */
static void plmysql_down_host(HOST *host)
{
    mysql_close(host->db);
    host->db = 0;
    host->ts = time((time_t *) 0) + RETRY_CONN_INTV;
    host->stat = STATFAIL;
    event_cancel_timer(dict_mysql_event, (char *) host);
}

/* mysql_parse_config - parse mysql configuration file */

static void mysql_parse_config(DICT_MYSQL *dict_mysql, const char *mysqlcf)
{
    const char *myname = "mysqlname_parse";
    CFG_PARSER *p;
    VSTRING *buf;
    int     i;
    char   *hosts;
    
    p = dict_mysql->parser = cfg_parser_alloc(mysqlcf);
    dict_mysql->username = cfg_get_str(p, "user", "", 0, 0);
    dict_mysql->password = cfg_get_str(p, "password", "", 0, 0);
    dict_mysql->dbname = cfg_get_str(p, "dbname", "", 1, 0);
    dict_mysql->result_format = cfg_get_str(p, "result_format", "%s", 1, 0);
    /*
     * XXX: The default should be non-zero for safety, but that is not
     * backwards compatible.
     */
    dict_mysql->expansion_limit = cfg_get_int(dict_mysql->parser,
					      "expansion_limit", 0, 0, 0);

    if ((dict_mysql->query = cfg_get_str(p, "query", NULL, 0, 0)) == 0) {
        /*
         * No query specified -- fallback to building it from components
         * (old style "select %s from %s where %s")
         */                 
	buf = vstring_alloc(64);
	db_common_sql_build_query(buf, p);
	dict_mysql->query = vstring_export(buf);
    }

    /*
     * Must parse all templates before we can use db_common_expand()
     */
    dict_mysql->ctx = 0;
    (void) db_common_parse(&dict_mysql->dict, &dict_mysql->ctx,
			   dict_mysql->query, 1);
    (void) db_common_parse(0, &dict_mysql->ctx, dict_mysql->result_format, 0);
    db_common_parse_domain(p, dict_mysql->ctx);

    /*
     * Maps that use substring keys should only be used with the full
     * input key.
     */
    if (db_common_dict_partial(dict_mysql->ctx))
	dict_mysql->dict.flags |= DICT_FLAG_PATTERN;
    else
	dict_mysql->dict.flags |= DICT_FLAG_FIXED;
    if (dict_mysql->dict.flags & DICT_FLAG_FOLD_FIX)
	dict_mysql->dict.fold_buf = vstring_alloc(10);

    hosts = cfg_get_str(p, "hosts", "", 0, 0);

    dict_mysql->hosts = argv_split(hosts, " ,\t\r\n");
    if (dict_mysql->hosts->argc == 0) {
	argv_add(dict_mysql->hosts, "localhost", ARGV_END);
	argv_terminate(dict_mysql->hosts);
	if (msg_verbose)
	    msg_info("%s: %s: no hostnames specified, defaulting to '%s'",
		     myname, mysqlcf, dict_mysql->hosts->argv[0]);
    }
    myfree(hosts);
}

/* dict_mysql_open - open MYSQL data base */

DICT   *dict_mysql_open(const char *name, int open_flags, int dict_flags)
{
    DICT_MYSQL *dict_mysql;

    /*
     * Sanity checks.
     */
    if (open_flags != O_RDONLY)
	msg_fatal("%s:%s map requires O_RDONLY access mode",
		  DICT_TYPE_MYSQL, name);

    dict_mysql = (DICT_MYSQL *) dict_alloc(DICT_TYPE_MYSQL, name,
					   sizeof(DICT_MYSQL));
    dict_mysql->dict.lookup = dict_mysql_lookup;
    dict_mysql->dict.close = dict_mysql_close;
    dict_mysql->dict.flags = dict_flags;
    mysql_parse_config(dict_mysql, name);
#if defined(MYSQL_VERSION_ID) && MYSQL_VERSION_ID >= 40000
    dict_mysql->active_host = 0;
#endif
    dict_mysql->pldb = plmysql_init(dict_mysql->hosts);
    if (dict_mysql->pldb == NULL)
	msg_fatal("couldn't intialize pldb!\n");
    return (DICT_DEBUG (&dict_mysql->dict));
}

/*
 * plmysql_init - initalize a MYSQL database.
 *		    Return NULL on failure, or a PLMYSQL * on success.
 */
static PLMYSQL *plmysql_init(ARGV *hosts)
{
    PLMYSQL *PLDB;
    int     i;

    if ((PLDB = (PLMYSQL *) mymalloc(sizeof(PLMYSQL))) == 0)
	msg_fatal("mymalloc of pldb failed");

    PLDB->len_hosts = hosts->argc;
    if ((PLDB->db_hosts = (HOST **) mymalloc(sizeof(HOST *) * hosts->argc)) == 0)
	return (0);
    for (i = 0; i < hosts->argc; i++)
	PLDB->db_hosts[i] = host_init(hosts->argv[i]);

    return PLDB;
}


/* host_init - initialize HOST structure */
static HOST *host_init(const char *hostname)
{
    const char *myname = "mysql host_init";
    HOST   *host = (HOST *) mymalloc(sizeof(HOST));
    const char *d = hostname;
    char   *s;

    host->db = 0;
    host->hostname = mystrdup(hostname);
    host->port = 0;
    host->stat = STATUNTRIED;
    host->ts = 0;

    /*
     * Ad-hoc parsing code. Expect "unix:pathname" or "inet:host:port", where
     * both "inet:" and ":port" are optional.
     */
    if (strncmp(d, "unix:", 5) == 0) {
	d += 5;
	host->type = TYPEUNIX;
    } else {
	if (strncmp(d, "inet:", 5) == 0)
	    d += 5;
	host->type = TYPEINET;
    }
    host->name = mystrdup(d);
    if ((s = split_at_right(host->name, ':')) != 0)
	host->port = ntohs(find_inet_port(s, "tcp"));
    if (strcasecmp(host->name, "localhost") == 0) {
	/* The MySQL way: this will actually connect over the UNIX socket */
	myfree(host->name);
	host->name = 0;
	host->type = TYPEUNIX;
    }

    if (msg_verbose > 1)
	msg_info("%s: host=%s, port=%d, type=%s", myname,
		 host->name ? host->name : "localhost",
		 host->port, host->type == TYPEUNIX ? "unix" : "inet");
    return host;
}

/* dict_mysql_close - close MYSQL database */

static void dict_mysql_close(DICT *dict)
{
    int     i;
    DICT_MYSQL *dict_mysql = (DICT_MYSQL *) dict;

    plmysql_dealloc(dict_mysql->pldb);
    cfg_parser_free(dict_mysql->parser);
    myfree(dict_mysql->username);
    myfree(dict_mysql->password);
    myfree(dict_mysql->dbname);
    myfree(dict_mysql->query);
    myfree(dict_mysql->result_format);
    if (dict_mysql->hosts)
    	argv_free(dict_mysql->hosts);
    if (dict_mysql->ctx)
	db_common_free_ctx(dict_mysql->ctx);
    if (dict->fold_buf)
	vstring_free(dict->fold_buf);
    dict_free(dict);
}

/* plmysql_dealloc - free memory associated with PLMYSQL close databases */
static void plmysql_dealloc(PLMYSQL *PLDB)
{
    int     i;

    for (i = 0; i < PLDB->len_hosts; i++) {
	event_cancel_timer(dict_mysql_event, (char *) (PLDB->db_hosts[i]));
	if (PLDB->db_hosts[i]->db)
	    mysql_close(PLDB->db_hosts[i]->db);
	myfree(PLDB->db_hosts[i]->hostname);
	if (PLDB->db_hosts[i]->name)
	    myfree(PLDB->db_hosts[i]->name);
	myfree((char *) PLDB->db_hosts[i]);
    }
    myfree((char *) PLDB->db_hosts);
    myfree((char *) (PLDB));
}

#endif
