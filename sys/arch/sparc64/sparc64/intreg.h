/*	$NetBSD: intreg.h,v 1.2.2.1 2000/06/22 17:04:33 minoura Exp $ */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	@(#)intreg.h	8.1 (Berkeley) 6/11/93
 */

#include <sparc64/sparc64/vaddrs.h>

/*
 * All sun4u interrupts arrive as interrupt packets.  These packets
 * consist of up to six (three on spitfires) quads.  The first one
 * contains the interrupt number.  This is either the device ID
 * or a pointer to the handler routine.
 * 
 * Device IDs are split into two parts: a 5-bit interrupt group number
 * and a 5-bit interrupt number.  We ignore this distinction.
 *
 */

#define MAXINTNUM	(1<<11)

/*
 * These masks are used for ienab_bis and ienab_bic, neither valid for
 * 4u machines.   We translate this to a set_softint and clear_softint.
 */
#define	IE_L14		(1<<14)	/* enable level 14 (counter 1) interrupts */
#define	IE_L10		(1<<10)	/* enable level 10 (counter 0) interrupts */
#define	IE_L8		(1<<8)	/* enable level 8 interrupts */
#define	IE_L6		(1<<6)	/* request software level 6 interrupt */
#define	IE_L4		(1<<4)	/* request software level 4 interrupt */
#define	IE_L1		(1<<1)	/* request software level 1 interrupt */

#ifndef _LOCORE
struct intrhand;	/* This is in cpu.h if you need it. */
void	send_softint __P((int cpu, int level, struct intrhand *ih));
#endif

