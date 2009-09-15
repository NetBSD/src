/*	$NetBSD: translit.c,v 1.1.1.1.2.2 2009/09/15 06:04:04 snj Exp $	*/

/*++
/* NAME
/*	translit 3
/* SUMMARY
/*	transliterate characters
/* SYNOPSIS
/*	#include <stringops.h>
/*
/*	char	*translit(buf, original, replacement)
/*	char	*buf;
/*	char	*original;
/*	char	*replacement;
/* DESCRIPTION
/*	translit() takes a null-terminated string, and replaces characters
/*	given in its \fIoriginal\fR argument by the corresponding characters
/*	in the \fIreplacement\fR string. The result value is the \fIbuf\fR
/*	argument.
/* BUGS
/*	Cannot replace null characters.
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

#include "sys_defs.h"
#include <string.h>

/* Utility library. */

#include "stringops.h"

char   *translit(char *string, const char *original, const char *replacement)
{
    char   *cp;
    const char *op;

    /*
     * For large inputs, should use a lookup table.
     */
    for (cp = string; *cp != 0; cp++) {
	for (op = original; *op != 0; op++) {
	    if (*cp == *op) {
		*cp = replacement[op - original];
		break;
	    }
	}
    }
    return (string);
}

#ifdef TEST

 /*
  * Usage: translit string1 string2
  * 
  * test program to perform the most basic operation of the UNIX tr command.
  */
#include <msg.h>
#include <vstring.h>
#include <vstream.h>
#include <vstring_vstream.h>

#define STR	vstring_str

int     main(int argc, char **argv)
{
    VSTRING *buf = vstring_alloc(100);

    if (argc != 3)
	msg_fatal("usage: %s string1 string2", argv[0]);
    while (vstring_fgets(buf, VSTREAM_IN))
	vstream_fputs(translit(STR(buf), argv[1], argv[2]), VSTREAM_OUT);
    vstream_fflush(VSTREAM_OUT);
    vstring_free(buf);
    return (0);
}

#endif
