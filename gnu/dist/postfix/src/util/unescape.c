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
/* DESCRIPTION
/*	unescape() translates C-like escape sequences in the null-terminated
/*	string \fIinput\fR and places the result in \fIresult\fR. The result
/*	is null-terminated.
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

#ifdef TEST

#include <vstring_vstream.h>

int     main(int unused_argc, char **unused_argv)
{
    VSTRING *in = vstring_alloc(10);
    VSTRING *out = vstring_alloc(10);

    while (vstring_fgets_nonl(in, VSTREAM_IN)) {
	unescape(out, vstring_str(in));
	vstream_fwrite(VSTREAM_OUT, vstring_str(out), VSTRING_LEN(out));
    }
    vstream_fflush(VSTREAM_OUT);
    exit(0);
}

#endif
