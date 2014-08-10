/*	$NetBSD: iostuff.h,v 1.1.1.2.22.1 2014/08/10 07:12:50 tls Exp $	*/

#ifndef _IOSTUFF_H_INCLUDED_
#define _IOSTUFF_H_INCLUDED_

/*++
/* NAME
/*	iostuff 3h
/* SUMMARY
/*	miscellaneous I/O primitives
/* SYNOPSIS
/*	#include <iostuff.h>
/* DESCRIPTION

 /*
  * External interface.
  */
extern int non_blocking(int, int);
extern int close_on_exec(int, int);
extern int open_limit(int);
extern int poll_fd(int, int, int, int, int);
extern off_t get_file_limit(void);
extern void set_file_limit(off_t);
extern ssize_t peekfd(int);
extern ssize_t write_buf(int, const char *, ssize_t, int);
extern ssize_t timed_read(int, void *, size_t, int, void *);
extern ssize_t timed_write(int, void *, size_t, int, void *);
extern void doze(unsigned);
extern void rand_sleep(unsigned, unsigned);
extern int duplex_pipe(int *);
extern int stream_recv_fd(int);
extern int stream_send_fd(int, int);
extern int unix_recv_fd(int);
extern int unix_send_fd(int, int);
extern ssize_t dummy_read(int, void *, size_t, int, void *);
extern ssize_t dummy_write(int, void *, size_t, int, void *);

#define readable(fd)		poll_fd((fd), POLL_FD_READ, 0, 1, 0)
#define writable(fd)		poll_fd((fd), POLL_FD_WRITE, 0, 1, 0)

#define read_wait(fd, timeout) poll_fd((fd), POLL_FD_READ, (timeout), 0, -1)
#define write_wait(fd, timeout) poll_fd((fd), POLL_FD_WRITE, (timeout), 0, -1)

extern int inet_windowsize;
extern void set_inet_windowsize(int, int);

#define POLL_FD_READ	0
#define POLL_FD_WRITE	1

#define BLOCKING	0
#define NON_BLOCKING	1

#define CLOSE_ON_EXEC	1
#define PASS_ON_EXEC	0

extern int unix_pass_fd_fix;
extern void set_unix_pass_fd_fix(const char *);

#define UNIX_PASS_FD_FIX_NONE		(0)
#define UNIX_PASS_FD_FIX_CMSG_LEN	(1<<0)

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*      Wietse Venema
/*      IBM T.J. Watson Research
/*      P.O. Box 704
/*      Yorktown Heights, NY 10598, USA
/* CREATION DATE
/*	Sat Jan 25 16:54:13 EST 1997
/*--*/

#endif
