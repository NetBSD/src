/*	$NetBSD: pipe_command.h,v 1.2 2017/02/14 01:16:45 christos Exp $	*/

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
#include <check_arg.h>

 /*
  * Global library.
  */
#include <mail_copy.h>
#include <dsn_buf.h>

 /*
  * Legacy API: type-unchecked arguments, internal use.
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
  * Safer API: type-checked arguments, external use.
  */
#define CA_PIPE_CMD_END		PIPE_CMD_END
#define CA_PIPE_CMD_COMMAND(v)	PIPE_CMD_COMMAND, CHECK_CPTR(PIPE_CMD, char, (v))
#define CA_PIPE_CMD_ARGV(v)	PIPE_CMD_ARGV, CHECK_PPTR(PIPE_CMD, char, (v))
#define CA_PIPE_CMD_COPY_FLAGS(v) PIPE_CMD_COPY_FLAGS, CHECK_VAL(PIPE_CMD, int, (v))
#define CA_PIPE_CMD_SENDER(v)	PIPE_CMD_SENDER, CHECK_CPTR(PIPE_CMD, char, (v))
#define CA_PIPE_CMD_DELIVERED(v) PIPE_CMD_DELIVERED, CHECK_CPTR(PIPE_CMD, char, (v))
#define CA_PIPE_CMD_UID(v)	PIPE_CMD_UID, CHECK_VAL(PIPE_CMD, uid_t, (v))
#define CA_PIPE_CMD_GID(v)	PIPE_CMD_GID, CHECK_VAL(PIPE_CMD, gid_t, (v))
#define CA_PIPE_CMD_TIME_LIMIT(v) PIPE_CMD_TIME_LIMIT, CHECK_VAL(PIPE_CMD, int, (v))
#define CA_PIPE_CMD_ENV(v)	PIPE_CMD_ENV, CHECK_PPTR(PIPE_CMD, char, (v))
#define CA_PIPE_CMD_SHELL(v)	PIPE_CMD_SHELL, CHECK_CPTR(PIPE_CMD, char, (v))
#define CA_PIPE_CMD_EOL(v)	PIPE_CMD_EOL, CHECK_CPTR(PIPE_CMD, char, (v))
#define CA_PIPE_CMD_EXPORT(v)	PIPE_CMD_EXPORT, CHECK_PPTR(PIPE_CMD, char, (v))
#define CA_PIPE_CMD_ORIG_RCPT(v) PIPE_CMD_ORIG_RCPT, CHECK_CPTR(PIPE_CMD, char, (v))
#define CA_PIPE_CMD_CWD(v)	PIPE_CMD_CWD, CHECK_CPTR(PIPE_CMD, char, (v))
#define CA_PIPE_CMD_CHROOT(v)	PIPE_CMD_CHROOT, CHECK_CPTR(PIPE_CMD, char, (v))

CHECK_VAL_HELPER_DCL(PIPE_CMD, uid_t);
CHECK_VAL_HELPER_DCL(PIPE_CMD, int);
CHECK_VAL_HELPER_DCL(PIPE_CMD, gid_t);
CHECK_PPTR_HELPER_DCL(PIPE_CMD, char);
CHECK_CPTR_HELPER_DCL(PIPE_CMD, char);

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
