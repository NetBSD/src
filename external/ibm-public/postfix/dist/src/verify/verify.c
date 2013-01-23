/*	$NetBSD: verify.c,v 1.1.1.3.4.1 2013/01/23 00:05:17 yamt Exp $	*/

/*++
/* NAME
/*	verify 8
/* SUMMARY
/*	Postfix address verification server
/* SYNOPSIS
/*	\fBverify\fR [generic Postfix daemon options]
/* DESCRIPTION
/*	The \fBverify\fR(8) address verification server maintains a record
/*	of what recipient addresses are known to be deliverable or
/*	undeliverable.
/*
/*	Addresses are verified by injecting probe messages into the
/*	Postfix queue. Probe messages are run through all the routing
/*	and rewriting machinery except for final delivery, and are
/*	discarded rather than being deferred or bounced.
/*
/*	Address verification relies on the answer from the nearest
/*	MTA for the specified address, and will therefore not detect
/*	all undeliverable addresses.
/*
/*	The \fBverify\fR(8) server is designed to run under control
/*	by the Postfix
/*	master server. It maintains an optional persistent database.
/*	To avoid being interrupted by "postfix stop" in the middle
/*	of a database update, the process runs in a separate process
/*	group.
/*
/*	The \fBverify\fR(8) server implements the following requests:
/* .IP "\fBupdate\fI address status text\fR"
/*	Update the status and text of the specified address.
/* .IP "\fBquery\fI address\fR"
/*	Look up the \fIstatus\fR and \fItext\fR for the specified
/*	\fIaddress\fR.
/*	If the status is unknown, a probe is sent and an "in progress"
/*	status is returned.
/* SECURITY
/* .ad
/* .fi
/*	The address verification server is not security-sensitive. It does
/*	not talk to the network, and it does not talk to local users.
/*	The verify server can run chrooted at fixed low privilege.
/*
/*	The address verification server can be coerced to store
/*	unlimited amounts of garbage. Limiting the cache expiry
/*	time
/*	trades one problem (disk space exhaustion) for another
/*	one (poor response time to client requests).
/*
/*	With Postfix version 2.5 and later, the \fBverify\fR(8)
/*	server no longer uses root privileges when opening the
/*	\fBaddress_verify_map\fR cache file. The file should now
/*	be stored under the Postfix-owned \fBdata_directory\fR.  As
/*	a migration aid, an attempt to open a cache file under a
/*	non-Postfix directory is redirected to the Postfix-owned
/*	\fBdata_directory\fR, and a warning is logged.
/* DIAGNOSTICS
/*	Problems and transactions are logged to \fBsyslogd\fR(8).
/* BUGS
/*	Address verification probe messages add additional traffic
/*	to the mail queue.
/*	Recipient verification may cause an increased load on
/*	down-stream servers in the case of a dictionary attack or
/*	a flood of backscatter bounces.
/*	Sender address verification may cause your site to be
/*	blacklisted by some providers.
/*
/*	If the persistent database ever gets corrupted then the world
/*	comes to an end and human intervention is needed. This violates
/*	a basic Postfix principle.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	Changes to \fBmain.cf\fR are not picked up automatically,
/*	as \fBverify\fR(8)
/*	processes are long-lived. Use the command "\fBpostfix reload\fR" after
/*	a configuration change.
/*
/*	The text below provides only a parameter summary. See
/*	\fBpostconf\fR(5) for more details including examples.
/* PROBE MESSAGE CONTROLS
/* .ad
/* .fi
/* .IP "\fBaddress_verify_sender ($double_bounce_sender)\fR"
/*	The sender address to use in address verification probes; prior
/*	to Postfix 2.5 the default was "postmaster".
/* .PP
/*	Available with Postfix 2.9 and later:
/* .IP "\fBaddress_verify_sender_ttl (0s)\fR"
/*	The time between changes in the time-dependent portion of address
/*	verification probe sender addresses.
/* CACHE CONTROLS
/* .ad
/* .fi
/* .IP "\fBaddress_verify_map (see 'postconf -d' output)\fR"
/*	Lookup table for persistent address verification status
/*	storage.
/* .IP "\fBaddress_verify_positive_expire_time (31d)\fR"
/*	The time after which a successful probe expires from the address
/*	verification cache.
/* .IP "\fBaddress_verify_positive_refresh_time (7d)\fR"
/*	The time after which a successful address verification probe needs
/*	to be refreshed.
/* .IP "\fBaddress_verify_negative_cache (yes)\fR"
/*	Enable caching of failed address verification probe results.
/* .IP "\fBaddress_verify_negative_expire_time (3d)\fR"
/*	The time after which a failed probe expires from the address
/*	verification cache.
/* .IP "\fBaddress_verify_negative_refresh_time (3h)\fR"
/*	The time after which a failed address verification probe needs to
/*	be refreshed.
/* .PP
/*	Available with Postfix 2.7 and later:
/* .IP "\fBaddress_verify_cache_cleanup_interval (12h)\fR"
/*	The amount of time between \fBverify\fR(8) address verification
/*	database cleanup runs.
/* PROBE MESSAGE ROUTING CONTROLS
/* .ad
/* .fi
/*	By default, probe messages are delivered via the same route
/*	as regular messages.  The following parameters can be used to
/*	override specific message routing mechanisms.
/* .IP "\fBaddress_verify_relayhost ($relayhost)\fR"
/*	Overrides the relayhost parameter setting for address verification
/*	probes.
/* .IP "\fBaddress_verify_transport_maps ($transport_maps)\fR"
/*	Overrides the transport_maps parameter setting for address verification
/*	probes.
/* .IP "\fBaddress_verify_local_transport ($local_transport)\fR"
/*	Overrides the local_transport parameter setting for address
/*	verification probes.
/* .IP "\fBaddress_verify_virtual_transport ($virtual_transport)\fR"
/*	Overrides the virtual_transport parameter setting for address
/*	verification probes.
/* .IP "\fBaddress_verify_relay_transport ($relay_transport)\fR"
/*	Overrides the relay_transport parameter setting for address
/*	verification probes.
/* .IP "\fBaddress_verify_default_transport ($default_transport)\fR"
/*	Overrides the default_transport parameter setting for address
/*	verification probes.
/* .PP
/*	Available in Postfix 2.3 and later:
/* .IP "\fBaddress_verify_sender_dependent_relayhost_maps ($sender_dependent_relayhost_maps)\fR"
/*	Overrides the sender_dependent_relayhost_maps parameter setting for address
/*	verification probes.
/* .PP
/*	Available in Postfix 2.7 and later:
/* .IP "\fBaddress_verify_sender_dependent_default_transport_maps ($sender_dependent_default_transport_maps)\fR"
/*	Overrides the sender_dependent_default_transport_maps parameter
/*	setting for address verification probes.
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
/* .IP "\fBprocess_id (read-only)\fR"
/*	The process ID of a Postfix command or daemon process.
/* .IP "\fBprocess_name (read-only)\fR"
/*	The process name of a Postfix command or daemon process.
/* .IP "\fBqueue_directory (see 'postconf -d' output)\fR"
/*	The location of the Postfix top-level queue directory.
/* .IP "\fBsyslog_facility (mail)\fR"
/*	The syslog facility of Postfix logging.
/* .IP "\fBsyslog_name (see 'postconf -d' output)\fR"
/*	The mail system name that is prepended to the process name in syslog
/*	records, so that "smtpd" becomes, for example, "postfix/smtpd".
/* SEE ALSO
/*	smtpd(8), Postfix SMTP server
/*	cleanup(8), enqueue Postfix message
/*	postconf(5), configuration parameters
/*	syslogd(5), system logging
/* README FILES
/* .ad
/* .fi
/*	Use "\fBpostconf readme_directory\fR" or
/*	"\fBpostconf html_directory\fR" to locate this information.
/* .na
/* .nf
/*	ADDRESS_VERIFICATION_README, address verification howto
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* HISTORY
/* .ad
/* .fi
/*	This service was introduced with Postfix version 2.1.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <htable.h>
#include <dict_ht.h>
#include <dict_cache.h>
#include <split_at.h>
#include <stringops.h>
#include <set_eugid.h>
#include <events.h>

/* Global library. */

#include <mail_conf.h>
#include <mail_params.h>
#include <mail_version.h>
#include <mail_proto.h>
#include <post_mail.h>
#include <data_redirect.h>
#include <verify_clnt.h>
#include <verify_sender_addr.h>

/* Server skeleton. */

#include <mail_server.h>

/* Application-specific. */

 /*
  * Tunable parameters.
  */
char   *var_verify_map;
int     var_verify_pos_exp;
int     var_verify_pos_try;
int     var_verify_neg_exp;
int     var_verify_neg_try;
int     var_verify_scan_cache;

 /*
  * State.
  */
static DICT_CACHE *verify_map;

 /*
  * Silly little macros.
  */
#define STR(x)			vstring_str(x)
#define STREQ(x,y)		(strcmp(x,y) == 0)

 /*
  * The address verification database consists of (address, data) tuples. The
  * format of the data field is "status:probed:updated:text". The meaning of
  * each field is:
  * 
  * status: one of the four recipient status codes (OK, DEFER, BOUNCE or TODO).
  * In the case of TODO, we have no information about the address, and the
  * address is being probed.
  * 
  * probed: if non-zero, the time the currently outstanding address probe was
  * sent. If zero, there is no outstanding address probe.
  * 
  * updated: if non-zero, the time the address probe result was received. If
  * zero, we have no information about the address, and the address is being
  * probed.
  * 
  * text: descriptive text from delivery agents etc.
  */

 /*
  * Quick test to see status without parsing the whole entry.
  */
#define STATUS_FROM_RAW_ENTRY(e) atoi(e)

/* verify_make_entry - construct table entry */

static void verify_make_entry(VSTRING *buf, int status, long probed,
			              long updated, const char *text)
{
    vstring_sprintf(buf, "%d:%ld:%ld:%s", status, probed, updated, text);
}

/* verify_parse_entry - parse table entry */

static int verify_parse_entry(char *buf, int *status, long *probed,
			              long *updated, char **text)
{
    char   *probed_text;
    char   *updated_text;

    if ((probed_text = split_at(buf, ':')) != 0
	&& (updated_text = split_at(probed_text, ':')) != 0
	&& (*text = split_at(updated_text, ':')) != 0
	&& alldig(buf)
	&& alldig(probed_text)
	&& alldig(updated_text)) {
	*probed = atol(probed_text);
	*updated = atol(updated_text);
	*status = atoi(buf);

	/*
	 * Coverity 200604: the code incorrectly tested (probed || updated),
	 * so that the sanity check never detected all-zero time stamps. Such
	 * records are never written. If we read a record with all-zero time
	 * stamps, then something is badly broken.
	 */
	if ((*status == DEL_RCPT_STAT_OK
	     || *status == DEL_RCPT_STAT_DEFER
	     || *status == DEL_RCPT_STAT_BOUNCE
	     || *status == DEL_RCPT_STAT_TODO)
	    && (*probed || *updated))
	    return (0);
    }
    msg_warn("bad address verify table entry: %.100s", buf);
    return (-1);
}

/* verify_stat2name - status to name */

static const char *verify_stat2name(int addr_status)
{
    if (addr_status == DEL_RCPT_STAT_OK)
	return ("deliverable");
    if (addr_status == DEL_RCPT_STAT_DEFER)
	return ("undeliverable");
    if (addr_status == DEL_RCPT_STAT_BOUNCE)
	return ("undeliverable");
    return (0);
}

/* verify_update_service - update address service */

static void verify_update_service(VSTREAM *client_stream)
{
    VSTRING *buf = vstring_alloc(10);
    VSTRING *addr = vstring_alloc(10);
    int     addr_status;
    VSTRING *text = vstring_alloc(10);
    const char *status_name;
    const char *raw_data;
    long    probed;
    long    updated;

    if (attr_scan(client_stream, ATTR_FLAG_STRICT,
		  ATTR_TYPE_STR, MAIL_ATTR_ADDR, addr,
		  ATTR_TYPE_INT, MAIL_ATTR_ADDR_STATUS, &addr_status,
		  ATTR_TYPE_STR, MAIL_ATTR_WHY, text,
		  ATTR_TYPE_END) == 3) {
	/* FIX 200501 IPv6 patch did not neuter ":" in address literals. */
	translit(STR(addr), ":", "_");
	if ((status_name = verify_stat2name(addr_status)) == 0) {
	    msg_warn("bad recipient status %d for recipient %s",
		     addr_status, STR(addr));
	    attr_print(client_stream, ATTR_FLAG_NONE,
		       ATTR_TYPE_INT, MAIL_ATTR_STATUS, VRFY_STAT_BAD,
		       ATTR_TYPE_END);
	} else {

	    /*
	     * Robustness: don't allow a failed probe to clobber an OK
	     * address before it expires. The failed probe is ignored so that
	     * the address will be re-probed upon the next query. As long as
	     * some probes succeed the address will remain cached as OK.
	     */
	    if (addr_status == DEL_RCPT_STAT_OK
		|| (raw_data = dict_cache_lookup(verify_map, STR(addr))) == 0
		|| STATUS_FROM_RAW_ENTRY(raw_data) != DEL_RCPT_STAT_OK) {
		probed = 0;
		updated = (long) time((time_t *) 0);
		verify_make_entry(buf, addr_status, probed, updated, STR(text));
		if (msg_verbose)
		    msg_info("PUT %s status=%d probed=%ld updated=%ld text=%s",
			STR(addr), addr_status, probed, updated, STR(text));
		dict_cache_update(verify_map, STR(addr), STR(buf));
	    }
	    attr_print(client_stream, ATTR_FLAG_NONE,
		       ATTR_TYPE_INT, MAIL_ATTR_STATUS, VRFY_STAT_OK,
		       ATTR_TYPE_END);
	}
    }
    vstring_free(buf);
    vstring_free(addr);
    vstring_free(text);
}

/* verify_post_mail_action - callback */

static void verify_post_mail_action(VSTREAM *stream, void *unused_context)
{

    /*
     * Probe messages need no body content, because they are never delivered,
     * deferred, or bounced.
     */
    if (stream != 0)
	post_mail_fclose(stream);
}

/* verify_query_service - query address status */

static void verify_query_service(VSTREAM *client_stream)
{
    VSTRING *addr = vstring_alloc(10);
    VSTRING *get_buf = 0;
    VSTRING *put_buf = 0;
    const char *raw_data;
    int     addr_status;
    long    probed;
    long    updated;
    char   *text;

    if (attr_scan(client_stream, ATTR_FLAG_STRICT,
		  ATTR_TYPE_STR, MAIL_ATTR_ADDR, addr,
		  ATTR_TYPE_END) == 1) {
	long    now = (long) time((time_t *) 0);

	/*
	 * Produce a default record when no usable record exists.
	 * 
	 * If negative caching is disabled, purge an expired record from the
	 * database.
	 * 
	 * XXX Assume that a probe is lost if no response is received in 1000
	 * seconds. If this number is too small the queue will slowly fill up
	 * with delayed probes.
	 * 
	 * XXX Maintain a moving average for the probe turnaround time, and
	 * allow probe "retransmission" when a probe is outstanding for, say
	 * some minimal amount of time (1000 sec) plus several times the
	 * observed probe turnaround time. This causes probing to back off
	 * when the mail system becomes congested.
	 */
#define POSITIVE_ENTRY_EXPIRED(addr_status, updated) \
    (addr_status == DEL_RCPT_STAT_OK && updated + var_verify_pos_exp < now)
#define NEGATIVE_ENTRY_EXPIRED(addr_status, updated) \
    (addr_status != DEL_RCPT_STAT_OK && updated + var_verify_neg_exp < now)
#define PROBE_TTL	1000

	/* FIX 200501 IPv6 patch did not neuter ":" in address literals. */
	translit(STR(addr), ":", "_");
	if ((raw_data = dict_cache_lookup(verify_map, STR(addr))) == 0	/* not found */
	    || ((get_buf = vstring_alloc(10)),
		vstring_strcpy(get_buf, raw_data),	/* malformed */
		verify_parse_entry(STR(get_buf), &addr_status, &probed,
				   &updated, &text) < 0)
	    || (now - probed > PROBE_TTL	/* safe to probe */
		&& (POSITIVE_ENTRY_EXPIRED(addr_status, updated)
		    || NEGATIVE_ENTRY_EXPIRED(addr_status, updated)))) {
	    addr_status = DEL_RCPT_STAT_TODO;
	    probed = 0;
	    updated = 0;
	    text = "Address verification in progress";
	    if (raw_data != 0 && var_verify_neg_cache == 0)
		dict_cache_delete(verify_map, STR(addr));
	}
	if (msg_verbose)
	    msg_info("GOT %s status=%d probed=%ld updated=%ld text=%s",
		     STR(addr), addr_status, probed, updated, text);

	/*
	 * Respond to the client.
	 */
	attr_print(client_stream, ATTR_FLAG_NONE,
		   ATTR_TYPE_INT, MAIL_ATTR_STATUS, VRFY_STAT_OK,
		   ATTR_TYPE_INT, MAIL_ATTR_ADDR_STATUS, addr_status,
		   ATTR_TYPE_STR, MAIL_ATTR_WHY, text,
		   ATTR_TYPE_END);

	/*
	 * Send a new probe when the information needs to be refreshed.
	 * 
	 * XXX For an initial proof of concept implementation, use synchronous
	 * mail submission. This needs to be made async for high-volume
	 * sites, which makes it even more interesting to eliminate duplicate
	 * queries while a probe is being built.
	 * 
	 * If negative caching is turned off, update the database only when
	 * refreshing an existing entry.
	 */
#define POSITIVE_REFRESH_NEEDED(addr_status, updated) \
    (addr_status == DEL_RCPT_STAT_OK && updated + var_verify_pos_try < now)
#define NEGATIVE_REFRESH_NEEDED(addr_status, updated) \
    (addr_status != DEL_RCPT_STAT_OK && updated + var_verify_neg_try < now)

	if (now - probed > PROBE_TTL
	    && (POSITIVE_REFRESH_NEEDED(addr_status, updated)
		|| NEGATIVE_REFRESH_NEEDED(addr_status, updated))) {
	    if (msg_verbose)
		msg_info("PROBE %s status=%d probed=%ld updated=%ld",
			 STR(addr), addr_status, now, updated);
	    post_mail_fopen_async(make_verify_sender_addr(), STR(addr),
				  INT_FILT_MASK_NONE,
				  DEL_REQ_FLAG_MTA_VRFY,
				  (VSTRING *) 0,
				  verify_post_mail_action,
				  (void *) 0);
	    if (updated != 0 || var_verify_neg_cache != 0) {
		put_buf = vstring_alloc(10);
		verify_make_entry(put_buf, addr_status, now, updated, text);
		if (msg_verbose)
		    msg_info("PUT %s status=%d probed=%ld updated=%ld text=%s",
			     STR(addr), addr_status, now, updated, text);
		dict_cache_update(verify_map, STR(addr), STR(put_buf));
	    }
	}
    }
    vstring_free(addr);
    if (get_buf)
	vstring_free(get_buf);
    if (put_buf)
	vstring_free(put_buf);
}

/* verify_cache_validator - cache cleanup validator */

static int verify_cache_validator(const char *addr, const char *raw_data,
			            char *context)
{
    VSTRING *get_buf = (VSTRING *) context;
    int     addr_status;
    long    probed;
    long    updated;
    char   *text;
    long    now = (long) event_time();

#define POS_OR_NEG_ENTRY_EXPIRED(stat, stamp) \
	(POSITIVE_ENTRY_EXPIRED((stat), (stamp)) \
	    || NEGATIVE_ENTRY_EXPIRED((stat), (stamp)))

    vstring_strcpy(get_buf, raw_data);
    return (verify_parse_entry(STR(get_buf), &addr_status,	/* syntax OK */
			       &probed, &updated, &text) == 0
	    && (now - probed < PROBE_TTL	/* probe in progress */
		|| !POS_OR_NEG_ENTRY_EXPIRED(addr_status, updated)));
}

/* verify_service - perform service for client */

static void verify_service(VSTREAM *client_stream, char *unused_service,
			           char **argv)
{
    VSTRING *request = vstring_alloc(10);

    /*
     * Sanity check. This service takes no command-line arguments.
     */
    if (argv[0])
	msg_fatal("unexpected command-line argument: %s", argv[0]);

    /*
     * This routine runs whenever a client connects to the socket dedicated
     * to the address verification service. All connection-management stuff
     * is handled by the common code in multi_server.c.
     */
    if (attr_scan(client_stream,
		  ATTR_FLAG_MORE | ATTR_FLAG_STRICT,
		  ATTR_TYPE_STR, MAIL_ATTR_REQ, request,
		  ATTR_TYPE_END) == 1) {
	if (STREQ(STR(request), VRFY_REQ_UPDATE)) {
	    verify_update_service(client_stream);
	} else if (STREQ(STR(request), VRFY_REQ_QUERY)) {
	    verify_query_service(client_stream);
	} else {
	    msg_warn("unrecognized request: \"%s\", ignored", STR(request));
	    attr_print(client_stream, ATTR_FLAG_NONE,
		       ATTR_TYPE_INT, MAIL_ATTR_STATUS, VRFY_STAT_BAD,
		       ATTR_TYPE_END);
	}
    }
    vstream_fflush(client_stream);
    vstring_free(request);
}

/* verify_dump - dump some statistics */

static void verify_dump(void)
{

    /*
     * Dump preliminary cache cleanup statistics when the process commits
     * suicide while a cache cleanup run is in progress. We can't currently
     * distinguish between "postfix reload" (we should restart) or "maximal
     * idle time reached" (we could finish the cache cleanup first).
     */
    dict_cache_close(verify_map);
    verify_map = 0;
}

/* post_jail_init - post-jail initialization */

static void post_jail_init(char *unused_name, char **unused_argv)
{

    /*
     * If the database is in volatile memory only, prevent automatic process
     * suicide after a limited number of client requests or after a limited
     * amount of idle time.
     */
    if (*var_verify_map == 0) {
	var_use_limit = 0;
	var_idle_limit = 0;
    }

    /*
     * Start the cache cleanup thread.
     */
    if (var_verify_scan_cache > 0) {
	int     cache_flags;

	cache_flags = DICT_CACHE_FLAG_STATISTICS;
	if (msg_verbose)
	    cache_flags |= DICT_CACHE_FLAG_VERBOSE;
	dict_cache_control(verify_map,
			   DICT_CACHE_CTL_FLAGS, cache_flags,
			   DICT_CACHE_CTL_INTERVAL, var_verify_scan_cache,
			   DICT_CACHE_CTL_VALIDATOR, verify_cache_validator,
			DICT_CACHE_CTL_CONTEXT, (char *) vstring_alloc(100),
			   DICT_CACHE_CTL_END);
    }
}

/* pre_jail_init - pre-jail initialization */

static void pre_jail_init(char *unused_name, char **unused_argv)
{
    mode_t  saved_mask;
    VSTRING *redirect;

    /*
     * Never, ever, get killed by a master signal, as that would corrupt the
     * database when we're in the middle of an update.
     */
    setsid();

    /*
     * Security: don't create root-owned files that contain untrusted data.
     * And don't create Postfix-owned files in root-owned directories,
     * either. We want a correct relationship between (file/directory)
     * ownership and (file/directory) content.
     * 
     * XXX Non-root open can violate the principle of least surprise: Postfix
     * can't open an *SQL config file for database read-write access, even
     * though it can open that same control file for database read-only
     * access.
     * 
     * The solution is to query a map type and obtain its properties before
     * opening it. A clean solution is to add a dict_info() API that is
     * similar to dict_open() except it returns properties (dict flags) only.
     * A pragmatic solution is to overload the existing API and have
     * dict_open() return a dummy map when given a null map name.
     * 
     * However, the proxymap daemon has been opening *SQL maps as non-root for
     * years now without anyone complaining, let's not solve a problem that
     * doesn't exist.
     */
    SAVE_AND_SET_EUGID(var_owner_uid, var_owner_gid);
    redirect = vstring_alloc(100);

    /*
     * Keep state in persistent (external) or volatile (internal) map.
     * 
     * Start the cache cleanup thread after permanently dropping privileges.
     */
#define VERIFY_DICT_OPEN_FLAGS (DICT_FLAG_DUP_REPLACE | DICT_FLAG_SYNC_UPDATE \
	    | DICT_FLAG_OPEN_LOCK)

    saved_mask = umask(022);
    verify_map =
	dict_cache_open(*var_verify_map ?
			data_redirect_map(redirect, var_verify_map) :
			"internal:verify",
			O_CREAT | O_RDWR, VERIFY_DICT_OPEN_FLAGS);
    (void) umask(saved_mask);

    /*
     * Clean up and restore privilege.
     */
    vstring_free(redirect);
    RESTORE_SAVED_EUGID();
}

MAIL_VERSION_STAMP_DECLARE;

/* main - pass control to the multi-threaded skeleton */

int     main(int argc, char **argv)
{
    static const CONFIG_STR_TABLE str_table[] = {
	VAR_VERIFY_MAP, DEF_VERIFY_MAP, &var_verify_map, 0, 0,
	VAR_VERIFY_SENDER, DEF_VERIFY_SENDER, &var_verify_sender, 0, 0,
	0,
    };
    static const CONFIG_TIME_TABLE time_table[] = {
	VAR_VERIFY_POS_EXP, DEF_VERIFY_POS_EXP, &var_verify_pos_exp, 1, 0,
	VAR_VERIFY_POS_TRY, DEF_VERIFY_POS_TRY, &var_verify_pos_try, 1, 0,
	VAR_VERIFY_NEG_EXP, DEF_VERIFY_NEG_EXP, &var_verify_neg_exp, 1, 0,
	VAR_VERIFY_NEG_TRY, DEF_VERIFY_NEG_TRY, &var_verify_neg_try, 1, 0,
	VAR_VERIFY_SCAN_CACHE, DEF_VERIFY_SCAN_CACHE, &var_verify_scan_cache, 0, 0,
	VAR_VERIFY_SENDER_TTL, DEF_VERIFY_SENDER_TTL, &var_verify_sender_ttl, 0, 0,
	0,
    };

    /*
     * Fingerprint executables and core dumps.
     */
    MAIL_VERSION_STAMP_ALLOCATE;

    multi_server_main(argc, argv, verify_service,
		      MAIL_SERVER_STR_TABLE, str_table,
		      MAIL_SERVER_TIME_TABLE, time_table,
		      MAIL_SERVER_PRE_INIT, pre_jail_init,
		      MAIL_SERVER_POST_INIT, post_jail_init,
		      MAIL_SERVER_SOLITARY,
		      MAIL_SERVER_EXIT, verify_dump,
		      0);
}
