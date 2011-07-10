/* $NetBSD: arbusle.c,v 1.1 2011/07/10 06:26:02 matt Exp $ */

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: arbusle.c,v 1.1 2011/07/10 06:26:02 matt Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/endian.h>

#if _BYTE_ORDER == _BIG_ENDIAN

#include <mips/atheros/include/arbusvar.h>

#define CHIP                    arbusle
#define CHIP_MEM                /* defined */
#define	CHIP_LITTLE_ENDIAN	/* defined */
#define CHIP_W1_BUS_START(v)    0x00000000UL
#define CHIP_W1_BUS_END(v)      0x1fffffffUL
#define CHIP_W1_SYS_START(v)    CHIP_W1_BUS_START(v)
#define CHIP_W1_SYS_END(v)      CHIP_W1_BUS_END(v)
//#define	CHIP_ACCESS_SIZE	4

#include <mips/mips/bus_space_alignstride_chipdep.c>
#endif /* _BYTE_ORDER == _BIG_ENDIAN */
