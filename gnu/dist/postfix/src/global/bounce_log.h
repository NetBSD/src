#ifndef _BOUNCE_LOG_H_INCLUDED_
#define _BOUNCE_LOG_H_INCLUDED_

/*++
/* NAME
/*	bounce_log 3h
/* SUMMARY
/*	bounce file reader
/* SYNOPSIS
/*	#include <bounce_log.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstream.h>
#include <vstring.h>

 /*
  * External interface.
  */
typedef struct {
    /* Private. */
    VSTREAM *fp;			/* open file */
    VSTRING *buf;			/* I/O buffer */
    VSTRING *rcpt_buf;			/* final recipient */
    VSTRING *orcp_buf;			/* original recipient */
    VSTRING *status_buf;		/* dsn code */
    const char *compat_status;		/* old logfile compatibility */
    VSTRING *action_buf;		/* dsn action */
    const char *compat_action;		/* old logfile compatibility */
    VSTRING *text_buf;			/* descriptive text */
    /* Public. */
    const char *recipient;		/* final recipient */
    const char *orig_rcpt;		/* original recipient */
    long    rcpt_offset;		/* queue file offset */
    const char *dsn_status;		/* dsn code */
    const char *dsn_action;		/* dsn action */
    const char *text;			/* descriptive text */
} BOUNCE_LOG;

extern BOUNCE_LOG *bounce_log_open(const char *, const char *, int, int);
extern BOUNCE_LOG *bounce_log_read(BOUNCE_LOG *);
extern BOUNCE_LOG *bounce_log_delrcpt(BOUNCE_LOG *);
extern BOUNCE_LOG *bounce_log_forge(const char *, const char *, long, const char *, const char *, const char *);
extern int bounce_log_close(BOUNCE_LOG *);

#define bounce_log_rewind(bp) vstream_fseek((bp)->fp, 0L, SEEK_SET)

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

#endif
