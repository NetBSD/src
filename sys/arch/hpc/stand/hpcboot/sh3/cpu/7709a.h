/*	$NetBSD: 7709a.h,v 1.1 2002/02/11 17:08:57 uch Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

#ifndef _HPCBOOT_SH_CPU_7709A_H_
#define _HPCBOOT_SH_CPU_7709A_H_

#define SH7709A_CACHE_LINESZ		16
#define SH7709A_CACHE_ENTRY		256
#define SH7709A_CACHE_WAY		4
#define SH7709A_CACHE_SIZE						\
	(SH7709A_CACHE_LINESZ * SH7709A_CACHE_ENTRY * SH7709A_CACHE_WAY)

#define SH7709A_CACHE_ENTRY_SHIFT	4
#define SH7709A_CACHE_ENTRY_MASK	0x00000ff0
#define SH7709A_CACHE_WAY_SHIFT		12
#define SH7709A_CACHE_WAY_MASK		0x00003000

#define SH7709A_CACHE_FLUSH()						\
__BEGIN_MACRO								\
	u_int32_t __e, __w, __wa, __a;					\
									\
	for (__w = 0; __w < SH7709A_CACHE_WAY; __w++) {			\
		__wa = SH3_CCA | __w << SH7709A_CACHE_WAY_SHIFT;	\
		for (__e = 0; __e < SH7709A_CACHE_ENTRY; __e++)	{	\
			__a = __wa |(__e << SH7709A_CACHE_ENTRY_SHIFT);	\
			_reg_read_4(__a) &= ~0x3; /* Clear U,V bit */	\
		}							\
	}								\
__END_MACRO

#define SH7709A_MMU_DISABLE	SH3_MMU_DISABLE

#endif // _HPCBOOT_SH_CPU_7709A_H_
