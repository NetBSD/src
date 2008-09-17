/*++
/* NAME
/*	bounce_templates 3
/* SUMMARY
/*	bounce template group support
/* SYNOPSIS
/*	#include <bounce_template.h>
/*
/*	typedef struct {
/* .in +4
/*		BOUNCE_TEMPLATE *failure;
/*		BOUNCE_TEMPLATE *delay;
/*		BOUNCE_TEMPLATE *success;
/*		BOUNCE_TEMPLATE *verify;
/* .in -4
/*	} BOUNCE_TEMPLATES;
/*
/*	BOUNCE_TEMPLATES *bounce_templates_create(void)
/*
/*	void	bounce_templates_free(templates)
/*	BOUNCE_TEMPLATES *templates;
/*
/*	void	bounce_templates_load(stream, templates)
/*	VSTREAM	*stream;
/*	BOUNCE_TEMPLATES *templates;
/*
/*	void	bounce_templates_expand(stream, templates)
/*	VSTREAM	*stream;
/*	BOUNCE_TEMPLATES *templates;
/*
/*	void	bounce_templates_dump(stream, templates)
/*	VSTREAM	*stream;
/*	BOUNCE_TEMPLATES *templates;
/* DESCRIPTION
/*	This module implements support for bounce template groups
/*	(i.e. groups that contain one template of each type).
/*
/*	bounce_templates_create() creates a bounce template group,
/*	with default settings.
/*
/*	bounce_templates_free() destroys a bounce template group.
/*
/*	bounce_templates_load() reads zero or more bounce templates
/*	from the specified file to override built-in templates.
/*
/*	bounce_templates_expand() expands $name macros and writes
/*	the text portions of the specified bounce template group
/*	to the specified stream.
/*
/*	bounce_templates_dump() writes the complete content of the
/*	specified bounce template group to the specified stream.
/*	The format is compatible with bounce_templates_load().
/* DIAGNOSTICS
/*	Fatal error: out of memory, undefined macro name in template.
/* SEE ALSO
/*	bounce_template(3) bounce template support
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
#include <ctype.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <stringops.h>
#include <vstring.h>
#include <vstream.h>
#include <vstring_vstream.h>

/* Global library. */

#include <mail_addr.h>
#include <mail_proto.h>

/* Application-specific. */

#include <bounce_template.h>

 /*
  * The fail template is for permanent failure.
  */
static const char *def_bounce_failure_body[] = {
    "This is the mail system at host $myhostname.",
    "",
    "I'm sorry to have to inform you that your message could not",
    "be delivered to one or more recipients. It's attached below.",
    "",
    "For further assistance, please send mail to " MAIL_ADDR_POSTMASTER ".",
    "",
    "If you do so, please include this problem report. You can",
    "delete your own text from the attached returned message.",
    "",
    "                   The mail system",
    0,
};

static const BOUNCE_TEMPLATE def_bounce_failure_template = {
    0,
    BOUNCE_TMPL_CLASS_FAILURE,
    "[built-in]",
    "us-ascii",
    MAIL_ATTR_ENC_7BIT,
    MAIL_ADDR_MAIL_DAEMON " (Mail Delivery System)",
    "Undelivered Mail Returned to Sender",
    "Postmaster Copy: Undelivered Mail",
    def_bounce_failure_body,
    &def_bounce_failure_template,
};

 /*
  * The delay template is for delayed mail notifications.
  */
static const char *def_bounce_delay_body[] = {
    "This is the mail system at host $myhostname.",
    "",
    "####################################################################",
    "# THIS IS A WARNING ONLY.  YOU DO NOT NEED TO RESEND YOUR MESSAGE. #",
    "####################################################################",
    "",
    "Your message could not be delivered for more than $delay_warning_time_hours hour(s)."
    ,
    "It will be retried until it is $maximal_queue_lifetime_days day(s) old.",
    "",
    "For further assistance, please send mail to " MAIL_ADDR_POSTMASTER ".",
    "",
    "If you do so, please include this problem report. You can",
    "delete your own text from the attached returned message.",
    "",
    "                   The mail system",
    0,
};

static const BOUNCE_TEMPLATE def_bounce_delay_template = {
    0,
    BOUNCE_TMPL_CLASS_DELAY,
    "[built-in]",
    "us-ascii",
    MAIL_ATTR_ENC_7BIT,
    MAIL_ADDR_MAIL_DAEMON " (Mail Delivery System)",
    "Delayed Mail (still being retried)",
    "Postmaster Warning: Delayed Mail",
    def_bounce_delay_body,
    &def_bounce_delay_template
};

 /*
  * The success template is for "delivered", "expanded" and "relayed" success
  * notifications.
  */
static const char *def_bounce_success_body[] = {
    "This is the mail system at host $myhostname.",
    "",
    "Your message was successfully delivered to the destination(s)",
    "listed below. If the message was delivered to mailbox you will",
    "receive no further notifications. Otherwise you may still receive",
    "notifications of mail delivery errors from other systems.",
    "",
    "                   The mail system",
    0,
};

static const BOUNCE_TEMPLATE def_bounce_success_template = {
    0,
    BOUNCE_TMPL_CLASS_SUCCESS,
    "[built-in]",
    "us-ascii",
    MAIL_ATTR_ENC_7BIT,
    MAIL_ADDR_MAIL_DAEMON " (Mail Delivery System)",
    "Successful Mail Delivery Report",
    0,
    def_bounce_success_body,
    &def_bounce_success_template,
};

 /*
  * The "verify" template is for verbose delivery (sendmail -v) and for
  * address verification (sendmail -bv).
  */
static const char *def_bounce_verify_body[] = {
    "This is the mail system at host $myhostname.",
    "",
    "Enclosed is the mail delivery report that you requested.",
    "",
    "                   The mail system",
    0,
};

static const BOUNCE_TEMPLATE def_bounce_verify_template = {
    0,
    BOUNCE_TMPL_CLASS_VERIFY,
    "[built-in]",
    "us-ascii",
    MAIL_ATTR_ENC_7BIT,
    MAIL_ADDR_MAIL_DAEMON " (Mail Delivery System)",
    "Mail Delivery Status Report",
    0,
    def_bounce_verify_body,
    &def_bounce_verify_template,
};

 /*
  * SLMs.
  */
#define STR(x)	vstring_str(x)

/* bounce_templates_create - create template group */

BOUNCE_TEMPLATES *bounce_templates_create(void)
{
    BOUNCE_TEMPLATES *bs;

    bs = (BOUNCE_TEMPLATES *) mymalloc(sizeof(*bs));
    bs->failure = bounce_template_create(&def_bounce_failure_template);
    bs->delay = bounce_template_create(&def_bounce_delay_template);
    bs->success = bounce_template_create(&def_bounce_success_template);
    bs->verify = bounce_template_create(&def_bounce_verify_template);
    return (bs);
}

/* bounce_templates_free - destroy template group */

void    bounce_templates_free(BOUNCE_TEMPLATES *bs)
{
    bounce_template_free(bs->failure);
    bounce_template_free(bs->delay);
    bounce_template_free(bs->success);
    bounce_template_free(bs->verify);
    myfree((char *) bs);
}

/* bounce_templates_load - load template or group from stream */

void    bounce_templates_load(VSTREAM *fp, BOUNCE_TEMPLATES *ts)
{
    VSTRING *line_buf;
    char   *member_name;
    VSTRING *multi_line_buf = 0;
    VSTRING *saved_member_name = 0;
    VSTRING *saved_end_marker = 0;
    char   *value;
    int     lineno;
    const char *err;
    char   *cp;
    int     len;			/* Grr... */

    /*
     * XXX That's a lot of non-reusable code to parse a configuration file.
     * Unfortunately, much of the "name = value" infrastructure is married to
     * the dict(3) class which doesn't really help here.
     */
    line_buf = vstring_alloc(100);
    lineno = 1;
    while (vstring_get_nonl(line_buf, fp) > 0) {
	lineno++;
	cp = STR(line_buf) + strspn(STR(line_buf), " \t\n\v\f\r");
	if (*cp == 0 || *cp == '#')
	    continue;
	if ((err = split_nameval(STR(line_buf), &member_name, &value)) != 0)
	    msg_fatal("%s, line %d: %s: \"%s\"",
		      VSTREAM_PATH(fp), lineno, err, STR(line_buf));
	if (value[0] == '<' && value[1] == '<') {
	    value += 2;
	    while (ISSPACE(*value))
		value++;
	    if (*value == 0)
		msg_fatal("%s, line %d: missing end marker after <<",
			  VSTREAM_PATH(fp), lineno);
	    if (!ISALNUM(*value))
		msg_fatal("%s, line %d: malformed end marker after <<",
			  VSTREAM_PATH(fp), lineno);
	    if (multi_line_buf == 0) {
		saved_member_name = vstring_alloc(100);
		saved_end_marker = vstring_alloc(100);
		multi_line_buf = vstring_alloc(100);
	    } else
		VSTRING_RESET(multi_line_buf);
	    vstring_strcpy(saved_member_name, member_name);
	    vstring_strcpy(saved_end_marker, value);
	    while (vstring_get_nonl(line_buf, fp) > 0) {
		lineno++;
		if (strcmp(STR(line_buf), STR(saved_end_marker)) == 0)
		    break;
		if (VSTRING_LEN(multi_line_buf) > 0)
		    vstring_strcat(multi_line_buf, "\n");
		vstring_strcat(multi_line_buf, STR(line_buf));
	    }
	    if (vstream_feof(fp))
		msg_warn("%s, line %d: missing \"%s\" end marker",
			  VSTREAM_PATH(fp), lineno, value);
	    member_name = STR(saved_member_name);
	    value = STR(multi_line_buf);
	}
#define MATCH_TMPL_NAME(tname, tname_len, mname) \
    (strncmp(tname, mname, tname_len = strlen(tname)) == 0 \
	&& strcmp(mname + tname_len, "_template") == 0)

	if (MATCH_TMPL_NAME(ts->failure->class, len, member_name))
	    bounce_template_load(ts->failure, VSTREAM_PATH(fp), value);
	else if (MATCH_TMPL_NAME(ts->delay->class, len, member_name))
	    bounce_template_load(ts->delay, VSTREAM_PATH(fp), value);
	else if (MATCH_TMPL_NAME(ts->success->class, len, member_name))
	    bounce_template_load(ts->success, VSTREAM_PATH(fp), value);
	else if (MATCH_TMPL_NAME(ts->verify->class, len, member_name))
	    bounce_template_load(ts->verify, VSTREAM_PATH(fp), value);
	else
	    msg_warn("%s, line %d: unknown template name: %s "
		     "-- ignoring this template",
		     VSTREAM_PATH(fp), lineno, member_name);
    }
    vstring_free(line_buf);
    if (multi_line_buf) {
	vstring_free(saved_member_name);
	vstring_free(saved_end_marker);
	vstring_free(multi_line_buf);
    }
}

/* bounce_plain_out - output line as plain text */

static int bounce_plain_out(VSTREAM *fp, const char *text)
{
    vstream_fprintf(fp, "%s\n", text);
    return (0);
}

/* bounce_templates_expand - dump expanded template group text to stream */

void    bounce_templates_expand(VSTREAM *fp, BOUNCE_TEMPLATES *ts)
{
    BOUNCE_TEMPLATE *tp;

    tp = ts->failure;
    vstream_fprintf(fp, "expanded_%s_text = <<EOF\n", tp->class);
    bounce_template_expand(bounce_plain_out, fp, tp);
    vstream_fprintf(fp, "EOF\n\n");

    tp = ts->delay;
    vstream_fprintf(fp, "expanded_%s_text = <<EOF\n", tp->class);
    bounce_template_expand(bounce_plain_out, fp, tp);
    vstream_fprintf(fp, "EOF\n\n");

    tp = ts->success;
    vstream_fprintf(fp, "expanded_%s_text = <<EOF\n", tp->class);
    bounce_template_expand(bounce_plain_out, fp, tp);
    vstream_fprintf(fp, "EOF\n\n");

    tp = ts->verify;
    vstream_fprintf(fp, "expanded_%s_text = <<EOF\n", tp->class);
    bounce_template_expand(bounce_plain_out, fp, tp);
    vstream_fprintf(fp, "EOF\n");
}

/* bounce_templates_dump - dump bounce template group to stream */

void    bounce_templates_dump(VSTREAM *fp, BOUNCE_TEMPLATES *ts)
{
    BOUNCE_TEMPLATE *tp;

    tp = ts->failure;
    vstream_fprintf(fp, "%s_template = <<EOF\n", tp->class);
    bounce_template_dump(fp, tp);
    vstream_fprintf(fp, "EOF\n\n");

    tp = ts->delay;
    vstream_fprintf(fp, "%s_template = <<EOF\n", tp->class);
    bounce_template_dump(fp, tp);
    vstream_fprintf(fp, "EOF\n\n");

    tp = ts->success;
    vstream_fprintf(fp, "%s_template = <<EOF\n", tp->class);
    bounce_template_dump(fp, tp);
    vstream_fprintf(fp, "EOF\n\n");

    tp = ts->verify;
    vstream_fprintf(fp, "%s_template = <<EOF\n", tp->class);
    bounce_template_dump(fp, tp);
    vstream_fprintf(fp, "EOF\n");
}
