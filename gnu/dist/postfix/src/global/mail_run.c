/*++
/* NAME
/*	mail_run 3
/* SUMMARY
/*	run mail component program
/* SYNOPSIS
/*	#include <mail_run.h>
/*
/*	int	mail_run_foreground(dir, argv)
/*	const char *dir;
/*	char	**argv;
/*
/*	int	mail_run_background(dir, argv)
/*	const char *dir;
/*	char	**argv;
/*
/*	NORETURN mail_run_replace(dir, argv)
/*	const char *dir;
/*	char	**argv;
/* DESCRIPTION
/*	This module runs programs that live in the mail program directory.
/*	Each routine takes a directory and a command-line array. The program
/*	pathname is built by prepending the directory and a slash to the
/*	command name.
/*
/*	mail_run_foreground() runs the named command in the foreground and
/*	waits until the command terminates.
/*
/*	mail_run_background() runs the named command in the background.
/*
/*	mail_run_replace() attempts to replace the current process by
/*	an instance of the named command. This function never returns.
/*
/*	Arguments:
/* .IP argv
/*	A null-terminated command-line vector. The first array element
/*	is the base name of the program to be executed.
/* DIAGNOSTICS
/*	The result is (-1) if the command could not be run. Otherwise,
/*	mail_run_foreground() returns the termination status of the
/*	command. mail_run_background() returns the process id in case
/*	of success.
/* CONFIGURATION PARAMETERS
/*	fork_attempts: number of attempts to fork() a process;
/*	fork_delay: delay in seconds between fork() attempts.
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
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

/* Utility library. */

#include <msg.h>
#include <stringops.h>
#include <mymalloc.h>

/* Global library. */

#include "mail_params.h"
#include "mail_run.h"

/* mail_run_foreground - run command in foreground */

int     mail_run_foreground(const char *dir, char **argv)
{
    int     count;
    char   *path;
    WAIT_STATUS_T status;
    int     pid;
    int     wpid;

#define RETURN(x) { myfree(path); return(x); }

    path = concatenate(dir, "/", argv[0], (char *) 0);

    for (count = 0; count < var_fork_tries; count++) {
	switch (pid = fork()) {
	case -1:
	    msg_warn("fork %s: %m", path);
	    break;
	case 0:
	    /* Reset the msg_cleanup() handlers in the child process. */
	    (void) msg_cleanup((MSG_CLEANUP_FN) 0);
	    execv(path, argv);
	    msg_fatal("execv %s: %m", path);
	default:
	    do {
		wpid = waitpid(pid, &status, 0);
	    } while (wpid == -1 && errno == EINTR);
	    RETURN(wpid == -1 ? -1 :
		   WIFEXITED(status) ? WEXITSTATUS(status) : 1)
	}
	sleep(var_fork_delay);
    }
    RETURN(-1);
}

/* mail_run_background - run command in background */

int     mail_run_background(const char *dir, char **argv)
{
    int     count;
    char   *path;
    int     pid;

#define RETURN(x) { myfree(path); return(x); }

    path = concatenate(dir, "/", argv[0], (char *) 0);

    for (count = 0; count < var_fork_tries; count++) {
	switch (pid = fork()) {
	case -1:
	    msg_warn("fork %s: %m", path);
	    break;
	case 0:
	    /* Reset the msg_cleanup() handlers in the child process. */
	    (void) msg_cleanup((MSG_CLEANUP_FN) 0);
	    execv(path, argv);
	    msg_fatal("execv %s: %m", path);
	default:
	    RETURN(pid);
	}
	sleep(var_fork_delay);
    }
    RETURN(-1);
}

/* mail_run_replace - run command, replacing current process */

NORETURN mail_run_replace(const char *dir, char **argv)
{
    char   *path;

    path = concatenate(dir, "/", argv[0], (char *) 0);
    execv(path, argv);
    msg_fatal("execv %s: %m", path);
}
