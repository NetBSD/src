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
/* .IP \fItable\fR
/*	Name of the table.
/* .IP \fIselect_field\fR
/*	Name of the result field.
/* .IP \fIwhere_field\fR
/*	Field used in the WHERE clause.
/* .IP \fIadditional_conditions\fR
/*	Additional conditions to the WHERE clause.
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
    CFG_PARSER *parser;
    char   *username;
    char   *password;
    char   *dbname;
    char   *table;
    char   *select_field;
    char   *where_field;
    char   *additional_conditions;
    char  **hostnames;
    int     len_hosts;
} MYSQL_NAME;

typedef struct {
    DICT    dict;
    PLMYSQL *pldb;
    MYSQL_NAME *name;
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
static PLMYSQL *plmysql_init(char *hostnames[], int);
static MYSQL_RES *plmysql_query(PLMYSQL *, const char *, char *, char *, char *);
static void plmysql_dealloc(PLMYSQL *);
static void plmysql_close_host(HOST *);
static void plmysql_down_host(HOST *);
static void plmysql_connect_single(HOST *, char *, char *, char *);
static const char *dict_mysql_lookup(DICT *, const char *);
DICT   *dict_mysql_open(const char *, int, int);
static void dict_mysql_close(DICT *);
static MYSQL_NAME *mysqlname_parse(const char *);
static HOST *host_init(const char *);



/**********************************************************************
 * public interface dict_mysql_lookup
 * find database entry return 0 if no alias found, set dict_errno
 * on errors to DICT_ERRBO_RETRY and set dict_errno to 0 on success
 *********************************************************************/
static const char *dict_mysql_lookup(DICT *dict, const char *name)
{
    MYSQL_RES *query_res;
    MYSQL_ROW row;
    DICT_MYSQL *dict_mysql;
    PLMYSQL *pldb;
    static VSTRING *result;
    static VSTRING *query = 0;
    int     i,
            j,
            numrows;
    char   *name_escaped = 0;

    dict_mysql = (DICT_MYSQL *) dict;
    pldb = dict_mysql->pldb;
    /* initialization  for query */
    query = vstring_alloc(24);
    vstring_strcpy(query, "");
    if ((name_escaped = (char *) mymalloc((sizeof(char) * (strlen(name) * 2) +1))) == NULL) {
	msg_fatal("dict_mysql_lookup: out of memory.");
    }
    /* prepare the query */
    mysql_escape_string(name_escaped, name, (unsigned int) strlen(name));
    vstring_sprintf(query, "select %s from %s where %s = '%s' %s", dict_mysql->name->select_field,
       dict_mysql->name->table, dict_mysql->name->where_field, name_escaped,
		    dict_mysql->name->additional_conditions);
    if (msg_verbose)
	msg_info("dict_mysql_lookup using sql query: %s", vstring_str(query));
    /* free mem associated with preparing the query */
    myfree(name_escaped);
    /* do the query - set dict_errno & cleanup if there's an error */
    if ((query_res = plmysql_query(pldb,
				   vstring_str(query),
				   dict_mysql->name->dbname,
				   dict_mysql->name->username,
				   dict_mysql->name->password)) == 0) {
	dict_errno = DICT_ERR_RETRY;
	vstring_free(query);
	return 0;
    }
    dict_errno = 0;
    /* free the vstring query */
    vstring_free(query);
    numrows = mysql_num_rows(query_res);
    if (msg_verbose)
	msg_info("dict_mysql_lookup: retrieved %d rows", numrows);
    if (numrows == 0) {
	mysql_free_result(query_res);
	return 0;
    }
    if (result == 0)
	result = vstring_alloc(10);
    vstring_strcpy(result, "");
    for (i = 0; i < numrows; i++) {
	row = mysql_fetch_row(query_res);
	if (i > 0)
	    vstring_strcat(result, ",");
	for (j = 0; j < mysql_num_fields(query_res); j++) {
	    if (row[j] == 0) {
		if (msg_verbose > 1)
		    msg_info("dict_mysql_lookup: null field #%d row #%d", j, i);
		mysql_free_result(query_res);
		return (0);
	    }
	    if (j > 0)
		vstring_strcat(result, ",");
	    vstring_strcat(result, row[j]);
	    if (msg_verbose > 1)
		msg_info("dict_mysql_lookup: retrieved field: %d: %s", j, row[j]);
	}
    }
    mysql_free_result(query_res);
    return vstring_str(result);
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
	/*
	 * Calling myrand() can deplete the random pool.
	 * Don't rely on the optimizer to weed out the call
	 * when count == 1.
	 */
	idx = (count > 1) ? 1 + (count - 1) * (double) myrand() / RAND_MAX : 1;

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

static MYSQL_RES *plmysql_query(PLMYSQL *PLDB,
				        const char *query,
				        char *dbname,
				        char *username,
				        char *password)
{
    HOST   *host;
    MYSQL_RES *res = 0;

    while ((host = dict_mysql_get_active(PLDB, dbname, username, password)) != NULL) {
	if (!(mysql_query(host->db, query))) {
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

/**********************************************************************
 * public interface dict_mysql_open
 *    create association with database with appropriate values
 *    parse the map's config file
 *    allocate memory
 **********************************************************************/
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
    dict_mysql->dict.flags = dict_flags | DICT_FLAG_FIXED;
    dict_mysql->name = mysqlname_parse(name);
    dict_mysql->pldb = plmysql_init(dict_mysql->name->hostnames,
				    dict_mysql->name->len_hosts);
    if (dict_mysql->pldb == NULL)
	msg_fatal("couldn't intialize pldb!\n");
    return (DICT_DEBUG (&dict_mysql->dict));
}

/* mysqlname_parse - parse mysql configuration file */
static MYSQL_NAME *mysqlname_parse(const char *mysqlcf)
{
    const char *myname = "mysqlname_parse";
    int     i;
    char   *hosts;
    MYSQL_NAME *name = (MYSQL_NAME *) mymalloc(sizeof(MYSQL_NAME));
    ARGV   *hosts_argv;

    /* parser */
    name->parser = cfg_parser_alloc(mysqlcf);

    /* username */
    name->username = cfg_get_str(name->parser, "user", "", 0, 0);

    /* password */
    name->password = cfg_get_str(name->parser, "password", "", 0, 0);

    /* database name */
    name->dbname = cfg_get_str(name->parser, "dbname", "", 1, 0);

    /* table name */
    name->table = cfg_get_str(name->parser, "table", "", 1, 0);

    /* select field */
    name->select_field = cfg_get_str(name->parser, "select_field", "", 1, 0);

    /* where field */
    name->where_field = cfg_get_str(name->parser, "where_field", "", 1, 0);

    /* additional conditions */
    name->additional_conditions = cfg_get_str(name->parser,
					      "additional_conditions",
					      "", 0, 0);

    /* mysql server hosts */
    hosts = cfg_get_str(name->parser, "hosts", "", 0, 0);

    /* coo argv interface */
    hosts_argv = argv_split(hosts, " ,\t\r\n");
    if (hosts_argv->argc == 0) {		/* no hosts specified,
						 * default to 'localhost' */
	if (msg_verbose)
	    msg_info("%s: %s: no hostnames specified, defaulting to 'localhost'",
		     myname, mysqlcf);
	argv_add(hosts_argv, "localhost", ARGV_END);
	argv_terminate(hosts_argv);
    }
    name->len_hosts = hosts_argv->argc;
    name->hostnames = (char **) mymalloc((sizeof(char *)) * name->len_hosts);
    i = 0;
    for (i = 0; hosts_argv->argv[i] != NULL; i++) {
	name->hostnames[i] = mystrdup(hosts_argv->argv[i]);
	if (msg_verbose)
	    msg_info("%s: %s: adding host '%s' to list of mysql server hosts",
		     myname, mysqlcf, name->hostnames[i]);
    }
    myfree(hosts);
    argv_free(hosts_argv);
    return name;
}


/*
 * plmysql_init - initalize a MYSQL database.
 *		    Return NULL on failure, or a PLMYSQL * on success.
 */
static PLMYSQL *plmysql_init(char *hostnames[], int len_hosts)
{
    PLMYSQL *PLDB;
    int     i;

    if ((PLDB = (PLMYSQL *) mymalloc(sizeof(PLMYSQL))) == NULL) {
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

/**********************************************************************
 * public interface dict_mysql_close
 * unregister, disassociate from database, freeing appropriate memory
 **********************************************************************/
static void dict_mysql_close(DICT *dict)
{
    int     i;
    DICT_MYSQL *dict_mysql = (DICT_MYSQL *) dict;

    plmysql_dealloc(dict_mysql->pldb);
    cfg_parser_free(dict_mysql->name->parser);
    myfree(dict_mysql->name->username);
    myfree(dict_mysql->name->password);
    myfree(dict_mysql->name->dbname);
    myfree(dict_mysql->name->table);
    myfree(dict_mysql->name->select_field);
    myfree(dict_mysql->name->where_field);
    myfree(dict_mysql->name->additional_conditions);
    for (i = 0; i < dict_mysql->name->len_hosts; i++) {
	myfree(dict_mysql->name->hostnames[i]);
    }
    myfree((char *) dict_mysql->name->hostnames);
    myfree((char *) dict_mysql->name);
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
