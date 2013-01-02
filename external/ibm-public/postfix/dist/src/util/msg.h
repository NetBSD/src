/*	$NetBSD: msg.h,v 1.1.1.2 2013/01/02 18:59:13 tron Exp $	*/

#ifndef _MSG_H_INCLUDED_
#define _MSG_H_INCLUDED_

/*++
/* NAME
/*	msg 3h
/* SUMMARY
/*	diagnostics interface
/* SYNOPSIS
/*	#include "msg.h"
/* DESCRIPTION
/*	.nf

/*
 * System library.
 */
#include <stdarg.h>
#include <time.h>

/*
 * External interface.
 */
typedef void (*MSG_CLEANUP_FN) (void);

extern int msg_verbose;

extern void PRINTFLIKE(1, 2) msg_info(const char *,...);
extern void PRINTFLIKE(1, 2) msg_warn(const char *,...);
extern void PRINTFLIKE(1, 2) msg_error(const char *,...);
extern NORETURN PRINTFLIKE(1, 2) msg_fatal(const char *,...);
extern NORETURN PRINTFLIKE(2, 3) msg_fatal_status(int, const char *,...);
extern NORETURN PRINTFLIKE(1, 2) msg_panic(const char *,...);

extern void vmsg_info(const char *, va_list);
extern void vmsg_warn(const char *, va_list);
extern void vmsg_error(const char *, va_list);
extern NORETURN vmsg_fatal(const char *, va_list);
extern NORETURN vmsg_fatal_status(int, const char *, va_list);
extern NORETURN vmsg_panic(const char *, va_list);

extern int msg_error_limit(int);
extern void msg_error_clear(void);
extern MSG_CLEANUP_FN msg_cleanup(MSG_CLEANUP_FN);

extern void PRINTFLIKE(4, 5) msg_rate_delay(time_t *, int,
				          void (*log_fn) (const char *,...),
					            const char *,...);

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
