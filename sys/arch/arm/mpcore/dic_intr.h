/*	$NetBSD: dic_intr.h,v 1.1.2.2 2011/04/21 01:40:52 rmind Exp $ */

/*
 * Copyright (c) 2010, 2011 Genetec Corporation.  All rights reserved.
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

#ifndef _ARM_MPCORE_DIC_INTR_H
#define _ARM_MPCORE_DIC_INTR_H

#ifndef	ARM_IRQ_HANDLER
#define	ARM_IRQ_HANDLER	_C_LABEL(mpcore_irq_handler)
#endif

#ifndef	PIC_MAXMAXSOURCES
#define	PIC_MAXMAXSOURCES	(256 + 64)
#endif

#ifndef	_LOCORE
int _splraise(int);
int _spllower(int);
void splx(int);

void mpcore_irq_handler(void *);

#endif	/* !_LOCORE */

#endif	/* _ARM_MPCORE_DIC_INTR_H */
