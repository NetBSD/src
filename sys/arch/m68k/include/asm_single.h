/*	$NetBSD: asm_single.h,v 1.3.28.1 2002/02/11 20:08:30 jdolecek Exp $	*/

/*
 * Copyright (c) 1996 Leo Weppelman.
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
 *      This product includes software developed by Leo Weppelman.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _M68K_ASM_SINGLE_H
#define _M68K_ASM_SINGLE_H
/*
 * Provide bit manipulation macro's that resolve to a single instruction.
 * These can be considered atomic on single processor architectures when
 * no page faults can occur when acessing <var>.
 * There primary use is to avoid race conditions when manipulating device
 * registers.
 */

#define single_inst_bset_b(var, bit)	\
	__asm __volatile ("orb %1,%0"	\
		: "=m" (var)		\
		: "di" ((u_char)bit), "0" (var))

#define single_inst_bclr_b(var, bit)	\
	__asm __volatile ("andb %1,%0"	\
		: "=m" (var)		\
		: "di" ((u_char)~(bit)), "0" (var))


#define single_inst_bset_w(var, bit)	\
	__asm __volatile ("orw %1,%0"	\
		: "=m" (var)		\
		: "di" ((u_short)bit), "0" (var))

#define single_inst_bclr_w(var, bit)	\
	__asm __volatile ("andw %1,%0"	\
		: "=m" (var)		\
		: "di" ((u_short)~(bit)), "0" (var))


#define single_inst_bset_l(var, bit)	\
	__asm __volatile ("orl %1,%0"	\
		: "=m" (var)		\
		: "di" ((u_long)bit), "0" (var))

#define single_inst_bclr_l(var, bit)	\
	__asm __volatile ("andl %1,%0"	\
		: "=m" (var)		\
		: "di" ((u_long)~(bit)), "0" (var))

#endif /* _M68K_ASM_SINGLE_H */
