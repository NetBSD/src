/*	$NetBSD: swapcontext.S,v 1.2 2003/01/18 11:12:54 thorpej Exp $	*/

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

#if defined(LIBC_SCCS) && !defined(lint)
	.text
	.asciz "$NetBSD: swapcontext.S,v 1.2 2003/01/18 11:12:54 thorpej Exp $"
#endif /* LIBC_SCCS && !lint */

ENTRY(swapcontext)
	stwu	1, -16(1)			# set up new stack frame
	mflr	0
	stw	0, 20(1)			# save link register
	stw	3, 8(1)				# must save oucp
	stw	4, 12(1)			# must save ucp
	bl	PIC_PLT(_C_LABEL(_getcontext))	# getcontext(oucp)
	cmpwi	3, 0
	bne	1f
	lwz	11, 8(1)			# load oucp for adjustment
	lwz	0, 20(1)
	stw	0, (48 + 34 * 4)(11)		# pc <- lr
	la	0, 16(1)
	stw	0, (48 +  1 * 4)(11)		# adjust sp
	lwz	3, 12(1)			# load ucp
	bl	PIC_PLT(_C_LABEL(setcontext))	# setcontext(ucp)
1:
	lwz	0, 20(1)
	mtlr	0
	la	1, 16(1)
	blr
