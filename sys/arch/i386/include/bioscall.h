/*	$NetBSD: bioscall.h,v 1.5 1998/10/03 02:14:52 jtk Exp $ */
/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by John Kohl.
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
#ifndef __I386_BIOSCALL_H__
#define __I386_BIOSCALL_H__

/*
 * virtual & physical address of the trampoline
 * that we use: page 1.
 */
#define BIOSTRAMP_BASE	NBPG

#ifndef _LOCORE
typedef union bios_register {
	struct {
		u_short hw_lo;
		u_short hw_hi;
	} halfword;
	u_int longword;
} bios_reg;

struct bioscallregs {
    bios_reg r_ax;
    bios_reg r_bx;
    bios_reg r_cx;
    bios_reg r_dx;
    bios_reg r_si;
    bios_reg r_di;
    bios_reg r_flags;
};
#define AX r_ax.halfword.hw_lo
#define AX_HI r_ax.halfword.hw_hi
#define EAX r_ax.longword
#define BX r_bx.halfword.hw_lo
#define BX_HI r_bx.halfword.hw_hi
#define EBX r_bx.longword
#define CX r_cx.halfword.hw_lo
#define CX_HI r_cx.halfword.hw_hi
#define ECX r_cx.longword
#define DX r_dx.halfword.hw_lo
#define DX_HI r_dx.halfword.hw_hi
#define EDX r_dx.longword
#define SI r_si.halfword.hw_lo
#define SI_HI r_si.halfword.hw_hi
#define ESI r_si.longword
#define DI r_di.halfword.hw_lo
#define DI_HI r_di.halfword.hw_hi
#define EDI r_di.longword
#define FLAGS r_flags.halfword.hw_lo
#define FLAGS_HI r_flags.halfword.hw_hi
#define EFLAGS r_flags.longword

void bioscall __P((int /* function*/ , struct bioscallregs * /* regs */));
#endif
#endif /* __I386_BIOSCALL_H__ */
