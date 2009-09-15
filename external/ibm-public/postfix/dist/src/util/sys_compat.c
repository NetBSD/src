/*	$NetBSD: sys_compat.c,v 1.1.1.1.2.2 2009/09/15 06:04:04 snj Exp $	*/

/*++
/* NAME
/*	sys_compat 3
/* SUMMARY
/*	compatibility routines
/* SYNOPSIS
/*	#include <sys_defs.h>
/*
/*	void	closefrom(int lowfd)
/*	int	lowfd;
/*
/*	const char *strerror(err)
/*	int	err;
/*
/*	int	setenv(name, value, clobber)
/*	const char *name;
/*	const char *value;
/*	int	clobber;
/*
/*	int	seteuid(euid)
/*	uid_t	euid;
/*
/*	int	setegid(egid)
/*	gid_t	euid;
/*
/*	int	mkfifo(path, mode)
/*	char	*path;
/*	int	mode;
/*
/*	int	waitpid(pid, statusp, options)
/*	int	pid;
/*	WAIT_STATUS_T *statusp;
/*	int	options;
/*
/*	int	setsid()
/*
/*	void	dup2_pass_on_exec(int oldd, int newd)
/*
/*	char	*inet_ntop(af, src, dst, size)
/*	int	af;
/*	const void *src;
/*	char	*dst;
/*	size_t	size;
/*
/*	int	inet_pton(af, src, dst)
/*	int	af;
/*	const char *src;
/*	void	*dst;
/* DESCRIPTION
/*	These routines are compiled for platforms that lack the functionality
/*	or that have broken versions that we prefer to stay away from.
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

#include "sys_defs.h"

 /*
  * ANSI strerror() emulation
  */
#ifdef MISSING_STRERROR

extern int errno;
extern char *sys_errlist[];
extern int sys_nerr;

#include <vstring.h>

/* strerror - print text corresponding to error */

const char *strerror(int err)
{
    static VSTRING *buf;

    if (err < 0 || err >= sys_nerr) {
	if (buf == 0)
	    buf = vstring_alloc(10);
	vstring_sprintf(buf, "Unknown error %d", err);
	return (vstring_str(buf));
    } else {
	return (sys_errlist[errno]);
    }
}

#endif

 /*
  * setenv() emulation on top of putenv().
  */
#ifdef MISSING_SETENV

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* setenv - update or insert environment (name,value) pair */

int     setenv(const char *name, const char *value, int clobber)
{
    char   *cp;

    if (clobber == 0 && getenv(name) != 0)
	return (0);
    if ((cp = malloc(strlen(name) + strlen(value) + 2)) == 0)
	return (1);
    sprintf(cp, "%s=%s", name, value);
    return (putenv(cp));
}

#endif

 /*
  * seteuid() and setegid() emulation, the HP-UX way
  */
#ifdef MISSING_SETEUID
#ifdef HAVE_SETRESUID
#include <unistd.h>

int     seteuid(uid_t euid)
{
    return setresuid(-1, euid, -1);
}

#else
#error MISSING_SETEUID
#endif

#endif

#ifdef MISSING_SETEGID
#ifdef HAVE_SETRESGID
#include <unistd.h>

int     setegid(gid_t egid)
{
    return setresgid(-1, egid, -1);
}

#else
#error MISSING_SETEGID
#endif

#endif

 /*
  * mkfifo() emulation - requires superuser privileges
  */
#ifdef MISSING_MKFIFO

#include <sys/stat.h>

int     mkfifo(char *path, int mode)
{
    return mknod(path, (mode & ~_S_IFMT) | _S_IFIFO, 0);
}

#endif

 /*
  * waitpid() emulation on top of Berkeley UNIX wait4()
  */
#ifdef MISSING_WAITPID
#ifdef HAS_WAIT4

#include <sys/wait.h>
#include <errno.h>

int     waitpid(int pid, WAIT_STATUS_T *status, int options)
{
    if (pid == -1)
	pid = 0;
    return wait4(pid, status, options, (struct rusage *) 0);
}

#else
#error MISSING_WAITPID
#endif

#endif

 /*
  * setsid() emulation, the Berkeley UNIX way
  */
#ifdef MISSING_SETSID

#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#ifdef TIOCNOTTY

#include <msg.h>

int     setsid(void)
{
    int     p = getpid();
    int     fd;

    if (setpgrp(p, p))
	return -1;

    fd = open("/dev/tty", O_RDONLY, 0);
    if (fd >= 0 || errno != ENXIO) {
	if (fd < 0) {
	    msg_warn("open /dev/tty: %m");
	    return -1;
	}
	if (ioctl(fd, TIOCNOTTY, 0)) {
	    msg_warn("ioctl TIOCNOTTY: %m");
	    return -1;
	}
	close(fd);
    }
    return 0;
}

#else
#error MISSING_SETSID
#endif

#endif

 /*
  * dup2_pass_on_exec() - dup2() and clear close-on-exec flag on the result
  */
#ifdef DUP2_DUPS_CLOSE_ON_EXEC

#include "iostuff.h"

int     dup2_pass_on_exec(int oldd, int newd)
{
    int     res;

    if ((res = dup2(oldd, newd)) >= 0)
	close_on_exec(newd, PASS_ON_EXEC);

    return res;
}

#endif

#ifndef HAS_CLOSEFROM

#include <unistd.h>
#include <errno.h>
#include <iostuff.h>

/* closefrom() - closes all file descriptors from the given one up */

int     closefrom(int lowfd)
{
    int     fd_limit = open_limit(0);
    int     fd;

    /*
     * lowfrom does not have an easy to determine upper limit. A process may
     * have files open that were inherited from a parent process with a less
     * restrictive resource limit.
     */
    if (lowfd < 0) {
	errno = EBADF;
	return (-1);
    }
    if (fd_limit > 500)
	fd_limit = 500;
    for (fd = lowfd; fd < fd_limit; fd++)
	(void) close(fd);

    return (0);
}

#endif

#ifdef MISSING_INET_NTOP

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

/* inet_ntop - convert binary address to printable address */

const char *inet_ntop(int af, const void *src, char *dst, size_t size)
{
    const unsigned char *addr;
    char    buffer[sizeof("255.255.255.255")];
    int     len;

    if (af != AF_INET) {
	errno = EAFNOSUPPORT;
	return (0);
    }
    addr = (const unsigned char *) src;
#if (CHAR_BIT > 8)
    sprintf(buffer, "%d.%d.%d.%d", addr[0] & 0xff,
	    addr[1] & 0xff, addr[2] & 0xff, addr[3] & 0xff);
#else
    sprintf(buffer, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
#endif
    if ((len = strlen(buffer)) >= size) {
	errno = ENOSPC;
	return (0);
    } else {
	memcpy(dst, buffer, len + 1);
	return (dst);
    }
}

#endif

#ifdef MISSING_INET_PTON

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

/* inet_pton - convert printable address to binary address */

int     inet_pton(int af, const char *src, void *dst)
{
    struct in_addr addr;

    /*
     * inet_addr() accepts a wider range of input formats than inet_pton();
     * the former accepts 1-, 2-, or 3-part dotted addresses, while the
     * latter requires dotted quad form.
     */
    if (af != AF_INET) {
	errno = EAFNOSUPPORT;
	return (-1);
    } else if ((addr.s_addr = inet_addr(src)) == INADDR_NONE
	       && strcmp(src, "255.255.255.255") != 0) {
	return (0);
    } else {
	memcpy(dst, (char *) &addr, sizeof(addr));
	return (1);
    }
}

#endif
