/*	$NetBSD: rtld_start.S,v 1.1 2003/03/25 13:11:54 scw Exp $	*/

/*
 * Copyright 2003 Wasabi Systems, Inc.
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

#ifdef _LP64
#define	PTRSZ	8
#else
#define	PTRSZ	4
#endif


#define	RTLD_START_FR_R2		0x00
#define	RTLD_START_FR_R3		0x08
#define	RTLD_START_FR_R4		0x10
#define	RTLD_START_FR_R7		0x18
#define	RTLD_START_FR_R28		0x20
#define	RTLD_START_FR_CLEANUP		0x28
#define	RTLD_START_FR_OBJ		(RTLD_START_FR_CLEANUP+PTRSZ)
#define	RTLD_START_FR_SIZE		(RTLD_START_FR_OBJ+PTRSZ)

ASENTRY_NOPROFILE(.rtld_start)
	addi	r15, -RTLD_START_FR_SIZE, r15
	st.q	r15, RTLD_START_FR_R2, r2
	st.q	r15, RTLD_START_FR_R3, r3
	st.q	r15, RTLD_START_FR_R4, r4
	st.q	r15, RTLD_START_FR_R7, r7
	st.q	r15, RTLD_START_FR_R28, r28

	pt/u	datalabel _GLOBAL_OFFSET_TABLE_, tr0
	gettr	tr0, r0
	pt/u	datalabel _DYNAMIC, tr1
	LDPTR	r0, 0, r28
	gettr	tr1, r2			/* &_DYNAMIC */
	sub	r2, r28, r28
	or	r28, r63, r3		/* relocbase */

	pt/l	_C_LABEL(_rtld_relocate_nonplt_self), tr0
	blink	tr0, r18
	add	r28, r63, r3		/* relocbase */
	addi	r15, RTLD_START_FR_CLEANUP, r2
	pt/l	_C_LABEL(_rtld), tr0
	blink	tr0, r18
	ptabs/l	r2, tr0

	ld.q	r15, RTLD_START_FR_R2, r2		/* argc */
	ld.q	r15, RTLD_START_FR_R3, r3		/* argv */
	ld.q	r15, RTLD_START_FR_R4, r4		/* envp */
	ld.q	r15, RTLD_START_FR_R7, r7		/* pstrings */
	ld.q	r15, RTLD_START_FR_R28, r28
	LDPTR	r15, RTLD_START_FR_CLEANUP, r5		/* cleanup */
	LDPTR	r15, RTLD_START_FR_OBJ, r6		/* obj */
	addi	r15, RTLD_START_FR_SIZE, r15
	blink	tr0, r63
	.size	_ASM_LABEL(.rtld_start),.-_ASM_LABEL(.rtld_start)


#define	RTLD_BIND_START_FR_R2		0x00
#define	RTLD_BIND_START_FR_R3		0x08
#define	RTLD_BIND_START_FR_R4		0x10
#define	RTLD_BIND_START_FR_R5		0x18
#define	RTLD_BIND_START_FR_R6		0x20
#define	RTLD_BIND_START_FR_R7		0x28
#define	RTLD_BIND_START_FR_R8		0x30
#define	RTLD_BIND_START_FR_R9		0x38
#define	RTLD_BIND_START_FR_DR0		0x40
#define	RTLD_BIND_START_FR_DR2		0x48
#define	RTLD_BIND_START_FR_DR4		0x50
#define	RTLD_BIND_START_FR_DR6		0x58
#define	RTLD_BIND_START_FR_DR8		0x60
#define	RTLD_BIND_START_FR_DR10		0x68
#define	RTLD_BIND_START_FR_SIZE		0x70

	/*
	 * On entry:
	 * r12 -> _GLOBAL_OFFSET_TABLE_ + GOT_BIAS (0x8000)
	 * r17 -> Obj_Entry *
	 * r21 -> reloff
	 */
ASENTRY_NOPROFILE(_rtld_bind_start)
	LINK_FRAME(RTLD_BIND_START_FR_SIZE)
	st.q	r14, RTLD_BIND_START_FR_R2, r2
	st.q	r14, RTLD_BIND_START_FR_R3, r3
	st.q	r14, RTLD_BIND_START_FR_R4, r4
	st.q	r14, RTLD_BIND_START_FR_R5, r5
	st.q	r14, RTLD_BIND_START_FR_R6, r6
	st.q	r14, RTLD_BIND_START_FR_R7, r7
	st.q	r14, RTLD_BIND_START_FR_R8, r8
	st.q	r14, RTLD_BIND_START_FR_R9, r9
	fst.d	r14, RTLD_BIND_START_FR_DR0, dr0
	fst.d	r14, RTLD_BIND_START_FR_DR2, dr2
	fst.d	r14, RTLD_BIND_START_FR_DR4, dr4
	fst.d	r14, RTLD_BIND_START_FR_DR6, dr6
	fst.d	r14, RTLD_BIND_START_FR_DR8, dr8
	fst.d	r14, RTLD_BIND_START_FR_DR10, dr10
	pt/l	_C_LABEL(_rtld_bind), tr0
	add	r17, r63, r2
	add	r21, r63, r3
	blink	tr0, r18
	ptabs/l	r2, tr0
	ld.q	r14, RTLD_BIND_START_FR_R2, r2
	ld.q	r14, RTLD_BIND_START_FR_R3, r3
	ld.q	r14, RTLD_BIND_START_FR_R4, r4
	ld.q	r14, RTLD_BIND_START_FR_R5, r5
	ld.q	r14, RTLD_BIND_START_FR_R6, r6
	ld.q	r14, RTLD_BIND_START_FR_R7, r7
	ld.q	r14, RTLD_BIND_START_FR_R8, r8
	ld.q	r14, RTLD_BIND_START_FR_R9, r9
	fld.d	r14, RTLD_BIND_START_FR_DR0, dr0
	fld.d	r14, RTLD_BIND_START_FR_DR2, dr2
	fld.d	r14, RTLD_BIND_START_FR_DR4, dr4
	fld.d	r14, RTLD_BIND_START_FR_DR6, dr6
	fld.d	r14, RTLD_BIND_START_FR_DR8, dr8
	fld.d	r14, RTLD_BIND_START_FR_DR10, dr10
	UNLINK_FRAME(RTLD_BIND_START_FR_SIZE)
	blink	tr0, r63
	.size	_ASM_LABEL(_rtld_bind_start),.-_ASM_LABEL(_rtld_bind_start)
