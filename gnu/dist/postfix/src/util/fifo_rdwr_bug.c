/*++
/* NAME
/*	fifo_rdwr_bug 1
/* SUMMARY
/*	fifo server test program
/* SYNOPSIS
/*	fifo_rdwr_bug
/* DESCRIPTION
/*	fifo_rdwr_bug creates a FIFO and opens it read-write mode.
/*	On BSD/OS 3.1 select() will report that the FIFO is readable
/*	even before any data is written to it. Doing an actual read
/*	causes the read to block; a non-blocking read fails.
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

#include <sys_defs.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define FIFO_PATH       "test-fifo"
#define perrorexit(s)   { perror(s); exit(1); }

static void cleanup(void)
{
    printf("Removing fifo %s...\n", FIFO_PATH);
    if (unlink(FIFO_PATH))
	perrorexit("unlink");
    printf("Done.\n");
}

int     main(int unused_argc, char **unused_argv)
{
    struct timeval tv;
    fd_set  read_fds;
    fd_set  except_fds;
    int     fd;

    (void) unlink(FIFO_PATH);

    printf("Creating fifo %s...\n", FIFO_PATH);
    if (mkfifo(FIFO_PATH, 0600) < 0)
	perrorexit("mkfifo");

    printf("Opening fifo %s, read-write mode...\n", FIFO_PATH);
    if ((fd = open(FIFO_PATH, O_RDWR, 0)) < 0) {
	perror("open");
	cleanup();
	exit(1);
    }
    printf("Selecting the fifo for readability...\n");
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);
    FD_ZERO(&except_fds);
    FD_SET(fd, &except_fds);
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    switch (select(fd + 1, &read_fds, (fd_set *) 0, &except_fds, &tv)) {
    case -1:
	perrorexit("select");
    default:
	if (FD_ISSET(fd, &read_fds)) {
	    printf("Opening a fifo read-write makes it readable!!\n");
	    break;
	}
    case 0:
	printf("The fifo is not readable, as it should be.\n");
	break;
    }
    cleanup();
    exit(0);
}
