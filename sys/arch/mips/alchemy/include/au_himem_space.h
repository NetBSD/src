/* $NetBSD: au_himem_space.h,v 1.1.10.2 2006/04/22 11:37:42 simonb Exp $ */

/*-
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Written by Garrett D'Amore for Itronix Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */ 

#ifndef _MIPS_ALCHEMY_AU_HIMEM_SPACE_H_
#define	_MIPS_ALCHEMY_AU_HIMEM_SPACE_H_

/* bus endianness */
#define	AU_HIMEM_SPACE_BIG_ENDIAN	(1 << 0)
#define	AU_HIMEM_SPACE_LITTLE_ENDIAN	(1 << 1)

/* endian swapping done in hardware? */
#define	AU_HIMEM_SPACE_SWAP_HW		(1 << 2)

/* chip type */
#define	AU_HIMEM_SPACE_IO		(1 << 3)	/* no linear mapping */

extern void au_himem_space_init(bus_space_tag_t, const char *, paddr_t,
    bus_addr_t, bus_addr_t, int);

#endif /* _MIPS_ALCHEMY_AU_WIRED_SPACE_H_ */
