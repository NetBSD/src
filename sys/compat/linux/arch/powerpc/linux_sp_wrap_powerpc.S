/* $NetBSD: linux_sp_wrap_powerpc.S,v 1.1 2001/01/19 01:36:51 manu Exp $ */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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
 * This product includes software developed by the NetBSD
 * Foundation, Inc. and its contributors.
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

#include <machine/asm.h>

/* 
 * See linux_exec_powerpc.c. Since the executable expects its arguments 16 
 * bytes lower than the place ld.so expects them, and since it was not 
 * possible to just duplicate the argument (ld.so erases the pace where
 * the executable looks for the arguments), then we try to adjust the
 * stack pointer after ld.so has done its job, and before the executable
 * is actually run.
 *
 * This code is copied onto the stack before program startup, then
 * ld.so is fooled in thinking this is the actual program. This code adjust
 * the stack pointer so that the programm will find its arguments and then 
 * transfer control to the real program. The adjustment is of 16 bytes 
 *
 * XXX This is really a ugly hack, and it should die.
 */

#define LINUX_SP_WRAP_OFFSET 0x10

.globl	_C_LABEL(linux_sp_wrap_start)
.globl	_C_LABEL(linux_sp_wrap_end)
.globl	_C_LABEL(linux_sp_wrap_entry)
_C_LABEL(linux_sp_wrap_start):
	addi		1,1,LINUX_SP_WRAP_OFFSET
	mflr		12
	bl			1f
1:
	mflr		11
	mtlr		12
	lwz		12, _C_LABEL(linux_sp_wrap_entry)-1b(11)
	mtctr		12
	bctr
	mtctr		11
	bctr
_C_LABEL(linux_sp_wrap_entry):	
.long		0				/* orginal prog entry point. setup by kernel */
_C_LABEL(linux_sp_wrap_end):
