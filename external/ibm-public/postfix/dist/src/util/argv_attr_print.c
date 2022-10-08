/*	$NetBSD: argv_attr_print.c,v 1.3 2022/10/08 16:12:50 christos Exp $	*/

/*++
/* NAME
/*	argv_attr_print
/* SUMMARY
/*	write ARGV to stream
/* SYNOPSIS
/*	#include <argv_attr.h>
/*
/*	int	argv_attr_print(print_fn, stream, flags, ptr)
/*	ATTR_PRINT_COMMON_FN print_fn;
/*	VSTREAM *stream;
/*	int	flags;
/*	void	*ptr;
/* DESCRIPTION
/*	argv_attr_print() writes an ARGV to the named stream using
/*	the specified attribute print routine. argv_attr_print() is meant
/*	to be passed as a call-back to attr_print(), thusly:
/*
/*	... SEND_ATTR_FUNC(argv_attr_print, (const void *) argv), ...
/* DIAGNOSTICS
/*	Fatal: out of memory.
/*
/*	The result value is zero in case of success, non-zero
/*	otherwise.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>

 /*
  * Utility library.
  */
#include <argv.h>
#include <argv_attr.h>
#include <attr.h>
#include <vstream.h>
#include <msg.h>

/* argv_attr_print - write ARGV to stream */

int     argv_attr_print(ATTR_PRINT_COMMON_FN print_fn, VSTREAM *fp,
			        int flags, const void *ptr)
{
    ARGV   *argv = (ARGV *) ptr;
    int     n;
    int     ret;
    int     argc = argv ? argv->argc : 0;

    ret = print_fn(fp, flags | ATTR_FLAG_MORE,
		   SEND_ATTR_INT(ARGV_ATTR_SIZE, argc),
		   ATTR_TYPE_END);
    if (msg_verbose)
	msg_info("argv_attr_print count=%d", argc);
    for (n = 0; ret == 0 && n < argc; n++)
	ret = print_fn(fp, flags | ATTR_FLAG_MORE,
		       SEND_ATTR_STR(ARGV_ATTR_VALUE, argv->argv[n]),
		       ATTR_TYPE_END);
    if (msg_verbose)
	msg_info("argv_attr_print ret=%d", ret);
    /* Do not flush the stream. */
    return (ret);
}
