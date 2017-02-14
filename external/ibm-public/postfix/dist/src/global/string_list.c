/*	$NetBSD: string_list.c,v 1.2 2017/02/14 01:16:45 christos Exp $	*/

/*++
/* NAME
/*	string_list 3
/* SUMMARY
/*	match a string against a pattern list
/* SYNOPSIS
/*	#include <string_list.h>
/*
/*	STRING_LIST *string_list_init(pname, flags, pattern_list)
/*	const char *pname;
/*	int	flags;
/*	const char *pattern_list;
/*
/*	int	string_list_match(list, name)
/*	STRING_LIST *list;
/*	const char *name;
/*
/*	void string_list_free(list)
/*	STRING_LIST *list;
/* DESCRIPTION
/*	This is a convenience wrapper around the match_list module.
/*
/*	This module implements tests for list membership of a string.
/*
/*	Patterns are separated by whitespace and/or commas. A pattern
/*	is either a string, a file name (in which case the contents
/*	of the file are substituted for the file name) or a type:name
/*	lookup table specification.
/*
/*	A string matches a string list when it appears in the list of
/*	string patterns. The matching process is case insensitive.
/*	In order to reverse the result, precede a pattern with an
/*	exclamation point (!).
/*
/*	string_list_init() performs initializations. The pname
/*	argument specifies error reporting context. The flags argument
/*	is a bit-wise OR of zero or more of following:
/* .IP MATCH_FLAG_RETURN
/*	Request that string_list_match() logs a warning and returns
/*	zero with list->error set to a non-zero dictionary error
/*	code, instead of raising a fatal error.
/* .PP
/*	Specify MATCH_FLAG_NONE to request none of the above.
/*	The last argument specifies a list of string patterns.
/*
/*	string_list_match() matches the specified string against the
/*	compiled pattern list.
/*
/*	string_list_free() releases storage allocated by string_list_init().
/* DIAGNOSTICS
/*	Fatal error: unable to open or read a pattern file or table.
/* SEE ALSO
/*	match_list(3) generic list matching
/*	match_ops(3) match strings by name or by address
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

/* Utility library. */

#include <match_list.h>

/* Global library. */

#include "string_list.h"

#ifdef TEST

#include <stdlib.h>
#include <unistd.h>
#include <msg.h>
#include <vstream.h>
#include <vstring.h>
#include <msg_vstream.h>
#include <dict.h>
#include <stringops.h>			/* util_utf8_enable */

static void usage(char *progname)
{
    msg_fatal("usage: %s [-v] patterns string", progname);
}

int     main(int argc, char **argv)
{
    STRING_LIST *list;
    char   *string;
    int     ch;

    msg_vstream_init(argv[0], VSTREAM_ERR);

    while ((ch = GETOPT(argc, argv, "v")) > 0) {
	switch (ch) {
	case 'v':
	    msg_verbose++;
	    break;
	default:
	    usage(argv[0]);
	}
    }
    if (argc != optind + 2)
	usage(argv[0]);
    dict_allow_surrogate = 1;
    util_utf8_enable = 1;
    list = string_list_init("command line", MATCH_FLAG_RETURN, argv[optind]);
    string = argv[optind + 1];
    vstream_printf("%s: %s\n", string, string_list_match(list, string) ?
		   "YES" : list->error == 0 ? "NO" : "ERROR");
    vstream_fflush(VSTREAM_OUT);
    string_list_free(list);
    return (0);
}

#endif
