/*++
/* NAME
/*	smtp_proto 3
/* SUMMARY
/*	client SMTP/LMTP protocol
/* SYNOPSIS
/*	#include "smtp.h"
/*
/*	int	smtp_helo(state)
/*	SMTP_STATE *state;
/*
/*	int	smtp_xfer(state)
/*	SMTP_STATE *state;
/*
/*	int	smtp_rset(state)
/*	SMTP_STATE *state;
/*
/*	int	smtp_quit(state)
/*	SMTP_STATE *state;
/* DESCRIPTION
/*	In the subsequent text, SMTP implies LMTP.
/*	This module implements the client side of the SMTP protocol.
/*
/*	smtp_helo() performs the initial handshake with the SMTP server.
/*	When TLS is enabled, this includes STARTTLS negotiations.
/*
/*	smtp_xfer() sends message envelope information followed by the
/*	message data, and finishes the SMTP conversation. These operations
/*	are combined in one function, in order to implement SMTP pipelining.
/*	Recipients are marked as "done" in the mail queue file when
/*	bounced or delivered. The message delivery status is updated
/*	accordingly.
/*
/*	smtp_rset() sends a single RSET command and waits for the
/*	response. In case of a negative reply it sets the
/*	CANT_RSET_THIS_SESSION flag.
/*
/*	smtp_quit() sends a single QUIT command and waits for the
/*	response if configured to do so. It always turns off connection
/*	caching.
/* DIAGNOSTICS
/*	smtp_helo(), smtp_xfer(), smtp_rset() and smtp_quit() return
/*	0 in case of success, -1 in case of failure. For smtp_xfer(),
/*	smtp_rset() and smtp_quit(), success means the ability to
/*	perform an SMTP conversation, not necessarily the ability
/*	to deliver mail, or the achievement of server happiness.
/*
/*	In case of a rejected or failed connection, a connection
/*	is marked as "bad, do not cache". Otherwise, connection
/*	caching may be turned off (without being marked "bad") at
/*	the discretion of the code that implements the individual
/*	protocol steps.
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
/*	Connection caching in cooperation with:
/*	Victor Duchovni
/*	Morgan Stanley
/*
/*	TLS support originally by:
/*	Lutz Jaenicke
/*	BTU Cottbus
/*	Allgemeine Elektrotechnik
/*	Universitaetsplatz 3-4
/*	D-03044 Cottbus, Germany
/*--*/

/* System library. */

#include <sys_defs.h>
#include <sys/stat.h>
#include <sys/socket.h>			/* shutdown(2) */
#include <netinet/in.h>			/* ntohs() */
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
#include <split_at.h>
#include <name_code.h>
#include <name_mask.h>

/* Global library. */

#include <mail_params.h>
#include <smtp_stream.h>
#include <mail_queue.h>
#include <recipient_list.h>
#include <deliver_request.h>
#include <defer.h>
#include <bounce.h>
#include <record.h>
#include <rec_type.h>
#include <off_cvt.h>
#include <mark_corrupt.h>
#include <quote_821_local.h>
#include <quote_822_local.h>
#include <mail_proto.h>
#include <mime_state.h>
#include <ehlo_mask.h>
#include <maps.h>
#include <tok822.h>
#include <mail_addr_map.h>
#include <ext_prop.h>
#include <lex_822.h>
#include <dsn_mask.h>
#include <xtext.h>

/* Application-specific. */

#include "smtp.h"
#include "smtp_sasl.h"

 /*
  * Sender and receiver state. A session does not necessarily go through a
  * linear progression, but states are guaranteed to not jump backwards.
  * Normal sessions go from MAIL->RCPT->DATA->DOT->QUIT->LAST. The states
  * MAIL, RCPT, and DATA may also be followed by ABORT->QUIT->LAST.
  * 
  * When connection caching is enabled, the QUIT state is suppressed. Normal
  * sessions proceed as MAIL->RCPT->DATA->DOT->LAST, while aborted sessions
  * end with ABORT->LAST. The connection is left open for a limited time. An
  * RSET probe should be sent before attempting to reuse an open connection
  * for a new transaction.
  * 
  * The code to send an RSET probe is a special case with its own initial state
  * and with its own dedicated state transitions. The session proceeds as
  * RSET->LAST. This code is kept inside the main protocol engine for
  * consistent error handling and error reporting. It is not to be confused
  * with the code that sends RSET to abort a mail transaction in progress.
  * 
  * The code to send QUIT without message delivery transaction jumps into the
  * main state machine. If this introduces complications, then we should
  * introduce a second QUIT state with its own dedicated state transitions,
  * just like we did for RSET probes.
  * 
  * By default, the receiver skips the QUIT response. Some SMTP servers
  * disconnect after responding to ".", and some SMTP servers wait before
  * responding to QUIT.
  * 
  * Client states that are associated with sending mail (up to and including
  * SMTP_STATE_DOT) must have smaller numerical values than the non-sending
  * states (SMTP_STATE_ABORT .. SMTP_STATE_LAST).
  */
#define SMTP_STATE_XFORWARD_NAME_ADDR 0
#define SMTP_STATE_XFORWARD_PROTO_HELO 1
#define SMTP_STATE_MAIL		2
#define SMTP_STATE_RCPT		3
#define SMTP_STATE_DATA		4
#define SMTP_STATE_DOT		5
#define SMTP_STATE_ABORT	6
#define SMTP_STATE_RSET		7
#define SMTP_STATE_QUIT		8
#define SMTP_STATE_LAST		9

int    *xfer_timeouts[SMTP_STATE_LAST] = {
    &var_smtp_xfwd_tmout,		/* name/addr */
    &var_smtp_xfwd_tmout,		/* helo/proto */
    &var_smtp_mail_tmout,
    &var_smtp_rcpt_tmout,
    &var_smtp_data0_tmout,
    &var_smtp_data2_tmout,
    &var_smtp_rset_tmout,
    &var_smtp_rset_tmout,
    &var_smtp_quit_tmout,
};

char   *xfer_states[SMTP_STATE_LAST] = {
    "sending XFORWARD name/address",
    "sending XFORWARD protocol/helo_name",
    "sending MAIL FROM",
    "sending RCPT TO",
    "sending DATA command",
    "sending end of data -- message may be sent more than once",
    "sending final RSET",
    "sending RSET probe",
    "sending QUIT",
};

char   *xfer_request[SMTP_STATE_LAST] = {
    "XFORWARD name/address command",
    "XFORWARD helo/protocol command",
    "MAIL FROM command",
    "RCPT TO command",
    "DATA command",
    "end of DATA command",
    "final RSET command",
    "RSET probe",
    "QUIT command",
};

#define SMTP_MIME_DOWNGRADE(session, request) \
    (var_disable_mime_oconv == 0 \
     && (session->features & SMTP_FEATURE_8BITMIME) == 0 \
     && strcmp(request->encoding, MAIL_ATTR_ENC_7BIT) != 0)

static int smtp_start_tls(SMTP_STATE *);

 /*
  * Call-back information for header/body checks. We don't provide call-backs
  * for actions that change the message delivery time or destination.
  */
static void smtp_hbc_logger(void *, const char *, const char *, const char *, const char *);
static void smtp_text_out(void *, int, const char *, ssize_t, off_t);

HBC_CALL_BACKS smtp_hbc_callbacks[1] = {
    smtp_hbc_logger,
    smtp_text_out,
};

/* smtp_helo - perform initial handshake with SMTP server */

int     smtp_helo(SMTP_STATE *state)
{
    const char *myname = "smtp_helo";
    SMTP_SESSION *session = state->session;
    DELIVER_REQUEST *request = state->request;
    SMTP_RESP *resp;
    SMTP_RESP fake;
    int     except;
    char   *lines;
    char   *words;
    char   *word;
    int     n;
    static const NAME_CODE xforward_features[] = {
	XFORWARD_NAME, SMTP_FEATURE_XFORWARD_NAME,
	XFORWARD_ADDR, SMTP_FEATURE_XFORWARD_ADDR,
	XFORWARD_PORT, SMTP_FEATURE_XFORWARD_PORT,
	XFORWARD_PROTO, SMTP_FEATURE_XFORWARD_PROTO,
	XFORWARD_HELO, SMTP_FEATURE_XFORWARD_HELO,
	XFORWARD_DOMAIN, SMTP_FEATURE_XFORWARD_DOMAIN,
	0, 0,
    };
    SOCKOPT_SIZE optlen;
    const char *ehlo_words;
    int     discard_mask;
    static const NAME_MASK pix_bug_table[] = {
	PIX_BUG_DISABLE_ESMTP, SMTP_FEATURE_PIX_NO_ESMTP,
	PIX_BUG_DELAY_DOTCRLF, SMTP_FEATURE_PIX_DELAY_DOTCRLF,
	0,
    };
    const char *pix_bug_words;
    const char *pix_bug_source;
    int     pix_bug_mask;

#ifdef USE_TLS
    int     saved_features = session->features;
    int     tls_helo_status;

#endif
    const char *NOCLOBBER where;

    /*
     * Prepare for disaster.
     */
    smtp_timeout_setup(state->session->stream, var_smtp_helo_tmout);
    if ((except = vstream_setjmp(state->session->stream)) != 0)
	return (smtp_stream_except(state, except, where));

    /*
     * If not recursing after STARTTLS, examine the server greeting banner
     * and decide if we are going to send EHLO as the next command.
     */
    if ((state->misc_flags & SMTP_MISC_FLAG_IN_STARTTLS) == 0) {

	/*
	 * Read and parse the server's SMTP greeting banner.
	 */
	where = "receiving the initial server greeting";
	switch ((resp = smtp_chat_resp(session))->code / 100) {
	case 2:
	    break;
	case 5:
	    if (var_smtp_skip_5xx_greeting)
		STR(resp->dsn_buf)[0] = '4';
	    /* FALLTHROUGH */
	default:
	    return (smtp_site_fail(state, session->host, resp,
				   "host %s refused to talk to me: %s",
				   session->namaddr,
				   translit(resp->str, "\n", " ")));
	}

	/*
	 * If the policy table specifies a bogus TLS security level, fail
	 * now.
	 */
#ifdef USE_TLS
	if (session->tls_level == TLS_LEV_INVALID)
	    /* Warning is already logged. */
	    return (smtp_site_fail(state, DSN_BY_LOCAL_MTA,
				   SMTP_RESP_FAKE(&fake, "4.7.0"),
				   "client TLS configuration problem"));
#endif

	/*
	 * XXX Some PIX firewall versions require flush before ".<CR><LF>" so
	 * it does not span a packet boundary. This hurts performance so it
	 * is not on by default.
	 */
	if (resp->str[strspn(resp->str, "20 *\t\n")] == 0) {
	    if (smtp_pix_bug_maps != 0
		&& (pix_bug_words =
		    maps_find(smtp_pix_bug_maps,
			      state->session->addr, 0)) != 0) {
		pix_bug_source = VAR_SMTP_PIX_BUG_MAPS;
	    } else {
		pix_bug_words = var_smtp_pix_bug_words;
		pix_bug_source = VAR_SMTP_PIX_BUG_WORDS;
	    }
	    if (*pix_bug_words) {
		pix_bug_mask = name_mask_opt(pix_bug_source, pix_bug_table,
					 pix_bug_words, NAME_MASK_ANY_CASE);
		msg_info("%s: enabling PIX workarounds: %s for %s",
			 request->queue_id,
			 str_name_mask("pix workaround bitmask",
				       pix_bug_table, pix_bug_mask),
			 session->namaddrport);
		session->features |= pix_bug_mask;
	    }
	}

	/*
	 * See if we are talking to ourself. This should not be possible with
	 * the way we implement DNS lookups. However, people are known to
	 * sometimes screw up the naming service. And, mailer loops are still
	 * possible when our own mailer routing tables are mis-configured.
	 */
	words = resp->str;
	(void) mystrtok(&words, "- \t\n");
	for (n = 0; (word = mystrtok(&words, " \t\n")) != 0; n++) {
	    if (n == 0 && strcasecmp(word, var_myhostname) == 0) {
		if (state->misc_flags & SMTP_MISC_FLAG_LOOP_DETECT)
		    msg_warn("host %s greeted me with my own hostname %s",
			     session->namaddrport, var_myhostname);
	    } else if (strcasecmp(word, "ESMTP") == 0)
		session->features |= SMTP_FEATURE_ESMTP;
	}
	if ((state->misc_flags & SMTP_MISC_FLAG_USE_LMTP) == 0) {
	    if (var_smtp_always_ehlo
		&& (session->features & SMTP_FEATURE_PIX_NO_ESMTP) == 0)
		session->features |= SMTP_FEATURE_ESMTP;
	    if (var_smtp_never_ehlo
		|| (session->features & SMTP_FEATURE_PIX_NO_ESMTP) != 0)
		session->features &= ~SMTP_FEATURE_ESMTP;
	} else {
	    session->features |= SMTP_FEATURE_ESMTP;
	}
    }

    /*
     * If recursing after STARTTLS, there is no server greeting banner.
     * Always send EHLO as the next command.
     */
    else {
	session->features |= SMTP_FEATURE_ESMTP;
    }

    /*
     * Return the compliment. Fall back to SMTP if our ESMTP recognition
     * heuristic failed.
     */
    if ((state->misc_flags & SMTP_MISC_FLAG_USE_LMTP) == 0) {
	where = "performing the EHLO handshake";
	if (session->features & SMTP_FEATURE_ESMTP) {
	    smtp_chat_cmd(session, "EHLO %s", var_smtp_helo_name);
	    if ((resp = smtp_chat_resp(session))->code / 100 != 2) {
		if (resp->code == 421)
		    return (smtp_site_fail(state, session->host, resp,
					"host %s refused to talk to me: %s",
					   session->namaddr,
					   translit(resp->str, "\n", " ")));
		else
		    session->features &= ~SMTP_FEATURE_ESMTP;
	    }
	}
	if ((session->features & SMTP_FEATURE_ESMTP) == 0) {
	    where = "performing the HELO handshake";
	    smtp_chat_cmd(session, "HELO %s", var_smtp_helo_name);
	    if ((resp = smtp_chat_resp(session))->code / 100 != 2)
		return (smtp_site_fail(state, session->host, resp,
				       "host %s refused to talk to me: %s",
				       session->namaddr,
				       translit(resp->str, "\n", " ")));
	}
    } else {
	where = "performing the LHLO handshake";
	smtp_chat_cmd(session, "LHLO %s", var_smtp_helo_name);
	if ((resp = smtp_chat_resp(session))->code / 100 != 2)
	    return (smtp_site_fail(state, session->host, resp,
				   "host %s refused to talk to me: %s",
				   session->namaddr,
				   translit(resp->str, "\n", " ")));
    }

    /*
     * No early returns allowed, to ensure consistent handling of TLS and
     * SASL policies.
     */
    if (session->features & SMTP_FEATURE_ESMTP) {

	/*
	 * Determine what server EHLO keywords to ignore, typically to avoid
	 * inter-operability problems.
	 */
	if (smtp_ehlo_dis_maps == 0
	    || (ehlo_words = maps_find(smtp_ehlo_dis_maps,
				       state->session->addr, 0)) == 0)
	    ehlo_words = var_smtp_ehlo_dis_words;
	discard_mask = ehlo_mask(ehlo_words);
	if (discard_mask && !(discard_mask & EHLO_MASK_SILENT))
	    msg_info("discarding EHLO keywords: %s",
		     str_ehlo_mask(discard_mask));

	/*
	 * Pick up some useful features offered by the SMTP server. XXX Until
	 * we have a portable routine to convert from string to off_t with
	 * proper overflow detection, ignore the message size limit
	 * advertised by the SMTP server. Otherwise, we might do the wrong
	 * thing when the server advertises a really huge message size limit.
	 * 
	 * XXX Allow for "code (SP|-) ehlo-keyword (SP|=) ehlo-param...",
	 * because MicroSoft implemented AUTH based on an old draft.
	 */
	lines = resp->str;
	for (n = 0; (words = mystrtok(&lines, "\n")) != 0; /* see below */ ) {
	    if (mystrtok(&words, "- ")
		&& (word = mystrtok(&words, " \t=")) != 0) {
		if (n == 0) {
		    if (session->helo != 0)
			myfree(session->helo);

		    /*
		     * XXX: Keep the original case: we don't expect a single
		     * SMTP server to randomly change the case of its helo
		     * response. If different capitalization is detected, we
		     * should assume disjoint TLS caches.
		     */
		    session->helo = mystrdup(word);
		    if (strcasecmp(word, var_myhostname) == 0
			&& (state->misc_flags & SMTP_MISC_FLAG_LOOP_DETECT) != 0) {
			msg_warn("host %s replied to HELO/EHLO"
				 " with my own hostname %s",
				 session->namaddrport, var_myhostname);
			if (session->features & SMTP_FEATURE_BEST_MX)
			    return (smtp_site_fail(state, DSN_BY_LOCAL_MTA,
					     SMTP_RESP_FAKE(&fake, "5.4.6"),
					 "mail for %s loops back to myself",
						   request->nexthop));
			else
			    return (smtp_site_fail(state, DSN_BY_LOCAL_MTA,
					     SMTP_RESP_FAKE(&fake, "4.4.6"),
					 "mail for %s loops back to myself",
						   request->nexthop));
		    }
		} else if (strcasecmp(word, "8BITMIME") == 0) {
		    if ((discard_mask & EHLO_MASK_8BITMIME) == 0)
			session->features |= SMTP_FEATURE_8BITMIME;
		} else if (strcasecmp(word, "PIPELINING") == 0) {
		    if ((discard_mask & EHLO_MASK_PIPELINING) == 0)
			session->features |= SMTP_FEATURE_PIPELINING;
		} else if (strcasecmp(word, "XFORWARD") == 0) {
		    if ((discard_mask & EHLO_MASK_XFORWARD) == 0)
			while ((word = mystrtok(&words, " \t")) != 0)
			    session->features |=
				name_code(xforward_features,
					  NAME_CODE_FLAG_NONE, word);
		} else if (strcasecmp(word, "SIZE") == 0) {
		    if ((discard_mask & EHLO_MASK_SIZE) == 0) {
			session->features |= SMTP_FEATURE_SIZE;
			if ((word = mystrtok(&words, " \t")) != 0) {
			    if (!alldig(word))
				msg_warn("bad EHLO SIZE limit \"%s\" from %s",
					 word, session->namaddrport);
			    else
				session->size_limit = off_cvt_string(word);
			}
		    }
#ifdef USE_TLS
		} else if (strcasecmp(word, "STARTTLS") == 0) {
		    /* Ignored later if we already sent STARTTLS. */
		    if ((discard_mask & EHLO_MASK_STARTTLS) == 0)
			session->features |= SMTP_FEATURE_STARTTLS;
#endif
#ifdef USE_SASL_AUTH
		} else if (var_smtp_sasl_enable
			   && strcasecmp(word, "AUTH") == 0) {
		    if ((discard_mask & EHLO_MASK_AUTH) == 0)
			smtp_sasl_helo_auth(session, words);
#endif
		} else if (strcasecmp(word, "DSN") == 0) {
		    if ((discard_mask & EHLO_MASK_DSN) == 0)
			session->features |= SMTP_FEATURE_DSN;
		}
		n++;
	    }
	}
    }
    if (msg_verbose)
	msg_info("server features: 0x%x size %.0f",
		 session->features, (double) session->size_limit);

    /*
     * We use SMTP command pipelining if the server said it supported it.
     * Since we use blocking I/O, RFC 2197 says that we should inspect the
     * TCP window size and not send more than this amount of information.
     * Unfortunately this information is unavailable using the sockets
     * interface. However, we *can* get the TCP send buffer size on the local
     * TCP/IP stack. We should be able to fill this buffer without being
     * blocked, and then the kernel will effectively do non-blocking I/O for
     * us by automatically writing out the contents of its send buffer while
     * we are reading in the responses. In addition to TCP buffering we have
     * to be aware of application-level buffering by the vstream module,
     * which is limited to a couple kbytes.
     * 
     * XXX No need to do this before and after STARTTLS, but it's not a big deal
     * if we do.
     */
    if (session->features & SMTP_FEATURE_PIPELINING) {
	optlen = sizeof(session->sndbufsize);
	if (getsockopt(vstream_fileno(session->stream), SOL_SOCKET,
		     SO_SNDBUF, (char *) &session->sndbufsize, &optlen) < 0)
	    msg_fatal("%s: getsockopt: %m", myname);
	if (session->sndbufsize > VSTREAM_BUFSIZE)
	    session->sndbufsize = VSTREAM_BUFSIZE;
	if (session->sndbufsize == 0) {
	    session->sndbufsize = VSTREAM_BUFSIZE;
	    if (setsockopt(vstream_fileno(session->stream), SOL_SOCKET,
		      SO_SNDBUF, (char *) &session->sndbufsize, optlen) < 0)
		msg_fatal("%s: setsockopt: %m", myname);
	}
	if (msg_verbose)
	    msg_info("Using %s PIPELINING, TCP send buffer size is %d",
		     (state->misc_flags &
		      SMTP_MISC_FLAG_USE_LMTP) ? "LMTP" : "ESMTP",
		     session->sndbufsize);
    } else {
	session->sndbufsize = 0;
    }

#ifdef USE_TLS

    /*
     * Skip this part if we already sent STARTTLS.
     */
    if ((state->misc_flags & SMTP_MISC_FLAG_IN_STARTTLS) == 0) {

	/*
	 * Optionally log unused STARTTLS opportunities.
	 */
	if ((session->features & SMTP_FEATURE_STARTTLS) &&
	    var_smtp_tls_note_starttls_offer &&
	    session->tls_level <= TLS_LEV_NONE)
	    msg_info("Host offered STARTTLS: [%s]", session->host);

	/*
	 * Decide whether or not to send STARTTLS.
	 */
	if ((session->features & SMTP_FEATURE_STARTTLS) != 0
	    && smtp_tls_ctx != 0 && session->tls_level >= TLS_LEV_MAY) {

	    /*
	     * Prepare for disaster.
	     */
	    smtp_timeout_setup(state->session->stream, var_smtp_starttls_tmout);
	    if ((except = vstream_setjmp(state->session->stream)) != 0)
		return (smtp_stream_except(state, except,
					"receiving the STARTTLS response"));

	    /*
	     * Send STARTTLS. Recurse when the server accepts STARTTLS, after
	     * resetting the SASL and EHLO features lists.
	     * 
	     * Reset the SASL mechanism list to avoid spurious warnings.
	     * 
	     * Use the smtp_sasl_tls_security_options feature to allow SASL
	     * mechanisms that may not be allowed with plain-text
	     * connections.
	     */
	    smtp_chat_cmd(session, "STARTTLS");
	    if ((resp = smtp_chat_resp(session))->code / 100 == 2) {
#ifdef USE_SASL_AUTH
		if (session->features & SMTP_FEATURE_AUTH)
		    smtp_sasl_cleanup(session);
#endif
		session->features = saved_features;
		/* XXX Mix-up of per-session and per-request flags. */
		state->misc_flags |= SMTP_MISC_FLAG_IN_STARTTLS;
		tls_helo_status = smtp_start_tls(state);
		state->misc_flags &= ~SMTP_MISC_FLAG_IN_STARTTLS;
		return (tls_helo_status);
	    }

	    /*
	     * Give up if we must use TLS but the server rejects STARTTLS
	     * although support for it was announced in the EHLO response.
	     */
	    session->features &= ~SMTP_FEATURE_STARTTLS;
	    if (session->tls_level >= TLS_LEV_ENCRYPT)
		return (smtp_site_fail(state, session->host, resp,
		    "TLS is required, but host %s refused to start TLS: %s",
				       session->namaddr,
				       translit(resp->str, "\n", " ")));
	    /* Else try to continue in plain-text mode. */
	}

	/*
	 * Give up if we must use TLS but can't for various reasons.
	 * 
	 * 200412 Be sure to provide the default clause at the bottom of this
	 * block. When TLS is required we must never, ever, end up in
	 * plain-text mode.
	 */
	if (session->tls_level >= TLS_LEV_ENCRYPT) {
	    if (!(session->features & SMTP_FEATURE_STARTTLS)) {
		return (smtp_site_fail(state, DSN_BY_LOCAL_MTA,
				       SMTP_RESP_FAKE(&fake, "4.7.4"),
			  "TLS is required, but was not offered by host %s",
				       session->namaddr));
	    } else if (smtp_tls_ctx == 0) {
		return (smtp_site_fail(state, DSN_BY_LOCAL_MTA,
				       SMTP_RESP_FAKE(&fake, "4.7.5"),
		     "TLS is required, but our TLS engine is unavailable"));
	    } else {
		msg_warn("%s: TLS is required but unavailable, don't know why",
			 myname);
		return (smtp_site_fail(state, DSN_BY_LOCAL_MTA,
				       SMTP_RESP_FAKE(&fake, "4.7.0"),
				       "TLS is required, but unavailable"));
	    }
	}
    }
#endif
#ifdef USE_SASL_AUTH
    if (var_smtp_sasl_enable && (session->features & SMTP_FEATURE_AUTH))
	return (smtp_sasl_helo_login(state));
#endif

    return (0);
}

#ifdef USE_TLS

/* smtp_start_tls - turn on TLS and recurse into the HELO dialog */

static int smtp_start_tls(SMTP_STATE *state)
{
    SMTP_SESSION *session = state->session;
    TLS_CLIENT_START_PROPS tls_props;
    VSTRING *serverid;
    SMTP_RESP fake;

    /*
     * Turn off SMTP connection caching. When the TLS handshake succeeds, we
     * can't reuse the SMTP connection. Reason: we can't turn off TLS in one
     * process, save the connection to the cache which is shared with all
     * SMTP clients, migrate the connection to another SMTP client, and
     * resume TLS there. When the TLS handshake fails, we can't reuse the
     * SMTP connection either, because the conversation is in an unknown
     * state.
     */
    DONT_CACHE_THIS_SESSION;

    /*
     * As of Postfix 2.5, tls_client_start() tries hard to always complete
     * the TLS handshake. It records the verification and match status in the
     * resulting TLScontext. It is now up to the application to abort the TLS
     * connection if it chooses.
     * 
     * XXX When tls_client_start() fails then we don't know what state the SMTP
     * connection is in, so we give up on this connection even if we are not
     * required to use TLS.
     * 
     * The following assumes sites that use TLS in a perverse configuration:
     * multiple hosts per hostname, or even multiple hosts per IP address.
     * All this without a shared TLS session cache, and they still want to
     * use TLS session caching???
     * 
     * The TLS session cache records the trust chain verification status of
     * cached sessions. Different transports may have different CAfile or
     * CApath settings, perhaps to allow authenticated connections to sites
     * with private CA certs without trusting said private certs for other
     * sites. So we cannot assume that a trust chain valid for one transport
     * is valid for another. Therefore the client session id must include
     * either the transport name or the values of CAfile and CApath. We use
     * the transport name.
     * 
     * XXX: We store only one session per lookup key. Ideally the the key maps
     * 1-to-1 to a server TLS session cache. We use the IP address, port and
     * ehlo response name to build a lookup key that works for split caches
     * (that announce distinct names) behind a load balancer.
     * 
     * XXX: The TLS library may salt the serverid with further details of the
     * protocol and cipher requirements.
     * 
     * Large parameter lists are error-prone, so we emulate a language feature
     * that C does not have natively: named parameter lists.
     */
    serverid = vstring_alloc(10);
    vstring_sprintf(serverid, "%s:%s:%u:%s", state->service, session->addr,
		  ntohs(session->port), session->helo ? session->helo : "");
    session->tls_context =
	TLS_CLIENT_START(&tls_props,
			 ctx = smtp_tls_ctx,
			 stream = session->stream,
			 log_level = var_smtp_tls_loglevel,
			 timeout = var_smtp_starttls_tmout,
			 tls_level = session->tls_level,
			 nexthop = session->tls_nexthop,
			 host = session->host,
			 namaddr = session->namaddrport,
			 serverid = vstring_str(serverid),
			 protocols = session->tls_protocols,
			 cipher_grade = session->tls_grade,
			 cipher_exclusions
			 = vstring_str(session->tls_exclusions),
			 matchargv = session->tls_matchargv,
			 fpt_dgst = var_smtp_tls_fpt_dgst);
    vstring_free(serverid);

    if (session->tls_context == 0) {

	/*
	 * We must avoid further I/O, the peer is in an undefined state.
	 */
	(void) vstream_fpurge(session->stream, VSTREAM_PURGE_BOTH);
	DONT_USE_DEAD_SESSION;

	/*
	 * If TLS is optional, try delivery to the same server over a
	 * plaintext connection. Otherwise we would defer mail forever with
	 * destinations that have no alternate MX host.
	 * 
	 * Don't fall back to plaintext if we were willing to use SASL-over-TLS
	 * authentication. If the server doesn't announce SASL support over
	 * plaintext connections, then we don't want delivery to fail with
	 * "relay access denied".
	 */
	if (session->tls_level == TLS_LEV_MAY
#ifdef USE_SASL_AUTH
	    && !(var_smtp_sasl_enable
		 && *var_smtp_sasl_passwd
		 && smtp_sasl_passwd_lookup(session))
#endif
	    )
	    RETRY_AS_PLAINTEXT;
	return (smtp_site_fail(state, DSN_BY_LOCAL_MTA,
			       SMTP_RESP_FAKE(&fake, "4.7.5"),
			       "Cannot start TLS: handshake failure"));
    }

    /*
     * If we are verifying the server certificate and are not happy with the
     * result, abort the delivery here. We have a usable TLS session with the
     * server, so no need to disable I/O, ... we can even be polite and send
     * "QUIT".
     * 
     * See src/tls/tls_level.c. Levels above encrypt require matching. Levels >=
     * verify require CA trust.
     */
    if (session->tls_level >= TLS_LEV_VERIFY)
	if (!TLS_CERT_IS_TRUSTED(session->tls_context))
	    return (smtp_site_fail(state, DSN_BY_LOCAL_MTA,
				   SMTP_RESP_FAKE(&fake, "4.7.5"),
				   "Server certificate not trusted"));
    if (session->tls_level > TLS_LEV_ENCRYPT)
	if (!TLS_CERT_IS_MATCHED(session->tls_context))
	    return (smtp_site_fail(state, DSN_BY_LOCAL_MTA,
				   SMTP_RESP_FAKE(&fake, "4.7.5"),
				   "Server certificate not verified"));

    /*
     * At this point we have to re-negotiate the "EHLO" to reget the
     * feature-list.
     */
    return (smtp_helo(state));
}

#endif

/* smtp_hbc_logger - logging call-back for header/body checks */

static void smtp_hbc_logger(void *context, const char *action,
			            const char *where, const char *content,
			            const char *text)
{
    const SMTP_STATE *state = (SMTP_STATE *) context;

    if (*text) {
	msg_info("%s: %s: %s %.60s: %s",
		 state->request->queue_id, action, where, content, text);
    } else {
	msg_info("%s: %s: %s %.60s",
		 state->request->queue_id, action, where, content);
    }
}

/* smtp_text_out - output one header/body record */

static void smtp_text_out(void *context, int rec_type,
			          const char *text, ssize_t len,
			          off_t unused_offset)
{
    SMTP_STATE *state = (SMTP_STATE *) context;
    SMTP_SESSION *session = state->session;
    ssize_t data_left;
    const char *data_start;

    /*
     * Deal with an impedance mismatch between Postfix queue files (record
     * length <= $message_line_length_limit) and SMTP (DATA record length <=
     * $smtp_line_length_limit). The code below does a little too much work
     * when the SMTP line length limit is disabled, but it avoids code
     * duplication, and thus, it avoids testing and maintenance problems.
     */
    data_left = len;
    data_start = text;
    do {
	if (state->space_left == var_smtp_line_limit
	    && data_left > 0 && *data_start == '.')
	    smtp_fputc('.', session->stream);
	if (var_smtp_line_limit > 0 && data_left >= state->space_left) {
	    smtp_fputs(data_start, state->space_left, session->stream);
	    data_start += state->space_left;
	    data_left -= state->space_left;
	    state->space_left = var_smtp_line_limit;
	    if (data_left > 0 || rec_type == REC_TYPE_CONT) {
		smtp_fputc(' ', session->stream);
		state->space_left -= 1;
	    }
	} else {
	    if (rec_type == REC_TYPE_CONT) {
		smtp_fwrite(data_start, data_left, session->stream);
		state->space_left -= data_left;
	    } else {
		smtp_fputs(data_start, data_left, session->stream);
		state->space_left = var_smtp_line_limit;
	    }
	    break;
	}
    } while (data_left > 0);
}

/* smtp_format_out - output one header/body record */

static void PRINTFLIKE(3, 4) smtp_format_out(void *, int, const char *,...);

static void smtp_format_out(void *context, int rec_type, const char *fmt,...)
{
    static VSTRING *vp;
    va_list ap;

    if (vp == 0)
	vp = vstring_alloc(100);
    va_start(ap, fmt);
    vstring_vsprintf(vp, fmt, ap);
    va_end(ap);
    smtp_text_out(context, rec_type, vstring_str(vp), VSTRING_LEN(vp), 0);
}

/* smtp_header_out - output one message header */

static void smtp_header_out(void *context, int unused_header_class,
			            const HEADER_OPTS *unused_info,
			            VSTRING *buf, off_t offset)
{
    char   *start = vstring_str(buf);
    char   *line;
    char   *next_line;

    /*
     * This code destroys the header. We could try to avoid clobbering it,
     * but we're not going to use the data any further.
     */
    for (line = start; line; line = next_line) {
	next_line = split_at(line, '\n');
	smtp_text_out(context, REC_TYPE_NORM, line, next_line ?
		      next_line - line - 1 : strlen(line), offset);
    }
}

/* smtp_header_rewrite - rewrite message header before output */

static void smtp_header_rewrite(void *context, int header_class,
				        const HEADER_OPTS *header_info,
				        VSTRING *buf, off_t offset)
{
    SMTP_STATE *state = (SMTP_STATE *) context;
    int     did_rewrite = 0;
    char   *line;
    char   *start;
    char   *next_line;
    char   *end_line;
    char   *result;

    /*
     * Apply optional header filtering.
     */
    if (smtp_header_checks) {
	result = hbc_header_checks(context, smtp_header_checks, header_class,
				   header_info, buf, offset);
	if (result == 0)
	    return;
	if (result != STR(buf)) {
	    vstring_strcpy(buf, result);
	    myfree(result);
	}
    }

    /*
     * Rewrite primary header addresses that match the smtp_generic_maps. The
     * cleanup server already enforces that all headers have proper lengths
     * and that all addresses are in proper form, so we don't have to repeat
     * that.
     */
    if (smtp_generic_maps && header_info && header_class == MIME_HDR_PRIMARY
	&& (header_info->flags & (HDR_OPT_SENDER | HDR_OPT_RECIP)) != 0) {
	TOK822 *tree;
	TOK822 **addr_list;
	TOK822 **tpp;

	tree = tok822_parse(vstring_str(buf)
			    + strlen(header_info->name) + 1);
	addr_list = tok822_grep(tree, TOK822_ADDR);
	for (tpp = addr_list; *tpp; tpp++)
	    did_rewrite |= smtp_map11_tree(tpp[0], smtp_generic_maps,
				     smtp_ext_prop_mask & EXT_PROP_GENERIC);
	if (did_rewrite) {
	    vstring_truncate(buf, strlen(header_info->name));
	    vstring_strcat(buf, ": ");
	    tok822_externalize(buf, tree, TOK822_STR_HEAD);
	}
	myfree((char *) addr_list);
	tok822_free_tree(tree);
    }

    /*
     * Pass through unmodified headers without reconstruction.
     */
    if (did_rewrite == 0) {
	smtp_header_out(context, header_class, header_info, buf, offset);
	return;
    }

    /*
     * A rewritten address list contains one address per line. The code below
     * replaces newlines by spaces, to fit as many addresses on a line as
     * possible (without rearranging the order of addresses). Prepending
     * white space to the beginning of lines is delegated to the output
     * routine.
     * 
     * Code derived from cleanup_fold_header().
     */
    for (line = start = vstring_str(buf); line != 0; line = next_line) {
	end_line = line + strcspn(line, "\n");
	if (line > start) {
	    if (end_line - start < 70) {	/* TAB counts as one */
		line[-1] = ' ';
	    } else {
		start = line;
	    }
	}
	next_line = *end_line ? end_line + 1 : 0;
    }

    /*
     * Prepend a tab to continued header lines that went through the address
     * rewriting machinery. Just like smtp_header_out(), this code destroys
     * the header. We could try to avoid clobbering it, but we're not going
     * to use the data any further.
     * 
     * Code derived from cleanup_out_header().
     */
    for (line = start = vstring_str(buf); line != 0; line = next_line) {
	next_line = split_at(line, '\n');
	if (line == start || IS_SPACE_TAB(*line)) {
	    smtp_text_out(state, REC_TYPE_NORM, line, next_line ?
			  next_line - line - 1 : strlen(line), offset);
	} else {
	    smtp_format_out(state, REC_TYPE_NORM, "\t%s", line);
	}
    }
}

/* smtp_body_rewrite - rewrite message body before output */

static void smtp_body_rewrite(void *context, int type,
			              const char *buf, ssize_t len,
			              off_t offset)
{
    SMTP_STATE *state = (SMTP_STATE *) context;
    char   *result;

    /*
     * Apply optional body filtering.
     */
    if (smtp_body_checks) {
	result = hbc_body_checks(context, smtp_body_checks, buf, len, offset);
	if (result == buf) {
	    smtp_text_out(state, type, buf, len, offset);
	} else if (result != 0) {
	    smtp_text_out(state, type, result, strlen(result), offset);
	    myfree(result);
	}
    }
}

/* smtp_mime_fail - MIME problem */

static void smtp_mime_fail(SMTP_STATE *state, int mime_errs)
{
    const MIME_STATE_DETAIL *detail;
    SMTP_RESP fake;

    detail = mime_state_detail(mime_errs);
    smtp_mesg_fail(state, DSN_BY_LOCAL_MTA,
		   SMTP_RESP_FAKE(&fake, detail->dsn),
		   "%s", detail->text);
}

/* smtp_loop - exercise the SMTP protocol engine */

static int smtp_loop(SMTP_STATE *state, NOCLOBBER int send_state,
		             NOCLOBBER int recv_state)
{
    const char *myname = "smtp_loop";
    DELIVER_REQUEST *request = state->request;
    SMTP_SESSION *session = state->session;
    SMTP_RESP *resp;
    RECIPIENT *rcpt;
    VSTRING *next_command = vstring_alloc(100);
    int    *NOCLOBBER survivors = 0;
    NOCLOBBER int next_state;
    NOCLOBBER int next_rcpt;
    NOCLOBBER int send_rcpt;
    NOCLOBBER int recv_rcpt;
    NOCLOBBER int nrcpt;
    NOCLOBBER int recv_done;
    int     except;
    int     rec_type;
    NOCLOBBER int prev_type = 0;
    NOCLOBBER int sndbuffree;
    NOCLOBBER int mail_from_rejected;
    NOCLOBBER int downgrading;
    int     mime_errs;
    SMTP_RESP fake;
    int     fail_status;

    /*
     * Macros for readability.
     */
#define REWRITE_ADDRESS(dst, src) do { \
	vstring_strcpy(dst, src); \
	if (*(src) && smtp_generic_maps) \
	    smtp_map11_internal(dst, smtp_generic_maps, \
		smtp_ext_prop_mask & EXT_PROP_GENERIC); \
    } while (0)

#define QUOTE_ADDRESS(dst, src) do { \
	if (*(src) && var_smtp_quote_821_env) { \
	    quote_821_local(dst, src); \
	} else { \
	    vstring_strcpy(dst, src); \
	} \
    } while (0)

    /* Caution: changes to RETURN() also affect code outside the main loop. */

#define RETURN(x) do { \
	if (recv_state != SMTP_STATE_LAST) \
	    DONT_CACHE_THIS_SESSION; \
	vstring_free(next_command); \
	if (survivors) \
	    myfree((char *) survivors); \
	if (session->mime_state) \
	    session->mime_state = mime_state_free(session->mime_state); \
	return (x); \
    } while (0)

#define SENDER_IS_AHEAD \
	(recv_state < send_state || recv_rcpt != send_rcpt)

#define SENDER_IN_WAIT_STATE \
	(send_state == SMTP_STATE_DOT || send_state == SMTP_STATE_LAST)

#define SENDING_MAIL \
	(recv_state <= SMTP_STATE_DOT)

#define CANT_RSET_THIS_SESSION \
	(session->features |= SMTP_FEATURE_RSET_REJECTED)

    /*
     * Sanity check. We don't want smtp_chat() to inadvertently flush the
     * output buffer. That means someone broke pipelining support.
     */
    if (session->sndbufsize > VSTREAM_BUFSIZE)
	msg_panic("bad sndbufsize %d > VSTREAM_BUFSIZE %d",
		  session->sndbufsize, VSTREAM_BUFSIZE);

    /*
     * Miscellaneous initialization. Some of this might be done in
     * smtp_xfer() but that just complicates interfaces and data structures.
     */
    sndbuffree = session->sndbufsize;

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
    next_rcpt = send_rcpt = recv_rcpt = recv_done = 0;
    mail_from_rejected = 0;

    /*
     * Prepare for disaster. This should not be needed because the design
     * guarantees that no output is flushed before smtp_chat_resp() is
     * called.
     * 
     * 1) Every SMTP command fits entirely in a VSTREAM output buffer.
     * 
     * 2) smtp_loop() never invokes smtp_chat_cmd() without making sure that
     * there is sufficient space for the command in the output buffer.
     * 
     * 3) smtp_loop() flushes the output buffer to avoid server timeouts.
     * 
     * Changing any of these would violate the design, and would likely break
     * SMTP pipelining.
     * 
     * We set up the error handler anyway (only upon entry to avoid wasting
     * resources) because 1) there is code below that expects that VSTREAM
     * timeouts are enabled, and 2) this allows us to detect if someone broke
     * Postfix by introducing spurious flush before read operations.
     */
    if (send_state < SMTP_STATE_XFORWARD_NAME_ADDR
	|| send_state > SMTP_STATE_QUIT)
	msg_panic("%s: bad sender state %d (receiver state %d)",
		  myname, send_state, recv_state);
    smtp_timeout_setup(session->stream,
		       *xfer_timeouts[send_state]);
    if ((except = vstream_setjmp(session->stream)) != 0) {
	msg_warn("smtp_proto: spurious flush before read in send state %d",
		 send_state);
	RETURN(SENDING_MAIL ? smtp_stream_except(state, except,
					     xfer_states[send_state]) : -1);
    }

    /*
     * The main protocol loop.
     */
    do {

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
	     * Build the XFORWARD command. With properly sanitized
	     * information, the command length stays within the 512 byte
	     * command line length limit.
	     */
	case SMTP_STATE_XFORWARD_NAME_ADDR:
	    vstring_strcpy(next_command, XFORWARD_CMD);
	    if (session->features & SMTP_FEATURE_XFORWARD_NAME) {
		vstring_strcat(next_command, " " XFORWARD_NAME "=");
		xtext_quote_append(next_command,
				   DEL_REQ_ATTR_AVAIL(request->client_name) ?
			   request->client_name : XFORWARD_UNAVAILABLE, "");
	    }
	    if (session->features & SMTP_FEATURE_XFORWARD_ADDR) {
		vstring_strcat(next_command, " " XFORWARD_ADDR "=");
		xtext_quote_append(next_command,
				   DEL_REQ_ATTR_AVAIL(request->client_addr) ?
			   request->client_addr : XFORWARD_UNAVAILABLE, "");
	    }
	    if (session->features & SMTP_FEATURE_XFORWARD_PORT) {
		vstring_strcat(next_command, " " XFORWARD_PORT "=");
		xtext_quote_append(next_command,
				   DEL_REQ_ATTR_AVAIL(request->client_port) ?
			   request->client_port : XFORWARD_UNAVAILABLE, "");
	    }
	    if (session->send_proto_helo)
		next_state = SMTP_STATE_XFORWARD_PROTO_HELO;
	    else
		next_state = SMTP_STATE_MAIL;
	    break;

	case SMTP_STATE_XFORWARD_PROTO_HELO:
	    vstring_strcpy(next_command, XFORWARD_CMD);
	    if (session->features & SMTP_FEATURE_XFORWARD_PROTO) {
		vstring_strcat(next_command, " " XFORWARD_PROTO "=");
		xtext_quote_append(next_command,
				 DEL_REQ_ATTR_AVAIL(request->client_proto) ?
			  request->client_proto : XFORWARD_UNAVAILABLE, "");
	    }
	    if (session->features & SMTP_FEATURE_XFORWARD_HELO) {
		vstring_strcat(next_command, " " XFORWARD_HELO "=");
		xtext_quote_append(next_command,
				   DEL_REQ_ATTR_AVAIL(request->client_helo) ?
			   request->client_helo : XFORWARD_UNAVAILABLE, "");
	    }
	    if (session->features & SMTP_FEATURE_XFORWARD_DOMAIN) {
		vstring_strcat(next_command, " " XFORWARD_DOMAIN "=");
		xtext_quote_append(next_command,
			 DEL_REQ_ATTR_AVAIL(request->rewrite_context) == 0 ?
				   XFORWARD_UNAVAILABLE :
		     strcmp(request->rewrite_context, MAIL_ATTR_RWR_LOCAL) ?
			      XFORWARD_DOM_REMOTE : XFORWARD_DOM_LOCAL, "");
	    }
	    next_state = SMTP_STATE_MAIL;
	    break;

	    /*
	     * Build the MAIL FROM command.
	     */
	case SMTP_STATE_MAIL:
	    request->msg_stats.reuse_count = session->reuse_count;
	    GETTIMEOFDAY(&request->msg_stats.conn_setup_done);
	    REWRITE_ADDRESS(session->scratch2, request->sender);
	    QUOTE_ADDRESS(session->scratch, vstring_str(session->scratch2));
	    vstring_sprintf(next_command, "MAIL FROM:<%s>",
			    vstring_str(session->scratch));
	    /* XXX Don't announce SIZE if we're going to MIME downgrade. */
	    if (session->features & SMTP_FEATURE_SIZE	/* RFC 1870 */
		&& !SMTP_MIME_DOWNGRADE(session, request))
		vstring_sprintf_append(next_command, " SIZE=%lu",
				       request->data_size);
	    if (session->features & SMTP_FEATURE_8BITMIME) {	/* RFC 1652 */
		if (strcmp(request->encoding, MAIL_ATTR_ENC_8BIT) == 0)
		    vstring_strcat(next_command, " BODY=8BITMIME");
		else if (strcmp(request->encoding, MAIL_ATTR_ENC_7BIT) == 0)
		    vstring_strcat(next_command, " BODY=7BIT");
		else if (strcmp(request->encoding, MAIL_ATTR_ENC_NONE) != 0)
		    msg_warn("%s: unknown content encoding: %s",
			     request->queue_id, request->encoding);
	    }
	    if (session->features & SMTP_FEATURE_DSN) {
		if (request->dsn_envid[0]) {
		    vstring_sprintf_append(next_command, " ENVID=");
		    xtext_quote_append(next_command, request->dsn_envid, "+=");
		}
		if (request->dsn_ret)
		    vstring_sprintf_append(next_command, " RET=%s",
					   dsn_ret_str(request->dsn_ret));
	    }

	    /*
	     * We authenticate the local MTA only, but not the sender.
	     */
#ifdef USE_SASL_AUTH
	    if (var_smtp_sasl_enable
		&& (session->features & SMTP_FEATURE_AUTH))
		vstring_strcat(next_command, " AUTH=<>");
#endif
	    next_state = SMTP_STATE_RCPT;
	    break;

	    /*
	     * Build one RCPT TO command before we have seen the MAIL FROM
	     * response.
	     */
	case SMTP_STATE_RCPT:
	    rcpt = request->rcpt_list.info + send_rcpt;
	    REWRITE_ADDRESS(session->scratch2, rcpt->address);
	    QUOTE_ADDRESS(session->scratch, vstring_str(session->scratch2));
	    vstring_sprintf(next_command, "RCPT TO:<%s>",
			    vstring_str(session->scratch));
	    if (session->features & SMTP_FEATURE_DSN) {
		/* XXX DSN xtext encode address value not type. */
		if (rcpt->dsn_orcpt[0]) {
		    xtext_quote(session->scratch, rcpt->dsn_orcpt, "+=");
		    vstring_sprintf_append(next_command, " ORCPT=%s",
					   vstring_str(session->scratch));
		} else if (rcpt->orig_addr[0]) {
		    quote_822_local(session->scratch, rcpt->orig_addr);
		    vstring_sprintf(session->scratch2, "rfc822;%s",
				    vstring_str(session->scratch));
		    xtext_quote(session->scratch, vstring_str(session->scratch2), "+=");
		    vstring_sprintf_append(next_command, " ORCPT=%s",
					   vstring_str(session->scratch));
		}
		if (rcpt->dsn_notify)
		    vstring_sprintf_append(next_command, " NOTIFY=%s",
					   dsn_notify_str(rcpt->dsn_notify));
	    }
	    if ((next_rcpt = send_rcpt + 1) == SMTP_RCPT_LEFT(state))
		next_state = DEL_REQ_TRACE_ONLY(request->flags) ?
		    SMTP_STATE_ABORT : SMTP_STATE_DATA;
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
	     * Build the "." command after we have seen the DATA response
	     * (DATA is a protocol synchronization point).
	     * 
	     * Changing the connection caching state here is safe because it
	     * affects none of the not-yet processed replies to
	     * already-generated commands.
	     */
	case SMTP_STATE_DOT:
	    vstring_strcpy(next_command, ".");
	    if (THIS_SESSION_IS_EXPIRED)
		DONT_CACHE_THIS_SESSION;
	    next_state = THIS_SESSION_IS_CACHED ?
		SMTP_STATE_LAST : SMTP_STATE_QUIT;
	    break;

	    /*
	     * The SMTP_STATE_ABORT sender state is entered by the sender
	     * when it has verified all recipients; or it is entered by the
	     * receiver when all recipients are verified or rejected, and is
	     * then left before the bottom of the main loop.
	     * 
	     * Changing the connection caching state here is safe because there
	     * are no not-yet processed replies to already-generated
	     * commands.
	     */
	case SMTP_STATE_ABORT:
	    vstring_strcpy(next_command, "RSET");
	    if (THIS_SESSION_IS_EXPIRED)
		DONT_CACHE_THIS_SESSION;
	    next_state = THIS_SESSION_IS_CACHED ?
		SMTP_STATE_LAST : SMTP_STATE_QUIT;
	    break;

	    /*
	     * Build the RSET command. This is entered as initial state from
	     * smtp_rset() and has its own dedicated state transitions. It is
	     * used to find out the status of a cached session before
	     * attempting mail delivery.
	     */
	case SMTP_STATE_RSET:
	    vstring_strcpy(next_command, "RSET");
	    next_state = SMTP_STATE_LAST;
	    break;

	    /*
	     * Build the QUIT command before we have seen the "." or RSET
	     * response. This is entered as initial state from smtp_quit(),
	     * or is reached near the end of any non-cached session.
	     * 
	     * Changing the connection caching state here is safe. If this
	     * command is pipelined together with a preceding command, then
	     * connection caching was already turned off. Do not clobber the
	     * "bad connection" flag.
	     */
	case SMTP_STATE_QUIT:
	    vstring_strcpy(next_command, "QUIT");
	    next_state = SMTP_STATE_LAST;
	    if (THIS_SESSION_IS_CACHED)
		DONT_CACHE_THIS_SESSION;
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
		if (recv_state < SMTP_STATE_XFORWARD_NAME_ADDR
		    || recv_state > SMTP_STATE_QUIT)
		    msg_panic("%s: bad receiver state %d (sender state %d)",
			      myname, recv_state, send_state);

		/*
		 * Receive the next server response. Use the proper timeout,
		 * and log the proper client state in case of trouble.
		 * 
		 * XXX If we lose the connection before sending end-of-data,
		 * find out if the server sent a premature end-of-data reply.
		 * If this read attempt fails, report "lost connection while
		 * sending message body", not "lost connection while sending
		 * end-of-data".
		 * 
		 * "except" becomes zero just above the protocol loop, and stays
		 * zero or triggers an early return from the loop. In just
		 * one case: loss of the connection when sending the message
		 * body, we record the exception, and keep processing in the
		 * hope of detecting a premature 5XX. We must be careful to
		 * not clobber this non-zero value once it is set. The
		 * variable need not survive longjmp() calls, since the only
		 * setjmp() which does not return early is the one sets this
		 * condition, subquent failures always return early.
		 */
#define LOST_CONNECTION_INSIDE_DATA (except == SMTP_ERR_EOF)

		smtp_timeout_setup(session->stream,
				   *xfer_timeouts[recv_state]);
		if (LOST_CONNECTION_INSIDE_DATA) {
		    if (vstream_setjmp(session->stream) != 0)
			RETURN(smtp_stream_except(state, SMTP_ERR_EOF,
						  "sending message body"));
		} else {
		    if ((except = vstream_setjmp(session->stream)) != 0)
			RETURN(SENDING_MAIL ? smtp_stream_except(state, except,
					     xfer_states[recv_state]) : -1);
		}
		resp = smtp_chat_resp(session);

		/*
		 * Process the response.
		 */
		switch (recv_state) {

		    /*
		     * Process the XFORWARD response.
		     */
		case SMTP_STATE_XFORWARD_NAME_ADDR:
		    if (resp->code / 100 != 2)
			msg_warn("host %s said: %s (in reply to %s)",
				 session->namaddrport,
				 translit(resp->str, "\n", " "),
			       xfer_request[SMTP_STATE_XFORWARD_NAME_ADDR]);
		    if (session->send_proto_helo)
			recv_state = SMTP_STATE_XFORWARD_PROTO_HELO;
		    else
			recv_state = SMTP_STATE_MAIL;
		    break;

		case SMTP_STATE_XFORWARD_PROTO_HELO:
		    if (resp->code / 100 != 2)
			msg_warn("host %s said: %s (in reply to %s)",
				 session->namaddrport,
				 translit(resp->str, "\n", " "),
			      xfer_request[SMTP_STATE_XFORWARD_PROTO_HELO]);
		    recv_state = SMTP_STATE_MAIL;
		    break;

		    /*
		     * Process the MAIL FROM response. When the server
		     * rejects the sender, set the mail_from_rejected flag so
		     * that the receiver may apply a course correction.
		     */
		case SMTP_STATE_MAIL:
		    if (resp->code / 100 != 2) {
			smtp_mesg_fail(state, session->host, resp,
				       "host %s said: %s (in reply to %s)",
				       session->namaddr,
				       translit(resp->str, "\n", " "),
				       xfer_request[SMTP_STATE_MAIL]);
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
			if (resp->code == 552) {
			    resp->code = 452;
			    resp->dsn[0] = '4';
			}
#endif
			rcpt = request->rcpt_list.info + recv_rcpt;
			if (resp->code / 100 == 2) {
			    if (state->misc_flags & SMTP_MISC_FLAG_USE_LMTP) {
				if (survivors == 0)
				    survivors = (int *)
					mymalloc(request->rcpt_list.len
						 * sizeof(int));
				survivors[nrcpt] = recv_rcpt;
			    }
			    ++nrcpt;
			    /* If trace-only, mark the recipient done. */
			    if (DEL_REQ_TRACE_ONLY(request->flags)) {
				translit(resp->str, "\n", " ");
				smtp_rcpt_done(state, resp, rcpt);
			    }
			} else {
			    smtp_rcpt_fail(state, rcpt, session->host, resp,
					"host %s said: %s (in reply to %s)",
					   session->namaddr,
					   translit(resp->str, "\n", " "),
					   xfer_request[SMTP_STATE_RCPT]);
			}
		    }
		    /* If trace-only, send RSET instead of DATA. */
		    if (++recv_rcpt == SMTP_RCPT_LEFT(state))
			recv_state = DEL_REQ_TRACE_ONLY(request->flags) ?
			    SMTP_STATE_ABORT : SMTP_STATE_DATA;
		    /* XXX Also: record if non-delivering session. */
		    break;

		    /*
		     * Process the DATA response. When the server rejects
		     * DATA, set nrcpt to a negative value so that the
		     * receiver can apply a course correction.
		     */
		case SMTP_STATE_DATA:
		    if (resp->code / 100 != 3) {
			if (nrcpt > 0)
			    smtp_mesg_fail(state, session->host, resp,
					"host %s said: %s (in reply to %s)",
					   session->namaddr,
					   translit(resp->str, "\n", " "),
					   xfer_request[SMTP_STATE_DATA]);
			nrcpt = -1;
		    }
		    recv_state = SMTP_STATE_DOT;
		    break;

		    /*
		     * Process the end of message response. Ignore the
		     * response when no recipient was accepted: all
		     * recipients are dead already, and the next receiver
		     * state is SMTP_STATE_LAST/QUIT regardless. Otherwise,
		     * if the message transfer fails, bounce all remaining
		     * recipients, else cross off the recipients that were
		     * delivered.
		     */
		case SMTP_STATE_DOT:
		    GETTIMEOFDAY(&request->msg_stats.deliver_done);
		    if ((state->misc_flags & SMTP_MISC_FLAG_USE_LMTP) == 0) {
			if (nrcpt > 0) {
			    if (resp->code / 100 != 2) {
				smtp_mesg_fail(state, session->host, resp,
					"host %s said: %s (in reply to %s)",
					       session->namaddr,
					     translit(resp->str, "\n", " "),
					       xfer_request[SMTP_STATE_DOT]);
			    } else {
				for (nrcpt = 0; nrcpt < recv_rcpt; nrcpt++) {
				    rcpt = request->rcpt_list.info + nrcpt;
				    if (!SMTP_RCPT_ISMARKED(rcpt)) {
					translit(resp->str, "\n", " ");
					smtp_rcpt_done(state, resp, rcpt);
				    }
				}
			    }
			}
		    }

		    /*
		     * With LMTP we have one response per accepted RCPT TO
		     * command. Stay in the SMTP_STATE_DOT state until we
		     * have collected all responses.
		     */
		    else {
			if (nrcpt > 0) {
			    rcpt = request->rcpt_list.info
				+ survivors[recv_done++];
			    if (resp->code / 100 != 2) {
				smtp_rcpt_fail(state, rcpt, session->host, resp,
					"host %s said: %s (in reply to %s)",
					       session->namaddr,
					     translit(resp->str, "\n", " "),
					       xfer_request[SMTP_STATE_DOT]);
			    } else {
				translit(resp->str, "\n", " ");
				smtp_rcpt_done(state, resp, rcpt);
			    }
			}
			if (msg_verbose)
			    msg_info("%s: got %d of %d end-of-data replies",
				     myname, recv_done, nrcpt);
			if (recv_done < nrcpt)
			    break;
		    }

		    /*
		     * XXX Do not change the connection caching state here,
		     * even if the connection caching timer expired between
		     * generating the command and processing the reply,
		     * otherwise the sender and receiver loops get out of
		     * sync. The caller will call smtp_quit() if appropriate.
		     */
		    if (var_skip_quit_resp || THIS_SESSION_IS_CACHED
			|| LOST_CONNECTION_INSIDE_DATA)
			recv_state = SMTP_STATE_LAST;
		    else
			recv_state = SMTP_STATE_QUIT;
		    break;

		    /*
		     * Receive the RSET response.
		     * 
		     * The SMTP_STATE_ABORT sender state is entered by the
		     * sender when it has verified all recipients; or it is
		     * entered by the receiver when all recipients are
		     * verified or rejected, and is then left before the
		     * bottom of the main loop.
		     * 
		     * XXX Do not change the connection caching state here, even
		     * if the server rejected RSET or if the connection
		     * caching timer expired between generating the command
		     * and processing the reply, otherwise the sender and
		     * receiver loops get out of sync. The caller will call
		     * smtp_quit() if appropriate.
		     */
		case SMTP_STATE_ABORT:
		    recv_state = (var_skip_quit_resp || THIS_SESSION_IS_CACHED ?
				  SMTP_STATE_LAST : SMTP_STATE_QUIT);
		    break;

		    /*
		     * This is the initial receiver state from smtp_rset().
		     * It is used to find out the status of a cached session
		     * before attempting mail delivery.
		     */
		case SMTP_STATE_RSET:
		    if (resp->code / 100 != 2)
			CANT_RSET_THIS_SESSION;
		    recv_state = SMTP_STATE_LAST;
		    break;

		    /*
		     * Receive, but otherwise ignore, the QUIT response.
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
	    sndbuffree = session->sndbufsize;

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
		if (THIS_SESSION_IS_EXPIRED)
		    DONT_CACHE_THIS_SESSION;
		next_state = THIS_SESSION_IS_CACHED ?
		    SMTP_STATE_LAST : SMTP_STATE_QUIT;
		/* XXX Also: record if non-delivering session. */
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
	 * 
	 * XXX If there is a hard MIME error while downgrading to 7-bit mail,
	 * disconnect ungracefully, because there is no other way to cancel a
	 * transaction in progress.
	 */
	if (send_state == SMTP_STATE_DOT && nrcpt > 0) {

	    smtp_timeout_setup(session->stream,
			       var_smtp_data1_tmout);

	    if ((except = vstream_setjmp(session->stream)) == 0) {

		if (vstream_fseek(state->src, request->data_offset, SEEK_SET) < 0)
		    msg_fatal("seek queue file: %m");

		downgrading = SMTP_MIME_DOWNGRADE(session, request);

		/*
		 * XXX Don't downgrade just because generic_maps is turned
		 * on.
		 */
#define SMTP_ANY_CHECKS (smtp_header_checks || smtp_body_checks)

		if (downgrading || smtp_generic_maps || SMTP_ANY_CHECKS)
		    session->mime_state = mime_state_alloc(downgrading ?
							   MIME_OPT_DOWNGRADE
						 | MIME_OPT_REPORT_NESTING :
						      SMTP_ANY_CHECKS == 0 ?
						     MIME_OPT_DISABLE_MIME :
							   0,
							   smtp_generic_maps
						     || smtp_header_checks ?
						       smtp_header_rewrite :
							   smtp_header_out,
						     (MIME_STATE_ANY_END) 0,
							   smtp_body_checks ?
							 smtp_body_rewrite :
							   smtp_text_out,
						     (MIME_STATE_ANY_END) 0,
						   (MIME_STATE_ERR_PRINT) 0,
							   (void *) state);
		state->space_left = var_smtp_line_limit;

		while ((rec_type = rec_get(state->src, session->scratch, 0)) > 0) {
		    if (rec_type != REC_TYPE_NORM && rec_type != REC_TYPE_CONT)
			break;
		    if (session->mime_state == 0) {
			smtp_text_out((void *) state, rec_type,
				      vstring_str(session->scratch),
				      VSTRING_LEN(session->scratch),
				      (off_t) 0);
		    } else {
			mime_errs =
			    mime_state_update(session->mime_state, rec_type,
					      vstring_str(session->scratch),
					      VSTRING_LEN(session->scratch));
			if (mime_errs) {
			    smtp_mime_fail(state, mime_errs);
			    RETURN(0);
			}
		    }
		    prev_type = rec_type;
		}

		if (session->mime_state) {

		    /*
		     * The cleanup server normally ends MIME content with a
		     * normal text record. The following code is needed to
		     * flush an internal buffer when someone submits 8-bit
		     * mail not ending in newline via /usr/sbin/sendmail
		     * while MIME input processing is turned off, and MIME
		     * 8bit->7bit conversion is requested upon delivery.
		     * 
		     * Or some error while doing generic address mapping.
		     */
		    mime_errs =
			mime_state_update(session->mime_state, rec_type, "", 0);
		    if (mime_errs) {
			smtp_mime_fail(state, mime_errs);
			RETURN(0);
		    }
		} else if (prev_type == REC_TYPE_CONT)	/* missing newline */
		    smtp_fputs("", 0, session->stream);
		if ((session->features & SMTP_FEATURE_PIX_DELAY_DOTCRLF) != 0
		    && request->msg_stats.incoming_arrival.tv_sec
		  <= vstream_ftime(session->stream) - var_smtp_pix_thresh) {
		    smtp_flush(session->stream);/* hurts performance */
		    sleep(var_smtp_pix_delay);	/* not to mention this */
		}
		if (vstream_ferror(state->src))
		    msg_fatal("queue file read error");
		if (rec_type != REC_TYPE_XTRA) {
		    msg_warn("%s: bad record type: %d in message content",
			     request->queue_id, rec_type);
		    fail_status = smtp_mesg_fail(state, DSN_BY_LOCAL_MTA,
					     SMTP_RESP_FAKE(&fake, "5.3.0"),
					     "unreadable mail queue entry");
		    /* Bailing out, abort stream with prejudice */
		    (void) vstream_fpurge(session->stream, VSTREAM_PURGE_BOTH);
		    DONT_USE_DEAD_SESSION;
		    /* If bounce_append() succeeded, status is still 0 */
		    if (state->status == 0)
			(void) mark_corrupt(state->src);
		    /* Don't override smtp_mesg_fail() here. */
		    RETURN(fail_status);
		}
	    } else {
		if (!LOST_CONNECTION_INSIDE_DATA)
		    RETURN(smtp_stream_except(state, except,
					      "sending message body"));

		/*
		 * We will clear the stream error flag to try and read a
		 * premature 5XX response, so it is important to flush any
		 * unwritten data. Otherwise, we will try to flush it again
		 * before reading, which may incur an unnecessary delay and
		 * will prevent the reading of any response that is not
		 * already buffered (bundled with the DATA 354 response).
		 * 
		 * Not much point in sending QUIT at this point, skip right to
		 * SMTP_STATE_LAST. The read engine above will likewise avoid
		 * looking for a QUIT response.
		 */
		(void) vstream_fpurge(session->stream, VSTREAM_PURGE_WRITE);
		next_state = SMTP_STATE_LAST;
	    }
	}

	/*
	 * Copy the next command to the buffer and update the sender state.
	 */
	if (except == 0) {
	    if (sndbuffree > 0)
		sndbuffree -= VSTRING_LEN(next_command) + 2;
	    smtp_chat_cmd(session, "%s", vstring_str(next_command));
	} else {
	    DONT_CACHE_THIS_SESSION;
	}
	send_state = next_state;
	send_rcpt = next_rcpt;
    } while (recv_state != SMTP_STATE_LAST);
    RETURN(0);
}

/* smtp_xfer - send a batch of envelope information and the message data */

int     smtp_xfer(SMTP_STATE *state)
{
    DELIVER_REQUEST *request = state->request;
    SMTP_SESSION *session = state->session;
    SMTP_RESP fake;
    int     send_state;
    int     recv_state;
    int     send_name_addr;
    int     result;

    /*
     * Sanity check. Recipients should be unmarked at this point.
     */
    if (SMTP_RCPT_LEFT(state) <= 0)
	msg_panic("smtp_xfer: bad recipient count: %d",
		  SMTP_RCPT_LEFT(state));
    if (SMTP_RCPT_ISMARKED(request->rcpt_list.info))
	msg_panic("smtp_xfer: bad recipient status: %d",
		  request->rcpt_list.info->u.status);

    /*
     * See if we should even try to send this message at all. This code sits
     * here rather than in the EHLO processing code, because of SMTP
     * connection caching.
     */
    if (session->size_limit > 0 && session->size_limit < request->data_size) {
	smtp_mesg_fail(state, DSN_BY_LOCAL_MTA,
		       SMTP_RESP_FAKE(&fake, "5.3.4"),
		    "message size %lu exceeds size limit %.0f of server %s",
		       request->data_size, (double) session->size_limit,
		       session->namaddr);
	/* Redundant. We abort this delivery attempt. */
	state->misc_flags |= SMTP_MISC_FLAG_COMPLETE_SESSION;
	return (0);
    }

    /*
     * Use the XFORWARD command to forward client attributes only when a
     * minimal amount of information is available.
     */
    send_name_addr =
	var_smtp_send_xforward
	&& (((session->features & SMTP_FEATURE_XFORWARD_NAME)
	     && DEL_REQ_ATTR_AVAIL(request->client_name))
	    || ((session->features & SMTP_FEATURE_XFORWARD_ADDR)
		&& DEL_REQ_ATTR_AVAIL(request->client_addr)));
    session->send_proto_helo =
	var_smtp_send_xforward
	&& (((session->features & SMTP_FEATURE_XFORWARD_PROTO)
	     && DEL_REQ_ATTR_AVAIL(request->client_proto))
	    || ((session->features & SMTP_FEATURE_XFORWARD_HELO)
		&& DEL_REQ_ATTR_AVAIL(request->client_helo)));
    if (send_name_addr)
	recv_state = send_state = SMTP_STATE_XFORWARD_NAME_ADDR;
    else if (session->send_proto_helo)
	recv_state = send_state = SMTP_STATE_XFORWARD_PROTO_HELO;
    else
	recv_state = send_state = SMTP_STATE_MAIL;

    /*
     * Remember this session's "normal completion", even if the server 4xx-ed
     * some or all recipients. Connection or handshake errors with a later MX
     * host should not cause this destination be marked as unreachable.
     */
    result = smtp_loop(state, send_state, recv_state);

    if (result == 0
    /* Just in case */
	&& vstream_ferror(session->stream) == 0
	&& vstream_feof(session->stream) == 0)
	state->misc_flags |= SMTP_MISC_FLAG_COMPLETE_SESSION;

    return (result);
}

/* smtp_rset - send a lone RSET command */

int     smtp_rset(SMTP_STATE *state)
{

    /*
     * This works because SMTP_STATE_RSET is a dedicated sender/recipient
     * entry state, with SMTP_STATE_LAST as next sender/recipient state.
     */
    return (smtp_loop(state, SMTP_STATE_RSET, SMTP_STATE_RSET));
}

/* smtp_quit - send a lone QUIT command */

int     smtp_quit(SMTP_STATE *state)
{

    /*
     * This works because SMTP_STATE_QUIT is the last state with a sender
     * action, with SMTP_STATE_LAST as the next sender/recipient state.
     */
    return (smtp_loop(state, SMTP_STATE_QUIT, var_skip_quit_resp ?
		      SMTP_STATE_LAST : SMTP_STATE_QUIT));
}
