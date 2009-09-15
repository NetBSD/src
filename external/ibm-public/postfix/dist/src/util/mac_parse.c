/*	$NetBSD: mac_parse.c,v 1.1.1.1.2.2 2009/09/15 06:04:00 snj Exp $	*/

/*++
/* NAME
/*	mac_parse 3
/* SUMMARY
/*	locate macro references in string
/* SYNOPSIS
/*	#include <mac_parse.h>
/*
/*	int	mac_parse(string, action, context)
/*	const char *string;
/*	int	(*action)(int type, VSTRING *buf, char *context);
/* DESCRIPTION
/*	This module recognizes macro expressions in null-terminated
/*	strings.  Macro expressions have the form $name, $(text) or
/*	${text}. A macro name consists of alphanumerics and/or
/*	underscore. Text other than macro expressions is treated
/*	as literal text.
/*
/*	mac_parse() breaks up its string argument into macro references
/*	and other text, and invokes the \fIaction\fR routine for each item
/*	found.  With each action routine call, the \fItype\fR argument
/*	indicates what was found, \fIbuf\fR contains a copy of the text
/*	found, and \fIcontext\fR is passed on unmodified from the caller.
/*	The application is at liberty to clobber \fIbuf\fR.
/* .IP MAC_PARSE_LITERAL
/*	The content of \fIbuf\fR is literal text.
/* .IP MAC_PARSE_EXPR
/*	The content of \fIbuf\fR is a macro expression: either a
/*	bare macro name without the preceding "$", or all the text
/*	inside $() or ${}.
/* .PP
/*	The action routine result value is the bit-wise OR of zero or more
/*	of the following:
/* .IP	MAC_PARSE_ERROR
/*	A parsing error was detected.
/* .IP	MAC_PARSE_UNDEF
/*	A macro was expanded but not defined.
/* .PP
/*	Use the constant MAC_PARSE_OK when no error was detected.
/* SEE ALSO
/*	dict(3) dictionary interface.
/* DIAGNOSTICS
/*	Fatal errors: out of memory. malformed macro name.
/*
/*	The result value is the bit-wise OR of zero or more of the
/*	following:
/* .IP	MAC_PARSE_ERROR
/*	A parsing error was detected.
/* .IP	MAC_PARSE_UNDEF
/*	A macro was expanded but not defined.
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

/* Utility library. */

#include <msg.h>
#include <mac_parse.h>

 /*
  * Helper macro for consistency. Null-terminate the temporary buffer,
  * execute the action, and reset the temporary buffer for re-use.
  */
#define MAC_PARSE_ACTION(status, type, buf, context) \
	do { \
	    VSTRING_TERMINATE(buf); \
	    status |= action((type), (buf), (context)); \
	    VSTRING_RESET(buf); \
	} while(0)

/* mac_parse - split string into literal text and macro references */

int     mac_parse(const char *value, MAC_PARSE_FN action, char *context)
{
    const char *myname = "mac_parse";
    VSTRING *buf = vstring_alloc(1);	/* result buffer */
    const char *vp;			/* value pointer */
    const char *pp;			/* open_paren pointer */
    const char *ep;			/* string end pointer */
    static char open_paren[] = "({";
    static char close_paren[] = ")}";
    int     level;
    int     status = 0;

#define SKIP(start, var, cond) \
        for (var = start; *var && (cond); var++);

    if (msg_verbose > 1)
	msg_info("%s: %s", myname, value);

    for (vp = value; *vp;) {
	if (*vp != '$') {			/* ordinary character */
	    VSTRING_ADDCH(buf, *vp);
	    vp += 1;
	} else if (vp[1] == '$') {		/* $$ becomes $ */
	    VSTRING_ADDCH(buf, *vp);
	    vp += 2;
	} else {				/* found bare $ */
	    if (VSTRING_LEN(buf) > 0)
		MAC_PARSE_ACTION(status, MAC_PARSE_LITERAL, buf, context);
	    vp += 1;
	    pp = open_paren;
	    if (*vp == *pp || *vp == *++pp) {	/* ${x} or $(x) */
		level = 1;
		vp += 1;
		for (ep = vp; level > 0; ep++) {
		    if (*ep == 0) {
			msg_warn("truncated macro reference: \"%s\"", value);
			status |= MAC_PARSE_ERROR;
			break;
		    }
		    if (*ep == *pp)
			level++;
		    if (*ep == close_paren[pp - open_paren])
			level--;
		}
		if (status & MAC_PARSE_ERROR)
		    break;
		vstring_strncat(buf, vp, level > 0 ? ep - vp : ep - vp - 1);
		vp = ep;
	    } else {				/* plain $x */
		SKIP(vp, ep, ISALNUM(*ep) || *ep == '_');
		vstring_strncat(buf, vp, ep - vp);
		vp = ep;
	    }
	    if (VSTRING_LEN(buf) == 0) {
		status |= MAC_PARSE_ERROR;
		msg_warn("empty macro name: \"%s\"", value);
		break;
	    }
	    MAC_PARSE_ACTION(status, MAC_PARSE_EXPR, buf, context);
	}
    }
    if (VSTRING_LEN(buf) > 0 && (status & MAC_PARSE_ERROR) == 0)
	MAC_PARSE_ACTION(status, MAC_PARSE_LITERAL, buf, context);

    /*
     * Cleanup.
     */
    vstring_free(buf);

    return (status);
}

#ifdef TEST

 /*
  * Proof-of-concept test program. Read strings from stdin, print parsed
  * result to stdout.
  */
#include <vstring_vstream.h>

/* mac_parse_print - print parse tree */

static int mac_parse_print(int type, VSTRING *buf, char *unused_context)
{
    char   *type_name;

    switch (type) {
    case MAC_PARSE_EXPR:
	type_name = "MAC_PARSE_EXPR";
	break;
    case MAC_PARSE_LITERAL:
	type_name = "MAC_PARSE_LITERAL";
	break;
    default:
	msg_panic("unknown token type %d", type);
    }
    vstream_printf("%s \"%s\"\n", type_name, vstring_str(buf));
    return (0);
}

int     main(int unused_argc, char **unused_argv)
{
    VSTRING *buf = vstring_alloc(1);

    while (vstring_fgets_nonl(buf, VSTREAM_IN)) {
	mac_parse(vstring_str(buf), mac_parse_print, (char *) 0);
	vstream_fflush(VSTREAM_OUT);
    }
    vstring_free(buf);
    return (0);
}

#endif
