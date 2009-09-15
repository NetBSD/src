/*	$NetBSD: recdump.c,v 1.1.1.1.2.2 2009/09/15 06:02:53 snj Exp $	*/

/*++
/* NAME
/*	recdump 1
/* SUMMARY
/*	convert record stream to printable form
/* SYNOPSIS
/*	recdump
/* DESCRIPTION
/*	recdump reads a record stream from standard input and
/*	writes the content to standard output in printable form.
/* DIAGNOSTICS
/*	Problems are reported to the standard error stream.
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
#include <stdlib.h>

/* Utility library. */

#include <msg_vstream.h>

/* Global library. */

#include <record.h>
#include <rec_streamlf.h>
#include <rec_type.h>

int     main(int unused_argc, char **argv)
{
    VSTRING *buf = vstring_alloc(100);
    long    offset;
    int     type;

    msg_vstream_init(argv[0], VSTREAM_OUT);

    while (offset = vstream_ftell(VSTREAM_IN),
	   ((type = rec_get(VSTREAM_IN, buf, 0)) != REC_TYPE_EOF
	   && type != REC_TYPE_ERROR)) {
	vstream_fprintf(VSTREAM_OUT, "%15s|%4ld|%3ld|%s\n",
			rec_type_name(type), offset,
			(long) VSTRING_LEN(buf), vstring_str(buf));
    }
    vstream_fflush(VSTREAM_OUT);
    vstring_free(buf);
    exit(0);
}
