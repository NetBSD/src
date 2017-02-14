/*	$NetBSD: attr_override.c,v 1.2 2017/02/14 01:16:45 christos Exp $	*/

/*++
/* NAME
/*	attr_override 3
/* SUMMARY
/*	apply name=value settings from string
/* SYNOPSIS
/*	#include <attr_override.h>
/*
/*	void	attr_override(bp, delimiters, parens, ... CA_ATTR_OVER_END);
/*	char	*bp;
/*	const char *delimiters;
/*	const char *parens;
/* DESCRIPTION
/*	This routine updates the values of known in-memory variables
/*	based on the name=value specifications from an input string.
/*	The input format supports parentheses around name=value to
/*	allow whitespace around "=" and within values.
/*
/*	This may be used, for example, with client endpoint
/*	specifications or with policy tables to allow selective
/*	overrides of global main.cf parameter settings (timeouts,
/*	fall-back policies, etc.).
/*
/*	Arguments:
/* .IP bp
/*	Pointer to input string. The input is modified.
/* .IP "delimiters, parens"
/*	See mystrtok(3) for description. Typical values are
/*	CHARS_COMMA_SP and CHARS_BRACE, respectively.
/* .PP
/*	The parens argument is followed by a list of macros
/*	with arguments. Each macro may appear only once.  The list
/*	must be terminated with CA_ATTR_OVER_END which has no argument.
/*	The following describes the expected values.
/* .IP "CA_ATTR_OVER_STR_TABLE(const ATTR_OVER_STR *)"
/*	The macro argument specifies a null-terminated table with
/*	attribute names, assignment targets, and range limits which
/*	should be the same as for the corresponding main.cf parameters.
/* .IP "CA_ATTR_OVER_TIME_TABLE(const ATTR_OVER_TIME *)"
/*	The macro argument specifies a null-terminated table with
/*	attribute names, their default time units (leading digits
/*	are skipped), assignment targets, and range limits which
/*	should be the same as for the corresponding main.cf parameters.
/* .IP "CA_ATTR_OVER_INT_TABLE(const ATTR_OVER_INT *)"
/*	The macro argument specifies a null-terminated table with
/*	attribute names, assignment targets, and range limits which
/*	should be the same as for the corresponding main.cf parameters.
/* SEE ALSO
/*	mystrtok(3), safe tokenizer
/*	extpar(3), extract text from parentheses
/*	split_nameval(3), name-value splitter
/* DIAGNOSTICS
/*	Panic: interface violations.
/*
/*	Fatal errors: memory allocation problem, syntax error,
/*	out-of-range error.
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

 /*
  * System library.
  */
#include <sys_defs.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>			/* strtol() */

 /*
  * Utility library.
  */
#include <msg.h>
#include <stringops.h>

 /*
  * Global library.
  */
#include <mail_conf.h>
#include <conv_time.h>
#include <attr_override.h>

/* attr_override - apply settings from list of attribute=value pairs */

void    attr_override(char *cp, const char *sep, const char *parens,...)
{
    static const char myname[] = "attr_override";
    va_list ap;
    int     idx;
    char   *nameval;
    const ATTR_OVER_INT *int_table = 0;
    const ATTR_OVER_STR *str_table = 0;
    const ATTR_OVER_TIME *time_table = 0;

    /*
     * Get the lookup tables and assignment targets.
     */
    va_start(ap, parens);
    while ((idx = va_arg(ap, int)) != ATTR_OVER_END) {
	switch (idx) {
	case ATTR_OVER_INT_TABLE:
	    if (int_table)
		msg_panic("%s: multiple ATTR_OVER_INT_TABLE", myname);
	    int_table = va_arg(ap, const ATTR_OVER_INT *);
	    break;
	case ATTR_OVER_STR_TABLE:
	    if (str_table)
		msg_panic("%s: multiple ATTR_OVER_STR_TABLE", myname);
	    str_table = va_arg(ap, const ATTR_OVER_STR *);
	    break;
	case ATTR_OVER_TIME_TABLE:
	    if (time_table)
		msg_panic("%s: multiple ATTR_OVER_TIME_TABLE", myname);
	    time_table = va_arg(ap, const ATTR_OVER_TIME *);
	    break;
	default:
	    msg_panic("%s: unknown argument type: %d", myname, idx);
	}
    }
    va_end(ap);

    /*
     * Process each attribute=value override in the input string.
     */
    while ((nameval = mystrtokq(&cp, sep, parens)) != 0) {
	int     found = 0;
	char   *key;
	char   *value;
	const char *err;
	const ATTR_OVER_INT *ip;
	const ATTR_OVER_STR *sp;
	const ATTR_OVER_TIME *tp;
	int     int_val;
	int     def_unit;
	char   *end;
	long    longval;

	/*
	 * Split into name and value.
	 */
	/* { name = value } */
	if (*nameval == parens[0]
	    && (err = extpar(&nameval, parens, EXTPAR_FLAG_NONE)) != 0)
	    msg_fatal("%s in \"%s\"", err, nameval);
	if ((err = split_nameval(nameval, &key, &value)) != 0)
	    msg_fatal("malformed option: %s: \"...%s...\"", err, nameval);

	/*
	 * Look up the name and apply the value.
	 */
	for (sp = str_table; sp != 0 && found == 0 && sp->name != 0; sp++) {
	    if (strcmp(sp->name, key) != 0)
		continue;
	    check_mail_conf_str(sp->name, value, sp->min, sp->max);
	    sp->target[0] = value;
	    found = 1;
	}
	for (ip = int_table; ip != 0 && found == 0 && ip->name != 0; ip++) {
	    if (strcmp(ip->name, key) != 0)
		continue;
	    /* XXX Duplicated from mail_conf_int(3). */
	    errno = 0;
	    int_val = longval = strtol(value, &end, 10);
	    if (*value == 0 || *end != 0 || errno == ERANGE
		|| longval != int_val)
		msg_fatal("bad numerical configuration: %s = %s", key, value);
	    check_mail_conf_int(key, int_val, ip->min, ip->max);
	    ip->target[0] = int_val;
	    found = 1;
	}
	for (tp = time_table; tp != 0 && found == 0 && tp->name != 0; tp++) {
	    if (strcmp(tp->name, key) != 0)
		continue;
	    def_unit = tp->defval[strspn(tp->defval, "0123456789")];
	    if (conv_time(value, &int_val, def_unit) == 0)
		msg_fatal("%s: bad time value or unit: %s", key, value);
	    check_mail_conf_time(key, int_val, tp->min, tp->max);
	    tp->target[0] = int_val;
	    found = 1;
	}
	if (found == 0)
	    msg_fatal("unknown option: \"%s = %s\"", key, value);
    }
}
