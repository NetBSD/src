/*++
/* NAME
/*	exec_command 3
/* SUMMARY
/*	execute command
/* SYNOPSIS
/*	#include <exec_command.h>
/*
/*	NORETURN exec_command(command)
/*	const char *command;
/* DESCRIPTION
/*	\fIexec_command\fR() replaces the current process by an instance
/*	of \fIcommand\fR. This routine uses a simple heuristic to avoid
/*	the overhead of running a command shell interpreter.
/* DIAGNOSTICS
/*	This routine never returns. All errors are fatal.
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

/* System library. */

#include <sys_defs.h>
#include <unistd.h>
#include <string.h>
#ifdef USE_PATHS_H
#include <paths.h>
#endif
#include <errno.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <argv.h>
#include <exec_command.h>

/* Application-specific. */

#define SPACE_TAB	" \t"

/* exec_command - exec command */

NORETURN exec_command(const char *command)
{
    ARGV   *argv;

    /*
     * Character filter. In this particular case, we allow space and tab in
     * addition to the regular character set.
     */
    static char ok_chars[] = "1234567890!@%-_=+:,./\
abcdefghijklmnopqrstuvwxyz\
ABCDEFGHIJKLMNOPQRSTUVWXYZ" SPACE_TAB;

    /*
     * See if this command contains any shell magic characters.
     */
    if (command[strspn(command, ok_chars)] == 0) {

	/*
	 * No shell meta characters found, so we can try to avoid the overhead
	 * of running a shell. Just split the command on whitespace and exec
	 * the result directly.
	 */
	argv = argv_split(command, SPACE_TAB);
	(void) execvp(argv->argv[0], argv->argv);

	/*
	 * Auch. Perhaps they're using some shell built-in command.
	 */
	if (errno != ENOENT || strchr(argv->argv[0], '/') != 0)
	    msg_fatal("execvp %s: %m", argv->argv[0]);

	/*
	 * Not really necessary, but...
	 */
	argv_free(argv);
    }

    /*
     * Pass the command to a shell.
     */
    (void) execl(_PATH_BSHELL, "sh", "-c", command, (char *) 0);
    msg_fatal("execl %s: %m", _PATH_BSHELL);
}

#ifdef TEST

 /*
  * Yet another proof-of-concept test program.
  */
#include <vstream.h>
#include <msg_vstream.h>

int     main(int argc, char **argv)
{
    msg_vstream_init(argv[0], VSTREAM_ERR);
    if (argc != 2)
	msg_fatal("usage: %s 'command'", argv[0]);
    exec_command(argv[1]);
}

#endif
