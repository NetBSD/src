/*	$NetBSD: milter.c,v 1.1.1.4 2007/08/02 08:05:18 heas Exp $	*/

/*++
/* NAME
/*	milter 3
/* SUMMARY
/*	generic MTA-side mail filter interface
/* SYNOPSIS
/*	#include <milter.h>
/*
/*	MILTERS	*milter_create(milter_names, conn_timeout, cmd_timeout,
/*					msg_timeout, protocol, def_action,
/*					conn_macros, helo_macros,
/*					mail_macros, rcpt_macros,
/*					data_macros, eod_macros,
/*					unk_macros)
/*	const char *milter_names;
/*	int	conn_timeout;
/*	int	cmd_timeout;
/*	int	msg_timeout;
/*	const char *protocol;
/*	const char *def_action;
/*	const char *conn_macros;
/*	const char *helo_macros;
/*	const char *mail_macros;
/*	const char *rcpt_macrps;
/*	const char *data_macros;
/*	const char *eod_macros;
/*	const char *unk_macros;
/*
/*	void	milter_free(milters)
/*	MILTERS	*milters;
/*
/*	void	milter_macro_callback(milters, mac_lookup, mac_context)
/*	const char *(*mac_lookup)(const char *name, void *context);
/*	void	*mac_context;
/*
/*	void	milter_edit_callback(milters, add_header, upd_header,
/*					ins_header, del_header, add_rcpt,
/*					del_rcpt, repl_body, context)
/*	MILTERS	*milters;
/*	const char *(*add_header) (void *context, char *name, char *value);
/*	const char *(*upd_header) (void *context, ssize_t index,
/*				char *name, char *value);
/*	const char *(*ins_header) (void *context, ssize_t index,
/*				char *name, char *value);
/*	const char *(*del_header) (void *context, ssize_t index, char *name);
/*	const char *(*add_rcpt) (void *context, char *rcpt);
/*	const char *(*del_rcpt) (void *context, char *rcpt);
/*	const char *(*repl_body) (void *context, VSTRING *body);
/*	void	*context;
/*
/*	const char *milter_conn_event(milters, client_name, client_addr,
/*					client_port, addr_family)
/*	MILTERS	*milters;
/*	const char *client_name;
/*	const char *client_addr;
/*	const char *client_port;
/*	int	addr_family;
/*
/*	const char *milter_disc_event(milters)
/*	MILTERS	*milters;
/*
/*	const char *milter_helo_event(milters, helo_name, esmtp_flag)
/*	MILTERS	*milters;
/*	const char *helo_name;
/*	int	esmtp_flag;
/*
/*	const char *milter_mail_event(milters, argv)
/*	MILTERS	*milters;
/*	const char **argv;
/*
/*	const char *milter_rcpt_event(milters, argv)
/*	MILTERS	*milters;
/*	const char **argv;
/*
/*	const char *milter_data_event(milters)
/*	MILTERS	*milters;
/*
/*	const char *milter_unknown_event(milters, command)
/*	MILTERS	*milters;
/*	const char *command;
/*
/*	const char *milter_other_event(milters)
/*	MILTERS	*milters;
/*
/*	const char *milter_message(milters, qfile, data_offset)
/*	MILTERS	*milters;
/*	VSTREAM *qfile;
/*	off_t	data_offset;
/*
/*	const char *milter_abort(milters)
/*	MILTERS	*milters;
/*
/*	int	milter_send(milters, fp)
/*	MILTERS	*milters;
/*	VSTREAM *fp;
/*
/*	MILTERS	*milter_receive(fp, count)
/*	VSTREAM	*fp;
/*	int	count;
/*
/*	int	milter_dummy(milters, fp)
/*	MILTERS	*milters;
/*	VSTREAM *fp;
/* DESCRIPTION
/*	The functions in this module manage one or more milter (mail
/*	filter) clients. Currently, only the Sendmail 8 filter
/*	protocol is supported.
/*
/*	The functions that inspect content or envelope commands
/*	return either an SMTP reply ([45]XX followed by enhanced
/*	status code and text), "D" (discard), "H" (quarantine), or
/*	a null pointer, which means "no news is good news".
/*
/*	milter_create() instantiates the milter clients specified
/*	with the milter_names argument.  The conn_macros etc.
/*	arguments specify the names of macros that are sent to the
/*	mail filter applications upon a connect etc. event. This
/*	function should be called during process initialization,
/*	before entering a chroot jail. The timeout parameters specify
/*	time limits for the completion of the specified request
/*	classes. The protocol parameter specifies a protocol version
/*	and optional extensions.  When the milter application is
/*	unavailable, the milter client will go into a suitable error
/*	state as specified with the def_action parameter (i.e.
/*	reject, tempfail or accept all subsequent events).
/*
/*	milter_free() disconnects from the milter instances that
/*	are still opened, and destroys the data structures created
/*	by milter_create(). This function is safe to call at any
/*	point after milter_create().
/*
/*	milter_macro_callback() specifies a call-back function and
/*	context for macro lookup. This function must be called
/*	before milter_conn_event().
/*
/*	milter_edit_callback() specifies call-back functions and
/*	context for editing the queue file after the end-of-data
/*	is received. This function must be called before milter_message();
/*
/*	milter_conn_event() reports an SMTP client connection event
/*	to the specified milter instances, after sending the macros
/*	specified with the milter_create() conn_macros argument.
/*	This function must be called before reporting any other
/*	events.
/*
/*	milter_disc_event() reports an SMTP client disconnection
/*	event to the specified milter instances. No events can
/*	reported after this call. To simplify usage, redundant calls
/*	of this function are NO-OPs and don't raise a run-time
/*	error.
/*
/*	milter_helo_event() reports a HELO or EHLO event to the
/*	specified milter instances, after sending the macros that
/*	were specified with the milter_create() helo_macros argument.
/*
/*	milter_mail_event() reports a MAIL FROM event to the specified
/*	milter instances, after sending the macros that were specified
/*	with the milter_create() mail_macros argument.
/*
/*	milter_rcpt_event() reports an RCPT TO event to the specified
/*	milter instances, after sending the macros that were specified
/*	with the milter_create() rcpt_macros argument.
/*
/*	milter_data_event() reports a DATA event to the specified
/*	milter instances, after sending the macros that were specified
/*	with the milter_create() data_macros argument.
/*
/*	milter_unknown_event() reports an unknown command event to
/*	the specified milter instances, after sending the macros
/*	that were specified with the milter_create() unk_macros
/*	argument.
/*
/*	milter_other_event() returns the current default mail filter
/*	reply for the current SMTP connection state; it does not
/*	change milter states. A null pointer result means that all
/*	is well. This function can be used for SMTP commands such
/*	as AUTH, STARTTLS that don't have their own milter event
/*	routine.
/*
/*	milter_message() sends the message header and body to the
/*	to the specified milter instances, and sends the macros
/*	specified with the milter_create() eod_macros argument at
/*	the end.  Each milter sees the result of any changes made
/*	by a preceding milter. This function must be called with
/*	as argument an open Postfix queue file.
/*
/*	milter_abort() cancels a mail transaction in progress.  To
/*	simplify usage, redundant calls of this function are NO-OPs
/*	and don't raise a run-time error.
/*
/*	milter_send() sends a list of mail filters over the specified
/*	stream. When given a null list pointer, a "no filter"
/*	indication is sent.  The result is non-zero in case of
/*	error.
/*
/*	milter_receive() receives the specified number of mail
/*	filters over the specified stream. The result is a null
/*	pointer when no milters were sent, or when an error happened.
/*
/*	milter_dummy() is like milter_send(), except that it sends
/*	a dummy, but entirely valid, mail filter list.
/* SEE ALSO
/*	milter8(3) Sendmail 8 Milter protocol
/* DIAGNOSTICS
/*	Panic: interface violation.
/*	Fatal errors: memory allocation problem.
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
#include <stringops.h>
#include <argv.h>
#include <attr.h>

/* Global library. */

#include <mail_proto.h>
#include <record.h>
#include <rec_type.h>

/* Postfix Milter library. */

#include <milter.h>

/* Application-specific. */

 /*
  * SLMs.
  */
#define STR(x)	vstring_str(x)

/* milter_macro_lookup - look up macros */

static ARGV *milter_macro_lookup(MILTERS *milters, const char *macro_names)
{
    const char *myname = "milter_macro_lookup";
    char   *saved_names = mystrdup(macro_names);
    char   *cp = saved_names;
    ARGV   *argv = argv_alloc(10);
    const char *value;
    const char *name;

    while ((name = mystrtok(&cp, ", \t\r\n")) != 0) {
	if (msg_verbose)
	    msg_info("%s: \"%s\"", myname, name);
	if ((value = milters->mac_lookup(name, milters->mac_context)) != 0) {
	    if (msg_verbose)
		msg_info("%s: result \"%s\"", myname, value);
	    argv_add(argv, name, value, (char *) 0);
	}
    }
    myfree(saved_names);
    return (argv);
}

/* milter_macro_callback - specify macro lookup */

void    milter_macro_callback(MILTERS *milters,
		           const char *(*mac_lookup) (const char *, void *),
			              void *mac_context)
{
    milters->mac_lookup = mac_lookup;
    milters->mac_context = mac_context;
}

/* milter_edit_callback - specify queue file edit call-back information */

void    milter_edit_callback(MILTERS *milters,
		         const char *(*add_header) (void *, char *, char *),
	        const char *(*upd_header) (void *, ssize_t, char *, char *),
	        const char *(*ins_header) (void *, ssize_t, char *, char *),
		        const char *(*del_header) (void *, ssize_t, char *),
			           const char *(*add_rcpt) (void *, char *),
			           const char *(*del_rcpt) (void *, char *),
		          const char *(*repl_body) (void *, int, VSTRING *),
			             void *chg_context)
{
    milters->add_header = add_header;
    milters->upd_header = upd_header;
    milters->ins_header = ins_header;
    milters->del_header = del_header;
    milters->add_rcpt = add_rcpt;
    milters->del_rcpt = del_rcpt;
    milters->repl_body = repl_body;
    milters->chg_context = chg_context;
}

/* milter_conn_event - report connect event */

const char *milter_conn_event(MILTERS *milters,
			              const char *client_name,
			              const char *client_addr,
			              const char *client_port,
			              unsigned addr_family)
{
    const char *resp;
    MILTER *m;
    ARGV   *macros;

    if (msg_verbose)
	msg_info("report connect to all milters");
    macros = milter_macro_lookup(milters, milters->conn_macros);
    for (resp = 0, m = milters->milter_list; resp == 0 && m != 0; m = m->next)
	resp = m->conn_event(m, client_name, client_addr, client_port,
			     addr_family, macros);
    argv_free(macros);
    return (resp);
}

/* milter_helo_event - report helo event */

const char *milter_helo_event(MILTERS *milters, const char *helo_name,
			              int esmtp_flag)
{
    const char *resp;
    MILTER *m;
    ARGV   *macros;

    if (msg_verbose)
	msg_info("report helo to all milters");
    macros = milters->helo_macros == 0 ? 0 :
	milter_macro_lookup(milters, milters->helo_macros);
    for (resp = 0, m = milters->milter_list; resp == 0 && m != 0; m = m->next)
	resp = m->helo_event(m, helo_name, esmtp_flag, macros);
    if (macros)
	argv_free(macros);
    return (resp);
}

/* milter_mail_event - report mail from event */

const char *milter_mail_event(MILTERS *milters, const char **argv)
{
    const char *resp;
    MILTER *m;
    ARGV   *macros;

    if (msg_verbose)
	msg_info("report sender to all milters");
    macros = milters->mail_macros == 0 ? 0 :
	milter_macro_lookup(milters, milters->mail_macros);
    for (resp = 0, m = milters->milter_list; resp == 0 && m != 0; m = m->next)
	resp = m->mail_event(m, argv, macros);
    if (macros)
	argv_free(macros);
    return (resp);
}

/* milter_rcpt_event - report rcpt to event */

const char *milter_rcpt_event(MILTERS *milters, const char **argv)
{
    const char *resp;
    MILTER *m;
    ARGV   *macros;

    if (msg_verbose)
	msg_info("report recipient to all milters");
    macros = milters->rcpt_macros == 0 ? 0 :
	milter_macro_lookup(milters, milters->rcpt_macros);
    for (resp = 0, m = milters->milter_list; resp == 0 && m != 0; m = m->next)
	resp = m->rcpt_event(m, argv, macros);
    if (macros)
	argv_free(macros);
    return (resp);
}

/* milter_data_event - report data event */

const char *milter_data_event(MILTERS *milters)
{
    const char *myname = "milter_data_event";
    const char *resp;
    MILTER *m;
    ARGV   *macros = 0;

    if (msg_verbose)
	msg_info("report data to all milters");
    for (resp = 0, m = milters->milter_list; resp == 0 && m != 0; m = m->next) {
	if (m->data_event) {
	    if (macros == 0 && milters->data_macros)
		macros = milter_macro_lookup(milters, milters->data_macros);
	    resp = m->data_event(m, macros);
	} else {
	    if (msg_verbose)
		msg_info("%s: skip milter %s (command unimplemented)",
			 myname, m->name);
	}
    }
    if (macros)
	argv_free(macros);
    return (resp);
}

/* milter_unknown_event - report unknown command */

const char *milter_unknown_event(MILTERS *milters, const char *command)
{
    const char *myname = "milter_unknown_event";
    const char *resp;
    MILTER *m;
    ARGV   *macros = 0;

    if (msg_verbose)
	msg_info("report unknown command to all milters");
    for (resp = 0, m = milters->milter_list; resp == 0 && m != 0; m = m->next) {
	if (m->unknown_event) {
	    if (macros == 0 && milters->unk_macros)
		macros = milter_macro_lookup(milters, milters->unk_macros);
	    resp = m->unknown_event(m, command, macros);
	} else {
	    if (msg_verbose)
		msg_info("%s: skip milter %s (command unimplemented)",
			 myname, m->name);
	}
    }
    if (macros)
	argv_free(macros);
    return (resp);
}

/* milter_other_event - other SMTP event */

const char *milter_other_event(MILTERS *milters)
{
    const char *resp;
    MILTER *m;

    if (msg_verbose)
	msg_info("query milter states for other event");
    for (resp = 0, m = milters->milter_list; resp == 0 && m != 0; m = m->next)
	resp = m->other_event(m);
    return (resp);
}

/* milter_message - inspect message content */

const char *milter_message(MILTERS *milters, VSTREAM *fp, off_t data_offset)
{
    const char *resp;
    MILTER *m;
    ARGV   *macros;

    if (msg_verbose)
	msg_info("inspect content by all milters");
    macros = milters->eod_macros == 0 ? 0 :
	milter_macro_lookup(milters, milters->eod_macros);
    for (resp = 0, m = milters->milter_list; resp == 0 && m != 0; m = m->next)
	resp = m->message(m, fp, data_offset, macros);
    if (macros)
	argv_free(macros);
    return (resp);
}

/* milter_abort - cancel message receiving state, all milters */

void    milter_abort(MILTERS *milters)
{
    MILTER *m;

    if (msg_verbose)
	msg_info("abort all milters");
    for (m = milters->milter_list; m != 0; m = m->next)
	m->abort(m);
}

/* milter_disc_event - report client disconnect event to all milters */

void    milter_disc_event(MILTERS *milters)
{
    MILTER *m;

    if (msg_verbose)
	msg_info("disconnect event to all milters");
    for (m = milters->milter_list; m != 0; m = m->next)
	m->disc_event(m);
}

/* milter_create - create milter list */

MILTERS *milter_create(const char *names,
		               int conn_timeout,
		               int cmd_timeout,
		               int msg_timeout,
		               const char *protocol,
		               const char *def_action,
		               const char *conn_macros,
		               const char *helo_macros,
		               const char *mail_macros,
		               const char *rcpt_macros,
		               const char *data_macros,
		               const char *eod_macros,
		               const char *unk_macros)
{
    MILTERS *milters;
    MILTER *head = 0;
    MILTER *tail = 0;
    char   *name;
    MILTER *milter;
    const char *sep = ", \t\r\n";

    /*
     * Parse the milter list.
     */
    milters = (MILTERS *) mymalloc(sizeof(*milters));
    if (names != 0) {
	char   *saved_names = mystrdup(names);
	char   *cp = saved_names;

	while ((name = mystrtok(&cp, sep)) != 0) {
	    milter = milter8_create(name, conn_timeout, cmd_timeout,
				    msg_timeout, protocol, def_action,
				    milters);
	    if (head == 0) {
		head = milter;
	    } else {
		tail->next = milter;
	    }
	    tail = milter;
	}
	myfree(saved_names);
    }
    milters->milter_list = head;
    milters->mac_lookup = 0;
    milters->mac_context = 0;
    milters->conn_macros = mystrdup(conn_macros);
    milters->helo_macros = mystrdup(helo_macros);
    milters->mail_macros = mystrdup(mail_macros);
    milters->rcpt_macros = mystrdup(rcpt_macros);
    milters->data_macros = mystrdup(data_macros);
    milters->eod_macros = mystrdup(eod_macros);
    milters->unk_macros = mystrdup(unk_macros);
    milters->add_header = 0;
    milters->upd_header = milters->ins_header = 0;
    milters->del_header = 0;
    milters->add_rcpt = milters->del_rcpt = 0;
    milters->chg_context = 0;
    return (milters);
}

/* milter_free - destroy all milters */

void    milter_free(MILTERS *milters)
{
    MILTER *m;
    MILTER *next;

    if (msg_verbose)
	msg_info("free all milters");
    for (m = milters->milter_list; m != 0; m = next)
	next = m->next, m->free(m);
    if (milters->conn_macros)
	myfree(milters->conn_macros);
    if (milters->helo_macros)
	myfree(milters->helo_macros);
    if (milters->mail_macros)
	myfree(milters->mail_macros);
    if (milters->rcpt_macros)
	myfree(milters->rcpt_macros);
    if (milters->rcpt_macros)
	myfree(milters->data_macros);
    if (milters->eod_macros)
	myfree(milters->eod_macros);
    if (milters->unk_macros)
	myfree(milters->unk_macros);
    myfree((char *) milters);
}

 /*
  * Temporary protocol to send /receive milter instances. This needs to be
  * extended with type information when we support both Sendmail8 and
  * Sendmail X protocols.
  */
#define MAIL_ATTR_MILT_CONN	"conn_macros"
#define MAIL_ATTR_MILT_HELO	"helo_macros"
#define MAIL_ATTR_MILT_MAIL	"mail_macros"
#define MAIL_ATTR_MILT_RCPT	"rcpt_macros"
#define MAIL_ATTR_MILT_DATA	"data_macros"
#define MAIL_ATTR_MILT_EOD	"eod_macros"
#define MAIL_ATTR_MILT_UNK	"unk_macros"

/* milter_dummy - send empty milter list */

int     milter_dummy(MILTERS *milters, VSTREAM *stream)
{
    MILTERS dummy = *milters;

    dummy.milter_list = 0;
    return (milter_send(&dummy, stream));
}

/* milter_send - send Milter instances over stream */

int     milter_send(MILTERS *milters, VSTREAM *stream)
{
    MILTER *m;
    int     status = 0;
    int     count = 0;

    /*
     * XXX Optimization: send only the filters that are actually used in the
     * remote process. No point sending a filter that looks at HELO commands
     * to a cleanup server. For now we skip only the filters that are known
     * to be disabled (either in global error state or in global accept
     * state).
     */
    if (milters != 0)
	for (m = milters->milter_list; m != 0; m = m->next)
	    if (m->active(m))
		count++;
    (void) rec_fprintf(stream, REC_TYPE_MILT_COUNT, "%d", count);

    /*
     * Send the filter macro names.
     */
    (void) attr_print(stream, ATTR_FLAG_MORE,
		   ATTR_TYPE_STR, MAIL_ATTR_MILT_CONN, milters->conn_macros,
		   ATTR_TYPE_STR, MAIL_ATTR_MILT_HELO, milters->helo_macros,
		   ATTR_TYPE_STR, MAIL_ATTR_MILT_MAIL, milters->mail_macros,
		   ATTR_TYPE_STR, MAIL_ATTR_MILT_RCPT, milters->rcpt_macros,
		   ATTR_TYPE_STR, MAIL_ATTR_MILT_DATA, milters->data_macros,
		      ATTR_TYPE_STR, MAIL_ATTR_MILT_EOD, milters->eod_macros,
		      ATTR_TYPE_STR, MAIL_ATTR_MILT_UNK, milters->unk_macros,
		      ATTR_TYPE_END);

    /*
     * Send the filter instances.
     */
    for (m = milters->milter_list; m != 0; m = m->next)
	if (m->active(m) && (status = m->send(m, stream)) != 0)
	    break;
    if (status != 0
	|| attr_scan(stream, ATTR_FLAG_STRICT,
		     ATTR_TYPE_INT, MAIL_ATTR_STATUS, &status,
		     ATTR_TYPE_END) != 1
	|| status != 0) {
	msg_warn("cannot send milters to service %s", VSTREAM_PATH(stream));
	return (-1);
    }
    return (0);
}

/* milter_receive - receive milters from stream */

MILTERS *milter_receive(VSTREAM *stream, int count)
{
    MILTERS *milters;
    MILTER *head = 0;
    MILTER *tail = 0;
    MILTER *milter = 0;
    VSTRING *conn_macros;
    VSTRING *helo_macros;
    VSTRING *mail_macros;
    VSTRING *rcpt_macros;
    VSTRING *data_macros;
    VSTRING *eod_macros;
    VSTRING *unk_macros;

    /*
     * Receive filter macros.
     */
#define FREE_BUFFERS() do { \
	vstring_free(conn_macros); vstring_free(helo_macros); \
	vstring_free(mail_macros); vstring_free(rcpt_macros); \
	vstring_free(data_macros); vstring_free(eod_macros); \
	vstring_free(unk_macros); \
   } while (0)

    conn_macros = vstring_alloc(10);
    helo_macros = vstring_alloc(10);
    mail_macros = vstring_alloc(10);
    rcpt_macros = vstring_alloc(10);
    data_macros = vstring_alloc(10);
    eod_macros = vstring_alloc(10);
    unk_macros = vstring_alloc(10);
    if (attr_scan(stream, ATTR_FLAG_STRICT | ATTR_FLAG_MORE,
		  ATTR_TYPE_STR, MAIL_ATTR_MILT_CONN, conn_macros,
		  ATTR_TYPE_STR, MAIL_ATTR_MILT_HELO, helo_macros,
		  ATTR_TYPE_STR, MAIL_ATTR_MILT_MAIL, mail_macros,
		  ATTR_TYPE_STR, MAIL_ATTR_MILT_RCPT, rcpt_macros,
		  ATTR_TYPE_STR, MAIL_ATTR_MILT_DATA, data_macros,
		  ATTR_TYPE_STR, MAIL_ATTR_MILT_EOD, eod_macros,
		  ATTR_TYPE_STR, MAIL_ATTR_MILT_UNK, unk_macros,
		  ATTR_TYPE_END) != 7) {
	FREE_BUFFERS();
	return (0);
    }
#define NO_MILTERS	((char *) 0)
#define NO_TIMEOUTS	0, 0, 0
#define NO_PROTOCOL	((char *) 0)
#define NO_ACTION	((char *) 0)

    milters = milter_create(NO_MILTERS, NO_TIMEOUTS, NO_PROTOCOL, NO_ACTION,
			    STR(conn_macros), STR(helo_macros),
			    STR(mail_macros), STR(rcpt_macros),
			    STR(data_macros), STR(eod_macros),
			    STR(unk_macros));
    FREE_BUFFERS();

    /*
     * Receive the filters.
     */
    for (; count > 0; count--) {
	if ((milter = milter8_receive(stream, milters)) == 0) {
	    msg_warn("cannot receive milters via service %s socket",
		     VSTREAM_PATH(stream));
	    milter_free(milters);
	    return (0);
	}
	if (head == 0) {
	    /* Coverity: milter_free() depends on milters->milter_list. */
	    milters->milter_list = head = milter;
	} else {
	    tail->next = milter;
	}
	tail = milter;
    }

    (void) attr_print(stream, ATTR_FLAG_NONE,
		      ATTR_TYPE_INT, MAIL_ATTR_STATUS, 0,
		      ATTR_TYPE_END);
    return (milters);
}

#ifdef TEST

 /*
  * Proof-of-concept test program. This can be used interactively, but is
  * typically used for automated regression tests from a script.
  */

/* System library. */

#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>

/* Utility library. */

#include "msg_vstream.h"
#include "vstring_vstream.h"

/* Global library. */

#include <mail_params.h>

int     var_milt_conn_time = 10;
int     var_milt_cmd_time = 10;
int     var_milt_msg_time = 100;
char   *var_milt_protocol = DEF_MILT_PROTOCOL;
char   *var_milt_def_action = DEF_MILT_DEF_ACTION;

static void usage(void)
{
    vstream_fprintf(VSTREAM_ERR, "usage: \n"
		    "	create names...		create and connect\n"
#if 0
		    "	conn_macros names...	define connect macros\n"
		    "	helo_macros names...	define helo command macros\n"
		    "	mail_macros names...	define mail command macros\n"
		    "	rcpt_macros names...	define rcpt command macros\n"
		    "	data_macros names...	define data command macros\n"
		    "	unk_macros names...	unknown command macros\n"
		    "	message_macros names...	define message macros\n"
#endif
		    "	free			disconnect and destroy\n"
		    "	connect name addr port family\n"
		    "	helo hostname\n"
		    "	ehlo hostname\n"
		    "	mail from sender...\n"
		    "	rcpt to recipient...\n"
		    "	data\n"
		    "	disconnect\n"
		    "	unknown command\n");
    vstream_fflush(VSTREAM_ERR);
}

int     main(int argc, char **argv)
{
    MILTERS *milters = 0;
    char   *conn_macros, *helo_macros, *mail_macros, *rcpt_macros;
    char   *data_macros, *eod_macros, *unk_macros;
    VSTRING *inbuf = vstring_alloc(100);
    char   *bufp;
    char   *cmd;
    int     ch;
    int     istty = isatty(vstream_fileno(VSTREAM_IN));

    conn_macros = helo_macros = mail_macros = rcpt_macros = data_macros
	= eod_macros = unk_macros = "";

    msg_vstream_init(argv[0], VSTREAM_ERR);
    while ((ch = GETOPT(argc, argv, "V:v")) > 0) {
	switch (ch) {
	default:
	    msg_fatal("usage: %s [-a action] [-p protocol] [-v]", argv[0]);
	case 'a':
	    var_milt_def_action = optarg;
	    break;
	case 'p':
	    var_milt_protocol = optarg;
	    break;
	case 'v':
	    msg_verbose++;
	    break;
	}
    }
    optind = OPTIND;

    for (;;) {
	const char *resp = 0;
	ARGV   *argv;
	char  **args;

	if (istty) {
	    vstream_printf("- ");
	    vstream_fflush(VSTREAM_OUT);
	}
	if (vstring_fgets_nonl(inbuf, VSTREAM_IN) <= 0)
	    break;
	bufp = vstring_str(inbuf);
	if (!istty) {
	    vstream_printf("> %s\n", bufp);
	    vstream_fflush(VSTREAM_OUT);
	}
	if (*bufp == '#')
	    continue;
	cmd = mystrtok(&bufp, " ");
	if (cmd == 0) {
	    usage();
	    continue;
	}
	argv = argv_split(bufp, " ");
	args = argv->argv;
	if (strcmp(cmd, "create") == 0 && argv->argc == 1) {
	    if (milters != 0) {
		msg_warn("deleting existing milters");
		milter_free(milters);
	    }
	    milters = milter_create(args[0], var_milt_conn_time,
				    var_milt_cmd_time, var_milt_msg_time,
				    var_milt_protocol, var_milt_def_action,
				    conn_macros, helo_macros, mail_macros,
				    rcpt_macros, data_macros, eod_macros,
				    unk_macros);
	} else if (strcmp(cmd, "free") == 0 && argv->argc == 0) {
	    if (milters == 0) {
		msg_warn("no milters");
		continue;
	    }
	    milter_free(milters);
	    milters = 0;
	} else if (strcmp(cmd, "connect") == 0 && argv->argc == 4) {
	    if (milters == 0) {
		msg_warn("no milters");
		continue;
	    }
	    resp = milter_conn_event(milters, args[0], args[1], args[2],
				 strcmp(args[3], "AF_INET") == 0 ? AF_INET :
			       strcmp(args[3], "AF_INET6") == 0 ? AF_INET6 :
				 strcmp(args[3], "AF_UNIX") == 0 ? AF_UNIX :
				     AF_UNSPEC);
	} else if (strcmp(cmd, "helo") == 0 && argv->argc == 1) {
	    if (milters == 0) {
		msg_warn("no milters");
		continue;
	    }
	    resp = milter_helo_event(milters, args[0], 0);
	} else if (strcmp(cmd, "ehlo") == 0 && argv->argc == 1) {
	    if (milters == 0) {
		msg_warn("no milters");
		continue;
	    }
	    resp = milter_helo_event(milters, args[0], 1);
	} else if (strcmp(cmd, "mail") == 0 && argv->argc > 0) {
	    if (milters == 0) {
		msg_warn("no milters");
		continue;
	    }
	    resp = milter_mail_event(milters, (const char **) args);
	} else if (strcmp(cmd, "rcpt") == 0 && argv->argc > 0) {
	    if (milters == 0) {
		msg_warn("no milters");
		continue;
	    }
	    resp = milter_rcpt_event(milters, (const char **) args);
	} else if (strcmp(cmd, "unknown") == 0 && argv->argc > 0) {
	    if (milters == 0) {
		msg_warn("no milters");
		continue;
	    }
	    resp = milter_unknown_event(milters, args[0]);
	} else if (strcmp(cmd, "data") == 0 && argv->argc == 0) {
	    if (milters == 0) {
		msg_warn("no milters");
		continue;
	    }
	    resp = milter_data_event(milters);
	} else if (strcmp(cmd, "disconnect") == 0 && argv->argc == 0) {
	    if (milters == 0) {
		msg_warn("no milters");
		continue;
	    }
	    milter_disc_event(milters);
	} else {
	    usage();
	}
	if (resp != 0)
	    msg_info("%s", resp);
	argv_free(argv);
    }
    if (milters != 0)
	milter_free(milters);
    vstring_free(inbuf);
    return (0);
}

#endif
