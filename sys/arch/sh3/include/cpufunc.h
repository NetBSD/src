/*	$NetBSD: cpufunc.h,v 1.11 2002/02/17 20:55:52 uch Exp $	*/

/*
 * Copyright (c) 1993 Charles Hannum.
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
 *      This product includes software developed by Charles Hannum.
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

/*
 * SH3 Version
 *
 * T.Horiuchi Brains Corp. 05/22/1998
 */

#ifndef _SH3_CPUFUNC_H_
#define	_SH3_CPUFUNC_H_

/*
 * Functions to provide access to SH3-specific instructions.
 */

#include <sys/cdefs.h>
#include <sys/types.h>

#ifdef _KERNEL

void enable_ext_intr(void);
void disable_ext_intr(void);
static __inline void breakpoint(void);

/*
 * memory-mapped register access method.
 */
#define	_reg_read_1(a)		(*(__volatile__ u_int8_t *)((vaddr_t)(a)))
#define	_reg_read_2(a)		(*(__volatile__ u_int16_t *)((vaddr_t)(a)))
#define	_reg_read_4(a)		(*(__volatile__ u_int32_t *)((vaddr_t)(a)))
#define	_reg_write_1(a, v)						\
	(*(__volatile__ u_int8_t *)(a) = (u_int8_t)(v))
#define	_reg_write_2(a, v)						\
	(*(__volatile__ u_int16_t *)(a) = (u_int16_t)(v))
#define	_reg_write_4(a, v)						\
	(*(__volatile__ u_int32_t *)(a) = (u_int32_t)(v))

/* XXXX ought to be in psl.h with spl() functions */

static __inline void
disable_intr(void)
{

	__asm __volatile("mov.l r0, @-r15");
	__asm __volatile("mov.l r1, @-r15");
	__asm __volatile("mov #0x10, r0");
	__asm __volatile("swap.b r0, r0");
	__asm __volatile("swap.w r0, r0");
	__asm __volatile("stc sr, r1");
	__asm __volatile("or r0, r1");
	__asm __volatile("ldc r1, sr");
	__asm __volatile("mov.l @r15+, r1");
	__asm __volatile("mov.l @r15+, r0");
}

static __inline void
enable_intr(void)
{

	__asm __volatile("mov.l r0, @-r15");
	__asm __volatile("mov.l r1, @-r15");
	__asm __volatile("mov #0x10, r0");
	__asm __volatile("swap.b r0, r0");
	__asm __volatile("swap.w r0, r0");
	__asm __volatile("not r0, r0");
	__asm __volatile("stc sr, r1");
	__asm __volatile("and r0, r1");
	__asm __volatile("ldc r1, sr");
	__asm __volatile("mov.l @r15+, r1");
	__asm __volatile("mov.l @r15+, r0");
}

static __inline void
breakpoint()
{

	__asm __volatile("trapa #0xc3");
}

#endif /* _KERNEL */
#endif /* !_SH3_CPUFUNC_H_ */
