/*++
/* NAME
/*	anvil 8
/* SUMMARY
/*	Postfix session count and request rate control
/* SYNOPSIS
/*	\fBanvil\fR [generic Postfix daemon options]
/* DESCRIPTION
/*	The Postfix \fBanvil\fR(8) server maintains statistics about
/*	client connection counts or client request rates. This
/*	information can be used to defend against clients that
/*	hammer a server with either too many simultaneous sessions,
/*	or with too many successive requests within a configurable
/*	time interval.  This server is designed to run under control
/*	by the Postfix \fBmaster\fR(8) server.
/*
/*	In the following text, \fBident\fR specifies a (service,
/*	client) combination. The exact syntax of that information
/*	is application-dependent; the \fBanvil\fR(8) server does
/*	not care.
/* CONNECTION COUNT/RATE CONTROL
/* .ad
/* .fi
/*	To register a new connection send the following request to
/*	the \fBanvil\fR(8) server:
/*
/* .nf
/*	    \fBrequest=connect\fR
/*	    \fBident=\fIstring\fR
/* .fi
/*
/*	The \fBanvil\fR(8) server answers with the number of
/*	simultaneous connections and the number of connections per
/*	unit time for the (service, client) combination specified
/*	with \fBident\fR:
/*
/* .nf
/*	    \fBstatus=0\fR
/*	    \fBcount=\fInumber\fR
/*	    \fBrate=\fInumber\fR
/* .fi
/*
/*	To register a disconnect event send the following request
/*	to the \fBanvil\fR(8) server:
/*
/* .nf
/*	    \fBrequest=disconnect\fR
/*	    \fBident=\fIstring\fR
/* .fi
/*
/*	The \fBanvil\fR(8) server replies with:
/*
/* .nf
/*	    \fBstatus=0\fR
/* .fi
/* MESSAGE RATE CONTROL
/* .ad
/* .fi
/*	To register a message delivery request send the following
/*	request to the \fBanvil\fR(8) server:
/*
/* .nf
/*	    \fBrequest=message\fR
/*	    \fBident=\fIstring\fR
/* .fi
/*
/*	The \fBanvil\fR(8) server answers with the number of message
/*	delivery requests per unit time for the (service, client)
/*	combination specified with \fBident\fR:
/*
/* .nf
/*	    \fBstatus=0\fR
/*	    \fBrate=\fInumber\fR
/* .fi
/* RECIPIENT RATE CONTROL
/* .ad
/* .fi
/*	To register a recipient request send the following request
/*	to the \fBanvil\fR(8) server:
/*
/* .nf
/*	    \fBrequest=recipient\fR
/*	    \fBident=\fIstring\fR
/* .fi
/*
/*	The \fBanvil\fR(8) server answers with the number of recipient
/*	addresses per unit time for the (service, client) combination
/*	specified with \fBident\fR:
/*
/* .nf
/*	    \fBstatus=0\fR
/*	    \fBrate=\fInumber\fR
/* .fi
/* TLS SESSION NEGOTIATION RATE CONTROL
/* .ad
/* .fi
/*	The features described in this section are available with
/*	Postfix 2.3 and later.
/*
/*	To register a request for a new (i.e. not cached) TLS session
/*	send the following request to the \fBanvil\fR(8) server:
/*
/* .nf
/*	    \fBrequest=newtls\fR
/*	    \fBident=\fIstring\fR
/* .fi
/*
/*	The \fBanvil\fR(8) server answers with the number of new
/*	TLS session requests per unit time for the (service, client)
/*	combination specified with \fBident\fR:
/*
/* .nf
/*	    \fBstatus=0\fR
/*	    \fBrate=\fInumber\fR
/* .fi
/*
/*	To retrieve new TLS session request rate information without
/*	updating the counter information, send:
/*
/* .nf
/*	    \fBrequest=newtls_report\fR
/*	    \fBident=\fIstring\fR
/* .fi
/*
/*	The \fBanvil\fR(8) server answers with the number of new
/*	TLS session requests per unit time for the (service, client)
/*	combination specified with \fBident\fR:
/*
/* .nf
/*	    \fBstatus=0\fR
/*	    \fBrate=\fInumber\fR
/* .fi
/* SECURITY
/* .ad
/* .fi
/*	The \fBanvil\fR(8) server does not talk to the network or to local
/*	users, and can run chrooted at fixed low privilege.
/*
/*	The \fBanvil\fR(8) server maintains an in-memory table with
/*	information about recent clients requests.  No persistent
/*	state is kept because standard system library routines are
/*	not sufficiently robust for update-intensive applications.
/*
/*	Although the in-memory state is kept only temporarily, this
/*	may require a lot of memory on systems that handle connections
/*	from many remote clients.  To reduce memory usage, reduce
/*	the time unit over which state is kept.
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
/*	multiple simultaneous clients, state is kept only for the last
/*	reported client.
/*
/*	The \fBanvil\fR(8) server automatically discards client
/*	request information after it expires.  To prevent the
/*	\fBanvil\fR(8) server from discarding client request rate
/*	information too early or too late, a rate limited service
/*	should always register connect/disconnect events even when
/*	it does not explicitly limit them.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	On low-traffic mail systems, changes to \fBmain.cf\fR are
/*	picked up automatically as \fBanvil\fR(8) processes run for
/*	only a limited amount of time. On other mail systems, use
/*	the command "\fBpostfix reload\fR" to speed up a change.
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
/*	The maximum amount of time that an idle Postfix daemon process waits
/*	for an incoming connection before terminating voluntarily.
/* .IP "\fBmax_use (100)\fR"
/*	The maximal number of incoming connections that a Postfix daemon
/*	process will service before terminating voluntarily.
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
#include <mail_version.h>
#include <mail_proto.h>
#include <anvil_clnt.h>

/* Server skeleton. */

#include <mail_server.h>

/* Application-specific. */

 /*
  * Configuration parameters.
  */
int     var_anvil_time_unit;
int     var_anvil_stat_time;

 /*
  * Global dynamic state.
  */
static HTABLE *anvil_remote_map;	/* indexed by service+ remote client */

 /*
  * Remote connection state, one instance for each (service, client) pair.
  */
typedef struct {
    char   *ident;			/* lookup key */
    int     count;			/* connection count */
    int     rate;			/* connection rate */
    int     mail;			/* message rate */
    int     rcpt;			/* recipient rate */
    int     ntls;			/* new TLS session rate */
    time_t  start;			/* time of first rate sample */
} ANVIL_REMOTE;

 /*
  * Local server state, one instance per anvil client connection. This allows
  * us to clean up remote connection state when a local server goes away
  * without cleaning up.
  */
typedef struct {
    ANVIL_REMOTE *anvil_remote;		/* XXX should be list */
} ANVIL_LOCAL;

 /*
  * The following operations are implemented as macros with recognizable
  * names so that we don't lose sight of what the code is trying to do.
  * 
  * Related operations are defined side by side so that the code implementing
  * them isn't pages apart.
  */

/* Create new (service, client) state. */

#define ANVIL_REMOTE_FIRST_CONN(remote, id) \
    do { \
	(remote)->ident = mystrdup(id); \
	(remote)->count = 1; \
	(remote)->rate = 1; \
	(remote)->mail = 0; \
	(remote)->rcpt = 0; \
	(remote)->ntls = 0; \
	(remote)->start = event_time(); \
    } while(0)

/* Destroy unused (service, client) state. */

#define ANVIL_REMOTE_FREE(remote) \
    do { \
	myfree((remote)->ident); \
	myfree((char *) (remote)); \
    } while(0)

/* Reset or update rate information for existing (service, client) state. */

#define ANVIL_REMOTE_RSET_RATE(remote, _start) \
    do { \
	(remote)->rate = 0; \
	(remote)->mail = 0; \
	(remote)->rcpt = 0; \
	(remote)->ntls = 0; \
	(remote)->start = _start; \
    } while(0)

#define ANVIL_REMOTE_INCR_RATE(remote, _what) \
    do { \
	time_t _now = event_time(); \
	if ((remote)->start + var_anvil_time_unit < _now) \
	    ANVIL_REMOTE_RSET_RATE((remote), _now); \
	if ((remote)->_what < INT_MAX) \
            (remote)->_what += 1; \
    } while(0)

/* Update existing (service, client) state. */

#define ANVIL_REMOTE_NEXT_CONN(remote) \
    do { \
	ANVIL_REMOTE_INCR_RATE((remote), rate); \
	if ((remote)->count == 0) \
	    event_cancel_timer(anvil_remote_expire, (char *) remote); \
	(remote)->count++; \
    } while(0)

#define ANVIL_REMOTE_INCR_MAIL(remote)	ANVIL_REMOTE_INCR_RATE((remote), mail)

#define ANVIL_REMOTE_INCR_RCPT(remote)	ANVIL_REMOTE_INCR_RATE((remote), rcpt)

#define ANVIL_REMOTE_INCR_NTLS(remote)	ANVIL_REMOTE_INCR_RATE((remote), ntls)

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

/* Add remote connection to local server. */

#define ANVIL_LOCAL_ADD_ONE(local, remote) \
    do { \
	/* XXX allow multiple remote clients per local server. */ \
	if ((local)->anvil_remote) \
	    ANVIL_REMOTE_DROP_ONE((local)->anvil_remote); \
	(local)->anvil_remote = (remote); \
    } while(0)

/* Test if this remote connection is listed for this local server. */

#define ANVIL_LOCAL_REMOTE_LINKED(local, remote) \
    ((local)->anvil_remote == (remote))

/* Drop specific remote connection from local server. */

#define ANVIL_LOCAL_DROP_ONE(local, remote) \
    do { \
	/* XXX allow multiple remote clients per local server. */ \
	if ((local)->anvil_remote == (remote)) \
	    (local)->anvil_remote = 0; \
    } while(0)

/* Drop all remote connections from local server. */

#define ANVIL_LOCAL_DROP_ALL(stream, local) \
    do { \
	 /* XXX allow multiple remote clients per local server. */ \
	if ((local)->anvil_remote) \
	    anvil_remote_disconnect((stream), (local)->anvil_remote->ident); \
    } while (0)

 /*
  * Lookup table to map request names to action routines.
  */
typedef struct {
    const char *name;
    void    (*action) (VSTREAM *, const char *);
} ANVIL_REQ_TABLE;

 /*
  * Run-time statistics for maximal connection counts and event rates. These
  * store the peak resource usage, remote connection, and time. Absent a
  * query interface, this information is logged at process exit time and at
  * configurable intervals.
  */
typedef struct {
    int     value;			/* peak value */
    char   *ident;			/* lookup key */
    time_t  when;			/* time of peak value */
} ANVIL_MAX;

static ANVIL_MAX max_conn_count;	/* peak connection count */
static ANVIL_MAX max_conn_rate;		/* peak connection rate */
static ANVIL_MAX max_mail_rate;		/* peak message rate */
static ANVIL_MAX max_rcpt_rate;		/* peak recipient rate */
static ANVIL_MAX max_ntls_rate;		/* peak new TLS session rate */

static int max_cache_size;		/* peak cache size */
static time_t max_cache_time;		/* time of peak size */

/* Update/report peak usage. */

#define ANVIL_MAX_UPDATE(_max, _value, _ident) \
    do { \
	_max.value = _value; \
	if (_max.ident == 0) { \
	    _max.ident = mystrdup(_ident); \
	} else if (!STREQ(_max.ident, _ident)) { \
	    myfree(_max.ident); \
	    _max.ident = mystrdup(_ident); \
	} \
	_max.when = event_time(); \
    } while (0)

#define ANVIL_MAX_RATE_REPORT(_max, _name) \
    do { \
	if (_max.value > 0) { \
	    msg_info("statistics: max " _name " rate %d/%ds for (%s) at %.15s", \
		_max.value, var_anvil_time_unit, \
		_max.ident, ctime(&_max.when) + 4); \
	    _max.value = 0; \
	} \
    } while (0);

#define ANVIL_MAX_COUNT_REPORT(_max, _name) \
    do { \
	if (_max.value > 0) { \
	    msg_info("statistics: max " _name " count %d for (%s) at %.15s", \
		_max.value, _max.ident, ctime(&_max.when) + 4); \
	    _max.value = 0; \
	} \
    } while (0);

 /*
  * Silly little macros.
  */
#define STR(x)			vstring_str(x)
#define STREQ(x,y)		(strcmp((x), (y)) == 0)

/* anvil_remote_expire - purge expired connection state */

static void anvil_remote_expire(int unused_event, char *context)
{
    ANVIL_REMOTE *anvil_remote = (ANVIL_REMOTE *) context;
    const char *myname = "anvil_remote_expire";

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
    const char *myname = "anvil_remote_lookup";

    if (msg_verbose)
	msg_info("%s fd=%d stream=0x%lx ident=%s",
		 myname, vstream_fileno(client_stream),
		 (unsigned long) client_stream, ident);

    /*
     * Look up remote client information.
     */
    if ((anvil_remote =
	 (ANVIL_REMOTE *) htable_find(anvil_remote_map, ident)) == 0) {
	attr_print_plain(client_stream, ATTR_FLAG_NONE,
			 ATTR_TYPE_INT, ANVIL_ATTR_STATUS, ANVIL_STAT_OK,
			 ATTR_TYPE_INT, ANVIL_ATTR_COUNT, 0,
			 ATTR_TYPE_INT, ANVIL_ATTR_RATE, 0,
			 ATTR_TYPE_INT, ANVIL_ATTR_MAIL, 0,
			 ATTR_TYPE_INT, ANVIL_ATTR_RCPT, 0,
			 ATTR_TYPE_INT, ANVIL_ATTR_NTLS, 0,
			 ATTR_TYPE_END);
    } else {

	/*
	 * Do not report stale information.
	 */
	if (anvil_remote->start != 0
	    && anvil_remote->start + var_anvil_time_unit < event_time())
	    ANVIL_REMOTE_RSET_RATE(anvil_remote, 0);
	attr_print_plain(client_stream, ATTR_FLAG_NONE,
			 ATTR_TYPE_INT, ANVIL_ATTR_STATUS, ANVIL_STAT_OK,
		       ATTR_TYPE_INT, ANVIL_ATTR_COUNT, anvil_remote->count,
			 ATTR_TYPE_INT, ANVIL_ATTR_RATE, anvil_remote->rate,
			 ATTR_TYPE_INT, ANVIL_ATTR_MAIL, anvil_remote->mail,
			 ATTR_TYPE_INT, ANVIL_ATTR_RCPT, anvil_remote->rcpt,
			 ATTR_TYPE_INT, ANVIL_ATTR_NTLS, anvil_remote->ntls,
			 ATTR_TYPE_END);
    }
}

/* anvil_remote_conn_update - instantiate or update connection info */

static ANVIL_REMOTE *anvil_remote_conn_update(VSTREAM *client_stream, const char *ident)
{
    ANVIL_REMOTE *anvil_remote;
    ANVIL_LOCAL *anvil_local;
    const char *myname = "anvil_remote_conn_update";

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
	ANVIL_REMOTE_FIRST_CONN(anvil_remote, ident);
	htable_enter(anvil_remote_map, ident, (char *) anvil_remote);
	if (max_cache_size < anvil_remote_map->used) {
	    max_cache_size = anvil_remote_map->used;
	    max_cache_time = event_time();
	}
    } else {
	ANVIL_REMOTE_NEXT_CONN(anvil_remote);
    }

    /*
     * Record this connection under the local server information, so that we
     * can clean up all its connection state when the local server goes away.
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
     * Respond to the local server.
     */
    attr_print_plain(client_stream, ATTR_FLAG_NONE,
		     ATTR_TYPE_INT, ANVIL_ATTR_STATUS, ANVIL_STAT_OK,
		     ATTR_TYPE_INT, ANVIL_ATTR_COUNT, anvil_remote->count,
		     ATTR_TYPE_INT, ANVIL_ATTR_RATE, anvil_remote->rate,
		     ATTR_TYPE_END);

    /*
     * Update peak statistics.
     */
    if (anvil_remote->rate > max_conn_rate.value)
	ANVIL_MAX_UPDATE(max_conn_rate, anvil_remote->rate, anvil_remote->ident);
    if (anvil_remote->count > max_conn_count.value)
	ANVIL_MAX_UPDATE(max_conn_count, anvil_remote->count, anvil_remote->ident);
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
     * Update message delivery request rate and respond to local server.
     */
    ANVIL_REMOTE_INCR_MAIL(anvil_remote);
    attr_print_plain(client_stream, ATTR_FLAG_NONE,
		     ATTR_TYPE_INT, ANVIL_ATTR_STATUS, ANVIL_STAT_OK,
		     ATTR_TYPE_INT, ANVIL_ATTR_RATE, anvil_remote->mail,
		     ATTR_TYPE_END);

    /*
     * Update peak statistics.
     */
    if (anvil_remote->mail > max_mail_rate.value)
	ANVIL_MAX_UPDATE(max_mail_rate, anvil_remote->mail, anvil_remote->ident);
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
     * Update recipient address rate and respond to local server.
     */
    ANVIL_REMOTE_INCR_RCPT(anvil_remote);
    attr_print_plain(client_stream, ATTR_FLAG_NONE,
		     ATTR_TYPE_INT, ANVIL_ATTR_STATUS, ANVIL_STAT_OK,
		     ATTR_TYPE_INT, ANVIL_ATTR_RATE, anvil_remote->rcpt,
		     ATTR_TYPE_END);

    /*
     * Update peak statistics.
     */
    if (anvil_remote->rcpt > max_rcpt_rate.value)
	ANVIL_MAX_UPDATE(max_rcpt_rate, anvil_remote->rcpt, anvil_remote->ident);
}

/* anvil_remote_newtls - register newtls event */

static void anvil_remote_newtls(VSTREAM *client_stream, const char *ident)
{
    ANVIL_REMOTE *anvil_remote;

    /*
     * Be prepared for "postfix reload" after "connect".
     */
    if ((anvil_remote =
	 (ANVIL_REMOTE *) htable_find(anvil_remote_map, ident)) == 0)
	anvil_remote = anvil_remote_conn_update(client_stream, ident);

    /*
     * Update newtls rate and respond to local server.
     */
    ANVIL_REMOTE_INCR_NTLS(anvil_remote);
    attr_print_plain(client_stream, ATTR_FLAG_NONE,
		     ATTR_TYPE_INT, ANVIL_ATTR_STATUS, ANVIL_STAT_OK,
		     ATTR_TYPE_INT, ANVIL_ATTR_RATE, anvil_remote->ntls,
		     ATTR_TYPE_END);

    /*
     * Update peak statistics.
     */
    if (anvil_remote->ntls > max_ntls_rate.value)
	ANVIL_MAX_UPDATE(max_ntls_rate, anvil_remote->ntls, anvil_remote->ident);
}

/* anvil_remote_newtls_stat - report newtls stats */

static void anvil_remote_newtls_stat(VSTREAM *client_stream, const char *ident)
{
    ANVIL_REMOTE *anvil_remote;
    int     rate;

    /*
     * Be prepared for "postfix reload" after "connect".
     */
    if ((anvil_remote =
	 (ANVIL_REMOTE *) htable_find(anvil_remote_map, ident)) == 0) {
	rate = 0;
    }

    /*
     * Do not report stale information.
     */
    else {
	if (anvil_remote->start != 0
	    && anvil_remote->start + var_anvil_time_unit < event_time())
	    ANVIL_REMOTE_RSET_RATE(anvil_remote, 0);
	rate = anvil_remote->ntls;
    }

    /*
     * Respond to local server.
     */
    attr_print_plain(client_stream, ATTR_FLAG_NONE,
		     ATTR_TYPE_INT, ANVIL_ATTR_STATUS, ANVIL_STAT_OK,
		     ATTR_TYPE_INT, ANVIL_ATTR_RATE, rate,
		     ATTR_TYPE_END);
}

/* anvil_remote_disconnect - report disconnect event */

static void anvil_remote_disconnect(VSTREAM *client_stream, const char *ident)
{
    ANVIL_REMOTE *anvil_remote;
    ANVIL_LOCAL *anvil_local;
    const char *myname = "anvil_remote_disconnect";

    if (msg_verbose)
	msg_info("%s fd=%d stream=0x%lx ident=%s",
		 myname, vstream_fileno(client_stream),
		 (unsigned long) client_stream, ident);

    /*
     * Update local and remote info if this remote connection is listed for
     * this local server.
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
     * Respond to the local server.
     */
    attr_print_plain(client_stream, ATTR_FLAG_NONE,
		     ATTR_TYPE_INT, ANVIL_ATTR_STATUS, ANVIL_STAT_OK,
		     ATTR_TYPE_END);
}

/* anvil_service_done - clean up */

static void anvil_service_done(VSTREAM *client_stream, char *unused_service,
			               char **unused_argv)
{
    ANVIL_LOCAL *anvil_local;
    const char *myname = "anvil_service_done";

    if (msg_verbose)
	msg_info("%s fd=%d stream=0x%lx",
		 myname, vstream_fileno(client_stream),
		 (unsigned long) client_stream);

    /*
     * Look up the local server, and get rid of any remote connection state
     * that we still have for this local server. Do not destroy remote client
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

/* anvil_status_dump - log and reset extreme usage */

static void anvil_status_dump(char *unused_name, char **unused_argv)
{
    ANVIL_MAX_RATE_REPORT(max_conn_rate, "connection");
    ANVIL_MAX_COUNT_REPORT(max_conn_count, "connection");
    ANVIL_MAX_RATE_REPORT(max_mail_rate, "message");
    ANVIL_MAX_RATE_REPORT(max_rcpt_rate, "recipient");
    ANVIL_MAX_RATE_REPORT(max_ntls_rate, "newtls");

    if (max_cache_size > 0) {
	msg_info("statistics: max cache size %d at %.15s",
		 max_cache_size, ctime(&max_cache_time) + 4);
	max_cache_size = 0;
    }
}

/* anvil_status_update - log and reset extreme usage periodically */

static void anvil_status_update(int unused_event, char *context)
{
    anvil_status_dump((char *) 0, (char **) 0);
    event_request_timer(anvil_status_update, context, var_anvil_stat_time);
}

/* anvil_service - perform service for client */

static void anvil_service(VSTREAM *client_stream, char *unused_service, char **argv)
{
    static VSTRING *request;
    static VSTRING *ident;
    static const ANVIL_REQ_TABLE request_table[] = {
	ANVIL_REQ_CONN, anvil_remote_connect,
	ANVIL_REQ_MAIL, anvil_remote_mail,
	ANVIL_REQ_RCPT, anvil_remote_rcpt,
	ANVIL_REQ_NTLS, anvil_remote_newtls,
	ANVIL_REQ_DISC, anvil_remote_disconnect,
	ANVIL_REQ_NTLS_STAT, anvil_remote_newtls_stat,
	ANVIL_REQ_LOOKUP, anvil_remote_lookup,
	0, 0,
    };
    const ANVIL_REQ_TABLE *rp;

    /*
     * Sanity check. This service takes no command-line arguments.
     */
    if (argv[0])
	msg_fatal("unexpected command-line argument: %s", argv[0]);

    /*
     * Initialize.
     */
    if (request == 0) {
	request = vstring_alloc(10);
	ident = vstring_alloc(10);
    }

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
	for (rp = request_table; /* see below */ ; rp++) {
	    if (rp->name == 0) {
		msg_warn("unrecognized request: \"%s\", ignored", STR(request));
		attr_print_plain(client_stream, ATTR_FLAG_NONE,
			  ATTR_TYPE_INT, ANVIL_ATTR_STATUS, ANVIL_STAT_FAIL,
				 ATTR_TYPE_END);
		break;
	    }
	    if (STREQ(rp->name, STR(request))) {
		rp->action(client_stream, STR(ident));
		break;
	    }
	}
	vstream_fflush(client_stream);
    } else {
	/* Note: invokes anvil_service_done() */
	multi_server_disconnect(client_stream);
    }
    if (msg_verbose)
	msg_info("--- end request ---");
}

/* post_jail_init - post-jail initialization */

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

MAIL_VERSION_STAMP_DECLARE;

/* main - pass control to the multi-threaded skeleton */

int     main(int argc, char **argv)
{
    static const CONFIG_TIME_TABLE time_table[] = {
	VAR_ANVIL_TIME_UNIT, DEF_ANVIL_TIME_UNIT, &var_anvil_time_unit, 1, 0,
	VAR_ANVIL_STAT_TIME, DEF_ANVIL_STAT_TIME, &var_anvil_stat_time, 1, 0,
	0,
    };

    /*
     * Fingerprint executables and core dumps.
     */
    MAIL_VERSION_STAMP_ALLOCATE;

    multi_server_main(argc, argv, anvil_service,
		      MAIL_SERVER_TIME_TABLE, time_table,
		      MAIL_SERVER_POST_INIT, post_jail_init,
		      MAIL_SERVER_SOLITARY,
		      MAIL_SERVER_PRE_DISCONN, anvil_service_done,
		      MAIL_SERVER_EXIT, anvil_status_dump,
		      0);
}
