/*++
/* NAME
/*	proxymap 8
/* SUMMARY
/*	Postfix lookup table proxy server
/* SYNOPSIS
/*	\fBproxymap\fR [generic Postfix daemon options]
/* DESCRIPTION
/*	The \fBproxymap\fR server provides read-only table
/*	lookup service to Postfix processes. The purpose
/*	of the service is:
/* .IP \(bu
/*	To overcome chroot restrictions. For example, a chrooted SMTP
/*	server needs access to the system passwd file in order to
/*	reject mail for non-existent local addresses, but it is not
/*	practical to maintain a copy of the passwd file in the chroot
/*	jail.  The solution:
/* .sp
/*	local_recipient_maps =
/* .ti +4
/*	proxy:unix:passwd.byname $alias_maps
/* .IP \(bu
/*	To consolidate the number of open lookup tables by sharing
/*	one open table among multiple processes. For example, making
/*	mysql connections from every Postfix daemon process results
/*	in "too many connections" errors. The solution:
/* .sp
/*	virtual_alias_maps =
/* .ti +4
/*	proxy:mysql:/etc/postfix/virtual_alias.cf
/* .sp
/*	The total number of connections is limited by the number of
/*	proxymap server processes.
/* .PP
/*	The proxymap server implements the following requests:
/* .IP "\fBopen\fR \fImaptype:mapname flags\fR"
/*	Open the table with type \fImaptype\fR and name \fImapname\fR,
/*	as controlled by \fIflags\fR. The reply includes the \fImaptype\fR
/*	dependent flags (to distinguish a fixed string table from regular
/*	a expression table).
/* .IP "\fBlookup\fR \fImaptype:mapname flags key\fR"
/*	Look up the data stored under the requested key.
/*	The reply is the request completion status code (below) and
/*	the lookup result value.
/*	The \fImaptype:mapname\fR and \fIflags\fR are the same
/*	as with the \fBopen\fR request.
/* .PP
/*	There is no \fBclose\fR command, nor are tables implicitly closed
/*	when a client disconnects. The purpose is to share tables among
/*	multiple client processes.
/* SERVER PROCESS MANAGEMENT
/* .ad
/* .fi
/*	\fBproxymap\fR servers run under control by the Postfix \fBmaster\fR
/*	server.  Each server can handle multiple simultaneous connections.
/*	When all servers are busy while a client connects, the \fBmaster\fR
/*	creates a new \fBproxymap\fR server process, provided that the
/*	process limit is not exceeded.
/*	Each server terminates after serving at least \fB$max_use\fR clients
/*	or after \fB$max_idle\fR seconds of idle time.
/* SECURITY
/* .ad
/* .fi
/*	The proxymap server opens only tables that are approved via the
/*	\fBproxy_read_maps\fR configuration parameter, does not talk to
/*	users, and can run at fixed low privilege, chrooted or not.
/*	However, running the proxymap server chrooted severely limits
/*	usability, because it can open only chrooted tables.
/*
/*	The proxymap server is not a trusted daemon process, and must
/*	not be used to look up sensitive information such as user or
/*	group IDs, mailbox file/directory names or external commands.
/* DIAGNOSTICS
/*	Problems and transactions are logged to \fBsyslogd\fR(8).
/* BUGS
/*	The proxymap server provides service to multiple clients,
/*	and must therefore not be used for tables that have high-latency
/*	lookups.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	On busy mail systems a long time may pass before proxymap(8) relevant
/*	changes to \fBmain.cf\fR are picked up. Use the command
/*	"\fBpostfix reload\fR" to speed up a change.
/*
/*	The text below provides only a parameter summary. See
/*	postconf(5) for more details including examples.
/* .IP "\fBconfig_directory (see 'postconf -d' output)\fR"
/*	The default location of the Postfix main.cf and master.cf
/*	configuration files.
/* .IP "\fBdaemon_timeout (18000s)\fR"
/*	How much time a Postfix daemon process may take to handle a
/*	request before it is terminated by a built-in watchdog timer.
/* .IP "\fBipc_timeout (3600s)\fR"
/*	The time limit for sending or receiving information over an internal
/*	communication channel.
/* .IP "\fBmax_idle (100s)\fR"
/*	The maximum amount of time that an idle Postfix daemon process
/*	waits for the next service request before exiting.
/* .IP "\fBmax_use (100)\fR"
/*	The maximal number of connection requests before a Postfix daemon
/*	process terminates.
/* .IP "\fBprocess_id (read-only)\fR"
/*	The process ID of a Postfix command or daemon process.
/* .IP "\fBprocess_name (read-only)\fR"
/*	The process name of a Postfix command or daemon process.
/* .IP "\fBproxy_read_maps (see 'postconf -d' output)\fR"
/*	The lookup tables that the proxymap(8) server is allowed to access.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* HISTORY
/* .ad
/* .fi
/*	The proxymap service was introduced with Postfix 2.0.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <vstring.h>
#include <htable.h>
#include <stringops.h>
#include <dict.h>

/* Global library. */

#include <mail_conf.h>
#include <mail_params.h>
#include <mail_proto.h>
#include <dict_proxy.h>

/* Server skeleton. */

#include <mail_server.h>

/* Application-specific. */

 /*
  * XXX All but the last are needed here so that $name expansion dependencies
  * aren't too broken. The fix is to gather all parameter default settings in
  * one place.
  */
char   *var_local_rcpt_maps;
char   *var_virt_alias_maps;
char   *var_virt_alias_doms;
char   *var_virt_mailbox_maps;
char   *var_virt_mailbox_doms;
char   *var_relay_rcpt_maps;
char   *var_relay_domains;
char   *var_canonical_maps;
char   *var_send_canon_maps;
char   *var_rcpt_canon_maps;
char   *var_relocated_maps;
char   *var_transport_maps;
char   *var_proxy_read_maps;

 /*
  * The pre-approved, pre-parsed list of maps.
  */
static HTABLE *proxy_read_maps;

 /*
  * Shared and static to reduce memory allocation overhead.
  */
static VSTRING *request;
static VSTRING *request_map;
static VSTRING *request_key;
static VSTRING *map_type_name_flags;

 /*
  * Silly little macros.
  */
#define STR(x)			vstring_str(x)
#define VSTREQ(x,y)		(strcmp(STR(x),y) == 0)

/* proxy_map_find - look up or open table */

static DICT *proxy_map_find(const char *map_type_name, int request_flags,
			            int *statp)
{
    DICT   *dict;

#define PROXY_COLON	DICT_TYPE_PROXY ":"
#define PROXY_COLON_LEN	(sizeof(PROXY_COLON) - 1)
#define READ_OPEN_FLAGS	O_RDONLY

    /*
     * Canonicalize the map name. If the map is not on the approved list,
     * deny the request.
     */
#define PROXY_MAP_FIND_ERROR_RETURN(x)  { *statp = (x); return (0); }

    while (strncmp(map_type_name, PROXY_COLON, PROXY_COLON_LEN) == 0)
	map_type_name += PROXY_COLON_LEN;
    if (strchr(map_type_name, ':') == 0)
	PROXY_MAP_FIND_ERROR_RETURN(PROXY_STAT_BAD);
    if (htable_locate(proxy_read_maps, map_type_name) == 0) {
	msg_warn("request for unapproved table: \"%s\"", map_type_name);
	msg_warn("to approve this table for %s access, list %s:%s in %s:%s",
		 MAIL_SERVICE_PROXYMAP, DICT_TYPE_PROXY, map_type_name,
		 MAIN_CONF_FILE, VAR_PROXY_READ_MAPS);
	PROXY_MAP_FIND_ERROR_RETURN(PROXY_STAT_DENY);
    }

    /*
     * Open one instance of a map for each combination of name+flags.
     */
    vstring_sprintf(map_type_name_flags, "%s:%o",
		    map_type_name, request_flags);
    if ((dict = dict_handle(STR(map_type_name_flags))) == 0)
	dict = dict_open(map_type_name, READ_OPEN_FLAGS, request_flags);
    if (dict == 0)
	msg_panic("proxy_map_find: dict_open null result");
    dict_register(STR(map_type_name_flags), dict);
    return (dict);
}

/* proxymap_lookup_service - remote lookup service */

static void proxymap_lookup_service(VSTREAM *client_stream)
{
    int     request_flags;
    DICT   *dict;
    const char *reply_value;
    int     reply_status;

    /*
     * Process the request.
     */
    if (attr_scan(client_stream, ATTR_FLAG_STRICT,
		  ATTR_TYPE_STR, MAIL_ATTR_TABLE, request_map,
		  ATTR_TYPE_NUM, MAIL_ATTR_FLAGS, &request_flags,
		  ATTR_TYPE_STR, MAIL_ATTR_KEY, request_key,
		  ATTR_TYPE_END) != 3) {
	reply_status = PROXY_STAT_BAD;
	reply_value = "";
    } else if ((dict = proxy_map_find(STR(request_map), request_flags,
				      &reply_status)) == 0) {
	reply_value = "";
    } else if ((reply_value = dict_get(dict, STR(request_key))) != 0) {
	reply_status = PROXY_STAT_OK;
    } else if (dict_errno == 0) {
	reply_status = PROXY_STAT_NOKEY;
	reply_value = "";
    } else {
	reply_status = PROXY_STAT_RETRY;
	reply_value = "";
    }

    /*
     * Respond to the client.
     */
    attr_print(client_stream, ATTR_FLAG_NONE,
	       ATTR_TYPE_NUM, MAIL_ATTR_STATUS, reply_status,
	       ATTR_TYPE_STR, MAIL_ATTR_VALUE, reply_value,
	       ATTR_TYPE_END);
}

/* proxymap_open_service - open remote lookup table */

static void proxymap_open_service(VSTREAM *client_stream)
{
    int     request_flags;
    DICT   *dict;
    int     reply_status;
    int     reply_flags;

    /*
     * Process the request.
     */
    if (attr_scan(client_stream, ATTR_FLAG_STRICT,
		  ATTR_TYPE_STR, MAIL_ATTR_TABLE, request_map,
		  ATTR_TYPE_NUM, MAIL_ATTR_FLAGS, &request_flags,
		  ATTR_TYPE_END) != 2) {
	reply_status = PROXY_STAT_BAD;
	reply_flags = 0;
    } else if ((dict = proxy_map_find(STR(request_map), request_flags,
				      &reply_status)) == 0) {
	reply_flags = 0;
    } else {
	reply_status = PROXY_STAT_OK;
	reply_flags = dict->flags;
    }

    /*
     * Respond to the client.
     */
    attr_print(client_stream, ATTR_FLAG_NONE,
	       ATTR_TYPE_NUM, MAIL_ATTR_STATUS, reply_status,
	       ATTR_TYPE_NUM, MAIL_ATTR_FLAGS, reply_flags,
	       ATTR_TYPE_END);
}

/* proxymap_service - perform service for client */

static void proxymap_service(VSTREAM *client_stream, char *unused_service,
			             char **argv)
{

    /*
     * Sanity check. This service takes no command-line arguments.
     */
    if (argv[0])
	msg_fatal("unexpected command-line argument: %s", argv[0]);

    /*
     * This routine runs whenever a client connects to the socket dedicated
     * to the proxymap service. All connection-management stuff is handled by
     * the common code in multi_server.c.
     */
    if (attr_scan(client_stream,
		  ATTR_FLAG_MORE | ATTR_FLAG_STRICT,
		  ATTR_TYPE_STR, MAIL_ATTR_REQ, request,
		  ATTR_TYPE_END) == 1) {
	if (VSTREQ(request, PROXY_REQ_LOOKUP)) {
	    proxymap_lookup_service(client_stream);
	} else if (VSTREQ(request, PROXY_REQ_OPEN)) {
	    proxymap_open_service(client_stream);
	} else {
	    msg_warn("unrecognized request: \"%s\", ignored", STR(request));
	    attr_print(client_stream, ATTR_FLAG_NONE,
		       ATTR_TYPE_NUM, MAIL_ATTR_STATUS, PROXY_STAT_BAD,
		       ATTR_TYPE_END);
	}
    }
    vstream_fflush(client_stream);
}

/* dict_proxy_open - intercept remote map request from inside library */

DICT   *dict_proxy_open(const char *map, int open_flags, int dict_flags)
{
    if (msg_verbose)
	msg_info("dict_proxy_open(%s, 0%o, 0%o) called from internal routine",
		 map, open_flags, dict_flags);
    while (strncmp(map, PROXY_COLON, PROXY_COLON_LEN) == 0)
	map += PROXY_COLON_LEN;
    return (dict_open(map, open_flags, dict_flags));
}

/* post_jail_init - initialization after privilege drop */

static void post_jail_init(char *unused_name, char **unused_argv)
{
    const char *sep = ", \t\r\n";
    char   *saved_filter;
    char   *bp;
    char   *type_name;

    /*
     * Pre-allocate buffers.
     */
    request = vstring_alloc(10);
    request_map = vstring_alloc(10);
    request_key = vstring_alloc(10);
    map_type_name_flags = vstring_alloc(10);

    /*
     * Prepare the pre-approved list of proxied tables.
     */
    saved_filter = bp = mystrdup(var_proxy_read_maps);
    proxy_read_maps = htable_create(13);
    while ((type_name = mystrtok(&bp, sep)) != 0) {
	if (strncmp(type_name, PROXY_COLON, PROXY_COLON_LEN))
	    continue;
	do {
	    type_name += PROXY_COLON_LEN;
	} while (!strncmp(type_name, PROXY_COLON, PROXY_COLON_LEN));
	if (strchr(type_name, ':') != 0
	    && htable_locate(proxy_read_maps, type_name) == 0)
	    (void) htable_enter(proxy_read_maps, type_name, (char *) 0);
    }
    myfree(saved_filter);
}

/* pre_accept - see if tables have changed */

static void pre_accept(char *unused_name, char **unused_argv)
{
    const char *table;

    if ((table = dict_changed_name()) != 0) {
	msg_info("table %s has changed -- restarting", table);
	exit(0);
    }
}

/* main - pass control to the multi-threaded skeleton */

int     main(int argc, char **argv)
{
    static CONFIG_STR_TABLE str_table[] = {
	VAR_LOCAL_RCPT_MAPS, DEF_LOCAL_RCPT_MAPS, &var_local_rcpt_maps, 0, 0,
	VAR_VIRT_ALIAS_MAPS, DEF_VIRT_ALIAS_MAPS, &var_virt_alias_maps, 0, 0,
	VAR_VIRT_ALIAS_DOMS, DEF_VIRT_ALIAS_DOMS, &var_virt_alias_doms, 0, 0,
	VAR_VIRT_MAILBOX_MAPS, DEF_VIRT_MAILBOX_MAPS, &var_virt_mailbox_maps, 0, 0,
	VAR_VIRT_MAILBOX_DOMS, DEF_VIRT_MAILBOX_DOMS, &var_virt_mailbox_doms, 0, 0,
	VAR_RELAY_RCPT_MAPS, DEF_RELAY_RCPT_MAPS, &var_relay_rcpt_maps, 0, 0,
	VAR_RELAY_DOMAINS, DEF_RELAY_DOMAINS, &var_relay_domains, 0, 0,
	VAR_CANONICAL_MAPS, DEF_CANONICAL_MAPS, &var_canonical_maps, 0, 0,
	VAR_SEND_CANON_MAPS, DEF_SEND_CANON_MAPS, &var_send_canon_maps, 0, 0,
	VAR_RCPT_CANON_MAPS, DEF_RCPT_CANON_MAPS, &var_rcpt_canon_maps, 0, 0,
	VAR_RELOCATED_MAPS, DEF_RELOCATED_MAPS, &var_relocated_maps, 0, 0,
	VAR_TRANSPORT_MAPS, DEF_TRANSPORT_MAPS, &var_transport_maps, 0, 0,
	VAR_PROXY_READ_MAPS, DEF_PROXY_READ_MAPS, &var_proxy_read_maps, 0, 0,
	0,
    };

    multi_server_main(argc, argv, proxymap_service,
		      MAIL_SERVER_STR_TABLE, str_table,
		      MAIL_SERVER_POST_INIT, post_jail_init,
		      MAIL_SERVER_PRE_ACCEPT, pre_accept,
		      0);
}
