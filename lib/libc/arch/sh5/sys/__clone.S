/*	$NetBSD: __clone.S,v 1.4 2003/08/10 20:39:15 scw Exp $	*/

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

#include <sys/errno.h>
#include "SYS.h"

#ifdef WEAK_ALIAS
WEAK_ALIAS(clone,__clone)
#endif

/*
 * int clone(int (*fn)(void *), void *stack, int flags, void *arg);
 *                 r2                 r3         r4           r5
 */
ENTRY(__clone)
	pta/u	1f, tr0		/* Preload error path */
	beq/u	r2, r63, tr0	/* Bomb if fn is NULL */
	beq/u	r3, r63, tr0	/* Bomb if stack is NULL */
	ptabs/u	r2, tr1 	/* Preserve "fn" in tr1 (syscall won't trash) */
	or	r4, r63, r2	/* Shuffle syscall args: flags */
	SYSTRAP(__clone)	/*         syscall args: stack already in r3 */
	pta/u	2f, tr0
	bne/u	r0, r63, tr0	/* Bail on error */
	ptabs/l	r18, tr0
	beq/l	r3, r63, tr0	/* If parent, just return. */
	or	r5, r63, r2	/* Clone's "arg" was safe in r5 */
	blink	tr1, r18	/* Invoke the clone */
	PIC_PROLOGUE
	PIC_PTAL(_C_LABEL(_exit), r0, tr0)
	blink	tr0, r63	/* If clone returns, invoke _exit(3) */
	/*NOTREACHED*/

1:	movi	EINVAL, r2
2:	PIC_PROLOGUE
	PIC_PTAL(_C_LABEL(__cerror), r0, tr0)
	blink	tr0, r63
	/*NOTREACHED*/
