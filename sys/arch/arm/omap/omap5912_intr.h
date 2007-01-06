/*	$NetBSD: omap5912_intr.h,v 1.1 2007/01/06 00:53:11 christos Exp $ */

/*
 * Define the OMAP5912 specific information and then include the generic OMAP
 * interrupt header.
 */

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain this list of conditions
 *    and the following disclaimer.
 * 2. Redistributions in binary form must reproduce this list of conditions
 *    and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ANY
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARM_OMAP_OMAP5912_INTR_H_
#define _ARM_OMAP_OMAP5912_INTR_H_

#ifndef _LOCORE

#define OMAP_INTC_DEVICE		"omap5912intc"

#define OMAP_INT_L1_NIRQ		32	/* Number of level 1 IRQs */
#define OMAP_INT_L2_NIRQ		128	/* Number of level 2 IRQs */
#define OMAP_INT_L2_IRQ			0	/* L1 IRQ that L2 IRQs feed */
#define OMAP_INT_L2_FIQ			2	/* L1 IRQ that L2 FIQs feed */
#define OMAP_FREE_IRQ_NUM		28	/* Number of free IRQs */

/*
 * The interrupt controller is used before we do the kernel configuration, so
 * we have to have the register addresses hard-coded in here, instead of being
 * able to get them from the config file.
 */
#define OMAP_INT_L1_BASE	0xFFFECB00
#define OMAP_INT_L2_BASE	0xFFFE0000

#endif /* ! _LOCORE */

#include <arm/omap/omap_intr.h>		/* Include the OMAP common header */

#endif /* _ARM_OMAP_OMAP5912_INTR_H_ */
