/*	$NetBSD: postscreen_smtpd.c,v 1.1.1.2 2013/01/02 18:59:04 tron Exp $	*/

/*++
/* NAME
/*	postscreen_smtpd 3
/* SUMMARY
/*	postscreen built-in SMTP server engine
/* SYNOPSIS
/*	#include <postscreen.h>
/*
/*	void	psc_smtpd_pre_jail_init(void)
/*
/*	void	psc_smtpd_init(void)
/*
/*	void	psc_smtpd_tests(state)
/*	PSC_STATE *state;
/*
/*	void	PSC_SMTPD_X21(state, final_reply)
/*	PSC_STATE *state;
/*	const char *final_reply;
/* DESCRIPTION
/*	psc_smtpd_pre_jail_init() performs one-time per-process
/*	initialization during the "before chroot" execution phase.
/*
/*	psc_smtpd_init() performs one-time per-process initialization.
/*
/*	psc_smtpd_tests() starts up an SMTP server engine for deep
/*	protocol tests and for collecting helo/sender/recipient
/*	information.
/*
/*	PSC_SMTPD_X21() redirects the SMTP client to an SMTP server
/*	engine, which sends the specified final reply at the first
/*	legitimate opportunity without doing any protocol tests.
/*
/*	Unlike the Postfix SMTP server, this engine does not announce
/*	PIPELINING support. This exposes spambots that pipeline
/*	their commands anyway. Like the Postfix SMTP server, this
/*	engine will accept input with bare newline characters. To
/*	pass the "pipelining" and "bare newline" test, the client
/*	has to properly speak SMTP all the way to the RCPT TO
/*	command. These tests fail if the client violates the protocol
/*	at any stage.
/*
/*	No support is announced for AUTH, XCLIENT or XFORWARD.
/*	Clients that need this should be whitelisted or should talk
/*	directly to the submission service.
/*
/*	The engine rejects RCPT TO and VRFY commands with the
/*	state->rcpt_reply response which depends on program history,
/*	rejects ETRN with a generic response, and closes the
/*	connection after QUIT.
/*
/*	Since this engine defers or rejects all non-junk commands,
/*	there is no point maintaining separate counters for "error"
/*	commands and "junk" commands.  Instead, the engine maintains
/*	a per-session command counter, and terminates the session
/*	with a 421 reply when the command count exceeds the limit.
/*
/*	We limit the command count, as well as the total time to
/*	receive a command. This limits the time per client more
/*	effectively than would be possible with read() timeouts.
/*
/*	There is no concern about getting blocked on output.  The
/*	psc_send() routine uses non-blocking output, and discards
/*	output that the client is not willing to receive.
/* PROTOCOL INSPECTION VERSUS CONTENT INSPECTION
/*	The goal of postscreen is to keep spambots away from Postfix.
/*	To recognize spambots, postscreen measures properties of
/*	the client IP address and of the client SMTP protocol
/*	implementation.  These client properties don't change with
/*	each delivery attempt.  Therefore it is possible to make a
/*	long-term decision after a single measurement.  For example,
/*	allow a good client to skip the DNSBL test for 24 hours,
/*	or to skip the pipelining test for one week.
/*
/*	If postscreen were to measure properties of message content
/*	(MIME compliance, etc.) then it would measure properties
/*	that may change with each delivery attempt.  Here, it would
/*	be wrong to make a long-term decision after a single
/*	measurement. Instead, postscreen would need to develop a
/*	ranking based on the content of multiple messages from the
/*	same client.
/*
/*	Many spambots avoid spamming the same site repeatedly.
/*	Thus, postscreen must make decisions after a single
/*	measurement. Message content is not a good indicator for
/*	making long-term decisions after single measurements, and
/*	that is why postscreen does not inspect message content.
/* REJECTING RCPT TO VERSUS SENDING LIVE SOCKETS TO SMTPD(8)
/*	When post-handshake protocol tests are enabled, postscreen
/*	rejects the RCPT TO command from a good client, and forces
/*	it to deliver mail in a later session. This is why
/*	post-handshake protocol tests have a longer expiration time
/*	than pre-handshake tests.
/*
/*	Instead, postscreen could send the network socket to smtpd(8)
/*	and ship the session history (including TLS and other SMTP
/*	or non-SMTP attributes) as auxiliary data. The Postfix SMTP
/*	server would then use new code to replay the session history,
/*	and would use existing code to validate the client, helo,
/*	sender and recipient address.
/*
/*	Such an approach would increase the implementation and
/*	maintenance effort, because:
/*
/*	1) New replay code would be needed in smtpd(8), such that
/*	the HELO, EHLO, and MAIL command handlers can delay their
/*	error responses until the RCPT TO reply.
/*
/*	2) postscreen(8) would have to implement more of smtpd(8)'s
/*	syntax checks, to avoid confusing delayed "syntax error"
/*	and other error responses syntax error responses while
/*	replaying history.
/*
/*	3) New code would be needed in postscreen(8) and smtpd(8)
/*	to send and receive the session history (including TLS and
/*	other SMTP or non-SMTP attributes) as auxiliary data while
/*	sending the network socket from postscreen(8) to smtpd(8).
/* REJECTING RCPT TO VERSUS PROXYING LIVE SESSIONS TO SMTPD(8)
/*	An alternative would be to proxy the session history to a
/*	real Postfix SMTP process, presumably passing TLS and other
/*	attributes via an extended XCLIENT implementation. That
/*	would require all the work described in 2) above, plus
/*	duplication of all the features of the smtpd(8) TLS engine,
/*	plus additional XCLIENT support for a lot more attributes.
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
#include <ctype.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <stringops.h>
#include <mymalloc.h>
#include <iostuff.h>
#include <vstring.h>

/* Global library. */

#include <mail_params.h>
#include <mail_proto.h>
#include <is_header.h>
#include <string_list.h>
#include <maps.h>
#include <ehlo_mask.h>
#include <lex_822.h>

/* TLS library. */

#include <tls.h>

/* Application-specific. */

#include <postscreen.h>

 /*
  * Plan for future body processing. See smtp-sink.c. For now, we have no
  * per-session push-back except for the single-character push-back that
  * VSTREAM guarantees after we read one character.
  */
#define PSC_SMTPD_HAVE_PUSH_BACK(state)	(0)
#define PSC_SMTPD_PUSH_BACK_CHAR(state, ch) \
	vstream_ungetc((state)->smtp_client_stream, (ch))
#define PSC_SMTPD_NEXT_CHAR(state) \
	VSTREAM_GETC((state)->smtp_client_stream)

#define PSC_SMTPD_BUFFER_EMPTY(state) \
	(!PSC_SMTPD_HAVE_PUSH_BACK(state) \
	&& vstream_peek((state)->smtp_client_stream) <= 0)

#define PSC_SMTPD_PEEK_DATA(state) \
	vstream_peek_data((state)->smtp_client_stream)
#define PSC_SMTPD_PEEK_LEN(state) \
	vstream_peek((state)->smtp_client_stream)

 /*
  * Dynamic reply strings. To minimize overhead we format these once.
  */
static char *psc_smtpd_greeting;	/* smtp banner */
static char *psc_smtpd_helo_reply;	/* helo reply */
static char *psc_smtpd_ehlo_reply_plain;/* multi-line ehlo reply, non-TLS */
static char *psc_smtpd_ehlo_reply_tls;	/* multi-line ehlo reply, with TLS */
static char *psc_smtpd_timeout_reply;	/* timeout reply */
static char *psc_smtpd_421_reply;	/* generic final_reply value */

 /*
  * Forward declaration, needed by PSC_CLEAR_EVENT_REQUEST.
  */
static void psc_smtpd_time_event(int, char *);
static void psc_smtpd_read_event(int, char *);

 /*
  * Encapsulation. The STARTTLS, EHLO and AUTH command handlers temporarily
  * suspend SMTP command events, send an asynchronous proxy request, and
  * resume SMTP command events after receiving the asynchrounous proxy
  * response (the EHLO handler must asynchronously talk to the auth server
  * before it can announce the SASL mechanism list; the list can depend on
  * the client IP address and on the presence on TLS encryption).
  */
#define PSC_RESUME_SMTP_CMD_EVENTS(state) do { \
	PSC_READ_EVENT_REQUEST2(vstream_fileno((state)->smtp_client_stream), \
			       psc_smtpd_read_event, psc_smtpd_time_event, \
			       (char *) (state), PSC_EFF_CMD_TIME_LIMIT); \
	if (!PSC_SMTPD_BUFFER_EMPTY(state)) \
	    psc_smtpd_read_event(EVENT_READ, (char *) state); \
    } while (0)

#define PSC_SUSPEND_SMTP_CMD_EVENTS(state) \
    PSC_CLEAR_EVENT_REQUEST(vstream_fileno((state)->smtp_client_stream), \
			   psc_smtpd_time_event, (char *) (state));

 /*
  * Command parser support.
  */
#define PSC_SMTPD_NEXT_TOKEN(ptr) mystrtok(&(ptr), " ")

 /*
  * EHLO keyword filter
  */
static MAPS *psc_ehlo_discard_maps;
static int psc_ehlo_discard_mask;

 /*
  * Command editing filter.
  */
static DICT *psc_cmd_filter;

 /*
  * Encapsulation. We must not forget turn off input/timer events when we
  * terminate the SMTP protocol engine.
  * 
  * It would be safer to turn off input/timer events after each event, and to
  * turn on input/timer events again when we want more input. But experience
  * with the Postfix smtp-source and smtp-sink tools shows that this would
  * noticeably increase the run-time cost.
  */
#define PSC_CLEAR_EVENT_DROP_SESSION_STATE(state, event, reply) do { \
	PSC_CLEAR_EVENT_REQUEST(vstream_fileno((state)->smtp_client_stream), \
				   (event), (char *) (state)); \
	PSC_DROP_SESSION_STATE((state), (reply)); \
    } while (0);

#define PSC_CLEAR_EVENT_HANGUP(state, event) do { \
	PSC_CLEAR_EVENT_REQUEST(vstream_fileno((state)->smtp_client_stream), \
				   (event), (char *) (state)); \
	psc_hangup_event(state); \
    } while (0);

/* psc_helo_cmd - record HELO and respond */

static int psc_helo_cmd(PSC_STATE *state, char *args)
{
    char   *helo_name = PSC_SMTPD_NEXT_TOKEN(args);

    /*
     * smtpd(8) incompatibility: we ignore extra words; smtpd(8) saves them.
     */
    if (helo_name == 0)
	return (PSC_SEND_REPLY(state, "501 Syntax: HELO hostname\r\n"));

    PSC_STRING_UPDATE(state->helo_name, helo_name);
    PSC_STRING_RESET(state->sender);
    /* Don't downgrade state->protocol, in case some test depends on this. */
    return (PSC_SEND_REPLY(state, psc_smtpd_helo_reply));
}

/* psc_smtpd_format_ehlo_reply - format EHLO response */

static void psc_smtpd_format_ehlo_reply(VSTRING *buf, int discard_mask
				   /* , const char *sasl_mechanism_list */ )
{
    const char *myname = "psc_smtpd_format_ehlo_reply";
    int     saved_len = 0;

    if (msg_verbose)
	msg_info("%s: discard_mask %s", myname, str_ehlo_mask(discard_mask));

#define PSC_EHLO_APPEND(save, buf, fmt) do { \
	(save) = LEN(buf); \
	vstring_sprintf_append((buf), (fmt)); \
    } while (0)

#define PSC_EHLO_APPEND1(save, buf, fmt, arg1) do { \
	(save) = LEN(buf); \
	vstring_sprintf_append((buf), (fmt), (arg1)); \
    } while (0)

    vstring_sprintf(psc_temp, "250-%s\r\n", var_myhostname);
    if ((discard_mask & EHLO_MASK_SIZE) == 0) {
	if (var_message_limit)
	    PSC_EHLO_APPEND1(saved_len, psc_temp, "250-SIZE %lu\r\n",
			     (unsigned long) var_message_limit);
	else
	    PSC_EHLO_APPEND(saved_len, psc_temp, "250-SIZE\r\n");
    }
    if ((discard_mask & EHLO_MASK_VRFY) == 0 && var_disable_vrfy_cmd == 0)
	PSC_EHLO_APPEND(saved_len, psc_temp, "250-VRFY\r\n");
    if ((discard_mask & EHLO_MASK_ETRN) == 0)
	PSC_EHLO_APPEND(saved_len, psc_temp, "250-ETRN\r\n");
    if ((discard_mask & EHLO_MASK_STARTTLS) == 0 && var_psc_use_tls)
	PSC_EHLO_APPEND(saved_len, psc_temp, "250-STARTTLS\r\n");
#ifdef TODO_SASL_AUTH
    if ((discard_mask & EHLO_MASK_AUTH) == 0 && sasl_mechanism_list
	&& (!var_psc_tls_auth_only || (discard_mask & EHLO_MASK_STARTTLS))) {
	PSC_EHLO_APPEND1(saved_len, psc_temp, "AUTH %s", sasl_mechanism_list);
	if (var_broken_auth_clients)
	    PSC_EHLO_APPEND1(saved_len, psc_temp, "AUTH=%s", sasl_mechanism_list);
    }
#endif
    if ((discard_mask & EHLO_MASK_ENHANCEDSTATUSCODES) == 0)
	PSC_EHLO_APPEND(saved_len, psc_temp, "250-ENHANCEDSTATUSCODES\r\n");
    if ((discard_mask & EHLO_MASK_8BITMIME) == 0)
	PSC_EHLO_APPEND(saved_len, psc_temp, "250-8BITMIME\r\n");
    if ((discard_mask & EHLO_MASK_DSN) == 0)
	PSC_EHLO_APPEND(saved_len, psc_temp, "250-DSN\r\n");
    STR(psc_temp)[saved_len + 3] = ' ';
}

/* psc_ehlo_cmd - record EHLO and respond */

static int psc_ehlo_cmd(PSC_STATE *state, char *args)
{
    char   *helo_name = PSC_SMTPD_NEXT_TOKEN(args);
    const char *ehlo_words;
    int     discard_mask;
    char   *reply;

    /*
     * smtpd(8) incompatibility: we ignore extra words; smtpd(8) saves them.
     */
    if (helo_name == 0)
	return (PSC_SEND_REPLY(state, "501 Syntax: EHLO hostname\r\n"));

    PSC_STRING_UPDATE(state->helo_name, helo_name);
    PSC_STRING_RESET(state->sender);
    state->protocol = MAIL_PROTO_ESMTP;

    /*
     * smtpd(8) compatibility: dynamic reply filtering.
     */
    if (psc_ehlo_discard_maps != 0
	&& (ehlo_words = psc_maps_find(psc_ehlo_discard_maps,
				       state->smtp_client_addr, 0)) != 0
	&& (discard_mask = ehlo_mask(ehlo_words)) != psc_ehlo_discard_mask) {
	if (discard_mask && !(discard_mask & EHLO_MASK_SILENT))
	    msg_info("[%s]%s: discarding EHLO keywords: %s",
		  PSC_CLIENT_ADDR_PORT(state), str_ehlo_mask(discard_mask));
	if (state->flags & PSC_STATE_FLAG_USING_TLS)
	    discard_mask |= EHLO_MASK_STARTTLS;
	psc_smtpd_format_ehlo_reply(psc_temp, discard_mask);
	reply = STR(psc_temp);
	state->ehlo_discard_mask = discard_mask;
    } else if (psc_ehlo_discard_maps && psc_ehlo_discard_maps->error) {
	msg_fatal("%s lookup error for %s",
		  psc_ehlo_discard_maps->title, state->smtp_client_addr);
    } else if (state->flags & PSC_STATE_FLAG_USING_TLS) {
	reply = psc_smtpd_ehlo_reply_tls;
	state->ehlo_discard_mask = psc_ehlo_discard_mask | EHLO_MASK_STARTTLS;
    } else {
	reply = psc_smtpd_ehlo_reply_plain;
	state->ehlo_discard_mask = psc_ehlo_discard_mask;
    }
    return (PSC_SEND_REPLY(state, reply));
}

/* psc_starttls_resume - resume the SMTP protocol after tlsproxy activation */

static void psc_starttls_resume(int unused_event, char *context)
{
    const char *myname = "psc_starttls_resume";
    PSC_STATE *state = (PSC_STATE *) context;

    /*
     * Reset SMTP server state if STARTTLS was successful.
     */
    if (state->flags & PSC_STATE_FLAG_USING_TLS) {
	/* Purge the push-back buffer, when implemented. */
	PSC_STRING_RESET(state->helo_name);
	PSC_STRING_RESET(state->sender);
#ifdef TODO_SASL_AUTH
	/* Reset SASL AUTH state. Dovecot responses may change. */
#endif
    }

    /*
     * Resume read/timeout events. If we still have unread input, resume the
     * command processor immediately.
     */
    PSC_RESUME_SMTP_CMD_EVENTS(state);
}

/* psc_starttls_cmd - activate the tlsproxy server */

static int psc_starttls_cmd(PSC_STATE *state, char *args)
{
    const char *myname = "psc_starttls_cmd";

    /*
     * smtpd(8) incompatibility: we can't send a 4XX reply that TLS is
     * unavailable when tlsproxy(8) detects the problem too late.
     */
    if (PSC_SMTPD_NEXT_TOKEN(args) != 0)
	return (PSC_SEND_REPLY(state, "501 Syntax: EHLO hostname\r\n"));
    if (state->flags & PSC_STATE_FLAG_USING_TLS)
	return (PSC_SEND_REPLY(state,
			       "554 5.5.1 Error: TLS already active\r\n"));
    if (var_psc_use_tls == 0 || (state->ehlo_discard_mask & EHLO_MASK_STARTTLS))
	return (PSC_SEND_REPLY(state,
			   "502 5.5.1 Error: command not implemented\r\n"));

    /*
     * Suspend the SMTP protocol until psc_starttls_resume() is called.
     */
    PSC_SUSPEND_SMTP_CMD_EVENTS(state);
    psc_starttls_open(state, psc_starttls_resume);
    return (0);
}

/* psc_extract_addr - extract MAIL/RCPT address, unquoted form */

static char *psc_extract_addr(VSTRING *result, const char *string)
{
    const unsigned char *cp = (const unsigned char *) string;
    char   *addr;
    char   *colon;
    int     stop_at;
    int     inquote = 0;

    /*
     * smtpd(8) incompatibility: we allow more invalid address forms, and we
     * don't validate recipients. We are not going to deliver them so we
     * won't have to worry about deliverability. This may have to change when
     * we pass the socket to a real SMTP server and replay message envelope
     * commands.
     */

    /* Skip SP characters. */
    while (*cp && *cp == ' ')
	cp++;

    /* Choose the terminator for <addr> or bare addr. */
    if (*cp == '<') {
	cp++;
	stop_at = '>';
    } else {
	stop_at = ' ';
    }

    /* Skip to terminator or end. */
    VSTRING_RESET(result);
    for ( /* void */ ; *cp; cp++) {
	if (!inquote && *cp == stop_at)
	    break;
	if (*cp == '"') {
	    inquote = !inquote;
	} else {
	    if (*cp == '\\' && *++cp == 0)
		break;
	    VSTRING_ADDCH(result, *cp);
	}
    }
    VSTRING_TERMINATE(result);

    /*
     * smtpd(8) compatibility: truncate deprecated route address form. This
     * is primarily to simplify logfile analysis.
     */
    addr = STR(result);
    if (*addr == '@' && (colon = strchr(addr, ':')) != 0)
	addr = colon + 1;
    return (addr);
}

/* psc_mail_cmd - record MAIL and respond */

static int psc_mail_cmd(PSC_STATE *state, char *args)
{
    char   *colon;
    char   *addr;

    /*
     * smtpd(8) incompatibility: we never reject the sender, and we ignore
     * additional arguments.
     */
    if (var_psc_helo_required && state->helo_name == 0)
	return (PSC_SEND_REPLY(state,
			       "503 5.5.1 Error: send HELO/EHLO first\r\n"));
    if (state->sender != 0)
	return (PSC_SEND_REPLY(state,
			       "503 5.5.1 Error: nested MAIL command\r\n"));
    if (args == 0 || (colon = strchr(args, ':')) == 0)
	return (PSC_SEND_REPLY(state,
			       "501 5.5.4 Syntax: MAIL FROM:<address>\r\n"));
    if ((addr = psc_extract_addr(psc_temp, colon + 1)) == 0)
	return (PSC_SEND_REPLY(state,
			       "501 5.1.7 Bad sender address syntax\r\n"));
    PSC_STRING_UPDATE(state->sender, addr);
    return (PSC_SEND_REPLY(state, "250 2.1.0 Ok\r\n"));
}

/* psc_soften_reply - copy and soft-bounce a reply */

static char *psc_soften_reply(const char *reply)
{
    static VSTRING *buf = 0;

    if (buf == 0)
	buf = vstring_alloc(100);
    vstring_strcpy(buf, reply);
    if (reply[0] == '5')
	STR(buf)[0] = '4';
    if (reply[4] == '5')
	STR(buf)[4] = '4';
    return (STR(buf));
}

/* psc_rcpt_cmd record RCPT and respond */

static int psc_rcpt_cmd(PSC_STATE *state, char *args)
{
    char   *colon;
    char   *addr;

    /*
     * smtpd(8) incompatibility: we reject all recipients, and ignore
     * additional arguments.
     */
    if (state->sender == 0)
	return (PSC_SEND_REPLY(state,
			       "503 5.5.1 Error: need MAIL command\r\n"));
    if (args == 0 || (colon = strchr(args, ':')) == 0)
	return (PSC_SEND_REPLY(state,
			       "501 5.5.4 Syntax: RCPT TO:<address>\r\n"));
    if ((addr = psc_extract_addr(psc_temp, colon + 1)) == 0)
	return (PSC_SEND_REPLY(state,
			     "501 5.1.3 Bad recipient address syntax\r\n"));
    msg_info("NOQUEUE: reject: RCPT from [%s]:%s: %.*s; "
	     "from=<%s>, to=<%s>, proto=%s, helo=<%s>",
	     PSC_CLIENT_ADDR_PORT(state),
	     (int) strlen(state->rcpt_reply) - 2,
	     var_soft_bounce == 0 ? state->rcpt_reply :
	     psc_soften_reply(state->rcpt_reply),
	     state->sender, addr, state->protocol,
	     state->helo_name ? state->helo_name : "");
    return (PSC_SEND_REPLY(state, state->rcpt_reply));
}

/* psc_data_cmd - respond to DATA and disconnect */

static int psc_data_cmd(PSC_STATE *state, char *args)
{

    /*
     * smtpd(8) incompatibility: we reject all requests.
     */
    if (PSC_SMTPD_NEXT_TOKEN(args) != 0)
	return (PSC_SEND_REPLY(state,
			       "501 5.5.4 Syntax: DATA\r\n"));
    if (state->sender == 0)
	return (PSC_SEND_REPLY(state,
			       "503 5.5.1 Error: need RCPT command\r\n"));

    /*
     * We really would like to hang up the connection as early as possible,
     * so that we dont't have to deal with broken zombies that fall silent at
     * the first reject response. For now we rely on stress-dependent command
     * read timeouts.
     * 
     * If we proceed into the data phase, enforce over-all DATA time limit.
     */
    return (PSC_SEND_REPLY(state,
			   "554 5.5.1 Error: no valid recipients\r\n"));
}

/* psc_rset_cmd - reset, send 250 OK */

static int psc_rset_cmd(PSC_STATE *state, char *unused_args)
{
    PSC_STRING_RESET(state->sender);
    return (PSC_SEND_REPLY(state, "250 2.0.0 Ok\r\n"));
}

/* psc_noop_cmd - respond to something */

static int psc_noop_cmd(PSC_STATE *state, char *unused_args)
{
    return (PSC_SEND_REPLY(state, "250 2.0.0 Ok\r\n"));
}

/* psc_vrfy_cmd - respond to VRFY */

static int psc_vrfy_cmd(PSC_STATE *state, char *args)
{

    /*
     * smtpd(8) incompatibility: we reject all requests, and ignore
     * additional arguments.
     */
    if (PSC_SMTPD_NEXT_TOKEN(args) == 0)
	return (PSC_SEND_REPLY(state,
			       "501 5.5.4 Syntax: VRFY address\r\n"));
    if (var_psc_disable_vrfy)
	return (PSC_SEND_REPLY(state,
			       "502 5.5.1 VRFY command is disabled\r\n"));
    return (PSC_SEND_REPLY(state, state->rcpt_reply));
}

/* psc_etrn_cmd - reset, send 250 OK */

static int psc_etrn_cmd(PSC_STATE *state, char *args)
{

    /*
     * smtpd(8) incompatibility: we reject all requests, and ignore
     * additional arguments.
     */
    if (var_psc_helo_required && state->helo_name == 0)
	return (PSC_SEND_REPLY(state,
			       "503 5.5.1 Error: send HELO/EHLO first\r\n"));
    if (PSC_SMTPD_NEXT_TOKEN(args) == 0)
	return (PSC_SEND_REPLY(state,
			       "500 Syntax: ETRN domain\r\n"));
    return (PSC_SEND_REPLY(state, "458 Unable to queue messages\r\n"));
}

/* psc_quit_cmd - respond to QUIT and disconnect */

static int psc_quit_cmd(PSC_STATE *state, char *unused_args)
{
    const char *myname = "psc_quit_cmd";

    PSC_CLEAR_EVENT_DROP_SESSION_STATE(state, psc_smtpd_time_event,
				       "221 2.0.0 Bye\r\n");
    /* Caution: state is now a dangling pointer. */
    return (0);
}

/* psc_smtpd_time_event - handle per-session time limit */

static void psc_smtpd_time_event(int event, char *context)
{
    const char *myname = "psc_smtpd_time_event";
    PSC_STATE *state = (PSC_STATE *) context;

    if (msg_verbose > 1)
	msg_info("%s: sq=%d cq=%d event %d on smtp socket %d from [%s]:%s flags=%s",
		 myname, psc_post_queue_length, psc_check_queue_length,
		 event, vstream_fileno(state->smtp_client_stream),
		 state->smtp_client_addr, state->smtp_client_port,
		 psc_print_state_flags(state->flags, myname));

    msg_info("COMMAND TIME LIMIT from [%s]:%s", PSC_CLIENT_ADDR_PORT(state));
    PSC_CLEAR_EVENT_DROP_SESSION_STATE(state, psc_smtpd_time_event,
				       psc_smtpd_timeout_reply);
}

 /*
  * The table of all SMTP commands that we know.
  */
typedef struct {
    const char *name;
    int     (*action) (PSC_STATE *, char *);
    int     flags;			/* see below */
} PSC_SMTPD_COMMAND;

#define PSC_SMTPD_CMD_FLAG_NONE		(0)	/* no flags (i.e. disabled) */
#define PSC_SMTPD_CMD_FLAG_ENABLE	(1<<0)	/* command is enabled */
#define PSC_SMTPD_CMD_FLAG_DESTROY	(1<<1)	/* dangling pointer alert */
#define PSC_SMTPD_CMD_FLAG_PRE_TLS	(1<<2)	/* allowed with mandatory TLS */
#define PSC_SMTPD_CMD_FLAG_SUSPEND	(1<<3)	/* suspend command engine */

static const PSC_SMTPD_COMMAND command_table[] = {
    "HELO", psc_helo_cmd, PSC_SMTPD_CMD_FLAG_ENABLE | PSC_SMTPD_CMD_FLAG_PRE_TLS,
    "EHLO", psc_ehlo_cmd, PSC_SMTPD_CMD_FLAG_ENABLE | PSC_SMTPD_CMD_FLAG_PRE_TLS,
    "STARTTLS", psc_starttls_cmd, PSC_SMTPD_CMD_FLAG_ENABLE | PSC_SMTPD_CMD_FLAG_PRE_TLS | PSC_SMTPD_CMD_FLAG_SUSPEND,
    "XCLIENT", psc_noop_cmd, PSC_SMTPD_CMD_FLAG_NONE,
    "XFORWARD", psc_noop_cmd, PSC_SMTPD_CMD_FLAG_NONE,
    "AUTH", psc_noop_cmd, PSC_SMTPD_CMD_FLAG_NONE,
    "MAIL", psc_mail_cmd, PSC_SMTPD_CMD_FLAG_ENABLE,
    "RCPT", psc_rcpt_cmd, PSC_SMTPD_CMD_FLAG_ENABLE,
    "DATA", psc_data_cmd, PSC_SMTPD_CMD_FLAG_ENABLE,
    /* ".", psc_dot_cmd, PSC_SMTPD_CMD_FLAG_NONE, */
    "RSET", psc_rset_cmd, PSC_SMTPD_CMD_FLAG_ENABLE,
    "NOOP", psc_noop_cmd, PSC_SMTPD_CMD_FLAG_ENABLE | PSC_SMTPD_CMD_FLAG_PRE_TLS,
    "VRFY", psc_vrfy_cmd, PSC_SMTPD_CMD_FLAG_ENABLE,
    "ETRN", psc_etrn_cmd, PSC_SMTPD_CMD_FLAG_ENABLE,
    "QUIT", psc_quit_cmd, PSC_SMTPD_CMD_FLAG_ENABLE | PSC_SMTPD_CMD_FLAG_DESTROY | PSC_SMTPD_CMD_FLAG_PRE_TLS,
    0,
};

/* psc_smtpd_read_event - pseudo responder */

static void psc_smtpd_read_event(int event, char *context)
{
    const char *myname = "psc_smtpd_read_event";
    PSC_STATE *state = (PSC_STATE *) context;
    int     ch;
    struct cmd_trans {
	int     state;
	int     want;
	int     next_state;
    };

#define PSC_SMTPD_CMD_ST_ANY		0
#define PSC_SMTPD_CMD_ST_CR		1
#define PSC_SMTPD_CMD_ST_CR_LF		2

    static const struct cmd_trans cmd_trans[] = {
	PSC_SMTPD_CMD_ST_ANY, '\r', PSC_SMTPD_CMD_ST_CR,
	PSC_SMTPD_CMD_ST_CR, '\n', PSC_SMTPD_CMD_ST_CR_LF,
	0, 0, 0,
    };
    const struct cmd_trans *transp;
    char   *cmd_buffer_ptr;
    char   *command;
    const PSC_SMTPD_COMMAND *cmdp;
    int     write_stat;

    if (msg_verbose > 1)
	msg_info("%s: sq=%d cq=%d event %d on smtp socket %d from [%s]:%s flags=%s",
		 myname, psc_post_queue_length, psc_check_queue_length,
		 event, vstream_fileno(state->smtp_client_stream),
		 state->smtp_client_addr, state->smtp_client_port,
		 psc_print_state_flags(state->flags, myname));

    /*
     * Basic liveness requirements.
     * 
     * Drain all input in the VSTREAM buffer, otherwise this socket will not
     * receive further read event notification until the client disconnects!
     * 
     * To suspend this loop temporarily before the buffer is drained, use the
     * PSC_SUSPEND_SMTP_CMD_EVENTS() and PSC_RESUME_SMTP_CMD_EVENTS() macros,
     * and set the PSC_SMTPD_CMD_FLAG_SUSPEND flag in the command table.
     * 
     * Don't try to read input before it has arrived, otherwise we would starve
     * the pseudo threads of other sessions. Get out of here as soon as the
     * VSTREAM read buffer dries up. Do not look for more input in kernel
     * buffers. That input wasn't likely there when psc_smtpd_read_event()
     * was called. Also, yielding the pseudo thread will improve fairness for
     * other pseudo threads.
     */

    /*
     * Note: on entry into this function the VSTREAM buffer may or may not be
     * empty, so we test the "no more input" condition at the bottom of the
     * loops.
     */
    for (;;) {

	/*
	 * Read one command line, possibly one fragment at a time.
	 */
	for (;;) {

	    if ((ch = PSC_SMTPD_NEXT_CHAR(state)) == VSTREAM_EOF) {
		PSC_CLEAR_EVENT_HANGUP(state, psc_smtpd_time_event);
		return;
	    }

	    /*
	     * Sanity check. We don't want to store infinitely long commands.
	     */
	    if (state->read_state == PSC_SMTPD_CMD_ST_ANY
		&& VSTRING_LEN(state->cmd_buffer) >= var_line_limit) {
		msg_info("COMMAND LENGTH LIMIT from [%s]:%s",
			 PSC_CLIENT_ADDR_PORT(state));
		PSC_CLEAR_EVENT_DROP_SESSION_STATE(state, psc_smtpd_time_event,
						   psc_smtpd_421_reply);
		return;
	    }
	    VSTRING_ADDCH(state->cmd_buffer, ch);

	    /*
	     * Try to match the current character desired by the state
	     * machine. If that fails, try to restart the machine with a
	     * match for its first state. Like smtpd(8), we understand lines
	     * ending in <CR><LF> and bare <LF>. Unlike smtpd(8), we may
	     * treat lines ending in bare <LF> as an offense.
	     */
	    for (transp = cmd_trans; transp->state != state->read_state; transp++)
		if (transp->want == 0)
		    msg_panic("%s: command_read: unknown state: %d",
			      myname, state->read_state);
	    if (ch == transp->want)
		state->read_state = transp->next_state;
	    else if (ch == cmd_trans[0].want)
		state->read_state = cmd_trans[0].next_state;
	    else
		state->read_state = PSC_SMTPD_CMD_ST_ANY;
	    if (state->read_state == PSC_SMTPD_CMD_ST_CR_LF) {
		vstring_truncate(state->cmd_buffer,
				 VSTRING_LEN(state->cmd_buffer) - 2);
		break;
	    }

	    /*
	     * Bare newline test.
	     */
	    if (ch == '\n') {
		if ((state->flags & PSC_STATE_MASK_BARLF_TODO_SKIP)
		    == PSC_STATE_FLAG_BARLF_TODO) {
		    msg_info("BARE NEWLINE from [%s]:%s",
			     PSC_CLIENT_ADDR_PORT(state));
		    PSC_FAIL_SESSION_STATE(state, PSC_STATE_FLAG_BARLF_FAIL);
		    PSC_UNPASS_SESSION_STATE(state, PSC_STATE_FLAG_BARLF_PASS);
		    state->barlf_stamp = PSC_TIME_STAMP_DISABLED;	/* XXX */
		    /* Skip this test for the remainder of this session. */
		    PSC_SKIP_SESSION_STATE(state, "bare newline test",
					   PSC_STATE_FLAG_BARLF_SKIP);
		    switch (psc_barlf_action) {
		    case PSC_ACT_DROP:
			PSC_CLEAR_EVENT_DROP_SESSION_STATE(state,
						       psc_smtpd_time_event,
					    "521 5.5.1 Protocol error\r\n");
			return;
		    case PSC_ACT_ENFORCE:
			PSC_ENFORCE_SESSION_STATE(state,
					    "550 5.5.1 Protocol error\r\n");
			break;
		    case PSC_ACT_IGNORE:
			PSC_UNFAIL_SESSION_STATE(state,
						 PSC_STATE_FLAG_BARLF_FAIL);
			/* Temporarily whitelist until something expires. */
			PSC_PASS_SESSION_STATE(state, "bare newline test",
					       PSC_STATE_FLAG_BARLF_PASS);
			state->barlf_stamp = event_time() + psc_min_ttl;
			break;
		    default:
			msg_panic("%s: unknown bare_newline action value %d",
				  myname, psc_barlf_action);
		    }
		}
		vstring_truncate(state->cmd_buffer,
				 VSTRING_LEN(state->cmd_buffer) - 1);
		break;
	    }

	    /*
	     * Yield this pseudo thread when the VSTREAM buffer is empty in
	     * the middle of a command.
	     * 
	     * XXX Do not reset the read timeout. The entire command must be
	     * received within the time limit.
	     */
	    if (PSC_SMTPD_BUFFER_EMPTY(state))
		return;
	}

	/*
	 * Terminate the command buffer, and apply the last-resort command
	 * editing workaround.
	 */
	VSTRING_TERMINATE(state->cmd_buffer);
	if (psc_cmd_filter != 0) {
	    const char *cp;

	    for (cp = STR(state->cmd_buffer); *cp && IS_SPACE_TAB(*cp); cp++)
		 /* void */ ;
	    if ((cp = psc_dict_get(psc_cmd_filter, cp)) != 0) {
		msg_info("[%s]:%s: replacing command \"%.100s\" with \"%.100s\"",
			 state->smtp_client_addr, state->smtp_client_port,
			 STR(state->cmd_buffer), cp);
		vstring_strcpy(state->cmd_buffer, cp);
	    } else if (psc_cmd_filter->error != 0) {
		msg_fatal("%s:%s lookup error for \"%.100s\"",
			  psc_cmd_filter->type, psc_cmd_filter->name, cp);
	    }
	}

	/*
	 * Reset the command buffer write pointer and state machine in
	 * preparation for the next command. For this to work as expected,
	 * VSTRING_RESET() must be non-destructive. We just can't ask for the
	 * VSTRING_LEN() and vstring_end() results.
	 */
	state->read_state = PSC_SMTPD_CMD_ST_ANY;
	VSTRING_RESET(state->cmd_buffer);

	/*
	 * Process the command line.
	 * 
	 * Caution: some command handlers terminate the session and destroy the
	 * session state structure. When this happens we must leave the SMTP
	 * engine to avoid a dangling pointer problem.
	 */
	cmd_buffer_ptr = STR(state->cmd_buffer);
	if (msg_verbose)
	    msg_info("< [%s]:%s: %s", state->smtp_client_addr,
		     state->smtp_client_port, cmd_buffer_ptr);

	/* Parse the command name. */
	if ((command = PSC_SMTPD_NEXT_TOKEN(cmd_buffer_ptr)) == 0)
	    command = "";

	/*
	 * The non-SMTP, PIPELINING and command COUNT tests depend on the
	 * client command handler.
	 * 
	 * Caution: cmdp->name and cmdp->action may be null on loop exit.
	 */
	for (cmdp = command_table; cmdp->name != 0; cmdp++)
	    if (strcasecmp(command, cmdp->name) == 0)
		break;

	if ((state->flags & PSC_STATE_FLAG_SMTPD_X21)
	    && cmdp->action != psc_quit_cmd) {
	    PSC_CLEAR_EVENT_DROP_SESSION_STATE(state, psc_smtpd_time_event,
					       state->final_reply);
	    return;
	}
	/* Non-SMTP command test. */
	if ((state->flags & PSC_STATE_MASK_NSMTP_TODO_SKIP)
	    == PSC_STATE_FLAG_NSMTP_TODO && cmdp->name == 0
	    && (is_header(command)
	/* Ignore forbid_cmds lookup errors. Non-critical feature. */
		|| (*var_psc_forbid_cmds
		    && string_list_match(psc_forbid_cmds, command)))) {
	    printable(command, '?');
	    msg_info("NON-SMTP COMMAND from [%s]:%s %.100s %.100s",
		     PSC_CLIENT_ADDR_PORT(state), command, cmd_buffer_ptr);
	    PSC_FAIL_SESSION_STATE(state, PSC_STATE_FLAG_NSMTP_FAIL);
	    PSC_UNPASS_SESSION_STATE(state, PSC_STATE_FLAG_NSMTP_PASS);
	    state->nsmtp_stamp = PSC_TIME_STAMP_DISABLED;	/* XXX */
	    /* Skip this test for the remainder of this SMTP session. */
	    PSC_SKIP_SESSION_STATE(state, "non-smtp test",
				   PSC_STATE_FLAG_NSMTP_SKIP);
	    switch (psc_nsmtp_action) {
	    case PSC_ACT_DROP:
		PSC_CLEAR_EVENT_DROP_SESSION_STATE(state,
						   psc_smtpd_time_event,
		   "521 5.7.0 Error: I can break rules, too. Goodbye.\r\n");
		return;
	    case PSC_ACT_ENFORCE:
		PSC_ENFORCE_SESSION_STATE(state,
					  "550 5.5.1 Protocol error\r\n");
		break;
	    case PSC_ACT_IGNORE:
		PSC_UNFAIL_SESSION_STATE(state,
					 PSC_STATE_FLAG_NSMTP_FAIL);
		/* Temporarily whitelist until something else expires. */
		PSC_PASS_SESSION_STATE(state, "non-smtp test",
				       PSC_STATE_FLAG_NSMTP_PASS);
		state->nsmtp_stamp = event_time() + psc_min_ttl;
		break;
	    default:
		msg_panic("%s: unknown non_smtp_command action value %d",
			  myname, psc_nsmtp_action);
	    }
	}
	/* Command PIPELINING test. */
	if ((state->flags & PSC_STATE_MASK_PIPEL_TODO_SKIP)
	    == PSC_STATE_FLAG_PIPEL_TODO && !PSC_SMTPD_BUFFER_EMPTY(state)) {
	    printable(command, '?');
	    escape(psc_temp, PSC_SMTPD_PEEK_DATA(state),
		   PSC_SMTPD_PEEK_LEN(state) < 100 ?
		   PSC_SMTPD_PEEK_LEN(state) : 100);
	    msg_info("COMMAND PIPELINING from [%s]:%s after %.100s: %s",
		     PSC_CLIENT_ADDR_PORT(state), command, STR(psc_temp));
	    PSC_FAIL_SESSION_STATE(state, PSC_STATE_FLAG_PIPEL_FAIL);
	    PSC_UNPASS_SESSION_STATE(state, PSC_STATE_FLAG_PIPEL_PASS);
	    state->pipel_stamp = PSC_TIME_STAMP_DISABLED;	/* XXX */
	    /* Skip this test for the remainder of this SMTP session. */
	    PSC_SKIP_SESSION_STATE(state, "pipelining test",
				   PSC_STATE_FLAG_PIPEL_SKIP);
	    switch (psc_pipel_action) {
	    case PSC_ACT_DROP:
		PSC_CLEAR_EVENT_DROP_SESSION_STATE(state,
						   psc_smtpd_time_event,
					    "521 5.5.1 Protocol error\r\n");
		return;
	    case PSC_ACT_ENFORCE:
		PSC_ENFORCE_SESSION_STATE(state,
					  "550 5.5.1 Protocol error\r\n");
		break;
	    case PSC_ACT_IGNORE:
		PSC_UNFAIL_SESSION_STATE(state,
					 PSC_STATE_FLAG_PIPEL_FAIL);
		/* Temporarily whitelist until something else expires. */
		PSC_PASS_SESSION_STATE(state, "pipelining test",
				       PSC_STATE_FLAG_PIPEL_PASS);
		state->pipel_stamp = event_time() + psc_min_ttl;
		break;
	    default:
		msg_panic("%s: unknown pipelining action value %d",
			  myname, psc_pipel_action);
	    }
	}

	/*
	 * The following tests don't pass until the client gets all the way
	 * to the RCPT TO command. However, the client can still fail these
	 * tests with some later command.
	 */
	if (cmdp->action == psc_rcpt_cmd) {
	    if ((state->flags & PSC_STATE_MASK_BARLF_TODO_PASS_FAIL)
		== PSC_STATE_FLAG_BARLF_TODO) {
		PSC_PASS_SESSION_STATE(state, "bare newline test",
				       PSC_STATE_FLAG_BARLF_PASS);
		/* XXX Reset to PSC_TIME_STAMP_DISABLED on failure. */
		state->barlf_stamp = event_time() + var_psc_barlf_ttl;
	    }
	    if ((state->flags & PSC_STATE_MASK_NSMTP_TODO_PASS_FAIL)
		== PSC_STATE_FLAG_NSMTP_TODO) {
		PSC_PASS_SESSION_STATE(state, "non-smtp test",
				       PSC_STATE_FLAG_NSMTP_PASS);
		/* XXX Reset to PSC_TIME_STAMP_DISABLED on failure. */
		state->nsmtp_stamp = event_time() + var_psc_nsmtp_ttl;
	    }
	    if ((state->flags & PSC_STATE_MASK_PIPEL_TODO_PASS_FAIL)
		== PSC_STATE_FLAG_PIPEL_TODO) {
		PSC_PASS_SESSION_STATE(state, "pipelining test",
				       PSC_STATE_FLAG_PIPEL_PASS);
		/* XXX Reset to PSC_TIME_STAMP_DISABLED on failure. */
		state->pipel_stamp = event_time() + var_psc_pipel_ttl;
	    }
	}
	/* Command COUNT limit test. */
	if (++state->command_count > var_psc_cmd_count
	    && cmdp->action != psc_quit_cmd) {
	    msg_info("COMMAND COUNT LIMIT from [%s]:%s",
		     PSC_CLIENT_ADDR_PORT(state));
	    PSC_CLEAR_EVENT_DROP_SESSION_STATE(state, psc_smtpd_time_event,
					       psc_smtpd_421_reply);
	    return;
	}
	/* Finally, execute the command. */
	if (cmdp->name == 0 || (cmdp->flags & PSC_SMTPD_CMD_FLAG_ENABLE) == 0) {
	    write_stat = PSC_SEND_REPLY(state,
			     "502 5.5.2 Error: command not recognized\r\n");
	} else if (var_psc_enforce_tls
		   && (state->flags & PSC_STATE_FLAG_USING_TLS) == 0
		   && (cmdp->flags & PSC_SMTPD_CMD_FLAG_PRE_TLS) == 0) {
	    write_stat = PSC_SEND_REPLY(state,
		       "530 5.7.0 Must issue a STARTTLS command first\r\n");
	} else {
	    write_stat = cmdp->action(state, cmd_buffer_ptr);
	    if (cmdp->flags & PSC_SMTPD_CMD_FLAG_DESTROY)
		return;
	}

	/*
	 * Terminate the session after a write error.
	 */
	if (write_stat < 0) {
	    PSC_CLEAR_EVENT_HANGUP(state, psc_smtpd_time_event);
	    return;
	}

	/*
	 * We're suspended, waiting for some external event to happen.
	 * Hopefully, someone will call us back to process the remainder of
	 * the pending input, otherwise we could hang.
	 */
	if (cmdp->flags & PSC_SMTPD_CMD_FLAG_SUSPEND)
	    return;

	/*
	 * Reset the command read timeout before reading the next command.
	 */
	event_request_timer(psc_smtpd_time_event, (char *) state,
			    PSC_EFF_CMD_TIME_LIMIT);

	/*
	 * Yield this pseudo thread when the VSTREAM buffer is empty.
	 */
	if (PSC_SMTPD_BUFFER_EMPTY(state))
	    return;
    }
}

/* psc_smtpd_tests - per-session deep protocol test initialization */

void    psc_smtpd_tests(PSC_STATE *state)
{
    static char *myname = "psc_smtpd_tests";

    /*
     * Report errors and progress in the context of this test.
     */
    PSC_BEGIN_TESTS(state, "tests after SMTP handshake");

    /*
     * Initialize per-session state that is used only by the dummy engine:
     * the command read buffer and the command read state machine.
     */
    state->cmd_buffer = vstring_alloc(100);
    state->read_state = PSC_SMTPD_CMD_ST_ANY;

    /*
     * Opportunistically make postscreen more useful by turning on the
     * pipelining and non-SMTP command tests when a pre-handshake test
     * failed, or when some deep test is configured as enabled.
     * 
     * XXX Make "opportunistically" configurable for each test.
     */
    if ((state->flags & PSC_STATE_FLAG_SMTPD_X21) == 0) {
	state->flags |= PSC_STATE_MASK_SMTPD_TODO;
    } else {
	state->flags &= ~PSC_STATE_MASK_SMTPD_TODO;
    }

    /*
     * Send no SMTP banner to pregreeting clients. This eliminates a lot of
     * "NON-SMTP COMMAND" events, and improves sender/recipient logging.
     */
    if ((state->flags & PSC_STATE_FLAG_PREGR_FAIL) == 0
	&& PSC_SEND_REPLY(state, psc_smtpd_greeting) != 0) {
	psc_hangup_event(state);
	return;
    }

    /*
     * Wait for the client to respond.
     */
    PSC_READ_EVENT_REQUEST2(vstream_fileno(state->smtp_client_stream),
			    psc_smtpd_read_event, psc_smtpd_time_event,
			    (char *) state, PSC_EFF_CMD_TIME_LIMIT);
}

/* psc_smtpd_init - per-process deep protocol test initialization */

void    psc_smtpd_init(void)
{

    /*
     * Initialize the server banner.
     */
    vstring_sprintf(psc_temp, "220 %s\r\n", var_smtpd_banner);
    psc_smtpd_greeting = mystrdup(STR(psc_temp));

    /*
     * Initialize the HELO reply.
     */
    vstring_sprintf(psc_temp, "250 %s\r\n", var_myhostname);
    psc_smtpd_helo_reply = mystrdup(STR(psc_temp));

    /*
     * STARTTLS support. Note the complete absence of #ifdef USE_TLS
     * throughout the postscreen(8) source code. If Postfix is built without
     * TLS support, then the TLS proxy will simply report that TLS is not
     * available, and conventional error handling will take care of the
     * issue.
     * 
     * Legacy code copied from smtpd(8). The pre-fabricated EHLO reply depends
     * on this.
     */
    if (*var_psc_tls_level) {
	switch (tls_level_lookup(var_psc_tls_level)) {
	default:
	    msg_fatal("Invalid TLS level \"%s\"", var_psc_tls_level);
	    /* NOTREACHED */
	    break;
	case TLS_LEV_SECURE:
	case TLS_LEV_VERIFY:
	case TLS_LEV_FPRINT:
	    msg_warn("%s: unsupported TLS level \"%s\", using \"encrypt\"",
		     VAR_PSC_TLS_LEVEL, var_psc_tls_level);
	    /* FALLTHROUGH */
	case TLS_LEV_ENCRYPT:
	    var_psc_enforce_tls = var_psc_use_tls = 1;
	    break;
	case TLS_LEV_MAY:
	    var_psc_enforce_tls = 0;
	    var_psc_use_tls = 1;
	    break;
	case TLS_LEV_NONE:
	    var_psc_enforce_tls = var_psc_use_tls = 0;
	    break;
	}
    }
    var_psc_use_tls = var_psc_use_tls || var_psc_enforce_tls;
#ifdef TODO_SASL_AUTH
    var_psc_tls_auth_only = var_psc_tls_auth_only || var_psc_enforce_tls;
#endif

    /*
     * Initialize the EHLO reply. Once for plaintext sessions, and once for
     * TLS sessions.
     */
    psc_smtpd_format_ehlo_reply(psc_temp, psc_ehlo_discard_mask);
    psc_smtpd_ehlo_reply_plain = mystrdup(STR(psc_temp));

    psc_smtpd_format_ehlo_reply(psc_temp,
				psc_ehlo_discard_mask | EHLO_MASK_STARTTLS);
    psc_smtpd_ehlo_reply_tls = mystrdup(STR(psc_temp));

    /*
     * Initialize the 421 timeout reply.
     */
    vstring_sprintf(psc_temp, "421 4.4.2 %s Error: timeout exceeded\r\n",
		    var_myhostname);
    psc_smtpd_timeout_reply = mystrdup(STR(psc_temp));

    /*
     * Initialize the generic 421 reply.
     */
    vstring_sprintf(psc_temp, "421 %s Service unavailable - try again later\r\n",
		    var_myhostname);
    psc_smtpd_421_reply = mystrdup(STR(psc_temp));

    /*
     * Initialize the reply footer.
     */
    if (*var_psc_rej_footer)
	psc_expand_init();
}

/* psc_smtpd_pre_jail_init - per-process deep protocol test initialization */

void    psc_smtpd_pre_jail_init(void)
{

    /*
     * Determine what server ESMTP features to suppress, typically to avoid
     * inter-operability problems. We do the default filter here, and
     * determine client-dependent filtering on the fly.
     * 
     * XXX Bugger. This means we have to restart when the table changes!
     */
    if (*var_psc_ehlo_dis_maps)
	psc_ehlo_discard_maps = maps_create(VAR_PSC_EHLO_DIS_MAPS,
					    var_psc_ehlo_dis_maps,
					    DICT_FLAG_LOCK);
    psc_ehlo_discard_mask = ehlo_mask(var_psc_ehlo_dis_words);

    /*
     * Last-resort command editing support.
     */
    if (*var_psc_cmd_filter)
	psc_cmd_filter = dict_open(var_psc_cmd_filter, O_RDONLY,
				   DICT_FLAG_LOCK | DICT_FLAG_FOLD_FIX);
}
