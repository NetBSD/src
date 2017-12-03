/*	$NetBSD: linux_mmap.h,v 1.4.44.1 2017/12/03 11:36:53 jdolecek Exp $	*/

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

#ifndef _ALPHA_LINUX_MMAP_H
#define _ALPHA_LINUX_MMAP_H

/* LINUX_PROT_* defined in common/linux_mmap.h */

/* LINUX_MAP_SHARED/PRIVATE defined in common/linux_mmap.h */

#define LINUX_MAP_ANON		0x00010
#define LINUX_MAP_FIXED		0x00100
#define LINUX_MAP_LOCKED	0x08000

/* the following flags are silently ignored */

#define LINUX_MAP_HASSEMAPHORE	0x00200
#define LINUX_MAP_INHERIT	0x00400
#define LINUX_MAP_UNALIGNED	0x00800

#define LINUX_MAP_GROWSDOWN	0x01000
#define LINUX_MAP_DENYWRITE	0x02000
#define	LINUX_MAP_EXECUTABLE	0x04000

#define LINUX_MAP_NORESERVE	0x10000

#endif /* !_ALPHA_LINUX_MMAP_H */
