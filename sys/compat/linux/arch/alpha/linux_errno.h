/* 	$NetBSD: linux_errno.h,v 1.1.12.1 2001/03/12 13:29:51 bouyer Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Eric Haszlakiewicz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ALPHA_LINUX_ERRNO_H
#define _ALPHA_LINUX_ERRNO_H

/*
 * Linux/Alpha returns negative errors to userland  
 * The libc makes the errno positive.  
 */
#define LINUX_SCERR_SIGN -

/* Linux switches EDEADLK and EAGAIN. */
#undef	LINUX_EDEADLK
#define LINUX_EDEADLK		11
#undef	LINUX_EAGAIN
#define LINUX_EAGAIN		35

#define LINUX_EWOULDBLOCK	LINUX_EAGAIN
#define LINUX_EINPROGRESS	36	/* Operation now in progress */
#define LINUX_EALREADY		37	/* Operation already in progress */
#define LINUX_ENOTSOCK		38	/* Socket operation on non-socket */
#define LINUX_EDESTADDRREQ	39	/* Destination address required */
#define LINUX_EMSGSIZE		40	/* Message too long */
#define LINUX_EPROTOTYPE	41	/* Protocol wrong type for socket */
#define LINUX_ENOPROTOOPT	42	/* Protocol not available */
#define LINUX_EPROTONOSUPPORT	43	/* Protocol not supported */
#define LINUX_ESOCKTNOSUPPORT	44	/* Socket type not supported */
#define LINUX_EOPNOTSUPP	45	/* Operation not supported on transport endpoint */
#define LINUX_EPFNOSUPPORT	46	/* Protocol family not supported */
#define LINUX_EAFNOSUPPORT	47	/* Address family not supported by protocol */
#define LINUX_EADDRINUSE	48	/* Address already in use */
#define LINUX_EADDRNOTAVAIL	49	/* Cannot assign requested address */
#define LINUX_ENETDOWN		50	/* Network is down */
#define LINUX_ENETUNREACH	51	/* Network is unreachable */
#define LINUX_ENETRESET		52	/* Network connection reset */
#define LINUX_ECONNABORTED	53	/* Software caused connection abort */
#define LINUX_ECONNRESET	54	/* Connection reset by peer */
#define LINUX_ENOBUFS		55	/* No buffer space available */
#define LINUX_EISCONN		56	/* Transport endpoint is already connected */
#define LINUX_ENOTCONN		57	/* Transport endpoint is not connected */
#define LINUX_ESHUTDOWN		58	/* Cannot send after transport endpoint shutdown */
#define LINUX_ETOOMANYREFS	59	/* Too many references: cannot splice */
#define LINUX_ETIMEDOUT		60	/* Connection timed out */
#define LINUX_ECONNREFUSED	61	/* Connection refused */
#define LINUX_ELOOP		62	/* Too many symbolic links encountered */
#define LINUX_ENAMETOOLONG	63	/* File name too long */
#define LINUX_EHOSTDOWN		64	/* Host is down */
#define LINUX_EHOSTUNREACH	65	/* No route to host */
#define LINUX_ENOTEMPTY		66	/* Directory not empty */

#define LINUX_EUSERS		68	/* Too many users */
#define LINUX_EDQUOT		69	/* Quota exceeded */
#define LINUX_ESTALE		70	/* Stale NFS file handle */
#define LINUX_EREMOTE		71	/* Object is remote */

#define LINUX_ENOLCK		77	/* No record locks available */
#define LINUX_ENOSYS		78	/* Function not implemented */

#define LINUX_ENOMSG		80	/* No message of desired type */
#define LINUX_EIDRM		81	/* Identifier removed */
#define LINUX_ENOSR		82	/* Out of streams resources */
#define LINUX_ETIME		83	/* Timer expired */
#define LINUX_EBADMSG		84	/* Not a data message */
#define LINUX_EPROTO		85	/* Protocol error */
#define LINUX_ENODATA		86	/* No data available */
#define LINUX_ENOSTR		87	/* Device not a stream */

#define LINUX_ENOPKG		92	/* Package not installed */

#define LINUX_EILSEQ		116	/* Illegal byte sequence */

/* The following are just random noise.. */
#define LINUX_ECHRNG		88	/* Channel number out of range */
#define LINUX_EL2NSYNC		89	/* Level 2 not synchronized */
#define LINUX_EL3HLT		90	/* Level 3 halted */
#define LINUX_EL3RST		91	/* Level 3 reset */

#define LINUX_ELNRNG		93	/* Link number out of range */
#define LINUX_EUNATCH		94	/* Protocol driver not attached */
#define LINUX_ENOCSI		95	/* No CSI structure available */
#define LINUX_EL2HLT		96	/* Level 2 halted */
#define LINUX_EBADE		97	/* Invalid exchange */
#define LINUX_EBADR		98	/* Invalid request descriptor */
#define LINUX_EXFULL		99	/* Exchange full */
#define LINUX_ENOANO		100	/* No anode */
#define LINUX_EBADRQC		101	/* Invalid request code */
#define LINUX_EBADSLT		102	/* Invalid slot */

#define LINUX_EDEADLOCK	LINUX_EDEADLK

#define LINUX_EBFONT		104	/* Bad font file format */
#define LINUX_ENONET		105	/* Machine is not on the network */
#define LINUX_ENOLINK		106	/* Link has been severed */
#define LINUX_EADV		107	/* Advertise error */
#define LINUX_ESRMNT		108	/* Srmount error */
#define LINUX_ECOMM		109	/* Communication error on send */
#define LINUX_EMULTIHOP		110	/* Multihop attempted */
#define LINUX_EDOTDOT		111	/* RFS specific error */
#define LINUX_EOVERFLOW		112	/* Value too large for defined data type */
#define LINUX_ENOTUNIQ		113	/* Name not unique on network */
#define LINUX_EBADFD		114	/* File descriptor in bad state */
#define LINUX_EREMCHG		115	/* Remote address changed */

#define LINUX_EUCLEAN		117	/* Structure needs cleaning */
#define LINUX_ENOTNAM		118	/* Not a XENIX named type file */
#define LINUX_ENAVAIL		119	/* No XENIX semaphores available */
#define LINUX_EISNAM		120	/* Is a named type file */
#define LINUX_EREMOTEIO		121	/* Remote I/O error */

#define LINUX_ELIBACC		122	/* Can't access a shared library */
#define LINUX_ELIBBAD		123	/* Accessing a corrupted shared library */
#define LINUX_ELIBSCN		124	/* .lib section in a.out corrupted */
#define LINUX_ELIBMAX		125	/* Link in too many shared libraries */
#define LINUX_ELIBEXEC		126	/* Cannot exec a shared library directly */
#define LINUX_ERESTART		127	/* Restart interrupted system call */
#define LINUX_ESTRPIPE		128	/* Streams pipe error */

#define LINUX_ENOMEDIUM		129	/* No medium found */
#define LINUX_EMEDIUMTYPE	130	/* Wrong medium type */

#endif /* !_ALPHA_LINUX_ERRNO_H */
