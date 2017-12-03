/*	$NetBSD: exynos_intr.h,v 1.1.10.3 2017/12/03 11:35:56 jdolecek Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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

#ifndef _ARM_SAMSUNG_EXYNOS_INTR_H_
#define _ARM_SAMSUNG_EXYNOS_INTR_H_

/*
 * The GIC supports
 *   - 16 Software Generated Interrupts (SGIs)
 *   - 16 Private Peripheral Interrupts (PPIs)
 *   - 127 Shared Peripheral Interrupts (SPIs)
 */

#define	EXYNOS_NSPI		128
#define	EXYNOS_COMBINERBASE	EXYNOS_SPIBASE + EXYNOS_NSPI

#define	EXYNOS_BITSPERGROUP	8

#define	EXYNOS_COMBINERIRQ(g, b) \
    (EXYNOS_COMBINERBASE + ((g) * EXYNOS_BITSPERGROUP + (b)))

#define	IRQ_MCT_LTIMER		IRQ_PPI(12)

#endif /* _ARM_SAMSUNG_EXYNOS_INTR_H_ */

