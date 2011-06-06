/*	$NetBSD: altivec_subr.S,v 1.2.8.2 2011/06/06 09:06:29 jruoho Exp $	*/
/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include "opt_altivec.h"

RCSID("$NetBSD: altivec_subr.S,v 1.2.8.2 2011/06/06 09:06:29 jruoho Exp $")

#ifdef ALTIVEC
/*
 * LINTSTUB: void vec_load_from_vreg(const struct vreg *vreg);
 */
ENTRY(vec_load_from_vreg)
	/*
	 * Restore VSCR by first loading it into a vector and then into
	 * VSCR. (this needs to done before loading the user's vector
	 * registers since we need to use a scratch vector register)
	 */
	vxor %v0,%v0,%v0
	li %r4,VREG_VSCR; lvewx %v0,%r3,%r4
	mtvscr %v0

	/*
	 * Now load the vector registers.  We do it this way so that if on
	 * a superscalar cpu, we can get some concurrency.
	 */
	li %r4,VREG_V0; lvx %v0,%r3,%r4
	li %r5,VREG_V1; lvx %v1,%r3,%r5
	li %r6,VREG_V2; lvx %v2,%r3,%r6
	li %r7,VREG_V3; lvx %v3,%r3,%r7

	li %r4,VREG_V4; lvx %v4,%r3,%r4
	li %r5,VREG_V5; lvx %v5,%r3,%r5
	li %r6,VREG_V6; lvx %v6,%r3,%r6
	li %r7,VREG_V7; lvx %v7,%r3,%r7

	li %r4,VREG_V8; lvx %v8,%r3,%r4
	li %r5,VREG_V9; lvx %v9,%r3,%r5
	li %r6,VREG_V10; lvx %v10,%r3,%r6
	li %r7,VREG_V11; lvx %v11,%r3,%r7

	li %r4,VREG_V12; lvx %v12,%r3,%r4
	li %r5,VREG_V13; lvx %v13,%r3,%r5
	li %r6,VREG_V14; lvx %v14,%r3,%r6
	li %r7,VREG_V15; lvx %v15,%r3,%r7

	li %r4,VREG_V16; lvx %v16,%r3,%r4
	li %r5,VREG_V17; lvx %v17,%r3,%r5
	li %r6,VREG_V18; lvx %v18,%r3,%r6
	li %r7,VREG_V19; lvx %v19,%r3,%r7

	li %r4,VREG_V20; lvx %v20,%r3,%r4
	li %r5,VREG_V21; lvx %v21,%r3,%r5
	li %r6,VREG_V22; lvx %v22,%r3,%r6
	li %r7,VREG_V23; lvx %v23,%r3,%r7

	li %r4,VREG_V24; lvx %v24,%r3,%r4
	li %r5,VREG_V25; lvx %v25,%r3,%r5
	li %r6,VREG_V26; lvx %v26,%r3,%r6
	li %r7,VREG_V27; lvx %v27,%r3,%r7

	li %r4,VREG_V28; lvx %v28,%r3,%r4
	li %r5,VREG_V29; lvx %v29,%r3,%r5
	li %r6,VREG_V30; lvx %v30,%r3,%r6
	li %r7,VREG_V31; lvx %v31,%r3,%r7

	isync
	blr
END(vec_load_from_vreg)

/*
 * LINTSTUB: void vec_unload_to_vreg(struct vreg *vreg);
 */
ENTRY(vec_unload_to_vreg)
	/*
	 * Store the vector registers.  We do it this way so that if on
	 * a superscalar cpu, we can get some concurrency.
	 */
	li %r4,VREG_V0; stvx %v0,%r3,%r4
	li %r5,VREG_V1; stvx %v1,%r3,%r5
	li %r6,VREG_V2; stvx %v2,%r3,%r6
	li %r7,VREG_V3; stvx %v3,%r3,%r7

	li %r4,VREG_V4; stvx %v4,%r3,%r4
	li %r5,VREG_V5; stvx %v5,%r3,%r5
	li %r6,VREG_V6; stvx %v6,%r3,%r6
	li %r7,VREG_V7; stvx %v7,%r3,%r7

	li %r4,VREG_V8; stvx %v8,%r3,%r4
	li %r5,VREG_V9; stvx %v9,%r3,%r5
	li %r6,VREG_V10; stvx %v10,%r3,%r6
	li %r7,VREG_V11; stvx %v11,%r3,%r7

	li %r4,VREG_V12; stvx %v12,%r3,%r4
	li %r5,VREG_V13; stvx %v13,%r3,%r5
	li %r6,VREG_V14; stvx %v14,%r3,%r6
	li %r7,VREG_V15; stvx %v15,%r3,%r7

	li %r4,VREG_V16; stvx %v16,%r3,%r4
	li %r5,VREG_V17; stvx %v17,%r3,%r5
	li %r6,VREG_V18; stvx %v18,%r3,%r6
	li %r7,VREG_V19; stvx %v19,%r3,%r7

	li %r4,VREG_V20; stvx %v20,%r3,%r4
	li %r5,VREG_V21; stvx %v21,%r3,%r5
	li %r6,VREG_V22; stvx %v22,%r3,%r6
	li %r7,VREG_V23; stvx %v23,%r3,%r7

	li %r4,VREG_V24; stvx %v24,%r3,%r4
	li %r5,VREG_V25; stvx %v25,%r3,%r5
	li %r6,VREG_V26; stvx %v26,%r3,%r6
	li %r7,VREG_V27; stvx %v27,%r3,%r7

	li %r4,VREG_V28; stvx %v28,%r3,%r4
	li %r5,VREG_V29; stvx %v29,%r3,%r5
	li %r6,VREG_V30; stvx %v30,%r3,%r6
	li %r7,VREG_V31; stvx %v31,%r3,%r7

	/*
	 * Save VSCR but remember to restore the vector that used to save it.
	 */
	mfvscr %v31
	li %r4,VREG_VSCR; stvewx %v31,%r3,%r4	/* low word only */

	lvx %v31,%r3,%r7	/* restore v31 */
	
	isync
	blr
END(vec_load_from_vreg)
#endif /* ALTIVEC */
