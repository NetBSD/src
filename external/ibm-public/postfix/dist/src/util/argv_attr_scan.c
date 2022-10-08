/*	$NetBSD: argv_attr_scan.c,v 1.3 2022/10/08 16:12:50 christos Exp $	*/

/*++
/* NAME
/*	argv_attr_scan
/* SUMMARY
/*	read ARGV from stream
/* SYNOPSIS
/*	#include <argv_attr.h>
/*
/*	int	argv_attr_scan(scan_fn, stream, flags, ptr)
/*	ATTR_SCAN_COMMON_FN scan_fn;
/*	VSTREAM *stream;
/*	int	flags;
/*	void	*ptr;
/* DESCRIPTION
/*	argv_attr_scan() creates an ARGV and reads its contents
/*	from the named stream using the specified attribute scan
/*	routine. argv_attr_scan() is meant to be passed as a call-back
/*	to attr_scan(), thusly:
/*
/*	ARGV *argv = 0;
/*	...
/*	... RECV_ATTR_FUNC(argv_attr_scan, (void *) &argv), ...
/*	...
/*	if (argv)
/*	    argv_free(argv);
/* DIAGNOSTICS
/*	Fatal: out of memory.
/*
/*	In case of error, this function returns non-zero and creates
/*	an ARGV null pointer.
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
#include <msg.h>
#include <vstream.h>
#include <vstring.h>

/* argv_attr_scan - write ARGV to stream */

int     argv_attr_scan(ATTR_PRINT_COMMON_FN scan_fn, VSTREAM *fp,
		               int flags, void *ptr)
{
    ARGV   *argv = 0;
    int     size;
    int     ret;

    if ((ret = scan_fn(fp, flags | ATTR_FLAG_MORE,
		       RECV_ATTR_INT(ARGV_ATTR_SIZE, &size),
		       ATTR_TYPE_END)) == 1) {
	if (msg_verbose)
	    msg_info("argv_attr_scan count=%d", size);
	if (size < 0 || size > ARGV_ATTR_MAX) {
	    msg_warn("invalid size %d from %s while reading ARGV",
		     size, VSTREAM_PATH(fp));
	    ret = -1;
	} else if (size > 0) {
	    VSTRING *buffer = vstring_alloc(100);

	    argv = argv_alloc(size);
	    while (ret == 1 && size-- > 0) {
		if ((ret = scan_fn(fp, flags | ATTR_FLAG_MORE,
				   RECV_ATTR_STR(ARGV_ATTR_VALUE, buffer),
				   ATTR_TYPE_END)) == 1)
		    argv_add(argv, vstring_str(buffer), ARGV_END);
	    }
	    argv_terminate(argv);
	    vstring_free(buffer);
	}
    }
    *(ARGV **) ptr = argv;
    if (msg_verbose)
	msg_info("argv_attr_scan ret=%d", ret);
    return (ret);
}
