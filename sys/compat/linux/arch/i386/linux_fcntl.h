/*	$NetBSD: linux_fcntl.h,v 1.5.20.2 2010/11/06 08:08:24 uebayasi Exp $	*/

/*-
 * Copyright (c) 1995, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden and Eric Haszlakiewicz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

/*
 * Various flag values used in Linux for open(2) and fcntl(2).
 */

#ifndef _I386_LINUX_FCNTL_H
#define _I386_LINUX_FCNTL_H

/* read/write mode for open(2) defined in common/linux_fcntl.h */

/* flags used in open(2) */
#define LINUX_O_CREAT		0x00040
#define LINUX_O_EXCL		0x00080
#define LINUX_O_NOCTTY		0x00100
#define LINUX_O_TRUNC		0x00200
#define LINUX_O_APPEND		0x00400
#define LINUX_O_NONBLOCK	0x00800
#define LINUX_O_NDELAY		LINUX_O_NONBLOCK
#define LINUX_O_SYNC		0x01000
#define LINUX_FASYNC		0x02000
#define LINUX_O_DIRECTORY	0x10000
#define LINUX_O_CLOEXEC		0x80000

/* fcntl(2) operations */
#define LINUX_F_DUPFD		0
#define LINUX_F_GETFD		1
#define LINUX_F_SETFD		2
#define LINUX_F_GETFL		3
#define LINUX_F_SETFL		4
#define LINUX_F_GETLK		5
#define LINUX_F_SETLK		6
#define LINUX_F_SETLKW		7
#define LINUX_F_SETOWN		8
#define LINUX_F_GETOWN		9

#define LINUX_F_RDLCK		0
#define LINUX_F_WRLCK		1
#define LINUX_F_UNLCK		2

#define LINUX_LOCK_EX		4
#define LINUX_LOCK_SH		8

#define LINUX_F_GETLK64		12
#define LINUX_F_SETLK64		13
#define LINUX_F_SETLKW64	14

#endif /* !_I386_LINUX_FCNTL_H */
