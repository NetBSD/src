/*	$NetBSD: sigsetjmp.S,v 1.1 1995/01/11 20:22:28 pk Exp $	*/
/*
 * Copyright (c) 1995 Paul Kranenburg
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
 *      This product includes software developed by Paul Kranenburg.
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
 *
 */

#include "SYS.h"

#ifdef PIC
	/*
	 * We cannot do PIC branches to external labels, hence all this crap.
	 */
	.data
L_setjmp:	.word	_setjmp
L__setjmp:	.word	__setjmp
L_longjmp:	.word	_longjmp
L__longjmp:	.word	__longjmp

	.text

ENTRY(sigsetjmp)
	PIC_PROLOGUE(%g1,%g2)
	cmp %o1,0
	st %o1,[%o0+40]		! jmpbuf[JBLEN]
	bne 1f
	nop
	ld [%g1+L__setjmp], %g1
	ba,a 2f
1:
	ld [%g1+L_setjmp], %g1
	ba,a 2f
	unimp 0

ENTRY(siglongjmp)
	PIC_PROLOGUE(%g1,%g2)
	ld [%o0+40],%g2
	cmp %g2,0
	bne 1f
	nop
	ld [%g1+L__longjmp], %g1
	ba,a 2f
1:
	ld [%g1+L_longjmp], %g1
2:
	ld [%g1],%g1
	jmp %g1
	nop
	unimp 0
#else

ENTRY(sigsetjmp)
	cmp %o1,0
	st %o1,[%o0+40]		! jmpbuf[JBLEN]
	be __setjmp
	nop
	ba _setjmp
	nop
	unimp 0

ENTRY(siglongjmp)
	ld [%o0+40],%g1
	cmp %g1,0
	be __longjmp
	nop
	ba _longjmp
	nop
	unimp 0
#endif
