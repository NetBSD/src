/*++
/* NAME
/*	dict_mysql 3
/* SUMMARY
/*	dictionary manager interface to db files
/* SYNOPSIS
/*	#include <dict.h>
/*	#include <dict_mysql.h>
/*
/*	DICT	*dict_mysql_open(name, dummy, unused_dict_flags)
/*	const char	*name;
/*	int     dummy;
/*	int     unused_dict_flags;
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
/*
/*	Arguments:
/* .IP name
/*	The path of the MySQL configuration file.  The file encodes a number of
/*	pieces of information: username, password, databasename, table,
/*	select_field, where_field, and hosts.  For example, if you want the map to
/*	reference databases of the name "your_db" and execute a query like this:
/*	select forw_addr from aliases where alias like '<some username>' against
/*	any database called "vmailer_info" located on hosts host1.some.domain and
/*	host2.some.domain, logging in as user "vmailer" and password "passwd" then
/*	the configuration file should read:
/*
/*	user = vmailer
/*	password = passwd
/*	DBname = vmailer_info
/*	table = aliases
/*	select_field = forw_addr
/*	where_field = alias
/*	hosts = host1.some.domain host2.some.domain
/*
/* .IP other_name
/*	reference for outside use.
/* .IP unusued_flags
/*	unused flags
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
#include "dict_mysql.h"
#include "argv.h"
#include "vstring.h"
#include "split_at.h"
#include "find_inet.h"

/* need some structs to help organize things */
typedef struct {
    MYSQL  *db;
    char   *hostname;
    int     stat;			/* STATUNTRIED | STATFAIL | STATCUR */
    time_t  ts;				/* used for attempting reconnection
					 * every so often if a host is down */
} HOST;

typedef struct {
    int     len_hosts;			/* number of hosts */
    HOST   *db_hosts;			/* the hosts on which the databases
					 * reside */
} PLMYSQL;

typedef struct {
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

#define STATACTIVE 0
#define STATFAIL 1
#define STATUNTRIED 2
#define RETRY_CONN_INTV 60		/* 1 minute */

/* internal function declarations */
static PLMYSQL *plmysql_init(char *hostnames[], int);
static MYSQL_RES *plmysql_query(PLMYSQL *, const char *, char *, char *, char *);
static void plmysql_dealloc(PLMYSQL *);
static void plmysql_down_host(HOST *);
static void plmysql_connect_single(HOST *, char *, char *, char *);
static int plmysql_ready_reconn(HOST *);
static const char *dict_mysql_lookup(DICT *, const char *);
DICT   *dict_mysql_open(const char *, int, int);
static void dict_mysql_close(DICT *);
static MYSQL_NAME *mysqlname_parse(const char *);
static HOST host_init(char *);



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

/*
 * plmysql_query - process a MySQL query.  Return MYSQL_RES* on success.
 *		     On failure, log failure and try other db instances.
 *		     on failure of all db instances, return 0;
 *		     close unnecessary active connections
 */

static MYSQL_RES *plmysql_query(PLMYSQL *PLDB,
				        const char *query,
				        char *dbname,
				        char *username,
				        char *password)
{
    int     i;
    HOST   *host;
    MYSQL_RES *res = 0;

    for (i = 0; i < PLDB->len_hosts; i++) {
	/* can't deal with typing or reading PLDB->db_hosts[i] over & over */
	host = &(PLDB->db_hosts[i]);
	if (msg_verbose > 1)
	    msg_info("dict_mysql: trying host %s stat %d, last res %p", host->hostname, host->stat, res);

	/* answer already found */
	if (res != 0 && host->stat == STATACTIVE) {
	    if (msg_verbose)
		msg_info("dict_mysql: closing unnessary connection to %s",
			 host->hostname);
	    plmysql_down_host(host);
	}
	/* try to connect for the first time if we don't have a result yet */
	if (res == 0 && host->stat == STATUNTRIED) {
	    if (msg_verbose)
		msg_info("dict_mysql: attempting to connect to host %s",
			 host->hostname);
	    plmysql_connect_single(host, dbname, username, password);
	}

	/*
	 * try to reconnect if we don't have an answer and the host had a
	 * prob in the past and it's time for it to reconnect
	 */
	if (res == 0 && host->stat == STATFAIL && host->ts < time((time_t *) 0)) {
	    if (msg_verbose)
		msg_info("dict_mysql: attempting to reconnect to host %s",
			 host->hostname);
	    plmysql_connect_single(host, dbname, username, password);
	}

	/*
	 * if we don't have a result and the current host is marked active,
	 * try the query.  If the query fails, mark the host STATFAIL
	 */
	if (res == 0 && host->stat == STATACTIVE) {
	    if (!(mysql_query(host->db, query))) {
		if ((res = mysql_store_result(host->db)) == 0) {
		    msg_warn("%s", mysql_error(host->db));
		    plmysql_down_host(host);
		} else {
		    if (msg_verbose)
			msg_info("dict_mysql: successful query from host %s", host->hostname);
		}
	    } else {
		msg_warn("%s", mysql_error(host->db));
		plmysql_down_host(host);
	    }
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
    char   *destination = host->hostname;
    char   *unix_socket = 0;
    char   *hostname = 0;
    char   *service;
    unsigned port = 0;

    /*
     * Ad-hoc parsing code. Expect "unix:pathname" or "inet:host:port", where
     * both "inet:" and ":port" are optional.
     */
    if (strncmp(destination, "unix:", 5) == 0) {
	unix_socket = destination + 5;
    } else {
	if (strncmp(destination, "inet:", 5) == 0)
	    destination += 5;
	hostname = mystrdup(destination);
	if ((service = split_at(hostname, ':')) != 0)
	    port = ntohs(find_inet_port(service, "tcp"));
    }

    host->db = mysql_init(NULL);
    if (mysql_real_connect(host->db, hostname, username, password, dbname, port, unix_socket, 0)) {
	if (msg_verbose)
	    msg_info("dict_mysql: successful connection to host %s",
		     host->hostname);
	host->stat = STATACTIVE;
    } else {
	msg_warn("%s", mysql_error(host->db));
	plmysql_down_host(host);
    }
    if (hostname)
	myfree(hostname);
}

/*
 * plmysql_down_host - mark a HOST down update ts if marked down
 * for the first time so that we'll know when to retry the connection
 */
static void plmysql_down_host(HOST *host)
{
    if (host->stat != STATFAIL) {
	host->ts = time((time_t *) 0) + RETRY_CONN_INTV;
	host->stat = STATFAIL;
    }
    mysql_close(host->db);
    host->db = 0;
}

/**********************************************************************
 * public interface dict_mysql_open
 *    create association with database with appropriate values
 *    parse the map's config file
 *    allocate memory
 **********************************************************************/
DICT   *dict_mysql_open(const char *name, int unused_open_flags, int dict_flags)
{
    DICT_MYSQL *dict_mysql;
    int     connections;

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
    dict_register(name, (DICT *) dict_mysql);
    return (DICT_DEBUG(&dict_mysql->dict));
}

/* mysqlname_parse - parse mysql configuration file */
static MYSQL_NAME *mysqlname_parse(const char *mysqlcf_path)
{
    int     i;
    char   *nameval;
    char   *hosts;
    MYSQL_NAME *name = (MYSQL_NAME *) mymalloc(sizeof(MYSQL_NAME));
    ARGV   *hosts_argv;
    VSTRING *opt_dict_name;

    /*
     * setup a dict containing info in the mysql cf file. the dict has a
     * name, and a path.  The name must be distinct from the path, or the
     * dict interface gets confused.  The name must be distinct for two
     * different paths, or the configuration info will cache across different
     * mysql maps, which can be confusing.
     */
    opt_dict_name = vstring_alloc(64);
    vstring_sprintf(opt_dict_name, "mysql opt dict %s", mysqlcf_path);
    dict_load_file(vstring_str(opt_dict_name), mysqlcf_path);
    /* mysql username lookup */
    if ((nameval = (char *) dict_lookup(vstring_str(opt_dict_name), "user")) == NULL)
	name->username = mystrdup("");
    else
	name->username = mystrdup(nameval);
    if (msg_verbose)
	msg_info("mysqlname_parse(): set username to '%s'", name->username);
    /* password lookup */
    if ((nameval = (char *) dict_lookup(vstring_str(opt_dict_name), "password")) == NULL)
	name->password = mystrdup("");
    else
	name->password = mystrdup(nameval);
    if (msg_verbose)
	msg_info("mysqlname_parse(): set password to '%s'", name->password);

    /* database name lookup */
    if ((nameval = (char *) dict_lookup(vstring_str(opt_dict_name), "dbname")) == NULL)
	msg_fatal("%s: mysql options file does not include database name", mysqlcf_path);
    else
	name->dbname = mystrdup(nameval);
    if (msg_verbose)
	msg_info("mysqlname_parse(): set database name to '%s'", name->dbname);

    /* table lookup */
    if ((nameval = (char *) dict_lookup(vstring_str(opt_dict_name), "table")) == NULL)
	msg_fatal("%s: mysql options file does not include table name", mysqlcf_path);
    else
	name->table = mystrdup(nameval);
    if (msg_verbose)
	msg_info("mysqlname_parse(): set table name to '%s'", name->table);

    /* select field lookup */
    if ((nameval = (char *) dict_lookup(vstring_str(opt_dict_name), "select_field")) == NULL)
	msg_fatal("%s: mysql options file does not include select field", mysqlcf_path);
    else
	name->select_field = mystrdup(nameval);
    if (msg_verbose)
	msg_info("mysqlname_parse(): set select_field to '%s'", name->select_field);

    /* where field lookup */
    if ((nameval = (char *) dict_lookup(vstring_str(opt_dict_name), "where_field")) == NULL)
	msg_fatal("%s: mysql options file does not include where field", mysqlcf_path);
    else
	name->where_field = mystrdup(nameval);
    if (msg_verbose)
	msg_info("mysqlname_parse(): set where_field to '%s'", name->where_field);

    /* additional conditions */
    if ((nameval = (char *) dict_lookup(vstring_str(opt_dict_name), "additional_conditions")) == NULL)
	name->additional_conditions = mystrdup("");
    else
	name->additional_conditions = mystrdup(nameval);
    if (msg_verbose)
	msg_info("mysqlname_parse(): set additional_conditions to '%s'", name->additional_conditions);

    /* mysql server hosts */
    if ((nameval = (char *) dict_lookup(vstring_str(opt_dict_name), "hosts")) == NULL)
	hosts = mystrdup("");
    else
	hosts = mystrdup(nameval);
    /* coo argv interface */
    hosts_argv = argv_split(hosts, " ,\t\r\n");

    if (hosts_argv->argc == 0) {		/* no hosts specified,
						 * default to 'localhost' */
	if (msg_verbose)
	    msg_info("mysqlname_parse(): no hostnames specified, defaulting to 'localhost'");
	argv_add(hosts_argv, "localhost", ARGV_END);
	argv_terminate(hosts_argv);
    }
    name->len_hosts = hosts_argv->argc;
    name->hostnames = (char **) mymalloc((sizeof(char *)) * name->len_hosts);
    i = 0;
    for (i = 0; hosts_argv->argv[i] != NULL; i++) {
	name->hostnames[i] = mystrdup(hosts_argv->argv[i]);
	if (msg_verbose)
	    msg_info("mysqlname_parse(): adding host '%s' to list of mysql server hosts",
		     name->hostnames[i]);
    }
    myfree(hosts);
    vstring_free(opt_dict_name);
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
    MYSQL  *dbs;
    int     i;
    HOST    host;

    if ((PLDB = (PLMYSQL *) mymalloc(sizeof(PLMYSQL))) == NULL) {
	msg_fatal("mymalloc of pldb failed");
    }
    PLDB->len_hosts = len_hosts;
    if ((PLDB->db_hosts = (HOST *) mymalloc(sizeof(HOST) * len_hosts)) == NULL)
	return NULL;
    for (i = 0; i < len_hosts; i++) {
	PLDB->db_hosts[i] = host_init(hostnames[i]);
    }
    return PLDB;
}


/* host_init - initialize HOST structure */
static HOST host_init(char *hostname)
{
    HOST    host;

    host.stat = STATUNTRIED;
    host.hostname = mystrdup(hostname);
    host.db = 0;
    host.ts = 0;
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
	if (PLDB->db_hosts[i].db)
	    mysql_close(PLDB->db_hosts[i].db);
	myfree(PLDB->db_hosts[i].hostname);
    }
    myfree((char *) PLDB->db_hosts);
    myfree((char *) (PLDB));
}

#endif
