/*	$NetBSD: _resumecontext.S,v 1.1.16.1 2008/01/09 01:33:54 matt Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wayne Knowles.
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

#include <machine/asm.h>
#include <machine/mcontext.h>

#if defined(SYSLIBC_SCCS) && !defined(lint)
	RCSID("$NetBSD: _resumecontext.S,v 1.1.16.1 2008/01/09 01:33:54 matt Exp $")
#endif /* SYSLIBC_SCCS && !lint */

	.import	_resumecontext, code

LEAF_ENTRY_NOPROFILE(__resumecontext)
#ifdef PIC
	ldw	HPPA_FRAME_EDP(%sp), %r19
	addil	LT%_C_LABEL(_resumecontext), %r19
	ldw	RT%_C_LABEL(_resumecontext)(%r1), %r1
#else
	ldil	L%_C_LABEL(_resumecontext), %r1
	ldo	R%_C_LABEL(_resumecontext)(%r1), %r1
#endif
	bv,n	%r0(%r1)
EXIT(__resumecontext)
	.end
