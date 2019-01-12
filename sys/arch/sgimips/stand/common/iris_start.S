/*	$NetBSD: iris_start.S,v 1.1 2019/01/12 16:44:47 tsutsui Exp $	*/

/*
 * Copyright (c) 2018 Naruaki Etomi
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
 * Silicon Graphics "IRIS" series MIPS processors machine bootloader.
 */

#include <mips/asm.h>
#include <mips/cpuregs.h>

	.globl	start
start:
	.set	noreorder
#ifdef __GP_SUPPORT__
	la	gp, _C_LABEL(_gp)
#endif
	/*
	 * We assume the bootprom isn't expecting us to preserve the saved
	 * registers.
	 */
	move	s0, a0				# save argc
	move	s1, a1				# save argv
	move	s2, a2				# save envp
	move	s3, ra				# save RA

	la	a0, _C_LABEL(edata)		# clear BSS
	move	a1, zero
	la	a2, _C_LABEL(end)
	jal	_C_LABEL(memset)		# memset(edata, 0, end - edata)
	 subu	a2, a2, a0

	move	a0, s0				# restore argc
	move	a1, s1				# restore argv
	move	a2, s2				# restore envp
	j	_C_LABEL(main)			# main(argc, argv)
	 move	ra, s3				# restore RA

LEAF(romrestart)
XLEAF(_rtt)
	PTR_LA	v0, 0xBFC00000			# indigo RESET
	j	v0
	 nop
END(romrestart)
