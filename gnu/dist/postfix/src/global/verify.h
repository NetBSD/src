#ifndef _VERIFY_H_INCLUDED_
#define _VERIFY_H_INCLUDED_

/*++
/* NAME
/*	verify 3h
/* SUMMARY
/*	update user message delivery record
/* SYNOPSIS
/*	#include <verify.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <time.h>
#include <stdarg.h>

 /*
  * Global library.
  */
#include <deliver_request.h>

 /*
  * External interface.
  */
extern int PRINTFLIKE(8, 9) verify_append(const char *, const char *,
					          const char *, const char *,
					          time_t, const char *,
					          int, const char *,...);
extern int vverify_append(const char *, const char *,
			         const char *, const char *,
			         time_t, const char *,
			         int, const char *, va_list);

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
