/*	$NetBSD: linux_sigcode.S,v 1.1.2.3 2001/04/21 17:54:32 bouyer Exp $ */

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * The following code is just a copy from sys/arch/powerpc/powerpc/sigcode.S 
 * Obviously, there are little chance that it works for Linux emulation, so 
 * it will have to be rewritten. 
 * Emmanuel Dreyfus <p99dreyf@criens.u-psud.fr>
 */

#include <machine/asm.h>

#include <compat/linux/linux_syscall.h>

/*
 * This is copied from sys/compat/linux/arch/powerpc/linux_machdep.h
 */
#define LINUX__SIGNAL_FRAMESIZE 64

/*
 * The following code gets copied to the top of the user stack on process
 * execution.  It does signal trampolining on signal delivery.
 *
 * On entry r1 points to a struct sigframe at bottom of current stack.
 * All other registers are unchanged.
 * 
 * XXX Linux just does this: 
 *  0x38007777   li r0,0x7777
 *  0x44000002   sc
 */
	.globl	_C_LABEL(linux_sigcode),_C_LABEL(linux_esigcode)
_C_LABEL(linux_sigcode):
	addi	1,1,-16			/* reserved space for callee */
	blrl
	addi	3,1,16+LINUX__SIGNAL_FRAMESIZE	/* compute &sf_sc */
	li	0,LINUX_SYS_sigreturn
	sc				/* linux_sys_sigreturn(scp) */
	li	0,LINUX_SYS_exit
	sc				/* linux_sys_exit(errno) */
_C_LABEL(linux_esigcode):
