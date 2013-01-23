/*	$NetBSD: mac_expand.c,v 1.1.1.2.4.1 2013/01/23 00:05:16 yamt Exp $	*/

/*++
/* NAME
/*	mac_expand 3
/* SUMMARY
/*	attribute expansion
/* SYNOPSIS
/*	#include <mac_expand.h>
/*
/*	int	mac_expand(result, pattern, flags, filter, lookup, context)
/*	VSTRING *result;
/*	const char *pattern;
/*	int	flags;
/*	const char *filter;
/*	const char *lookup(const char *key, int mode, char *context)
/*	char	*context;
/* DESCRIPTION
/*	This module implements parameter-less macro expansions, both
/*	conditional and unconditional, and both recursive and non-recursive.
/*
/*	In this text, an attribute is considered "undefined" when its value
/*	is a null pointer.  Otherwise, the attribute is considered "defined"
/*	and is expected to have as value a null-terminated string.
/*
/*	The following expansions are implemented:
/* .IP "$name, ${name}, $(name)"
/*	Unconditional expansion. If the named attribute value is non-empty, the
/*	expansion is the value of the named attribute,  optionally subjected
/*	to further $name expansions.  Otherwise, the expansion is empty.
/* .IP "${name?text}, $(name?text)"
/*	Conditional expansion. If the named attribute value is non-empty, the
/*	expansion is the given text, subjected to another iteration of
/*	$name expansion.  Otherwise, the expansion is empty.
/* .IP "${name:text}, $(name:text)"
/*	Conditional expansion. If the attribute value is empty or undefined,
/*	the expansion is the given text, subjected to another iteration
/*	of $name expansion.  Otherwise, the expansion is empty.
/* .PP
/*	Arguments:
/* .IP result
/*	Storage for the result of expansion. The result is truncated
/*	upon entry.
/* .IP pattern
/*	The string to be expanded.
/* .IP flags
/*	Bit-wise OR of zero or more of the following:
/* .RS
/* .IP MAC_EXP_FLAG_RECURSE
/*	Expand macros in lookup results. This should never be done with
/*	data whose origin is untrusted.
/* .IP MAC_EXP_FLAG_APPEND
/*	Append text to the result buffer without truncating it.
/* .IP MAC_EXP_FLAG_SCAN
/*	Invoke the call-back function each macro name in the input
/*	string, including macro names in the values of conditional
/*	expressions.  Do not expand macros, and do not write to the
/*	result argument.
/* .PP
/*	The constant MAC_EXP_FLAG_NONE specifies a manifest null value.
/* .RE
/* .IP filter
/*	A null pointer, or a null-terminated array of characters that
/*	are allowed to appear in an expansion. Illegal characters are
/*	replaced by underscores.
/* .IP lookup
/*	The attribute lookup routine. Arguments are: the attribute name,
/*	MAC_EXP_MODE_TEST to test the existence of the named attribute
/*	or MAC_EXP_MODE_USE to use the value of the named attribute,
/*	and the caller context that was given to mac_expand(). A null
/*	result value means that the requested attribute was not defined.
/* .IP context
/*	Caller context that is passed on to the attribute lookup routine.
/* DIAGNOSTICS
/*	Fatal errors: out of memory.  Warnings: syntax errors, unreasonable
/*	macro nesting.
/*
/*	The result value is the binary OR of zero or more of the following:
/* .IP MAC_PARSE_ERROR
/*	A syntax error was found in \fBpattern\fR, or some macro had
/*	an unreasonable nesting depth.
/* .IP MAC_PARSE_UNDEF
/*	A macro was expanded but its value not defined.
/* SEE ALSO
/*	mac_parse(3) locate macro references in string.
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
#include <vstring.h>
#include <mymalloc.h>
#include <mac_parse.h>
#include <mac_expand.h>

 /*
  * Little helper structure.
  */
typedef struct {
    VSTRING *result;			/* result buffer */
    int     flags;			/* features */
    const char *filter;			/* character filter */
    MAC_EXP_LOOKUP_FN lookup;		/* lookup routine */
    char   *context;			/* caller context */
    int     status;			/* findings */
    int     level;			/* nesting level */
} MAC_EXP;

/* mac_expand_callback - callback for mac_parse */

static int mac_expand_callback(int type, VSTRING *buf, char *ptr)
{
    MAC_EXP *mc = (MAC_EXP *) ptr;
    int     lookup_mode;
    const char *text;
    char   *cp;
    int     ch;
    ssize_t len;

    /*
     * Sanity check.
     */
    if (mc->level++ > 100) {
	msg_warn("unreasonable macro call nesting: \"%s\"", vstring_str(buf));
	mc->status |= MAC_PARSE_ERROR;
    }
    if (mc->status & MAC_PARSE_ERROR)
	return (mc->status);

    /*
     * $Name etc. reference.
     * 
     * In order to support expansion of lookup results, we must save the lookup
     * result. We use the input buffer since it will not be needed anymore.
     */
    if (type == MAC_PARSE_EXPR) {

	/*
	 * Look for the ? or : delimiter. In case of a syntax error, return
	 * without doing damage, and issue a warning instead.
	 */
	for (cp = vstring_str(buf); /* void */ ; cp++) {
	    if ((ch = *cp) == 0) {
		lookup_mode = MAC_EXP_MODE_USE;
		break;
	    }
	    if (ch == '?' || ch == ':') {
		*cp++ = 0;
		lookup_mode = MAC_EXP_MODE_TEST;
		break;
	    }
	    if (!ISALNUM(ch) && ch != '_') {
		msg_warn("macro name syntax error: \"%s\"", vstring_str(buf));
		mc->status |= MAC_PARSE_ERROR;
		return (mc->status);
	    }
	}

	/*
	 * Look up the named parameter.
	 */
	text = mc->lookup(vstring_str(buf), lookup_mode, mc->context);

	/*
	 * Perform the requested substitution.
	 */
	switch (ch) {
	case '?':
	    if ((text != 0 && *text != 0) || (mc->flags & MAC_EXP_FLAG_SCAN))
		mac_parse(cp, mac_expand_callback, (char *) mc);
	    break;
	case ':':
	    if (text == 0 || *text == 0 || (mc->flags & MAC_EXP_FLAG_SCAN))
		mac_parse(cp, mac_expand_callback, (char *) mc);
	    break;
	default:
	    if (text == 0) {
		mc->status |= MAC_PARSE_UNDEF;
	    } else if (*text == 0 || (mc->flags & MAC_EXP_FLAG_SCAN)) {
		 /* void */ ;
	    } else if (mc->flags & MAC_EXP_FLAG_RECURSE) {
		vstring_strcpy(buf, text);
		mac_parse(vstring_str(buf), mac_expand_callback, (char *) mc);
	    } else {
		len = VSTRING_LEN(mc->result);
		vstring_strcat(mc->result, text);
		if (mc->filter) {
		    cp = vstring_str(mc->result) + len;
		    while (*(cp += strspn(cp, mc->filter)))
			*cp++ = '_';
		}
	    }
	    break;
	}
    }

    /*
     * Literal text.
     */
    else if ((mc->flags & MAC_EXP_FLAG_SCAN) == 0) {
	vstring_strcat(mc->result, vstring_str(buf));
    }

    mc->level--;

    return (mc->status);
}

/* mac_expand - expand $name instances */

int     mac_expand(VSTRING *result, const char *pattern, int flags,
		           const char *filter,
		           MAC_EXP_LOOKUP_FN lookup, char *context)
{
    MAC_EXP mc;
    int     status;

    /*
     * Bundle up the request and do the substitutions.
     */
    mc.result = result;
    mc.flags = flags;
    mc.filter = filter;
    mc.lookup = lookup;
    mc.context = context;
    mc.status = 0;
    mc.level = 0;
    if ((flags & (MAC_EXP_FLAG_APPEND | MAC_EXP_FLAG_SCAN)) == 0)
	VSTRING_RESET(result);
    status = mac_parse(pattern, mac_expand_callback, (char *) &mc);
    if ((flags & MAC_EXP_FLAG_SCAN) == 0)
	VSTRING_TERMINATE(result);

    return (status);
}

#ifdef TEST

 /*
  * This code certainly deserves a stand-alone test program.
  */
#include <stdlib.h>
#include <stringops.h>
#include <htable.h>
#include <vstream.h>
#include <vstring_vstream.h>

static const char *lookup(const char *name, int unused_mode, char *context)
{
    HTABLE *table = (HTABLE *) context;

    return (htable_find(table, name));
}

int     main(int unused_argc, char **unused_argv)
{
    VSTRING *buf = vstring_alloc(100);
    VSTRING *result = vstring_alloc(100);
    char   *cp;
    char   *name;
    char   *value;
    HTABLE *table;
    int     stat;

    while (!vstream_feof(VSTREAM_IN)) {

	table = htable_create(0);

	/*
	 * Read a block of definitions, terminated with an empty line.
	 */
	while (vstring_get_nonl(buf, VSTREAM_IN) != VSTREAM_EOF) {
	    vstream_printf("<< %s\n", vstring_str(buf));
	    vstream_fflush(VSTREAM_OUT);
	    if (VSTRING_LEN(buf) == 0)
		break;
	    cp = vstring_str(buf);
	    name = mystrtok(&cp, " \t\r\n=");
	    value = mystrtok(&cp, " \t\r\n=");
	    htable_enter(table, name, value ? mystrdup(value) : 0);
	}

	/*
	 * Read a block of patterns, terminated with an empty line or EOF.
	 */
	while (vstring_get_nonl(buf, VSTREAM_IN) != VSTREAM_EOF) {
	    vstream_printf("<< %s\n", vstring_str(buf));
	    vstream_fflush(VSTREAM_OUT);
	    if (VSTRING_LEN(buf) == 0)
		break;
	    cp = vstring_str(buf);
	    VSTRING_RESET(result);
	    stat = mac_expand(result, vstring_str(buf), MAC_EXP_FLAG_NONE,
			      (char *) 0, lookup, (char *) table);
	    vstream_printf("stat=%d result=%s\n", stat, vstring_str(result));
	    vstream_fflush(VSTREAM_OUT);
	}
	htable_free(table, myfree);
	vstream_printf("\n");
    }

    /*
     * Clean up.
     */
    vstring_free(buf);
    vstring_free(result);
    exit(0);
}

#endif
