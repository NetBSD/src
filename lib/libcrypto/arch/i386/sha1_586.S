/*	$NetBSD: sha1_586.S,v 1.2 2003/11/13 19:36:31 itojun Exp $	*/

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
 * Modified from the output of `perl sha1-586.pl elf' by
 * Markus Friedl <markus@openbsd.org> and
 * Jun-ichiro itojun Hagino <itojun@itojun.org>
 */

#include <machine/asm.h>

ENTRY(sha1_block_asm_data_order)
	movl	12(%esp),	%ecx
	pushl	%esi
	sall	$6,		%ecx
	movl	12(%esp),	%esi
	pushl	%ebp
	addl	%esi,		%ecx
	pushl	%ebx
	movl	16(%esp),	%ebp
	pushl	%edi
	movl	12(%ebp),	%edx
	subl	$108,		%esp
	movl	16(%ebp),	%edi
	movl	8(%ebp),	%ebx
	movl	%ecx,		68(%esp)
	/* First we need to setup the X array */
L000start:
	/* First, load the words onto the stack in network byte order */
	movl	(%esi),		%eax
	movl	4(%esi),	%ecx
	/* bswapl eax */
	xchg	%al,		%ah
	rorl	$16,		%eax
	xchg	%al,		%ah
	/* bswapl ecx */
	xchg	%cl,		%ch
	rorl	$16,		%ecx
	xchg	%cl,		%ch
	movl	%eax,		(%esp)
	movl	%ecx,		4(%esp)
	movl	8(%esi),	%eax
	movl	12(%esi),	%ecx
	/* bswapl eax */
	xchg	%al,		%ah
	rorl	$16,		%eax
	xchg	%al,		%ah
	/* bswapl ecx */
	xchg	%cl,		%ch
	rorl	$16,		%ecx
	xchg	%cl,		%ch
	movl	%eax,		8(%esp)
	movl	%ecx,		12(%esp)
	movl	16(%esi),	%eax
	movl	20(%esi),	%ecx
	/* bswapl eax */
	xchg	%al,		%ah
	rorl	$16,		%eax
	xchg	%al,		%ah
	/* bswapl ecx */
	xchg	%cl,		%ch
	rorl	$16,		%ecx
	xchg	%cl,		%ch
	movl	%eax,		16(%esp)
	movl	%ecx,		20(%esp)
	movl	24(%esi),	%eax
	movl	28(%esi),	%ecx
	/* bswapl eax */
	xchg	%al,		%ah
	rorl	$16,		%eax
	xchg	%al,		%ah
	/* bswapl ecx */
	xchg	%cl,		%ch
	rorl	$16,		%ecx
	xchg	%cl,		%ch
	movl	%eax,		24(%esp)
	movl	%ecx,		28(%esp)
	movl	32(%esi),	%eax
	movl	36(%esi),	%ecx
	/* bswapl eax */
	xchg	%al,		%ah
	rorl	$16,		%eax
	xchg	%al,		%ah
	/* bswapl ecx */
	xchg	%cl,		%ch
	rorl	$16,		%ecx
	xchg	%cl,		%ch
	movl	%eax,		32(%esp)
	movl	%ecx,		36(%esp)
	movl	40(%esi),	%eax
	movl	44(%esi),	%ecx
	/* bswapl eax */
	xchg	%al,		%ah
	rorl	$16,		%eax
	xchg	%al,		%ah
	/* bswapl ecx */
	xchg	%cl,		%ch
	rorl	$16,		%ecx
	xchg	%cl,		%ch
	movl	%eax,		40(%esp)
	movl	%ecx,		44(%esp)
	movl	48(%esi),	%eax
	movl	52(%esi),	%ecx
	/* bswapl eax */
	xchg	%al,		%ah
	rorl	$16,		%eax
	xchg	%al,		%ah
	/* bswapl ecx */
	xchg	%cl,		%ch
	rorl	$16,		%ecx
	xchg	%cl,		%ch
	movl	%eax,		48(%esp)
	movl	%ecx,		52(%esp)
	movl	56(%esi),	%eax
	movl	60(%esi),	%ecx
	/* bswapl eax */
	xchg	%al,		%ah
	rorl	$16,		%eax
	xchg	%al,		%ah
	/* bswapl ecx */
	xchg	%cl,		%ch
	rorl	$16,		%ecx
	xchg	%cl,		%ch
	movl	%eax,		56(%esp)
	movl	%ecx,		60(%esp)
	/* We now have the X array on the stack */
	/* starting at sp-4 */
	movl	%esi,		132(%esp)
L001shortcut:

	/* Start processing */
	movl	(%ebp),		%eax
	movl	4(%ebp),	%ecx
	/* 00_15 0 */
	movl	%ebx,		%esi
	movl	%eax,		%ebp
	xorl	%edx,		%esi
	roll	$5,		%ebp
	andl	%ecx,		%esi
	addl	%edi,		%ebp
.byte 209
.byte 201		/* rorl $1 %ecx */
	movl	(%esp),		%edi
.byte 209
.byte 201		/* rorl $1 %ecx */
	xorl	%edx,		%esi
	leal	1518500249(%ebp,%edi,1),%ebp
	movl	%ecx,		%edi
	addl	%ebp,		%esi
	xorl	%ebx,		%edi
	movl	%esi,		%ebp
	andl	%eax,		%edi
	roll	$5,		%ebp
	addl	%edx,		%ebp
	movl	4(%esp),	%edx
.byte 209
.byte 200		/* rorl $1 %eax */
	xorl	%ebx,		%edi
.byte 209
.byte 200		/* rorl $1 %eax */
	leal	1518500249(%ebp,%edx,1),%ebp
	addl	%ebp,		%edi
	/* 00_15 2 */
	movl	%eax,		%edx
	movl	%edi,		%ebp
	xorl	%ecx,		%edx
	roll	$5,		%ebp
	andl	%esi,		%edx
	addl	%ebx,		%ebp
.byte 209
.byte 206		/* rorl $1 %esi */
	movl	8(%esp),	%ebx
.byte 209
.byte 206		/* rorl $1 %esi */
	xorl	%ecx,		%edx
	leal	1518500249(%ebp,%ebx,1),%ebp
	movl	%esi,		%ebx
	addl	%ebp,		%edx
	xorl	%eax,		%ebx
	movl	%edx,		%ebp
	andl	%edi,		%ebx
	roll	$5,		%ebp
	addl	%ecx,		%ebp
	movl	12(%esp),	%ecx
.byte 209
.byte 207		/* rorl $1 %edi */
	xorl	%eax,		%ebx
.byte 209
.byte 207		/* rorl $1 %edi */
	leal	1518500249(%ebp,%ecx,1),%ebp
	addl	%ebp,		%ebx
	/* 00_15 4 */
	movl	%edi,		%ecx
	movl	%ebx,		%ebp
	xorl	%esi,		%ecx
	roll	$5,		%ebp
	andl	%edx,		%ecx
	addl	%eax,		%ebp
.byte 209
.byte 202		/* rorl $1 %edx */
	movl	16(%esp),	%eax
.byte 209
.byte 202		/* rorl $1 %edx */
	xorl	%esi,		%ecx
	leal	1518500249(%ebp,%eax,1),%ebp
	movl	%edx,		%eax
	addl	%ebp,		%ecx
	xorl	%edi,		%eax
	movl	%ecx,		%ebp
	andl	%ebx,		%eax
	roll	$5,		%ebp
	addl	%esi,		%ebp
	movl	20(%esp),	%esi
.byte 209
.byte 203		/* rorl $1 %ebx */
	xorl	%edi,		%eax
.byte 209
.byte 203		/* rorl $1 %ebx */
	leal	1518500249(%ebp,%esi,1),%ebp
	addl	%ebp,		%eax
	/* 00_15 6 */
	movl	%ebx,		%esi
	movl	%eax,		%ebp
	xorl	%edx,		%esi
	roll	$5,		%ebp
	andl	%ecx,		%esi
	addl	%edi,		%ebp
.byte 209
.byte 201		/* rorl $1 %ecx */
	movl	24(%esp),	%edi
.byte 209
.byte 201		/* rorl $1 %ecx */
	xorl	%edx,		%esi
	leal	1518500249(%ebp,%edi,1),%ebp
	movl	%ecx,		%edi
	addl	%ebp,		%esi
	xorl	%ebx,		%edi
	movl	%esi,		%ebp
	andl	%eax,		%edi
	roll	$5,		%ebp
	addl	%edx,		%ebp
	movl	28(%esp),	%edx
.byte 209
.byte 200		/* rorl $1 %eax */
	xorl	%ebx,		%edi
.byte 209
.byte 200		/* rorl $1 %eax */
	leal	1518500249(%ebp,%edx,1),%ebp
	addl	%ebp,		%edi
	/* 00_15 8 */
	movl	%eax,		%edx
	movl	%edi,		%ebp
	xorl	%ecx,		%edx
	roll	$5,		%ebp
	andl	%esi,		%edx
	addl	%ebx,		%ebp
.byte 209
.byte 206		/* rorl $1 %esi */
	movl	32(%esp),	%ebx
.byte 209
.byte 206		/* rorl $1 %esi */
	xorl	%ecx,		%edx
	leal	1518500249(%ebp,%ebx,1),%ebp
	movl	%esi,		%ebx
	addl	%ebp,		%edx
	xorl	%eax,		%ebx
	movl	%edx,		%ebp
	andl	%edi,		%ebx
	roll	$5,		%ebp
	addl	%ecx,		%ebp
	movl	36(%esp),	%ecx
.byte 209
.byte 207		/* rorl $1 %edi */
	xorl	%eax,		%ebx
.byte 209
.byte 207		/* rorl $1 %edi */
	leal	1518500249(%ebp,%ecx,1),%ebp
	addl	%ebp,		%ebx
	/* 00_15 10 */
	movl	%edi,		%ecx
	movl	%ebx,		%ebp
	xorl	%esi,		%ecx
	roll	$5,		%ebp
	andl	%edx,		%ecx
	addl	%eax,		%ebp
.byte 209
.byte 202		/* rorl $1 %edx */
	movl	40(%esp),	%eax
.byte 209
.byte 202		/* rorl $1 %edx */
	xorl	%esi,		%ecx
	leal	1518500249(%ebp,%eax,1),%ebp
	movl	%edx,		%eax
	addl	%ebp,		%ecx
	xorl	%edi,		%eax
	movl	%ecx,		%ebp
	andl	%ebx,		%eax
	roll	$5,		%ebp
	addl	%esi,		%ebp
	movl	44(%esp),	%esi
.byte 209
.byte 203		/* rorl $1 %ebx */
	xorl	%edi,		%eax
.byte 209
.byte 203		/* rorl $1 %ebx */
	leal	1518500249(%ebp,%esi,1),%ebp
	addl	%ebp,		%eax
	/* 00_15 12 */
	movl	%ebx,		%esi
	movl	%eax,		%ebp
	xorl	%edx,		%esi
	roll	$5,		%ebp
	andl	%ecx,		%esi
	addl	%edi,		%ebp
.byte 209
.byte 201		/* rorl $1 %ecx */
	movl	48(%esp),	%edi
.byte 209
.byte 201		/* rorl $1 %ecx */
	xorl	%edx,		%esi
	leal	1518500249(%ebp,%edi,1),%ebp
	movl	%ecx,		%edi
	addl	%ebp,		%esi
	xorl	%ebx,		%edi
	movl	%esi,		%ebp
	andl	%eax,		%edi
	roll	$5,		%ebp
	addl	%edx,		%ebp
	movl	52(%esp),	%edx
.byte 209
.byte 200		/* rorl $1 %eax */
	xorl	%ebx,		%edi
.byte 209
.byte 200		/* rorl $1 %eax */
	leal	1518500249(%ebp,%edx,1),%ebp
	addl	%ebp,		%edi
	/* 00_15 14 */
	movl	%eax,		%edx
	movl	%edi,		%ebp
	xorl	%ecx,		%edx
	roll	$5,		%ebp
	andl	%esi,		%edx
	addl	%ebx,		%ebp
.byte 209
.byte 206		/* rorl $1 %esi */
	movl	56(%esp),	%ebx
.byte 209
.byte 206		/* rorl $1 %esi */
	xorl	%ecx,		%edx
	leal	1518500249(%ebp,%ebx,1),%ebp
	movl	%esi,		%ebx
	addl	%ebp,		%edx
	xorl	%eax,		%ebx
	movl	%edx,		%ebp
	andl	%edi,		%ebx
	roll	$5,		%ebp
	addl	%ecx,		%ebp
	movl	60(%esp),	%ecx
.byte 209
.byte 207		/* rorl $1 %edi */
	xorl	%eax,		%ebx
.byte 209
.byte 207		/* rorl $1 %edi */
	leal	1518500249(%ebp,%ecx,1),%ebp
	addl	%ebp,		%ebx
	/* 16_19 16 */
	nop
	movl	(%esp),		%ebp
	movl	8(%esp),	%ecx
	xorl	%ebp,		%ecx
	movl	32(%esp),	%ebp
	xorl	%ebp,		%ecx
	movl	52(%esp),	%ebp
	xorl	%ebp,		%ecx
	movl	%edi,		%ebp
.byte 209
.byte 193		/* roll $1 %ecx */
	xorl	%esi,		%ebp
	movl	%ecx,		(%esp)
	andl	%edx,		%ebp
	leal	1518500249(%ecx,%eax,1),%ecx
	xorl	%esi,		%ebp
	movl	%ebx,		%eax
	addl	%ebp,		%ecx
	roll	$5,		%eax
.byte 209
.byte 202		/* rorl $1 %edx */
	addl	%eax,		%ecx
	movl	4(%esp),	%eax
	movl	12(%esp),	%ebp
	xorl	%ebp,		%eax
	movl	36(%esp),	%ebp
	xorl	%ebp,		%eax
	movl	56(%esp),	%ebp
.byte 209
.byte 202		/* rorl $1 %edx */
	xorl	%ebp,		%eax
.byte 209
.byte 192		/* roll $1 %eax */
	movl	%edx,		%ebp
	xorl	%edi,		%ebp
	movl	%eax,		4(%esp)
	andl	%ebx,		%ebp
	leal	1518500249(%eax,%esi,1),%eax
	xorl	%edi,		%ebp
	movl	%ecx,		%esi
	roll	$5,		%esi
.byte 209
.byte 203		/* rorl $1 %ebx */
	addl	%esi,		%eax
.byte 209
.byte 203		/* rorl $1 %ebx */
	addl	%ebp,		%eax
	/* 16_19 18 */
	movl	8(%esp),	%ebp
	movl	16(%esp),	%esi
	xorl	%ebp,		%esi
	movl	40(%esp),	%ebp
	xorl	%ebp,		%esi
	movl	60(%esp),	%ebp
	xorl	%ebp,		%esi
	movl	%ebx,		%ebp
.byte 209
.byte 198		/* roll $1 %esi */
	xorl	%edx,		%ebp
	movl	%esi,		8(%esp)
	andl	%ecx,		%ebp
	leal	1518500249(%esi,%edi,1),%esi
	xorl	%edx,		%ebp
	movl	%eax,		%edi
	addl	%ebp,		%esi
	roll	$5,		%edi
.byte 209
.byte 201		/* rorl $1 %ecx */
	addl	%edi,		%esi
	movl	12(%esp),	%edi
	movl	20(%esp),	%ebp
	xorl	%ebp,		%edi
	movl	44(%esp),	%ebp
	xorl	%ebp,		%edi
	movl	(%esp),		%ebp
.byte 209
.byte 201		/* rorl $1 %ecx */
	xorl	%ebp,		%edi
.byte 209
.byte 199		/* roll $1 %edi */
	movl	%ecx,		%ebp
	xorl	%ebx,		%ebp
	movl	%edi,		12(%esp)
	andl	%eax,		%ebp
	leal	1518500249(%edi,%edx,1),%edi
	xorl	%ebx,		%ebp
	movl	%esi,		%edx
	roll	$5,		%edx
.byte 209
.byte 200		/* rorl $1 %eax */
	addl	%edx,		%edi
.byte 209
.byte 200		/* rorl $1 %eax */
	addl	%ebp,		%edi
	/* 20_39 20 */
	movl	16(%esp),	%edx
	movl	24(%esp),	%ebp
	xorl	%ebp,		%edx
	movl	48(%esp),	%ebp
	xorl	%ebp,		%edx
	movl	4(%esp),	%ebp
	xorl	%ebp,		%edx
	movl	%esi,		%ebp
.byte 209
.byte 194		/* roll $1 %edx */
	xorl	%eax,		%ebp
	movl	%edx,		16(%esp)
	xorl	%ecx,		%ebp
	leal	1859775393(%edx,%ebx,1),%edx
	movl	%edi,		%ebx
	roll	$5,		%ebx
.byte 209
.byte 206		/* rorl $1 %esi */
	addl	%ebp,		%ebx
.byte 209
.byte 206		/* rorl $1 %esi */
	addl	%ebx,		%edx
	/* 20_39 21 */
	movl	20(%esp),	%ebx
	movl	28(%esp),	%ebp
	xorl	%ebp,		%ebx
	movl	52(%esp),	%ebp
	xorl	%ebp,		%ebx
	movl	8(%esp),	%ebp
	xorl	%ebp,		%ebx
	movl	%edi,		%ebp
.byte 209
.byte 195		/* roll $1 %ebx */
	xorl	%esi,		%ebp
	movl	%ebx,		20(%esp)
	xorl	%eax,		%ebp
	leal	1859775393(%ebx,%ecx,1),%ebx
	movl	%edx,		%ecx
	roll	$5,		%ecx
.byte 209
.byte 207		/* rorl $1 %edi */
	addl	%ebp,		%ecx
.byte 209
.byte 207		/* rorl $1 %edi */
	addl	%ecx,		%ebx
	/* 20_39 22 */
	movl	24(%esp),	%ecx
	movl	32(%esp),	%ebp
	xorl	%ebp,		%ecx
	movl	56(%esp),	%ebp
	xorl	%ebp,		%ecx
	movl	12(%esp),	%ebp
	xorl	%ebp,		%ecx
	movl	%edx,		%ebp
.byte 209
.byte 193		/* roll $1 %ecx */
	xorl	%edi,		%ebp
	movl	%ecx,		24(%esp)
	xorl	%esi,		%ebp
	leal	1859775393(%ecx,%eax,1),%ecx
	movl	%ebx,		%eax
	roll	$5,		%eax
.byte 209
.byte 202		/* rorl $1 %edx */
	addl	%ebp,		%eax
.byte 209
.byte 202		/* rorl $1 %edx */
	addl	%eax,		%ecx
	/* 20_39 23 */
	movl	28(%esp),	%eax
	movl	36(%esp),	%ebp
	xorl	%ebp,		%eax
	movl	60(%esp),	%ebp
	xorl	%ebp,		%eax
	movl	16(%esp),	%ebp
	xorl	%ebp,		%eax
	movl	%ebx,		%ebp
.byte 209
.byte 192		/* roll $1 %eax */
	xorl	%edx,		%ebp
	movl	%eax,		28(%esp)
	xorl	%edi,		%ebp
	leal	1859775393(%eax,%esi,1),%eax
	movl	%ecx,		%esi
	roll	$5,		%esi
.byte 209
.byte 203		/* rorl $1 %ebx */
	addl	%ebp,		%esi
.byte 209
.byte 203		/* rorl $1 %ebx */
	addl	%esi,		%eax
	/* 20_39 24 */
	movl	32(%esp),	%esi
	movl	40(%esp),	%ebp
	xorl	%ebp,		%esi
	movl	(%esp),		%ebp
	xorl	%ebp,		%esi
	movl	20(%esp),	%ebp
	xorl	%ebp,		%esi
	movl	%ecx,		%ebp
.byte 209
.byte 198		/* roll $1 %esi */
	xorl	%ebx,		%ebp
	movl	%esi,		32(%esp)
	xorl	%edx,		%ebp
	leal	1859775393(%esi,%edi,1),%esi
	movl	%eax,		%edi
	roll	$5,		%edi
.byte 209
.byte 201		/* rorl $1 %ecx */
	addl	%ebp,		%edi
.byte 209
.byte 201		/* rorl $1 %ecx */
	addl	%edi,		%esi
	/* 20_39 25 */
	movl	36(%esp),	%edi
	movl	44(%esp),	%ebp
	xorl	%ebp,		%edi
	movl	4(%esp),	%ebp
	xorl	%ebp,		%edi
	movl	24(%esp),	%ebp
	xorl	%ebp,		%edi
	movl	%eax,		%ebp
.byte 209
.byte 199		/* roll $1 %edi */
	xorl	%ecx,		%ebp
	movl	%edi,		36(%esp)
	xorl	%ebx,		%ebp
	leal	1859775393(%edi,%edx,1),%edi
	movl	%esi,		%edx
	roll	$5,		%edx
.byte 209
.byte 200		/* rorl $1 %eax */
	addl	%ebp,		%edx
.byte 209
.byte 200		/* rorl $1 %eax */
	addl	%edx,		%edi
	/* 20_39 26 */
	movl	40(%esp),	%edx
	movl	48(%esp),	%ebp
	xorl	%ebp,		%edx
	movl	8(%esp),	%ebp
	xorl	%ebp,		%edx
	movl	28(%esp),	%ebp
	xorl	%ebp,		%edx
	movl	%esi,		%ebp
.byte 209
.byte 194		/* roll $1 %edx */
	xorl	%eax,		%ebp
	movl	%edx,		40(%esp)
	xorl	%ecx,		%ebp
	leal	1859775393(%edx,%ebx,1),%edx
	movl	%edi,		%ebx
	roll	$5,		%ebx
.byte 209
.byte 206		/* rorl $1 %esi */
	addl	%ebp,		%ebx
.byte 209
.byte 206		/* rorl $1 %esi */
	addl	%ebx,		%edx
	/* 20_39 27 */
	movl	44(%esp),	%ebx
	movl	52(%esp),	%ebp
	xorl	%ebp,		%ebx
	movl	12(%esp),	%ebp
	xorl	%ebp,		%ebx
	movl	32(%esp),	%ebp
	xorl	%ebp,		%ebx
	movl	%edi,		%ebp
.byte 209
.byte 195		/* roll $1 %ebx */
	xorl	%esi,		%ebp
	movl	%ebx,		44(%esp)
	xorl	%eax,		%ebp
	leal	1859775393(%ebx,%ecx,1),%ebx
	movl	%edx,		%ecx
	roll	$5,		%ecx
.byte 209
.byte 207		/* rorl $1 %edi */
	addl	%ebp,		%ecx
.byte 209
.byte 207		/* rorl $1 %edi */
	addl	%ecx,		%ebx
	/* 20_39 28 */
	movl	48(%esp),	%ecx
	movl	56(%esp),	%ebp
	xorl	%ebp,		%ecx
	movl	16(%esp),	%ebp
	xorl	%ebp,		%ecx
	movl	36(%esp),	%ebp
	xorl	%ebp,		%ecx
	movl	%edx,		%ebp
.byte 209
.byte 193		/* roll $1 %ecx */
	xorl	%edi,		%ebp
	movl	%ecx,		48(%esp)
	xorl	%esi,		%ebp
	leal	1859775393(%ecx,%eax,1),%ecx
	movl	%ebx,		%eax
	roll	$5,		%eax
.byte 209
.byte 202		/* rorl $1 %edx */
	addl	%ebp,		%eax
.byte 209
.byte 202		/* rorl $1 %edx */
	addl	%eax,		%ecx
	/* 20_39 29 */
	movl	52(%esp),	%eax
	movl	60(%esp),	%ebp
	xorl	%ebp,		%eax
	movl	20(%esp),	%ebp
	xorl	%ebp,		%eax
	movl	40(%esp),	%ebp
	xorl	%ebp,		%eax
	movl	%ebx,		%ebp
.byte 209
.byte 192		/* roll $1 %eax */
	xorl	%edx,		%ebp
	movl	%eax,		52(%esp)
	xorl	%edi,		%ebp
	leal	1859775393(%eax,%esi,1),%eax
	movl	%ecx,		%esi
	roll	$5,		%esi
.byte 209
.byte 203		/* rorl $1 %ebx */
	addl	%ebp,		%esi
.byte 209
.byte 203		/* rorl $1 %ebx */
	addl	%esi,		%eax
	/* 20_39 30 */
	movl	56(%esp),	%esi
	movl	(%esp),		%ebp
	xorl	%ebp,		%esi
	movl	24(%esp),	%ebp
	xorl	%ebp,		%esi
	movl	44(%esp),	%ebp
	xorl	%ebp,		%esi
	movl	%ecx,		%ebp
.byte 209
.byte 198		/* roll $1 %esi */
	xorl	%ebx,		%ebp
	movl	%esi,		56(%esp)
	xorl	%edx,		%ebp
	leal	1859775393(%esi,%edi,1),%esi
	movl	%eax,		%edi
	roll	$5,		%edi
.byte 209
.byte 201		/* rorl $1 %ecx */
	addl	%ebp,		%edi
.byte 209
.byte 201		/* rorl $1 %ecx */
	addl	%edi,		%esi
	/* 20_39 31 */
	movl	60(%esp),	%edi
	movl	4(%esp),	%ebp
	xorl	%ebp,		%edi
	movl	28(%esp),	%ebp
	xorl	%ebp,		%edi
	movl	48(%esp),	%ebp
	xorl	%ebp,		%edi
	movl	%eax,		%ebp
.byte 209
.byte 199		/* roll $1 %edi */
	xorl	%ecx,		%ebp
	movl	%edi,		60(%esp)
	xorl	%ebx,		%ebp
	leal	1859775393(%edi,%edx,1),%edi
	movl	%esi,		%edx
	roll	$5,		%edx
.byte 209
.byte 200		/* rorl $1 %eax */
	addl	%ebp,		%edx
.byte 209
.byte 200		/* rorl $1 %eax */
	addl	%edx,		%edi
	/* 20_39 32 */
	movl	(%esp),		%edx
	movl	8(%esp),	%ebp
	xorl	%ebp,		%edx
	movl	32(%esp),	%ebp
	xorl	%ebp,		%edx
	movl	52(%esp),	%ebp
	xorl	%ebp,		%edx
	movl	%esi,		%ebp
.byte 209
.byte 194		/* roll $1 %edx */
	xorl	%eax,		%ebp
	movl	%edx,		(%esp)
	xorl	%ecx,		%ebp
	leal	1859775393(%edx,%ebx,1),%edx
	movl	%edi,		%ebx
	roll	$5,		%ebx
.byte 209
.byte 206		/* rorl $1 %esi */
	addl	%ebp,		%ebx
.byte 209
.byte 206		/* rorl $1 %esi */
	addl	%ebx,		%edx
	/* 20_39 33 */
	movl	4(%esp),	%ebx
	movl	12(%esp),	%ebp
	xorl	%ebp,		%ebx
	movl	36(%esp),	%ebp
	xorl	%ebp,		%ebx
	movl	56(%esp),	%ebp
	xorl	%ebp,		%ebx
	movl	%edi,		%ebp
.byte 209
.byte 195		/* roll $1 %ebx */
	xorl	%esi,		%ebp
	movl	%ebx,		4(%esp)
	xorl	%eax,		%ebp
	leal	1859775393(%ebx,%ecx,1),%ebx
	movl	%edx,		%ecx
	roll	$5,		%ecx
.byte 209
.byte 207		/* rorl $1 %edi */
	addl	%ebp,		%ecx
.byte 209
.byte 207		/* rorl $1 %edi */
	addl	%ecx,		%ebx
	/* 20_39 34 */
	movl	8(%esp),	%ecx
	movl	16(%esp),	%ebp
	xorl	%ebp,		%ecx
	movl	40(%esp),	%ebp
	xorl	%ebp,		%ecx
	movl	60(%esp),	%ebp
	xorl	%ebp,		%ecx
	movl	%edx,		%ebp
.byte 209
.byte 193		/* roll $1 %ecx */
	xorl	%edi,		%ebp
	movl	%ecx,		8(%esp)
	xorl	%esi,		%ebp
	leal	1859775393(%ecx,%eax,1),%ecx
	movl	%ebx,		%eax
	roll	$5,		%eax
.byte 209
.byte 202		/* rorl $1 %edx */
	addl	%ebp,		%eax
.byte 209
.byte 202		/* rorl $1 %edx */
	addl	%eax,		%ecx
	/* 20_39 35 */
	movl	12(%esp),	%eax
	movl	20(%esp),	%ebp
	xorl	%ebp,		%eax
	movl	44(%esp),	%ebp
	xorl	%ebp,		%eax
	movl	(%esp),		%ebp
	xorl	%ebp,		%eax
	movl	%ebx,		%ebp
.byte 209
.byte 192		/* roll $1 %eax */
	xorl	%edx,		%ebp
	movl	%eax,		12(%esp)
	xorl	%edi,		%ebp
	leal	1859775393(%eax,%esi,1),%eax
	movl	%ecx,		%esi
	roll	$5,		%esi
.byte 209
.byte 203		/* rorl $1 %ebx */
	addl	%ebp,		%esi
.byte 209
.byte 203		/* rorl $1 %ebx */
	addl	%esi,		%eax
	/* 20_39 36 */
	movl	16(%esp),	%esi
	movl	24(%esp),	%ebp
	xorl	%ebp,		%esi
	movl	48(%esp),	%ebp
	xorl	%ebp,		%esi
	movl	4(%esp),	%ebp
	xorl	%ebp,		%esi
	movl	%ecx,		%ebp
.byte 209
.byte 198		/* roll $1 %esi */
	xorl	%ebx,		%ebp
	movl	%esi,		16(%esp)
	xorl	%edx,		%ebp
	leal	1859775393(%esi,%edi,1),%esi
	movl	%eax,		%edi
	roll	$5,		%edi
.byte 209
.byte 201		/* rorl $1 %ecx */
	addl	%ebp,		%edi
.byte 209
.byte 201		/* rorl $1 %ecx */
	addl	%edi,		%esi
	/* 20_39 37 */
	movl	20(%esp),	%edi
	movl	28(%esp),	%ebp
	xorl	%ebp,		%edi
	movl	52(%esp),	%ebp
	xorl	%ebp,		%edi
	movl	8(%esp),	%ebp
	xorl	%ebp,		%edi
	movl	%eax,		%ebp
.byte 209
.byte 199		/* roll $1 %edi */
	xorl	%ecx,		%ebp
	movl	%edi,		20(%esp)
	xorl	%ebx,		%ebp
	leal	1859775393(%edi,%edx,1),%edi
	movl	%esi,		%edx
	roll	$5,		%edx
.byte 209
.byte 200		/* rorl $1 %eax */
	addl	%ebp,		%edx
.byte 209
.byte 200		/* rorl $1 %eax */
	addl	%edx,		%edi
	/* 20_39 38 */
	movl	24(%esp),	%edx
	movl	32(%esp),	%ebp
	xorl	%ebp,		%edx
	movl	56(%esp),	%ebp
	xorl	%ebp,		%edx
	movl	12(%esp),	%ebp
	xorl	%ebp,		%edx
	movl	%esi,		%ebp
.byte 209
.byte 194		/* roll $1 %edx */
	xorl	%eax,		%ebp
	movl	%edx,		24(%esp)
	xorl	%ecx,		%ebp
	leal	1859775393(%edx,%ebx,1),%edx
	movl	%edi,		%ebx
	roll	$5,		%ebx
.byte 209
.byte 206		/* rorl $1 %esi */
	addl	%ebp,		%ebx
.byte 209
.byte 206		/* rorl $1 %esi */
	addl	%ebx,		%edx
	/* 20_39 39 */
	movl	28(%esp),	%ebx
	movl	36(%esp),	%ebp
	xorl	%ebp,		%ebx
	movl	60(%esp),	%ebp
	xorl	%ebp,		%ebx
	movl	16(%esp),	%ebp
	xorl	%ebp,		%ebx
	movl	%edi,		%ebp
.byte 209
.byte 195		/* roll $1 %ebx */
	xorl	%esi,		%ebp
	movl	%ebx,		28(%esp)
	xorl	%eax,		%ebp
	leal	1859775393(%ebx,%ecx,1),%ebx
	movl	%edx,		%ecx
	roll	$5,		%ecx
.byte 209
.byte 207		/* rorl $1 %edi */
	addl	%ebp,		%ecx
.byte 209
.byte 207		/* rorl $1 %edi */
	addl	%ecx,		%ebx
	/* 40_59 40 */
	movl	32(%esp),	%ecx
	movl	40(%esp),	%ebp
	xorl	%ebp,		%ecx
	movl	(%esp),		%ebp
	xorl	%ebp,		%ecx
	movl	20(%esp),	%ebp
	xorl	%ebp,		%ecx
	movl	%edx,		%ebp
.byte 209
.byte 193		/* roll $1 %ecx */
	orl	%edi,		%ebp
	movl	%ecx,		32(%esp)
	andl	%esi,		%ebp
	leal	2400959708(%ecx,%eax,1),%ecx
	movl	%edx,		%eax
.byte 209
.byte 202		/* rorl $1 %edx */
	andl	%edi,		%eax
	orl	%eax,		%ebp
	movl	%ebx,		%eax
	roll	$5,		%eax
	addl	%eax,		%ebp
	movl	36(%esp),	%eax
	addl	%ebp,		%ecx
	movl	44(%esp),	%ebp
	xorl	%ebp,		%eax
	movl	4(%esp),	%ebp
	xorl	%ebp,		%eax
	movl	24(%esp),	%ebp
.byte 209
.byte 202		/* rorl $1 %edx */
	xorl	%ebp,		%eax
.byte 209
.byte 192		/* roll $1 %eax */
	movl	%ebx,		%ebp
	movl	%eax,		36(%esp)
	orl	%edx,		%ebp
	leal	2400959708(%eax,%esi,1),%eax
	movl	%ebx,		%esi
	andl	%edi,		%ebp
	andl	%edx,		%esi
	orl	%esi,		%ebp
	movl	%ecx,		%esi
	roll	$5,		%esi
.byte 209
.byte 203		/* rorl $1 %ebx */
	addl	%esi,		%ebp
.byte 209
.byte 203		/* rorl $1 %ebx */
	addl	%ebp,		%eax
	/* 40_59 41 */
	/* 40_59 42 */
	movl	40(%esp),	%esi
	movl	48(%esp),	%ebp
	xorl	%ebp,		%esi
	movl	8(%esp),	%ebp
	xorl	%ebp,		%esi
	movl	28(%esp),	%ebp
	xorl	%ebp,		%esi
	movl	%ecx,		%ebp
.byte 209
.byte 198		/* roll $1 %esi */
	orl	%ebx,		%ebp
	movl	%esi,		40(%esp)
	andl	%edx,		%ebp
	leal	2400959708(%esi,%edi,1),%esi
	movl	%ecx,		%edi
.byte 209
.byte 201		/* rorl $1 %ecx */
	andl	%ebx,		%edi
	orl	%edi,		%ebp
	movl	%eax,		%edi
	roll	$5,		%edi
	addl	%edi,		%ebp
	movl	44(%esp),	%edi
	addl	%ebp,		%esi
	movl	52(%esp),	%ebp
	xorl	%ebp,		%edi
	movl	12(%esp),	%ebp
	xorl	%ebp,		%edi
	movl	32(%esp),	%ebp
.byte 209
.byte 201		/* rorl $1 %ecx */
	xorl	%ebp,		%edi
.byte 209
.byte 199		/* roll $1 %edi */
	movl	%eax,		%ebp
	movl	%edi,		44(%esp)
	orl	%ecx,		%ebp
	leal	2400959708(%edi,%edx,1),%edi
	movl	%eax,		%edx
	andl	%ebx,		%ebp
	andl	%ecx,		%edx
	orl	%edx,		%ebp
	movl	%esi,		%edx
	roll	$5,		%edx
.byte 209
.byte 200		/* rorl $1 %eax */
	addl	%edx,		%ebp
.byte 209
.byte 200		/* rorl $1 %eax */
	addl	%ebp,		%edi
	/* 40_59 43 */
	/* 40_59 44 */
	movl	48(%esp),	%edx
	movl	56(%esp),	%ebp
	xorl	%ebp,		%edx
	movl	16(%esp),	%ebp
	xorl	%ebp,		%edx
	movl	36(%esp),	%ebp
	xorl	%ebp,		%edx
	movl	%esi,		%ebp
.byte 209
.byte 194		/* roll $1 %edx */
	orl	%eax,		%ebp
	movl	%edx,		48(%esp)
	andl	%ecx,		%ebp
	leal	2400959708(%edx,%ebx,1),%edx
	movl	%esi,		%ebx
.byte 209
.byte 206		/* rorl $1 %esi */
	andl	%eax,		%ebx
	orl	%ebx,		%ebp
	movl	%edi,		%ebx
	roll	$5,		%ebx
	addl	%ebx,		%ebp
	movl	52(%esp),	%ebx
	addl	%ebp,		%edx
	movl	60(%esp),	%ebp
	xorl	%ebp,		%ebx
	movl	20(%esp),	%ebp
	xorl	%ebp,		%ebx
	movl	40(%esp),	%ebp
.byte 209
.byte 206		/* rorl $1 %esi */
	xorl	%ebp,		%ebx
.byte 209
.byte 195		/* roll $1 %ebx */
	movl	%edi,		%ebp
	movl	%ebx,		52(%esp)
	orl	%esi,		%ebp
	leal	2400959708(%ebx,%ecx,1),%ebx
	movl	%edi,		%ecx
	andl	%eax,		%ebp
	andl	%esi,		%ecx
	orl	%ecx,		%ebp
	movl	%edx,		%ecx
	roll	$5,		%ecx
.byte 209
.byte 207		/* rorl $1 %edi */
	addl	%ecx,		%ebp
.byte 209
.byte 207		/* rorl $1 %edi */
	addl	%ebp,		%ebx
	/* 40_59 45 */
	/* 40_59 46 */
	movl	56(%esp),	%ecx
	movl	(%esp),		%ebp
	xorl	%ebp,		%ecx
	movl	24(%esp),	%ebp
	xorl	%ebp,		%ecx
	movl	44(%esp),	%ebp
	xorl	%ebp,		%ecx
	movl	%edx,		%ebp
.byte 209
.byte 193		/* roll $1 %ecx */
	orl	%edi,		%ebp
	movl	%ecx,		56(%esp)
	andl	%esi,		%ebp
	leal	2400959708(%ecx,%eax,1),%ecx
	movl	%edx,		%eax
.byte 209
.byte 202		/* rorl $1 %edx */
	andl	%edi,		%eax
	orl	%eax,		%ebp
	movl	%ebx,		%eax
	roll	$5,		%eax
	addl	%eax,		%ebp
	movl	60(%esp),	%eax
	addl	%ebp,		%ecx
	movl	4(%esp),	%ebp
	xorl	%ebp,		%eax
	movl	28(%esp),	%ebp
	xorl	%ebp,		%eax
	movl	48(%esp),	%ebp
.byte 209
.byte 202		/* rorl $1 %edx */
	xorl	%ebp,		%eax
.byte 209
.byte 192		/* roll $1 %eax */
	movl	%ebx,		%ebp
	movl	%eax,		60(%esp)
	orl	%edx,		%ebp
	leal	2400959708(%eax,%esi,1),%eax
	movl	%ebx,		%esi
	andl	%edi,		%ebp
	andl	%edx,		%esi
	orl	%esi,		%ebp
	movl	%ecx,		%esi
	roll	$5,		%esi
.byte 209
.byte 203		/* rorl $1 %ebx */
	addl	%esi,		%ebp
.byte 209
.byte 203		/* rorl $1 %ebx */
	addl	%ebp,		%eax
	/* 40_59 47 */
	/* 40_59 48 */
	movl	(%esp),		%esi
	movl	8(%esp),	%ebp
	xorl	%ebp,		%esi
	movl	32(%esp),	%ebp
	xorl	%ebp,		%esi
	movl	52(%esp),	%ebp
	xorl	%ebp,		%esi
	movl	%ecx,		%ebp
.byte 209
.byte 198		/* roll $1 %esi */
	orl	%ebx,		%ebp
	movl	%esi,		(%esp)
	andl	%edx,		%ebp
	leal	2400959708(%esi,%edi,1),%esi
	movl	%ecx,		%edi
.byte 209
.byte 201		/* rorl $1 %ecx */
	andl	%ebx,		%edi
	orl	%edi,		%ebp
	movl	%eax,		%edi
	roll	$5,		%edi
	addl	%edi,		%ebp
	movl	4(%esp),	%edi
	addl	%ebp,		%esi
	movl	12(%esp),	%ebp
	xorl	%ebp,		%edi
	movl	36(%esp),	%ebp
	xorl	%ebp,		%edi
	movl	56(%esp),	%ebp
.byte 209
.byte 201		/* rorl $1 %ecx */
	xorl	%ebp,		%edi
.byte 209
.byte 199		/* roll $1 %edi */
	movl	%eax,		%ebp
	movl	%edi,		4(%esp)
	orl	%ecx,		%ebp
	leal	2400959708(%edi,%edx,1),%edi
	movl	%eax,		%edx
	andl	%ebx,		%ebp
	andl	%ecx,		%edx
	orl	%edx,		%ebp
	movl	%esi,		%edx
	roll	$5,		%edx
.byte 209
.byte 200		/* rorl $1 %eax */
	addl	%edx,		%ebp
.byte 209
.byte 200		/* rorl $1 %eax */
	addl	%ebp,		%edi
	/* 40_59 49 */
	/* 40_59 50 */
	movl	8(%esp),	%edx
	movl	16(%esp),	%ebp
	xorl	%ebp,		%edx
	movl	40(%esp),	%ebp
	xorl	%ebp,		%edx
	movl	60(%esp),	%ebp
	xorl	%ebp,		%edx
	movl	%esi,		%ebp
.byte 209
.byte 194		/* roll $1 %edx */
	orl	%eax,		%ebp
	movl	%edx,		8(%esp)
	andl	%ecx,		%ebp
	leal	2400959708(%edx,%ebx,1),%edx
	movl	%esi,		%ebx
.byte 209
.byte 206		/* rorl $1 %esi */
	andl	%eax,		%ebx
	orl	%ebx,		%ebp
	movl	%edi,		%ebx
	roll	$5,		%ebx
	addl	%ebx,		%ebp
	movl	12(%esp),	%ebx
	addl	%ebp,		%edx
	movl	20(%esp),	%ebp
	xorl	%ebp,		%ebx
	movl	44(%esp),	%ebp
	xorl	%ebp,		%ebx
	movl	(%esp),		%ebp
.byte 209
.byte 206		/* rorl $1 %esi */
	xorl	%ebp,		%ebx
.byte 209
.byte 195		/* roll $1 %ebx */
	movl	%edi,		%ebp
	movl	%ebx,		12(%esp)
	orl	%esi,		%ebp
	leal	2400959708(%ebx,%ecx,1),%ebx
	movl	%edi,		%ecx
	andl	%eax,		%ebp
	andl	%esi,		%ecx
	orl	%ecx,		%ebp
	movl	%edx,		%ecx
	roll	$5,		%ecx
.byte 209
.byte 207		/* rorl $1 %edi */
	addl	%ecx,		%ebp
.byte 209
.byte 207		/* rorl $1 %edi */
	addl	%ebp,		%ebx
	/* 40_59 51 */
	/* 40_59 52 */
	movl	16(%esp),	%ecx
	movl	24(%esp),	%ebp
	xorl	%ebp,		%ecx
	movl	48(%esp),	%ebp
	xorl	%ebp,		%ecx
	movl	4(%esp),	%ebp
	xorl	%ebp,		%ecx
	movl	%edx,		%ebp
.byte 209
.byte 193		/* roll $1 %ecx */
	orl	%edi,		%ebp
	movl	%ecx,		16(%esp)
	andl	%esi,		%ebp
	leal	2400959708(%ecx,%eax,1),%ecx
	movl	%edx,		%eax
.byte 209
.byte 202		/* rorl $1 %edx */
	andl	%edi,		%eax
	orl	%eax,		%ebp
	movl	%ebx,		%eax
	roll	$5,		%eax
	addl	%eax,		%ebp
	movl	20(%esp),	%eax
	addl	%ebp,		%ecx
	movl	28(%esp),	%ebp
	xorl	%ebp,		%eax
	movl	52(%esp),	%ebp
	xorl	%ebp,		%eax
	movl	8(%esp),	%ebp
.byte 209
.byte 202		/* rorl $1 %edx */
	xorl	%ebp,		%eax
.byte 209
.byte 192		/* roll $1 %eax */
	movl	%ebx,		%ebp
	movl	%eax,		20(%esp)
	orl	%edx,		%ebp
	leal	2400959708(%eax,%esi,1),%eax
	movl	%ebx,		%esi
	andl	%edi,		%ebp
	andl	%edx,		%esi
	orl	%esi,		%ebp
	movl	%ecx,		%esi
	roll	$5,		%esi
.byte 209
.byte 203		/* rorl $1 %ebx */
	addl	%esi,		%ebp
.byte 209
.byte 203		/* rorl $1 %ebx */
	addl	%ebp,		%eax
	/* 40_59 53 */
	/* 40_59 54 */
	movl	24(%esp),	%esi
	movl	32(%esp),	%ebp
	xorl	%ebp,		%esi
	movl	56(%esp),	%ebp
	xorl	%ebp,		%esi
	movl	12(%esp),	%ebp
	xorl	%ebp,		%esi
	movl	%ecx,		%ebp
.byte 209
.byte 198		/* roll $1 %esi */
	orl	%ebx,		%ebp
	movl	%esi,		24(%esp)
	andl	%edx,		%ebp
	leal	2400959708(%esi,%edi,1),%esi
	movl	%ecx,		%edi
.byte 209
.byte 201		/* rorl $1 %ecx */
	andl	%ebx,		%edi
	orl	%edi,		%ebp
	movl	%eax,		%edi
	roll	$5,		%edi
	addl	%edi,		%ebp
	movl	28(%esp),	%edi
	addl	%ebp,		%esi
	movl	36(%esp),	%ebp
	xorl	%ebp,		%edi
	movl	60(%esp),	%ebp
	xorl	%ebp,		%edi
	movl	16(%esp),	%ebp
.byte 209
.byte 201		/* rorl $1 %ecx */
	xorl	%ebp,		%edi
.byte 209
.byte 199		/* roll $1 %edi */
	movl	%eax,		%ebp
	movl	%edi,		28(%esp)
	orl	%ecx,		%ebp
	leal	2400959708(%edi,%edx,1),%edi
	movl	%eax,		%edx
	andl	%ebx,		%ebp
	andl	%ecx,		%edx
	orl	%edx,		%ebp
	movl	%esi,		%edx
	roll	$5,		%edx
.byte 209
.byte 200		/* rorl $1 %eax */
	addl	%edx,		%ebp
.byte 209
.byte 200		/* rorl $1 %eax */
	addl	%ebp,		%edi
	/* 40_59 55 */
	/* 40_59 56 */
	movl	32(%esp),	%edx
	movl	40(%esp),	%ebp
	xorl	%ebp,		%edx
	movl	(%esp),		%ebp
	xorl	%ebp,		%edx
	movl	20(%esp),	%ebp
	xorl	%ebp,		%edx
	movl	%esi,		%ebp
.byte 209
.byte 194		/* roll $1 %edx */
	orl	%eax,		%ebp
	movl	%edx,		32(%esp)
	andl	%ecx,		%ebp
	leal	2400959708(%edx,%ebx,1),%edx
	movl	%esi,		%ebx
.byte 209
.byte 206		/* rorl $1 %esi */
	andl	%eax,		%ebx
	orl	%ebx,		%ebp
	movl	%edi,		%ebx
	roll	$5,		%ebx
	addl	%ebx,		%ebp
	movl	36(%esp),	%ebx
	addl	%ebp,		%edx
	movl	44(%esp),	%ebp
	xorl	%ebp,		%ebx
	movl	4(%esp),	%ebp
	xorl	%ebp,		%ebx
	movl	24(%esp),	%ebp
.byte 209
.byte 206		/* rorl $1 %esi */
	xorl	%ebp,		%ebx
.byte 209
.byte 195		/* roll $1 %ebx */
	movl	%edi,		%ebp
	movl	%ebx,		36(%esp)
	orl	%esi,		%ebp
	leal	2400959708(%ebx,%ecx,1),%ebx
	movl	%edi,		%ecx
	andl	%eax,		%ebp
	andl	%esi,		%ecx
	orl	%ecx,		%ebp
	movl	%edx,		%ecx
	roll	$5,		%ecx
.byte 209
.byte 207		/* rorl $1 %edi */
	addl	%ecx,		%ebp
.byte 209
.byte 207		/* rorl $1 %edi */
	addl	%ebp,		%ebx
	/* 40_59 57 */
	/* 40_59 58 */
	movl	40(%esp),	%ecx
	movl	48(%esp),	%ebp
	xorl	%ebp,		%ecx
	movl	8(%esp),	%ebp
	xorl	%ebp,		%ecx
	movl	28(%esp),	%ebp
	xorl	%ebp,		%ecx
	movl	%edx,		%ebp
.byte 209
.byte 193		/* roll $1 %ecx */
	orl	%edi,		%ebp
	movl	%ecx,		40(%esp)
	andl	%esi,		%ebp
	leal	2400959708(%ecx,%eax,1),%ecx
	movl	%edx,		%eax
.byte 209
.byte 202		/* rorl $1 %edx */
	andl	%edi,		%eax
	orl	%eax,		%ebp
	movl	%ebx,		%eax
	roll	$5,		%eax
	addl	%eax,		%ebp
	movl	44(%esp),	%eax
	addl	%ebp,		%ecx
	movl	52(%esp),	%ebp
	xorl	%ebp,		%eax
	movl	12(%esp),	%ebp
	xorl	%ebp,		%eax
	movl	32(%esp),	%ebp
.byte 209
.byte 202		/* rorl $1 %edx */
	xorl	%ebp,		%eax
.byte 209
.byte 192		/* roll $1 %eax */
	movl	%ebx,		%ebp
	movl	%eax,		44(%esp)
	orl	%edx,		%ebp
	leal	2400959708(%eax,%esi,1),%eax
	movl	%ebx,		%esi
	andl	%edi,		%ebp
	andl	%edx,		%esi
	orl	%esi,		%ebp
	movl	%ecx,		%esi
	roll	$5,		%esi
.byte 209
.byte 203		/* rorl $1 %ebx */
	addl	%esi,		%ebp
.byte 209
.byte 203		/* rorl $1 %ebx */
	addl	%ebp,		%eax
	/* 40_59 59 */
	/* 20_39 60 */
	movl	48(%esp),	%esi
	movl	56(%esp),	%ebp
	xorl	%ebp,		%esi
	movl	16(%esp),	%ebp
	xorl	%ebp,		%esi
	movl	36(%esp),	%ebp
	xorl	%ebp,		%esi
	movl	%ecx,		%ebp
.byte 209
.byte 198		/* roll $1 %esi */
	xorl	%ebx,		%ebp
	movl	%esi,		48(%esp)
	xorl	%edx,		%ebp
	leal	3395469782(%esi,%edi,1),%esi
	movl	%eax,		%edi
	roll	$5,		%edi
.byte 209
.byte 201		/* rorl $1 %ecx */
	addl	%ebp,		%edi
.byte 209
.byte 201		/* rorl $1 %ecx */
	addl	%edi,		%esi
	/* 20_39 61 */
	movl	52(%esp),	%edi
	movl	60(%esp),	%ebp
	xorl	%ebp,		%edi
	movl	20(%esp),	%ebp
	xorl	%ebp,		%edi
	movl	40(%esp),	%ebp
	xorl	%ebp,		%edi
	movl	%eax,		%ebp
.byte 209
.byte 199		/* roll $1 %edi */
	xorl	%ecx,		%ebp
	movl	%edi,		52(%esp)
	xorl	%ebx,		%ebp
	leal	3395469782(%edi,%edx,1),%edi
	movl	%esi,		%edx
	roll	$5,		%edx
.byte 209
.byte 200		/* rorl $1 %eax */
	addl	%ebp,		%edx
.byte 209
.byte 200		/* rorl $1 %eax */
	addl	%edx,		%edi
	/* 20_39 62 */
	movl	56(%esp),	%edx
	movl	(%esp),		%ebp
	xorl	%ebp,		%edx
	movl	24(%esp),	%ebp
	xorl	%ebp,		%edx
	movl	44(%esp),	%ebp
	xorl	%ebp,		%edx
	movl	%esi,		%ebp
.byte 209
.byte 194		/* roll $1 %edx */
	xorl	%eax,		%ebp
	movl	%edx,		56(%esp)
	xorl	%ecx,		%ebp
	leal	3395469782(%edx,%ebx,1),%edx
	movl	%edi,		%ebx
	roll	$5,		%ebx
.byte 209
.byte 206		/* rorl $1 %esi */
	addl	%ebp,		%ebx
.byte 209
.byte 206		/* rorl $1 %esi */
	addl	%ebx,		%edx
	/* 20_39 63 */
	movl	60(%esp),	%ebx
	movl	4(%esp),	%ebp
	xorl	%ebp,		%ebx
	movl	28(%esp),	%ebp
	xorl	%ebp,		%ebx
	movl	48(%esp),	%ebp
	xorl	%ebp,		%ebx
	movl	%edi,		%ebp
.byte 209
.byte 195		/* roll $1 %ebx */
	xorl	%esi,		%ebp
	movl	%ebx,		60(%esp)
	xorl	%eax,		%ebp
	leal	3395469782(%ebx,%ecx,1),%ebx
	movl	%edx,		%ecx
	roll	$5,		%ecx
.byte 209
.byte 207		/* rorl $1 %edi */
	addl	%ebp,		%ecx
.byte 209
.byte 207		/* rorl $1 %edi */
	addl	%ecx,		%ebx
	/* 20_39 64 */
	movl	(%esp),		%ecx
	movl	8(%esp),	%ebp
	xorl	%ebp,		%ecx
	movl	32(%esp),	%ebp
	xorl	%ebp,		%ecx
	movl	52(%esp),	%ebp
	xorl	%ebp,		%ecx
	movl	%edx,		%ebp
.byte 209
.byte 193		/* roll $1 %ecx */
	xorl	%edi,		%ebp
	movl	%ecx,		(%esp)
	xorl	%esi,		%ebp
	leal	3395469782(%ecx,%eax,1),%ecx
	movl	%ebx,		%eax
	roll	$5,		%eax
.byte 209
.byte 202		/* rorl $1 %edx */
	addl	%ebp,		%eax
.byte 209
.byte 202		/* rorl $1 %edx */
	addl	%eax,		%ecx
	/* 20_39 65 */
	movl	4(%esp),	%eax
	movl	12(%esp),	%ebp
	xorl	%ebp,		%eax
	movl	36(%esp),	%ebp
	xorl	%ebp,		%eax
	movl	56(%esp),	%ebp
	xorl	%ebp,		%eax
	movl	%ebx,		%ebp
.byte 209
.byte 192		/* roll $1 %eax */
	xorl	%edx,		%ebp
	movl	%eax,		4(%esp)
	xorl	%edi,		%ebp
	leal	3395469782(%eax,%esi,1),%eax
	movl	%ecx,		%esi
	roll	$5,		%esi
.byte 209
.byte 203		/* rorl $1 %ebx */
	addl	%ebp,		%esi
.byte 209
.byte 203		/* rorl $1 %ebx */
	addl	%esi,		%eax
	/* 20_39 66 */
	movl	8(%esp),	%esi
	movl	16(%esp),	%ebp
	xorl	%ebp,		%esi
	movl	40(%esp),	%ebp
	xorl	%ebp,		%esi
	movl	60(%esp),	%ebp
	xorl	%ebp,		%esi
	movl	%ecx,		%ebp
.byte 209
.byte 198		/* roll $1 %esi */
	xorl	%ebx,		%ebp
	movl	%esi,		8(%esp)
	xorl	%edx,		%ebp
	leal	3395469782(%esi,%edi,1),%esi
	movl	%eax,		%edi
	roll	$5,		%edi
.byte 209
.byte 201		/* rorl $1 %ecx */
	addl	%ebp,		%edi
.byte 209
.byte 201		/* rorl $1 %ecx */
	addl	%edi,		%esi
	/* 20_39 67 */
	movl	12(%esp),	%edi
	movl	20(%esp),	%ebp
	xorl	%ebp,		%edi
	movl	44(%esp),	%ebp
	xorl	%ebp,		%edi
	movl	(%esp),		%ebp
	xorl	%ebp,		%edi
	movl	%eax,		%ebp
.byte 209
.byte 199		/* roll $1 %edi */
	xorl	%ecx,		%ebp
	movl	%edi,		12(%esp)
	xorl	%ebx,		%ebp
	leal	3395469782(%edi,%edx,1),%edi
	movl	%esi,		%edx
	roll	$5,		%edx
.byte 209
.byte 200		/* rorl $1 %eax */
	addl	%ebp,		%edx
.byte 209
.byte 200		/* rorl $1 %eax */
	addl	%edx,		%edi
	/* 20_39 68 */
	movl	16(%esp),	%edx
	movl	24(%esp),	%ebp
	xorl	%ebp,		%edx
	movl	48(%esp),	%ebp
	xorl	%ebp,		%edx
	movl	4(%esp),	%ebp
	xorl	%ebp,		%edx
	movl	%esi,		%ebp
.byte 209
.byte 194		/* roll $1 %edx */
	xorl	%eax,		%ebp
	movl	%edx,		16(%esp)
	xorl	%ecx,		%ebp
	leal	3395469782(%edx,%ebx,1),%edx
	movl	%edi,		%ebx
	roll	$5,		%ebx
.byte 209
.byte 206		/* rorl $1 %esi */
	addl	%ebp,		%ebx
.byte 209
.byte 206		/* rorl $1 %esi */
	addl	%ebx,		%edx
	/* 20_39 69 */
	movl	20(%esp),	%ebx
	movl	28(%esp),	%ebp
	xorl	%ebp,		%ebx
	movl	52(%esp),	%ebp
	xorl	%ebp,		%ebx
	movl	8(%esp),	%ebp
	xorl	%ebp,		%ebx
	movl	%edi,		%ebp
.byte 209
.byte 195		/* roll $1 %ebx */
	xorl	%esi,		%ebp
	movl	%ebx,		20(%esp)
	xorl	%eax,		%ebp
	leal	3395469782(%ebx,%ecx,1),%ebx
	movl	%edx,		%ecx
	roll	$5,		%ecx
.byte 209
.byte 207		/* rorl $1 %edi */
	addl	%ebp,		%ecx
.byte 209
.byte 207		/* rorl $1 %edi */
	addl	%ecx,		%ebx
	/* 20_39 70 */
	movl	24(%esp),	%ecx
	movl	32(%esp),	%ebp
	xorl	%ebp,		%ecx
	movl	56(%esp),	%ebp
	xorl	%ebp,		%ecx
	movl	12(%esp),	%ebp
	xorl	%ebp,		%ecx
	movl	%edx,		%ebp
.byte 209
.byte 193		/* roll $1 %ecx */
	xorl	%edi,		%ebp
	movl	%ecx,		24(%esp)
	xorl	%esi,		%ebp
	leal	3395469782(%ecx,%eax,1),%ecx
	movl	%ebx,		%eax
	roll	$5,		%eax
.byte 209
.byte 202		/* rorl $1 %edx */
	addl	%ebp,		%eax
.byte 209
.byte 202		/* rorl $1 %edx */
	addl	%eax,		%ecx
	/* 20_39 71 */
	movl	28(%esp),	%eax
	movl	36(%esp),	%ebp
	xorl	%ebp,		%eax
	movl	60(%esp),	%ebp
	xorl	%ebp,		%eax
	movl	16(%esp),	%ebp
	xorl	%ebp,		%eax
	movl	%ebx,		%ebp
.byte 209
.byte 192		/* roll $1 %eax */
	xorl	%edx,		%ebp
	movl	%eax,		28(%esp)
	xorl	%edi,		%ebp
	leal	3395469782(%eax,%esi,1),%eax
	movl	%ecx,		%esi
	roll	$5,		%esi
.byte 209
.byte 203		/* rorl $1 %ebx */
	addl	%ebp,		%esi
.byte 209
.byte 203		/* rorl $1 %ebx */
	addl	%esi,		%eax
	/* 20_39 72 */
	movl	32(%esp),	%esi
	movl	40(%esp),	%ebp
	xorl	%ebp,		%esi
	movl	(%esp),		%ebp
	xorl	%ebp,		%esi
	movl	20(%esp),	%ebp
	xorl	%ebp,		%esi
	movl	%ecx,		%ebp
.byte 209
.byte 198		/* roll $1 %esi */
	xorl	%ebx,		%ebp
	movl	%esi,		32(%esp)
	xorl	%edx,		%ebp
	leal	3395469782(%esi,%edi,1),%esi
	movl	%eax,		%edi
	roll	$5,		%edi
.byte 209
.byte 201		/* rorl $1 %ecx */
	addl	%ebp,		%edi
.byte 209
.byte 201		/* rorl $1 %ecx */
	addl	%edi,		%esi
	/* 20_39 73 */
	movl	36(%esp),	%edi
	movl	44(%esp),	%ebp
	xorl	%ebp,		%edi
	movl	4(%esp),	%ebp
	xorl	%ebp,		%edi
	movl	24(%esp),	%ebp
	xorl	%ebp,		%edi
	movl	%eax,		%ebp
.byte 209
.byte 199		/* roll $1 %edi */
	xorl	%ecx,		%ebp
	movl	%edi,		36(%esp)
	xorl	%ebx,		%ebp
	leal	3395469782(%edi,%edx,1),%edi
	movl	%esi,		%edx
	roll	$5,		%edx
.byte 209
.byte 200		/* rorl $1 %eax */
	addl	%ebp,		%edx
.byte 209
.byte 200		/* rorl $1 %eax */
	addl	%edx,		%edi
	/* 20_39 74 */
	movl	40(%esp),	%edx
	movl	48(%esp),	%ebp
	xorl	%ebp,		%edx
	movl	8(%esp),	%ebp
	xorl	%ebp,		%edx
	movl	28(%esp),	%ebp
	xorl	%ebp,		%edx
	movl	%esi,		%ebp
.byte 209
.byte 194		/* roll $1 %edx */
	xorl	%eax,		%ebp
	movl	%edx,		40(%esp)
	xorl	%ecx,		%ebp
	leal	3395469782(%edx,%ebx,1),%edx
	movl	%edi,		%ebx
	roll	$5,		%ebx
.byte 209
.byte 206		/* rorl $1 %esi */
	addl	%ebp,		%ebx
.byte 209
.byte 206		/* rorl $1 %esi */
	addl	%ebx,		%edx
	/* 20_39 75 */
	movl	44(%esp),	%ebx
	movl	52(%esp),	%ebp
	xorl	%ebp,		%ebx
	movl	12(%esp),	%ebp
	xorl	%ebp,		%ebx
	movl	32(%esp),	%ebp
	xorl	%ebp,		%ebx
	movl	%edi,		%ebp
.byte 209
.byte 195		/* roll $1 %ebx */
	xorl	%esi,		%ebp
	movl	%ebx,		44(%esp)
	xorl	%eax,		%ebp
	leal	3395469782(%ebx,%ecx,1),%ebx
	movl	%edx,		%ecx
	roll	$5,		%ecx
.byte 209
.byte 207		/* rorl $1 %edi */
	addl	%ebp,		%ecx
.byte 209
.byte 207		/* rorl $1 %edi */
	addl	%ecx,		%ebx
	/* 20_39 76 */
	movl	48(%esp),	%ecx
	movl	56(%esp),	%ebp
	xorl	%ebp,		%ecx
	movl	16(%esp),	%ebp
	xorl	%ebp,		%ecx
	movl	36(%esp),	%ebp
	xorl	%ebp,		%ecx
	movl	%edx,		%ebp
.byte 209
.byte 193		/* roll $1 %ecx */
	xorl	%edi,		%ebp
	movl	%ecx,		48(%esp)
	xorl	%esi,		%ebp
	leal	3395469782(%ecx,%eax,1),%ecx
	movl	%ebx,		%eax
	roll	$5,		%eax
.byte 209
.byte 202		/* rorl $1 %edx */
	addl	%ebp,		%eax
.byte 209
.byte 202		/* rorl $1 %edx */
	addl	%eax,		%ecx
	/* 20_39 77 */
	movl	52(%esp),	%eax
	movl	60(%esp),	%ebp
	xorl	%ebp,		%eax
	movl	20(%esp),	%ebp
	xorl	%ebp,		%eax
	movl	40(%esp),	%ebp
	xorl	%ebp,		%eax
	movl	%ebx,		%ebp
.byte 209
.byte 192		/* roll $1 %eax */
	xorl	%edx,		%ebp
	movl	%eax,		52(%esp)
	xorl	%edi,		%ebp
	leal	3395469782(%eax,%esi,1),%eax
	movl	%ecx,		%esi
	roll	$5,		%esi
.byte 209
.byte 203		/* rorl $1 %ebx */
	addl	%ebp,		%esi
.byte 209
.byte 203		/* rorl $1 %ebx */
	addl	%esi,		%eax
	/* 20_39 78 */
	movl	56(%esp),	%esi
	movl	(%esp),		%ebp
	xorl	%ebp,		%esi
	movl	24(%esp),	%ebp
	xorl	%ebp,		%esi
	movl	44(%esp),	%ebp
	xorl	%ebp,		%esi
	movl	%ecx,		%ebp
.byte 209
.byte 198		/* roll $1 %esi */
	xorl	%ebx,		%ebp
	movl	%esi,		56(%esp)
	xorl	%edx,		%ebp
	leal	3395469782(%esi,%edi,1),%esi
	movl	%eax,		%edi
	roll	$5,		%edi
.byte 209
.byte 201		/* rorl $1 %ecx */
	addl	%ebp,		%edi
.byte 209
.byte 201		/* rorl $1 %ecx */
	addl	%edi,		%esi
	/* 20_39 79 */
	movl	60(%esp),	%edi
	movl	4(%esp),	%ebp
	xorl	%ebp,		%edi
	movl	28(%esp),	%ebp
	xorl	%ebp,		%edi
	movl	48(%esp),	%ebp
	xorl	%ebp,		%edi
	movl	%eax,		%ebp
.byte 209
.byte 199		/* roll $1 %edi */
	xorl	%ecx,		%ebp
	movl	%edi,		60(%esp)
	xorl	%ebx,		%ebp
	leal	3395469782(%edi,%edx,1),%edi
	movl	%esi,		%edx
	roll	$5,		%edx
	addl	%ebp,		%edx
	movl	128(%esp),	%ebp
.byte 209
.byte 200		/* rorl $1 %eax */
	addl	%edx,		%edi
.byte 209
.byte 200		/* rorl $1 %eax */
	/* End processing */

	movl	12(%ebp),	%edx
	addl	%ecx,		%edx
	movl	4(%ebp),	%ecx
	addl	%esi,		%ecx
	movl	%eax,		%esi
	movl	(%ebp),		%eax
	movl	%edx,		12(%ebp)
	addl	%edi,		%eax
	movl	16(%ebp),	%edi
	addl	%ebx,		%edi
	movl	8(%ebp),	%ebx
	addl	%esi,		%ebx
	movl	%eax,		(%ebp)
	movl	132(%esp),	%esi
	movl	%ebx,		8(%ebp)
	addl	$64,		%esi
	movl	68(%esp),	%eax
	movl	%edi,		16(%ebp)
	cmpl	%eax,		%esi
	movl	%ecx,		4(%ebp)
	jl	L000start
	addl	$108,		%esp
	popl	%edi
	popl	%ebx
	popl	%ebp
	popl	%esi
	ret

ENTRY(sha1_block_asm_host_order)
	movl	12(%esp),	%ecx
	pushl	%esi
	sall	$6,		%ecx
	movl	12(%esp),	%esi
	pushl	%ebp
	addl	%esi,		%ecx
	pushl	%ebx
	movl	16(%esp),	%ebp
	pushl	%edi
	movl	12(%ebp),	%edx
	subl	$108,		%esp
	movl	16(%ebp),	%edi
	movl	8(%ebp),	%ebx
	movl	%ecx,		68(%esp)
	/* First we need to setup the X array */
	movl	(%esi),		%eax
	movl	4(%esi),	%ecx
	movl	%eax,		(%esp)
	movl	%ecx,		4(%esp)
	movl	8(%esi),	%eax
	movl	12(%esi),	%ecx
	movl	%eax,		8(%esp)
	movl	%ecx,		12(%esp)
	movl	16(%esi),	%eax
	movl	20(%esi),	%ecx
	movl	%eax,		16(%esp)
	movl	%ecx,		20(%esp)
	movl	24(%esi),	%eax
	movl	28(%esi),	%ecx
	movl	%eax,		24(%esp)
	movl	%ecx,		28(%esp)
	movl	32(%esi),	%eax
	movl	36(%esi),	%ecx
	movl	%eax,		32(%esp)
	movl	%ecx,		36(%esp)
	movl	40(%esi),	%eax
	movl	44(%esi),	%ecx
	movl	%eax,		40(%esp)
	movl	%ecx,		44(%esp)
	movl	48(%esi),	%eax
	movl	52(%esi),	%ecx
	movl	%eax,		48(%esp)
	movl	%ecx,		52(%esp)
	movl	56(%esi),	%eax
	movl	60(%esi),	%ecx
	movl	%eax,		56(%esp)
	movl	%ecx,		60(%esp)
	jmp	L001shortcut
