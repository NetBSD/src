/* $NetBSD: start.S,v 1.1 2004/04/17 23:20:37 cl Exp $ */

/*
 *
 * Copyright (c) 2004 Christian Limpach.
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
 *      This product includes software developed by Christian Limpach.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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


#define	_LOCORE

#include <machine/xen.h>

	.data

	.space 8192
tmpstk:
	.long tmpstk, __KERNEL_DS

	.text

	.globl	text_start
text_start:

	.globl	load_start
load_start:
	cld

	lss	tmpstk,%esp
	movl	%esi,start_info		# save start_info pointer

	jmp	prepare

	.globl	do_reloc
do_reloc:
    	movl	4(%esp),%edi
	movl	%edi,%edx		# save high loader address
	movl	$loader,%esi
	movl	$loader_end,%ecx
	subl	%esi,%ecx
	rep movsb

	movl	%edx,%ecx
	movl	8(%esp),%esi
	subl	%esi,%ecx
	movl	$text_start,%edi
	movl	start_info,%ebx
	jmp	*%edx			# call loader in high memory

loader:
	cld
	rep movsb
	movl	%ebx,%esi		# restore start_info pointer
	xorl	%eax,%eax
	xorl	%ebx,%ebx
	xorl	%ecx,%ecx
	movl	$text_start,%edx
	jmp	*%edx
loader_end:
