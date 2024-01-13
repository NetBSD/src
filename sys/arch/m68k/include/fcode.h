/*	$NetBSD: fcode.h,v 1.2 2024/01/13 00:44:42 thorpej Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: cpu.h 1.16 91/03/25$
 *
 *	@(#)cpu.h	8.4 (Berkeley) 1/5/94
 */

#ifndef _M68K_FCODE_H_
#define	_M68K_FCODE_H_

/*
 * 680x0 Function Codes.
 *
 * Section 4.2 of the 68030 User's Manual describes the address spaces
 * selected by the Function Code:
 *
 *	FC2	FC1	FC0	Address space
 *	-----------------------------------------------------------
 *	 0	 0	 0	(Undefined, reserved for Motorola)
 *	 0	 0	 1	User Data space
 *	 0	 1	 0	User Program space
 *	 0	 1	 1	(Undefined, reserved for user)
 *	 1	 0	 0	(Undefined, reserved for Motorola)
 *	 1	 0	 1	Supervisor Data space
 *	 1	 1	 0	Supervisor Program space
 *	 1	 1	 1	CPU space
 */
#define	FC_UNDEF0	0
#define	FC_USERD	1	/* user data space */
#define	FC_USERP	2	/* user program space */
#define	FC_UNDEF3	3
#define	FC_UNDEF4	4
#define	FC_SUPERD	5	/* supervisor data space */
#define	FC_SUPERP	6	/* supervisor program space */
#define	FC_CPU		7	/* CPU space */

#ifdef _KERNEL

static inline int
getdfc(void)
{
	int rv;

	__asm volatile("movc %%dfc,%0" : "=d" (rv));

	return rv;
}

static inline int
getsfc(void)
{
	int rv;

	__asm volatile("movc %%sfc,%0" : "=d" (rv));

	return rv;
}

#endif /* _KERNEL */

#endif /* _M68K_FCODE_H_ */
