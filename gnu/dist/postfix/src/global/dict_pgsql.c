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
/* .IP \fItable\fR
/*	Name of the table.
/* .IP \fIselect_field\fR
/*	Name of the result field.
/* .IP \fIwhere_field\fR
/*	Field used in the WHERE clause.
/* .IP \fIadditional_conditions\fR
/*	Additional conditions to the WHERE clause.
/* .IP \fIquery\fR
/*	Query overriding \fItable\fR, \fIselect_field\fR,
/*	\fIwhere_field\fR, and \fIadditional_conditions\fR.  Before the
/*	query is actually issued, all occurrences of %s are replaced
/*	with the address to look up, %u are replaced with the user
/*	portion, and %d with the domain portion.
/* .IP \fIselect_function\fR
/*	Function to be used instead of the SELECT statement.  Overrides
/*	both \fIquery\fR and \fItable\fR, \fIselect_field\fR,
/*	\fIwhere_field\fR, and \fIadditional_conditions\fR settings.
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

/* Global library. */

#include "cfg_parser.h"

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
    CFG_PARSER *parser;
    char   *username;
    char   *password;
    char   *dbname;
    char   *table;
    char   *query;			/* if set, overrides fields, etc */
    char   *select_function;
    char   *select_field;
    char   *where_field;
    char   *additional_conditions;
    char  **hostnames;
    int     len_hosts;
} PGSQL_NAME;

typedef struct {
    DICT    dict;
    PLPGSQL *pldb;
    PGSQL_NAME *name;
} DICT_PGSQL;


/* Just makes things a little easier for me.. */
#define PGSQL_RES PGresult

/* internal function declarations */
static PLPGSQL *plpgsql_init(char *hostnames[], int);
static PGSQL_RES *plpgsql_query(PLPGSQL *, const char *, char *, char *, char *);
static void plpgsql_dealloc(PLPGSQL *);
static void plpgsql_close_host(HOST *);
static void plpgsql_down_host(HOST *);
static void plpgsql_connect_single(HOST *, char *, char *, char *);
static const char *dict_pgsql_lookup(DICT *, const char *);
DICT   *dict_pgsql_open(const char *, int, int);
static void dict_pgsql_close(DICT *);
static PGSQL_NAME *pgsqlname_parse(const char *);
static HOST *host_init(const char *);



/**********************************************************************
 * public interface dict_pgsql_lookup
 * find database entry return 0 if no alias found, set dict_errno
 * on errors to DICT_ERROR_RETRY and set dict_errno to 0 on success
 *********************************************************************/
static void pgsql_escape_string(char *new, const char *old, unsigned int len)
{
    unsigned int x,
            y;

    /*
     * XXX We really should be using an escaper that is provided by the PGSQL
     * library. The code below seems to be over-kill (see RUS-CERT Advisory
     * 2001-08:01), but it's better to be safe than to be sorry -- Wietse
     */
    for (x = 0, y = 0; x < len; x++, y++) {
	switch (old[x]) {
	case '\n':
	    new[y++] = '\\';
	    new[y] = 'n';
	    break;
	case '\r':
	    new[y++] = '\\';
	    new[y] = 'r';
	    break;
	case '\'':
	    new[y++] = '\\';
	    new[y] = '\'';
	    break;
	case '"':
	    new[y++] = '\\';
	    new[y] = '"';
	    break;
	case 0:
	    new[y++] = '\\';
	    new[y] = '0';
	    break;
	default:
	    new[y] = old[x];
	    break;
	}
    }
    new[y] = 0;
}

/*
 * expand a filter (lookup or result)
 */
static void dict_pgsql_expand_filter(char *filter, char *value, VSTRING *out)
{
    const char *myname = "dict_pgsql_expand_filter";
    char   *sub,
           *end;

    /*
     * Yes, replace all instances of %s with the address to look up. Replace
     * %u with the user portion, and %d with the domain portion.
     */
    sub = filter;
    end = sub + strlen(filter);
    while (sub < end) {

	/*
	 * Make sure it's %[sud] and not something else.  For backward
	 * compatibilty, treat anything other than %u or %d as %s, with a
	 * warning.
	 */
	if (*(sub) == '%') {
	    char   *u = value;
	    char   *p = strrchr(u, '@');

	    switch (*(sub + 1)) {
	    case 'd':
		if (p)
		    vstring_strcat(out, p + 1);
		break;
	    case 'u':
		if (p)
		    vstring_strncat(out, u, p - u);
		else
		    vstring_strcat(out, u);
		break;
	    default:
		msg_warn
		    ("%s: Invalid filter substitution format '%%%c'!",
		     myname, *(sub + 1));
		break;
	    case 's':
		vstring_strcat(out, u);
		break;
	    }
	    sub++;
	} else
	    vstring_strncat(out, sub, 1);
	sub++;
    }
}

static const char *dict_pgsql_lookup(DICT *dict, const char *name)
{
    PGSQL_RES *query_res;
    DICT_PGSQL *dict_pgsql;
    PLPGSQL *pldb;
    static VSTRING *result;
    static VSTRING *query = 0;
    int     i,
            j,
            numrows;
    char   *name_escaped = 0;
    int     isFunctionCall;
    int     numcols;

    dict_pgsql = (DICT_PGSQL *) dict;
    pldb = dict_pgsql->pldb;
    /* initialization  for query */
    query = vstring_alloc(24);
    vstring_strcpy(query, "");
    if ((name_escaped = (char *) mymalloc((sizeof(char) * (strlen(name) * 2) +1))) == NULL) {
	msg_fatal("dict_pgsql_lookup: out of memory.");
    }
    /* prepare the query */
    pgsql_escape_string(name_escaped, name, (unsigned int) strlen(name));

    /* Build SQL - either a select from table or select a function */

    isFunctionCall = (dict_pgsql->name->select_function != NULL);
    if (isFunctionCall) {
	vstring_sprintf(query, "select %s('%s')",
			dict_pgsql->name->select_function,
			name_escaped);
    } else if (dict_pgsql->name->query) {
	dict_pgsql_expand_filter(dict_pgsql->name->query, name_escaped, query);
    } else {
	vstring_sprintf(query, "select %s from %s where %s = '%s' %s",
			dict_pgsql->name->select_field,
			dict_pgsql->name->table,
			dict_pgsql->name->where_field,
			name_escaped,
			dict_pgsql->name->additional_conditions);
    }

    if (msg_verbose)
	msg_info("dict_pgsql_lookup using sql query: %s", vstring_str(query));

    /* free mem associated with preparing the query */
    myfree(name_escaped);

    /* do the query - set dict_errno & cleanup if there's an error */
    if ((query_res = plpgsql_query(pldb,
				   vstring_str(query),
				   dict_pgsql->name->dbname,
				   dict_pgsql->name->username,
				   dict_pgsql->name->password)) == 0) {
	dict_errno = DICT_ERR_RETRY;
	vstring_free(query);
	return 0;
    }
    dict_errno = 0;
    /* free the vstring query */
    vstring_free(query);
    numrows = PQntuples(query_res);
    if (msg_verbose)
	msg_info("dict_pgsql_lookup: retrieved %d rows", numrows);
    if (numrows == 0) {
	PQclear(query_res);
	return 0;
    }
    numcols = PQnfields(query_res);

    if (numcols == 1 && numrows == 1 && isFunctionCall) {

	/*
	 * We do the above check because PostgreSQL 7.3 will allow functions
	 * to return result sets
	 */
	if (PQgetisnull(query_res, 0, 0) == 1) {

	    /*
	     * Functions returning a single row & column that is null are
	     * deemed to have not found the key.
	     */
	    PQclear(query_res);
	    return 0;
	}
    }
    if (result == 0)
	result = vstring_alloc(10);

    vstring_strcpy(result, "");
    for (i = 0; i < numrows; i++) {
	if (i > 0)
	    vstring_strcat(result, ",");
	for (j = 0; j < numcols; j++) {
	    if (j > 0)
		vstring_strcat(result, ",");
	    vstring_strcat(result, PQgetvalue(query_res, i, j));
	    if (msg_verbose > 1)
		msg_info("dict_pgsql_lookup: retrieved field: %d: %s", j, PQgetvalue(query_res, i, j));
	}
    }
    PQclear(query_res);
    return vstring_str(result);
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
	/*
	 * Calling myrand() can deplete the random pool.
	 * Don't rely on the optimizer to weed out the call
	 * when count == 1.
	 */
	idx = (count > 1) ? 1 + (count - 1) * (double) myrand() / RAND_MAX : 1;

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

static PGSQL_RES *plpgsql_query(PLPGSQL *PLDB,
				        const char *query,
				        char *dbname,
				        char *username,
				        char *password)
{
    HOST   *host;
    PGSQL_RES *res = 0;

    while ((host = dict_pgsql_get_active(PLDB, dbname, username, password)) != NULL) {
	if ((res = PQexec(host->db, query)) == 0) {
	    msg_warn("pgsql query failed: %s", PQerrorMessage(host->db));
	    plpgsql_down_host(host);
	} else {
	    if (msg_verbose)
		msg_info("dict_pgsql: successful query from host %s", host->hostname);
	    event_request_timer(dict_pgsql_event, (char *) host, IDLE_CONN_INTV);
	    break;
	}
    }

    return res;
}

/*
 * plpgsql_connect_single -
 * used to reconnect to a single database when one is down or none is
 * connected yet. Log all errors and set the stat field of host accordingly
 */
static void plpgsql_connect_single(HOST *host, char *dbname, char *username, char *password)
{
    if ((host->db = PQsetdbLogin(host->name, host->port, NULL, NULL,
				 dbname, username, password)) != NULL) {
	if (PQstatus(host->db) == CONNECTION_OK) {
	    if (msg_verbose)
		msg_info("dict_pgsql: successful connection to host %s",
			 host->hostname);
	    host->stat = STATACTIVE;
	} else {
	    msg_warn("connect to pgsql server %s: %s",
		     host->hostname, PQerrorMessage(host->db));
	    plpgsql_down_host(host);
	}
    } else {
	msg_warn("connect to pgsql server %s: %s",
		 host->hostname, PQerrorMessage(host->db));
	plpgsql_down_host(host);
    }
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

/**********************************************************************
 * public interface dict_pgsql_open
 *    create association with database with appropriate values
 *    parse the map's config file
 *    allocate memory
 **********************************************************************/
DICT   *dict_pgsql_open(const char *name, int open_flags, int dict_flags)
{
    DICT_PGSQL *dict_pgsql;

    /*
     * Sanity checks.
     */
    if (open_flags != O_RDONLY)
	msg_fatal("%s:%s map requires O_RDONLY access mode",
		  DICT_TYPE_PGSQL, name);

    dict_pgsql = (DICT_PGSQL *) dict_alloc(DICT_TYPE_PGSQL, name,
					   sizeof(DICT_PGSQL));
    dict_pgsql->dict.lookup = dict_pgsql_lookup;
    dict_pgsql->dict.close = dict_pgsql_close;
    dict_pgsql->name = pgsqlname_parse(name);
    dict_pgsql->pldb = plpgsql_init(dict_pgsql->name->hostnames,
				    dict_pgsql->name->len_hosts);
    dict_pgsql->dict.flags = dict_flags | DICT_FLAG_FIXED;
    if (dict_pgsql->pldb == NULL)
	msg_fatal("couldn't intialize pldb!\n");
    return &dict_pgsql->dict;
}

/* pgsqlname_parse - parse pgsql configuration file */
static PGSQL_NAME *pgsqlname_parse(const char *pgsqlcf)
{
    const char *myname = "pgsqlname_parse";
    int     i;
    char   *hosts;
    PGSQL_NAME *name = (PGSQL_NAME *) mymalloc(sizeof(PGSQL_NAME));
    ARGV   *hosts_argv;

    name->parser = cfg_parser_alloc(pgsqlcf);

    /* username */
    name->username = cfg_get_str(name->parser, "user", "", 0, 0);

    /* password */
    name->password = cfg_get_str(name->parser, "password", "", 0, 0);

    /* database name lookup */
    name->dbname = cfg_get_str(name->parser, "dbname", "", 1, 0);

    /*
     * See what kind of lookup we have - a traditional 'select' or a function
     * call
     */
    name->select_function = cfg_get_str(name->parser, "select_function",
					NULL, 0, 0);
    name->query = cfg_get_str(name->parser, "query", NULL, 0, 0);

    if (name->select_function == 0 && name->query == 0) {

	/*
	 * We have an old style 'select %s from %s...' call
	 */

	/* table name */
	name->table = cfg_get_str(name->parser, "table", "", 1, 0);

	/* select field */
	name->select_field = cfg_get_str(name->parser, "select_field",
					 "", 1, 0);

	/* where field */
	name->where_field = cfg_get_str(name->parser, "where_field",
					"", 1, 0);

	/* additional conditions */
	name->additional_conditions = cfg_get_str(name->parser,
						  "additional_conditions",
						  "", 0, 0);
    } else {
	name->table = 0;
	name->select_field = 0;
	name->where_field = 0;
	name->additional_conditions = 0;
    }

    /* server hosts */
    hosts = cfg_get_str(name->parser, "hosts", "", 0, 0);

    /* coo argv interface */
    hosts_argv = argv_split(hosts, " ,\t\r\n");
    if (hosts_argv->argc == 0) {		/* no hosts specified,
						 * default to 'localhost' */
	if (msg_verbose)
	    msg_info("%s: %s: no hostnames specified, defaulting to 'localhost'",
		     myname, pgsqlcf);
	argv_add(hosts_argv, "localhost", ARGV_END);
	argv_terminate(hosts_argv);
    }
    name->len_hosts = hosts_argv->argc;
    name->hostnames = (char **) mymalloc((sizeof(char *)) * name->len_hosts);
    i = 0;
    for (i = 0; hosts_argv->argv[i] != NULL; i++) {
	name->hostnames[i] = mystrdup(hosts_argv->argv[i]);
	if (msg_verbose)
	    msg_info("%s: %s: adding host '%s' to list of pgsql server hosts",
		     myname, pgsqlcf, name->hostnames[i]);
    }
    myfree(hosts);
    argv_free(hosts_argv);
    return name;
}


/*
 * plpgsql_init - initalize a PGSQL database.
 *		    Return NULL on failure, or a PLPGSQL * on success.
 */
static PLPGSQL *plpgsql_init(char *hostnames[], int len_hosts)
{
    PLPGSQL *PLDB;
    int     i;

    if ((PLDB = (PLPGSQL *) mymalloc(sizeof(PLPGSQL))) == NULL) {
	msg_fatal("mymalloc of pldb failed");
    }
    PLDB->len_hosts = len_hosts;
    if ((PLDB->db_hosts = (HOST **) mymalloc(sizeof(HOST *) * len_hosts)) == NULL)
	return NULL;
    for (i = 0; i < len_hosts; i++) {
	PLDB->db_hosts[i] = host_init(hostnames[i]);
    }
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

/**********************************************************************
 * public interface dict_pgsql_close
 * unregister, disassociate from database, freeing appropriate memory
 **********************************************************************/
static void dict_pgsql_close(DICT *dict)
{
    int     i;
    DICT_PGSQL *dict_pgsql = (DICT_PGSQL *) dict;

    plpgsql_dealloc(dict_pgsql->pldb);
    cfg_parser_free(dict_pgsql->name->parser);
    myfree(dict_pgsql->name->username);
    myfree(dict_pgsql->name->password);
    myfree(dict_pgsql->name->dbname);
    if (dict_pgsql->name->table)
	myfree(dict_pgsql->name->table);
    if (dict_pgsql->name->query)
	myfree(dict_pgsql->name->query);
    if (dict_pgsql->name->select_function)
	myfree(dict_pgsql->name->select_function);
    if (dict_pgsql->name->select_field)
	myfree(dict_pgsql->name->select_field);
    if (dict_pgsql->name->where_field)
	myfree(dict_pgsql->name->where_field);
    if (dict_pgsql->name->additional_conditions)
	myfree(dict_pgsql->name->additional_conditions);
    for (i = 0; i < dict_pgsql->name->len_hosts; i++) {
	myfree(dict_pgsql->name->hostnames[i]);
    }
    myfree((char *) dict_pgsql->name->hostnames);
    myfree((char *) dict_pgsql->name);
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
