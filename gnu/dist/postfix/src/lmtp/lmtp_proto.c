/*++
/* NAME
/*	lmtp_proto 3
/* SUMMARY
/*	client LMTP protocol
/* SYNOPSIS
/*	#include "lmtp.h"
/*
/*	int	lmtp_lhlo(state)
/*	LMTP_STATE *state;
/*
/*	int	lmtp_xfer(state)
/*	LMTP_STATE *state;
/*
/*	int	lmtp_rset(state)
/*	LMTP_STATE *state;
/*
/*	int	lmtp_quit(state)
/*	LMTP_STATE *state;
/* DESCRIPTION
/*	This module implements the client side of the LMTP protocol.
/*
/*	lmtp_lhlo() performs the initial handshake with the LMTP server.
/*
/*	lmtp_xfer() sends message envelope information followed by the
/*	message data and sends QUIT when connection cacheing is disabled.
/*	These operations are combined in one function, in order to implement
/*	LMTP pipelining.
/*	Recipients are marked as "done" in the mail queue file when
/*	bounced or delivered. The message delivery status is updated
/*	accordingly.
/*
/*	lmtp_rset() sends a lone RSET command and waits for the response.
/*
/*	lmtp_quit() sends a lone QUIT command and waits for the response
/*	only if waiting for QUIT replies is enabled.
/* DIAGNOSTICS
/*	lmtp_lhlo(), lmtp_xfer(), lmtp_rset() and lmtp_quit() return 0 in
/*	case of success, -1 in case of failure. For lmtp_xfer(), lmtp_rset()
/*	and lmtp_quit(), success means the ability to perform an LMTP
/*	conversation, not necessarily OK replies from the server.
/*
/*	Warnings: corrupt message file. A corrupt message is marked
/*	as "corrupt" by changing its queue file permissions.
/* SEE ALSO
/*	lmtp(3h) internal data structures
/*	lmtp_chat(3) query/reply LMTP support
/*	lmtp_trouble(3) error handlers
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
/*	Alterations for LMTP by:
/*	Philip A. Prindeville
/*	Mirapoint, Inc.
/*	USA.
/*
/*	Additional work on LMTP by:
/*	Amos Gouaux
/*	University of Texas at Dallas
/*	P.O. Box 830688, MC34
/*	Richardson, TX 75083, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <sys/stat.h>
#include <sys/socket.h>			/* shutdown(2) */
#include <errno.h>
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

#include "lmtp.h"
#include "lmtp_sasl.h"

 /*
  * Sender and receiver state. A session does not necessarily go through a
  * linear progression, but states are guaranteed to not jump backwards.
  * Normal sessions go from MAIL->RCPT->DATA->DOT->QUIT->LAST. The states
  * MAIL, RCPT, and DATA may also be followed by ABORT->QUIT->LAST.
  * 
  * When connection cacheing is turned on, the transition diagram changes as
  * follows. Before sending mail over an existing connection, the client
  * sends a lone RSET command to find out if the connection still works. The
  * client sends QUIT only when closing a connection. The respective state
  * transitions are RSET->LAST and QUIT->LAST.
  * 
  * For the sake of code reuse, the non-pipelined RSET command is sent by the
  * same code that implements command pipelining, so that we can borrow from
  * the existing code for exception handling and error reporting.
  * 
  */
#define LMTP_STATE_MAIL		0
#define LMTP_STATE_RCPT		1
#define LMTP_STATE_DATA		2
#define LMTP_STATE_DOT		3
#define LMTP_STATE_ABORT	4
#define LMTP_STATE_RSET		5
#define LMTP_STATE_QUIT		6
#define LMTP_STATE_LAST		7

int    *xfer_timeouts[LMTP_STATE_LAST] = {
    &var_lmtp_mail_tmout,
    &var_lmtp_rcpt_tmout,
    &var_lmtp_data0_tmout,
    &var_lmtp_data2_tmout,
    &var_lmtp_rset_tmout,
    &var_lmtp_rset_tmout,
    &var_lmtp_quit_tmout,
};

char   *xfer_states[LMTP_STATE_LAST] = {
    "sending MAIL FROM",
    "sending RCPT TO",
    "sending DATA command",
    "sending end of data -- message may be sent more than once",
    "sending RSET",
    "sending RSET",
    "sending QUIT",
};

/* lmtp_lhlo - perform initial handshake with LMTP server */

int     lmtp_lhlo(LMTP_STATE *state)
{
    LMTP_SESSION *session = state->session;
    LMTP_RESP *resp;
    int     except;
    char   *lines;
    char   *words;
    char   *word;

    /*
     * Prepare for disaster.
     */
    smtp_timeout_setup(state->session->stream, var_lmtp_lhlo_tmout);
    if ((except = vstream_setjmp(state->session->stream)) != 0)
	return (lmtp_stream_except(state, except, "sending LHLO"));

    /*
     * Read and parse the server's LMTP greeting banner.
     */
    if (((resp = lmtp_chat_resp(state))->code / 100) != 2)
	return (lmtp_site_fail(state, resp->code,
			       "host %s refused to talk to me: %s",
			 session->namaddr, translit(resp->str, "\n", " ")));

    /*
     * Return the compliment.
     */
    lmtp_chat_cmd(state, "LHLO %s", var_myhostname);
    if ((resp = lmtp_chat_resp(state))->code / 100 != 2)
	return (lmtp_site_fail(state, resp->code,
			       "host %s refused to talk to me: %s",
			       session->namaddr,
			       translit(resp->str, "\n", " ")));

    /*
     * Pick up some useful features offered by the LMTP server. XXX Until we
     * have a portable routine to convert from string to off_t with proper
     * overflow detection, ignore the message size limit advertised by the
     * LMTP server. Otherwise, we might do the wrong thing when the server
     * advertises a really huge message size limit.
     */
    lines = resp->str;
    (void) mystrtok(&lines, "\n");
    while ((words = mystrtok(&lines, "\n")) != 0) {
	if (mystrtok(&words, "- ") && (word = mystrtok(&words, " \t")) != 0) {
	    if (strcasecmp(word, "8BITMIME") == 0)
		state->features |= LMTP_FEATURE_8BITMIME;
	    else if (strcasecmp(word, "PIPELINING") == 0)
		state->features |= LMTP_FEATURE_PIPELINING;
	    else if (strcasecmp(word, "SIZE") == 0)
		state->features |= LMTP_FEATURE_SIZE;
#ifdef USE_SASL_AUTH
	    else if (var_lmtp_sasl_enable && strcasecmp(word, "AUTH") == 0)
		lmtp_sasl_helo_auth(state, words);
#endif
	}
    }
    if (msg_verbose)
	msg_info("server features: 0x%x", state->features);

#ifdef USE_SASL_AUTH
    if (var_lmtp_sasl_enable && (state->features & SMTP_FEATURE_AUTH))
	return (lmtp_sasl_helo_login(state));
#endif

    /*
     * We use LMTP command pipelining if the server said it supported it.
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
     * 
     * XXX Apparently, the getsockopt() call causes trouble with UNIX-domain
     * sockets. Don't worry about command pipelining for local connections,
     * because they benefit little from pipelining.
     */
    if (state->features & LMTP_FEATURE_PIPELINING) {
	state->sndbufsize = 4 * 1024;
	if (msg_verbose)
	    msg_info("Using LMTP PIPELINING, send buffer size is %d",
		     state->sndbufsize);
    } else
	state->sndbufsize = 0;

    return (0);
}

/* lmtp_loop - the LMTP state machine */

static int lmtp_loop(LMTP_STATE *state, int send_state, int recv_state)
{
    char   *myname = "lmtp_loop";
    DELIVER_REQUEST *request = state->request;
    LMTP_SESSION *session = state->session;
    LMTP_RESP *resp;
    RECIPIENT *rcpt;
    VSTRING *next_command = vstring_alloc(100);
    int    *survivors = 0;
    int     next_state;
    int     next_rcpt;
    int     send_rcpt;
    int     recv_rcpt;
    int     nrcpt;
    int     except;
    int     rec_type;
    int     prev_type = 0;
    int     sndbuffree;
    int     mail_from_rejected;
    int     recv_dot;

    /*
     * Macros for readability. XXX Aren't LMTP addresses supposed to be case
     * insensitive?
     */
#define REWRITE_ADDRESS(addr) do { \
	  if (*(addr)) { \
	      quote_821_local(state->scratch, addr); \
	      myfree(addr); \
	      addr = mystrdup(vstring_str(state->scratch)); \
	      lowercase(addr); \
	  } \
    } while (0)

#define RETURN(x) do { \
	  vstring_free(next_command); \
	  if (survivors) \
	      myfree((char *) survivors); \
	  return (x); \
    } while (0)

#define SENDER_IS_AHEAD \
	(recv_state < send_state || recv_rcpt != send_rcpt)

#define SENDER_IN_WAIT_STATE \
	(send_state == LMTP_STATE_DOT || send_state == LMTP_STATE_LAST)

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
     * LMTP dialog with RSET.
     */
    nrcpt = 0;
    next_rcpt = send_rcpt = recv_rcpt = recv_dot = 0;
    mail_from_rejected = 0;
    sndbuffree = state->sndbufsize;

    while (recv_state != LMTP_STATE_LAST) {

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
	case LMTP_STATE_MAIL:
	    if (*request->sender)
		REWRITE_ADDRESS(request->sender);
	    vstring_sprintf(next_command, "MAIL FROM:<%s>", request->sender);
	    if (state->features & LMTP_FEATURE_SIZE)
		vstring_sprintf_append(next_command, " SIZE=%lu",
				       request->data_size);
	    next_state = LMTP_STATE_RCPT;
	    break;

	    /*
	     * Build one RCPT TO command before we have seen the MAIL FROM
	     * response.
	     */
	case LMTP_STATE_RCPT:
	    rcpt = request->rcpt_list.info + send_rcpt;
	    REWRITE_ADDRESS(rcpt->address);
	    vstring_sprintf(next_command, "RCPT TO:<%s>", rcpt->address);
	    if ((next_rcpt = send_rcpt + 1) == request->rcpt_list.len)
		next_state = LMTP_STATE_DATA;
	    break;

	    /*
	     * Build the DATA command before we have seen all the RCPT TO
	     * responses.
	     */
	case LMTP_STATE_DATA:
	    vstring_strcpy(next_command, "DATA");
	    next_state = LMTP_STATE_DOT;
	    break;

	    /*
	     * Build the "." command before we have seen the DATA response.
	     */
	case LMTP_STATE_DOT:
	    vstring_strcpy(next_command, ".");
	    next_state = var_lmtp_cache_conn ?
		LMTP_STATE_LAST : LMTP_STATE_QUIT;
	    break;

	    /*
	     * Can't happen. The LMTP_STATE_ABORT sender state is entered by
	     * the receiver and is left before the bottom of the main loop.
	     */
	case LMTP_STATE_ABORT:
	    msg_panic("%s: sender abort state", myname);

	    /*
	     * Build the RSET command.  This command does not really belong
	     * here because it is always sent without pipelining, but having
	     * it here means that we can reuse a lot of error handling code
	     * that already exists.
	     */
	case LMTP_STATE_RSET:
	    vstring_strcpy(next_command, "RSET");
	    next_state = LMTP_STATE_LAST;
	    break;

	    /*
	     * Build the QUIT command.
	     */
	case LMTP_STATE_QUIT:
	    vstring_strcpy(next_command, "QUIT");
	    next_state = LMTP_STATE_LAST;
	    break;

	    /*
	     * The final sender state has no action associated with it.
	     */
	case LMTP_STATE_LAST:
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
		if (recv_state < LMTP_STATE_MAIL
		    || recv_state > LMTP_STATE_QUIT)
		    msg_panic("%s: bad receiver state %d (sender state %d)",
			      myname, recv_state, send_state);

		/*
		 * Receive the next server response. Use the proper timeout,
		 * and log the proper client state in case of trouble.
		 */
		smtp_timeout_setup(state->session->stream,
				   *xfer_timeouts[recv_state]);
		if ((except = vstream_setjmp(state->session->stream)) != 0)
		    RETURN(lmtp_stream_except(state, except,
					      xfer_states[recv_state]));
		resp = lmtp_chat_resp(state);

		/*
		 * Process the response.
		 */
		switch (recv_state) {

		    /*
		     * Process the MAIL FROM response. When the server
		     * rejects the sender, set the mail_from_rejected flag so
		     * that the receiver may apply a course correction.
		     */
		case LMTP_STATE_MAIL:
		    if (resp->code / 100 != 2) {
			lmtp_mesg_fail(state, resp->code,
				       "host %s said: %s", session->namaddr,
				       translit(resp->str, "\n", " "));
			mail_from_rejected = 1;
		    }
		    recv_state = LMTP_STATE_RCPT;
		    break;

		    /*
		     * Process one RCPT TO response. If MAIL FROM was
		     * rejected, ignore RCPT TO responses: all recipients are
		     * dead already. When all recipients are rejected the
		     * receiver may apply a course correction.
		     */
		case LMTP_STATE_RCPT:
		    if (!mail_from_rejected) {
			rcpt = request->rcpt_list.info + recv_rcpt;
			if (resp->code / 100 == 2) {
			    if (survivors == 0)
				survivors = (int *)
				    mymalloc(request->rcpt_list.len
					     * sizeof(int));
			    survivors[nrcpt++] = recv_rcpt;
			} else {
			    lmtp_rcpt_fail(state, resp->code, rcpt,
				       "host %s said: %s", session->namaddr,
					   translit(resp->str, "\n", " "));
			    rcpt->offset = 0;	/* in case deferred */
			}
		    }
		    if (++recv_rcpt == request->rcpt_list.len)
			recv_state = LMTP_STATE_DATA;
		    break;

		    /*
		     * Process the DATA response. When the server rejects
		     * DATA, set nrcpt to a negative value so that the
		     * receiver can apply a course correction.
		     */
		case LMTP_STATE_DATA:
		    if (resp->code / 100 != 3) {
			if (nrcpt > 0)
			    lmtp_mesg_fail(state, resp->code,
				       "host %s said: %s", session->namaddr,
					   translit(resp->str, "\n", " "));
			nrcpt = -1;
		    }
		    recv_state = LMTP_STATE_DOT;
		    break;

		    /*
		     * Process the end of message response. Ignore the
		     * response when no recipient was accepted: all
		     * recipients are dead already, and the next receiver
		     * state is LMTP_STATE_LAST regardless. Otherwise, if the
		     * message transfer fails, bounce all remaining
		     * recipients, else cross off the recipients that were
		     * delivered.
		     */
		case LMTP_STATE_DOT:
		    if (nrcpt > 0) {
			rcpt = request->rcpt_list.info + survivors[recv_dot];
			if (resp->code / 100 == 2) {
			    if (rcpt->offset) {
				sent(request->queue_id, rcpt->address,
				     session->namaddr, request->arrival_time,
				     "%s", resp->str);
				if (request->flags & DEL_REQ_FLAG_SUCCESS)
				    deliver_completed(state->src, rcpt->offset);
				rcpt->offset = 0;
			    }
			} else {
			    lmtp_rcpt_fail(state, resp->code, rcpt,
				       "host %s said: %s", session->namaddr,
					   translit(resp->str, "\n", " "));
			    rcpt->offset = 0;	/* in case deferred */
			}
		    }

		    /*
		     * We get one response per valid RCPT TO:
		     */
		    if (msg_verbose)
			msg_info("%s: recv_dot = %d", myname, recv_dot);
		    if (++recv_dot >= nrcpt) {
			if (msg_verbose)
			    msg_info("%s: finished . command", myname);
			recv_state = var_lmtp_skip_quit_resp || var_lmtp_cache_conn ?
			    LMTP_STATE_LAST : LMTP_STATE_QUIT;
		    }
		    break;

		    /*
		     * Ignore the RSET response.
		     */
		case LMTP_STATE_ABORT:
		    recv_state = var_lmtp_skip_quit_resp || var_lmtp_cache_conn ?
			LMTP_STATE_LAST : LMTP_STATE_QUIT;
		    break;

		    /*
		     * Ignore the RSET response.
		     */
		case LMTP_STATE_RSET:
		    recv_state = LMTP_STATE_LAST;
		    break;

		    /*
		     * Ignore the QUIT response.
		     */
		case LMTP_STATE_QUIT:
		    recv_state = LMTP_STATE_LAST;
		    break;
		}
	    }

	    /*
	     * At this point, the sender and receiver are fully synchronized,
	     * so that the entire TCP send buffer becomes available again.
	     */
	    sndbuffree = state->sndbufsize;

	    /*
	     * We know the server response to every command that was sent.
	     * Apply a course correction if necessary: the sender wants to
	     * send RCPT TO but MAIL FROM was rejected; the sender wants to
	     * send DATA but all recipients were rejected; the sender wants
	     * to deliver the message but DATA was rejected.
	     */
	    if ((send_state == LMTP_STATE_RCPT && mail_from_rejected)
		|| (send_state == LMTP_STATE_DATA && nrcpt == 0)
		|| (send_state == LMTP_STATE_DOT && nrcpt < 0)) {
		send_state = recv_state = LMTP_STATE_ABORT;
		send_rcpt = recv_rcpt = 0;
		vstring_strcpy(next_command, "RSET");
		next_state = var_lmtp_cache_conn ?
		    LMTP_STATE_LAST : LMTP_STATE_QUIT;
		next_rcpt = 0;
	    }
	}

	/*
	 * Make the next sender state the current sender state.
	 */
	if (send_state == LMTP_STATE_LAST)
	    continue;

	/*
	 * Special case if the server accepted the DATA command. If the
	 * server accepted at least one recipient send the entire message.
	 * Otherwise, just send "." as per RFC 2197.
	 */
	if (send_state == LMTP_STATE_DOT && nrcpt > 0) {
	    smtp_timeout_setup(state->session->stream,
			       var_lmtp_data1_tmout);
	    if ((except = vstream_setjmp(state->session->stream)) != 0)
		RETURN(lmtp_stream_except(state, except,
					  "sending message body"));

	    if (vstream_fseek(state->src, request->data_offset, SEEK_SET) < 0)
		msg_fatal("seek queue file: %m");

	    while ((rec_type = rec_get(state->src, state->scratch, 0)) > 0) {
		if (rec_type != REC_TYPE_NORM && rec_type != REC_TYPE_CONT)
		    break;
		if (prev_type != REC_TYPE_CONT)
		    if (vstring_str(state->scratch)[0] == '.')
			smtp_fputc('.', session->stream);
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
	    if (vstream_ferror(state->src))
		msg_fatal("queue file read error");
	    if (rec_type != REC_TYPE_XTRA)
		RETURN(mark_corrupt(state->src));
	}

	/*
	 * Copy the next command to the buffer and update the sender state.
	 */
	if (state->sndbuffree > 0)
	    state->sndbuffree -= VSTRING_LEN(next_command) + 2;
	lmtp_chat_cmd(state, "%s", vstring_str(next_command));
	send_state = next_state;
	send_rcpt = next_rcpt;
    }

    RETURN(0);
}

/* lmtp_xfer - send a batch of envelope information and the message data */

int     lmtp_xfer(LMTP_STATE *state)
{
    return (lmtp_loop(state, LMTP_STATE_MAIL, LMTP_STATE_MAIL));
}

/* lmtp_rset - send a lone RSET command and wait for response */

int     lmtp_rset(LMTP_STATE *state)
{
    return (lmtp_loop(state, LMTP_STATE_RSET, LMTP_STATE_RSET));
}

/* lmtp_quit - send a lone QUIT command */

int     lmtp_quit(LMTP_STATE *state)
{
    return (lmtp_loop(state, LMTP_STATE_QUIT, var_lmtp_skip_quit_resp ?
		      LMTP_STATE_LAST : LMTP_STATE_QUIT));
}
