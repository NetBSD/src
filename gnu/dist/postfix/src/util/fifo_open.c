/*++
/* NAME
/*	fifo_open 1
/* SUMMARY
/*	fifo client test program
/* SYNOPSIS
/*	fifo_open
/* DESCRIPTION
/*	fifo_open creates a FIFO, then attempts to open it for writing
/*	with non-blocking mode enabled. According to the POSIX standard
/*	the open should succeed.
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

#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

#define FIFO_PATH	"test-fifo"
#define perrorexit(s)	{ perror(s); exit(1); }

static void cleanup(void)
{
    printf("Removing fifo %s...\n", FIFO_PATH);
    if (unlink(FIFO_PATH))
	perrorexit("unlink");
    printf("Done.\n");
}

static void stuck(int unused_sig)
{
    printf("Non-blocking, write-only open of FIFO blocked\n");
    cleanup();
    exit(1);
}

int     main(int unused_argc, char **unused_argv)
{
    (void) unlink(FIFO_PATH);
    printf("Creating fifo %s...\n", FIFO_PATH);
    if (mkfifo(FIFO_PATH, 0600) < 0)
	perrorexit("mkfifo");
    signal(SIGALRM, stuck);
    alarm(5);
    printf("Opening fifo %s, non-blocking, write-only mode...\n", FIFO_PATH);
    if (open(FIFO_PATH, O_WRONLY | O_NONBLOCK, 0) < 0) {
	perror("open");
	cleanup();
	exit(1);
    }
    printf("Non-blocking, write-only open of FIFO succeeded\n");
    cleanup();
    exit(0);
}
