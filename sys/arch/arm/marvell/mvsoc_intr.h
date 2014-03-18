/*	$NetBSD: mvsoc_intr.h,v 1.4 2014/03/18 06:46:14 matt Exp $	*/
/*
 * Copyright (c) 2010 KIYOHARA Takashi
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _MVSOC_INTR_H_
#define _MVSOC_INTR_H_

#ifdef _INTR_PRIVATE
#include "opt_mvsoc.h"

#if defined(ARMADAXP)
#define __HAVE_PIC_SET_PRIORITY
#endif
#endif

#define ARM_IRQ_HANDLER	_C_LABEL(mvsoc_irq_handler)

#ifndef _LOCORE
extern int (*find_pending_irqs)(void);

void mvsoc_irq_handler(void *);

#include <arm/pic/picvar.h>

static __inline void *
marvell_intr_establish(int irq, int ipl, int (*func)(void *), void *arg)
{

	return intr_establish(irq, ipl, IST_LEVEL_HIGH, func, arg);
}

#endif	/* _LOCORE */

#endif	/* _MVSOC_INTR_H_ */
