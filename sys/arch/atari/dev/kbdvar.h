/*	$NetBSD: kbdvar.h,v 1.11 2022/06/25 14:39:19 tsutsui Exp $	*/

/*
 * Copyright (c) 1996 Leo Weppelman.
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _KBDVAR_H
#define _KBDVAR_H

/*
 * Package types
 */
#define	KBD_MEM_PKG	0		/* Memory read package		*/
#define	KBD_AMS_PKG	1		/* Absolute mouse package	*/
#define	KBD_RMS_PKG	2		/* Relative mouse package	*/
#define	KBD_CLK_PKG	3		/* Clock package		*/
#define	KBD_JOY0_PKG	4		/* Joystick-0 package		*/
#define	KBD_JOY1_PKG	5		/* Joystick-1 package		*/
#define	KBD_TIMEO_PKG	6		/* Timeout package		*/

#ifdef _KERNEL
extern	uint8_t	kbd_modifier;

void	kbd_bell_gparms(u_int *, u_int *, u_int *);
void	kbd_bell_sparms(u_int, u_int, u_int);
void	kbd_write(const uint8_t *, int);
int	kbdgetcn(void);
void	kbdbell(void);
void	kbdenable(void);

/* Interrupt handler */
void	kbdintr(int);	/* called from locore.s */
#endif /* _KERNEL */

#endif /* _KBDVAR_H */
