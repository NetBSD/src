/*	$NetBSD: msg_output.h,v 1.1.1.1.2.2 2009/09/15 06:04:00 snj Exp $	*/

#ifndef _MSG_OUTPUT_FN_
#define _MSG_OUTPUT_FN_

/*++
/* NAME
/*	msg_output 3h
/* SUMMARY
/*	diagnostics output management
/* SYNOPSIS
/*	#include <msg_output.h>
/* DESCRIPTION

 /*
  * System library.
  */
#include <stdarg.h>

 /*
  * External interface. Severity levels are documented to be monotonically
  * increasing from 0 up to MSG_LAST.
  */
typedef void (*MSG_OUTPUT_FN) (int, const char *);
extern void msg_output(MSG_OUTPUT_FN);
extern void PRINTFLIKE(2, 3) msg_printf(int, const char *,...);
extern void msg_vprintf(int, const char *, va_list);
extern void msg_text(int, const char *);

#define MSG_INFO	0		/* informative */
#define	MSG_WARN	1		/* warning (non-fatal) */
#define MSG_ERROR	2		/* error (fatal) */
#define MSG_FATAL	3		/* software error (fatal) */
#define MSG_PANIC	4		/* software error (fatal) */

#define MSG_LAST	4		/* highest-numbered severity level */

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*      Wietse Venema
/*      IBM T.J. Watson Research
/*      P.O. Box 704
/*      Yorktown Heights, NY 10598, USA
/*--*/

#endif
