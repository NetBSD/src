/*	$NetBSD: unescape.c,v 1.1.1.1 2009/06/23 10:09:01 tron Exp $	*/

/*++
/* NAME
/*	unescape 3
/* SUMMARY
/*	translate C-like escape sequences
/* SYNOPSIS
/*	#include <stringops.h>
/*
/*	VSTRING	*unescape(result, input)
/*	VSTRING	*result;
/*	const char *input;
/*
/*	VSTRING	*escape(result, input, len)
/*	VSTRING	*result;
/*	const char *input;
/*	ssize_t len;
/* DESCRIPTION
/*	unescape() translates C-like escape sequences in the null-terminated
/*	string \fIinput\fR and places the result in \fIresult\fR. The result
/*	is null-terminated, and is the function result value.
/*
/*	escape() does the reverse transformation.
/*
/*	Escape sequences and their translations:
/* .IP \ea
/*	Bell character.
/* .IP \eb
/*	Backspace character.
/* .IP \ef
/*	formfeed character.
/* .IP \en
/*	newline character
/* .IP \er
/*	Carriage-return character.
/* .IP \et
/*	Horizontal tab character.
/* .IP \ev
/*	Vertical tab character.
/* .IP \e\e
/*	Backslash character.
/* .IP \e\fInum\fR
/*	8-bit character whose ASCII value is the 1..3 digit
/*	octal number \fInum\fR.
/* .IP \e\fIother\fR
/*	The backslash character is discarded.
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

#include <vstring.h>
#include <stringops.h>

/* unescape - process escape sequences */

VSTRING *unescape(VSTRING *result, const char *data)
{
    int     ch;
    int     oval;
    int     i;

#define UCHAR(cp)	((unsigned char *) (cp))
#define ISOCTAL(ch)	(ISDIGIT(ch) && (ch) != '8' && (ch) != '9')

    VSTRING_RESET(result);

    while ((ch = *UCHAR(data++)) != 0) {
	if (ch == '\\') {
	    if ((ch = *UCHAR(data++)) == 0)
		break;
	    switch (ch) {
	    case 'a':				/* \a -> audible bell */
		ch = '\a';
		break;
	    case 'b':				/* \b -> backspace */
		ch = '\b';
		break;
	    case 'f':				/* \f -> formfeed */
		ch = '\f';
		break;
	    case 'n':				/* \n -> newline */
		ch = '\n';
		break;
	    case 'r':				/* \r -> carriagereturn */
		ch = '\r';
		break;
	    case 't':				/* \t -> horizontal tab */
		ch = '\t';
		break;
	    case 'v':				/* \v -> vertical tab */
		ch = '\v';
		break;
	    case '0':				/* \nnn -> ASCII value */
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
		for (oval = ch - '0', i = 0;
		     i < 2 && (ch = *UCHAR(data)) != 0 && ISOCTAL(ch);
		     i++, data++) {
		    oval = (oval << 3) | (ch - '0');
		}
		ch = oval;
		break;
	    default:				/* \any -> any */
		break;
	    }
	}
	VSTRING_ADDCH(result, ch);
    }
    VSTRING_TERMINATE(result);
    return (result);
}

/* escape - reverse transformation */

VSTRING *escape(VSTRING *result, const char *data, ssize_t len)
{
    int     ch;

    VSTRING_RESET(result);
    while (len-- > 0) {
	ch = *UCHAR(data++);
	if (ISASCII(ch)) {
	    if (ISPRINT(ch)) {
		if (ch == '\\')
		    VSTRING_ADDCH(result, ch);
		VSTRING_ADDCH(result, ch);
		continue;
	    } else if (ch == '\a') {		/* \a -> audible bell */
		vstring_strcat(result, "\\a");
		continue;
	    } else if (ch == '\b') {		/* \b -> backspace */
		vstring_strcat(result, "\\b");
		continue;
	    } else if (ch == '\f') {		/* \f -> formfeed */
		vstring_strcat(result, "\\f");
		continue;
	    } else if (ch == '\n') {		/* \n -> newline */
		vstring_strcat(result, "\\n");
		continue;
	    } else if (ch == '\r') {		/* \r -> carriagereturn */
		vstring_strcat(result, "\\r");
		continue;
	    } else if (ch == '\t') {		/* \t -> horizontal tab */
		vstring_strcat(result, "\\t");
		continue;
	    } else if (ch == '\v') {		/* \v -> vertical tab */
		vstring_strcat(result, "\\v");
		continue;
	    }
	}
	if (ISDIGIT(*UCHAR(data)))
	    vstring_sprintf_append(result, "\\%03d", ch);
	else
	    vstring_sprintf_append(result, "\\%d", ch);
    }
    VSTRING_TERMINATE(result);
    return (result);
}

#ifdef TEST

#include <stdlib.h>
#include <string.h>
#include <msg.h>
#include <vstring_vstream.h>

int     main(int argc, char **argv)
{
    VSTRING *in = vstring_alloc(10);
    VSTRING *out = vstring_alloc(10);
    int     un_escape = 1;

    if (argc > 2 || (argc > 1 && (un_escape = strcmp(argv[1], "-e"))) != 0)
	msg_fatal("usage: %s [-e (escape)]", argv[0]);

    if (un_escape) {
	while (vstring_fgets_nonl(in, VSTREAM_IN)) {
	    unescape(out, vstring_str(in));
	    vstream_fwrite(VSTREAM_OUT, vstring_str(out), VSTRING_LEN(out));
	}
    } else {
	while (vstring_fgets(in, VSTREAM_IN)) {
	    escape(out, vstring_str(in), VSTRING_LEN(in));
	    vstream_fwrite(VSTREAM_OUT, vstring_str(out), VSTRING_LEN(out));
	}
    }
    vstream_fflush(VSTREAM_OUT);
    exit(0);
}

#endif
