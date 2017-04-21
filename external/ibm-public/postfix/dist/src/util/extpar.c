/*	$NetBSD: extpar.c,v 1.2.4.2 2017/04/21 16:52:53 bouyer Exp $	*/

/*++
/* NAME
/*	extpar 3
/* SUMMARY
/*	extract text from parentheses
/* SYNOPSIS
/*	#include <stringops.h>
/*
/*	char	*extpar(bp, parens, flags)
/*	char	**bp;
/*	const char *parens;
/*	int	flags;
/* DESCRIPTION
/*	extpar() extracts text from an input string that is enclosed
/*	in the specified parentheses, and updates the buffer pointer
/*	to point to that text.
/*
/*	Arguments:
/* .IP bp
/*	Pointer to buffer pointer. Both the buffer and the buffer
/*	pointer are modified.
/* .IP parens
/*	One matching pair of parentheses, opening parenthesis first.
/* .IP flags
/*	EXTPAR_FLAG_NONE, or the bitwise OR of one or more flags:
/* .RS
/* .IP EXTPAR_FLAG_EXTRACT
/*	This flag is intended to instruct expar() callers that
/*	expar() should be invoked. It has no effect on expar()
/*	itself.
/* .IP EXTPAR_FLAG_STRIP
/*	Skip whitespace after the opening parenthesis, and trim
/*	whitespace before the closing parenthesis.
/* .RE
/* DIAGNOSTICS
/*	panic: the input string does not start with the opening
/*	parenthesis.
/*
/*	In case of error the result value is a dynamically-allocated
/*	string with a description of the problem that includes a
/*	copy of the offending input.  A non-null result value should
/*	be destroyed with myfree(). The following decribes the errors
/*	and the state of the buffer and buffer pointer.
/* .IP "missing closing parenthesis"
/*	The buffer pointer points to text as if a closing parenthesis
/*	were present at the end of the input.
/* .IP "text after closing parenthesis"
/*	The buffer pointer points to text as if the offending text
/*	were not present.
/* SEE ALSO
/*	balpar(3) determine length of string in parentheses
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
#include <ctype.h>

 /*
  * Utility library.
  */
#include <msg.h>
#include <stringops.h>

/* extpar - extract text from parentheses */

char   *extpar(char **bp, const char *parens, int flags)
{
    char   *cp = *bp;
    char   *err = 0;
    size_t  len;

    if (cp[0] != parens[0])
	msg_panic("extpar: no '%c' at start of text: \"%s\"", parens[0], cp);
    if ((len = balpar(cp, parens)) == 0) {
	err = concatenate("missing '", parens + 1, "' in \"",
			  cp, "\"", (char *) 0);
	cp += 1;
    } else {
	if (cp[len] != 0)
	    err = concatenate("syntax error after '", parens + 1, "' in \"",
			      cp, "\"", (char *) 0);
	cp += 1;
	cp[len -= 2] = 0;
    }
    if (flags & EXTPAR_FLAG_STRIP) {
	trimblanks(cp, len)[0] = 0;
	while (ISSPACE(*cp))
	    cp++;
    }
    *bp = cp;
    return (err);
}
