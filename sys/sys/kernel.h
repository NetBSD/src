/*	$NetBSD: kernel.h,v 1.18 2003/01/21 13:56:53 kleink Exp $	*/

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)kernel.h	8.3 (Berkeley) 1/21/94
 */

#ifndef _SYS_KERNEL_H_
#define _SYS_KERNEL_H_

/* Global variables for the kernel. */

extern long hostid;
extern char hostname[MAXHOSTNAMELEN];
extern int hostnamelen;
extern char domainname[MAXHOSTNAMELEN];
extern int domainnamelen;

extern volatile struct timeval mono_time;
extern struct timeval boottime;
extern volatile struct timeval time;

extern int rtc_offset;		/* offset of rtc from UTC in minutes */

extern int cold;		/* still working on startup */
extern int tick;		/* usec per tick (1000000 / hz) */
extern int tickfix;		/* periodic tick adj. tick not integral */
extern int tickfixinterval;	/* interval at which to apply adjustment */
extern int tickadj;		/* "standard" clock skew, us./tick */
extern int hz;			/* system clock's frequency */
extern int stathz;		/* statistics clock's frequency */
extern int profhz;		/* profiling clock's frequency */
extern int lbolt;		/* once a second sleep address */

extern int profsrc;		/* profiling source */
#if defined(_KERNEL)
#define PROFSRC_CLOCK	0
#endif

/*
 * These globals indicate the number of times hardlock() has ticked,
 * and the successive attempt of softclock() to catch up with it.  These
 * are large unsigned numbers so that arithmetic can be performed on them
 * with no reasonable worry about an overflow occurring (we get over 500
 * million years with this).
 *
 * IMPORTANT NOTE: you *must* be at splclock() in order to read both
 * hardclock_ticks and softclock_ticks.  This is because the 64-bit
 * quantities may not be readable in an atomic fashion on all CPU types.
 */
extern u_int64_t hardclock_ticks, softclock_ticks;

#endif /* _SYS_KERNEL_H_ */
