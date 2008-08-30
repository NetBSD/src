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
  * Request arguments.
  */
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
