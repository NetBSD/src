/*++
/* NAME
/*	fifo_listen 3
/* SUMMARY
/*	start fifo listener
/* SYNOPSIS
/*	#include <listen.h>
/*
/*	int	fifo_listen(path, permissions, block_mode)
/*	const char *path;
/*	int	permissions;
/*	int	block_mode;
/* DESCRIPTION
/*	The \fBfifo_listen\fR routine creates the specified named pipe with
/*	the specified permissions, opens the FIFO read-write or read-only,
/*	depending on the host operating system, and returns the resulting
/*	file descriptor.
/*	The \fIblock_mode\fR argument is either NON_BLOCKING for
/*	a non-blocking socket, or BLOCKING for blocking mode.
/* DIAGNOSTICS
/*	Fatal errors: all system call failures.
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

/* System interfaces. */

#include <sys_defs.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

/* Utility library. */

#include "msg.h"
#include "iostuff.h"
#include "listen.h"

#define BUF_LEN	100

/* fifo_listen - create fifo listener */

int     fifo_listen(const char *path, int permissions, int block_mode)
{
    char    buf[BUF_LEN];
    static int open_mode = 0;
    char   *myname = "fifo_listen";
    struct stat st;
    int     fd;
    int     count;

    /*
     * Create a named pipe (fifo). Do whatever we can so we don't run into
     * trouble when this process is restarted after crash.  Make sure that we
     * open a fifo and not something else, then change permissions to what we
     * wanted them to be, because mkfifo() is subject to umask settings.
     * Instead we could zero the umask temporarily before creating the FIFO,
     * but that would cost even more system calls. Figure out if the fifo
     * needs to be opened O_RDWR or O_RDONLY. Some systems need one, some
     * need the other. If we choose the wrong mode, the fifo will stay
     * readable, causing the program to go into a loop.
     */
    if (unlink(path) && errno != ENOENT)
	msg_fatal("%s: remove %s: %m", myname, path);
    if (mkfifo(path, permissions) < 0)
	msg_fatal("%s: create fifo %s: %m", myname, path);
    switch (open_mode) {
    case 0:
	if ((fd = open(path, O_RDWR | O_NONBLOCK, 0)) < 0)
	    msg_fatal("%s: open %s: %m", myname, path);
	if (readable(fd) == 0) {
	    open_mode = O_RDWR | O_NONBLOCK;
	    break;
	} else {
	    open_mode = O_RDONLY | O_NONBLOCK;
	    if (msg_verbose)
		msg_info("open O_RDWR makes fifo readable - trying O_RDONLY");
	    (void) close(fd);
	    /* FALLTRHOUGH */
	}
    default:
	if ((fd = open(path, open_mode, 0)) < 0)
	    msg_fatal("%s: open %s: %m", myname, path);
	break;
    }

    /*
     * Make sure we opened a FIFO and skip any cruft that might have
     * accumulated before we opened it.
     */
    if (fstat(fd, &st) < 0)
	msg_fatal("%s: fstat %s: %m", myname, path);
    if (S_ISFIFO(st.st_mode) == 0)
	msg_fatal("%s: not a fifo: %s", myname, path);
    if (fchmod(fd, permissions) < 0)
	msg_fatal("%s: fchmod %s: %m", myname, path);
    non_blocking(fd, block_mode);
    while ((count = peekfd(fd)) > 0
	   && read(fd, buf, BUF_LEN < count ? BUF_LEN : count) > 0)
	 /* void */ ;
    return (fd);
}
