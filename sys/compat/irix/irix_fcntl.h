/*	$NetBSD: irix_fcntl.h,v 1.2 2002/06/01 16:16:36 manu Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#ifndef	_IRIX_FCNTL_H_
#define	_IRIX_FCNTL_H_

#include <compat/svr4/svr4_fcntl.h>

/* From IRIX's <sys/fcntl.h> */
#define IRIX_F_DUPFD		SVR4_F_DUPFD
#define IRIX_F_GETFD		SVR4_F_GETFD
#define IRIX_F_SETFD		SVR4_F_SETFD
#define IRIX_F_GETFL		SVR4_F_GETFL
#define IRIX_F_SETFL		SVR4_F_SETFL
#define IRIX_F_SETLK		SVR4_F_SETLK
#define IRIX_F_SETLKW		SVR4_F_SETLKW
#define IRIX_F_CHKFL		SVR4_F_CHKFL
#define IRIX_F_ALLOCSP		SVR4_F_ALLOCSP
#define IRIX_F_FREESP		SVR4_F_FREESP
#define IRIX_F_SETBSDLK		12
#define IRIX_F_SETBSDLKW	13
#define IRIX_F_GETLK		SVR4_F_GETLK
#define IRIX_F_CHKLK		15
#define IRIX_F_CHKLKW		16
#define IRIX_F_CLNLK		17
#define IRIX_F_RSETLK		SVR4_F_RSETLK
#define IRIX_F_RGETLK		SVR4_F_RGETLK
#define IRIX_F_RSETLKW		SVR4_F_RSETLKW
#define IRIX_F_GETOWN		SVR4_F_GETOWN
#define IRIX_F_SETOWN		SVR4_F_SETOWN
#define IRIX_F_DIOINFO		30
#define IRIX_F_FSGETXATTR	31
#define IRIX_F_FSSETXATTR	32
#define IRIX_F_GETLK64		SVR4_F_GETLK64
#define IRIX_F_SETLK64		SVR4_F_SETLK64
#define IRIX_F_SETLKW64		SVR4_F_SETLKW64
#define IRIX_F_ALLOCSP64	36
#define IRIX_F_FREESP64		SVR4_F_FREESP64
#define IRIX_F_GETBMAP		38
#define IRIX_F_FSSETDM		39
#define IRIX_F_RESVSP		40
#define IRIX_F_UNRESVSP		41
#define IRIX_F_RESVSP64		42
#define IRIX_F_UNRESVSP64	43
#define IRIX_F_GETBMAPA		44
#define IRIX_F_FSGETXATTRA	45
#define IRIX_F_SETBIOSIZE	46
#define IRIX_F_GETBIOSIZE	47
#define IRIX_F_GETOPS		50
#define IRIX_F_DMAPI		51
#define IRIX_F_FSYNC		52
#define IRIX_F_FSYNC64		53
#define IRIX_F_GETBDSATTR	54
#define IRIX_F_SETBDSATTR	55
#define IRIX_F_GETBMAPX		56
#define IRIX_F_SETPRIO		57
#define IRIX_F_GETPRIO		58

/* fcntl F_GETFL flags */
#define IRIX_FNDELAY		0x000004
#define IRIX_FAPPEND		0x000008
#define IRIX_FSYNC		0x000010
#define IRIX_FDSYNC		0x000020
#define IRIX_FRSYNC		0x000040
#define IRIX_FNONBLOCK		0x000080
#define IRIX_FASYNC		0x001000
#define IRIX_FLARGEFILE		0x002000
#define IRIX_FDIRECT		0x008000
#define IRIX_FBULK		0x010000
#define IRIX_FLCINVAL		0x020000
#define IRIX_FLCFLUSH		0x040000

#endif /* _IRIX_FCNTL_H_ */
