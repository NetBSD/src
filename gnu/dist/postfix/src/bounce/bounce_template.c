/*++
/* NAME
/*	bounce_template 3
/* SUMMARY
/*	bounce template support
/* SYNOPSIS
/*	#include <bounce_template.h>
/*
/*	BOUNCE_TEMPLATE *bounce_template_create(def_template)
/*	const BOUNCE_TEMPLATE *def_template;
/*
/*	void	bounce_template_free(template)
/*	BOUNCE_TEMPLATE *template;
/*
/*	void	bounce_template_load(template, stream, buffer, origin)
/*	BOUNCE_TEMPLATE *template;
/*	VSTREAM	*stream;
/*	const char *buffer;
/*	const char *origin;
/*
/*	void	bounce_template_headers(out_fn, stream, template,
/*					rcpt, postmaster_copy)
/*	int	(*out_fn)(VSTREAM *, const char *, ...);
/*	VSTREAM	*stream;
/*	BOUNCE_TEMPLATE *template;
/*	const char *rcpt;
/*	int	postmaster_copy;
/*
/*	const char *bounce_template_encoding(template)
/*	BOUNCE_TEMPLATE *template;
/*
/*	const char *bounce_template_charset(template)
/*	BOUNCE_TEMPLATE *template;
/*
/*	void	bounce_template_expand(out_fn, stream, template)
/*	int	(*out_fn)(VSTREAM *, const char *);
/*	VSTREAM	*stream;
/*	BOUNCE_TEMPLATE *template;
/*
/*	void	bounce_template_dump(stream, template)
/*	VSTREAM	*stream;
/*	BOUNCE_TEMPLATE *template;
/*
/*	int	IS_FAILURE_TEMPLATE(template)
/*	int	IS_DELAY_TEMPLATE(template)
/*	int	IS_SUCCESS_TEMPLATE(template)
/*	int	IS_VERIFY_TEMPLATE(template)
/*	BOUNCE_TEMPLATE *template;
/* DESCRIPTION
/*	This module implements the built-in and external bounce
/*	message template support. The content of a template are
/*	private. To access information within a template, use
/*	the API described in this document.
/*
/*	bounce_template_create() creates a template, with the
/*	specified default settings. The template defaults are not
/*	copied.
/*
/*	bounce_template_free() destroys a bounce message template.
/*
/*	bounce_template_load() overrides a bounce template with the
/*	specified buffer from the specified origin. The buffer and
/*	origin are copied. Specify a null buffer and origin pointer
/*	to reset the template to the defaults specified with
/*	bounce_template_create().
/*
/*	bounce_template_headers() sends the postmaster or non-postmaster
/*	From/Subject/To message headers to the specified stream.
/*	The recipient address is expected to be in RFC822 external
/*	form. The postmaster_copy argument is one of POSTMASTER_COPY
/*	or NO_POSTMASTER_COPY.
/*
/*	bounce_template_encoding() returns the encoding (MAIL_ATTR_ENC_7BIT
/*	or MAIL_ATTR_ENC_8BIT) for the bounce template message text.
/*
/*	bounce_template_charset() returns the character set for the
/*	bounce template message text.
/*
/*	bounce_template_expand() expands the body text of the
/*	specified template and writes the result to the specified
/*	stream.
/*
/*	bounce_template_dump() dumps the specified template to the
/*	specified stream.
/*
/*	The IS_MUMBLE_TEMPLATE() macros are predicates that
/*	determine whether the template is of the specified type.
/* DIAGNOSTICS
/*	Fatal error: out of memory, undefined macro name in template.
/* SEE ALSO
/*	bounce_templates(3) bounce template group support
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
#include <mac_expand.h>
#include <split_at.h>
#include <stringops.h>
#include <mymalloc.h>

/* Global library. */

#include <mail_params.h>
#include <mail_proto.h>
#include <mail_conf.h>
#include <is_header.h>

/* Application-specific. */

#include <bounce_template.h>

 /*
  * The following tables implement support for bounce template expansions of
  * $<parameter_name>_days ($<parameter_name>_hours, etc.). The expansion of
  * these is the actual parameter value divided by the number of seconds in a
  * day (hour, etc.), so that we can produce nicely formatted bounce messages
  * with time values converted into the appropriate units.
  * 
  * Ideally, the bounce template processor would strip the _days etc. suffix
  * from the parameter name, and use the parameter name to look up the actual
  * parameter value and its default value (the default value specifies the
  * default time unit of that parameter (seconds, minutes, etc.)), and use
  * this to convert the parameter string value into the corresponding number
  * of seconds. The bounce template processor would then use the _hours etc.
  * suffix from the bounce template to divide this number by the number of
  * seconds in an hour, etc. and produce the number that is needed for the
  * template.
  * 
  * Unfortunately, there exists no code to look up default values by parameter
  * name. If such code existed, then we could do the _days, _hours, etc.
  * conversion with every main.cf time parameter without having to know in
  * advance what time parameter names exist.
  * 
  * So we have to either maintain our own table of all time related main.cf
  * parameter names and defaults (like the postconf command does) or we make
  * a special case for a few parameters of special interest.
  * 
  * We go for the second solution. There are only a few parameters that need
  * this treatment, and there will be more special cases when individual
  * queue files get support for individual expiration times, and when other
  * queue file information needs to be reported in bounce template messages.
  * 
  * A really lame implementation would simply strip the optional s, h, d, etc.
  * suffix from the actual (string) parameter value and not do any conversion
  * at all to hours, days or weeks. But then the information in delay warning
  * notices could be seriously incorrect.
  */
typedef struct {
    const char *suffix;			/* days, hours, etc. */
    int     suffix_len;			/* byte count */
    int     divisor;			/* divisor */
} BOUNCE_TIME_DIVISOR;

#define STRING_AND_LEN(x) (x), (sizeof(x) - 1)

static const BOUNCE_TIME_DIVISOR time_divisors[] = {
    STRING_AND_LEN("seconds"), 1,
    STRING_AND_LEN("minutes"), 60,
    STRING_AND_LEN("hours"), 60 * 60,
    STRING_AND_LEN("days"), 24 * 60 * 60,
    STRING_AND_LEN("weeks"), 7 * 24 * 60 * 60,
    0, 0,
};

 /*
  * The few special-case main.cf parameters that have support for _days, etc.
  * suffixes for automatic conversion when expanded into a bounce template.
  */
typedef struct {
    const char *param_name;		/* parameter name */
    int     param_name_len;		/* name length */
    int    *value;			/* parameter value */
} BOUNCE_TIME_PARAMETER;

static const BOUNCE_TIME_PARAMETER time_parameter[] = {
    STRING_AND_LEN(VAR_DELAY_WARN_TIME), &var_delay_warn_time,
    STRING_AND_LEN(VAR_MAX_QUEUE_TIME), &var_max_queue_time,
    0, 0,
};

 /*
  * SLMs.
  */
#define STR(x) vstring_str(x)

/* bounce_template_create - create one template */

BOUNCE_TEMPLATE *bounce_template_create(const BOUNCE_TEMPLATE *prototype)
{
    BOUNCE_TEMPLATE *tp;

    tp = (BOUNCE_TEMPLATE *) mymalloc(sizeof(*tp));
    *tp = *prototype;
    return (tp);
}

/* bounce_template_free - destroy one template */

void    bounce_template_free(BOUNCE_TEMPLATE *tp)
{
    if (tp->buffer) {
	myfree(tp->buffer);
	myfree((char *) tp->origin);
    }
    myfree((char *) tp);
}

/* bounce_template_reset - reset template to default */

static void bounce_template_reset(BOUNCE_TEMPLATE *tp)
{
    myfree(tp->buffer);
    myfree((char *) tp->origin);
    *tp = *(tp->prototype);
}

/* bounce_template_load - override one template */

void    bounce_template_load(BOUNCE_TEMPLATE *tp, const char *origin,
			             const char *buffer)
{

    /*
     * Clean up after a previous call.
     */
    if (tp->buffer)
	bounce_template_reset(tp);

    /*
     * Postpone the work of template parsing until it is really needed. Most
     * bounce service calls never need a template.
     */
    if (buffer && origin) {
	tp->flags |= BOUNCE_TMPL_FLAG_NEW_BUFFER;
	tp->buffer = mystrdup(buffer);
	tp->origin = mystrdup(origin);
    }
}

/* bounce_template_parse_buffer - initialize template */

static void bounce_template_parse_buffer(BOUNCE_TEMPLATE *tp)
{
    char   *tval = tp->buffer;
    char   *cp;
    char  **cpp;
    int     cpp_len;
    int     cpp_used;
    int     hlen;
    char   *hval;

    /*
     * Sanity check.
     */
    if ((tp->flags & BOUNCE_TMPL_FLAG_NEW_BUFFER) == 0)
	msg_panic("bounce_template_parse_buffer: nothing to do here");
    tp->flags &= ~BOUNCE_TMPL_FLAG_NEW_BUFFER;

    /*
     * Discard the unusable template and use the default one instead.
     */
#define CLEANUP_AND_RETURN() do { \
	bounce_template_reset(tp); \
	return; \
    } while (0)

    /*
     * Parse pseudo-header labels and values.
     */
#define GETLINE(line, buf) \
        (((line) = (buf)) != 0 ? ((buf) = split_at((buf), '\n'), (line)) : 0)

    while ((GETLINE(cp, tval)) != 0 && (hlen = is_header(cp)) > 0) {
	for (hval = cp + hlen; *hval && (*hval == ':' || ISSPACE(*hval)); hval++)
	    *hval = 0;
	if (*hval == 0) {
	    msg_warn("%s: empty \"%s\" header value in %s template "
		     "-- ignoring this template",
		     tp->origin, cp, tp->class);
	    CLEANUP_AND_RETURN();
	}
	if (!allascii(hval)) {
	    msg_warn("%s: non-ASCII \"%s\" header value in %s template "
		     "-- ignoring this template",
		     tp->origin, cp, tp->class);
	    CLEANUP_AND_RETURN();
	}
	if (strcasecmp("charset", cp) == 0) {
	    tp->mime_charset = hval;
	} else if (strcasecmp("from", cp) == 0) {
	    tp->from = hval;
	} else if (strcasecmp("subject", cp) == 0) {
	    tp->subject = hval;
	} else if (strcasecmp("postmaster-subject", cp) == 0) {
	    if (tp->postmaster_subject == 0) {
		msg_warn("%s: inapplicable \"%s\" header label in %s template "
			 "-- ignoring this template",
			 tp->origin, cp, tp->class);
		CLEANUP_AND_RETURN();
	    }
	    tp->postmaster_subject = hval;
	} else {
	    msg_warn("%s: unknown \"%s\" header label in %s template "
		     "-- ignoring this template",
		     tp->origin, cp, tp->class);
	    CLEANUP_AND_RETURN();
	}
    }

    /*
     * Skip blank lines between header and message text.
     */
    while (cp && (*cp == 0 || allspace(cp)))
	(void) GETLINE(cp, tval);
    if (cp == 0) {
	msg_warn("%s: missing message text in %s template "
		 "-- ignoring this template",
		 tp->origin, tp->class);
	CLEANUP_AND_RETURN();
    }

    /*
     * Is this 7bit or 8bit text? If the character set is US-ASCII, then
     * don't allow 8bit text. Don't assume 8bit when charset was changed.
     */
#define NON_ASCII(p) ((p) && *(p) && !allascii((p)))

    if (NON_ASCII(cp) || NON_ASCII(tval)) {
	if (strcasecmp(tp->mime_charset, "us-ascii") == 0) {
	    msg_warn("%s: 8-bit message text in %s template",
		     tp->origin, tp->class);
	    msg_warn("please specify a charset value other than us-ascii");
	    msg_warn("-- ignoring this template for now");
	    CLEANUP_AND_RETURN();
	}
	tp->mime_encoding = MAIL_ATTR_ENC_8BIT;
    }

    /*
     * Collect the message text and null-terminate the result.
     */
    cpp_len = 10;
    cpp_used = 0;
    cpp = (char **) mymalloc(sizeof(*cpp) * cpp_len);
    while (cp) {
	cpp[cpp_used++] = cp;
	if (cpp_used >= cpp_len) {
	    cpp = (char **) myrealloc((char *) cpp,
				      sizeof(*cpp) * 2 * cpp_len);
	    cpp_len *= 2;
	}
	(void) GETLINE(cp, tval);
    }
    cpp[cpp_used] = 0;
    tp->message_text = (const char **) cpp;
}

/* bounce_template_lookup - lookup $name value */

static const char *bounce_template_lookup(const char *key, int unused_mode,
					          char *context)
{
    BOUNCE_TEMPLATE *tp = (BOUNCE_TEMPLATE *) context;
    const BOUNCE_TIME_PARAMETER *bp;
    const BOUNCE_TIME_DIVISOR *bd;
    static VSTRING *buf;
    int     result;

    /*
     * Look for parameter names that can have a time unit suffix, and scale
     * the time value according to the suffix.
     */
    for (bp = time_parameter; bp->param_name; bp++) {
	if (strncmp(key, bp->param_name, bp->param_name_len) == 0
	    && key[bp->param_name_len] == '_') {
	    for (bd = time_divisors; bd->suffix; bd++) {
		if (strcmp(key + bp->param_name_len + 1, bd->suffix) == 0) {
		    result = bp->value[0] / bd->divisor;
		    if (result > 999 && bd->divisor < 86400) {
			msg_warn("%s: excessive result \"%d\" in %s "
				 "template conversion of parameter \"%s\"",
				 tp->origin, result, tp->class, key);
			msg_warn("please increase time unit \"%s\" of \"%s\" "
			      "in %s template", bd->suffix, key, tp->class);
			msg_warn("for instructions see the bounce(5) manual");
		    } else if (result == 0 && bp->value[0] && bd->divisor > 1) {
			msg_warn("%s: zero result in %s template "
				 "conversion of parameter \"%s\"",
				 tp->origin, tp->class, key);
			msg_warn("please reduce time unit \"%s\" of \"%s\" "
			      "in %s template", bd->suffix, key, tp->class);
			msg_warn("for instructions see the bounce(5) manual");
		    }
		    if (buf == 0)
			buf = vstring_alloc(10);
		    vstring_sprintf(buf, "%d", result);
		    return (STR(buf));
		}
	    }
	    msg_fatal("%s: unrecognized suffix \"%s\" in parameter \"%s\"",
		      tp->origin,
		      key + bp->param_name_len + 1, key);
	}
    }
    return (mail_conf_lookup_eval(key));
}

/* bounce_template_headers - send template headers */

void    bounce_template_headers(BOUNCE_XP_PRN_FN out_fn, VSTREAM *fp,
				        BOUNCE_TEMPLATE *tp,
				        const char *rcpt,
				        int postmaster_copy)
{
    if (tp->flags & BOUNCE_TMPL_FLAG_NEW_BUFFER)
	bounce_template_parse_buffer(tp);

    out_fn(fp, "From: %s", tp->from);
    out_fn(fp, "Subject: %s", tp->postmaster_subject && postmaster_copy ?
	   tp->postmaster_subject : tp->subject);
    out_fn(fp, "To: %s", rcpt);
}

/* bounce_template_expand - expand template to stream */

void    bounce_template_expand(BOUNCE_XP_PUT_FN out_fn, VSTREAM *fp,
			               BOUNCE_TEMPLATE *tp)
{
    VSTRING *buf = vstring_alloc(100);
    const char **cpp;
    int     stat;
    const char *filter = "\t !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";

    if (tp->flags & BOUNCE_TMPL_FLAG_NEW_BUFFER)
	bounce_template_parse_buffer(tp);

    for (cpp = tp->message_text; *cpp; cpp++) {
	stat = mac_expand(buf, *cpp, MAC_EXP_FLAG_NONE, filter,
			  bounce_template_lookup, (char *) tp);
	if (stat & MAC_PARSE_ERROR)
	    msg_fatal("%s: bad $name syntax in %s template: %s",
		      tp->origin, tp->class, *cpp);
	if (stat & MAC_PARSE_UNDEF)
	    msg_fatal("%s: undefined $name in %s template: %s",
		      tp->origin, tp->class, *cpp);
	out_fn(fp, STR(buf));
    }
    vstring_free(buf);
}

/* bounce_template_dump - dump template to stream */

void    bounce_template_dump(VSTREAM *fp, BOUNCE_TEMPLATE *tp)
{
    const char **cpp;

    if (tp->flags & BOUNCE_TMPL_FLAG_NEW_BUFFER)
	bounce_template_parse_buffer(tp);

    vstream_fprintf(fp, "Charset: %s\n", tp->mime_charset);
    vstream_fprintf(fp, "From: %s\n", tp->from);
    vstream_fprintf(fp, "Subject: %s\n", tp->subject);
    if (tp->postmaster_subject)
	vstream_fprintf(fp, "Postmaster-Subject: %s\n",
			tp->postmaster_subject);
    vstream_fprintf(fp, "\n");
    for (cpp = tp->message_text; *cpp; cpp++)
	vstream_fprintf(fp, "%s\n", *cpp);
}
