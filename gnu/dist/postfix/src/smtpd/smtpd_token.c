/*++
/* NAME
/*	smtpd_token 3
/* SUMMARY
/*	tokenize SMTPD command
/* SYNOPSIS
/*	#include <smtpd_token.h>
/*
/*	typedef struct {
/* .in +4
/*	    int tokval;
/*	    char *strval;
/*	    /* other stuff... */
/* .in -4
/*	} SMTPD_TOKEN;
/*
/*	int	smtpd_token(str, argvp)
/*	char	*str;
/*	SMTPD_TOKEN **argvp;
/* DESCRIPTION
/*	smtpd_token() routine converts the string in \fIstr\fR to an
/*	array of tokens in \fIargvp\fR. The number of tokens is returned
/*	via the function return value.
/*
/*	Token types:
/* .IP SMTPD_TOK_OTHER
/*	The token is something else.
/* .IP SMTPD_TOK_ERROR
/*	A malformed token.
/* BUGS
/*	This tokenizer understands just enough to tokenize SMTPD commands.
/*	It understands backslash escapes, white space, quoted strings,
/*	and addresses (including quoted text) enclosed by < and >.
/*	The input is broken up into tokens by whitespace, except for
/*	whitespace that is protected by quotes etc.
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
#include <unistd.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <mymalloc.h>
#include <mvect.h>

/* Application-specific. */

#include "smtpd_token.h"

/* smtp_quoted - read until closing quote */

static char *smtp_quoted(char *cp, SMTPD_TOKEN *arg, int start, int last)
{
    static VSTRING *stack;
    int     wanted;
    int     c;

    /*
     * Parser stack. `ch' is always the most-recently entered character.
     */
#define ENTER_CHAR(buf, ch) VSTRING_ADDCH(buf, ch);
#define LEAVE_CHAR(buf, ch) { \
	vstring_truncate(buf, VSTRING_LEN(buf) - 1); \
	ch = vstring_end(buf)[-1]; \
    }

    if (stack == 0)
	stack = vstring_alloc(1);
    VSTRING_RESET(stack);
    ENTER_CHAR(stack, wanted = last);

    VSTRING_ADDCH(arg->vstrval, start);
    for (;;) {
	if ((c = *cp) == 0)
	    break;
	cp++;
	VSTRING_ADDCH(arg->vstrval, c);
	if (c == '\\') {			/* parse escape sequence */
	    if ((c = *cp) == 0)
		break;
	    cp++;
	    VSTRING_ADDCH(arg->vstrval, c);
	} else if (c == wanted) {		/* closing quote etc. */
	    if (VSTRING_LEN(stack) == 1)
		return (cp);
	    LEAVE_CHAR(stack, wanted);
	} else if (c == '"') {
	    ENTER_CHAR(stack, wanted = '"');	/* highest precedence */
	} else if (c == '<' && wanted == '>') {
	    ENTER_CHAR(stack, wanted = '>');	/* lowest precedence */
	}
    }
    arg->tokval = SMTPD_TOK_ERROR;		/* missing end */
    return (cp);
}

/* smtp_next_token - extract next token from input, update cp */

static char *smtp_next_token(char *cp, SMTPD_TOKEN *arg)
{
    int     c;

    VSTRING_RESET(arg->vstrval);
    arg->tokval = SMTPD_TOK_OTHER;

#define STR(x) vstring_str(x)
#define LEN(x) VSTRING_LEN(x)
#define STREQ(x,y,l) (strncasecmp((x), (y), (l)) == 0)

    for (;;) {
	if ((c = *cp) == 0)			/* end of input */
	    break;
	cp++;
	if (ISSPACE(c)) {			/* whitespace, skip */
	    while (*cp && ISSPACE(*cp))
		cp++;
	    if (LEN(arg->vstrval) > 0)		/* end of token */
		break;
	} else if (c == '<') {			/* <stuff> */
	    cp = smtp_quoted(cp, arg, c, '>');
	} else if (c == '"') {			/* "stuff" */
	    cp = smtp_quoted(cp, arg, c, c);
	} else if (c == ':') {			/* this is gross, but... */
	    VSTRING_ADDCH(arg->vstrval, c);
	    if (STREQ(STR(arg->vstrval), "to:", LEN(arg->vstrval))
		|| STREQ(STR(arg->vstrval), "from:", LEN(arg->vstrval)))
		break;
	} else {				/* other */
	    if (c == '\\') {
		VSTRING_ADDCH(arg->vstrval, c);
		if ((c = *cp) == 0)
		    break;
		cp++;
	    }
	    VSTRING_ADDCH(arg->vstrval, c);
	}
    }
    if (LEN(arg->vstrval) <= 0)			/* no token found */
	return (0);
    VSTRING_TERMINATE(arg->vstrval);
    arg->strval = vstring_str(arg->vstrval);
    return (cp);
}

/* smtpd_token_init - initialize token structures */

static void smtpd_token_init(char *ptr, int count)
{
    SMTPD_TOKEN *arg;
    int     n;

    for (arg = (SMTPD_TOKEN *) ptr, n = 0; n < count; arg++, n++)
	arg->vstrval = vstring_alloc(10);
}

/* smtpd_token - tokenize SMTPD command */

int     smtpd_token(char *cp, SMTPD_TOKEN **argvp)
{
    static SMTPD_TOKEN *smtp_argv;
    static MVECT mvect;
    int     n;

    if (smtp_argv == 0)
	smtp_argv = (SMTPD_TOKEN *) mvect_alloc(&mvect, sizeof(*smtp_argv), 1,
					    smtpd_token_init, (MVECT_FN) 0);
    for (n = 0; /* void */ ; n++) {
	smtp_argv = (SMTPD_TOKEN *) mvect_realloc(&mvect, n + 1);
	if ((cp = smtp_next_token(cp, smtp_argv + n)) == 0)
	    break;
    }
    *argvp = smtp_argv;
    return (n);
}

#ifdef TEST

 /*
  * Test program for the SMTPD command tokenizer.
  */

#include <stdlib.h>
#include <vstream.h>
#include <vstring_vstream.h>

int     main(int unused_argc, char **unused_argv)
{
    VSTRING *vp = vstring_alloc(10);
    int     tok_argc;
    SMTPD_TOKEN *tok_argv;
    int     i;

    for (;;) {
	if (isatty(STDIN_FILENO))
	    vstream_printf("enter SMTPD command: ");
	vstream_fflush(VSTREAM_OUT);
	if (vstring_get_nonl(vp, VSTREAM_IN) == VSTREAM_EOF)
	    break;
	if (*vstring_str(vp) == '#')
	    continue;
	if (!isatty(STDIN_FILENO))
	    vstream_printf("%s\n", vstring_str(vp));
	tok_argc = smtpd_token(vstring_str(vp), &tok_argv);
	for (i = 0; i < tok_argc; i++) {
	    vstream_printf("Token type:  %s\n",
			   tok_argv[i].tokval == SMTPD_TOK_OTHER ? "other" :
			   tok_argv[i].tokval == SMTPD_TOK_ERROR ? "error" :
			   "unknown");
	    vstream_printf("Token value: %s\n", tok_argv[i].strval);
	}
    }
    vstring_free(vp);
    exit(0);
}

#endif
