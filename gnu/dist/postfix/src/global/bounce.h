#ifndef _BOUNCE_H_INCLUDED_
#define _BOUNCE_H_INCLUDED_

/*++
/* NAME
/*	bounce 3h
/* SUMMARY
/*	bounce service client
/* SYNOPSIS
/*	#include <bounce.h>
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
  * Client interface.
  */
extern int PRINTFLIKE(8, 9) bounce_append(int, const char *,
					          const char *, const char *,
					          long, const char *, time_t,
					          const char *,...);
extern int vbounce_append(int, const char *, const char *, const char *, long,
		               const char *, time_t, const char *, va_list);
extern int bounce_flush(int, const char *, const char *, const char *, const char *);
extern int PRINTFLIKE(11, 12) bounce_one(int, const char *, const char *,
					         const char *, const char *,
					         const char *, const char *,
					         long, const char *, time_t,
					         const char *,...);
extern int vbounce_one(int, const char *, const char *, const char *,
		               const char *, const char *, const char *, long,
		               const char *, time_t, const char *, va_list);

 /*
  * Bounce/defer protocol commands.
  */
#define BOUNCE_CMD_APPEND	0	/* append log */
#define BOUNCE_CMD_FLUSH	1	/* send log */
#define BOUNCE_CMD_WARN		2	/* send warning, don't delete log */
#define BOUNCE_CMD_VERP		3	/* send log, verp style */
#define BOUNCE_CMD_ONE		4	/* send one recipient notice */
#define BOUNCE_CMD_TRACE	5	/* send delivery record */

 /*
  * Flags.
  */
#define BOUNCE_FLAG_NONE	0	/* no flags up */
#define BOUNCE_FLAG_CLEAN	(1<<0)	/* remove log on error */
#define BOUNCE_FLAG_DELRCPT	(1<<1)	/* delete recipient from queue file */

 /*
  * Backwards compatibility.
  */
#define BOUNCE_FLAG_KEEP	BOUNCE_FLAG_NONE

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
