/*	$NetBSD: postscreen_early.c,v 1.1.1.1.12.1 2014/08/19 23:59:44 tls Exp $	*/

/*++
/* NAME
/*	postscreen_early 3
/* SUMMARY
/*	postscreen pre-handshake tests
/* SYNOPSIS
/*	#include <postscreen.h>
/*
/*	void	psc_early_init(void)
/*
/*	void	psc_early_tests(state)
/*	PSC_STATE *state;
/* DESCRIPTION
/*	psc_early_tests() performs protocol tests before the SMTP
/*	handshake: the pregreet test and the DNSBL test. Control
/*	is passed to the psc_smtpd_tests() routine as appropriate.
/*
/*	psc_early_init() performs one-time initialization.
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
#include <sys/socket.h>
#include <limits.h>

/* Utility library. */

#include <msg.h>
#include <stringops.h>
#include <mymalloc.h>
#include <vstring.h>

/* Global library. */

#include <mail_params.h>

/* Application-specific. */

#include <postscreen.h>

static char *psc_teaser_greeting;
static VSTRING *psc_escape_buf;

/* psc_whitelist_non_dnsbl - whitelist pending non-dnsbl tests */

static void psc_whitelist_non_dnsbl(PSC_STATE *state)
{
    time_t  now;
    int     tindx;

    /*
     * If no tests failed (we can't undo those), and if the whitelist
     * threshold is met, flag non-dnsbl tests that are pending or disabled as
     * successfully completed, and set their expiration times equal to the
     * DNSBL expiration time, except for tests that would expire later.
     * 
     * Why flag disabled tests as passed? When a disabled test is turned on,
     * postscreen should not apply that test to clients that are already
     * whitelisted based on their combined DNSBL score.
     */
    if ((state->flags & PSC_STATE_MASK_ANY_FAIL) == 0
	&& state->dnsbl_score < var_psc_dnsbl_thresh
	&& var_psc_dnsbl_wthresh < 0
	&& state->dnsbl_score <= var_psc_dnsbl_wthresh) {
	now = event_time();
	for (tindx = 0; tindx < PSC_TINDX_COUNT; tindx++) {
	    if (tindx == PSC_TINDX_DNSBL)
		continue;
	    if ((state->flags & PSC_STATE_FLAG_BYTINDX_TODO(tindx))
		&& !(state->flags & PSC_STATE_FLAG_BYTINDX_PASS(tindx))) {
		if (msg_verbose)
		    msg_info("skip %s test for [%s]:%s",
			 psc_test_name(tindx), PSC_CLIENT_ADDR_PORT(state));
		/* Wrong for deep protocol tests, but we disable those. */
		state->flags |= PSC_STATE_FLAG_BYTINDX_DONE(tindx);
		/* This also disables pending deep protocol tests. */
		state->flags |= PSC_STATE_FLAG_BYTINDX_PASS(tindx);
	    }
	    /* Update expiration even if the test was completed or disabled. */
	    if (state->expire_time[tindx] < now + var_psc_dnsbl_ttl)
		state->expire_time[tindx] = now + var_psc_dnsbl_ttl;
	}
    }
}

/* psc_early_event - handle pre-greet, EOF, and DNSBL results. */

static void psc_early_event(int event, char *context)
{
    const char *myname = "psc_early_event";
    PSC_STATE *state = (PSC_STATE *) context;
    char    read_buf[PSC_READ_BUF_SIZE];
    int     read_count;
    DELTA_TIME elapsed;

    if (msg_verbose > 1)
	msg_info("%s: sq=%d cq=%d event %d on smtp socket %d from [%s]:%s flags=%s",
		 myname, psc_post_queue_length, psc_check_queue_length,
		 event, vstream_fileno(state->smtp_client_stream),
		 state->smtp_client_addr, state->smtp_client_port,
		 psc_print_state_flags(state->flags, myname));

    PSC_CLEAR_EVENT_REQUEST(vstream_fileno(state->smtp_client_stream),
			    psc_early_event, context);

    /*
     * XXX Be sure to empty the DNSBL lookup buffer otherwise we have a
     * memory leak.
     * 
     * XXX We can avoid "forgetting" to do this by keeping a pointer to the
     * DNSBL lookup buffer in the PSC_STATE structure. This also allows us to
     * shave off a hash table lookup when retrieving the DNSBL result.
     * 
     * A direct pointer increases the odds of dangling pointers. Hash-table
     * lookup is safer, and that is why it's done that way.
     */
    switch (event) {

	/*
	 * We either reached the end of the early tests time limit, or all
	 * early tests completed before the pregreet timer would go off.
	 */
    case EVENT_TIME:

	/*
	 * Check if the SMTP client spoke before its turn.
	 */
	if ((state->flags & PSC_STATE_FLAG_PREGR_TODO) != 0
	    && (state->flags & PSC_STATE_MASK_PREGR_FAIL_DONE) == 0) {
	    state->pregr_stamp = event_time() + var_psc_pregr_ttl;
	    PSC_PASS_SESSION_STATE(state, "pregreet test",
				   PSC_STATE_FLAG_PREGR_PASS);
	}
	if ((state->flags & PSC_STATE_FLAG_PREGR_FAIL)
	    && psc_pregr_action == PSC_ACT_IGNORE) {
	    PSC_UNFAIL_SESSION_STATE(state, PSC_STATE_FLAG_PREGR_FAIL);
	    /* Not: PSC_PASS_SESSION_STATE. Repeat this test the next time. */
	}

	/*
	 * Collect the DNSBL score, and whitelist other tests if applicable.
	 * Note: this score will be partial when some DNS lookup did not
	 * complete before the pregreet timer expired.
	 * 
	 * If the client is DNS blocklisted, drop the connection, send the
	 * client to a dummy protocol engine, or continue to the next test.
	 */
#define PSC_DNSBL_FORMAT \
	"%s 5.7.1 Service unavailable; client [%s] blocked using %s\r\n"
#define NO_DNSBL_SCORE	INT_MAX

	if (state->flags & PSC_STATE_FLAG_DNSBL_TODO) {
	    if (state->dnsbl_score == NO_DNSBL_SCORE) {
		state->dnsbl_score =
		    psc_dnsbl_retrieve(state->smtp_client_addr,
				       &state->dnsbl_name,
				       state->dnsbl_index);
		if (var_psc_dnsbl_wthresh < 0)
		    psc_whitelist_non_dnsbl(state);
	    }
	    if (state->dnsbl_score < var_psc_dnsbl_thresh) {
		state->dnsbl_stamp = event_time() + var_psc_dnsbl_ttl;
		PSC_PASS_SESSION_STATE(state, "dnsbl test",
				       PSC_STATE_FLAG_DNSBL_PASS);
	    } else {
		msg_info("DNSBL rank %d for [%s]:%s",
			 state->dnsbl_score, PSC_CLIENT_ADDR_PORT(state));
		PSC_FAIL_SESSION_STATE(state, PSC_STATE_FLAG_DNSBL_FAIL);
		switch (psc_dnsbl_action) {
		case PSC_ACT_DROP:
		    state->dnsbl_reply = vstring_sprintf(vstring_alloc(100),
						    PSC_DNSBL_FORMAT, "521",
						    state->smtp_client_addr,
							 state->dnsbl_name);
		    PSC_DROP_SESSION_STATE(state, STR(state->dnsbl_reply));
		    return;
		case PSC_ACT_ENFORCE:
		    state->dnsbl_reply = vstring_sprintf(vstring_alloc(100),
						    PSC_DNSBL_FORMAT, "550",
						    state->smtp_client_addr,
							 state->dnsbl_name);
		    PSC_ENFORCE_SESSION_STATE(state, STR(state->dnsbl_reply));
		    break;
		case PSC_ACT_IGNORE:
		    PSC_UNFAIL_SESSION_STATE(state, PSC_STATE_FLAG_DNSBL_FAIL);
		    /* Not: PSC_PASS_SESSION_STATE. Repeat this test. */
		    break;
		default:
		    msg_panic("%s: unknown dnsbl action value %d",
			      myname, psc_dnsbl_action);

		}
	    }
	}

	/*
	 * Pass the connection to a real SMTP server, or enter the dummy
	 * engine for deep tests.
	 */
	if ((state->flags & PSC_STATE_FLAG_NOFORWARD) != 0
	    || ((state->flags & PSC_STATE_MASK_SMTPD_PASS)
		!= PSC_STATE_FLAGS_TODO_TO_PASS(state->flags & PSC_STATE_MASK_SMTPD_TODO)))
	    psc_smtpd_tests(state);
	else
	    psc_conclude(state);
	return;

	/*
	 * EOF, or the client spoke before its turn. We simply drop the
	 * connection, or we continue waiting and allow DNS replies to
	 * trickle in.
	 */
    default:
	if ((read_count = recv(vstream_fileno(state->smtp_client_stream),
			  read_buf, sizeof(read_buf) - 1, MSG_PEEK)) <= 0) {
	    /* Avoid memory leak. */
	    if (state->dnsbl_score == NO_DNSBL_SCORE
		&& (state->flags & PSC_STATE_FLAG_DNSBL_TODO))
		(void) psc_dnsbl_retrieve(state->smtp_client_addr,
					  &state->dnsbl_name,
					  state->dnsbl_index);
	    /* XXX Wait for DNS replies to come in. */
	    psc_hangup_event(state);
	    return;
	}
	read_buf[read_count] = 0;
	escape(psc_escape_buf, read_buf, read_count);
	msg_info("PREGREET %d after %s from [%s]:%s: %.100s", read_count,
	       psc_format_delta_time(psc_temp, state->start_time, &elapsed),
		 PSC_CLIENT_ADDR_PORT(state), STR(psc_escape_buf));
	PSC_FAIL_SESSION_STATE(state, PSC_STATE_FLAG_PREGR_FAIL);
	switch (psc_pregr_action) {
	case PSC_ACT_DROP:
	    /* Avoid memory leak. */
	    if (state->dnsbl_score == NO_DNSBL_SCORE
		&& (state->flags & PSC_STATE_FLAG_DNSBL_TODO))
		(void) psc_dnsbl_retrieve(state->smtp_client_addr,
					  &state->dnsbl_name,
					  state->dnsbl_index);
	    PSC_DROP_SESSION_STATE(state, "521 5.5.1 Protocol error\r\n");
	    return;
	case PSC_ACT_ENFORCE:
	    /* We call psc_dnsbl_retrieve() when the timer expires. */
	    PSC_ENFORCE_SESSION_STATE(state, "550 5.5.1 Protocol error\r\n");
	    break;
	case PSC_ACT_IGNORE:
	    /* We call psc_dnsbl_retrieve() when the timer expires. */
	    /* We must handle this case after the timer expires. */
	    break;
	default:
	    msg_panic("%s: unknown pregreet action value %d",
		      myname, psc_pregr_action);
	}

	/*
	 * Terminate the greet delay if we're just waiting for the pregreet
	 * test to complete. It is safe to call psc_early_event directly,
	 * since we are already in that function.
	 * 
	 * XXX After this code passes all tests, swap around the two blocks in
	 * this switch statement and fall through from EVENT_READ into
	 * EVENT_TIME, instead of calling psc_early_event recursively.
	 */
	state->flags |= PSC_STATE_FLAG_PREGR_DONE;
	if (elapsed.dt_sec >= PSC_EFF_GREET_WAIT
	    || ((state->flags & PSC_STATE_MASK_EARLY_DONE)
		== PSC_STATE_FLAGS_TODO_TO_DONE(state->flags & PSC_STATE_MASK_EARLY_TODO)))
	    psc_early_event(EVENT_TIME, context);
	else
	    event_request_timer(psc_early_event, context,
				PSC_EFF_GREET_WAIT - elapsed.dt_sec);
	return;
    }
}

/* psc_early_dnsbl_event - cancel pregreet timer if waiting for DNS only */

static void psc_early_dnsbl_event(int unused_event, char *context)
{
    const char *myname = "psc_early_dnsbl_event";
    PSC_STATE *state = (PSC_STATE *) context;

    if (msg_verbose)
	msg_info("%s: notify [%s]:%s", myname, PSC_CLIENT_ADDR_PORT(state));

    /*
     * Collect the DNSBL score, and whitelist other tests if applicable.
     */
    state->dnsbl_score =
	psc_dnsbl_retrieve(state->smtp_client_addr, &state->dnsbl_name,
			   state->dnsbl_index);
    if (var_psc_dnsbl_wthresh < 0)
	psc_whitelist_non_dnsbl(state);

    /*
     * Terminate the greet delay if we're just waiting for DNSBL lookup to
     * complete. Don't call psc_early_event directly, that would result in a
     * dangling pointer.
     */
    state->flags |= PSC_STATE_FLAG_DNSBL_DONE;
    if ((state->flags & PSC_STATE_MASK_EARLY_DONE)
	== PSC_STATE_FLAGS_TODO_TO_DONE(state->flags & PSC_STATE_MASK_EARLY_TODO))
	event_request_timer(psc_early_event, context, EVENT_NULL_DELAY);
}

/* psc_early_tests - start the early (before protocol) tests */

void    psc_early_tests(PSC_STATE *state)
{
    const char *myname = "psc_early_tests";

    /*
     * Report errors and progress in the context of this test.
     */
    PSC_BEGIN_TESTS(state, "tests before SMTP handshake");

    /*
     * Run a PREGREET test. Send half the greeting banner, by way of teaser,
     * then wait briefly to see if the client speaks before its turn.
     */
    if ((state->flags & PSC_STATE_FLAG_PREGR_TODO) != 0
	&& psc_teaser_greeting != 0
	&& PSC_SEND_REPLY(state, psc_teaser_greeting) != 0) {
	psc_hangup_event(state);
	return;
    }

    /*
     * Run a DNS blocklist query.
     */
    if ((state->flags & PSC_STATE_FLAG_DNSBL_TODO) != 0)
	state->dnsbl_index =
	    psc_dnsbl_request(state->smtp_client_addr, psc_early_dnsbl_event,
			      (char *) state);
    else
	state->dnsbl_index = -1;
    state->dnsbl_score = NO_DNSBL_SCORE;

    /*
     * Wait for the client to respond or for DNS lookup to complete.
     */
    if ((state->flags & PSC_STATE_FLAG_PREGR_TODO) != 0)
	PSC_READ_EVENT_REQUEST(vstream_fileno(state->smtp_client_stream),
		       psc_early_event, (char *) state, PSC_EFF_GREET_WAIT);
    else
	event_request_timer(psc_early_event, (char *) state, PSC_EFF_GREET_WAIT);
}

/* psc_early_init - initialize early tests */

void    psc_early_init(void)
{
    if (*var_psc_pregr_banner) {
	vstring_sprintf(psc_temp, "220-%s\r\n", var_psc_pregr_banner);
	psc_teaser_greeting = mystrdup(STR(psc_temp));
	psc_escape_buf = vstring_alloc(100);
    }
}
