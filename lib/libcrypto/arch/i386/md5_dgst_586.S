/*	$NetBSD: md5_dgst_586.S,v 1.1 2000/07/31 19:08:03 thorpej Exp $	*/

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
 * Modified from the output of `perl md5-586.pl elf' by
 * Jason R. Thorpe <thorpej@zembu.com>.
 */

#include <machine/asm.h>

ENTRY(md5_block_asm_host_order)
	pushl	%esi
	pushl	%edi
	movl	12(%esp),	%edi
	movl	16(%esp),	%esi
	movl	20(%esp),	%ecx
	pushl	%ebp
	sall	$6,		%ecx
	pushl	%ebx
	addl	%esi,		%ecx
	subl	$64,		%ecx
	movl	(%edi),		%eax
	pushl	%ecx
	movl	4(%edi),	%ebx
	movl	8(%edi),	%ecx
	movl	12(%edi),	%edx
L000start:

	/* R0 section */
	movl	%ecx,		%edi
	movl	(%esi),		%ebp
	/* R0 0 */
	xorl	%edx,		%edi
	andl	%ebx,		%edi
	leal	3614090360(%eax,%ebp,1),%eax
	xorl	%edx,		%edi
	addl	%edi,		%eax
	movl	%ebx,		%edi
	roll	$7,		%eax
	movl	4(%esi),	%ebp
	addl	%ebx,		%eax
	/* R0 1 */
	xorl	%ecx,		%edi
	andl	%eax,		%edi
	leal	3905402710(%edx,%ebp,1),%edx
	xorl	%ecx,		%edi
	addl	%edi,		%edx
	movl	%eax,		%edi
	roll	$12,		%edx
	movl	8(%esi),	%ebp
	addl	%eax,		%edx
	/* R0 2 */
	xorl	%ebx,		%edi
	andl	%edx,		%edi
	leal	606105819(%ecx,%ebp,1),%ecx
	xorl	%ebx,		%edi
	addl	%edi,		%ecx
	movl	%edx,		%edi
	roll	$17,		%ecx
	movl	12(%esi),	%ebp
	addl	%edx,		%ecx
	/* R0 3 */
	xorl	%eax,		%edi
	andl	%ecx,		%edi
	leal	3250441966(%ebx,%ebp,1),%ebx
	xorl	%eax,		%edi
	addl	%edi,		%ebx
	movl	%ecx,		%edi
	roll	$22,		%ebx
	movl	16(%esi),	%ebp
	addl	%ecx,		%ebx
	/* R0 4 */
	xorl	%edx,		%edi
	andl	%ebx,		%edi
	leal	4118548399(%eax,%ebp,1),%eax
	xorl	%edx,		%edi
	addl	%edi,		%eax
	movl	%ebx,		%edi
	roll	$7,		%eax
	movl	20(%esi),	%ebp
	addl	%ebx,		%eax
	/* R0 5 */
	xorl	%ecx,		%edi
	andl	%eax,		%edi
	leal	1200080426(%edx,%ebp,1),%edx
	xorl	%ecx,		%edi
	addl	%edi,		%edx
	movl	%eax,		%edi
	roll	$12,		%edx
	movl	24(%esi),	%ebp
	addl	%eax,		%edx
	/* R0 6 */
	xorl	%ebx,		%edi
	andl	%edx,		%edi
	leal	2821735955(%ecx,%ebp,1),%ecx
	xorl	%ebx,		%edi
	addl	%edi,		%ecx
	movl	%edx,		%edi
	roll	$17,		%ecx
	movl	28(%esi),	%ebp
	addl	%edx,		%ecx
	/* R0 7 */
	xorl	%eax,		%edi
	andl	%ecx,		%edi
	leal	4249261313(%ebx,%ebp,1),%ebx
	xorl	%eax,		%edi
	addl	%edi,		%ebx
	movl	%ecx,		%edi
	roll	$22,		%ebx
	movl	32(%esi),	%ebp
	addl	%ecx,		%ebx
	/* R0 8 */
	xorl	%edx,		%edi
	andl	%ebx,		%edi
	leal	1770035416(%eax,%ebp,1),%eax
	xorl	%edx,		%edi
	addl	%edi,		%eax
	movl	%ebx,		%edi
	roll	$7,		%eax
	movl	36(%esi),	%ebp
	addl	%ebx,		%eax
	/* R0 9 */
	xorl	%ecx,		%edi
	andl	%eax,		%edi
	leal	2336552879(%edx,%ebp,1),%edx
	xorl	%ecx,		%edi
	addl	%edi,		%edx
	movl	%eax,		%edi
	roll	$12,		%edx
	movl	40(%esi),	%ebp
	addl	%eax,		%edx
	/* R0 10 */
	xorl	%ebx,		%edi
	andl	%edx,		%edi
	leal	4294925233(%ecx,%ebp,1),%ecx
	xorl	%ebx,		%edi
	addl	%edi,		%ecx
	movl	%edx,		%edi
	roll	$17,		%ecx
	movl	44(%esi),	%ebp
	addl	%edx,		%ecx
	/* R0 11 */
	xorl	%eax,		%edi
	andl	%ecx,		%edi
	leal	2304563134(%ebx,%ebp,1),%ebx
	xorl	%eax,		%edi
	addl	%edi,		%ebx
	movl	%ecx,		%edi
	roll	$22,		%ebx
	movl	48(%esi),	%ebp
	addl	%ecx,		%ebx
	/* R0 12 */
	xorl	%edx,		%edi
	andl	%ebx,		%edi
	leal	1804603682(%eax,%ebp,1),%eax
	xorl	%edx,		%edi
	addl	%edi,		%eax
	movl	%ebx,		%edi
	roll	$7,		%eax
	movl	52(%esi),	%ebp
	addl	%ebx,		%eax
	/* R0 13 */
	xorl	%ecx,		%edi
	andl	%eax,		%edi
	leal	4254626195(%edx,%ebp,1),%edx
	xorl	%ecx,		%edi
	addl	%edi,		%edx
	movl	%eax,		%edi
	roll	$12,		%edx
	movl	56(%esi),	%ebp
	addl	%eax,		%edx
	/* R0 14 */
	xorl	%ebx,		%edi
	andl	%edx,		%edi
	leal	2792965006(%ecx,%ebp,1),%ecx
	xorl	%ebx,		%edi
	addl	%edi,		%ecx
	movl	%edx,		%edi
	roll	$17,		%ecx
	movl	60(%esi),	%ebp
	addl	%edx,		%ecx
	/* R0 15 */
	xorl	%eax,		%edi
	andl	%ecx,		%edi
	leal	1236535329(%ebx,%ebp,1),%ebx
	xorl	%eax,		%edi
	addl	%edi,		%ebx
	movl	%ecx,		%edi
	roll	$22,		%ebx
	movl	4(%esi),	%ebp
	addl	%ecx,		%ebx

	/* R1 section */
	/* R1 16 */
	leal	4129170786(%eax,%ebp,1),%eax
	xorl	%ebx,		%edi
	andl	%edx,		%edi
	movl	24(%esi),	%ebp
	xorl	%ecx,		%edi
	addl	%edi,		%eax
	movl	%ebx,		%edi
	roll	$5,		%eax
	addl	%ebx,		%eax
	/* R1 17 */
	leal	3225465664(%edx,%ebp,1),%edx
	xorl	%eax,		%edi
	andl	%ecx,		%edi
	movl	44(%esi),	%ebp
	xorl	%ebx,		%edi
	addl	%edi,		%edx
	movl	%eax,		%edi
	roll	$9,		%edx
	addl	%eax,		%edx
	/* R1 18 */
	leal	643717713(%ecx,%ebp,1),%ecx
	xorl	%edx,		%edi
	andl	%ebx,		%edi
	movl	(%esi),		%ebp
	xorl	%eax,		%edi
	addl	%edi,		%ecx
	movl	%edx,		%edi
	roll	$14,		%ecx
	addl	%edx,		%ecx
	/* R1 19 */
	leal	3921069994(%ebx,%ebp,1),%ebx
	xorl	%ecx,		%edi
	andl	%eax,		%edi
	movl	20(%esi),	%ebp
	xorl	%edx,		%edi
	addl	%edi,		%ebx
	movl	%ecx,		%edi
	roll	$20,		%ebx
	addl	%ecx,		%ebx
	/* R1 20 */
	leal	3593408605(%eax,%ebp,1),%eax
	xorl	%ebx,		%edi
	andl	%edx,		%edi
	movl	40(%esi),	%ebp
	xorl	%ecx,		%edi
	addl	%edi,		%eax
	movl	%ebx,		%edi
	roll	$5,		%eax
	addl	%ebx,		%eax
	/* R1 21 */
	leal	38016083(%edx,%ebp,1),%edx
	xorl	%eax,		%edi
	andl	%ecx,		%edi
	movl	60(%esi),	%ebp
	xorl	%ebx,		%edi
	addl	%edi,		%edx
	movl	%eax,		%edi
	roll	$9,		%edx
	addl	%eax,		%edx
	/* R1 22 */
	leal	3634488961(%ecx,%ebp,1),%ecx
	xorl	%edx,		%edi
	andl	%ebx,		%edi
	movl	16(%esi),	%ebp
	xorl	%eax,		%edi
	addl	%edi,		%ecx
	movl	%edx,		%edi
	roll	$14,		%ecx
	addl	%edx,		%ecx
	/* R1 23 */
	leal	3889429448(%ebx,%ebp,1),%ebx
	xorl	%ecx,		%edi
	andl	%eax,		%edi
	movl	36(%esi),	%ebp
	xorl	%edx,		%edi
	addl	%edi,		%ebx
	movl	%ecx,		%edi
	roll	$20,		%ebx
	addl	%ecx,		%ebx
	/* R1 24 */
	leal	568446438(%eax,%ebp,1),%eax
	xorl	%ebx,		%edi
	andl	%edx,		%edi
	movl	56(%esi),	%ebp
	xorl	%ecx,		%edi
	addl	%edi,		%eax
	movl	%ebx,		%edi
	roll	$5,		%eax
	addl	%ebx,		%eax
	/* R1 25 */
	leal	3275163606(%edx,%ebp,1),%edx
	xorl	%eax,		%edi
	andl	%ecx,		%edi
	movl	12(%esi),	%ebp
	xorl	%ebx,		%edi
	addl	%edi,		%edx
	movl	%eax,		%edi
	roll	$9,		%edx
	addl	%eax,		%edx
	/* R1 26 */
	leal	4107603335(%ecx,%ebp,1),%ecx
	xorl	%edx,		%edi
	andl	%ebx,		%edi
	movl	32(%esi),	%ebp
	xorl	%eax,		%edi
	addl	%edi,		%ecx
	movl	%edx,		%edi
	roll	$14,		%ecx
	addl	%edx,		%ecx
	/* R1 27 */
	leal	1163531501(%ebx,%ebp,1),%ebx
	xorl	%ecx,		%edi
	andl	%eax,		%edi
	movl	52(%esi),	%ebp
	xorl	%edx,		%edi
	addl	%edi,		%ebx
	movl	%ecx,		%edi
	roll	$20,		%ebx
	addl	%ecx,		%ebx
	/* R1 28 */
	leal	2850285829(%eax,%ebp,1),%eax
	xorl	%ebx,		%edi
	andl	%edx,		%edi
	movl	8(%esi),	%ebp
	xorl	%ecx,		%edi
	addl	%edi,		%eax
	movl	%ebx,		%edi
	roll	$5,		%eax
	addl	%ebx,		%eax
	/* R1 29 */
	leal	4243563512(%edx,%ebp,1),%edx
	xorl	%eax,		%edi
	andl	%ecx,		%edi
	movl	28(%esi),	%ebp
	xorl	%ebx,		%edi
	addl	%edi,		%edx
	movl	%eax,		%edi
	roll	$9,		%edx
	addl	%eax,		%edx
	/* R1 30 */
	leal	1735328473(%ecx,%ebp,1),%ecx
	xorl	%edx,		%edi
	andl	%ebx,		%edi
	movl	48(%esi),	%ebp
	xorl	%eax,		%edi
	addl	%edi,		%ecx
	movl	%edx,		%edi
	roll	$14,		%ecx
	addl	%edx,		%ecx
	/* R1 31 */
	leal	2368359562(%ebx,%ebp,1),%ebx
	xorl	%ecx,		%edi
	andl	%eax,		%edi
	movl	20(%esi),	%ebp
	xorl	%edx,		%edi
	addl	%edi,		%ebx
	movl	%ecx,		%edi
	roll	$20,		%ebx
	addl	%ecx,		%ebx

	/* R2 section */
	/* R2 32 */
	xorl	%edx,		%edi
	xorl	%ebx,		%edi
	leal	4294588738(%eax,%ebp,1),%eax
	addl	%edi,		%eax
	roll	$4,		%eax
	movl	32(%esi),	%ebp
	movl	%ebx,		%edi
	/* R2 33 */
	leal	2272392833(%edx,%ebp,1),%edx
	addl	%ebx,		%eax
	xorl	%ecx,		%edi
	xorl	%eax,		%edi
	movl	44(%esi),	%ebp
	addl	%edi,		%edx
	movl	%eax,		%edi
	roll	$11,		%edx
	addl	%eax,		%edx
	/* R2 34 */
	xorl	%ebx,		%edi
	xorl	%edx,		%edi
	leal	1839030562(%ecx,%ebp,1),%ecx
	addl	%edi,		%ecx
	roll	$16,		%ecx
	movl	56(%esi),	%ebp
	movl	%edx,		%edi
	/* R2 35 */
	leal	4259657740(%ebx,%ebp,1),%ebx
	addl	%edx,		%ecx
	xorl	%eax,		%edi
	xorl	%ecx,		%edi
	movl	4(%esi),	%ebp
	addl	%edi,		%ebx
	movl	%ecx,		%edi
	roll	$23,		%ebx
	addl	%ecx,		%ebx
	/* R2 36 */
	xorl	%edx,		%edi
	xorl	%ebx,		%edi
	leal	2763975236(%eax,%ebp,1),%eax
	addl	%edi,		%eax
	roll	$4,		%eax
	movl	16(%esi),	%ebp
	movl	%ebx,		%edi
	/* R2 37 */
	leal	1272893353(%edx,%ebp,1),%edx
	addl	%ebx,		%eax
	xorl	%ecx,		%edi
	xorl	%eax,		%edi
	movl	28(%esi),	%ebp
	addl	%edi,		%edx
	movl	%eax,		%edi
	roll	$11,		%edx
	addl	%eax,		%edx
	/* R2 38 */
	xorl	%ebx,		%edi
	xorl	%edx,		%edi
	leal	4139469664(%ecx,%ebp,1),%ecx
	addl	%edi,		%ecx
	roll	$16,		%ecx
	movl	40(%esi),	%ebp
	movl	%edx,		%edi
	/* R2 39 */
	leal	3200236656(%ebx,%ebp,1),%ebx
	addl	%edx,		%ecx
	xorl	%eax,		%edi
	xorl	%ecx,		%edi
	movl	52(%esi),	%ebp
	addl	%edi,		%ebx
	movl	%ecx,		%edi
	roll	$23,		%ebx
	addl	%ecx,		%ebx
	/* R2 40 */
	xorl	%edx,		%edi
	xorl	%ebx,		%edi
	leal	681279174(%eax,%ebp,1),%eax
	addl	%edi,		%eax
	roll	$4,		%eax
	movl	(%esi),		%ebp
	movl	%ebx,		%edi
	/* R2 41 */
	leal	3936430074(%edx,%ebp,1),%edx
	addl	%ebx,		%eax
	xorl	%ecx,		%edi
	xorl	%eax,		%edi
	movl	12(%esi),	%ebp
	addl	%edi,		%edx
	movl	%eax,		%edi
	roll	$11,		%edx
	addl	%eax,		%edx
	/* R2 42 */
	xorl	%ebx,		%edi
	xorl	%edx,		%edi
	leal	3572445317(%ecx,%ebp,1),%ecx
	addl	%edi,		%ecx
	roll	$16,		%ecx
	movl	24(%esi),	%ebp
	movl	%edx,		%edi
	/* R2 43 */
	leal	76029189(%ebx,%ebp,1),%ebx
	addl	%edx,		%ecx
	xorl	%eax,		%edi
	xorl	%ecx,		%edi
	movl	36(%esi),	%ebp
	addl	%edi,		%ebx
	movl	%ecx,		%edi
	roll	$23,		%ebx
	addl	%ecx,		%ebx
	/* R2 44 */
	xorl	%edx,		%edi
	xorl	%ebx,		%edi
	leal	3654602809(%eax,%ebp,1),%eax
	addl	%edi,		%eax
	roll	$4,		%eax
	movl	48(%esi),	%ebp
	movl	%ebx,		%edi
	/* R2 45 */
	leal	3873151461(%edx,%ebp,1),%edx
	addl	%ebx,		%eax
	xorl	%ecx,		%edi
	xorl	%eax,		%edi
	movl	60(%esi),	%ebp
	addl	%edi,		%edx
	movl	%eax,		%edi
	roll	$11,		%edx
	addl	%eax,		%edx
	/* R2 46 */
	xorl	%ebx,		%edi
	xorl	%edx,		%edi
	leal	530742520(%ecx,%ebp,1),%ecx
	addl	%edi,		%ecx
	roll	$16,		%ecx
	movl	8(%esi),	%ebp
	movl	%edx,		%edi
	/* R2 47 */
	leal	3299628645(%ebx,%ebp,1),%ebx
	addl	%edx,		%ecx
	xorl	%eax,		%edi
	xorl	%ecx,		%edi
	movl	(%esi),		%ebp
	addl	%edi,		%ebx
	movl	$-1,		%edi
	roll	$23,		%ebx
	addl	%ecx,		%ebx

	/* R3 section */
	/* R3 48 */
	xorl	%edx,		%edi
	orl	%ebx,		%edi
	leal	4096336452(%eax,%ebp,1),%eax
	xorl	%ecx,		%edi
	movl	28(%esi),	%ebp
	addl	%edi,		%eax
	movl	$-1,		%edi
	roll	$6,		%eax
	xorl	%ecx,		%edi
	addl	%ebx,		%eax
	/* R3 49 */
	orl	%eax,		%edi
	leal	1126891415(%edx,%ebp,1),%edx
	xorl	%ebx,		%edi
	movl	56(%esi),	%ebp
	addl	%edi,		%edx
	movl	$-1,		%edi
	roll	$10,		%edx
	xorl	%ebx,		%edi
	addl	%eax,		%edx
	/* R3 50 */
	orl	%edx,		%edi
	leal	2878612391(%ecx,%ebp,1),%ecx
	xorl	%eax,		%edi
	movl	20(%esi),	%ebp
	addl	%edi,		%ecx
	movl	$-1,		%edi
	roll	$15,		%ecx
	xorl	%eax,		%edi
	addl	%edx,		%ecx
	/* R3 51 */
	orl	%ecx,		%edi
	leal	4237533241(%ebx,%ebp,1),%ebx
	xorl	%edx,		%edi
	movl	48(%esi),	%ebp
	addl	%edi,		%ebx
	movl	$-1,		%edi
	roll	$21,		%ebx
	xorl	%edx,		%edi
	addl	%ecx,		%ebx
	/* R3 52 */
	orl	%ebx,		%edi
	leal	1700485571(%eax,%ebp,1),%eax
	xorl	%ecx,		%edi
	movl	12(%esi),	%ebp
	addl	%edi,		%eax
	movl	$-1,		%edi
	roll	$6,		%eax
	xorl	%ecx,		%edi
	addl	%ebx,		%eax
	/* R3 53 */
	orl	%eax,		%edi
	leal	2399980690(%edx,%ebp,1),%edx
	xorl	%ebx,		%edi
	movl	40(%esi),	%ebp
	addl	%edi,		%edx
	movl	$-1,		%edi
	roll	$10,		%edx
	xorl	%ebx,		%edi
	addl	%eax,		%edx
	/* R3 54 */
	orl	%edx,		%edi
	leal	4293915773(%ecx,%ebp,1),%ecx
	xorl	%eax,		%edi
	movl	4(%esi),	%ebp
	addl	%edi,		%ecx
	movl	$-1,		%edi
	roll	$15,		%ecx
	xorl	%eax,		%edi
	addl	%edx,		%ecx
	/* R3 55 */
	orl	%ecx,		%edi
	leal	2240044497(%ebx,%ebp,1),%ebx
	xorl	%edx,		%edi
	movl	32(%esi),	%ebp
	addl	%edi,		%ebx
	movl	$-1,		%edi
	roll	$21,		%ebx
	xorl	%edx,		%edi
	addl	%ecx,		%ebx
	/* R3 56 */
	orl	%ebx,		%edi
	leal	1873313359(%eax,%ebp,1),%eax
	xorl	%ecx,		%edi
	movl	60(%esi),	%ebp
	addl	%edi,		%eax
	movl	$-1,		%edi
	roll	$6,		%eax
	xorl	%ecx,		%edi
	addl	%ebx,		%eax
	/* R3 57 */
	orl	%eax,		%edi
	leal	4264355552(%edx,%ebp,1),%edx
	xorl	%ebx,		%edi
	movl	24(%esi),	%ebp
	addl	%edi,		%edx
	movl	$-1,		%edi
	roll	$10,		%edx
	xorl	%ebx,		%edi
	addl	%eax,		%edx
	/* R3 58 */
	orl	%edx,		%edi
	leal	2734768916(%ecx,%ebp,1),%ecx
	xorl	%eax,		%edi
	movl	52(%esi),	%ebp
	addl	%edi,		%ecx
	movl	$-1,		%edi
	roll	$15,		%ecx
	xorl	%eax,		%edi
	addl	%edx,		%ecx
	/* R3 59 */
	orl	%ecx,		%edi
	leal	1309151649(%ebx,%ebp,1),%ebx
	xorl	%edx,		%edi
	movl	16(%esi),	%ebp
	addl	%edi,		%ebx
	movl	$-1,		%edi
	roll	$21,		%ebx
	xorl	%edx,		%edi
	addl	%ecx,		%ebx
	/* R3 60 */
	orl	%ebx,		%edi
	leal	4149444226(%eax,%ebp,1),%eax
	xorl	%ecx,		%edi
	movl	44(%esi),	%ebp
	addl	%edi,		%eax
	movl	$-1,		%edi
	roll	$6,		%eax
	xorl	%ecx,		%edi
	addl	%ebx,		%eax
	/* R3 61 */
	orl	%eax,		%edi
	leal	3174756917(%edx,%ebp,1),%edx
	xorl	%ebx,		%edi
	movl	8(%esi),	%ebp
	addl	%edi,		%edx
	movl	$-1,		%edi
	roll	$10,		%edx
	xorl	%ebx,		%edi
	addl	%eax,		%edx
	/* R3 62 */
	orl	%edx,		%edi
	leal	718787259(%ecx,%ebp,1),%ecx
	xorl	%eax,		%edi
	movl	36(%esi),	%ebp
	addl	%edi,		%ecx
	movl	$-1,		%edi
	roll	$15,		%ecx
	xorl	%eax,		%edi
	addl	%edx,		%ecx
	/* R3 63 */
	orl	%ecx,		%edi
	leal	3951481745(%ebx,%ebp,1),%ebx
	xorl	%edx,		%edi
	movl	24(%esp),	%ebp
	addl	%edi,		%ebx
	addl	$64,		%esi
	roll	$21,		%ebx
	movl	(%ebp),		%edi
	addl	%ecx,		%ebx
	addl	%edi,		%eax
	movl	4(%ebp),	%edi
	addl	%edi,		%ebx
	movl	8(%ebp),	%edi
	addl	%edi,		%ecx
	movl	12(%ebp),	%edi
	addl	%edi,		%edx
	movl	%eax,		(%ebp)
	movl	%ebx,		4(%ebp)
	movl	(%esp),		%edi
	movl	%ecx,		8(%ebp)
	movl	%edx,		12(%ebp)
	cmpl	%esi,		%edi
	jge	L000start
	popl	%eax
	popl	%ebx
	popl	%ebp
	popl	%edi
	popl	%esi
	ret
