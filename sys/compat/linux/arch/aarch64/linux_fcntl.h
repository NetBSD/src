/*	$NetBSD: linux_fcntl.h,v 1.1 2021/09/23 06:56:27 ryo Exp $	*/

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
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

#ifndef _AARCH64_LINUX_FCNTL_H
#define _AARCH64_LINUX_FCNTL_H

#define LINUX_O_CREAT		0x00000040
#define LINUX_O_EXCL		0x00000080
#define LINUX_O_NOCTTY		0x00000100
#define LINUX_O_TRUNC		0x00000200
#define LINUX_O_APPEND		0x00000400
#define LINUX_O_NONBLOCK	0x00000800
#define LINUX_O_NDELAY		LINUX_O_NONBLOCK
#define LINUX_O_SYNC		0x00001000
#define LINUX_FASYNC		0x00002000
#define LINUX_O_DIRECTORY	0x00004000
#define LINUX_O_NOFOLLOW	0x00008000
#define LINUX_O_DIRECT		0x00010000
#define LINUX_O_NOATIME		0x00040000
#define LINUX_O_CLOEXEC 	0x00080000

#define LINUX_F_DUPFD		0
#define LINUX_F_GETFD		1
#define LINUX_F_SETFD		2
#define LINUX_F_GETFL		3
#define LINUX_F_SETFL		4
#define LINUX_F_GETLK		5
#define LINUX_F_GETLK64 	LINUX_F_GETLK
#define LINUX_F_SETLK		6
#define LINUX_F_SETLK64 	LINUX_F_SETLK
#define LINUX_F_SETLKW		7
#define LINUX_F_SETLKW64	LINUX_F_SETLKW
#define LINUX_F_SETOWN		8
#define LINUX_F_GETOWN		9

#define LINUX_F_RDLCK		0
#define LINUX_F_WRLCK		1
#define LINUX_F_UNLCK		2
#define LINUX_F_EXLCK		4
#define LINUX_F_SHLCK		8

#define LINUX_LOCK_SH		0x00000001
#define LINUX_LOCK_EX		0x00000002
#define LINUX_LOCK_NB		0x00000004
#define LINUX_LOCK_UN		0x00000008

#endif /* !_AARCH64_LINUX_FCNTL_H */
