/*++
/* NAME
/*	select_bug 1
/* SUMMARY
/*	select test program
/* SYNOPSIS
/*	select_bug
/* DESCRIPTION
/*	select_bug forks child processes that perform select()
/*	on a shared socket, and sees if a wakeup affects other
/*	processes selecting on a different socket or stdin.
/* DIAGNOSTICS
/*	Problems are reported to the standard error stream.
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
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>			/* bzero() prototype for 44BSD */

/* Utility library. */

#include <msg.h>
#include <vstream.h>
#include <msg_vstream.h>

static pid_t fork_and_read_select(const char *what, int delay, int fd)
{
    struct timeval tv;
    pid_t   pid;
    fd_set  readfds;

    switch (pid = fork()) {
    case -1:
	msg_fatal("fork: %m");
    case 0:
	tv.tv_sec = delay;
	tv.tv_usec = 0;
	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);
	switch (select(fd + 1, &readfds, (fd_set *) 0, &readfds, &tv)) {
	case -1:
	    msg_fatal("select: %m");
	case 0:
	    msg_info("%s select timed out", what);
	    exit(0);
	default:
	    msg_info("%s select wakeup", what);
	    exit(0);
	}
    default:
	return (pid);
    }
}

main(int argc, char **argv)
{
    int     pair1[2];
    int     pair2[2];

    msg_vstream_init(argv[0], VSTREAM_ERR);

#define DELAY 1

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair1) < 0)
	msg_fatal("socketpair: %m");
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair2) < 0)
	msg_fatal("socketpair: %m");

    vstream_printf("Doing multiple select on socket1, then write to it...\n");
    vstream_fflush(VSTREAM_OUT);
    fork_and_read_select("socket1", DELAY, pair1[0]);	/* one */
    fork_and_read_select("socket1", DELAY, pair1[0]);	/* two */
    fork_and_read_select("socket2", DELAY, pair2[0]);
    fork_and_read_select("stdin", DELAY, 0);
    if (write(pair1[1], "", 1) != 1)
	msg_fatal("write: %m");
    while (wait((int *) 0) >= 0)
	 /* void */ ;
}
