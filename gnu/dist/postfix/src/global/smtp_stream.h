#ifndef _SMTP_STREAM_H_INCLUDED_
#define _SMTP_STREAM_H_INCLUDED_

/*++
/* NAME
/*	smtp_stream 3h
/* SUMMARY
/*	smtp stream I/O support
/* SYNOPSIS
/*	#include <smtp_stream.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <stdarg.h>
#include <setjmp.h>

 /*
  * Utility library.
  */
#include <vstring.h>
#include <vstream.h>

 /*
  * External interface.
  */
#define SMTP_ERR_EOF	1		/* unexpected client disconnect */
#define SMTP_ERR_TIME	2		/* time out */

extern void smtp_timeout_setup(VSTREAM *, int);
extern void PRINTFLIKE(2, 3) smtp_printf(VSTREAM *, const char *,...);
extern void smtp_flush(VSTREAM *);
extern int smtp_get(VSTRING *, VSTREAM *, int);
extern void smtp_fputs(const char *, int len, VSTREAM *);
extern void smtp_fwrite(const char *, int len, VSTREAM *);
extern void smtp_fputc(int, VSTREAM *);

extern void smtp_vprintf(VSTREAM *, const char *, va_list);

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
