/*	$NetBSD: anvil.c,v 1.1.1.2 2006/01/05 02:10:01 rpaulo Exp $	*/

/*++
/* NAME
/*	anvil 8
/* SUMMARY
/*	Postfix session count and request rate control
/* SYNOPSIS
/*	\fBanvil\fR [generic Postfix daemon options]
/* DESCRIPTION
/*	The Postfix \fBanvil\fR(8) server maintains short-term statistics
/*	to defend against clients that hammer a server with either too
/*	many simultaneous sessions, or with too many successive requests
/*	within a configurable time interval.
/*	This server is designed to run under control by the Postfix
/*	\fBmaster\fR(8) server.
/*
/*	The \fBanvil\fR(8) server maintains no persistent database. Standard
/*	library utilities do not meet Postfix performance and robustness
/*	requirements.
/* CONNECTION COUNT/RATE LIMITING
/* .ad
/* .fi
/*	When a remote client connects, a connection count (or rate) limited
/*	server should send the following request to the \fBanvil\fR(8) server:
/* .PP
/* .in +4
/*	\fBrequest=connect\fR
/* .br
/*	\fBident=\fIstring\fR
/* .in
/* .PP
/*	This registers a new connection for the (service, client)
/*	combination specified with \fBident\fR. The \fBanvil\fR(8) server
/*	answers with the number of simultaneous connections and the
/*	number of connections per unit time for that (service, client)
/*	combination:
/* .PP
/* .in +4
/*	\fBstatus=0\fR
/* .br
/*	\fBcount=\fInumber\fR
/* .br
/*	\fBrate=\fInumber\fR
/* .in
/* .PP
/*	The \fBrate\fR is computed as the number of connections
/*	that were registered in the current "time unit" interval.
/*	It is left up to the server to decide if the remote client
/*	exceeds the connection count (or rate) limit.
/* .PP
/*	When a remote client disconnects, a connection count (or rate) limited
/*	server should send the following request to the \fBanvil\fR(8) server:
/* .PP
/* .in +4
/*	\fBrequest=disconnect\fR
/* .br
/*	\fBident=\fIstring\fR
/* .in
/* .PP
/*	This registers a disconnect event for the (service, client)
/*	combination specified with \fBident\fR. The \fBanvil\fR(8)
/*	server replies with:
/* .PP
/* .ti +4
/*	\fBstatus=0\fR
/* MESSAGE RATE LIMITING
/* .ad
/* .fi
/*	When a remote client sends a message delivery request, a
/*	message rate limited server should send the following
/*	request to the \fBanvil\fR(8) server:
/* .PP
/* .in +4
/*	\fBrequest=message\fR
/* .br
/*	\fBident=\fIstring\fR
/* .in
/* .PP
/*	This registers a message delivery request for the (service, client)
/*	combination specified with \fBident\fR. The \fBanvil\fR(8) server
/*	answers with the number of message delivery requests per unit time
/*	for that (service, client) combination:
/* .PP
/* .in +4
/*	\fBstatus=0\fR
/* .br
/*	\fBrate=\fInumber\fR
/* .in
/* .PP
/*	In order to prevent the \fBanvil\fR(8) server from discarding client
/*	request rates too early or too late, a message rate limited
/*	service should also register connect/disconnect events.
/* RECIPIENT RATE LIMITING
/* .ad
/* .fi
/*	When a remote client sends a recipient address, a recipient
/*	rate limited server should send the following request to
/*	the \fBanvil\fR(8) server:
/* .PP
/* .in +4
/*	\fBrequest=recipient\fR
/* .br
/*	\fBident=\fIstring\fR
/* .in
/* .PP
/*	This registers a recipient request for the (service, client)
/*	combination specified with \fBident\fR. The \fBanvil\fR(8) server
/*	answers with the number of recipient addresses per unit time
/*	for that (service, client) combination:
/* .PP
/* .in +4
/*	\fBstatus=0\fR
/* .br
/*	\fBrate=\fInumber\fR
/* .in
/* .PP
/*	In order to prevent the \fBanvil\fR(8) server from discarding client
/*	request rates too early or too late, a recipient rate limited
/*	service should also register connect/disconnect events.
/* SECURITY
/* .ad
/* .fi
/*	The \fBanvil\fR(8) server does not talk to the network or to local
/*	users, and can run chrooted at fixed low privilege.
/*
/*	The \fBanvil\fR(8) server maintains an in-memory table with information
/*	about recent clients of a connection count (or rate) limited service.
/*	Although state is kept only temporarily, this may require a lot of
/*	memory on systems that handle connections from many remote clients.
/*	To reduce memory usage, reduce the time unit over which state
/*	is kept.
/* DIAGNOSTICS
/*	Problems and transactions are logged to \fBsyslogd\fR(8).
/*
/*	Upon exit, and every \fBanvil_status_update_time\fR
/*	seconds, the server logs the maximal count and rate values measured,
/*	together with (service, client) information and the time of day
/*	associated with those events.
/*	In order to avoid unnecessary overhead, no measurements
/*	are done for activity that isn't concurrency limited or
/*	rate limited.
/* BUGS
/*	Systems behind network address translating routers or proxies
/*	appear to have the same client address and can run into connection
/*	count and/or rate limits falsely.
/*
/*	In this preliminary implementation, a count (or rate) limited server
/*	can have only one remote client at a time. If a server reports
/*	multiple simultaneous clients, all but the last reported client
/*	are ignored.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	Changes to \fBmain.cf\fR are picked up automatically as \fBanvil\fR(8)
/*	processes run for only a limited amount of time. Use the command
/*	"\fBpostfix reload\fR" to speed up a change.
/*
/*	The text below provides only a parameter summary. See
/*	\fBpostconf\fR(5) for more details including examples.
/* .IP "\fBanvil_rate_time_unit (60s)\fR"
/*	The time unit over which client connection rates and other rates
/*	are calculated.
/* .IP "\fBanvil_status_update_time (600s)\fR"
/*	How frequently the \fBanvil\fR(8) connection and rate limiting server
/*	logs peak usage information.
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
/* .IP "\fBsyslog_facility (mail)\fR"
/*	The syslog facility of Postfix logging.
/* .IP "\fBsyslog_name (postfix)\fR"
/*	The mail system name that is prepended to the process name in syslog
/*	records, so that "smtpd" becomes, for example, "postfix/smtpd".
/* SEE ALSO
/*	smtpd(8), Postfix SMTP server
/*	postconf(5), configuration parameters
/*	master(5), generic daemon options
/* README FILES
/* .ad
/* .fi
/*	Use "\fBpostconf readme_directory\fR" or
/*	"\fBpostconf html_directory\fR" to locate this information.
/* .na
/* .nf
/*	TUNING_README, performance tuning
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* HISTORY
/* .ad
/* .fi
/*	The anvil service is available in Postfix 2.2 and later.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <sys/time.h>
#include <limits.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <htable.h>
#include <stringops.h>
#include <events.h>

/* Global library. */

#include <mail_conf.h>
#include <mail_params.h>
#include <mail_proto.h>
#include <anvil_clnt.h>

/* Server skeleton. */

#include <mail_server.h>

/* Application-specific. */

int     var_anvil_time_unit;
int     var_anvil_stat_time;

 /*
  * State.
  */
static HTABLE *anvil_remote_map;	/* indexed by service+ remote client */

 /*
  * Absent a real-time query interface, these are logged at process exit time
  * and at regular intervals.
  */
static int max_count;
static char *max_count_user;
static time_t max_count_time;

static int max_rate;
static char *max_rate_user;
static time_t max_rate_time;

static int max_mail;
static char *max_mail_user;
static time_t max_mail_time;

static int max_rcpt;
static char *max_rcpt_user;
static time_t max_rcpt_time;

static int max_cache;
static time_t max_cache_time;

 /*
  * Remote connection state, one instance for each (service, client) pair.
  */
typedef struct {
    char   *ident;			/* lookup key */
    int     count;			/* connection count */
    int     rate;			/* connection rate */
    int     mail;			/* message rate */
    int     rcpt;			/* recipient rate */
    time_t  start;			/* time of first rate sample */
} ANVIL_REMOTE;

 /*
  * Local server state, one per server instance. This allows us to clean up
  * connection state when a local server goes away without cleaning up.
  */
typedef struct {
    ANVIL_REMOTE *anvil_remote;		/* XXX should be list */
} ANVIL_LOCAL;

 /*
  * Silly little macros.
  */
#define STR(x)			vstring_str(x)
#define STREQ(x,y)		(strcmp((x), (y)) == 0)

 /*
  * The following operations are implemented as macros with recognizable
  * names so that we don't lose sight of what the code is trying to do.
  * 
  * Related operations are defined side by side so that the code implementing
  * them isn't pages apart.
  */

/* Create new (service, client) state. */

#define ANVIL_REMOTE_FIRST(remote, id) \
    do { \
	(remote)->ident = mystrdup(id); \
	(remote)->count = 1; \
	(remote)->rate = 1; \
	(remote)->mail = 0; \
	(remote)->rcpt = 0; \
	(remote)->start = event_time(); \
    } while(0)

/* Destroy unused (service, client) state. */

#define ANVIL_REMOTE_FREE(remote) \
    do { \
	myfree((remote)->ident); \
	myfree((char *) (remote)); \
    } while(0)

/* Add connection to (service, client) state. */

#define ANVIL_REMOTE_NEXT(remote) \
    do { \
	time_t _now = event_time(); \
	if ((remote)->start + var_anvil_time_unit < _now) { \
	    (remote)->rate = 1; \
	    (remote)->mail = 0; \
	    (remote)->rcpt = 0; \
	    (remote)->start = _now; \
	} else if ((remote)->rate < INT_MAX) { \
	    (remote)->rate += 1; \
	} \
	if ((remote)->count == 0) \
	    event_cancel_timer(anvil_remote_expire, (char *) remote); \
	(remote)->count++; \
    } while(0)

#define ANVIL_ADD_MAIL(remote) \
    do { \
	time_t _now = event_time(); \
	if ((remote)->start + var_anvil_time_unit < _now) { \
	    (remote)->rate = 0; \
	    (remote)->mail = 1; \
	    (remote)->rcpt = 0; \
	    (remote)->start = _now; \
	} else if ((remote)->mail < INT_MAX) { \
            (remote)->mail += 1; \
	} \
    } while(0)

#define ANVIL_ADD_RCPT(remote) \
    do { \
	time_t _now = event_time(); \
	if ((remote)->start + var_anvil_time_unit < _now) { \
	    (remote)->rate = 0; \
	    (remote)->mail = 0; \
	    (remote)->rcpt = 1; \
	    (remote)->start = _now; \
	} else if ((remote)->rcpt < INT_MAX) { \
            (remote)->rcpt += 1; \
	} \
    } while(0)

/* Drop connection from (service, client) state. */

#define ANVIL_REMOTE_DROP_ONE(remote) \
    do { \
	if ((remote) && (remote)->count > 0) { \
	    if (--(remote)->count == 0) \
		event_request_timer(anvil_remote_expire, (char *) remote, \
			var_anvil_time_unit); \
	} \
    } while(0)

/* Create local server state. */

#define ANVIL_LOCAL_INIT(local) \
    do { \
	(local)->anvil_remote = 0; \
    } while(0)

/* Add connection to local server. */

#define ANVIL_LOCAL_ADD_ONE(local, remote) \
    do { \
	/* XXX allow multiple remote clients per local server. */ \
	if ((local)->anvil_remote) \
	    ANVIL_REMOTE_DROP_ONE((local)->anvil_remote); \
	(local)->anvil_remote = (remote); \
    } while(0)

/* Test if this remote site is listed for this local client. */

#define ANVIL_LOCAL_REMOTE_LINKED(local, remote) \
    ((local)->anvil_remote == (remote))

/* Drop connection from local server. */

#define ANVIL_LOCAL_DROP_ONE(local, remote) \
    do { \
	/* XXX allow multiple remote clients per local server. */ \
	if ((local)->anvil_remote == (remote)) \
	    (local)->anvil_remote = 0; \
    } while(0)

/* Drop all connections from local server. */

#define ANVIL_LOCAL_DROP_ALL(stream, local) \
    do { \
	 /* XXX allow multiple remote clients per local server. */ \
	if ((local)->anvil_remote) \
	    anvil_remote_disconnect((stream), (local)->anvil_remote->ident); \
    } while (0)

/* anvil_remote_expire - purge expired connection state */

static void anvil_remote_expire(int unused_event, char *context)
{
    ANVIL_REMOTE *anvil_remote = (ANVIL_REMOTE *) context;
    char   *myname = "anvil_remote_expire";

    if (msg_verbose)
	msg_info("%s %s", myname, anvil_remote->ident);

    if (anvil_remote->count != 0)
	msg_panic("%s: bad connection count: %d",
		  myname, anvil_remote->count);

    htable_delete(anvil_remote_map, anvil_remote->ident,
		  (void (*) (char *)) 0);
    ANVIL_REMOTE_FREE(anvil_remote);

    if (msg_verbose)
	msg_info("%s: anvil_remote_map used=%d",
		 myname, anvil_remote_map->used);
}

/* anvil_remote_lookup - dump address status */

static void anvil_remote_lookup(VSTREAM *client_stream, const char *ident)
{
    ANVIL_REMOTE *anvil_remote;
    char   *myname = "anvil_remote_lookup";
    HTABLE_INFO **ht_info;
    HTABLE_INFO **ht;

    if (msg_verbose)
	msg_info("%s fd=%d stream=0x%lx ident=%s",
		 myname, vstream_fileno(client_stream),
		 (unsigned long) client_stream, ident);

    /*
     * Look up remote client information.
     */
    if (STREQ(ident, "*")) {
	attr_print_plain(client_stream, ATTR_FLAG_MORE,
			 ATTR_TYPE_NUM, ANVIL_ATTR_STATUS, ANVIL_STAT_OK,
			 ATTR_TYPE_END);
	ht_info = htable_list(anvil_remote_map);
	for (ht = ht_info; *ht; ht++) {
	    anvil_remote = (ANVIL_REMOTE *) ht[0]->value;
	    attr_print_plain(client_stream, ATTR_FLAG_MORE,
			     ATTR_TYPE_STR, ANVIL_ATTR_IDENT, ht[0]->key,
		       ATTR_TYPE_NUM, ANVIL_ATTR_COUNT, anvil_remote->count,
			 ATTR_TYPE_NUM, ANVIL_ATTR_RATE, anvil_remote->rate,
			 ATTR_TYPE_NUM, ANVIL_ATTR_MAIL, anvil_remote->mail,
			 ATTR_TYPE_NUM, ANVIL_ATTR_RCPT, anvil_remote->rcpt,
			     ATTR_TYPE_END);
	}
	attr_print_plain(client_stream, ATTR_FLAG_NONE, ATTR_TYPE_END);
	myfree((char *) ht_info);
    } else if ((anvil_remote =
	      (ANVIL_REMOTE *) htable_find(anvil_remote_map, ident)) == 0) {
	attr_print_plain(client_stream, ATTR_FLAG_NONE,
			 ATTR_TYPE_NUM, ANVIL_ATTR_STATUS, ANVIL_STAT_FAIL,
			 ATTR_TYPE_NUM, ANVIL_ATTR_COUNT, 0,
			 ATTR_TYPE_NUM, ANVIL_ATTR_RATE, 0,
			 ATTR_TYPE_NUM, ANVIL_ATTR_MAIL, 0,
			 ATTR_TYPE_NUM, ANVIL_ATTR_RCPT, 0,
			 ATTR_TYPE_END);
    } else {
	attr_print_plain(client_stream, ATTR_FLAG_NONE,
			 ATTR_TYPE_NUM, ANVIL_ATTR_STATUS, ANVIL_STAT_OK,
		       ATTR_TYPE_NUM, ANVIL_ATTR_COUNT, anvil_remote->count,
			 ATTR_TYPE_NUM, ANVIL_ATTR_RATE, anvil_remote->rate,
			 ATTR_TYPE_NUM, ANVIL_ATTR_MAIL, anvil_remote->mail,
			 ATTR_TYPE_NUM, ANVIL_ATTR_RCPT, anvil_remote->rcpt,
			 ATTR_TYPE_END);
    }
}

/* anvil_remote_conn_update - instantiate or update connection info */

static ANVIL_REMOTE *anvil_remote_conn_update(VSTREAM *client_stream, const char *ident)
{
    ANVIL_REMOTE *anvil_remote;
    ANVIL_LOCAL *anvil_local;
    char   *myname = "anvil_remote_conn_update";

    if (msg_verbose)
	msg_info("%s fd=%d stream=0x%lx ident=%s",
		 myname, vstream_fileno(client_stream),
		 (unsigned long) client_stream, ident);

    /*
     * Look up remote connection count information. Update remote connection
     * rate information. Simply reset the counter every var_anvil_time_unit
     * seconds. This is easier than maintaining a moving average and it gives
     * a quicker response to tresspassers.
     */
    if ((anvil_remote =
	 (ANVIL_REMOTE *) htable_find(anvil_remote_map, ident)) == 0) {
	anvil_remote = (ANVIL_REMOTE *) mymalloc(sizeof(*anvil_remote));
	ANVIL_REMOTE_FIRST(anvil_remote, ident);
	htable_enter(anvil_remote_map, ident, (char *) anvil_remote);
	if (max_cache < anvil_remote_map->used) {
	    max_cache = anvil_remote_map->used;
	    max_cache_time = event_time();
	}
    } else {
	ANVIL_REMOTE_NEXT(anvil_remote);
    }

    /*
     * Record this connection under the local client information, so that we
     * can clean up all its connection state when the local client goes away.
     */
    if ((anvil_local = (ANVIL_LOCAL *) vstream_context(client_stream)) == 0) {
	anvil_local = (ANVIL_LOCAL *) mymalloc(sizeof(*anvil_local));
	ANVIL_LOCAL_INIT(anvil_local);
	vstream_control(client_stream,
			VSTREAM_CTL_CONTEXT, (void *) anvil_local,
			VSTREAM_CTL_END);
    }
    ANVIL_LOCAL_ADD_ONE(anvil_local, anvil_remote);
    if (msg_verbose)
	msg_info("%s: anvil_local 0x%lx",
		 myname, (unsigned long) anvil_local);

    return (anvil_remote);
}

/* anvil_remote_connect - report connection event, query address status */

static void anvil_remote_connect(VSTREAM *client_stream, const char *ident)
{
    ANVIL_REMOTE *anvil_remote;

    /*
     * Update or instantiate connection info.
     */
    anvil_remote = anvil_remote_conn_update(client_stream, ident);

    /*
     * Respond to the local client.
     */
    attr_print_plain(client_stream, ATTR_FLAG_NONE,
		     ATTR_TYPE_NUM, ANVIL_ATTR_STATUS, ANVIL_STAT_OK,
		     ATTR_TYPE_NUM, ANVIL_ATTR_COUNT, anvil_remote->count,
		     ATTR_TYPE_NUM, ANVIL_ATTR_RATE, anvil_remote->rate,
		     ATTR_TYPE_END);

    /*
     * Update local statistics.
     */
    if (anvil_remote->rate > max_rate) {
	max_rate = anvil_remote->rate;
	if (max_rate_user == 0) {
	    max_rate_user = mystrdup(anvil_remote->ident);
	} else if (!STREQ(max_rate_user, anvil_remote->ident)) {
	    myfree(max_rate_user);
	    max_rate_user = mystrdup(anvil_remote->ident);
	}
	max_rate_time = event_time();
    }
    if (anvil_remote->count > max_count) {
	max_count = anvil_remote->count;
	if (max_count_user == 0) {
	    max_count_user = mystrdup(anvil_remote->ident);
	} else if (!STREQ(max_count_user, anvil_remote->ident)) {
	    myfree(max_count_user);
	    max_count_user = mystrdup(anvil_remote->ident);
	}
	max_count_time = event_time();
    }
}

/* anvil_remote_mail - register message delivery request */

static void anvil_remote_mail(VSTREAM *client_stream, const char *ident)
{
    ANVIL_REMOTE *anvil_remote;

    /*
     * Be prepared for "postfix reload" after "connect".
     */
    if ((anvil_remote =
	 (ANVIL_REMOTE *) htable_find(anvil_remote_map, ident)) == 0)
	anvil_remote = anvil_remote_conn_update(client_stream, ident);

    /*
     * Update message delivery request rate and respond to local client.
     */
    ANVIL_ADD_MAIL(anvil_remote);
    attr_print_plain(client_stream, ATTR_FLAG_NONE,
		     ATTR_TYPE_NUM, ANVIL_ATTR_STATUS, ANVIL_STAT_OK,
		     ATTR_TYPE_NUM, ANVIL_ATTR_RATE, anvil_remote->mail,
		     ATTR_TYPE_END);

    /*
     * Update local statistics.
     */
    if (anvil_remote->mail > max_mail) {
	max_mail = anvil_remote->mail;
	if (max_mail_user == 0) {
	    max_mail_user = mystrdup(anvil_remote->ident);
	} else if (!STREQ(max_mail_user, anvil_remote->ident)) {
	    myfree(max_mail_user);
	    max_mail_user = mystrdup(anvil_remote->ident);
	}
	max_mail_time = event_time();
    }
}

/* anvil_remote_rcpt - register recipient address event */

static void anvil_remote_rcpt(VSTREAM *client_stream, const char *ident)
{
    ANVIL_REMOTE *anvil_remote;

    /*
     * Be prepared for "postfix reload" after "connect".
     */
    if ((anvil_remote =
	 (ANVIL_REMOTE *) htable_find(anvil_remote_map, ident)) == 0)
	anvil_remote = anvil_remote_conn_update(client_stream, ident);

    /*
     * Update recipient address rate and respond to local client.
     */
    ANVIL_ADD_RCPT(anvil_remote);
    attr_print_plain(client_stream, ATTR_FLAG_NONE,
		     ATTR_TYPE_NUM, ANVIL_ATTR_STATUS, ANVIL_STAT_OK,
		     ATTR_TYPE_NUM, ANVIL_ATTR_RATE, anvil_remote->rcpt,
		     ATTR_TYPE_END);

    /*
     * Update local statistics.
     */
    if (anvil_remote->rcpt > max_rcpt) {
	max_rcpt = anvil_remote->rcpt;
	if (max_rcpt_user == 0) {
	    max_rcpt_user = mystrdup(anvil_remote->ident);
	} else if (!STREQ(max_rcpt_user, anvil_remote->ident)) {
	    myfree(max_rcpt_user);
	    max_rcpt_user = mystrdup(anvil_remote->ident);
	}
	max_rcpt_time = event_time();
    }
}

/* anvil_remote_disconnect - report disconnect event */

static void anvil_remote_disconnect(VSTREAM *client_stream, const char *ident)
{
    ANVIL_REMOTE *anvil_remote;
    ANVIL_LOCAL *anvil_local;
    char   *myname = "anvil_remote_disconnect";

    if (msg_verbose)
	msg_info("%s fd=%d stream=0x%lx ident=%s",
		 myname, vstream_fileno(client_stream),
		 (unsigned long) client_stream, ident);

    /*
     * Update local and remote info if this remote site is listed for this
     * local client.
     */
    if ((anvil_local = (ANVIL_LOCAL *) vstream_context(client_stream)) != 0
	&& (anvil_remote =
	    (ANVIL_REMOTE *) htable_find(anvil_remote_map, ident)) != 0
	&& ANVIL_LOCAL_REMOTE_LINKED(anvil_local, anvil_remote)) {
	ANVIL_REMOTE_DROP_ONE(anvil_remote);
	ANVIL_LOCAL_DROP_ONE(anvil_local, anvil_remote);
    }
    if (msg_verbose)
	msg_info("%s: anvil_local 0x%lx",
		 myname, (unsigned long) anvil_local);

    /*
     * Respond to the local client.
     */
    attr_print_plain(client_stream, ATTR_FLAG_NONE,
		     ATTR_TYPE_NUM, ANVIL_ATTR_STATUS, ANVIL_STAT_OK,
		     ATTR_TYPE_END);
}

/* anvil_service_done - clean up */

static void anvil_service_done(VSTREAM *client_stream, char *unused_service,
			               char **unused_argv)
{
    ANVIL_LOCAL *anvil_local;
    char   *myname = "anvil_service_done";

    if (msg_verbose)
	msg_info("%s fd=%d stream=0x%lx",
		 myname, vstream_fileno(client_stream),
		 (unsigned long) client_stream);

    /*
     * Look up the local client, and get rid of open remote connection state
     * that we still have for this local client. Do not destroy remote client
     * status information before it expires.
     */
    if ((anvil_local = (ANVIL_LOCAL *) vstream_context(client_stream)) != 0) {
	if (msg_verbose)
	    msg_info("%s: anvil_local 0x%lx",
		     myname, (unsigned long) anvil_local);
	ANVIL_LOCAL_DROP_ALL(client_stream, anvil_local);
	myfree((char *) anvil_local);
    } else if (msg_verbose)
	msg_info("client socket not found for fd=%d",
		 vstream_fileno(client_stream));
}

/* anvil_service - perform service for client */

static void anvil_service(VSTREAM *client_stream, char *unused_service, char **argv)
{
    VSTRING *request = vstring_alloc(10);
    VSTRING *ident = vstring_alloc(10);

    /*
     * Sanity check. This service takes no command-line arguments.
     */
    if (argv[0])
	msg_fatal("unexpected command-line argument: %s", argv[0]);

    /*
     * This routine runs whenever a client connects to the socket dedicated
     * to the client connection rate management service. All
     * connection-management stuff is handled by the common code in
     * multi_server.c.
     */
    if (msg_verbose)
	msg_info("--- start request ---");
    if (attr_scan_plain(client_stream,
			ATTR_FLAG_MISSING | ATTR_FLAG_STRICT,
			ATTR_TYPE_STR, ANVIL_ATTR_REQ, request,
			ATTR_TYPE_STR, ANVIL_ATTR_IDENT, ident,
			ATTR_TYPE_END) == 2) {
	if (STREQ(STR(request), ANVIL_REQ_CONN)) {
	    anvil_remote_connect(client_stream, STR(ident));
	} else if (STREQ(STR(request), ANVIL_REQ_MAIL)) {
	    anvil_remote_mail(client_stream, STR(ident));
	} else if (STREQ(STR(request), ANVIL_REQ_RCPT)) {
	    anvil_remote_rcpt(client_stream, STR(ident));
	} else if (STREQ(STR(request), ANVIL_REQ_DISC)) {
	    anvil_remote_disconnect(client_stream, STR(ident));
	} else if (STREQ(STR(request), ANVIL_REQ_LOOKUP)) {
	    anvil_remote_lookup(client_stream, STR(ident));
	} else {
	    msg_warn("unrecognized request: \"%s\", ignored", STR(request));
	    attr_print_plain(client_stream, ATTR_FLAG_NONE,
			  ATTR_TYPE_NUM, ANVIL_ATTR_STATUS, ANVIL_STAT_FAIL,
			     ATTR_TYPE_END);
	}
	vstream_fflush(client_stream);
    } else {
	/* Note: invokes anvil_service_done() */
	multi_server_disconnect(client_stream);
    }
    if (msg_verbose)
	msg_info("--- end request ---");
    vstring_free(ident);
    vstring_free(request);
}

/* post_jail_init - post-jail initialization */

static void anvil_status_update(int, char *);

static void post_jail_init(char *unused_name, char **unused_argv)
{

    /*
     * Dump and reset extreme usage every so often.
     */
    event_request_timer(anvil_status_update, (char *) 0, var_anvil_stat_time);

    /*
     * Initial client state tables.
     */
    anvil_remote_map = htable_create(1000);

    /*
     * Do not limit the number of client requests.
     */
    var_use_limit = 0;

    /*
     * Don't exit before the sampling interval ends.
     */
    if (var_idle_limit < var_anvil_time_unit)
	var_idle_limit = var_anvil_time_unit;
}

/* anvil_status_dump - log and reset extreme usage */

static void anvil_status_dump(char *unused_name, char **unused_argv)
{
    if (max_rate > 0) {
	msg_info("statistics: max connection rate %d/%ds for (%s) at %.15s",
		 max_rate, var_anvil_time_unit,
		 max_rate_user, ctime(&max_rate_time) + 4);
	max_rate = 0;
    }
    if (max_count > 0) {
	msg_info("statistics: max connection count %d for (%s) at %.15s",
		 max_count, max_count_user, ctime(&max_count_time) + 4);
	max_count = 0;
    }
    if (max_mail > 0) {
	msg_info("statistics: max message rate %d/%ds for (%s) at %.15s",
		 max_mail, var_anvil_time_unit,
		 max_mail_user, ctime(&max_mail_time) + 4);
	max_mail = 0;
    }
    if (max_rcpt > 0) {
	msg_info("statistics: max recipient rate %d/%ds for (%s) at %.15s",
		 max_rcpt, var_anvil_time_unit,
		 max_rcpt_user, ctime(&max_rcpt_time) + 4);
	max_rcpt = 0;
    }
    if (max_cache > 0) {
	msg_info("statistics: max cache size %d at %.15s",
		 max_cache, ctime(&max_cache_time) + 4);
	max_cache = 0;
    }
}

/* anvil_status_update - log and reset extreme usage periodically */

static void anvil_status_update(int unused_event, char *context)
{
    anvil_status_dump((char *) 0, (char **) 0);
    event_request_timer(anvil_status_update, context, var_anvil_stat_time);
}

/* main - pass control to the multi-threaded skeleton */

int     main(int argc, char **argv)
{
    static CONFIG_TIME_TABLE time_table[] = {
	VAR_ANVIL_TIME_UNIT, DEF_ANVIL_TIME_UNIT, &var_anvil_time_unit, 1, 0,
	VAR_ANVIL_STAT_TIME, DEF_ANVIL_STAT_TIME, &var_anvil_stat_time, 1, 0,
	0,
    };

    multi_server_main(argc, argv, anvil_service,
		      MAIL_SERVER_TIME_TABLE, time_table,
		      MAIL_SERVER_POST_INIT, post_jail_init,
		      MAIL_SERVER_SOLITARY,
		      MAIL_SERVER_PRE_DISCONN, anvil_service_done,
		      MAIL_SERVER_EXIT, anvil_status_dump,
		      0);
}
