/*	$NetBSD: msg_vstream.c,v 1.1.1.1.2.2 2009/09/15 06:04:00 snj Exp $	*/

/*++
/* NAME
/*	msg_vstream 3
/* SUMMARY
/*	report diagnostics to VSTREAM
/* SYNOPSIS
/*	#include <msg_vstream.h>
/*
/*	void	msg_vstream_init(progname, stream)
/*	const char *progname;
/*	VSTREAM	*stream;
/* DESCRIPTION
/*	This module implements support to report msg(3) diagnostics
/*	to a VSTREAM.
/*
/*	msg_vstream_init() sets the program name that appears in each output
/*	record, and directs diagnostics (see msg(3)) to the specified
/*	VSTREAM. The \fIprogname\fR argument is not copied.
/* SEE ALSO
/*	msg(3)
/* BUGS
/*	No guarantee that long records are written atomically.
/*	Only the last msg_vstream_init() call takes effect.
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

/* System libraries. */

#include <sys_defs.h>
#include <errno.h>
#include <stdlib.h>			/* 44BSD stdarg.h uses abort() */
#include <stdarg.h>

/* Utility library. */

#include "vstream.h"
#include "msg.h"
#include "msg_output.h"
#include "msg_vstream.h"

 /*
  * Private state.
  */
static const char *msg_tag;
static VSTREAM *msg_stream;

/* msg_vstream_print - log diagnostic to VSTREAM */

static void msg_vstream_print(int level, const char *text)
{
    static const char *level_text[] = {
	"info", "warning", "error", "fatal", "panic",
    };

    if (level < 0 || level >= (int) (sizeof(level_text) / sizeof(level_text[0])))
	msg_panic("invalid severity level: %d", level);
    if (level == MSG_INFO) {
	vstream_fprintf(msg_stream, "%s: %s\n",
			msg_tag, text);
    } else {
	vstream_fprintf(msg_stream, "%s: %s: %s\n",
			msg_tag, level_text[level], text);
    }
    vstream_fflush(msg_stream);
}

/* msg_vstream_init - initialize */

void    msg_vstream_init(const char *name, VSTREAM *vp)
{
    static int first_call = 1;

    msg_tag = name;
    msg_stream = vp;
    if (first_call) {
	first_call = 0;
	msg_output(msg_vstream_print);
    }
}
