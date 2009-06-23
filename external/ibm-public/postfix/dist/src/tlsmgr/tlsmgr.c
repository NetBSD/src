/*	$NetBSD: tlsmgr.c,v 1.1.1.1 2009/06/23 10:08:58 tron Exp $	*/

/*++
/* NAME
/*	tlsmgr 8
/* SUMMARY
/*	Postfix TLS session cache and PRNG manager
/* SYNOPSIS
/*	\fBtlsmgr\fR [generic Postfix daemon options]
/* DESCRIPTION
/*	The \fBtlsmgr\fR(8) manages the Postfix TLS session caches.
/*	It stores and retrieves cache entries on request by
/*	\fBsmtpd\fR(8) and \fBsmtp\fR(8) processes, and periodically
/*	removes entries that have expired.
/*
/*	The \fBtlsmgr\fR(8) also manages the PRNG (pseudo random number
/*	generator) pool. It answers queries by the \fBsmtpd\fR(8)
/*	and \fBsmtp\fR(8)
/*	processes to seed their internal PRNG pools.
/*
/*	The \fBtlsmgr\fR(8)'s PRNG pool is initially seeded from
/*	an external source (EGD, /dev/urandom, or regular file).
/*	It is updated at configurable pseudo-random intervals with
/*	data from the external source. It is updated periodically
/*	with data from TLS session cache entries and with the time
/*	of day, and is updated with the time of day whenever a
/*	process requests \fBtlsmgr\fR(8) service.
/*
/*	The \fBtlsmgr\fR(8) saves the PRNG state to an exchange file
/*	periodically and when the process terminates, and reads
/*	the exchange file when initializing its PRNG.
/* SECURITY
/* .ad
/* .fi
/*	The \fBtlsmgr\fR(8) is not security-sensitive. The code that maintains
/*	the external and internal PRNG pools does not "trust" the
/*	data that it manipulates, and the code that maintains the
/*	TLS session cache does not touch the contents of the cached
/*	entries, except for seeding its internal PRNG pool.
/*
/*	The \fBtlsmgr\fR(8) can be run chrooted and with reduced privileges.
/*	At process startup it connects to the entropy source and
/*	exchange file, and creates or truncates the optional TLS
/*	session cache files.
/*
/*	With Postfix version 2.5 and later, the \fBtlsmgr\fR(8) no
/*	longer uses root privileges when opening cache files. These
/*	files should now be stored under the Postfix-owned
/*	\fBdata_directory\fR.  As a migration aid, an attempt to
/*	open a cache file under a non-Postfix directory is redirected
/*	to the Postfix-owned \fBdata_directory\fR, and a warning
/*	is logged.
/* DIAGNOSTICS
/*	Problems and transactions are logged to the syslog daemon.
/* BUGS
/*	There is no automatic means to limit the number of entries in the
/*	TLS session caches and/or the size of the TLS cache files.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	Changes to \fBmain.cf\fR are not picked up automatically,
/*	because \fBtlsmgr\fR(8) is a persistent processes.  Use the
/*	command "\fBpostfix reload\fR" after a configuration change.
/*
/*	The text below provides only a parameter summary. See
/*	\fBpostconf\fR(5) for more details including examples.
/* TLS SESSION CACHE
/* .ad
/* .fi
/* .IP "\fBlmtp_tls_loglevel (0)\fR"
/*	The LMTP-specific version of the smtp_tls_loglevel
/*	configuration parameter.
/* .IP "\fBlmtp_tls_session_cache_database (empty)\fR"
/*	The LMTP-specific version of the smtp_tls_session_cache_database
/*	configuration parameter.
/* .IP "\fBlmtp_tls_session_cache_timeout (3600s)\fR"
/*	The LMTP-specific version of the smtp_tls_session_cache_timeout
/*	configuration parameter.
/* .IP "\fBsmtp_tls_loglevel (0)\fR"
/*	Enable additional Postfix SMTP client logging of TLS activity.
/* .IP "\fBsmtp_tls_session_cache_database (empty)\fR"
/*	Name of the file containing the optional Postfix SMTP client
/*	TLS session cache.
/* .IP "\fBsmtp_tls_session_cache_timeout (3600s)\fR"
/*	The expiration time of Postfix SMTP client TLS session cache
/*	information.
/* .IP "\fBsmtpd_tls_loglevel (0)\fR"
/*	Enable additional Postfix SMTP server logging of TLS activity.
/* .IP "\fBsmtpd_tls_session_cache_database (empty)\fR"
/*	Name of the file containing the optional Postfix SMTP server
/*	TLS session cache.
/* .IP "\fBsmtpd_tls_session_cache_timeout (3600s)\fR"
/*	The expiration time of Postfix SMTP server TLS session cache
/*	information.
/* PSEUDO RANDOM NUMBER GENERATOR
/* .ad
/* .fi
/* .IP "\fBtls_random_source (see 'postconf -d' output)\fR"
/*	The external entropy source for the in-memory \fBtlsmgr\fR(8) pseudo
/*	random number generator (PRNG) pool.
/* .IP "\fBtls_random_bytes (32)\fR"
/*	The number of bytes that \fBtlsmgr\fR(8) reads from $tls_random_source
/*	when (re)seeding the in-memory pseudo random number generator (PRNG)
/*	pool.
/* .IP "\fBtls_random_exchange_name (see 'postconf -d' output)\fR"
/*	Name of the pseudo random number generator (PRNG) state file
/*	that is maintained by \fBtlsmgr\fR(8).
/* .IP "\fBtls_random_prng_update_period (3600s)\fR"
/*	The time between attempts by \fBtlsmgr\fR(8) to save the state of
/*	the pseudo random number generator (PRNG) to the file specified
/*	with $tls_random_exchange_name.
/* .IP "\fBtls_random_reseed_period (3600s)\fR"
/*	The maximal time between attempts by \fBtlsmgr\fR(8) to re-seed the
/*	in-memory pseudo random number generator (PRNG) pool from external
/*	sources.
/* MISCELLANEOUS CONTROLS
/* .ad
/* .fi
/* .IP "\fBconfig_directory (see 'postconf -d' output)\fR"
/*	The default location of the Postfix main.cf and master.cf
/*	configuration files.
/* .IP "\fBdata_directory (see 'postconf -d' output)\fR"
/*	The directory with Postfix-writable data files (for example:
/*	caches, pseudo-random numbers).
/* .IP "\fBdaemon_timeout (18000s)\fR"
/*	How much time a Postfix daemon process may take to handle a
/*	request before it is terminated by a built-in watchdog timer.
/* .IP "\fBprocess_id (read-only)\fR"
/*	The process ID of a Postfix command or daemon process.
/* .IP "\fBprocess_name (read-only)\fR"
/*	The process name of a Postfix command or daemon process.
/* .IP "\fBsyslog_facility (mail)\fR"
/*	The syslog facility of Postfix logging.
/* .IP "\fBsyslog_name (see 'postconf -d' output)\fR"
/*	The mail system name that is prepended to the process name in syslog
/*	records, so that "smtpd" becomes, for example, "postfix/smtpd".
/* SEE ALSO
/*	smtp(8), Postfix SMTP client
/*	smtpd(8), Postfix SMTP server
/*	postconf(5), configuration parameters
/*	master(5), generic daemon options
/*	master(8), process manager
/*	syslogd(8), system logging
/* README FILES
/* .ad
/* .fi
/*	Use "\fBpostconf readme_directory\fR" or
/*	"\fBpostconf html_directory\fR" to locate this information.
/* .na
/* .nf
/*	TLS_README, Postfix TLS configuration and operation
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Lutz Jaenicke
/*	BTU Cottbus
/*	Allgemeine Elektrotechnik
/*	Universitaetsplatz 3-4
/*	D-03044 Cottbus, Germany
/*
/*	Adapted by:
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>			/* gettimeofday, not POSIX */
#include <limits.h>

#ifndef UCHAR_MAX
#define UCHAR_MAX 0xff
#endif

/* OpenSSL library. */

#ifdef USE_TLS
#include <openssl/rand.h>		/* For the PRNG */
#endif

/* Utility library. */

#include <msg.h>
#include <events.h>
#include <stringops.h>
#include <mymalloc.h>
#include <iostuff.h>
#include <vstream.h>
#include <vstring.h>
#include <vstring_vstream.h>
#include <attr.h>
#include <set_eugid.h>
#include <htable.h>

/* Global library. */

#include <mail_conf.h>
#include <mail_params.h>
#include <mail_version.h>
#include <tls_mgr.h>
#include <mail_proto.h>
#include <data_redirect.h>

/* Master process interface. */

#include <master_proto.h>
#include <mail_server.h>

/* TLS library. */

#ifdef USE_TLS
#include <tls.h>			/* TLS_MGR_SCACHE_<type> */
#include <tls_prng.h>
#include <tls_scache.h>

/* Application-specific. */

 /*
  * Tunables.
  */
char   *var_tls_rand_source;
int     var_tls_rand_bytes;
int     var_tls_reseed_period;
int     var_tls_prng_exch_period;
int     var_smtpd_tls_loglevel;
char   *var_smtpd_tls_scache_db;
int     var_smtpd_tls_scache_timeout;
int     var_smtp_tls_loglevel;
char   *var_smtp_tls_scache_db;
int     var_smtp_tls_scache_timeout;
int     var_lmtp_tls_loglevel;
char   *var_lmtp_tls_scache_db;
int     var_lmtp_tls_scache_timeout;
char   *var_tls_rand_exch_name;

 /*
  * Bound the time that we are willing to wait for an I/O operation. This
  * produces better error messages than waiting until the watchdog timer
  * kills the process.
  */
#define TLS_MGR_TIMEOUT	10

 /*
  * State for updating the PRNG exchange file.
  */
static TLS_PRNG_SRC *rand_exch;

 /*
  * State for seeding the internal PRNG from external source.
  */
static TLS_PRNG_SRC *rand_source_dev;
static TLS_PRNG_SRC *rand_source_egd;
static TLS_PRNG_SRC *rand_source_file;

 /*
  * The external entropy source type is encoded in the source name. The
  * obvious alternative is to have separate configuration parameters per
  * source type, so that one process can query multiple external sources.
  */
#define DEV_PREF "dev:"
#define DEV_PREF_LEN (sizeof((DEV_PREF)) - 1)
#define DEV_PATH(dev) ((dev) + EGD_PREF_LEN)

#define EGD_PREF "egd:"
#define EGD_PREF_LEN (sizeof((EGD_PREF)) - 1)
#define EGD_PATH(egd) ((egd) + EGD_PREF_LEN)

 /*
  * State for TLS session caches.
  */
typedef struct {
    char   *cache_label;		/* cache short-hand name */
    TLS_SCACHE *cache_info;		/* cache handle */
    int     cache_active;		/* cache status */
    char  **cache_db;			/* main.cf parameter value */
    int    *cache_loglevel;		/* main.cf parameter value */
    int    *cache_timeout;		/* main.cf parameter value */
} TLSMGR_SCACHE;

TLSMGR_SCACHE cache_table[] = {
    TLS_MGR_SCACHE_SMTPD, 0, 0, &var_smtpd_tls_scache_db,
    &var_smtpd_tls_loglevel, &var_smtpd_tls_scache_timeout,
    TLS_MGR_SCACHE_SMTP, 0, 0, &var_smtp_tls_scache_db,
    &var_smtp_tls_loglevel, &var_smtp_tls_scache_timeout,
    TLS_MGR_SCACHE_LMTP, 0, 0, &var_lmtp_tls_scache_db,
    &var_lmtp_tls_loglevel, &var_lmtp_tls_scache_timeout,
    0,
};

 /*
  * SLMs.
  */
#define STR(x)		vstring_str(x)
#define LEN(x)		VSTRING_LEN(x)
#define STREQ(x, y)	(strcmp((x), (y)) == 0)

/* tlsmgr_prng_exch_event - update PRNG exchange file */

static void tlsmgr_prng_exch_event(int unused_event, char *dummy)
{
    const char *myname = "tlsmgr_prng_exch_event";
    unsigned char randbyte;
    int     next_period;
    struct stat st;

    if (msg_verbose)
	msg_info("%s: update PRNG exchange file", myname);

    /*
     * Sanity check. If the PRNG exchange file was removed, there is no point
     * updating it further. Restart the process and update the new file.
     */
    if (fstat(rand_exch->fd, &st) < 0)
	msg_fatal("cannot fstat() the PRNG exchange file: %m");
    if (st.st_nlink == 0) {
	msg_warn("PRNG exchange file was removed -- exiting to reopen");
	sleep(1);
	exit(0);
    }
    tls_prng_exch_update(rand_exch);

    /*
     * Make prediction difficult for outsiders and calculate the time for the
     * next execution randomly.
     */
    RAND_bytes(&randbyte, 1);
    next_period = (var_tls_prng_exch_period * randbyte) / UCHAR_MAX;
    event_request_timer(tlsmgr_prng_exch_event, dummy, next_period);
}

/* tlsmgr_reseed_event - re-seed the internal PRNG pool */

static void tlsmgr_reseed_event(int unused_event, char *dummy)
{
    int     next_period;
    unsigned char randbyte;
    int     must_exit = 0;

    /*
     * Reseed the internal PRNG from external source. Errors are recoverable.
     * We simply restart and reconnect without making a fuss. This is OK
     * because we do require that exchange file updates succeed. The exchange
     * file is the only entropy source that really matters in the long term.
     * 
     * If the administrator specifies an external randomness source that we
     * could not open upon start-up, restart to see if we can open it now
     * (and log a nagging warning if we can't).
     */
    if (*var_tls_rand_source) {

	/*
	 * Source is a random device.
	 */
	if (rand_source_dev) {
	    if (tls_prng_dev_read(rand_source_dev, var_tls_rand_bytes) <= 0) {
		msg_info("cannot read from entropy device %s: %m -- "
			 "exiting to reopen", DEV_PATH(var_tls_rand_source));
		must_exit = 1;
	    }
	}

	/*
	 * Source is an EGD compatible socket.
	 */
	else if (rand_source_egd) {
	    if (tls_prng_egd_read(rand_source_egd, var_tls_rand_bytes) <= 0) {
		msg_info("lost connection to EGD server %s -- "
		     "exiting to reconnect", EGD_PATH(var_tls_rand_source));
		must_exit = 1;
	    }
	}

	/*
	 * Source is a regular file. Read the content once and close the
	 * file.
	 */
	else if (rand_source_file) {
	    if (tls_prng_file_read(rand_source_file, var_tls_rand_bytes) <= 0)
		msg_warn("cannot read from entropy file %s: %m",
			 var_tls_rand_source);
	    tls_prng_file_close(rand_source_file);
	    rand_source_file = 0;
	    var_tls_rand_source[0] = 0;
	}

	/*
	 * Could not open the external source upon start-up. See if we can
	 * open it this time. Save PRNG state before we exit.
	 */
	else {
	    msg_info("exiting to reopen external entropy source %s",
		     var_tls_rand_source);
	    must_exit = 1;
	}
    }

    /*
     * Save PRNG state in case we must exit.
     */
    if (must_exit) {
	if (rand_exch)
	    tls_prng_exch_update(rand_exch);
	sleep(1);
	exit(0);
    }

    /*
     * Make prediction difficult for outsiders and calculate the time for the
     * next execution randomly.
     */
    RAND_bytes(&randbyte, 1);
    next_period = (var_tls_reseed_period * randbyte) / UCHAR_MAX;
    event_request_timer(tlsmgr_reseed_event, dummy, next_period);
}

/* tlsmgr_cache_run_event - start TLS session cache scan */

static void tlsmgr_cache_run_event(int unused_event, char *ctx)
{
    const char *myname = "tlsmgr_cache_run_event";
    TLSMGR_SCACHE *cache = (TLSMGR_SCACHE *) ctx;

    /*
     * This routine runs when it is time for another TLS session cache scan.
     * Make sure this routine gets called again in the future.
     * 
     * Don't start a new scan when the timer goes off while cache cleanup is
     * still in progress.
     */
    if (cache->cache_info->verbose)
	msg_info("%s: start TLS %s session cache cleanup",
		 myname, cache->cache_label);

    if (cache->cache_active == 0)
	cache->cache_active =
	    tls_scache_sequence(cache->cache_info, DICT_SEQ_FUN_FIRST,
				TLS_SCACHE_SEQUENCE_NOTHING);

    event_request_timer(tlsmgr_cache_run_event, (char *) cache,
			cache->cache_info->timeout);
}

/* tlsmgr_loop - TLS manager main loop */

static int tlsmgr_loop(char *unused_name, char **unused_argv)
{
    struct timeval tv;
    int     active = 0;
    TLSMGR_SCACHE *ent;

    /*
     * Update the PRNG pool with the time of day. We do it here after every
     * event (including internal timer events and external client request
     * events), instead of doing it in individual event call-back routines.
     */
    GETTIMEOFDAY(&tv);
    RAND_seed(&tv, sizeof(struct timeval));

    /*
     * This routine runs as part of the event handling loop, after the event
     * manager has delivered a timer or I/O event, or after it has waited for
     * a specified amount of time. The result value of tlsmgr_loop()
     * specifies how long the event manager should wait for the next event.
     * 
     * We use this loop to interleave TLS session cache cleanup with other
     * activity. Interleaved processing is needed when we use a client-server
     * protocol for entropy and session state exchange with smtp(8) and
     * smtpd(8) processes.
     */
#define DONT_WAIT	0
#define WAIT_FOR_EVENT	(-1)

    for (ent = cache_table; ent->cache_label; ++ent) {
	if (ent->cache_info && ent->cache_active)
	    active |= ent->cache_active =
		tls_scache_sequence(ent->cache_info, DICT_SEQ_FUN_NEXT,
				    TLS_SCACHE_SEQUENCE_NOTHING);
    }

    return (active ? DONT_WAIT : WAIT_FOR_EVENT);
}

/* tlsmgr_request_receive - receive request */

static int tlsmgr_request_receive(VSTREAM *client_stream, VSTRING *request)
{
    int     count;

    /*
     * Kluge: choose the protocol depending on the request size.
     */
    if (read_wait(vstream_fileno(client_stream), var_ipc_timeout) < 0) {
	msg_warn("timeout while waiting for data from %s",
		 VSTREAM_PATH(client_stream));
	return (-1);
    }
    if ((count = peekfd(vstream_fileno(client_stream))) < 0) {
	msg_warn("cannot examine read buffer of %s: %m",
		 VSTREAM_PATH(client_stream));
	return (-1);
    }

    /*
     * Short request: master trigger. Use the string+null protocol.
     */
    if (count <= 2) {
	if (vstring_get_null(request, client_stream) == VSTREAM_EOF) {
	    msg_warn("end-of-input while reading request from %s: %m",
		     VSTREAM_PATH(client_stream));
	    return (-1);
	}
    }

    /*
     * Long request: real tlsmgr client. Use the attribute list protocol.
     */
    else {
	if (attr_scan(client_stream,
		      ATTR_FLAG_MORE | ATTR_FLAG_STRICT,
		      ATTR_TYPE_STR, TLS_MGR_ATTR_REQ, request,
		      ATTR_TYPE_END) != 1) {
	    return (-1);
	}
    }
    return (0);
}

/* tlsmgr_service - respond to external request */

static void tlsmgr_service(VSTREAM *client_stream, char *unused_service,
			           char **argv)
{
    static VSTRING *request = 0;
    static VSTRING *cache_type = 0;
    static VSTRING *cache_id = 0;
    static VSTRING *buffer = 0;
    int     len;
    static char wakeup[] = {		/* master wakeup request */
	TRIGGER_REQ_WAKEUP,
	0,
    };
    TLSMGR_SCACHE *ent;
    int     status = TLS_MGR_STAT_FAIL;

    /*
     * Sanity check. This service takes no command-line arguments.
     */
    if (argv[0])
	msg_fatal("unexpected command-line argument: %s", argv[0]);

    /*
     * Initialize. We're select threaded, so we can use static buffers.
     */
    if (request == 0) {
	request = vstring_alloc(10);
	cache_type = vstring_alloc(10);
	cache_id = vstring_alloc(10);
	buffer = vstring_alloc(10);
    }

    /*
     * This routine runs whenever a client connects to the socket dedicated
     * to the tlsmgr service (including wake up events sent by the master).
     * All connection-management stuff is handled by the common code in
     * multi_server.c.
     */
    if (tlsmgr_request_receive(client_stream, request) == 0) {

	/*
	 * Load session from cache.
	 */
	if (STREQ(STR(request), TLS_MGR_REQ_LOOKUP)) {
	    if (attr_scan(client_stream, ATTR_FLAG_STRICT,
			  ATTR_TYPE_STR, TLS_MGR_ATTR_CACHE_TYPE, cache_type,
			  ATTR_TYPE_STR, TLS_MGR_ATTR_CACHE_ID, cache_id,
			  ATTR_TYPE_END) == 2) {
		for (ent = cache_table; ent->cache_label; ++ent)
		    if (strcmp(ent->cache_label, STR(cache_type)) == 0)
			break;
		if (ent->cache_label == 0) {
		    msg_warn("bogus cache type \"%s\" in \"%s\" request",
			     STR(cache_type), TLS_MGR_REQ_LOOKUP);
		    VSTRING_RESET(buffer);
		} else if (ent->cache_info == 0) {

		    /*
		     * Cache type valid, but not enabled
		     */
		    VSTRING_RESET(buffer);
		} else {
		    status = tls_scache_lookup(ent->cache_info,
					       STR(cache_id), buffer) ?
			TLS_MGR_STAT_OK : TLS_MGR_STAT_ERR;
		}
	    }
	    attr_print(client_stream, ATTR_FLAG_NONE,
		       ATTR_TYPE_INT, MAIL_ATTR_STATUS, status,
		       ATTR_TYPE_DATA, TLS_MGR_ATTR_SESSION,
		       LEN(buffer), STR(buffer),
		       ATTR_TYPE_END);
	}

	/*
	 * Save session to cache.
	 */
	else if (STREQ(STR(request), TLS_MGR_REQ_UPDATE)) {
	    if (attr_scan(client_stream, ATTR_FLAG_STRICT,
			  ATTR_TYPE_STR, TLS_MGR_ATTR_CACHE_TYPE, cache_type,
			  ATTR_TYPE_STR, TLS_MGR_ATTR_CACHE_ID, cache_id,
			  ATTR_TYPE_DATA, TLS_MGR_ATTR_SESSION, buffer,
			  ATTR_TYPE_END) == 3) {
		for (ent = cache_table; ent->cache_label; ++ent)
		    if (strcmp(ent->cache_label, STR(cache_type)) == 0)
			break;
		if (ent->cache_label == 0) {
		    msg_warn("bogus cache type \"%s\" in \"%s\" request",
			     STR(cache_type), TLS_MGR_REQ_UPDATE);
		} else if (ent->cache_info != 0) {
		    status =
			tls_scache_update(ent->cache_info, STR(cache_id),
					  STR(buffer), LEN(buffer)) ?
			TLS_MGR_STAT_OK : TLS_MGR_STAT_ERR;
		}
	    }
	    attr_print(client_stream, ATTR_FLAG_NONE,
		       ATTR_TYPE_INT, MAIL_ATTR_STATUS, status,
		       ATTR_TYPE_END);
	}

	/*
	 * Delete session from cache.
	 */
	else if (STREQ(STR(request), TLS_MGR_REQ_DELETE)) {
	    if (attr_scan(client_stream, ATTR_FLAG_STRICT,
			  ATTR_TYPE_STR, TLS_MGR_ATTR_CACHE_TYPE, cache_type,
			  ATTR_TYPE_STR, TLS_MGR_ATTR_CACHE_ID, cache_id,
			  ATTR_TYPE_END) == 2) {
		for (ent = cache_table; ent->cache_label; ++ent)
		    if (strcmp(ent->cache_label, STR(cache_type)) == 0)
			break;
		if (ent->cache_label == 0) {
		    msg_warn("bogus cache type \"%s\" in \"%s\" request",
			     STR(cache_type), TLS_MGR_REQ_DELETE);
		} else if (ent->cache_info != 0) {
		    status = tls_scache_delete(ent->cache_info,
					       STR(cache_id)) ?
			TLS_MGR_STAT_OK : TLS_MGR_STAT_ERR;
		}
	    }
	    attr_print(client_stream, ATTR_FLAG_NONE,
		       ATTR_TYPE_INT, MAIL_ATTR_STATUS, status,
		       ATTR_TYPE_END);
	}

	/*
	 * Entropy request.
	 */
	else if (STREQ(STR(request), TLS_MGR_REQ_SEED)) {
	    if (attr_scan(client_stream, ATTR_FLAG_STRICT,
			  ATTR_TYPE_INT, TLS_MGR_ATTR_SIZE, &len,
			  ATTR_TYPE_END) == 1) {
		VSTRING_RESET(buffer);
		if (len <= 0 || len > 255) {
		    msg_warn("bogus seed length \"%d\" in \"%s\" request",
			     len, TLS_MGR_REQ_SEED);
		} else {
		    VSTRING_SPACE(buffer, len);
		    RAND_bytes((unsigned char *) STR(buffer), len);
		    VSTRING_AT_OFFSET(buffer, len);	/* XXX not part of the
							 * official interface */
		    status = TLS_MGR_STAT_OK;
		}
	    }
	    attr_print(client_stream, ATTR_FLAG_NONE,
		       ATTR_TYPE_INT, MAIL_ATTR_STATUS, status,
		       ATTR_TYPE_DATA, TLS_MGR_ATTR_SEED,
		       LEN(buffer), STR(buffer),
		       ATTR_TYPE_END);
	}

	/*
	 * Caching policy request.
	 */
	else if (STREQ(STR(request), TLS_MGR_REQ_POLICY)) {
	    int     cachable = 0;

	    if (attr_scan(client_stream, ATTR_FLAG_STRICT,
			  ATTR_TYPE_STR, TLS_MGR_ATTR_CACHE_TYPE, cache_type,
			  ATTR_TYPE_END) == 1) {
		for (ent = cache_table; ent->cache_label; ++ent)
		    if (strcmp(ent->cache_label, STR(cache_type)) == 0)
			break;
		if (ent->cache_label == 0) {
		    msg_warn("bogus cache type \"%s\" in \"%s\" request",
			     STR(cache_type), TLS_MGR_REQ_POLICY);
		} else {
		    cachable = (ent->cache_info != 0) ? 1 : 0;
		    status = TLS_MGR_STAT_OK;
		}
	    }
	    attr_print(client_stream, ATTR_FLAG_NONE,
		       ATTR_TYPE_INT, MAIL_ATTR_STATUS, status,
		       ATTR_TYPE_INT, TLS_MGR_ATTR_CACHABLE, cachable,
		       ATTR_TYPE_END);
	}

	/*
	 * Master trigger. Normally, these triggers arrive only after some
	 * other process requested the tlsmgr's service. The purpose is to
	 * restart the tlsmgr after it aborted due to a fatal run-time error,
	 * so that it can continue its housekeeping even while nothing is
	 * using TLS.
	 * 
	 * XXX Which begs the question, if TLS isn't used often, do we need a
	 * tlsmgr background process? It could terminate when the session
	 * caches are empty.
	 */
	else if (STREQ(STR(request), wakeup)) {
	    if (msg_verbose)
		msg_info("received master trigger");
	    multi_server_disconnect(client_stream);
	    return;				/* NOT: vstream_fflush */
	}
    }

    /*
     * Protocol error.
     */
    else {
	attr_print(client_stream, ATTR_FLAG_NONE,
		   ATTR_TYPE_INT, MAIL_ATTR_STATUS, TLS_MGR_STAT_FAIL,
		   ATTR_TYPE_END);
    }
    vstream_fflush(client_stream);
}

/* tlsmgr_pre_init - pre-jail initialization */

static void tlsmgr_pre_init(char *unused_name, char **unused_argv)
{
    char   *path;
    struct timeval tv;
    TLSMGR_SCACHE *ent;
    VSTRING *redirect;
    HTABLE *dup_filter;
    const char *dup_label;

    /*
     * If nothing else works then at least this will get us a few bits of
     * entropy.
     * 
     * XXX This is our first call into the OpenSSL library. We should find out
     * if this can be moved to the post-jail initialization phase, without
     * breaking compatibility with existing installations.
     */
    GETTIMEOFDAY(&tv);
    tv.tv_sec ^= getpid();
    RAND_seed(&tv, sizeof(struct timeval));

    /*
     * Open the external entropy source. We will not be able to open it again
     * after we are sent to chroot jail, so we keep it open. Errors are not
     * fatal. The exchange file (see below) is the only entropy source that
     * really matters in the long run.
     * 
     * Security note: we open the entropy source while privileged, but we don't
     * access the source until after we release privileges. This way, none of
     * the OpenSSL code gets to execute while we are privileged.
     */
    if (*var_tls_rand_source) {

	/*
	 * Source is a random device.
	 */
	if (!strncmp(var_tls_rand_source, DEV_PREF, DEV_PREF_LEN)) {
	    path = DEV_PATH(var_tls_rand_source);
	    rand_source_dev = tls_prng_dev_open(path, TLS_MGR_TIMEOUT);
	    if (rand_source_dev == 0)
		msg_warn("cannot open entropy device %s: %m", path);
	}

	/*
	 * Source is an EGD compatible socket.
	 */
	else if (!strncmp(var_tls_rand_source, EGD_PREF, EGD_PREF_LEN)) {
	    path = EGD_PATH(var_tls_rand_source);
	    rand_source_egd = tls_prng_egd_open(path, TLS_MGR_TIMEOUT);
	    if (rand_source_egd == 0)
		msg_warn("cannot connect to EGD server %s: %m", path);
	}

	/*
	 * Source is regular file. We read this only once.
	 */
	else {
	    rand_source_file =
		tls_prng_file_open(var_tls_rand_source, TLS_MGR_TIMEOUT);
	}
    } else {
	msg_warn("no entropy source specified with parameter %s",
		 VAR_TLS_RAND_SOURCE);
	msg_warn("encryption keys etc. may be predictable");
    }

    /*
     * Security: don't create root-owned files that contain untrusted data.
     * And don't create Postfix-owned files in root-owned directories,
     * either. We want a correct relationship between (file/directory)
     * ownership and (file/directory) content.
     */
    SAVE_AND_SET_EUGID(var_owner_uid, var_owner_gid);
    redirect = vstring_alloc(100);

    /*
     * Open the PRNG exchange file before going to jail, but don't use root
     * privileges. Start the exchange file read/update pseudo thread after
     * dropping privileges.
     */
    if (*var_tls_rand_exch_name) {
	rand_exch =
	    tls_prng_exch_open(data_redirect_file(redirect,
						  var_tls_rand_exch_name));
	if (rand_exch == 0)
	    msg_fatal("cannot open PRNG exchange file %s: %m",
		      var_tls_rand_exch_name);
    }

    /*
     * Open the session cache files and discard old information before going
     * to jail, but don't use root privilege. Start the cache maintenance
     * pseudo threads after dropping privileges.
     */
    dup_filter = htable_create(sizeof(cache_table) / sizeof(cache_table[0]));
    for (ent = cache_table; ent->cache_label; ++ent) {
	if (**ent->cache_db) {
	    if ((dup_label = htable_find(dup_filter, *ent->cache_db)) != 0)
		msg_fatal("do not use the same TLS cache file %s for %s and %s",
			  *ent->cache_db, dup_label, ent->cache_label);
	    htable_enter(dup_filter, *ent->cache_db, ent->cache_label);
	    ent->cache_info =
		tls_scache_open(data_redirect_map(redirect, *ent->cache_db),
				ent->cache_label,
				*ent->cache_loglevel >= 2,
				*ent->cache_timeout);
	}
    }
    htable_free(dup_filter, (void (*) (char *)) 0);

    /*
     * Clean up and restore privilege.
     */
    vstring_free(redirect);
    RESTORE_SAVED_EUGID();
}

/* tlsmgr_post_init - post-jail initialization */

static void tlsmgr_post_init(char *unused_name, char **unused_argv)
{
    TLSMGR_SCACHE *ent;

#define NULL_EVENT	(0)
#define NULL_CONTEXT	((char *) 0)

    /*
     * This routine runs after the skeleton code has entered the chroot jail,
     * but before any client requests are serviced. Prevent automatic process
     * suicide after a limited number of client requests or after a limited
     * amount of idle time.
     */
    var_use_limit = 0;
    var_idle_limit = 0;

    /*
     * Start the internal PRNG re-seeding pseudo thread first.
     */
    if (*var_tls_rand_source) {
	if (var_tls_reseed_period > INT_MAX / UCHAR_MAX)
	    var_tls_reseed_period = INT_MAX / UCHAR_MAX;
	tlsmgr_reseed_event(NULL_EVENT, NULL_CONTEXT);
    }

    /*
     * Start the exchange file read/update pseudo thread.
     */
    if (*var_tls_rand_exch_name) {
	if (var_tls_prng_exch_period > INT_MAX / UCHAR_MAX)
	    var_tls_prng_exch_period = INT_MAX / UCHAR_MAX;
	tlsmgr_prng_exch_event(NULL_EVENT, NULL_CONTEXT);
    }

    /*
     * Start the cache maintenance pseudo threads last. Strictly speaking
     * there is nothing to clean up after we truncate the database to zero
     * length, but early cleanup makes verbose logging more informative (we
     * get positive confirmation that the cleanup threads are running).
     */
    for (ent = cache_table; ent->cache_label; ++ent)
	if (ent->cache_info)
	    tlsmgr_cache_run_event(NULL_EVENT, (char *) ent);
}

/* tlsmgr_before_exit - save PRNG state before exit */

static void tlsmgr_before_exit(char *unused_service_name, char **unused_argv)
{

    /*
     * Save state before we exit after "postfix reload".
     */
    if (rand_exch)
	tls_prng_exch_update(rand_exch);
}

MAIL_VERSION_STAMP_DECLARE;

/* main - the main program */

int     main(int argc, char **argv)
{
    static const CONFIG_STR_TABLE str_table[] = {
	VAR_TLS_RAND_SOURCE, DEF_TLS_RAND_SOURCE, &var_tls_rand_source, 0, 0,
	VAR_TLS_RAND_EXCH_NAME, DEF_TLS_RAND_EXCH_NAME, &var_tls_rand_exch_name, 0, 0,
	VAR_SMTPD_TLS_SCACHE_DB, DEF_SMTPD_TLS_SCACHE_DB, &var_smtpd_tls_scache_db, 0, 0,
	VAR_SMTP_TLS_SCACHE_DB, DEF_SMTP_TLS_SCACHE_DB, &var_smtp_tls_scache_db, 0, 0,
	VAR_LMTP_TLS_SCACHE_DB, DEF_LMTP_TLS_SCACHE_DB, &var_lmtp_tls_scache_db, 0, 0,
	0,
    };
    static const CONFIG_TIME_TABLE time_table[] = {
	VAR_TLS_RESEED_PERIOD, DEF_TLS_RESEED_PERIOD, &var_tls_reseed_period, 1, 0,
	VAR_TLS_PRNG_UPD_PERIOD, DEF_TLS_PRNG_UPD_PERIOD, &var_tls_prng_exch_period, 1, 0,
	VAR_SMTPD_TLS_SCACHTIME, DEF_SMTPD_TLS_SCACHTIME, &var_smtpd_tls_scache_timeout, 0, 0,
	VAR_SMTP_TLS_SCACHTIME, DEF_SMTP_TLS_SCACHTIME, &var_smtp_tls_scache_timeout, 0, 0,
	VAR_LMTP_TLS_SCACHTIME, DEF_LMTP_TLS_SCACHTIME, &var_lmtp_tls_scache_timeout, 0, 0,
	0,
    };
    static const CONFIG_INT_TABLE int_table[] = {
	VAR_TLS_RAND_BYTES, DEF_TLS_RAND_BYTES, &var_tls_rand_bytes, 1, 0,
	VAR_SMTPD_TLS_LOGLEVEL, DEF_SMTPD_TLS_LOGLEVEL, &var_smtpd_tls_loglevel, 0, 0,
	VAR_SMTP_TLS_LOGLEVEL, DEF_SMTP_TLS_LOGLEVEL, &var_smtp_tls_loglevel, 0, 0,
	VAR_LMTP_TLS_LOGLEVEL, DEF_LMTP_TLS_LOGLEVEL, &var_lmtp_tls_loglevel, 0, 0,
	0,
    };

    /*
     * Fingerprint executables and core dumps.
     */
    MAIL_VERSION_STAMP_ALLOCATE;

    /*
     * Use the multi service skeleton, and require that no-one else is
     * monitoring our service port while this process runs.
     */
    multi_server_main(argc, argv, tlsmgr_service,
		      MAIL_SERVER_TIME_TABLE, time_table,
		      MAIL_SERVER_INT_TABLE, int_table,
		      MAIL_SERVER_STR_TABLE, str_table,
		      MAIL_SERVER_PRE_INIT, tlsmgr_pre_init,
		      MAIL_SERVER_POST_INIT, tlsmgr_post_init,
		      MAIL_SERVER_EXIT, tlsmgr_before_exit,
		      MAIL_SERVER_LOOP, tlsmgr_loop,
		      MAIL_SERVER_SOLITARY,
		      0);
}

#else

/* tlsmgr_service - respond to external trigger(s), non-TLS version */

static void tlsmgr_service(VSTREAM *unused_stream, char *unused_service,
			           char **unused_argv)
{
    msg_info("TLS support is not compiled in -- exiting");
}

/* main - the main program, non-TLS version */

int     main(int argc, char **argv)
{

    /*
     * 200411 We can't simply use msg_fatal() here, because the logging
     * hasn't been initialized. The text would disappear because stderr is
     * redirected to /dev/null.
     * 
     * We invoke multi_server_main() to complete program initialization
     * (including logging) and then invoke the tlsmgr_service() routine to
     * log the message that says why this program will not run.
     */
    multi_server_main(argc, argv, tlsmgr_service,
		      0);
}

#endif
