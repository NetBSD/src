/*	$NetBSD: milter.c,v 1.2 2017/02/14 01:16:45 christos Exp $	*/

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
/*					data_macros, eoh_macros,
/*					eod_macros, unk_macros,
/*					macro_deflts)
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
/*	const char *eoh_macros;
/*	const char *eod_macros;
/*	const char *unk_macros;
/*	const char *macro_deflts;
/*
/*	void	milter_free(milters)
/*	MILTERS	*milters;
/*
/*	void	milter_macro_callback(milters, mac_lookup, mac_context)
/*	const char *(*mac_lookup)(const char *name, void *context);
/*	void	*mac_context;
/*
/*	void	milter_edit_callback(milters, add_header, upd_header,
/*					ins_header, del_header, chg_from,
/*					add_rcpt, add_rcpt_par, del_rcpt,
/*					repl_body, context)
/*	MILTERS	*milters;
/*	MILTER_ADD_HEADER_FN add_header;
/*	MILTER_EDIT_HEADER_FN upd_header;
/*	MILTER_EDIT_HEADER_FN ins_header;
/*	MILTER_DEL_HEADER_FN del_header;
/*	MILTER_EDIT_FROM_FN chg_from;
/*	MILTER_EDIT_RCPT_FN add_rcpt;
/*	MILTER_EDIT_RCPT_PAR_FN add_rcpt_par;
/*	MILTER_EDIT_RCPT_FN del_rcpt;
/*	MILTER_EDIT_BODY_FN repl_body;
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
/*	const char *milter_rcpt_event(milters, flags, argv)
/*	MILTERS	*milters;
/*	int	flags;
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
/*	const char *milter_message(milters, qfile, data_offset, auto_hdrs)
/*	MILTERS	*milters;
/*	VSTREAM *qfile;
/*	off_t	data_offset;
/*	ARGV	*auto_hdrs;
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
/*	status code and text), "D" (discard), "H" (quarantine),
/*	"S" (shutdown connection), or a null pointer, which means
/*	"no news is good news".
/*
/*	milter_create() instantiates the milter clients specified
/*	with the milter_names argument.  The conn_macros etc.
/*	arguments specify the names of macros that are sent to the
/*	mail filter applications upon a connect etc. event, and the
/*	macro_deflts argument specifies macro defaults that will be used
/*	only if the application's lookup call-back returns null. This
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
/*	with the milter_create() rcpt_macros argument. The flags
/*	argument supports the following:
/* .IP MILTER_FLAG_WANT_RCPT_REJ
/*	When this flag is cleared, invoke all milters.  When this
/*	flag is set, invoke only milters that want to receive
/*	rejected recipients; with Sendmail V8 Milters, {rcpt_mailer}
/*	is set to "error", {rcpt_host} is set to an enhanced status
/*	code, and {rcpt_addr} is set to descriptive text.
/* .PP
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
/*	specified with the milter_create() eoh_macros after the
/*	message header, and with the eod_macros argument at
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
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

/* System library. */

#include <sys_defs.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <stringops.h>
#include <argv.h>
#include <attr.h>
#include <htable.h>

/* Global library. */

#include <mail_proto.h>
#include <record.h>
#include <rec_type.h>
#include <mail_params.h>
#include <attr_override.h>

/* Postfix Milter library. */

#include <milter.h>

/* Application-specific. */

 /*
  * SLMs.
  */
#define STR(x)	vstring_str(x)

/* milter_macro_defaults_create - parse default macro entries */

HTABLE *milter_macro_defaults_create(const char *macro_defaults)
{
    const char myname[] = "milter_macro_defaults_create";
    char   *saved_defaults = mystrdup(macro_defaults);
    char   *cp = saved_defaults;
    HTABLE *table = 0;
    VSTRING *canon_buf = 0;
    char   *nameval;

    while ((nameval = mystrtokq(&cp, CHARS_COMMA_SP, CHARS_BRACE)) != 0) {
	const char *err;
	char   *name;
	char   *value;

	/*
	 * Split the input into (name, value) pairs. Allow the forms
	 * name=value and  { name = value }, where the last form ignores
	 * whitespace after the opening "{", around the "=", and before the
	 * closing "}". A name may also be specified as {name}.
	 * 
	 * Use the form {name} for table lookups, because that is the form of
	 * the S8_MAC_* macro names.
	 */
	if (*nameval == CHARS_BRACE[0]
	    && nameval[balpar(nameval, CHARS_BRACE)] != '='
	    && (err = extpar(&nameval, CHARS_BRACE, EXTPAR_FLAG_NONE)) != 0)
	    msg_fatal("malformed default macro entry: %s in \"%s\"",
		      err, macro_defaults);
	if ((err = split_nameval(nameval, &name, &value)) != 0)
	    msg_fatal("malformed default macro entry: %s in \"%s\"",
		      err, macro_defaults);
	if (*name != '{')			/* } */
	    name = STR(vstring_sprintf(canon_buf ? canon_buf :
			    (canon_buf = vstring_alloc(20)), "{%s}", name));
	if (table == 0)
	    table = htable_create(1);
	if (htable_find(table, name) != 0) {
	    msg_warn("ignoring multiple default macro entries for %s in \"%s\"",
		     name, macro_defaults);
	} else {
	    (void) htable_enter(table, name, mystrdup(value));
	    if (msg_verbose)
		msg_info("%s: add name=%s default=%s", myname, name, value);
	}
    }
    myfree(saved_defaults);
    if (canon_buf)
	vstring_free(canon_buf);
    return (table);
}

/* milter_macro_lookup - look up macros */

static ARGV *milter_macro_lookup(MILTERS *milters, const char *macro_names)
{
    const char *myname = "milter_macro_lookup";
    char   *saved_names = mystrdup(macro_names);
    char   *cp = saved_names;
    ARGV   *argv = argv_alloc(10);
    VSTRING *canon_buf = vstring_alloc(20);
    const char *value;
    const char *name;

    while ((name = mystrtok(&cp, CHARS_COMMA_SP)) != 0) {
	if (msg_verbose)
	    msg_info("%s: \"%s\"", myname, name);
	if (*name != '{')			/* } */
	    name = STR(vstring_sprintf(canon_buf, "{%s}", name));
	if ((value = milters->mac_lookup(name, milters->mac_context)) != 0) {
	    if (msg_verbose)
		msg_info("%s: result \"%s\"", myname, value);
	    argv_add(argv, name, value, (char *) 0);
	} else if (milters->macro_defaults != 0
	     && (value = htable_find(milters->macro_defaults, name)) != 0) {
	    if (msg_verbose)
		msg_info("%s: using default \"%s\"", myname, value);
	    argv_add(argv, name, value, (char *) 0);
	}
    }
    myfree(saved_names);
    vstring_free(canon_buf);
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
			             MILTER_ADD_HEADER_FN add_header,
			             MILTER_EDIT_HEADER_FN upd_header,
			             MILTER_EDIT_HEADER_FN ins_header,
			             MILTER_DEL_HEADER_FN del_header,
			             MILTER_EDIT_FROM_FN chg_from,
			             MILTER_EDIT_RCPT_FN add_rcpt,
			             MILTER_EDIT_RCPT_PAR_FN add_rcpt_par,
			             MILTER_EDIT_RCPT_FN del_rcpt,
			             MILTER_EDIT_BODY_FN repl_body,
			             void *chg_context)
{
    milters->add_header = add_header;
    milters->upd_header = upd_header;
    milters->ins_header = ins_header;
    milters->del_header = del_header;
    milters->chg_from = chg_from;
    milters->add_rcpt = add_rcpt;
    milters->add_rcpt_par = add_rcpt_par;
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
    ARGV   *global_macros = 0;
    ARGV   *any_macros;

#define MILTER_MACRO_EVAL(global_macros, m, milters, member) \
	((m->macros && m->macros->member[0]) ? \
	    milter_macro_lookup(milters, m->macros->member) : \
		global_macros ? global_macros : \
		    (global_macros = \
		         milter_macro_lookup(milters, milters->macros->member)))

    if (msg_verbose)
	msg_info("report connect to all milters");
    for (resp = 0, m = milters->milter_list; resp == 0 && m != 0; m = m->next) {
	any_macros = MILTER_MACRO_EVAL(global_macros, m, milters, conn_macros);
	resp = m->conn_event(m, client_name, client_addr, client_port,
			     addr_family, any_macros);
	if (any_macros != global_macros)
	    argv_free(any_macros);
    }
    if (global_macros)
	argv_free(global_macros);
    return (resp);
}

/* milter_helo_event - report helo event */

const char *milter_helo_event(MILTERS *milters, const char *helo_name,
			              int esmtp_flag)
{
    const char *resp;
    MILTER *m;
    ARGV   *global_macros = 0;
    ARGV   *any_macros;

    if (msg_verbose)
	msg_info("report helo to all milters");
    for (resp = 0, m = milters->milter_list; resp == 0 && m != 0; m = m->next) {
	any_macros = MILTER_MACRO_EVAL(global_macros, m, milters, helo_macros);
	resp = m->helo_event(m, helo_name, esmtp_flag, any_macros);
	if (any_macros != global_macros)
	    argv_free(any_macros);
    }
    if (global_macros)
	argv_free(global_macros);
    return (resp);
}

/* milter_mail_event - report mail from event */

const char *milter_mail_event(MILTERS *milters, const char **argv)
{
    const char *resp;
    MILTER *m;
    ARGV   *global_macros = 0;
    ARGV   *any_macros;

    if (msg_verbose)
	msg_info("report sender to all milters");
    for (resp = 0, m = milters->milter_list; resp == 0 && m != 0; m = m->next) {
	any_macros = MILTER_MACRO_EVAL(global_macros, m, milters, mail_macros);
	resp = m->mail_event(m, argv, any_macros);
	if (any_macros != global_macros)
	    argv_free(any_macros);
    }
    if (global_macros)
	argv_free(global_macros);
    return (resp);
}

/* milter_rcpt_event - report rcpt to event */

const char *milter_rcpt_event(MILTERS *milters, int flags, const char **argv)
{
    const char *resp;
    MILTER *m;
    ARGV   *global_macros = 0;
    ARGV   *any_macros;

    if (msg_verbose)
	msg_info("report recipient to all milters (flags=0x%x)", flags);
    for (resp = 0, m = milters->milter_list; resp == 0 && m != 0; m = m->next) {
	if ((flags & MILTER_FLAG_WANT_RCPT_REJ) == 0
	    || (m->flags & MILTER_FLAG_WANT_RCPT_REJ) != 0) {
	    any_macros =
		MILTER_MACRO_EVAL(global_macros, m, milters, rcpt_macros);
	    resp = m->rcpt_event(m, argv, any_macros);
	    if (any_macros != global_macros)
		argv_free(any_macros);
	}
    }
    if (global_macros)
	argv_free(global_macros);
    return (resp);
}

/* milter_data_event - report data event */

const char *milter_data_event(MILTERS *milters)
{
    const char *resp;
    MILTER *m;
    ARGV   *global_macros = 0;
    ARGV   *any_macros;

    if (msg_verbose)
	msg_info("report data to all milters");
    for (resp = 0, m = milters->milter_list; resp == 0 && m != 0; m = m->next) {
	any_macros = MILTER_MACRO_EVAL(global_macros, m, milters, data_macros);
	resp = m->data_event(m, any_macros);
	if (any_macros != global_macros)
	    argv_free(any_macros);
    }
    if (global_macros)
	argv_free(global_macros);
    return (resp);
}

/* milter_unknown_event - report unknown command */

const char *milter_unknown_event(MILTERS *milters, const char *command)
{
    const char *resp;
    MILTER *m;
    ARGV   *global_macros = 0;
    ARGV   *any_macros;

    if (msg_verbose)
	msg_info("report unknown command to all milters");
    for (resp = 0, m = milters->milter_list; resp == 0 && m != 0; m = m->next) {
	any_macros = MILTER_MACRO_EVAL(global_macros, m, milters, unk_macros);
	resp = m->unknown_event(m, command, any_macros);
	if (any_macros != global_macros)
	    argv_free(any_macros);
    }
    if (global_macros)
	argv_free(global_macros);
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

const char *milter_message(MILTERS *milters, VSTREAM *fp, off_t data_offset,
			           ARGV *auto_hdrs)
{
    const char *resp;
    MILTER *m;
    ARGV   *global_eoh_macros = 0;
    ARGV   *global_eod_macros = 0;
    ARGV   *any_eoh_macros;
    ARGV   *any_eod_macros;

    if (msg_verbose)
	msg_info("inspect content by all milters");
    for (resp = 0, m = milters->milter_list; resp == 0 && m != 0; m = m->next) {
	any_eoh_macros = MILTER_MACRO_EVAL(global_eoh_macros, m, milters, eoh_macros);
	any_eod_macros = MILTER_MACRO_EVAL(global_eod_macros, m, milters, eod_macros);
	resp = m->message(m, fp, data_offset, any_eoh_macros, any_eod_macros,
			  auto_hdrs);
	if (any_eoh_macros != global_eoh_macros)
	    argv_free(any_eoh_macros);
	if (any_eod_macros != global_eod_macros)
	    argv_free(any_eod_macros);
    }
    if (global_eoh_macros)
	argv_free(global_eoh_macros);
    if (global_eod_macros)
	argv_free(global_eod_macros);
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

 /*
  * Table-driven parsing of main.cf parameter overrides for specific Milters.
  * We derive the override names from the corresponding main.cf parameter
  * names by skipping the redundant "milter_" prefix.
  */
static ATTR_OVER_TIME time_table[] = {
    7 + VAR_MILT_CONN_TIME, DEF_MILT_CONN_TIME, 0, 1, 0,
    7 + VAR_MILT_CMD_TIME, DEF_MILT_CMD_TIME, 0, 1, 0,
    7 + VAR_MILT_MSG_TIME, DEF_MILT_MSG_TIME, 0, 1, 0,
    0,
};
static ATTR_OVER_STR str_table[] = {
    7 + VAR_MILT_PROTOCOL, 0, 1, 0,
    7 + VAR_MILT_DEF_ACTION, 0, 1, 0,
    0,
};

#define link_override_table_to_variable(table, var) \
	do { table[var##_offset].target = &var; } while (0)

#define my_conn_timeout_offset	0
#define my_cmd_timeout_offset	1
#define my_msg_timeout_offset	2

#define	my_protocol_offset	0
#define	my_def_action_offset	1

/* milter_new - create milter list */

MILTERS *milter_new(const char *names,
		            int conn_timeout,
		            int cmd_timeout,
		            int msg_timeout,
		            const char *protocol,
		            const char *def_action,
		            MILTER_MACROS *macros,
		            HTABLE *macro_defaults)
{
    MILTERS *milters;
    MILTER *head = 0;
    MILTER *tail = 0;
    char   *name;
    MILTER *milter;
    const char *sep = CHARS_COMMA_SP;
    const char *parens = CHARS_BRACE;
    int     my_conn_timeout;
    int     my_cmd_timeout;
    int     my_msg_timeout;
    const char *my_protocol;
    const char *my_def_action;

    /*
     * Initialize.
     */
    link_override_table_to_variable(time_table, my_conn_timeout);
    link_override_table_to_variable(time_table, my_cmd_timeout);
    link_override_table_to_variable(time_table, my_msg_timeout);
    link_override_table_to_variable(str_table, my_protocol);
    link_override_table_to_variable(str_table, my_def_action);

    /*
     * Parse the milter list.
     */
    milters = (MILTERS *) mymalloc(sizeof(*milters));
    if (names != 0 && *names != 0) {
	char   *saved_names = mystrdup(names);
	char   *cp = saved_names;
	char   *op;
	char   *err;

	/*
	 * Instantiate Milters, allowing for per-Milter overrides.
	 */
	while ((name = mystrtokq(&cp, sep, parens)) != 0) {
	    my_conn_timeout = conn_timeout;
	    my_cmd_timeout = cmd_timeout;
	    my_msg_timeout = msg_timeout;
	    my_protocol = protocol;
	    my_def_action = def_action;
	    if (name[0] == parens[0]) {
		op = name;
		if ((err = extpar(&op, parens, EXTPAR_FLAG_NONE)) != 0)
		    msg_fatal("milter service syntax error: %s", err);
		if ((name = mystrtok(&op, sep)) == 0)
		    msg_fatal("empty milter definition: \"%s\"", names);
		attr_override(op, sep, parens,
			      CA_ATTR_OVER_STR_TABLE(str_table),
			      CA_ATTR_OVER_TIME_TABLE(time_table),
			      CA_ATTR_OVER_END);
	    }
	    milter = milter8_create(name, my_conn_timeout, my_cmd_timeout,
				    my_msg_timeout, my_protocol,
				    my_def_action, milters);
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
    milters->macros = macros;
    milters->macro_defaults = macro_defaults;
    milters->add_header = 0;
    milters->upd_header = milters->ins_header = 0;
    milters->del_header = 0;
    milters->add_rcpt = milters->del_rcpt = 0;
    milters->repl_body = 0;
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
    if (milters->macros)
	milter_macros_free(milters->macros);
    if (milters->macro_defaults)
	htable_free(milters->macro_defaults, myfree);
    myfree((void *) milters);
}

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
     * 
     * XXX We must send *some* information, even when there are no active
     * filters, otherwise the cleanup server would try to apply its own
     * non_smtpd_milters settings.
     */
    if (milters != 0)
	for (m = milters->milter_list; m != 0; m = m->next)
	    if (m->active(m))
		count++;
    (void) rec_fprintf(stream, REC_TYPE_MILT_COUNT, "%d", count);

    if (msg_verbose)
	msg_info("send %d milters", count);

    /*
     * XXX Optimization: don't send or receive further information when there
     * aren't any active filters.
     */
    if (count <= 0)
	return (0);

    /*
     * Send the filter macro name lists.
     */
    (void) attr_print(stream, ATTR_FLAG_MORE,
		      SEND_ATTR_FUNC(milter_macros_print,
				     (void *) milters->macros),
		      ATTR_TYPE_END);

    /*
     * Send the filter macro defaults.
     */
    count = milters->macro_defaults ? milters->macro_defaults->used : 0;
    (void) attr_print(stream, ATTR_FLAG_MORE,
		      SEND_ATTR_INT(MAIL_ATTR_SIZE, count),
		      ATTR_TYPE_END);
    if (count > 0)
	(void) attr_print(stream, ATTR_FLAG_MORE,
			  SEND_ATTR_HASH(milters->macro_defaults),
			  ATTR_TYPE_END);

    /*
     * Send the filter instances.
     */
    for (m = milters->milter_list; m != 0; m = m->next)
	if (m->active(m) && (status = m->send(m, stream)) != 0)
	    break;

    /*
     * Over to you.
     */
    if (status != 0
	|| attr_scan(stream, ATTR_FLAG_STRICT,
		     RECV_ATTR_INT(MAIL_ATTR_STATUS, &status),
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
    int     macro_default_count;

    if (msg_verbose)
	msg_info("receive %d milters", count);

    /*
     * XXX We must instantiate a MILTERS structure even when the sender has
     * no active filters, otherwise the cleanup server would try to use its
     * own non_smtpd_milters settings.
     */
#define NO_MILTERS	((char *) 0)
#define NO_TIMEOUTS	0, 0, 0
#define NO_PROTOCOL	((char *) 0)
#define NO_ACTION	((char *) 0)
#define NO_MACROS	((MILTER_MACROS *) 0)
#define NO_MACRO_DEFLTS	((HTABLE *) 0)

    milters = milter_new(NO_MILTERS, NO_TIMEOUTS, NO_PROTOCOL, NO_ACTION,
			 NO_MACROS, NO_MACRO_DEFLTS);

    /*
     * XXX Optimization: don't send or receive further information when there
     * aren't any active filters.
     */
    if (count <= 0)
	return (milters);

    /*
     * Receive the global macro name lists.
     */
    milters->macros = milter_macros_alloc(MILTER_MACROS_ALLOC_ZERO);
    if (attr_scan(stream, ATTR_FLAG_STRICT | ATTR_FLAG_MORE,
		  RECV_ATTR_FUNC(milter_macros_scan,
				 (void *) milters->macros),
		  ATTR_TYPE_END) != 1) {
	milter_free(milters);
	return (0);
    }

    /*
     * Receive the filter macro defaults.
     */
    if (attr_scan(stream, ATTR_FLAG_STRICT | ATTR_FLAG_MORE,
		  RECV_ATTR_INT(MAIL_ATTR_SIZE, &macro_default_count),
		  ATTR_TYPE_END) != 1
	|| (macro_default_count > 0
	    && attr_scan(stream, ATTR_FLAG_STRICT | ATTR_FLAG_MORE,
			 RECV_ATTR_HASH(milters->macro_defaults
					= htable_create(1)),
			 ATTR_TYPE_END) != macro_default_count)) {
	milter_free(milters);
	return (0);
    }

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

    /*
     * Over to you.
     */
    (void) attr_print(stream, ATTR_FLAG_NONE,
		      SEND_ATTR_INT(MAIL_ATTR_STATUS, 0),
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
    char   *data_macros, *eoh_macros, *eod_macros, *unk_macros;
    char   *macro_deflts;
    VSTRING *inbuf = vstring_alloc(100);
    char   *bufp;
    char   *cmd;
    int     ch;
    int     istty = isatty(vstream_fileno(VSTREAM_IN));

    conn_macros = helo_macros = mail_macros = rcpt_macros = data_macros
	= eoh_macros = eod_macros = unk_macros = macro_deflts = "";

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
				    rcpt_macros, data_macros, eoh_macros,
				    eod_macros, unk_macros, macro_deflts);
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
	    resp = milter_rcpt_event(milters, 0, (const char **) args);
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
