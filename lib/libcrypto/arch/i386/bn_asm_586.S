/*	$NetBSD: bn_asm_586.S,v 1.1 2000/07/31 19:57:31 thorpej Exp $	*/

/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscapes SSL.
 * 
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@cryptsoft.com).
 * 
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from 
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 * 
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */

/*
 * Modified from the output of `perl bn-586.pl elf' by
 * Jason R. Thorpe <thorpej@zembu.com>.
 */

#include <machine/asm.h>

ENTRY(bn_mul_add_words)
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi


	xorl	%esi,		%esi
	movl	20(%esp),	%edi
	movl	28(%esp),	%ecx
	movl	24(%esp),	%ebx
	andl	$4294967288,	%ecx
	movl	32(%esp),	%ebp
	pushl	%ecx
	jz	L000maw_finish
L001maw_loop:
	movl	%ecx,		(%esp)
	/* Round 0 */
	movl	(%ebx),		%eax
	mull	%ebp
	addl	%esi,		%eax
	movl	(%edi),		%esi
	adcl	$0,		%edx
	addl	%esi,		%eax
	adcl	$0,		%edx
	movl	%eax,		(%edi)
	movl	%edx,		%esi
	/* Round 4 */
	movl	4(%ebx),	%eax
	mull	%ebp
	addl	%esi,		%eax
	movl	4(%edi),	%esi
	adcl	$0,		%edx
	addl	%esi,		%eax
	adcl	$0,		%edx
	movl	%eax,		4(%edi)
	movl	%edx,		%esi
	/* Round 8 */
	movl	8(%ebx),	%eax
	mull	%ebp
	addl	%esi,		%eax
	movl	8(%edi),	%esi
	adcl	$0,		%edx
	addl	%esi,		%eax
	adcl	$0,		%edx
	movl	%eax,		8(%edi)
	movl	%edx,		%esi
	/* Round 12 */
	movl	12(%ebx),	%eax
	mull	%ebp
	addl	%esi,		%eax
	movl	12(%edi),	%esi
	adcl	$0,		%edx
	addl	%esi,		%eax
	adcl	$0,		%edx
	movl	%eax,		12(%edi)
	movl	%edx,		%esi
	/* Round 16 */
	movl	16(%ebx),	%eax
	mull	%ebp
	addl	%esi,		%eax
	movl	16(%edi),	%esi
	adcl	$0,		%edx
	addl	%esi,		%eax
	adcl	$0,		%edx
	movl	%eax,		16(%edi)
	movl	%edx,		%esi
	/* Round 20 */
	movl	20(%ebx),	%eax
	mull	%ebp
	addl	%esi,		%eax
	movl	20(%edi),	%esi
	adcl	$0,		%edx
	addl	%esi,		%eax
	adcl	$0,		%edx
	movl	%eax,		20(%edi)
	movl	%edx,		%esi
	/* Round 24 */
	movl	24(%ebx),	%eax
	mull	%ebp
	addl	%esi,		%eax
	movl	24(%edi),	%esi
	adcl	$0,		%edx
	addl	%esi,		%eax
	adcl	$0,		%edx
	movl	%eax,		24(%edi)
	movl	%edx,		%esi
	/* Round 28 */
	movl	28(%ebx),	%eax
	mull	%ebp
	addl	%esi,		%eax
	movl	28(%edi),	%esi
	adcl	$0,		%edx
	addl	%esi,		%eax
	adcl	$0,		%edx
	movl	%eax,		28(%edi)
	movl	%edx,		%esi

	movl	(%esp),		%ecx
	addl	$32,		%ebx
	addl	$32,		%edi
	subl	$8,		%ecx
	jnz	L001maw_loop
L000maw_finish:
	movl	32(%esp),	%ecx
	andl	$7,		%ecx
	jnz	L002maw_finish2
	jmp	L003maw_end
_ALIGN_TEXT
L002maw_finish2:
	/* Tail Round 0 */
	movl	(%ebx),		%eax
	mull	%ebp
	addl	%esi,		%eax
	movl	(%edi),		%esi
	adcl	$0,		%edx
	addl	%esi,		%eax
	adcl	$0,		%edx
	decl	%ecx
	movl	%eax,		(%edi)
	movl	%edx,		%esi
	jz	L003maw_end
	/* Tail Round 1 */
	movl	4(%ebx),	%eax
	mull	%ebp
	addl	%esi,		%eax
	movl	4(%edi),	%esi
	adcl	$0,		%edx
	addl	%esi,		%eax
	adcl	$0,		%edx
	decl	%ecx
	movl	%eax,		4(%edi)
	movl	%edx,		%esi
	jz	L003maw_end
	/* Tail Round 2 */
	movl	8(%ebx),	%eax
	mull	%ebp
	addl	%esi,		%eax
	movl	8(%edi),	%esi
	adcl	$0,		%edx
	addl	%esi,		%eax
	adcl	$0,		%edx
	decl	%ecx
	movl	%eax,		8(%edi)
	movl	%edx,		%esi
	jz	L003maw_end
	/* Tail Round 3 */
	movl	12(%ebx),	%eax
	mull	%ebp
	addl	%esi,		%eax
	movl	12(%edi),	%esi
	adcl	$0,		%edx
	addl	%esi,		%eax
	adcl	$0,		%edx
	decl	%ecx
	movl	%eax,		12(%edi)
	movl	%edx,		%esi
	jz	L003maw_end
	/* Tail Round 4 */
	movl	16(%ebx),	%eax
	mull	%ebp
	addl	%esi,		%eax
	movl	16(%edi),	%esi
	adcl	$0,		%edx
	addl	%esi,		%eax
	adcl	$0,		%edx
	decl	%ecx
	movl	%eax,		16(%edi)
	movl	%edx,		%esi
	jz	L003maw_end
	/* Tail Round 5 */
	movl	20(%ebx),	%eax
	mull	%ebp
	addl	%esi,		%eax
	movl	20(%edi),	%esi
	adcl	$0,		%edx
	addl	%esi,		%eax
	adcl	$0,		%edx
	decl	%ecx
	movl	%eax,		20(%edi)
	movl	%edx,		%esi
	jz	L003maw_end
	/* Tail Round 6 */
	movl	24(%ebx),	%eax
	mull	%ebp
	addl	%esi,		%eax
	movl	24(%edi),	%esi
	adcl	$0,		%edx
	addl	%esi,		%eax
	adcl	$0,		%edx
	movl	%eax,		24(%edi)
	movl	%edx,		%esi
L003maw_end:
	movl	%esi,		%eax
	popl	%ecx
	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret

ENTRY(bn_mul_words)
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi


	xorl	%esi,		%esi
	movl	20(%esp),	%edi
	movl	24(%esp),	%ebx
	movl	28(%esp),	%ebp
	movl	32(%esp),	%ecx
	andl	$4294967288,	%ebp
	jz	L004mw_finish
L005mw_loop:
	/* Round 0 */
	movl	(%ebx),		%eax
	mull	%ecx
	addl	%esi,		%eax
	adcl	$0,		%edx
	movl	%eax,		(%edi)
	movl	%edx,		%esi
	/* Round 4 */
	movl	4(%ebx),	%eax
	mull	%ecx
	addl	%esi,		%eax
	adcl	$0,		%edx
	movl	%eax,		4(%edi)
	movl	%edx,		%esi
	/* Round 8 */
	movl	8(%ebx),	%eax
	mull	%ecx
	addl	%esi,		%eax
	adcl	$0,		%edx
	movl	%eax,		8(%edi)
	movl	%edx,		%esi
	/* Round 12 */
	movl	12(%ebx),	%eax
	mull	%ecx
	addl	%esi,		%eax
	adcl	$0,		%edx
	movl	%eax,		12(%edi)
	movl	%edx,		%esi
	/* Round 16 */
	movl	16(%ebx),	%eax
	mull	%ecx
	addl	%esi,		%eax
	adcl	$0,		%edx
	movl	%eax,		16(%edi)
	movl	%edx,		%esi
	/* Round 20 */
	movl	20(%ebx),	%eax
	mull	%ecx
	addl	%esi,		%eax
	adcl	$0,		%edx
	movl	%eax,		20(%edi)
	movl	%edx,		%esi
	/* Round 24 */
	movl	24(%ebx),	%eax
	mull	%ecx
	addl	%esi,		%eax
	adcl	$0,		%edx
	movl	%eax,		24(%edi)
	movl	%edx,		%esi
	/* Round 28 */
	movl	28(%ebx),	%eax
	mull	%ecx
	addl	%esi,		%eax
	adcl	$0,		%edx
	movl	%eax,		28(%edi)
	movl	%edx,		%esi

	addl	$32,		%ebx
	addl	$32,		%edi
	subl	$8,		%ebp
	jz	L004mw_finish
	jmp	L005mw_loop
L004mw_finish:
	movl	28(%esp),	%ebp
	andl	$7,		%ebp
	jnz	L006mw_finish2
	jmp	L007mw_end
_ALIGN_TEXT
L006mw_finish2:
	/* Tail Round 0 */
	movl	(%ebx),		%eax
	mull	%ecx
	addl	%esi,		%eax
	adcl	$0,		%edx
	movl	%eax,		(%edi)
	movl	%edx,		%esi
	decl	%ebp
	jz	L007mw_end
	/* Tail Round 1 */
	movl	4(%ebx),	%eax
	mull	%ecx
	addl	%esi,		%eax
	adcl	$0,		%edx
	movl	%eax,		4(%edi)
	movl	%edx,		%esi
	decl	%ebp
	jz	L007mw_end
	/* Tail Round 2 */
	movl	8(%ebx),	%eax
	mull	%ecx
	addl	%esi,		%eax
	adcl	$0,		%edx
	movl	%eax,		8(%edi)
	movl	%edx,		%esi
	decl	%ebp
	jz	L007mw_end
	/* Tail Round 3 */
	movl	12(%ebx),	%eax
	mull	%ecx
	addl	%esi,		%eax
	adcl	$0,		%edx
	movl	%eax,		12(%edi)
	movl	%edx,		%esi
	decl	%ebp
	jz	L007mw_end
	/* Tail Round 4 */
	movl	16(%ebx),	%eax
	mull	%ecx
	addl	%esi,		%eax
	adcl	$0,		%edx
	movl	%eax,		16(%edi)
	movl	%edx,		%esi
	decl	%ebp
	jz	L007mw_end
	/* Tail Round 5 */
	movl	20(%ebx),	%eax
	mull	%ecx
	addl	%esi,		%eax
	adcl	$0,		%edx
	movl	%eax,		20(%edi)
	movl	%edx,		%esi
	decl	%ebp
	jz	L007mw_end
	/* Tail Round 6 */
	movl	24(%ebx),	%eax
	mull	%ecx
	addl	%esi,		%eax
	adcl	$0,		%edx
	movl	%eax,		24(%edi)
	movl	%edx,		%esi
L007mw_end:
	movl	%esi,		%eax
	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret

ENTRY(bn_sqr_words)
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi


	movl	20(%esp),	%esi
	movl	24(%esp),	%edi
	movl	28(%esp),	%ebx
	andl	$4294967288,	%ebx
	jz	L008sw_finish
L009sw_loop:
	/* Round 0 */
	movl	(%edi),		%eax
	mull	%eax
	movl	%eax,		(%esi)
	movl	%edx,		4(%esi)
	/* Round 4 */
	movl	4(%edi),	%eax
	mull	%eax
	movl	%eax,		8(%esi)
	movl	%edx,		12(%esi)
	/* Round 8 */
	movl	8(%edi),	%eax
	mull	%eax
	movl	%eax,		16(%esi)
	movl	%edx,		20(%esi)
	/* Round 12 */
	movl	12(%edi),	%eax
	mull	%eax
	movl	%eax,		24(%esi)
	movl	%edx,		28(%esi)
	/* Round 16 */
	movl	16(%edi),	%eax
	mull	%eax
	movl	%eax,		32(%esi)
	movl	%edx,		36(%esi)
	/* Round 20 */
	movl	20(%edi),	%eax
	mull	%eax
	movl	%eax,		40(%esi)
	movl	%edx,		44(%esi)
	/* Round 24 */
	movl	24(%edi),	%eax
	mull	%eax
	movl	%eax,		48(%esi)
	movl	%edx,		52(%esi)
	/* Round 28 */
	movl	28(%edi),	%eax
	mull	%eax
	movl	%eax,		56(%esi)
	movl	%edx,		60(%esi)

	addl	$32,		%edi
	addl	$64,		%esi
	subl	$8,		%ebx
	jnz	L009sw_loop
L008sw_finish:
	movl	28(%esp),	%ebx
	andl	$7,		%ebx
	jz	L010sw_end
	/* Tail Round 0 */
	movl	(%edi),		%eax
	mull	%eax
	movl	%eax,		(%esi)
	decl	%ebx
	movl	%edx,		4(%esi)
	jz	L010sw_end
	/* Tail Round 1 */
	movl	4(%edi),	%eax
	mull	%eax
	movl	%eax,		8(%esi)
	decl	%ebx
	movl	%edx,		12(%esi)
	jz	L010sw_end
	/* Tail Round 2 */
	movl	8(%edi),	%eax
	mull	%eax
	movl	%eax,		16(%esi)
	decl	%ebx
	movl	%edx,		20(%esi)
	jz	L010sw_end
	/* Tail Round 3 */
	movl	12(%edi),	%eax
	mull	%eax
	movl	%eax,		24(%esi)
	decl	%ebx
	movl	%edx,		28(%esi)
	jz	L010sw_end
	/* Tail Round 4 */
	movl	16(%edi),	%eax
	mull	%eax
	movl	%eax,		32(%esi)
	decl	%ebx
	movl	%edx,		36(%esi)
	jz	L010sw_end
	/* Tail Round 5 */
	movl	20(%edi),	%eax
	mull	%eax
	movl	%eax,		40(%esi)
	decl	%ebx
	movl	%edx,		44(%esi)
	jz	L010sw_end
	/* Tail Round 6 */
	movl	24(%edi),	%eax
	mull	%eax
	movl	%eax,		48(%esi)
	movl	%edx,		52(%esi)
L010sw_end:
	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret

ENTRY(bn_div_words)
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi

	movl	20(%esp),	%edx
	movl	24(%esp),	%eax
	movl	28(%esp),	%ebx
	divl	%ebx
	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret

ENTRY(bn_add_words)
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi


	movl	20(%esp),	%ebx
	movl	24(%esp),	%esi
	movl	28(%esp),	%edi
	movl	32(%esp),	%ebp
	xorl	%eax,		%eax
	andl	$4294967288,	%ebp
	jz	L011aw_finish
L012aw_loop:
	/* Round 0 */
	movl	(%esi),		%ecx
	movl	(%edi),		%edx
	addl	%eax,		%ecx
	movl	$0,		%eax
	adcl	%eax,		%eax
	addl	%edx,		%ecx
	adcl	$0,		%eax
	movl	%ecx,		(%ebx)
	/* Round 1 */
	movl	4(%esi),	%ecx
	movl	4(%edi),	%edx
	addl	%eax,		%ecx
	movl	$0,		%eax
	adcl	%eax,		%eax
	addl	%edx,		%ecx
	adcl	$0,		%eax
	movl	%ecx,		4(%ebx)
	/* Round 2 */
	movl	8(%esi),	%ecx
	movl	8(%edi),	%edx
	addl	%eax,		%ecx
	movl	$0,		%eax
	adcl	%eax,		%eax
	addl	%edx,		%ecx
	adcl	$0,		%eax
	movl	%ecx,		8(%ebx)
	/* Round 3 */
	movl	12(%esi),	%ecx
	movl	12(%edi),	%edx
	addl	%eax,		%ecx
	movl	$0,		%eax
	adcl	%eax,		%eax
	addl	%edx,		%ecx
	adcl	$0,		%eax
	movl	%ecx,		12(%ebx)
	/* Round 4 */
	movl	16(%esi),	%ecx
	movl	16(%edi),	%edx
	addl	%eax,		%ecx
	movl	$0,		%eax
	adcl	%eax,		%eax
	addl	%edx,		%ecx
	adcl	$0,		%eax
	movl	%ecx,		16(%ebx)
	/* Round 5 */
	movl	20(%esi),	%ecx
	movl	20(%edi),	%edx
	addl	%eax,		%ecx
	movl	$0,		%eax
	adcl	%eax,		%eax
	addl	%edx,		%ecx
	adcl	$0,		%eax
	movl	%ecx,		20(%ebx)
	/* Round 6 */
	movl	24(%esi),	%ecx
	movl	24(%edi),	%edx
	addl	%eax,		%ecx
	movl	$0,		%eax
	adcl	%eax,		%eax
	addl	%edx,		%ecx
	adcl	$0,		%eax
	movl	%ecx,		24(%ebx)
	/* Round 7 */
	movl	28(%esi),	%ecx
	movl	28(%edi),	%edx
	addl	%eax,		%ecx
	movl	$0,		%eax
	adcl	%eax,		%eax
	addl	%edx,		%ecx
	adcl	$0,		%eax
	movl	%ecx,		28(%ebx)

	addl	$32,		%esi
	addl	$32,		%edi
	addl	$32,		%ebx
	subl	$8,		%ebp
	jnz	L012aw_loop
L011aw_finish:
	movl	32(%esp),	%ebp
	andl	$7,		%ebp
	jz	L013aw_end
	/* Tail Round 0 */
	movl	(%esi),		%ecx
	movl	(%edi),		%edx
	addl	%eax,		%ecx
	movl	$0,		%eax
	adcl	%eax,		%eax
	addl	%edx,		%ecx
	adcl	$0,		%eax
	decl	%ebp
	movl	%ecx,		(%ebx)
	jz	L013aw_end
	/* Tail Round 1 */
	movl	4(%esi),	%ecx
	movl	4(%edi),	%edx
	addl	%eax,		%ecx
	movl	$0,		%eax
	adcl	%eax,		%eax
	addl	%edx,		%ecx
	adcl	$0,		%eax
	decl	%ebp
	movl	%ecx,		4(%ebx)
	jz	L013aw_end
	/* Tail Round 2 */
	movl	8(%esi),	%ecx
	movl	8(%edi),	%edx
	addl	%eax,		%ecx
	movl	$0,		%eax
	adcl	%eax,		%eax
	addl	%edx,		%ecx
	adcl	$0,		%eax
	decl	%ebp
	movl	%ecx,		8(%ebx)
	jz	L013aw_end
	/* Tail Round 3 */
	movl	12(%esi),	%ecx
	movl	12(%edi),	%edx
	addl	%eax,		%ecx
	movl	$0,		%eax
	adcl	%eax,		%eax
	addl	%edx,		%ecx
	adcl	$0,		%eax
	decl	%ebp
	movl	%ecx,		12(%ebx)
	jz	L013aw_end
	/* Tail Round 4 */
	movl	16(%esi),	%ecx
	movl	16(%edi),	%edx
	addl	%eax,		%ecx
	movl	$0,		%eax
	adcl	%eax,		%eax
	addl	%edx,		%ecx
	adcl	$0,		%eax
	decl	%ebp
	movl	%ecx,		16(%ebx)
	jz	L013aw_end
	/* Tail Round 5 */
	movl	20(%esi),	%ecx
	movl	20(%edi),	%edx
	addl	%eax,		%ecx
	movl	$0,		%eax
	adcl	%eax,		%eax
	addl	%edx,		%ecx
	adcl	$0,		%eax
	decl	%ebp
	movl	%ecx,		20(%ebx)
	jz	L013aw_end
	/* Tail Round 6 */
	movl	24(%esi),	%ecx
	movl	24(%edi),	%edx
	addl	%eax,		%ecx
	movl	$0,		%eax
	adcl	%eax,		%eax
	addl	%edx,		%ecx
	adcl	$0,		%eax
	movl	%ecx,		24(%ebx)
L013aw_end:
	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret

ENTRY(bn_sub_words)
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi


	movl	20(%esp),	%ebx
	movl	24(%esp),	%esi
	movl	28(%esp),	%edi
	movl	32(%esp),	%ebp
	xorl	%eax,		%eax
	andl	$4294967288,	%ebp
	jz	L014aw_finish
L015aw_loop:
	/* Round 0 */
	movl	(%esi),		%ecx
	movl	(%edi),		%edx
	subl	%eax,		%ecx
	movl	$0,		%eax
	adcl	%eax,		%eax
	subl	%edx,		%ecx
	adcl	$0,		%eax
	movl	%ecx,		(%ebx)
	/* Round 1 */
	movl	4(%esi),	%ecx
	movl	4(%edi),	%edx
	subl	%eax,		%ecx
	movl	$0,		%eax
	adcl	%eax,		%eax
	subl	%edx,		%ecx
	adcl	$0,		%eax
	movl	%ecx,		4(%ebx)
	/* Round 2 */
	movl	8(%esi),	%ecx
	movl	8(%edi),	%edx
	subl	%eax,		%ecx
	movl	$0,		%eax
	adcl	%eax,		%eax
	subl	%edx,		%ecx
	adcl	$0,		%eax
	movl	%ecx,		8(%ebx)
	/* Round 3 */
	movl	12(%esi),	%ecx
	movl	12(%edi),	%edx
	subl	%eax,		%ecx
	movl	$0,		%eax
	adcl	%eax,		%eax
	subl	%edx,		%ecx
	adcl	$0,		%eax
	movl	%ecx,		12(%ebx)
	/* Round 4 */
	movl	16(%esi),	%ecx
	movl	16(%edi),	%edx
	subl	%eax,		%ecx
	movl	$0,		%eax
	adcl	%eax,		%eax
	subl	%edx,		%ecx
	adcl	$0,		%eax
	movl	%ecx,		16(%ebx)
	/* Round 5 */
	movl	20(%esi),	%ecx
	movl	20(%edi),	%edx
	subl	%eax,		%ecx
	movl	$0,		%eax
	adcl	%eax,		%eax
	subl	%edx,		%ecx
	adcl	$0,		%eax
	movl	%ecx,		20(%ebx)
	/* Round 6 */
	movl	24(%esi),	%ecx
	movl	24(%edi),	%edx
	subl	%eax,		%ecx
	movl	$0,		%eax
	adcl	%eax,		%eax
	subl	%edx,		%ecx
	adcl	$0,		%eax
	movl	%ecx,		24(%ebx)
	/* Round 7 */
	movl	28(%esi),	%ecx
	movl	28(%edi),	%edx
	subl	%eax,		%ecx
	movl	$0,		%eax
	adcl	%eax,		%eax
	subl	%edx,		%ecx
	adcl	$0,		%eax
	movl	%ecx,		28(%ebx)

	addl	$32,		%esi
	addl	$32,		%edi
	addl	$32,		%ebx
	subl	$8,		%ebp
	jnz	L015aw_loop
L014aw_finish:
	movl	32(%esp),	%ebp
	andl	$7,		%ebp
	jz	L016aw_end
	/* Tail Round 0 */
	movl	(%esi),		%ecx
	movl	(%edi),		%edx
	subl	%eax,		%ecx
	movl	$0,		%eax
	adcl	%eax,		%eax
	subl	%edx,		%ecx
	adcl	$0,		%eax
	decl	%ebp
	movl	%ecx,		(%ebx)
	jz	L016aw_end
	/* Tail Round 1 */
	movl	4(%esi),	%ecx
	movl	4(%edi),	%edx
	subl	%eax,		%ecx
	movl	$0,		%eax
	adcl	%eax,		%eax
	subl	%edx,		%ecx
	adcl	$0,		%eax
	decl	%ebp
	movl	%ecx,		4(%ebx)
	jz	L016aw_end
	/* Tail Round 2 */
	movl	8(%esi),	%ecx
	movl	8(%edi),	%edx
	subl	%eax,		%ecx
	movl	$0,		%eax
	adcl	%eax,		%eax
	subl	%edx,		%ecx
	adcl	$0,		%eax
	decl	%ebp
	movl	%ecx,		8(%ebx)
	jz	L016aw_end
	/* Tail Round 3 */
	movl	12(%esi),	%ecx
	movl	12(%edi),	%edx
	subl	%eax,		%ecx
	movl	$0,		%eax
	adcl	%eax,		%eax
	subl	%edx,		%ecx
	adcl	$0,		%eax
	decl	%ebp
	movl	%ecx,		12(%ebx)
	jz	L016aw_end
	/* Tail Round 4 */
	movl	16(%esi),	%ecx
	movl	16(%edi),	%edx
	subl	%eax,		%ecx
	movl	$0,		%eax
	adcl	%eax,		%eax
	subl	%edx,		%ecx
	adcl	$0,		%eax
	decl	%ebp
	movl	%ecx,		16(%ebx)
	jz	L016aw_end
	/* Tail Round 5 */
	movl	20(%esi),	%ecx
	movl	20(%edi),	%edx
	subl	%eax,		%ecx
	movl	$0,		%eax
	adcl	%eax,		%eax
	subl	%edx,		%ecx
	adcl	$0,		%eax
	decl	%ebp
	movl	%ecx,		20(%ebx)
	jz	L016aw_end
	/* Tail Round 6 */
	movl	24(%esi),	%ecx
	movl	24(%edi),	%edx
	subl	%eax,		%ecx
	movl	$0,		%eax
	adcl	%eax,		%eax
	subl	%edx,		%ecx
	adcl	$0,		%eax
	movl	%ecx,		24(%ebx)
L016aw_end:
	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret
