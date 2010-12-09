/*	$NetBSD: errlist.c,v 1.18 2010/12/09 21:27:31 joerg Exp $	*/

/*
 * Copyright (c) 1982, 1985, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)errlst.c	8.2 (Berkeley) 11/16/93";
#else
__RCSID("$NetBSD: errlist.c,v 1.18 2010/12/09 21:27:31 joerg Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include <errno.h>

#define EL(x)	x

static const char *const errlist[] = {
	EL(("Undefined error: 0")),		/*  0 - ENOERROR */
	EL(("Operation not permitted")),	/*  1 - EPERM */
	EL(("No such file or directory")),	/*  2 - ENOENT */
	EL(("No such process")),		/*  3 - ESRCH */
	EL(("Interrupted system call")),	/*  4 - EINTR */
	EL(("Input/output error")),		/*  5 - EIO */
	EL(("Device not configured")),		/*  6 - ENXIO */
	EL(("Argument list too long")),		/*  7 - E2BIG */
	EL(("Exec format error")),		/*  8 - ENOEXEC */
	EL(("Bad file descriptor")),		/*  9 - EBADF */
	EL(("No child processes")),		/* 10 - ECHILD */
	EL(("Resource deadlock avoided")),	/* 11 - EDEADLK */
	EL(("Cannot allocate memory")),		/* 12 - ENOMEM */
	EL(("Permission denied")),		/* 13 - EACCES */
	EL(("Bad address")),			/* 14 - EFAULT */
	EL(("Block device required")),		/* 15 - ENOTBLK */
	EL(("Device busy")),			/* 16 - EBUSY */
	EL(("File exists")),			/* 17 - EEXIST */
	EL(("Cross-device link")),		/* 18 - EXDEV */
						/* 19 - ENODEV */
	EL(("Operation not supported by device")),
	EL(("Not a directory")),		/* 20 - ENOTDIR */
	EL(("Is a directory")),			/* 21 - EISDIR */
	EL(("Invalid argument")),		/* 22 - EINVAL */
	EL(("Too many open files in system")),	/* 23 - ENFILE */
	EL(("Too many open files")),		/* 24 - EMFILE */
	EL(("Inappropriate ioctl for device")),	/* 25 - ENOTTY */
	EL(("Text file busy")),			/* 26 - ETXTBSY */
	EL(("File too large")),			/* 27 - EFBIG */
	EL(("No space left on device")),	/* 28 - ENOSPC */
	EL(("Illegal seek")),			/* 29 - ESPIPE */
	EL(("Read-only file system")),		/* 30 - EROFS */
	EL(("Too many links")),			/* 31 - EMLINK */
	EL(("Broken pipe")),			/* 32 - EPIPE */

/* math software */
						/* 33 - EDOM */
	EL(("Numerical argument out of domain")),
	EL(("Result too large or too small")),	/* 34 - ERANGE */

/* non-blocking and interrupt i/o */
						/* 35 - EAGAIN */
	EL(("Resource temporarily unavailable")),
						/* 35 - EWOULDBLOCK */
	EL(("Operation now in progress")),	/* 36 - EINPROGRESS */
	EL(("Operation already in progress")),	/* 37 - EALREADY */

/* ipc/network software -- argument errors */
	EL(("Socket operation on non-socket")),	/* 38 - ENOTSOCK */
	EL(("Destination address required")),	/* 39 - EDESTADDRREQ */
	EL(("Message too long")),		/* 40 - EMSGSIZE */
	EL(("Protocol wrong type for socket")),	/* 41 - EPROTOTYPE */
	EL(("Protocol option not available")),	/* 42 - ENOPROTOOPT */
	EL(("Protocol not supported")),		/* 43 - EPROTONOSUPPORT */
	EL(("Socket type not supported")),	/* 44 - ESOCKTNOSUPPORT */
	EL(("Operation not supported")),	/* 45 - EOPNOTSUPP */
	EL(("Protocol family not supported")),	/* 46 - EPFNOSUPPORT */
						/* 47 - EAFNOSUPPORT */
	EL(("Address family not supported by protocol family")),
	EL(("Address already in use")),		/* 48 - EADDRINUSE */
	EL(("Can't assign requested address")),	/* 49 - EADDRNOTAVAIL */

/* ipc/network software -- operational errors */
	EL(("Network is down")),		/* 50 - ENETDOWN */
	EL(("Network is unreachable")),		/* 51 - ENETUNREACH */
						/* 52 - ENETRESET */
	EL(("Network dropped connection on reset")),
						/* 53 - ECONNABORTED */
	EL(("Software caused connection abort")),
	EL(("Connection reset by peer")),	/* 54 - ECONNRESET */
	EL(("No buffer space available")),	/* 55 - ENOBUFS */
	EL(("Socket is already connected")),	/* 56 - EISCONN */
	EL(("Socket is not connected")),	/* 57 - ENOTCONN */
						/* 58 - ESHUTDOWN */
	EL(("Can't send after socket shutdown")),
						/* 59 - ETOOMANYREFS */
	EL(("Too many references: can't splice")),
	EL(("Operation timed out")),		/* 60 - ETIMEDOUT */
	EL(("Connection refused")),		/* 61 - ECONNREFUSED */
						/* 62 - ELOOP */
	EL(("Too many levels of symbolic links")),
	EL(("File name too long")),		/* 63 - ENAMETOOLONG */

/* should be rearranged */
	EL(("Host is down")),			/* 64 - EHOSTDOWN */
	EL(("No route to host")),		/* 65 - EHOSTUNREACH */
	EL(("Directory not empty")),		/* 66 - ENOTEMPTY */

/* quotas & mush */
	EL(("Too many processes")),		/* 67 - EPROCLIM */
	EL(("Too many users")),			/* 68 - EUSERS */
	EL(("Disc quota exceeded")),		/* 69 - EDQUOT */

/* Network File System */
	EL(("Stale NFS file handle")),		/* 70 - ESTALE */
	EL(("Too many levels of remote in path")),/* 71 - EREMOTE */
	EL(("RPC struct is bad")),		/* 72 - EBADRPC */
	EL(("RPC version wrong")),		/* 73 - ERPCMISMATCH */
	EL(("RPC prog. not avail")),		/* 74 - EPROGUNAVAIL */
	EL(("Program version wrong")),		/* 75 - EPROGMISMATCH */
	EL(("Bad procedure for program")),	/* 76 - EPROCUNAVAIL */

	EL(("No locks available")),		/* 77 - ENOLCK */
	EL(("Function not implemented")),	/* 78 - ENOSYS */
						/* 79 - EFTYPE */
	EL(("Inappropriate file type or format")),
	EL(("Authentication error")),		/* 80 - EAUTH */
	EL(("Need authenticator")),		/* 81 - ENEEDAUTH */

/* SystemV IPC */
	EL(("Identifier removed")),		/* 82 - EIDRM */
	EL(("No message of desired type")),	/* 83 - ENOMSG */
						/* 84 - EOVERFLOW */
	EL(("Value too large to be stored in data type")),

/* Wide/multibyte-character handling, ISO/IEC 9899/AMD1:1995 */
	EL(("Illegal byte sequence")),		/* 85 - EILSEQ */

/* Base, Realtime, Threads or Thread Priority Scheduling option errors */
	EL(("Not supported")),			/* 86 - ENOTSUP */

/* Realtime option errors */
	EL(("Operation Canceled")),		/* 87 - ECANCELED */

/* Realtime, XSI STREAMS option errors */
	EL(("Bad or Corrupt message")),		/* 88 - EBADMSG */

/* XSI STREAMS option errors  */
	EL(("No message available")),		/* 89 - ENODATA */
	EL(("No STREAM resources")),		/* 90 - ENOSR */
	EL(("Not a STREAM")),			/* 91 - ENOSTR */
	EL(("STREAM ioctl timeout")),		/* 92 - ETIME */

/* File system extended attribute errors */
	EL(("Attribute not found")),		/* 93 - ENOATTR */

	EL(("Multihop attempted")),		/* 94 - EMULTIHOP */
	EL(("Link has been severed")),		/* 95 - ENOLINK */
	EL(("Protocol error")),			/* 96 - EPROTO */
};

#undef EL

const int sys_nerr = sizeof(errlist) / sizeof(errlist[0]);

const char * const *sys_errlist = errlist;
