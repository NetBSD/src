/*	$NetBSD: postscreen_send.c,v 1.1.1.3 2013/01/02 18:59:04 tron Exp $	*/

/*++
/* NAME
/*	postscreen_send 3
/* SUMMARY
/*	postscreen low-level output
/* SYNOPSIS
/*	#include <postscreen.h>
/*
/*	int	psc_send_reply(state, text)
/*	PSC_STATE *state;
/*	const char *text;
/*
/*	int	PSC_SEND_REPLY(state, text)
/*	PSC_STATE *state;
/*	const char *text;
/*
/*	void	psc_send_socket(state)
/*	PSC_STATE *state;
/* DESCRIPTION
/*	psc_send_reply() sends the specified text to the specified
/*	remote SMTP client.  In case of an immediate error, it logs
/*	a warning (except EPIPE) with the client address and port,
/*	and returns a non-zero result (all errors including EPIPE).
/*
/*	psc_send_reply() does a best effort to send the reply, but
/*	it won't block when the output is throttled by a hostile
/*	peer.
/*
/*	PSC_SEND_REPLY() is a legacy wrapper for psc_send_reply().
/*	It will eventually be replaced by its expansion.
/*
/*	psc_send_socket() sends the specified socket to the real
/*	Postfix SMTP server. The socket is delivered in the background.
/*	This function must be called after all other session-related
/*	work is finished including postscreen cache updates.
/*
/*	In case of an immediate error, psc_send_socket() sends a 421
/*	reply to the remote SMTP client and closes the connection.
/*	If the 220- greeting was sent, sending 421 would be invalid;
/*	instead, the client is redirected to the dummy SMTP engine
/*	which sends the 421 reply at the first legitimate opportunity.
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
#include <string.h>
#include <errno.h>

/* Utility library. */

#include <msg.h>
#include <iostuff.h>
#include <connect.h>

/* Global library. */

#include <mail_params.h>
#include <smtp_reply_footer.h>

/* Application-specific. */

#include <postscreen.h>

 /*
  * This program screens all inbound SMTP connections, so it better not waste
  * time.
  */
#define PSC_SEND_SOCK_CONNECT_TIMEOUT	1
#define PSC_SEND_SOCK_NOTIFY_TIMEOUT	100

/* psc_send_reply - send reply to remote SMTP client */

int     psc_send_reply(PSC_STATE *state, const char *text)
{
    ssize_t start;
    int     ret;

    if (msg_verbose)
	msg_info("> [%s]:%s: %.*s", state->smtp_client_addr,
		 state->smtp_client_port, (int) strlen(text) - 2, text);

    /*
     * Append the new text to earlier text that could not be sent because the
     * output was throttled.
     */
    start = VSTRING_LEN(state->send_buf);
    vstring_strcat(state->send_buf, text);

    /*
     * For soft_bounce support, we also fix the REJECT logging before the
     * dummy SMTP engine calls the psc_send_reply() output routine. We do
     * some double work, but it is for debugging only.
     */
    if (var_soft_bounce) {
	if (text[0] == '5')
	    STR(state->send_buf)[start + 0] = '4';
	if (text[4] == '5')
	    STR(state->send_buf)[start + 4] = '4';
    }

    /*
     * Append the optional reply footer.
     */
    if (*var_psc_rej_footer && (*text == '4' || *text == '5'))
	smtp_reply_footer(state->send_buf, start, var_psc_rej_footer,
			  STR(psc_expand_filter), psc_expand_lookup,
			  (char *) state);

    /*
     * Do a best effort sending text, but don't block when the output is
     * throttled by a hostile peer.
     */
    ret = write(vstream_fileno(state->smtp_client_stream),
		STR(state->send_buf), LEN(state->send_buf));
    if (ret > 0)
	vstring_truncate(state->send_buf, ret - LEN(state->send_buf));
    if (ret < 0 && errno != EAGAIN && errno != EPIPE && errno != ECONNRESET)
	msg_warn("write [%s]:%s: %m", state->smtp_client_addr,
		 state->smtp_client_port);
    return (ret < 0 && errno != EAGAIN);
}

/* psc_send_socket_close_event - file descriptor has arrived or timeout */

static void psc_send_socket_close_event(int event, char *context)
{
    const char *myname = "psc_send_socket_close_event";
    PSC_STATE *state = (PSC_STATE *) context;

    if (msg_verbose > 1)
	msg_info("%s: sq=%d cq=%d event %d on send socket %d from [%s]:%s",
		 myname, psc_post_queue_length, psc_check_queue_length,
		 event, state->smtp_server_fd, state->smtp_client_addr,
		 state->smtp_client_port);

    /*
     * The real SMTP server has closed the local IPC channel, or we have
     * reached the limit of our patience. In the latter case it is still
     * possible that the real SMTP server will receive the socket so we
     * should not interfere.
     */
    PSC_CLEAR_EVENT_REQUEST(state->smtp_server_fd, psc_send_socket_close_event,
			    context);
    if (event == EVENT_TIME)
	msg_warn("timeout sending connection to service %s",
		 psc_smtpd_service_name);
    psc_free_session_state(state);
}

/* psc_send_socket - send socket to real SMTP server process */

void    psc_send_socket(PSC_STATE *state)
{
    const char *myname = "psc_send_socket";
    int     server_fd;

    if (msg_verbose > 1)
	msg_info("%s: sq=%d cq=%d send socket %d from [%s]:%s",
		 myname, psc_post_queue_length, psc_check_queue_length,
		 vstream_fileno(state->smtp_client_stream),
		 state->smtp_client_addr, state->smtp_client_port);

    /*
     * Connect to the real SMTP service over a local IPC channel, send the
     * file descriptor, and close the file descriptor to save resources.
     * Experience has shown that some systems will discard information when
     * we close a channel immediately after writing. Thus, we waste resources
     * waiting for the remote side to close the local IPC channel first. The
     * good side of waiting is that we learn when the real SMTP server is
     * falling behind.
     * 
     * This is where we would forward the connection to an SMTP server that
     * provides an appropriate level of service for this client class. For
     * example, a server that is more forgiving, or one that is more
     * suspicious. Alternatively, we could send attributes along with the
     * socket with client reputation information, making everything even more
     * Postfix-specific.
     */
    if ((server_fd =
	 PASS_CONNECT(psc_smtpd_service_name, NON_BLOCKING,
		      PSC_SEND_SOCK_CONNECT_TIMEOUT)) < 0) {
	msg_warn("cannot connect to service %s: %m", psc_smtpd_service_name);
	if (state->flags & PSC_STATE_FLAG_PREGR_TODO) {
	    PSC_SMTPD_X21(state, "421 4.3.2 No system resources\r\n");
	} else {
	    PSC_SEND_REPLY(state, "421 4.3.2 All server ports are busy\r\n");
	    psc_free_session_state(state);
	}
	return;
    }
    if (LOCAL_SEND_FD(server_fd,
		      vstream_fileno(state->smtp_client_stream)) < 0) {
	msg_warn("cannot pass connection to service %s: %m",
		 psc_smtpd_service_name);
	(void) close(server_fd);
	if (state->flags & PSC_STATE_FLAG_PREGR_TODO) {
	    PSC_SMTPD_X21(state, "421 4.3.2 No system resources\r\n");
	} else {
	    PSC_SEND_REPLY(state, "421 4.3.2 No system resources\r\n");
	    psc_free_session_state(state);
	}
	return;
    } else {

	/*
	 * Closing the smtp_client_fd here triggers a FreeBSD 7.1 kernel bug
	 * where smtp-source sometimes sees the connection being closed after
	 * it has already received the real SMTP server's 220 greeting!
	 */
#if 0
	PSC_DEL_CLIENT_STATE(state);
#endif
	PSC_ADD_SERVER_STATE(state, server_fd);
	PSC_READ_EVENT_REQUEST(state->smtp_server_fd, psc_send_socket_close_event,
			       (char *) state, PSC_SEND_SOCK_NOTIFY_TIMEOUT);
	return;
    }
}
