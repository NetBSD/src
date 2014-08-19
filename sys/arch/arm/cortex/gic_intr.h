/*	$NetBSD: gic_intr.h,v 1.1.2.1 2014/08/20 00:02:45 tls Exp $	*/
/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#ifndef _ARM_CORTEX_GIC_INTR_H_
#define _ARM_CORTEX_GIC_INTR_H_

#define	ARM_IRQ_HANDLER		_C_LABEL(armgic_irq_handler)

#ifndef _LOCORE

#define	__HAVE_PIC_SET_PRIORITY
#if defined(__HAVE_FAST_SOFTINTS) && 0
#define	__HAVE_PIC_FAST_SOFTINTS
#endif

#ifndef PIC_MAXSOURCES
#error PIC_MAXSOURCES needs to be defined
#endif
#ifndef PIC_MAXMAXSOURCES
#error PIC_MAXMAXSOURCES needs to be defined
#endif

#define IRQ_SGI(n)		( 0 + ((n) & 15))
#define IRQ_PPI(n)		(16 + ((n) & 15))
#define IRQ_SPI(n)		(32 + (n))
#define GIC_MAXSOURCES(n)	IRQ_SPI(n)

void armgic_irq_handler(void *);

#include <arm/pic/picvar.h>

#endif

#endif /* _ARM_CORTEX_GIC_INTR_H_ */
