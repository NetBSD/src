/*	$NetBSD: swapcontext.S,v 1.1 2003/01/21 21:03:25 kleink Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Klaus Klein.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include "SYS.h"

#if defined(SYSLIBC_SCCS) && !defined(lint)
	RCSID("$NetBSD: swapcontext.S,v 1.1 2003/01/21 21:03:25 kleink Exp $")
#endif /* SYSLIBC_SCCS && !lint */

ENTRY(swapcontext)
	movd	S_ARG0,tos		/* push oucp */
	bsr	_C_LABEL(_getcontext)	/* getcontext(oucp) */
	adjspd	-4
	cmpqd	0,r0			/* success? */
	bne	0f
	movd	S_ARG0,r2
	addr	4(sp),(36 +  8 * 4)(r2)	/* Adjust saved sp ... */
	movd	0(sp),(36 + 11 * 4)(r2)	/* ... and pc. */
	/* Note: a return value of 0 has been arranged for by getcontext(). */
	movd	S_ARG1,tos		/* push ucp */
	bsr	_C_LABEL(setcontext)	/* setcontext(ucp) */
	adjspd	-4
	/* No need to error-check: setcontext() must have failed. */
0:
	ret	0
