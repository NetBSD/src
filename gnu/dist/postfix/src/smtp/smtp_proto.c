/*++
/* NAME
/*	smtp_proto 3
/* SUMMARY
/*	client SMTP protocol
/* SYNOPSIS
/*	#include "smtp.h"
/*
/*	int	smtp_helo(state)
/*	SMTP_STATE *state;
/*
/*	int	smtp_xfer(state)
/*	SMTP_STATE *state;
/* DESCRIPTION
/*	This module implements the client side of the SMTP protocol.
/*
/*	smtp_helo() performs the initial handshake with the SMTP server.
/*
/*	smtp_xfer() sends message envelope information followed by the
/*	message data, and finishes the SMTP conversation. These operations
/*	are combined in one function, in order to implement SMTP pipelining.
/*	Recipients are marked as "done" in the mail queue file when
/*	bounced or delivered. The message delivery status is updated
/*	accordingly.
/* DIAGNOSTICS
/*	smtp_helo() and smtp_xfer() return 0 in case of success, -1 in case
/*	of failure. For smtp_xfer(), success means the ability to perform
/*	an SMTP conversation, not necessarily the ability to deliver mail.
/*
/*	Warnings: corrupt message file. A corrupt message is marked
/*	as "corrupt" by changing its queue file permissions.
/* BUGS
/*	Some SMTP servers will abort when the number of recipients
/*	for one message exceeds their capacity. This behavior violates
/*	the SMTP protocol.
/*	The only way around this is to limit the number of recipients
/*	per transaction to an artificially-low value.
/* SEE ALSO
/*	smtp(3h) internal data structures
/*	smtp_chat(3) query/reply SMTP support
/*	smtp_trouble(3) error handlers
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Pipelining code in cooperation with:
/*	Jon Ribbens
/*	Oaktree Internet Solutions Ltd.,
/*	Internet House,
/*	Canal Basin,
/*	Coventry,
/*	CV1 4LY, United Kingdom.
/*
/*--*/

/* System library. */

#include <sys_defs.h>
#include <sys/stat.h>
#include <sys/socket.h>			/* shutdown(2) */
#include <string.h>
#include <unistd.h>
#include <stdlib.h>			/* 44BSD stdarg.h uses abort() */
#include <stdarg.h>
#include <time.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <vstream.h>
#include <vstring_vstream.h>
#include <stringops.h>
#include <mymalloc.h>
#include <iostuff.h>

/* Global library. */

#include <mail_params.h>
#include <smtp_stream.h>
#include <mail_queue.h>
#include <recipient_list.h>
#include <deliver_request.h>
#include <deliver_completed.h>
#include <defer.h>
#include <bounce.h>
#include <sent.h>
#include <record.h>
#include <rec_type.h>
#include <off_cvt.h>
#include <mark_corrupt.h>
#include <quote_821_local.h>

/* Application-specific. */

#include "smtp.h"
#include "smtp_sasl.h"

 /*
  * Sender and receiver state. A session does not necessarily go through a
  * linear progression, but states are guaranteed to not jump backwards.
  * Normal sessions go from MAIL->RCPT->DATA->DOT->QUIT->LAST. The states
  * MAIL, RCPT, and DATA may also be followed by ABORT->QUIT->LAST.
  * 
  * By default, the receiver skips the QUIT response. Some SMTP servers
  * disconnect after responding to ".", and some SMTP servers wait before
  * responding to QUIT.
  * 
  * Client states that are associated with sending mail (up to and including
  * SMTP_STATE_DOT) must have smaller numerical values than the non-sending
  * states (SMTP_STATE_ABORT .. SMTP_STATE_LAST).
  */
#define SMTP_STATE_MAIL		0
#define SMTP_STATE_RCPT		1
#define SMTP_STATE_DATA		2
#define SMTP_STATE_DOT		3
#define SMTP_STATE_ABORT	4
#define SMTP_STATE_QUIT		5
#define SMTP_STATE_LAST		6

int    *xfer_timeouts[SMTP_STATE_LAST] = {
    &var_smtp_mail_tmout,
    &var_smtp_rcpt_tmout,
    &var_smtp_data0_tmout,
    &var_smtp_data2_tmout,
    &var_smtp_quit_tmout,
    &var_smtp_quit_tmout,
};

char   *xfer_states[SMTP_STATE_LAST] = {
    "sending MAIL FROM",
    "sending RCPT TO",
    "sending DATA command",
    "sending end of data -- message may be sent more than once",
    "sending final RSET",
    "sending QUIT",
};

/* smtp_helo - perform initial handshake with SMTP server */

int     smtp_helo(SMTP_STATE *state)
{
    SMTP_SESSION *session = state->session;
    DELIVER_REQUEST *request = state->request;
    SMTP_RESP *resp;
    int     except;
    char   *lines;
    char   *words;
    char   *word;
    int     n;

    /*
     * Prepare for disaster.
     */
    smtp_timeout_setup(state->session->stream, var_smtp_helo_tmout);
    if ((except = vstream_setjmp(state->session->stream)) != 0)
	return (smtp_stream_except(state, except, "sending HELO"));

    /*
     * Read and parse the server's SMTP greeting banner.
     */
    if (((resp = smtp_chat_resp(state))->code / 100) != 2)
	return (smtp_site_fail(state, resp->code,
			       "host %s refused to talk to me: %s",
			 session->namaddr, translit(resp->str, "\n", " ")));

    /*
     * XXX Some PIX firewall versions require flush before ".<CR><LF>" so it
     * does not span a packet boundary. This hurts performance so it is not
     * on by default.
     */
    if (resp->str[strspn(resp->str, "20 *\t\n")] == 0)
	state->features |= SMTP_FEATURE_MAYBEPIX;

    /*
     * See if we are talking to ourself. This should not be possible with the
     * way we implement DNS lookups. However, people are known to sometimes
     * screw up the naming service. And, mailer loops are still possible when
     * our own mailer routing tables are mis-configured.
     */
    words = resp->str;
    (void) mystrtok(&words, "- \t\n");
    for (n = 0; (word = mystrtok(&words, " \t\n")) != 0; n++) {
	if (n == 0 && strcasecmp(word, var_myhostname) == 0) {
	    msg_warn("host %s greeted me with my own hostname %s",
		     session->namaddr, var_myhostname);
	} else if (strcasecmp(word, "ESMTP") == 0)
	    state->features |= SMTP_FEATURE_ESMTP;
    }
    if (var_smtp_always_ehlo && (state->features & SMTP_FEATURE_MAYBEPIX) == 0)
	state->features |= SMTP_FEATURE_ESMTP;
    if (var_smtp_never_ehlo || (state->features & SMTP_FEATURE_MAYBEPIX) != 0)
	state->features &= ~SMTP_FEATURE_ESMTP;

    /*
     * Return the compliment. Fall back to SMTP if our ESMTP recognition
     * heuristic failed.
     */
    if (state->features & SMTP_FEATURE_ESMTP) {
	smtp_chat_cmd(state, "EHLO %s", var_myhostname);
	if ((resp = smtp_chat_resp(state))->code / 100 != 2)
	    state->features &= ~SMTP_FEATURE_ESMTP;
    }
    if ((state->features & SMTP_FEATURE_ESMTP) == 0) {
	smtp_chat_cmd(state, "HELO %s", var_myhostname);
	if ((resp = smtp_chat_resp(state))->code / 100 != 2)
	    return (smtp_site_fail(state, resp->code,
				   "host %s refused to talk to me: %s",
				   session->namaddr,
				   translit(resp->str, "\n", " ")));
    }

    /*
     * Pick up some useful features offered by the SMTP server. XXX Until we
     * have a portable routine to convert from string to off_t with proper
     * overflow detection, ignore the message size limit advertised by the
     * SMTP server. Otherwise, we might do the wrong thing when the server
     * advertises a really huge message size limit.
     * 
     * XXX Allow for "code (SP|-) ehlo-keyword (SP|=) ehlo-param...", because
     * MicroSoft implemented AUTH based on an old draft.
     */
    lines = resp->str;
    while ((words = mystrtok(&lines, "\n")) != 0) {
	if (mystrtok(&words, "- ") && (word = mystrtok(&words, " \t=")) != 0) {
	    if (strcasecmp(word, "8BITMIME") == 0)
		state->features |= SMTP_FEATURE_8BITMIME;
	    else if (strcasecmp(word, "PIPELINING") == 0)
		state->features |= SMTP_FEATURE_PIPELINING;
	    else if (strcasecmp(word, "SIZE") == 0) {
		state->features |= SMTP_FEATURE_SIZE;
		if ((word = mystrtok(&words, " \t")) != 0) {
		    if (!alldig(word))
			msg_warn("bad size limit \"%s\" in EHLO reply from %s",
				 word, session->namaddr);
		    else
			state->size_limit = off_cvt_string(word);
		}
	    }
#ifdef USE_SASL_AUTH
	    else if (var_smtp_sasl_enable && strcasecmp(word, "AUTH") == 0)
		smtp_sasl_helo_auth(state, words);
#endif
	    else if (strcasecmp(word, var_myhostname) == 0) {
		msg_warn("host %s replied to HELO/EHLO with my own hostname %s",
			 session->namaddr, var_myhostname);
		return (smtp_site_fail(state, session->best ? 550 : 450,
				       "mail for %s loops back to myself",
				       request->nexthop));
	    }
	}
    }
    if (msg_verbose)
	msg_info("server features: 0x%x size %.0f",
		 state->features, (double) state->size_limit);

#ifdef USE_SASL_AUTH
    if (var_smtp_sasl_enable && (state->features & SMTP_FEATURE_AUTH))
	return (smtp_sasl_helo_login(state));
#endif

    return (0);
}

/* smtp_xfer - send a batch of envelope information and the message data */

int     smtp_xfer(SMTP_STATE *state)
{
    char   *myname = "smtp_xfer";
    DELIVER_REQUEST *request = state->request;
    SMTP_SESSION *session = state->session;
    SMTP_RESP *resp;
    RECIPIENT *rcpt;
    VSTRING *next_command = vstring_alloc(100);
    int     next_state;
    int     next_rcpt;
    int     send_state;
    int     recv_state;
    int     send_rcpt;
    int     recv_rcpt;
    int     nrcpt;
    int     except;
    int     rec_type;
    int     prev_type = 0;
    int     sndbufsize;
    int     sndbuffree;
    SOCKOPT_SIZE optlen = sizeof(sndbufsize);
    int     mail_from_rejected;

    /*
     * Macros for readability.
     */
#define REWRITE_ADDRESS(addr) do { \
	if (*(addr)) { \
	    quote_821_local(state->scratch, addr); \
	    smtp_unalias_addr(state->scratch2, vstring_str(state->scratch)); \
	    myfree(addr); \
	    addr = mystrdup(vstring_str(state->scratch2)); \
	} \
    } while (0)

#define RETURN(x) do { vstring_free(next_command); return (x); } while (0)

#define SENDER_IS_AHEAD \
	(recv_state < send_state || recv_rcpt != send_rcpt)

#define SENDER_IN_WAIT_STATE \
	(send_state == SMTP_STATE_DOT || send_state == SMTP_STATE_LAST)

#define SENDING_MAIL \
	(recv_state <= SMTP_STATE_DOT)

    /*
     * See if we should even try to send this message at all. This code sits
     * here rather than in the EHLO processing code, because of future SMTP
     * connection caching.
     */
    if (state->size_limit > 0 && state->size_limit < request->data_size) {
	smtp_mesg_fail(state, 552,
		    "message size %lu exceeds size limit %.0f of server %s",
		       request->data_size, (double) state->size_limit,
		       session->namaddr);
	RETURN(0);
    }

    /*
     * We use SMTP command pipelining if the server said it supported it.
     * Since we use blocking I/O, RFC 2197 says that we should inspect the
     * TCP window size and not send more than this amount of information.
     * Unfortunately this information is not available using the sockets
     * interface. However, we *can* get the TCP send buffer size on the local
     * TCP/IP stack. We should be able to fill this buffer without being
     * blocked, and then the kernel will effectively do non-blocking I/O for
     * us by automatically writing out the contents of its send buffer while
     * we are reading in the responses. In addition to TCP buffering we have
     * to be aware of application-level buffering by the vstream module,
     * which is limited to a couple kbytes.
     */
    if (state->features & SMTP_FEATURE_PIPELINING) {
	if (getsockopt(vstream_fileno(state->session->stream), SOL_SOCKET,
		       SO_SNDBUF, (char *) &sndbufsize, &optlen) < 0)
	    msg_fatal("%s: getsockopt: %m", myname);
	if (msg_verbose)
	    msg_info("Using ESMTP PIPELINING, TCP send buffer size is %d",
		     sndbufsize);
    } else {
	sndbufsize = 0;
    }
    sndbuffree = sndbufsize;

    /*
     * Pipelining support requires two loops: one loop for sending and one
     * for receiving. Each loop has its own independent state. Most of the
     * time the sender can run ahead of the receiver by as much as the TCP
     * send buffer permits. There are only two places where the sender must
     * wait for status information from the receiver: once after sending DATA
     * and once after sending QUIT.
     * 
     * The sender state advances until the TCP send buffer would overflow, or
     * until the sender needs status information from the receiver. At that
     * point the receiver starts processing responses. Once the receiver has
     * caught up with the sender, the sender resumes sending commands. If the
     * receiver detects a serious problem (MAIL FROM rejected, all RCPT TO
     * commands rejected, DATA rejected) it forces the sender to abort the
     * SMTP dialog with RSET and QUIT.
     */
    nrcpt = 0;
    recv_state = send_state = SMTP_STATE_MAIL;
    next_rcpt = send_rcpt = recv_rcpt = 0;
    mail_from_rejected = 0;

    while (recv_state != SMTP_STATE_LAST) {

	/*
	 * Build the next command.
	 */
	switch (send_state) {

	    /*
	     * Sanity check.
	     */
	default:
	    msg_panic("%s: bad sender state %d", myname, send_state);

	    /*
	     * Build the MAIL FROM command.
	     */
	case SMTP_STATE_MAIL:
	    if (*request->sender)
		if (var_disable_dns == 0)
		    REWRITE_ADDRESS(request->sender);
	    vstring_sprintf(next_command, "MAIL FROM:<%s>", request->sender);
	    if (state->features & SMTP_FEATURE_SIZE)
		vstring_sprintf_append(next_command, " SIZE=%lu",
				       request->data_size);
	    next_state = SMTP_STATE_RCPT;
	    break;

	    /*
	     * Build one RCPT TO command before we have seen the MAIL FROM
	     * response.
	     */
	case SMTP_STATE_RCPT:
	    rcpt = request->rcpt_list.info + send_rcpt;
	    if (var_disable_dns == 0)
		REWRITE_ADDRESS(rcpt->address);
	    vstring_sprintf(next_command, "RCPT TO:<%s>", rcpt->address);
	    if ((next_rcpt = send_rcpt + 1) == request->rcpt_list.len)
		next_state = SMTP_STATE_DATA;
	    break;

	    /*
	     * Build the DATA command before we have seen all the RCPT TO
	     * responses.
	     */
	case SMTP_STATE_DATA:
	    vstring_strcpy(next_command, "DATA");
	    next_state = SMTP_STATE_DOT;
	    break;

	    /*
	     * Build the "." command before we have seen the DATA response.
	     */
	case SMTP_STATE_DOT:
	    vstring_strcpy(next_command, ".");
	    next_state = SMTP_STATE_QUIT;
	    break;

	    /*
	     * Can't happen. The SMTP_STATE_ABORT sender state is entered by
	     * the receiver and is left before the bottom of the main loop.
	     */
	case SMTP_STATE_ABORT:
	    msg_panic("%s: sender abort state", myname);

	    /*
	     * Build the QUIT command before we have seen the "." or RSET
	     * response.
	     */
	case SMTP_STATE_QUIT:
	    vstring_strcpy(next_command, "QUIT");
	    next_state = SMTP_STATE_LAST;
	    break;

	    /*
	     * The final sender state has no action associated with it.
	     */
	case SMTP_STATE_LAST:
	    VSTRING_RESET(next_command);
	    break;
	}
	VSTRING_TERMINATE(next_command);

	/*
	 * Process responses until the receiver has caught up. Vstreams
	 * automatically flush buffered output when reading new data.
	 * 
	 * Flush unsent output if command pipelining is off or if no I/O
	 * happened for a while. This limits the accumulation of client-side
	 * delays in pipelined sessions.
	 */
	if (SENDER_IN_WAIT_STATE
	    || (SENDER_IS_AHEAD
		&& (VSTRING_LEN(next_command) + 2 > sndbuffree
	    || time((time_t *) 0) - vstream_ftime(session->stream) > 10))) {
	    while (SENDER_IS_AHEAD) {

		/*
		 * Sanity check.
		 */
		if (recv_state < SMTP_STATE_MAIL
		    || recv_state > SMTP_STATE_QUIT)
		    msg_panic("%s: bad receiver state %d (sender state %d)",
			      myname, recv_state, send_state);

		/*
		 * Receive the next server response. Use the proper timeout,
		 * and log the proper client state in case of trouble.
		 */
		smtp_timeout_setup(state->session->stream,
				   *xfer_timeouts[recv_state]);
		if ((except = vstream_setjmp(state->session->stream)) != 0)
		    RETURN(SENDING_MAIL ? smtp_stream_except(state, except,
					     xfer_states[recv_state]) : -1);
		resp = smtp_chat_resp(state);

		/*
		 * Process the response.
		 */
		switch (recv_state) {

		    /*
		     * Process the MAIL FROM response. When the server
		     * rejects the sender, set the mail_from_rejected flag so
		     * that the receiver may apply a course correction.
		     */
		case SMTP_STATE_MAIL:
		    if (resp->code / 100 != 2) {
			smtp_mesg_fail(state, resp->code,
				       "host %s said: %s", session->namaddr,
				       translit(resp->str, "\n", " "));
			mail_from_rejected = 1;
		    }
		    recv_state = SMTP_STATE_RCPT;
		    break;

		    /*
		     * Process one RCPT TO response. If MAIL FROM was
		     * rejected, ignore RCPT TO responses: all recipients are
		     * dead already. When all recipients are rejected the
		     * receiver may apply a course correction.
		     * 
		     * XXX 2821: Section 4.5.3.1 says that a 552 RCPT TO reply
		     * must be treated as if the server replied with 452.
		     * However, this causes "too much mail data" to be
		     * treated as a recoverable error, which is wrong. I'll
		     * stick with RFC 821.
		     */
		case SMTP_STATE_RCPT:
		    if (!mail_from_rejected) {
#ifdef notdef
			if (resp->code == 552)
			    resp->code = 452;
#endif
			if (resp->code / 100 == 2) {
			    ++nrcpt;
			} else {
			    rcpt = request->rcpt_list.info + recv_rcpt;
			    smtp_rcpt_fail(state, resp->code, rcpt,
				       "host %s said: %s", session->namaddr,
					   translit(resp->str, "\n", " "));
			    rcpt->offset = 0;	/* in case deferred */
			}
		    }
		    if (++recv_rcpt == request->rcpt_list.len)
			recv_state = SMTP_STATE_DATA;
		    break;

		    /*
		     * Process the DATA response. When the server rejects
		     * DATA, set nrcpt to a negative value so that the
		     * receiver can apply a course correction.
		     */
		case SMTP_STATE_DATA:
		    if (resp->code / 100 != 3) {
			if (nrcpt > 0)
			    smtp_mesg_fail(state, resp->code,
				       "host %s said: %s", session->namaddr,
					   translit(resp->str, "\n", " "));
			nrcpt = -1;
		    }
		    recv_state = SMTP_STATE_DOT;
		    break;

		    /*
		     * Process the end of message response. Ignore the
		     * response when no recipient was accepted: all
		     * recipients are dead already, and the next receiver
		     * state is SMTP_STATE_QUIT regardless. Otherwise, if the
		     * message transfer fails, bounce all remaining
		     * recipients, else cross off the recipients that were
		     * delivered.
		     */
		case SMTP_STATE_DOT:
		    if (nrcpt > 0) {
			if (resp->code / 100 != 2) {
			    smtp_mesg_fail(state, resp->code,
					   "host %s said: %s",
					   session->namaddr,
					   translit(resp->str, "\n", " "));
			} else {
			    for (nrcpt = 0; nrcpt < recv_rcpt; nrcpt++) {
				rcpt = request->rcpt_list.info + nrcpt;
				if (rcpt->offset) {
				    sent(request->queue_id, rcpt->address,
					 session->namaddr,
					 request->arrival_time, "%s",
					 resp->str);
				    if (request->flags & DEL_REQ_FLAG_SUCCESS)
					deliver_completed(state->src, rcpt->offset);
				    rcpt->offset = 0;
				}
			    }
			}
		    }
		    recv_state = (var_skip_quit_resp ?
				  SMTP_STATE_LAST : SMTP_STATE_QUIT);
		    break;

		    /*
		     * Ignore the RSET response.
		     */
		case SMTP_STATE_ABORT:
		    recv_state = (var_skip_quit_resp ?
				  SMTP_STATE_LAST : SMTP_STATE_QUIT);
		    break;

		    /*
		     * Ignore the QUIT response.
		     */
		case SMTP_STATE_QUIT:
		    recv_state = SMTP_STATE_LAST;
		    break;
		}
	    }

	    /*
	     * At this point, the sender and receiver are fully synchronized,
	     * so that the entire TCP send buffer becomes available again.
	     */
	    sndbuffree = sndbufsize;

	    /*
	     * We know the server response to every command that was sent.
	     * Apply a course correction if necessary: the sender wants to
	     * send RCPT TO but MAIL FROM was rejected; the sender wants to
	     * send DATA but all recipients were rejected; the sender wants
	     * to deliver the message but DATA was rejected.
	     */
	    if ((send_state == SMTP_STATE_RCPT && mail_from_rejected)
		|| (send_state == SMTP_STATE_DATA && nrcpt == 0)
		|| (send_state == SMTP_STATE_DOT && nrcpt < 0)) {
		send_state = recv_state = SMTP_STATE_ABORT;
		send_rcpt = recv_rcpt = 0;
		vstring_strcpy(next_command, "RSET");
		next_state = SMTP_STATE_QUIT;
		next_rcpt = 0;
	    }
	}

	/*
	 * Make the next sender state the current sender state.
	 */
	if (send_state == SMTP_STATE_LAST)
	    continue;

	/*
	 * Special case if the server accepted the DATA command. If the
	 * server accepted at least one recipient send the entire message.
	 * Otherwise, just send "." as per RFC 2197.
	 */
	if (send_state == SMTP_STATE_DOT && nrcpt > 0) {
	    smtp_timeout_setup(state->session->stream,
			       var_smtp_data1_tmout);
	    if ((except = vstream_setjmp(state->session->stream)) != 0)
		RETURN(smtp_stream_except(state, except,
					  "sending message body"));

	    if (vstream_fseek(state->src, request->data_offset, SEEK_SET) < 0)
		msg_fatal("seek queue file: %m");

	    while ((rec_type = rec_get(state->src, state->scratch, 0)) > 0) {
		if (rec_type != REC_TYPE_NORM && rec_type != REC_TYPE_CONT)
		    break;
		if (prev_type != REC_TYPE_CONT)
		    if (vstring_str(state->scratch)[0] == '.')
			smtp_fputc('.', session->stream);
		if (var_smtp_break_lines)
		    rec_type = REC_TYPE_NORM;
		if (rec_type == REC_TYPE_CONT)
		    smtp_fwrite(vstring_str(state->scratch),
				VSTRING_LEN(state->scratch),
				session->stream);
		else
		    smtp_fputs(vstring_str(state->scratch),
			       VSTRING_LEN(state->scratch),
			       session->stream);
		prev_type = rec_type;
	    }

	    if (prev_type == REC_TYPE_CONT)	/* missing newline at end */
		smtp_fputs("", 0, session->stream);
	    if ((state->features & SMTP_FEATURE_MAYBEPIX) != 0
		&& request->arrival_time < vstream_ftime(session->stream)
		- var_smtp_pix_thresh) {
		msg_info("%s: enabling PIX <CRLF>.<CRLF> workaround for %s",
			 request->queue_id, session->namaddr);
		vstream_fflush(session->stream);/* hurts performance */
		sleep(var_smtp_pix_delay);	/* not to mention this */
	    }
	    if (vstream_ferror(state->src))
		msg_fatal("queue file read error");
	    if (rec_type != REC_TYPE_XTRA)
		RETURN(mark_corrupt(state->src));
	}

	/*
	 * Copy the next command to the buffer and update the sender state.
	 */
	if (sndbuffree > 0)
	    sndbuffree -= VSTRING_LEN(next_command) + 2;
	smtp_chat_cmd(state, "%s", vstring_str(next_command));
	send_state = next_state;
	send_rcpt = next_rcpt;
    }

    RETURN(0);
}
