/*	$NetBSD: info_log_addr_form.c,v 1.2 2020/03/18 19:05:16 christos Exp $	*/

/*++
/* NAME
/*	info_log_addr_form 3
/* SUMMARY
/*	format internal-form information for info logging
/* SYNOPSIS
/*	#include <info_log_addr_form.h>
/*
/*	const char *info_log_addr_form_recipient(
/*	const char *recipient_addr)
/*
/*	const char *info_log_addr_form_sender_addr(
/*	const char *sender_addr)
/* DESCRIPTION
/*	info_log_addr_form_recipient() and info_log_addr_form_sender_addr()
/*	format an internal-form recipient or sender email address
/*	for non-debug logging. Each function has its own private
/*	buffer. Each call overwrites the result from a previous call.
/*
/*	Note: the empty address is passed unchanged; it is not
/*	formatted as "".
/* .IP recipient_addr
/* .IP *sender_addr
/*	An internal-form email address.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>

 /*
  * Utility library.
  */
#include <msg.h>
#include <name_code.h>
#include <vstring.h>

 /*
  * Global library.
  */
#include <info_log_addr_form.h>
#include <mail_addr_form.h>
#include <mail_params.h>
#include <quote_822_local.h>

#define INFO_LOG_ADDR_FORM_VAL_NOT_SET	0
#define INFO_LOG_ADDR_FORM_VAL_INTERNAL	1
#define INFO_LOG_ADDR_FORM_VAL_EXTERNAL	2

/* Format for info logging. */

int     info_log_addr_form_form = INFO_LOG_ADDR_FORM_VAL_NOT_SET;

#define STR(x)	vstring_str(x)

/* info_log_addr_form_init - one-time initialization */

static void info_log_addr_form_init(void)
{
    static NAME_CODE info_log_addr_form_table[] = {
	INFO_LOG_ADDR_FORM_NAME_EXTERNAL, INFO_LOG_ADDR_FORM_VAL_EXTERNAL,
	INFO_LOG_ADDR_FORM_NAME_INTERNAL, INFO_LOG_ADDR_FORM_VAL_INTERNAL,
	0, INFO_LOG_ADDR_FORM_VAL_NOT_SET,
    };
    info_log_addr_form_form = name_code(info_log_addr_form_table,
					NAME_CODE_FLAG_NONE,
					var_info_log_addr_form);

    if (info_log_addr_form_form == INFO_LOG_ADDR_FORM_VAL_NOT_SET)
	msg_fatal("invalid parameter setting \"%s = %s\"",
		  VAR_INFO_LOG_ADDR_FORM, var_info_log_addr_form);
}

/* info_log_addr_form - format an email address for info logging */

static VSTRING *info_log_addr_form(VSTRING *buf, const char *addr)
{
    const char myname[] = "info_log_addr_form";

    if (buf == 0)
	buf = vstring_alloc(100);
    if (info_log_addr_form_form == INFO_LOG_ADDR_FORM_VAL_NOT_SET)
	info_log_addr_form_init();
    if (*addr == 0
	|| info_log_addr_form_form == INFO_LOG_ADDR_FORM_VAL_INTERNAL) {
	vstring_strcpy(buf, addr);
    } else if (info_log_addr_form_form == INFO_LOG_ADDR_FORM_VAL_EXTERNAL) {
	quote_822_local(buf, addr);
    } else {
	msg_panic("%s: bad format type: %d",
		  myname, info_log_addr_form_form);
    }
    return (buf);
}

/* info_log_addr_form_recipient - format a recipient address for info logging */

const char *info_log_addr_form_recipient(const char *recipient_addr)
{
    static VSTRING *recipient_buffer = 0;

    recipient_buffer = info_log_addr_form(recipient_buffer, recipient_addr);
    return (STR(recipient_buffer));
}

/* info_log_addr_form_sender - format a sender address for info logging */

const char *info_log_addr_form_sender(const char *sender_addr)
{
    static VSTRING *sender_buffer = 0;

    sender_buffer = info_log_addr_form(sender_buffer, sender_addr);
    return (STR(sender_buffer));
}
