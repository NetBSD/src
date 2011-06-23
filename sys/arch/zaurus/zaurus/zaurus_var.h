/*	$NetBSD: zaurus_var.h,v 1.2.86.1 2011/06/23 14:19:52 cherry Exp $ */
/*	$OpenBSD: zaurus_var.h,v 1.4 2005/07/01 23:56:47 uwe Exp $	*/
/*	NetBSD: lubbock_var.h,v 1.1 2003/06/18 10:51:15 bsh Exp */

/*
 * Copyright (c) 2002, 2003  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Genetec Corporation may not be used to endorse or 
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ZAURUS_ZAURUS_VAR_H
#define _ZAURUS_ZAURUS_VAR_H

#ifdef _KERNEL

#define ZAURUS_C860		0xC0860
#define ZAURUS_C1000		0xC1000
#define ZAURUS_C3000		0xC3000

#define ZAURUS_ISC860		(zaurusmod == ZAURUS_C860)
#define ZAURUS_ISC1000		(zaurusmod == ZAURUS_C1000)
#define ZAURUS_ISC3000		(zaurusmod == ZAURUS_C3000)

extern int zaurusmod;
extern int glass_console;

void zaurus_restart(void);

#endif	/* _KERNEL */

#endif /* _ZAURUS_ZAURUS_VAR_H */
