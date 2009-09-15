/*	$NetBSD: fifo_rdonly_bug.c,v 1.1.1.1.2.2 2009/09/15 06:03:57 snj Exp $	*/

/*++
/* NAME
/*	fifo_rdonly_bug 1
/* SUMMARY
/*	fifo server test program
/* SYNOPSIS
/*	fifo_rdonly_bug
/* DESCRIPTION
/*	fifo_rdonly_bug creates a FIFO and opens it read only. It
/*	then opens the FIFO for writing, writes one byte, and closes
/*	the writing end. On Linux Redhat 4.2 and 5.0, and HP-UX 9.05
/*	and 10.20, select() will report that the FIFO remains readable
/*	even after multiple read operations.
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
#include <sys/stat.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#define FIFO_PATH	"test-fifo"
#define TRIGGER_DELAY	5

#define perrorexit(s)   { perror(s); exit(1); }

static void cleanup(void)
{
    printf("Removing fifo %s...\n", FIFO_PATH);
    if (unlink(FIFO_PATH))
	perrorexit("unlink");
    printf("Done.\n");
}

static void perrorcleanup(char *str)
{
    perror(str);
    cleanup();
    exit(0);
}

static void readable_event(int fd)
{
    char    ch;
    static int count = 0;

    if (read(fd, &ch, 1) < 0) {
	perror("read");
	sleep(1);
    }
    if (count++ > 5) {
	printf("FIFO remains readable after multiple reads.\n");
	cleanup();
	exit(1);
    }
}

int     main(int unused_argc, char **unused_argv)
{
    struct timeval tv;
    fd_set  read_fds;
    fd_set  except_fds;
    int     fd;
    int     fd2;

    (void) unlink(FIFO_PATH);

    printf("Create fifo %s...\n", FIFO_PATH);
    if (mkfifo(FIFO_PATH, 0600) < 0)
	perrorexit("mkfifo");

    printf("Open fifo %s, read-only mode...\n", FIFO_PATH);
    if ((fd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK, 0)) < 0)
	perrorcleanup("open");

    printf("Write one byte to the fifo, then close it...\n");
    if ((fd2 = open(FIFO_PATH, O_WRONLY, 0)) < 0)
	perrorcleanup("open fifo O_WRONLY");
    if (write(fd2, "", 1) < 1)
	perrorcleanup("write one byte to fifo");
    if (close(fd2) < 0)
	perrorcleanup("close fifo");

    printf("Selecting the fifo for readability...\n");

    for (;;) {
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
	    if (FD_ISSET(fd, &except_fds)) {
		printf("Exceptional fifo condition! You are not normal!\n");
		readable_event(fd);
	    } else if (FD_ISSET(fd, &read_fds)) {
		printf("Readable fifo condition\n");
		readable_event(fd);
	    }
	    break;
	case 0:
	    printf("The fifo is not readable. You're normal.\n");
	    cleanup();
	    exit(0);
	    break;
	}
    }
}
