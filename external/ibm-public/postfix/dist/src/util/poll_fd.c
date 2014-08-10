/*	$NetBSD: poll_fd.c,v 1.1.1.1.2.2 2014/08/10 07:12:50 tls Exp $	*/

/*++
/* NAME
/*	poll_fd 3
/* SUMMARY
/*	wait until file descriptor becomes readable or writable
/* SYNOPSIS
/*	#include <iostuff.h>
/*
/*	int	readable(fd)
/*	int	fd;
/*
/*	int	writable(fd)
/*	int	fd;
/*
/*	int	read_wait(fd, time_limit)
/*	int	fd;
/*	int	time_limit;
/*
/*	int	write_wait(fd, time_limit)
/*	int	fd;
/*	int	time_limit;
/*
/*	int	poll_fd(fd, request, time_limit, true_res, false_res)
/*	int	fd;
/*	int	request;
/*	int	time_limit;
/*	int	true_res;
/*	int	false_res;
/* DESCRIPTION
/*	The read*() and write*() functions in this module are macros
/*	that provide a convenient interface to poll_fd().
/*
/*	readable() asks the kernel if the specified file descriptor
/*	is readable, i.e. a read operation would not block.
/*
/*	writable() asks the kernel if the specified file descriptor
/*	is writable, i.e. a write operation would not block.
/*
/*	read_wait() waits until the specified file descriptor becomes
/*	readable, or until the time limit is reached.
/*
/*	write_wait() waits until the specified file descriptor
/*	becomes writable, or until the time limit is reached.
/*
/*	poll_fd() waits until the specified file descriptor becomes
/*	readable or writable, or until the time limit is reached.
/*
/*	Arguments:
/* .IP fd
/*	File descriptor. With implementations based on select(), a
/*	best effort is made to handle descriptors >=FD_SETSIZE.
/* .IP request
/*	POLL_FD_READ (wait until readable) or POLL_FD_WRITE (wait
/*	until writable).
/* .IP time_limit
/*	A positive value specifies a time limit in seconds. A zero
/*	value effects a poll (return immediately).  A negative value
/*	means wait until the requested POLL_FD_READ or POLL_FD_WRITE
/*	condition becomes true.
/* .IP true_res
/*	Result value when the requested POLL_FD_READ or POLL_FD_WRITE
/*	condition is true.
/* .IP false_res
/*	Result value when the requested POLL_FD_READ or POLL_FD_WRITE
/*	condition is false.
/* DIAGNOSTICS
/*	Panic: interface violation. All system call errors are fatal
/*	unless specified otherwise.
/*
/*	readable() and writable() return 1 when the requested
/*	POLL_FD_READ or POLL_FD_WRITE condition is true, zero when
/*	it is false. They never return an error indication.
/*
/*	read_wait() and write_wait() return zero when the requested
/*	POLL_FD_READ or POLL_FD_WRITE condition is true, -1 (with
/*	errno set to ETIMEDOUT) when it is false.
/*
/*	poll_fd() returns true_res when the requested POLL_FD_READ
/*	or POLL_FD_WRITE condition is true, false_res when it is
/*	false.  When poll_fd() returns a false_res value < 0, it
/*	also sets errno to ETIMEDOUT.
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
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

 /*
  * Use poll() with fall-back to select(). MacOSX needs this for devices.
  */
#if defined(USE_SYSV_POLL_THEN_SELECT)
#define poll_fd_sysv	poll_fd
#define USE_SYSV_POLL
#define USE_BSD_SELECT
int     poll_fd_bsd(int, int, int, int, int);

 /*
  * Use select() only.
  */
#elif defined(USE_BSD_SELECT)
#define poll_fd_bsd	poll_fd
#undef USE_SYSV_POLL

 /*
  * Use poll() only.
  */
#elif defined(USE_SYSV_POLL)
#define poll_fd_sysv	poll_fd

 /*
  * Sanity check.
  */
#else
#error "specify USE_SYSV_POLL, USE_BSD_SELECT or USE_SYSV_POLL_THEN_SELECT"
#endif

#ifdef USE_SYSV_POLL
#include <poll.h>
#endif

#ifdef USE_SYS_SELECT_H
#include <sys/select.h>
#endif

/* Utility library. */

#include <msg.h>
#include <iostuff.h>

#ifdef USE_BSD_SELECT

/* poll_fd_bsd - block with time_limit until file descriptor is ready */

int     poll_fd_bsd(int fd, int request, int time_limit,
		            int true_res, int false_res)
{
    fd_set  req_fds;
    fd_set *read_fds;
    fd_set *write_fds;
    fd_set  except_fds;
    struct timeval tv;
    struct timeval *tp;
    int     temp_fd = -1;

    /*
     * Sanity checks.
     */
    if (FD_SETSIZE <= fd) {
	if ((temp_fd = dup(fd)) < 0 || temp_fd >= FD_SETSIZE)
	    msg_fatal("descriptor %d does not fit FD_SETSIZE %d", fd, FD_SETSIZE);
	fd = temp_fd;
    }

    /*
     * Use select() so we do not depend on alarm() and on signal() handlers.
     * Restart select() when interrupted by some signal. Some select()
     * implementations reduce the time to wait when interrupted, which is
     * exactly what we want.
     */
    FD_ZERO(&req_fds);
    FD_SET(fd, &req_fds);
    except_fds = req_fds;
    if (request == POLL_FD_READ) {
	read_fds = &req_fds;
	write_fds = 0;
    } else if (request == POLL_FD_WRITE) {
	read_fds = 0;
	write_fds = &req_fds;
    } else {
	msg_panic("poll_fd: bad request %d", request);
    }

    if (time_limit >= 0) {
	tv.tv_usec = 0;
	tv.tv_sec = time_limit;
	tp = &tv;
    } else {
	tp = 0;
    }

    for (;;) {
	switch (select(fd + 1, read_fds, write_fds, &except_fds, tp)) {
	case -1:
	    if (errno != EINTR)
		msg_fatal("select: %m");
	    continue;
	case 0:
	    if (temp_fd != -1)
		(void) close(temp_fd);
	    if (false_res < 0)
		errno = ETIMEDOUT;
	    return (false_res);
	default:
	    if (temp_fd != -1)
		(void) close(temp_fd);
	    return (true_res);
	}
    }
}

#endif

#ifdef USE_SYSV_POLL

#ifdef USE_SYSV_POLL_THEN_SELECT
#define HANDLE_SYSV_POLL_ERROR(fd, req, time_limit, true_res, false_res) \
	return (poll_fd_bsd((fd), (req), (time_limit), (true_res), (false_res)))
#else
#define HANDLE_SYSV_POLL_ERROR(fd, req, time_limit, true_res, false_res) \
	msg_fatal("poll: %m")
#endif

/* poll_fd_sysv - block with time_limit until file descriptor is ready */

int     poll_fd_sysv(int fd, int request, int time_limit,
		             int true_res, int false_res)
{
    struct pollfd pollfd;

    /*
     * System-V poll() is optimal for polling a few descriptors.
     */
#define WAIT_FOR_EVENT	(-1)

    pollfd.fd = fd;
    if (request == POLL_FD_READ) {
	pollfd.events = POLLIN;
    } else if (request == POLL_FD_WRITE) {
	pollfd.events = POLLOUT;
    } else {
	msg_panic("poll_fd: bad request %d", request);
    }

    for (;;) {
	switch (poll(&pollfd, 1, time_limit < 0 ?
		     WAIT_FOR_EVENT : time_limit * 1000)) {
	case -1:
	    if (errno != EINTR)
		HANDLE_SYSV_POLL_ERROR(fd, request, time_limit,
				       true_res, false_res);
	    continue;
	case 0:
	    if (false_res < 0)
		errno = ETIMEDOUT;
	    return (false_res);
	default:
	    if (pollfd.revents & POLLNVAL)
		HANDLE_SYSV_POLL_ERROR(fd, request, time_limit,
				       true_res, false_res);
	    return (true_res);
	}
    }
}

#endif
