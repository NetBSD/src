/*	$NetBSD: svr4_mman.h,v 1.2.16.1 2002/03/16 16:00:40 jdolecek Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
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

#ifndef _SVR4_MMAN_H_
#define _SVR4_MMAN_H_

/*
 * Commands and flags passed to memcntl().
 * Most of them are the same as <sys/mman.h>, but we need the MC_
 * memcntl command definitions.
 */

#define SVR4_MC_SYNC		1
#define SVR4_MC_LOCK		2
#define SVR4_MC_UNLOCK		3
#define SVR4_MC_ADVISE		4
#define SVR4_MC_LOCKAS		5
#define SVR4_MC_UNLOCKAS	6

/*
 * Flags passed to traditional mmap() syscall.  The type (SHARED/PRIVATE)
 * is the same as in NetBSD.
 */

#define SVR4_MAP_TYPE		0x00f
#define SVR4_MAP_COPYFLAGS	0x07f	/* flags same as NetBSD */
#define SVR4_MAP_ANON		0x100

#endif /* !_SVR4_MMAN_H */
