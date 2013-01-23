/*	$NetBSD: postscreen_state.c,v 1.1.1.1.6.1 2013/01/23 00:05:10 yamt Exp $	*/

/*++
/* NAME
/*	postscreen_state 3
/* SUMMARY
/*	postscreen session state and queue length management
/* SYNOPSIS
/*	#include <postscreen.h>
/*
/*	PSC_STATE *psc_new_session_state(stream, addr, port)
/*	VSTREAM *stream;
/*	const char *addr;
/*	const char *port;
/*
/*	void	psc_free_session_state(state)
/*	PSC_STATE *state;
/*
/*	char	*psc_print_state_flags(flags, context)
/*	int	flags;
/*	const char *context;
/*
/*	void	PSC_ADD_SERVER_STATE(state, server_fd)
/*	PSC_STATE *state;
/*	int	server_fd;
/*
/*	void	PSC_DEL_CLIENT_STATE(state)
/*	PSC_STATE *state;
/*
/*	void	PSC_DROP_SESSION_STATE(state, final_reply)
/*	PSC_STATE *state;
/*	const char *final_reply;
/*
/*	void	PSC_ENFORCE_SESSION_STATE(state, rcpt_reply)
/*	PSC_STATE *state;
/*	const char *rcpt_reply;
/*
/*	void	PSC_PASS_SESSION_STATE(state, testname, pass_flag)
/*	PSC_STATE *state;
/*	const char *testname;
/*	int	pass_flag;
/*
/*	void	PSC_FAIL_SESSION_STATE(state, fail_flag)
/*	PSC_STATE *state;
/*	int	fail_flag;
/*
/*	void	PSC_UNFAIL_SESSION_STATE(state, fail_flag)
/*	PSC_STATE *state;
/*	int	fail_flag;
/* DESCRIPTION
/*	This module maintains per-client session state, and two
/*	global file descriptor counters:
/* .IP psc_check_queue_length
/*	The total number of remote SMTP client sockets.
/* .IP psc_post_queue_length
/*	The total number of server file descriptors that are currently
/*	in use for client file descriptor passing. This number
/*	equals the number of client file descriptors in transit.
/* .PP
/*	psc_new_session_state() creates a new session state object
/*	for the specified client stream, and increments the
/*	psc_check_queue_length counter.  The flags and per-test time
/*	stamps are initialized with PSC_INIT_TESTS().  The addr and
/*	port arguments are null-terminated strings with the remote
/*	SMTP client endpoint. The _reply members are set to
/*	polite "try again" SMTP replies. The protocol member is set
/*	to "SMTP".
/*
/*	The psc_stress variable is set to non-zero when
/*	psc_check_queue_length passes over a high-water mark.
/*
/*	psc_free_session_state() destroys the specified session state
/*	object, closes the applicable I/O channels, and decrements
/*	the applicable file descriptor counters: psc_check_queue_length
/*	and psc_post_queue_length.
/*
/*	The psc_stress variable is reset to zero when psc_check_queue_length
/*	passes under a low-water mark.
/*
/*	psc_print_state_flags() converts per-session flags into
/*	human-readable form. The context is for error reporting.
/*	The result is overwritten upon each call.
/*
/*	PSC_ADD_SERVER_STATE() updates the specified session state
/*	object with the specified server file descriptor, and
/*	increments the global psc_post_queue_length file descriptor
/*	counter.
/*
/*	PSC_DEL_CLIENT_STATE() updates the specified session state
/*	object, closes the client stream, and decrements the global
/*	psc_check_queue_length file descriptor counter.
/*
/*	PSC_DROP_SESSION_STATE() updates the specified session state
/*	object and closes the client stream after sending the
/*	specified SMTP reply.
/*
/*	PSC_ENFORCE_SESSION_STATE() updates the specified session
/*	state object. It arranges that the built-in SMTP engine
/*	logs sender/recipient information and rejects all RCPT TO
/*	commands with the specified SMTP reply.
/*
/*	PSC_PASS_SESSION_STATE() sets the specified "pass" flag.
/*	The testname is used for debug logging.
/*
/*	PSC_FAIL_SESSION_STATE() sets the specified "fail" flag.
/*
/*	PSC_UNFAIL_SESSION_STATE() unsets the specified "fail" flag.
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

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <name_mask.h>
#include <htable.h>

/* Global library. */

#include <mail_proto.h>

/* Master server protocols. */

#include <mail_server.h>

/* Application-specific. */

#include <postscreen.h>

/* psc_new_session_state - fill in connection state for event processing */

PSC_STATE *psc_new_session_state(VSTREAM *stream,
				         const char *addr,
				         const char *port)
{
    PSC_STATE *state;
    HTABLE_INFO *ht;

    state = (PSC_STATE *) mymalloc(sizeof(*state));
    PSC_INIT_TESTS(state);
    if ((state->smtp_client_stream = stream) != 0)
	psc_check_queue_length++;
    state->smtp_server_fd = (-1);
    state->smtp_client_addr = mystrdup(addr);
    state->smtp_client_port = mystrdup(port);
    state->send_buf = vstring_alloc(100);
    state->test_name = "TEST NAME HERE";
    state->dnsbl_reply = 0;
    state->final_reply = "421 4.3.2 Service currently unavailable\r\n";
    state->rcpt_reply = "450 4.3.2 Service currently unavailable\r\n";
    state->command_count = 0;
    state->protocol = MAIL_PROTO_SMTP;
    state->helo_name = 0;
    state->sender = 0;
    state->cmd_buffer = 0;
    state->read_state = 0;
    state->ehlo_discard_mask = 0;		/* XXX Should be ~0 */
    state->expand_buf = 0;

    /*
     * Update the stress level.
     */
    if (psc_stress == 0
	&& psc_check_queue_length >= psc_hiwat_check_queue_length) {
	psc_stress = 1;
	msg_info("entering STRESS mode with %d connections",
		 psc_check_queue_length);
    }

    /*
     * Update the per-client session count.
     */
    if ((ht = htable_locate(psc_client_concurrency, addr)) == 0)
	ht = htable_enter(psc_client_concurrency, addr, (char *) 0);
    ht->value += 1;
    state->client_concurrency = CAST_CHAR_PTR_TO_INT(ht->value);

    return (state);
}

/* psc_free_session_state - destroy connection state including connections */

void    psc_free_session_state(PSC_STATE *state)
{
    const char *myname = "psc_free_session_state";
    HTABLE_INFO *ht;

    /*
     * Update the per-client session count.
     */
    if ((ht = htable_locate(psc_client_concurrency,
			    state->smtp_client_addr)) == 0)
	msg_panic("%s: unknown client address: %s",
		  myname, state->smtp_client_addr);
    if (--(ht->value) == 0)
	htable_delete(psc_client_concurrency, state->smtp_client_addr,
		      (void (*) (char *)) 0);

    if (state->smtp_client_stream != 0) {
	event_server_disconnect(state->smtp_client_stream);
	psc_check_queue_length--;
    }
    if (state->smtp_server_fd >= 0) {
	close(state->smtp_server_fd);
	psc_post_queue_length--;
    }
    if (state->send_buf != 0)
	state->send_buf = vstring_free(state->send_buf);
    myfree(state->smtp_client_addr);
    myfree(state->smtp_client_port);
    if (state->dnsbl_reply)
	vstring_free(state->dnsbl_reply);
    if (state->helo_name)
	myfree(state->helo_name);
    if (state->sender)
	myfree(state->sender);
    if (state->cmd_buffer)
	vstring_free(state->cmd_buffer);
    if (state->expand_buf)
	vstring_free(state->expand_buf);
    myfree((char *) state);

    if (psc_check_queue_length < 0 || psc_post_queue_length < 0)
	msg_panic("bad queue length: check_queue=%d, post_queue=%d",
		  psc_check_queue_length, psc_post_queue_length);

    /*
     * Update the stress level.
     */
    if (psc_stress != 0
	&& psc_check_queue_length <= psc_lowat_check_queue_length) {
	psc_stress = 0;
	msg_info("leaving STRESS mode with %d connections",
		 psc_check_queue_length);
    }
}

/* psc_print_state_flags - format state flags */

const char *psc_print_state_flags(int flags, const char *context)
{
    static const NAME_MASK flags_mask[] = {
	"NOFORWARD", PSC_STATE_FLAG_NOFORWARD,
	"USING_TLS", PSC_STATE_FLAG_USING_TLS,
	"NEW", PSC_STATE_FLAG_NEW,
	"BLIST_FAIL", PSC_STATE_FLAG_BLIST_FAIL,
	"HANGUP", PSC_STATE_FLAG_HANGUP,
	/* unused */
	"WLIST_FAIL", PSC_STATE_FLAG_WLIST_FAIL,

	"PENAL_UPDATE", PSC_STATE_FLAG_PENAL_UPDATE,
	"PENAL_FAIL", PSC_STATE_FLAG_PENAL_FAIL,

	"PREGR_FAIL", PSC_STATE_FLAG_PREGR_FAIL,
	"PREGR_PASS", PSC_STATE_FLAG_PREGR_PASS,
	"PREGR_TODO", PSC_STATE_FLAG_PREGR_TODO,
	"PREGR_DONE", PSC_STATE_FLAG_PREGR_DONE,

	"DNSBL_FAIL", PSC_STATE_FLAG_DNSBL_FAIL,
	"DNSBL_PASS", PSC_STATE_FLAG_DNSBL_PASS,
	"DNSBL_TODO", PSC_STATE_FLAG_DNSBL_TODO,
	"DNSBL_DONE", PSC_STATE_FLAG_DNSBL_DONE,

	"PIPEL_FAIL", PSC_STATE_FLAG_PIPEL_FAIL,
	"PIPEL_PASS", PSC_STATE_FLAG_PIPEL_PASS,
	"PIPEL_TODO", PSC_STATE_FLAG_PIPEL_TODO,
	"PIPEL_SKIP", PSC_STATE_FLAG_PIPEL_SKIP,

	"NSMTP_FAIL", PSC_STATE_FLAG_NSMTP_FAIL,
	"NSMTP_PASS", PSC_STATE_FLAG_NSMTP_PASS,
	"NSMTP_TODO", PSC_STATE_FLAG_NSMTP_TODO,
	"NSMTP_SKIP", PSC_STATE_FLAG_NSMTP_SKIP,

	"BARLF_FAIL", PSC_STATE_FLAG_BARLF_FAIL,
	"BARLF_PASS", PSC_STATE_FLAG_BARLF_PASS,
	"BARLF_TODO", PSC_STATE_FLAG_BARLF_TODO,
	"BARLF_SKIP", PSC_STATE_FLAG_BARLF_SKIP,
	0,
    };

    return (str_name_mask_opt((VSTRING *) 0, context, flags_mask, flags,
			      NAME_MASK_PIPE | NAME_MASK_NUMBER));
}
