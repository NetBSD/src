/*	$NetBSD: scache.c,v 1.1.1.2 2006/01/05 02:14:39 rpaulo Exp $	*/

/*++
/* NAME
/*	scache 8
/* SUMMARY
/*	Postfix shared connection cache server
/* SYNOPSIS
/*	\fBscache\fR [generic Postfix daemon options]
/* DESCRIPTION
/*	The \fBscache\fR(8) server maintains a shared multi-connection
/*	cache. This information can be used by, for example, Postfix
/*	SMTP clients or other Postfix delivery agents.
/*
/*	The connection cache is organized into logical destination
/*	names, physical endpoint names, and connections.
/*
/*	As a specific example, logical SMTP destinations specify
/*	(transport, domain, port), and physical SMTP endpoints
/*	specify (transport, IP address, port).  An SMTP connection
/*	may be saved after a successful mail transaction.
/*
/*	In the general case, one logical destination may refer to
/*	zero or more physical endpoints, one physical endpoint may
/*	be referenced by zero or more logical destinations, and
/*	one endpoint may refer to zero or more connections.
/*
/*	The exact syntax of a logical destination or endpoint name
/*	is application dependent; the \fBscache\fR(8) server does
/*	not care.  A connection is stored as a file descriptor together
/*	with application-dependent information that is needed to
/*	re-activate a connection object. Again, the \fBscache\fR(8)
/*	server is completely unaware of the details of that
/*	information.
/*
/*	All information is stored with a finite time to live (ttl).
/*	The connection cache daemon terminates when no client is
/*	connected for \fBmax_idle\fR time units.
/*
/*	This server implements the following requests:
/* .IP "\fBsave_endp\fI ttl endpoint endpoint_properties file_descriptor\fR"
/*	Save the specified file descriptor and connection property data
/*	under the specified endpoint name. The endpoint properties
/*	are used by the client to re-activate a passivated connection
/*	object.
/* .IP "\fBfind_endp\fI endpoint\fR"
/*	Look up cached properties and a cached file descriptor for the
/*	specified endpoint.
/* .IP "\fBsave_dest\fI ttl destination destination_properties endpoint\fR"
/*	Save the binding between a logical destination and an
/*	endpoint under the destination name, together with destination
/*	specific connection properties. The destination properties
/*	are used by the client to re-activate a passivated connection
/*	object.
/* .IP "\fBfind_dest\fI destination\fR"
/*	Look up cached destination properties, cached endpoint properties,
/*	and a cached file descriptor for the specified logical destination.
/* SECURITY
/* .ad
/* .fi
/*	The \fBscache\fR(8) server is not security-sensitive. It does not
/*	talk to the network, and it does not talk to local users.
/*	The \fBscache\fR(8) server can run chrooted at fixed low privilege.
/*
/*	The \fBscache\fR(8) server is not a trusted process. It must
/*	not be used to store information that is security sensitive.
/* DIAGNOSTICS
/*	Problems and transactions are logged to \fBsyslogd\fR(8).
/* BUGS
/*	The session cache cannot be shared among multiple machines.
/*
/*	When a connection expires from the cache, it is closed without
/*	the appropriate protocol specific handshake.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	Changes to \fBmain.cf\fR are picked up automatically as \fBscache\fR(8)
/*	processes run for only a limited amount of time. Use the command
/*	"\fBpostfix reload\fR" to speed up a change.
/*
/*	The text below provides only a parameter summary. See
/*	\fBpostconf\fR(5) for more details including examples.
/* RESOURCE CONTROLS
/* .ad
/* .fi
/* .IP "\fBconnection_cache_ttl_limit (2s)\fR"
/*	The maximal time-to-live value that the \fBscache\fR(8) connection
/*	cache server
/*	allows.
/* .IP "\fBconnection_cache_status_update_time (600s)\fR"
/*	How frequently the \fBscache\fR(8) server logs usage statistics with
/*	connection cache hit and miss rates for logical destinations and for
/*	physical endpoints.
/* MISCELLANEOUS CONTROLS
/* .ad
/* .fi
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
/*	smtp(8), SMTP client
/*	postconf(5), configuration parameters
/*	master(8), process manager
/*	syslogd(8), system logging
/* README FILES
/* .ad
/* .fi
/*	Use "\fBpostconf readme_directory\fR" or
/*	"\fBpostconf html_directory\fR" to locate this information.
/* .na
/* .nf
/*	CONNECTION_CACHE_README, Postfix connection cache
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* HISTORY
/*	This service was introduced with Postfix version 2.2.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <time.h>

/* Utility library. */

#include <msg.h>
#include <iostuff.h>
#include <htable.h>
#include <ring.h>
#include <events.h>

/* Global library. */

#include <mail_params.h>
#include <mail_proto.h>
#include <scache.h>

/* Single server skeleton. */

#include <mail_server.h>
#include <mail_conf.h>

/* Application-specific. */

 /*
  * Tunable parameters.
  */
int     var_scache_ttl_lim;
int     var_scache_stat_time;

 /*
  * Request parameters.
  */
static VSTRING *scache_request;
static VSTRING *scache_dest_label;
static VSTRING *scache_dest_prop;
static VSTRING *scache_endp_label;
static VSTRING *scache_endp_prop;

#ifdef CANT_WRITE_BEFORE_SENDING_FD
static VSTRING *scache_dummy;

#endif

 /*
  * Session cache instance.
  */
static SCACHE *scache;

 /*
  * Statistics.
  */
static int scache_dest_hits;
static int scache_dest_miss;
static int scache_dest_count;
static int scache_endp_hits;
static int scache_endp_miss;
static int scache_endp_count;
static int scache_sess_count;
time_t  scache_start_time;

 /*
  * Silly little macros.
  */
#define STR(x)			vstring_str(x)
#define VSTREQ(x,y)		(strcmp(STR(x),y) == 0)

/* scache_save_endp_service - protocol to save endpoint->stream binding */

static void scache_save_endp_service(VSTREAM *client_stream)
{
    const char *myname = "scache_save_endp_service";
    int     ttl;
    int     fd;
    SCACHE_SIZE size;

    if (attr_scan(client_stream,
		  ATTR_FLAG_STRICT,
		  ATTR_TYPE_NUM, MAIL_ATTR_TTL, &ttl,
		  ATTR_TYPE_STR, MAIL_ATTR_LABEL, scache_endp_label,
		  ATTR_TYPE_STR, MAIL_ATTR_PROP, scache_endp_prop,
		  ATTR_TYPE_END) != 3
	|| ttl <= 0) {
	msg_warn("%s: bad or missing request parameter", myname);
	attr_print(client_stream, ATTR_FLAG_NONE,
		   ATTR_TYPE_NUM, MAIL_ATTR_STATUS, SCACHE_STAT_BAD,
		   ATTR_TYPE_END);
	return;
    } else if (
#ifdef CANT_WRITE_BEFORE_SENDING_FD
	       attr_print(client_stream, ATTR_FLAG_NONE,
			  ATTR_TYPE_STR, MAIL_ATTR_DUMMY, "",
			  ATTR_TYPE_END) != 0
	       || vstream_fflush(client_stream) != 0
	       || read_wait(vstream_fileno(client_stream),
			    client_stream->timeout) < 0	/* XXX */
	       ||
#endif
	       (fd = LOCAL_RECV_FD(vstream_fileno(client_stream))) < 0) {
	msg_warn("%s: unable to receive file descriptor: %m", myname);
	(void) attr_print(client_stream, ATTR_FLAG_NONE,
			  ATTR_TYPE_NUM, MAIL_ATTR_STATUS, SCACHE_STAT_FAIL,
			  ATTR_TYPE_END);
	return;
    } else {
	scache_save_endp(scache,
			 ttl > var_scache_ttl_lim ? var_scache_ttl_lim : ttl,
			 STR(scache_endp_label), STR(scache_endp_prop), fd);
	(void) attr_print(client_stream, ATTR_FLAG_NONE,
			  ATTR_TYPE_NUM, MAIL_ATTR_STATUS, SCACHE_STAT_OK,
			  ATTR_TYPE_END);
	scache_size(scache, &size);
	if (size.endp_count > scache_endp_count)
	    scache_endp_count = size.endp_count;
	if (size.sess_count > scache_sess_count)
	    scache_sess_count = size.sess_count;
	return;
    }
}

/* scache_find_endp_service - protocol to find connection for endpoint */

static void scache_find_endp_service(VSTREAM *client_stream)
{
    const char *myname = "scache_find_endp_service";
    int     fd;

    if (attr_scan(client_stream,
		  ATTR_FLAG_STRICT,
		  ATTR_TYPE_STR, MAIL_ATTR_LABEL, scache_endp_label,
		  ATTR_TYPE_END) != 1) {
	msg_warn("%s: bad or missing request parameter", myname);
	attr_print(client_stream, ATTR_FLAG_NONE,
		   ATTR_TYPE_NUM, MAIL_ATTR_STATUS, SCACHE_STAT_BAD,
		   ATTR_TYPE_STR, MAIL_ATTR_PROP, "",
		   ATTR_TYPE_END);
	return;
    } else if ((fd = scache_find_endp(scache, STR(scache_endp_label),
				      scache_endp_prop)) < 0) {
	attr_print(client_stream, ATTR_FLAG_NONE,
		   ATTR_TYPE_NUM, MAIL_ATTR_STATUS, SCACHE_STAT_FAIL,
		   ATTR_TYPE_STR, MAIL_ATTR_PROP, "",
		   ATTR_TYPE_END);
	scache_endp_miss++;
	return;
    } else {
	attr_print(client_stream, ATTR_FLAG_NONE,
		   ATTR_TYPE_NUM, MAIL_ATTR_STATUS, SCACHE_STAT_OK,
		   ATTR_TYPE_STR, MAIL_ATTR_PROP, STR(scache_endp_prop),
		   ATTR_TYPE_END);
	if (vstream_fflush(client_stream) != 0
#ifdef CANT_WRITE_BEFORE_SENDING_FD
	    || attr_scan(client_stream, ATTR_FLAG_STRICT,
			 ATTR_TYPE_STR, MAIL_ATTR_DUMMY, scache_dummy,
			 ATTR_TYPE_END) != 1
#endif
	    || LOCAL_SEND_FD(vstream_fileno(client_stream), fd) < 0
#ifdef MUST_READ_AFTER_SENDING_FD
	    || attr_scan(client_stream, ATTR_FLAG_STRICT,
			 ATTR_TYPE_STR, MAIL_ATTR_DUMMY, scache_dummy,
			 ATTR_TYPE_END) != 1
#endif
	    )
	    msg_warn("%s: cannot send file descriptor: %m", myname);
	if (close(fd) < 0)
	    msg_warn("close(%d): %m", fd);
	scache_endp_hits++;
	return;
    }
}

/* scache_save_dest_service - protocol to save destination->endpoint binding */

static void scache_save_dest_service(VSTREAM *client_stream)
{
    const char *myname = "scache_save_dest_service";
    int     ttl;
    SCACHE_SIZE size;

    if (attr_scan(client_stream,
		  ATTR_FLAG_STRICT,
		  ATTR_TYPE_NUM, MAIL_ATTR_TTL, &ttl,
		  ATTR_TYPE_STR, MAIL_ATTR_LABEL, scache_dest_label,
		  ATTR_TYPE_STR, MAIL_ATTR_PROP, scache_dest_prop,
		  ATTR_TYPE_STR, MAIL_ATTR_LABEL, scache_endp_label,
		  ATTR_TYPE_END) != 4
	|| ttl <= 0) {
	msg_warn("%s: bad or missing request parameter", myname);
	attr_print(client_stream, ATTR_FLAG_NONE,
		   ATTR_TYPE_NUM, MAIL_ATTR_STATUS, SCACHE_STAT_BAD,
		   ATTR_TYPE_END);
	return;
    } else {
	scache_save_dest(scache,
			 ttl > var_scache_ttl_lim ? var_scache_ttl_lim : ttl,
			 STR(scache_dest_label), STR(scache_dest_prop),
			 STR(scache_endp_label));
	attr_print(client_stream, ATTR_FLAG_NONE,
		   ATTR_TYPE_NUM, MAIL_ATTR_STATUS, SCACHE_STAT_OK,
		   ATTR_TYPE_END);
	scache_size(scache, &size);
	if (size.dest_count > scache_dest_count)
	    scache_dest_count = size.dest_count;
	if (size.endp_count > scache_endp_count)
	    scache_endp_count = size.endp_count;
	return;
    }
}

/* scache_find_dest_service - protocol to find connection for destination */

static void scache_find_dest_service(VSTREAM *client_stream)
{
    const char *myname = "scache_find_dest_service";
    int     fd;

    if (attr_scan(client_stream,
		  ATTR_FLAG_STRICT,
		  ATTR_TYPE_STR, MAIL_ATTR_LABEL, scache_dest_label,
		  ATTR_TYPE_END) != 1) {
	msg_warn("%s: bad or missing request parameter", myname);
	attr_print(client_stream, ATTR_FLAG_NONE,
		   ATTR_TYPE_NUM, MAIL_ATTR_STATUS, SCACHE_STAT_BAD,
		   ATTR_TYPE_STR, MAIL_ATTR_PROP, "",
		   ATTR_TYPE_STR, MAIL_ATTR_PROP, "",
		   ATTR_TYPE_END);
	return;
    } else if ((fd = scache_find_dest(scache, STR(scache_dest_label),
				      scache_dest_prop,
				      scache_endp_prop)) < 0) {
	attr_print(client_stream, ATTR_FLAG_NONE,
		   ATTR_TYPE_NUM, MAIL_ATTR_STATUS, SCACHE_STAT_FAIL,
		   ATTR_TYPE_STR, MAIL_ATTR_PROP, "",
		   ATTR_TYPE_STR, MAIL_ATTR_PROP, "",
		   ATTR_TYPE_END);
	scache_dest_miss++;
	return;
    } else {
	attr_print(client_stream, ATTR_FLAG_NONE,
		   ATTR_TYPE_NUM, MAIL_ATTR_STATUS, SCACHE_STAT_OK,
		   ATTR_TYPE_STR, MAIL_ATTR_PROP, STR(scache_dest_prop),
		   ATTR_TYPE_STR, MAIL_ATTR_PROP, STR(scache_endp_prop),
		   ATTR_TYPE_END);
	if (vstream_fflush(client_stream) != 0
#ifdef CANT_WRITE_BEFORE_SENDING_FD
	    || attr_scan(client_stream, ATTR_FLAG_STRICT,
			 ATTR_TYPE_STR, MAIL_ATTR_DUMMY, scache_dummy,
			 ATTR_TYPE_END) != 1
#endif
	    || LOCAL_SEND_FD(vstream_fileno(client_stream), fd) < 0
#ifdef MUST_READ_AFTER_SENDING_FD
	    || attr_scan(client_stream, ATTR_FLAG_STRICT,
			 ATTR_TYPE_STR, MAIL_ATTR_DUMMY, scache_dummy,
			 ATTR_TYPE_END) != 1
#endif
	    )
	    msg_warn("%s: cannot send file descriptor: %m", myname);
	if (close(fd) < 0)
	    msg_warn("close(%d): %m", fd);
	scache_dest_hits++;
	return;
    }
}

/* scache_service - perform service for client */

static void scache_service(VSTREAM *client_stream, char *unused_service,
			           char **argv)
{

    /*
     * Sanity check. This service takes no command-line arguments.
     */
    if (argv[0])
	msg_fatal("unexpected command-line argument: %s", argv[0]);

    /*
     * This routine runs whenever a client connects to the UNIX-domain socket
     * dedicated to the scache service. All connection-management stuff is
     * handled by the common code in multi_server.c.
     */
do {
    if (attr_scan(client_stream,
		  ATTR_FLAG_MORE | ATTR_FLAG_STRICT,
		  ATTR_TYPE_STR, MAIL_ATTR_REQ, scache_request,
		  ATTR_TYPE_END) == 1) {
	if (VSTREQ(scache_request, SCACHE_REQ_SAVE_DEST)) {
	    scache_save_dest_service(client_stream);
	} else if (VSTREQ(scache_request, SCACHE_REQ_FIND_DEST)) {
	    scache_find_dest_service(client_stream);
	} else if (VSTREQ(scache_request, SCACHE_REQ_SAVE_ENDP)) {
	    scache_save_endp_service(client_stream);
	} else if (VSTREQ(scache_request, SCACHE_REQ_FIND_ENDP)) {
	    scache_find_endp_service(client_stream);
	} else {
	    msg_warn("unrecognized request: \"%s\", ignored",
		     STR(scache_request));
	    attr_print(client_stream, ATTR_FLAG_NONE,
		       ATTR_TYPE_NUM, MAIL_ATTR_STATUS, SCACHE_STAT_BAD,
		       ATTR_TYPE_END);
	}
    }
} while (vstream_peek(client_stream) > 0);
    vstream_fflush(client_stream);
}

/* scache_status_dump - log and reset cache statistics */

static void scache_status_dump(char *unused_name, char **unused_argv)
{
    if (scache_dest_hits || scache_dest_miss
	|| scache_endp_hits || scache_endp_miss
	|| scache_dest_count || scache_endp_count
	|| scache_sess_count)
	msg_info("statistics: start interval %.15s",
		 ctime(&scache_start_time) + 4);

    if (scache_dest_hits || scache_dest_miss) {
	msg_info("statistics: domain lookup hits=%d miss=%d success=%d%%",
		 scache_dest_hits, scache_dest_miss,
		 scache_dest_hits * 100
		 / (scache_dest_hits + scache_dest_miss));
	scache_dest_hits = scache_dest_miss = 0;
    }
    if (scache_endp_hits || scache_endp_miss) {
	msg_info("statistics: address lookup hits=%d miss=%d success=%d%%",
		 scache_endp_hits, scache_endp_miss,
		 scache_endp_hits * 100
		 / (scache_endp_hits + scache_endp_miss));
	scache_endp_hits = scache_endp_miss = 0;
    }
    if (scache_dest_count || scache_endp_count || scache_sess_count) {
	msg_info("statistics: max simultaneous domains=%d addresses=%d connection=%d",
		 scache_dest_count, scache_endp_count, scache_sess_count);
	scache_dest_count = 0;
	scache_endp_count = 0;
	scache_sess_count = 0;
    }
    scache_start_time = event_time();
}

/* scache_status_update - log and reset cache statistics periodically */

static void scache_status_update(int unused_event, char *context)
{
    scache_status_dump((char *) 0, (char **) 0);
    event_request_timer(scache_status_update, context, var_scache_stat_time);
}

/* post_jail_init - initialization after privilege drop */

static void post_jail_init(char *unused_name, char **unused_argv)
{

    /*
     * Pre-allocate the cache instance.
     */
    scache = scache_multi_create();

    /*
     * Pre-allocate buffers.
     */
    scache_request = vstring_alloc(10);
    scache_dest_label = vstring_alloc(10);
    scache_dest_prop = vstring_alloc(10);
    scache_endp_label = vstring_alloc(10);
    scache_endp_prop = vstring_alloc(10);
#ifdef CANT_WRITE_BEFORE_SENDING_FD
    scache_dummy = vstring_alloc(10);
#endif

    /*
     * Disable the max_use limit. We still terminate when no client is
     * connected for $idle_limit time units.
     */
    var_use_limit = 0;

    /*
     * Dump and reset cache statistics every so often.
     */
    event_request_timer(scache_status_update, (char *) 0, var_scache_stat_time);
    scache_start_time = event_time();
}

/* main - pass control to the multi-threaded skeleton */

int     main(int argc, char **argv)
{
    static CONFIG_TIME_TABLE time_table[] = {
	VAR_SCACHE_TTL_LIM, DEF_SCACHE_TTL_LIM, &var_scache_ttl_lim, 1, 0,
	VAR_SCACHE_STAT_TIME, DEF_SCACHE_STAT_TIME, &var_scache_stat_time, 1, 0,
	0,
    };

    multi_server_main(argc, argv, scache_service,
		      MAIL_SERVER_TIME_TABLE, time_table,
		      MAIL_SERVER_POST_INIT, post_jail_init,
		      MAIL_SERVER_EXIT, scache_status_dump,
		      MAIL_SERVER_SOLITARY,
		      0);
}
