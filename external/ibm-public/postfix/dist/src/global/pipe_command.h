/*	$NetBSD: pipe_command.h,v 1.1.1.1 2009/06/23 10:08:47 tron Exp $	*/

#ifndef _PIPE_COMMAND_H_INCLUDED_
#define _PIPE_COMMAND_H_INCLUDED_

/*++
/* NAME
/*	pipe_command 3h
/* SUMMARY
/*	deliver message to external command
/* SYNOPSIS
/*	#include <pipe_command.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstream.h>
#include <vstring.h>

 /*
  * Global library.
  */
#include <mail_copy.h>
#include <dsn_buf.h>

 /*
  * Request arguments.
  */
#define PIPE_CMD_END		0	/* terminator */
#define PIPE_CMD_COMMAND	1	/* command is string */
#define PIPE_CMD_ARGV		2	/* command is array */
#define PIPE_CMD_COPY_FLAGS	3	/* mail_copy() flags */
#define PIPE_CMD_SENDER		4	/* mail_copy() sender */
#define PIPE_CMD_DELIVERED	5	/* mail_copy() recipient */
#define PIPE_CMD_UID		6	/* privileges */
#define PIPE_CMD_GID		7	/* privileges */
#define PIPE_CMD_TIME_LIMIT	8	/* time limit */
#define PIPE_CMD_ENV		9	/* extra environment */
#define PIPE_CMD_SHELL		10	/* alternative shell */
#define PIPE_CMD_EOL		11	/* record delimiter */
#define PIPE_CMD_EXPORT		12	/* exportable environment */
#define PIPE_CMD_ORIG_RCPT	13	/* mail_copy() original recipient */
#define PIPE_CMD_CWD		14	/* working directory */
#define PIPE_CMD_CHROOT		15	/* chroot() before exec() */

 /*
  * Command completion status.
  */
#define PIPE_STAT_OK		0	/* success */
#define PIPE_STAT_DEFER		1	/* try again */
#define PIPE_STAT_BOUNCE	2	/* failed */
#define PIPE_STAT_CORRUPT	3	/* corrupted file */

extern int pipe_command(VSTREAM *, DSN_BUF *,...);

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
