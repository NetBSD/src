/*	$NetBSD: cpuregs.h,v 1.2.2.3 2002/02/28 04:10:43 nathanw Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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

#ifdef _KERNEL
/* R5900 has Enable Interrupt Enable bit. */
#undef	MIPS_SR_INT_IE
#define MIPS_SR_INT_IE			0x00010001	/* EIE + IE */

#undef COP0_SYNC
#define COP0_SYNC	sync.p 

/* 
 * R5900 has INT5,1,0 only don't support software interrupt 
 * MIPS_SOFT_INT_MASK_1 and MIPS_SOFT_INT_MASK_1 are emulated by kernel.
 */
#undef MIPS_INT_MASK
#define MIPS_INT_MASK		0x0c00
#undef MIPS_HARD_INT_MASK
#define MIPS_HARD_INT_MASK	0x0c00

#undef MIPS_COP_0_LLADDR
#undef MIPS_COP_0_WATCH_LO
#undef MIPS_COP_0_WATCH_HI
#undef MIPS_COP_0_TLB_XCONTEXT
#undef MIPS_COP_0_ECC
#undef MIPS_COP_0_CACHE_ERR
#undef MIPS_COP_0_DESAVE

/* Exception vector */
#define R5900_TLB_REFIL_EXC_VEC		0x80000000
#define R5900_COUNTER_EXC_VEC		0x80000080
#define R5900_DEBUG_EXC_VEC		0x80000100
#define R5900_COMMON_EXC_VEC		0x80000180
#define R5900_INTERRUPT_EXC_VEC		0x80000200

/* MMU  R5900 support 32bit-mode only */
#define dmtc0	mtc0
#define dmfc0	mfc0

#endif /* _KERNEL */
