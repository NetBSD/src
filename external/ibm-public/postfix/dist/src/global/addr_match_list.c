/*	$NetBSD: addr_match_list.c,v 1.2 2017/02/14 01:16:45 christos Exp $	*/

/*++
/* NAME
/*	addr_match_list 3
/* SUMMARY
/*	address list membership
/* SYNOPSIS
/*	#include <addr_match_list.h>
/*
/*	ADDR_MATCH_LIST *addr_match_list_init(pname, flags, pattern_list)
/*	const char *pname;
/*	int	flags;
/*	const char *pattern_list;
/*
/*	int	addr_match_list_match(list, addr)
/*	ADDR_MATCH_LIST *list;
/*	const char *addr;
/*
/*	void	addr_match_list_free(list)
/*	ADDR_MATCH_LIST *list;
/* DESCRIPTION
/*	This is a convenience wrapper around the match_list module.
/*
/*	This module implements tests for list membership of a
/*	network address.
/*
/*	A list pattern specifies an internet address, or a network/mask
/*	pattern, where the mask specifies the number of bits in the
/*	network part.  When a pattern specifies a file name, its
/*	contents are substituted for the file name; when a pattern
/*	is a type:name table specification, table lookup is used
/*	instead.  Patterns are separated by whitespace and/or commas.
/*	In order to reverse the result, precede a pattern with an
/*	exclamation point (!).
/*
/*	A host matches a list when its address matches a pattern.
/*	The matching process is case insensitive.
/*
/*	addr_match_list_init() performs initializations. The pname
/*	argument specifies error reporting context. The flags
/*	argument is the bit-wise OR of zero or more of the following:
/* .IP MATCH_FLAG_RETURN
/*	Request that addr_match_list_match() logs a warning and
/*	returns zero with list->error set to a non-zero dictionary
/*	error code, instead of raising a fatal error.
/* .PP
/*	Specify MATCH_FLAG_NONE to request none of the above.
/*	The last argument is a list of patterns, or the absolute
/*	pathname of a file with patterns.
/*
/*	addr_match_list_match() matches the specified host address
/*	against the specified list of patterns.
/*
/*	addr_match_list_free() releases storage allocated by
/*	addr_match_list_init().
/* DIAGNOSTICS
/*	Fatal errors: unable to open or read a pattern file; invalid
/*	pattern. Panic: interface violations.
/* SEE ALSO
/*	match_list(3) generic list matching
/*	match_ops(3) match host by name or by address
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

#include "addr_match_list.h"

#ifdef TEST

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <msg.h>
#include <vstream.h>
#include <vstring_vstream.h>
#include <msg_vstream.h>
#include <dict.h>
#include <stringops.h>			/* util_utf8_enable */

static void usage(char *progname)
{
    msg_fatal("usage: %s [-v] pattern_list address", progname);
}

int     main(int argc, char **argv)
{
    ADDR_MATCH_LIST *list;
    char   *addr;
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
    list = addr_match_list_init("command line", MATCH_FLAG_PARENT
				| MATCH_FLAG_RETURN, argv[optind]);
    addr = argv[optind + 1];
    if (strcmp(addr, "-") == 0) {
	VSTRING *buf = vstring_alloc(100);

	while (vstring_get_nonl(buf, VSTREAM_IN) != VSTREAM_EOF)
	    vstream_printf("%s: %s\n", vstring_str(buf),
			   addr_match_list_match(list, vstring_str(buf)) ?
			   "YES" : list->error == 0 ? "NO" : "ERROR");
	vstring_free(buf);
    } else {
	vstream_printf("%s: %s\n", addr,
		       addr_match_list_match(list, addr) > 0 ?
		       "YES" : list->error == 0 ? "NO" : "ERROR");
    }
    vstream_fflush(VSTREAM_OUT);
    addr_match_list_free(list);
    return (0);
}

#endif
