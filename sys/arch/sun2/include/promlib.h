/*	$NetBSD: promlib.h,v 1.8.74.1 2008/06/02 13:22:45 mjf Exp $ */

/*-
 * Copyright (c) 1999, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg and Matthew Fredette.
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

/*
 * Macros and functions to hide the Sun2 PROM interface from the 
 * common Sun parts of the kernel.
 */

#ifndef _MACHINE_PROMLIB_H_
#define _MACHINE_PROMLIB_H_

#include <machine/mon.h>

#ifdef	_SUN2_PROMLIB_PRIVATE
#define	romVectorPtr	((struct sunromvec *) SUN2_PROM_BASE)
#endif	/* _SUN2_PROMLIB_PRIVATE */

/*
 * We define both of these, since there are places where their
 * functionality is the same, and some common code knows that
 * (e.g. dev/sun/zs.c).  
 */
#define PROM_OLDMON	0
#define PROM_OBP_V0	1

void	prom_init(void);	/* To setup promops */

#define	prom_version()	(PROM_OLDMON)
int	prom_memsize(void);
int	prom_stdin(void);
int	prom_stdout(void);
int	prom_kbdid(void);
int	prom_getchar(void);
int	prom_peekchar(void);
void	prom_putchar(int);
void	prom_putstr(char *, int);
void	prom_printf(const char *, ...);
void	prom_abort(void);
void	prom_halt(void) __attribute__((__noreturn__));
void	prom_boot(const char *) __attribute__((__noreturn__));
char	*prom_getbootpath(void);
char	*prom_getbootfile(void);
char	*prom_getbootargs(void);
int	prom_sd_target(int);
#define	callrom		prom_abort

/*
 * We also provide these, to keep #ifdef'ing down in common
 * code.  These should be revisited.  The worst offender is
 * our definition of CPU_ISSUN4, for the benefit of 
 * sys/dev/sun/fb.c.
 */
#define PROM_OBP_V2	2
#define PROM_OBP_V3	3
#define PROM_OPENFIRM	4
#define prom_getpropint(a, b, c) (0)
#define CPU_ISSUN4 (1)

#endif /* _MACHINE_PROMLIB_H_ */
