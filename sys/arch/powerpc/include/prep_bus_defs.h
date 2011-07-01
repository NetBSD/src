/* $NetBSD: prep_bus_defs.h,v 1.1 2011/07/01 17:29:39 dyoung Exp $ */

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour
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

#ifndef _POWERPC_PREP_BUS_DEFS_H_
#define _POWERPC_PREP_BUS_DEFS_H_

#define PREP_BUS_SPACE_IO		0x80000000
#define PREP_BUS_SPACE_MEM		0xC0000000

#define PHYS_TO_BUS_MEM(t,x)		((x) | PREP_BUS_SPACE_IO)
#define BUS_MEM_TO_PHYS(t,x)		((x) & ~PREP_BUS_SPACE_IO)

#define PREP_PHYS_SIZE_IO		0x3f800000
#define PREP_PHYS_SIZE_MEM		0x3f000000
#define PREP_ISA_SIZE_IO		0x00010000
#define PREP_ISA_SIZE_MEM		0x01000000
#define PREP_EISA_SIZE_IO		0x0000f000
#define PREP_EISA_SIZE_MEM		0x3f000000

#define PREP_PHYS_RESVD_START_IO	0x10000
#define PREP_PHYS_RESVD_SIZE_IO		0x7F0000

#endif /* _POWERPC_PREP_BUS_DEFS_H_ */
