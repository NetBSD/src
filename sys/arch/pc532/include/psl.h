/*	$NetBSD: psl.h,v 1.27 2002/05/25 04:27:22 simonb Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)psl.h	5.2 (Berkeley) 1/18/91
 */

#ifndef _NS532_PSL_H_
#define _NS532_PSL_H_

/*
 * 32532 processor status longword.
 */
#define	PSL_C		0x00000001	/* carry bit */
#define	PSL_T		0x00000002	/* trace enable bit */
#define	PSL_L		0x00000004	/* less bit */
#define	PSL_V		0x00000010	/* overflow bit */
#define	PSL_F		0x00000020	/* flag bit */
#define	PSL_Z		0x00000040	/* zero bit */
#define	PSL_N		0x00000080	/* negative bit */

#define PSL_USER	0x00000100	/* User mode bit */
#define PSL_US		0x00000200	/* User stack mode bit */
#define PSL_P		0x00000400	/* Prevent TRC trap */
#define	PSL_I		0x00000800	/* interrupt enable bit */

#define	PSL_USERSET	(PSL_USER | PSL_US | PSL_I)
#define	PSL_USERSTATIC	(PSL_USER | PSL_US | PSL_I)
#define	USERMODE(psr)	(((psr) & PSL_USER) == PSL_USER)

/* The PSR versions ... */
#define PSR_USR PSL_USER

#endif /* _NS532_PSL_H_ */
