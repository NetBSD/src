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

 /*
  * Global library.
  */
#include <deliver_request.h>
#include <dsn_buf.h>

 /*
  * Client interface.
  */
extern int bounce_append(int, const char *, MSG_STATS *, RECIPIENT *,
			         const char *, DSN *);
extern int bounce_flush(int, const char *, const char *, const char *,
			        const char *, const char *, int);
extern int bounce_flush_verp(int, const char *, const char *, const char *,
		             const char *, const char *, int, const char *);
extern int bounce_one(int, const char *, const char *, const char *,
		              const char *, const char *,
		              int, MSG_STATS *, RECIPIENT *,
		              const char *, DSN *);

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
  * Macros to make obscure code more readable.
  */
#define NO_DSN_DCODE		((char *) 0)
#define NO_RELAY_AGENT		"none"
#define NO_DSN_RMTA		((char *) 0)

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
