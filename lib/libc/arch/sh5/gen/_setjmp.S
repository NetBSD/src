/*	$NetBSD: _setjmp.S,v 1.1 2002/07/05 13:31:30 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <machine/asm.h>

#define _O(n)	((n)*8)

/*
 * C library -- _setjmp, _longjmp
 *
 *      _longjmp(a,v)
 * will generate a "return(v)" from
 * the last call to
 *      _setjmp(a)
 * by restoring registers from the stack,
 * The previous signal state is NOT restored.
 */

ENTRY_NOPROFILE(_setjmp)
	st.q	r2, _O(_JB_R10), r10
	st.q	r2, _O(_JB_R11), r11
	st.q	r2, _O(_JB_R12), r12
	st.q	r2, _O(_JB_R13), r13
	st.q	r2, _O(_JB_R14), r14
	st.q	r2, _O(_JB_R26), r26
	st.q	r2, _O(_JB_R27), r27
	st.q	r2, _O(_JB_R28), r28
	st.q	r2, _O(_JB_R29), r29
	st.q	r2, _O(_JB_R30), r30
	st.q	r2, _O(_JB_R31), r31
	st.q	r2, _O(_JB_R32), r32
	st.q	r2, _O(_JB_R33), r33
	st.q	r2, _O(_JB_R34), r34
	st.q	r2, _O(_JB_R35), r35
	st.q	r2, _O(_JB_R44), r44
	st.q	r2, _O(_JB_R45), r45
	st.q	r2, _O(_JB_R46), r46
	st.q	r2, _O(_JB_R47), r47
	st.q	r2, _O(_JB_R48), r48
	st.q	r2, _O(_JB_R49), r49
	st.q	r2, _O(_JB_R50), r50
	st.q	r2, _O(_JB_R51), r51
	st.q	r2, _O(_JB_R52), r52
	st.q	r2, _O(_JB_R53), r53
	st.q	r2, _O(_JB_R54), r54
	st.q	r2, _O(_JB_R55), r55
	st.q	r2, _O(_JB_R56), r56
	st.q	r2, _O(_JB_R57), r57
	st.q	r2, _O(_JB_R58), r58
	st.q	r2, _O(_JB_R59), r59
	ptabs/l	r18, tr0
	gettr	tr5, r0
	st.q	r2, _O(_JB_TR5), r0
	gettr	tr6, r0
	st.q	r2, _O(_JB_TR6), r0
	gettr	tr7, r0
	st.q	r2, _O(_JB_TR7), r0
	st.q	r2, _O(_JB_SP), r15
	st.q	r2, _O(_JB_PC), r18
	movi	0, r2
	blink	tr0, r63


ENTRY_NOPROFILE(_longjmp)
	ld.q	r2, _O(_JB_R10), r10
	ld.q	r2, _O(_JB_R11), r11
	ld.q	r2, _O(_JB_R12), r12
	ld.q	r2, _O(_JB_R13), r13
	ld.q	r2, _O(_JB_R14), r14
	ld.q	r2, _O(_JB_R26), r26
	ld.q	r2, _O(_JB_R27), r27
	ld.q	r2, _O(_JB_R28), r28
	ld.q	r2, _O(_JB_R29), r29
	ld.q	r2, _O(_JB_R30), r30
	ld.q	r2, _O(_JB_R31), r31
	ld.q	r2, _O(_JB_R32), r32
	ld.q	r2, _O(_JB_R33), r33
	ld.q	r2, _O(_JB_R34), r34
	ld.q	r2, _O(_JB_R35), r35
	ld.q	r2, _O(_JB_R40), r44
	ld.q	r2, _O(_JB_R45), r45
	ld.q	r2, _O(_JB_R46), r46
	ld.q	r2, _O(_JB_R47), r47
	ld.q	r2, _O(_JB_R48), r48
	ld.q	r2, _O(_JB_R49), r49
	ld.q	r2, _O(_JB_R50), r50
	ld.q	r2, _O(_JB_R51), r51
	ld.q	r2, _O(_JB_R52), r52
	ld.q	r2, _O(_JB_R53), r53
	ld.q	r2, _O(_JB_R54), r54
	ld.q	r2, _O(_JB_R55), r55
	ld.q	r2, _O(_JB_R56), r56
	ld.q	r2, _O(_JB_R57), r57
	ld.q	r2, _O(_JB_R58), r58
	ld.q	r2, _O(_JB_R59), r59
	ld.q	r2, _O(_JB_TR5), r0
	ptabs/u	r0, tr5
	ld.q	r2, _O(_JB_TR6), r0
	ptabs/u	r0, tr6
	ld.q	r2, _O(_JB_TR7), r0
	ptabs/u	r0, tr7
	ld.q	r2, _O(_JB_PC), r18
	ptabs/l	r18, tr0
	ld.q	r2, _O(_JB_SP), r15
	movi	1, r2
	cmvne	r3, r3, r2
	blink	tr0, r63
