/*	$NetBSD: atomic_op_asm.h,v 1.1.2.1 2007/04/17 04:28:23 thorpej Exp $	*/

/*-
 * Copyright (c) 2006, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran and Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _ATOMIC_OP_ASM_H_
#define	_ATOMIC_OP_ASM_H_

#include <machine/asm.h>

#if defined(_KERNEL)

#include "opt_multiprocessor.h"
#include "opt_cputype.h"

/*
 * If we're configured for multiprocessor, add a "lock" prefix.
 * XXX We could nop this and hot-patch the "lock" in when we detect
 * XXX MP, but for now we don't bother.
 */
#if defined(MULTIPROCESSOR)
#define	LOCK		lock
#else
#define	LOCK		/* nothing */
#endif

#define	ATOMIC_OP_ALIAS(a,s)	STRONG_ALIAS(a,s)

#else /* _KERNEL */

/*
 * In user-space, we always use "lock" because we don't really have a good
 * way to fix-up all of the uses.
 */
#define	LOCK		lock

#define	ATOMIC_OP_ALIAS(a,s)	WEAK_ALIAS(a,s)

#endif /* _KERNEL */

#define	END(name,a)	.align a; LABEL(name)

#endif /* _ATOMIC_OP_ASM_H_ */
