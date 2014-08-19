/*	$NetBSD: postscreen_tests.c,v 1.1.1.1.12.2 2014/08/19 23:59:44 tls Exp $	*/

/*++
/* NAME
/*	postscreen_tests 3
/* SUMMARY
/*	postscreen tests timestamp/flag bulk support
/* SYNOPSIS
/*	#include <postscreen.h>
/*
/*	void	PSC_INIT_TESTS(state)
/*	PSC_STATE *state;
/*
/*	void	psc_new_tests(state)
/*	PSC_STATE *state;
/*
/*	void	psc_parse_tests(state, stamp_text, time_value)
/*	PSC_STATE *state;
/*	const char *stamp_text;
/*	time_t time_value;
/*
/*	char	*psc_print_tests(buffer, state)
/*	VSTRING	*buffer;
/*	PSC_STATE *state;
/*
/*	char	*psc_print_grey_key(buffer, client, helo, sender, rcpt)
/*	VSTRING	*buffer;
/*	const char *client;
/*	const char *helo;
/*	const char *sender;
/*	const char *rcpt;
/*
/*	const char *psc_test_name(tindx)
/*	int	tindx;
/* DESCRIPTION
/*	The functions in this module overwrite the per-test expiration
/*	time stamps and all flags bits.  Some functions are implemented
/*	as unsafe macros, meaning they evaluate one or more arguments
/*	multiple times.
/*
/*	PSC_INIT_TESTS() is an unsafe macro that sets the per-test
/*	expiration time stamps to PSC_TIME_STAMP_INVALID, and that
/*	zeroes all the flags bits. These values are not meant to
/*	be stored into the postscreen(8) cache.
/*
/*	psc_new_tests() sets all test expiration time stamps to
/*	PSC_TIME_STAMP_NEW, and overwrites all flags bits. Only
/*	enabled tests are flagged with PSC_STATE_FLAG_TODO; the
/*	object is flagged with PSC_STATE_FLAG_NEW.
/*
/*	psc_parse_tests() parses a cache file record and overwrites
/*	all flags bits. Tests are considered "expired" when they
/*	would be expired at the specified time value. Only enabled
/*	tests are flagged as "expired"; the object is flagged as
/*	"new" if some enabled tests have "new" time stamps.
/*
/*	psc_print_tests() creates a cache file record for the
/*	specified flags and per-test expiration time stamps.
/*	This may modify the time stamps for disabled tests.
/*
/*	psc_print_grey_key() prints a greylist lookup key.
/*
/*	psc_test_name() returns the name for the specified text
/*	index.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <stdio.h>			/* sscanf */
#include <stdlib.h>			/* strtoul */

/* Utility library. */

#include <msg.h>
#include <name_code.h>

/* Global library. */

#include <mail_params.h>

/* Application-specific. */

#include <postscreen.h>

 /*
  * Kludge to detect if some test is enabled.
  */
#define PSC_PREGR_TEST_ENABLE()	(*var_psc_pregr_banner != 0)
#define PSC_DNSBL_TEST_ENABLE()	(*var_psc_dnsbl_sites != 0)

 /*
  * Format of a persistent cache entry (which is almost but not quite the
  * same as the in-memory representation).
  * 
  * Each cache entry has one time stamp for each test.
  * 
  * - A time stamp of PSC_TIME_STAMP_INVALID must never appear in the cache. It
  * is reserved for in-memory objects that are still being initialized.
  * 
  * - A time stamp of PSC_TIME_STAMP_NEW indicates that the test never passed.
  * Postscreen will log the client with "pass new" when it passes the final
  * test.
  * 
  * - A time stamp of PSC_TIME_STAMP_DISABLED indicates that the test never
  * passed, and that the test was disabled when the cache entry was written.
  * 
  * - Otherwise, the test was passed, and the time stamp indicates when that
  * test result expires.
  * 
  * A cache entry is expired when the time stamps of all passed tests are
  * expired.
  */

/* psc_new_tests - initialize new test results from scratch */

void    psc_new_tests(PSC_STATE *state)
{

    /*
     * We know this client is brand new.
     */
    state->flags = PSC_STATE_FLAG_NEW;

    /*
     * Give all tests a PSC_TIME_STAMP_NEW time stamp, so that we can later
     * recognize cache entries that haven't passed all enabled tests. When we
     * write a cache entry to the database, any new-but-disabled tests will
     * get a PSC_TIME_STAMP_DISABLED time stamp.
     */
    state->pregr_stamp = PSC_TIME_STAMP_NEW;
    state->dnsbl_stamp = PSC_TIME_STAMP_NEW;
    state->pipel_stamp = PSC_TIME_STAMP_NEW;
    state->nsmtp_stamp = PSC_TIME_STAMP_NEW;
    state->barlf_stamp = PSC_TIME_STAMP_NEW;

    /*
     * Don't flag disabled tests as "todo", because there would be no way to
     * make those bits go away.
     */
    if (PSC_PREGR_TEST_ENABLE())
	state->flags |= PSC_STATE_FLAG_PREGR_TODO;
    if (PSC_DNSBL_TEST_ENABLE())
	state->flags |= PSC_STATE_FLAG_DNSBL_TODO;
    if (var_psc_pipel_enable)
	state->flags |= PSC_STATE_FLAG_PIPEL_TODO;
    if (var_psc_nsmtp_enable)
	state->flags |= PSC_STATE_FLAG_NSMTP_TODO;
    if (var_psc_barlf_enable)
	state->flags |= PSC_STATE_FLAG_BARLF_TODO;
}

/* psc_parse_tests - parse test results from cache */

void    psc_parse_tests(PSC_STATE *state,
			        const char *stamp_str,
			        time_t time_value)
{
    const char *start = stamp_str;
    char   *cp;
    time_t *time_stamps = state->expire_time;
    time_t *sp;

    /*
     * We don't know what tests have expired or have never passed.
     */
    state->flags = 0;

    /*
     * Parse the cache entry, and allow for older postscreen versions that
     * implemented fewer tests. We pretend that the newer tests were disabled
     * at the time that the cache entry was written.
     * 
     * Flag the cache entry as "new" when the cache entry has fields for all
     * enabled tests, but the remote SMTP client has not yet passed all those
     * tests.
     */
    for (sp = time_stamps; sp < time_stamps + PSC_TINDX_COUNT; sp++) {
	*sp = strtoul(start, &cp, 10);
	if (*start == 0 || (*cp != '\0' && *cp != ';') || errno == ERANGE)
	    *sp = PSC_TIME_STAMP_DISABLED;
	if (*sp == PSC_TIME_STAMP_NEW)
	    state->flags |= PSC_STATE_FLAG_NEW;
	if (msg_verbose)
	    msg_info("%s -> %lu", start, (unsigned long) *sp);
	if (*cp == ';')
	    start = cp + 1;
	else
	    start = cp;
    }

    /*
     * Don't flag disabled tests as "todo", because there would be no way to
     * make those bits go away.
     */
    if (PSC_PREGR_TEST_ENABLE() && time_value > state->pregr_stamp)
	state->flags |= PSC_STATE_FLAG_PREGR_TODO;
    if (PSC_DNSBL_TEST_ENABLE() && time_value > state->dnsbl_stamp)
	state->flags |= PSC_STATE_FLAG_DNSBL_TODO;
    if (var_psc_pipel_enable && time_value > state->pipel_stamp)
	state->flags |= PSC_STATE_FLAG_PIPEL_TODO;
    if (var_psc_nsmtp_enable && time_value > state->nsmtp_stamp)
	state->flags |= PSC_STATE_FLAG_NSMTP_TODO;
    if (var_psc_barlf_enable && time_value > state->barlf_stamp)
	state->flags |= PSC_STATE_FLAG_BARLF_TODO;

    /*
     * If any test has expired, proactively refresh tests that will expire
     * soon. This can increase the occurrence of client-visible delays, but
     * avoids questions about why a client can pass some test and then fail
     * within seconds. The proactive refresh time is really a surrogate for
     * the user's curiosity level, and therefore hard to choose optimally.
     */
#ifdef VAR_PSC_REFRESH_TIME
    if ((state->flags & PSC_STATE_MASK_ANY_TODO) != 0
	&& var_psc_refresh_time > 0) {
	time_t  refresh_time = time_value + var_psc_refresh_time;

	if (PSC_PREGR_TEST_ENABLE() && refresh_time > state->pregr_stamp)
	    state->flags |= PSC_STATE_FLAG_PREGR_TODO;
	if (PSC_DNSBL_TEST_ENABLE() && refresh_time > state->dnsbl_stamp)
	    state->flags |= PSC_STATE_FLAG_DNSBL_TODO;
	if (var_psc_pipel_enable && refresh_time > state->pipel_stamp)
	    state->flags |= PSC_STATE_FLAG_PIPEL_TODO;
	if (var_psc_nsmtp_enable && refresh_time > state->nsmtp_stamp)
	    state->flags |= PSC_STATE_FLAG_NSMTP_TODO;
	if (var_psc_barlf_enable && refresh_time > state->barlf_stamp)
	    state->flags |= PSC_STATE_FLAG_BARLF_TODO;
    }
#endif

    /*
     * Gratuitously make postscreen logging more useful by turning on all
     * enabled pre-handshake tests when any pre-handshake test is turned on.
     * 
     * XXX Don't enable PREGREET gratuitously before the test expires. With a
     * short TTL for DNSBL whitelisting, turning on PREGREET would force a
     * full postscreen_greet_wait too frequently.
     */
#if 0
    if (state->flags & PSC_STATE_MASK_EARLY_TODO) {
	if (PSC_PREGR_TEST_ENABLE())
	    state->flags |= PSC_STATE_FLAG_PREGR_TODO;
	if (PSC_DNSBL_TEST_ENABLE())
	    state->flags |= PSC_STATE_FLAG_DNSBL_TODO;
    }
#endif
}

/* psc_print_tests - print postscreen cache record */

char   *psc_print_tests(VSTRING *buf, PSC_STATE *state)
{
    const char *myname = "psc_print_tests";

    /*
     * Sanity check.
     */
    if ((state->flags & PSC_STATE_MASK_ANY_UPDATE) == 0)
	msg_panic("%s: attempt to save a no-update record", myname);

    /*
     * Give disabled tests a dummy time stamp so that we don't log a client
     * with "pass new" when some disabled test becomes enabled at some later
     * time.
     */
    if (PSC_PREGR_TEST_ENABLE() == 0 && state->pregr_stamp == PSC_TIME_STAMP_NEW)
	state->pregr_stamp = PSC_TIME_STAMP_DISABLED;
    if (PSC_DNSBL_TEST_ENABLE() == 0 && state->dnsbl_stamp == PSC_TIME_STAMP_NEW)
	state->dnsbl_stamp = PSC_TIME_STAMP_DISABLED;
    if (var_psc_pipel_enable == 0 && state->pipel_stamp == PSC_TIME_STAMP_NEW)
	state->pipel_stamp = PSC_TIME_STAMP_DISABLED;
    if (var_psc_nsmtp_enable == 0 && state->nsmtp_stamp == PSC_TIME_STAMP_NEW)
	state->nsmtp_stamp = PSC_TIME_STAMP_DISABLED;
    if (var_psc_barlf_enable == 0 && state->barlf_stamp == PSC_TIME_STAMP_NEW)
	state->barlf_stamp = PSC_TIME_STAMP_DISABLED;

    vstring_sprintf(buf, "%lu;%lu;%lu;%lu;%lu",
		    (unsigned long) state->pregr_stamp,
		    (unsigned long) state->dnsbl_stamp,
		    (unsigned long) state->pipel_stamp,
		    (unsigned long) state->nsmtp_stamp,
		    (unsigned long) state->barlf_stamp);
    return (STR(buf));
}

/* psc_print_grey_key - print postscreen cache record */

char   *psc_print_grey_key(VSTRING *buf, const char *client,
			           const char *helo, const char *sender,
			           const char *rcpt)
{
    return (STR(vstring_sprintf(buf, "%s/%s/%s/%s",
				client, helo, sender, rcpt)));
}

/* psc_test_name - map test index to symbolic name */

const char *psc_test_name(int tindx)
{
    const char *myname = "psc_test_name";
    const NAME_CODE test_name_map[] = {
	PSC_TNAME_PREGR, PSC_TINDX_PREGR,
	PSC_TNAME_DNSBL, PSC_TINDX_DNSBL,
	PSC_TNAME_PIPEL, PSC_TINDX_PIPEL,
	PSC_TNAME_NSMTP, PSC_TINDX_NSMTP,
	PSC_TNAME_BARLF, PSC_TINDX_BARLF,
	0, -1,
    };
    const char *result;

    if ((result = str_name_code(test_name_map, tindx)) == 0)
	msg_panic("%s: bad index %d", myname, tindx);
    return (result);
}
