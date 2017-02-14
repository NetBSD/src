/*	$NetBSD: spawn_command.h,v 1.2 2017/02/14 01:16:49 christos Exp $	*/

#ifndef _SPAWN_COMMAND_H_INCLUDED_
#define _SPAWN_COMMAND_H_INCLUDED_

/*++
/* NAME
/*	spawn_command 3h
/* SUMMARY
/*	run external command
/* SYNOPSIS
/*	#include <spawn_command.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <check_arg.h>

/* Legacy API: type-unchecked arguments, internal use. */
#define SPAWN_CMD_END		0	/* terminator */
#define SPAWN_CMD_ARGV		1	/* command is array */
#define SPAWN_CMD_COMMAND	2	/* command is string */
#define SPAWN_CMD_STDIN		3	/* mail_copy() flags */
#define SPAWN_CMD_STDOUT	4	/* mail_copy() sender */
#define SPAWN_CMD_STDERR	5	/* mail_copy() recipient */
#define SPAWN_CMD_UID		6	/* privileges */
#define SPAWN_CMD_GID		7	/* privileges */
#define SPAWN_CMD_TIME_LIMIT	8	/* time limit */
#define SPAWN_CMD_ENV		9	/* extra environment */
#define SPAWN_CMD_SHELL		10	/* alternative shell */
#define SPAWN_CMD_EXPORT	11	/* exportable parameters */

/* Safer API: type-checked arguments, external use. */
#define CA_SPAWN_CMD_END	SPAWN_CMD_END
#define CA_SPAWN_CMD_ARGV(v)	SPAWN_CMD_ARGV, CHECK_PPTR(CA_SPAWN_CMD, char, (v))
#define CA_SPAWN_CMD_COMMAND(v)	SPAWN_CMD_COMMAND, CHECK_CPTR(CA_SPAWN_CMD, char, (v))
#define CA_SPAWN_CMD_STDIN(v)	SPAWN_CMD_STDIN, CHECK_VAL(CA_SPAWN_CMD, int, (v))
#define CA_SPAWN_CMD_STDOUT(v)	SPAWN_CMD_STDOUT, CHECK_VAL(CA_SPAWN_CMD, int, (v))
#define CA_SPAWN_CMD_STDERR(v)	SPAWN_CMD_STDERR, CHECK_VAL(CA_SPAWN_CMD, int, (v))
#define CA_SPAWN_CMD_UID(v)	SPAWN_CMD_UID, CHECK_VAL(CA_SPAWN_CMD, uid_t, (v))
#define CA_SPAWN_CMD_GID(v)	SPAWN_CMD_GID, CHECK_VAL(CA_SPAWN_CMD, gid_t, (v))
#define CA_SPAWN_CMD_TIME_LIMIT(v) SPAWN_CMD_TIME_LIMIT, CHECK_VAL(CA_SPAWN_CMD, int, (v))
#define CA_SPAWN_CMD_ENV(v)	SPAWN_CMD_ENV, CHECK_PPTR(CA_SPAWN_CMD, char, (v))
#define CA_SPAWN_CMD_SHELL(v)	SPAWN_CMD_SHELL, CHECK_CPTR(CA_SPAWN_CMD, char, (v))
#define CA_SPAWN_CMD_EXPORT(v)	SPAWN_CMD_EXPORT, CHECK_PPTR(CA_SPAWN_CMD, char, (v))

CHECK_VAL_HELPER_DCL(CA_SPAWN_CMD, uid_t);
CHECK_VAL_HELPER_DCL(CA_SPAWN_CMD, int);
CHECK_VAL_HELPER_DCL(CA_SPAWN_CMD, gid_t);
CHECK_PPTR_HELPER_DCL(CA_SPAWN_CMD, char);
CHECK_CPTR_HELPER_DCL(CA_SPAWN_CMD, char);

extern WAIT_STATUS_T spawn_command(int,...);

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
