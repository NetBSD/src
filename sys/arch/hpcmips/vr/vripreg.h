/*	$NetBSD: vripreg.h,v 1.2 2001/04/18 11:07:28 sato Exp $	*/

/*-
 * Copyright (c) 1999
 *         Shin Takemura and PocketBSD Project. All rights reserved.
 * Copyright (c) 2001 SATO Kazumi, All rights reserved.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
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
 */

#define VRIP_NO_ADDR		0x00000000
/*
 * VR4101-4121 registers
 */
#define VR4102_BCU_ADDR		0x0b000000
#define VR4102_DMAAU_ADDR	0x0b000020
#define VR4102_DCU_ADDR		0x0b000040
#define VR4102_CMU_ADDR		0x0b000060
#define VR4102_ICU_ADDR		0x0b000080
#define VR4102_PMU_ADDR		0x0b0000a0
#define VR4102_RTC_ADDR		0x0b0000c0
#define VR4102_DSU_ADDR		0x0b0000e0
#define VR4102_GIU_ADDR		0x0b000100
#define VR4102_PIU_ADDR		0x0b000120
#define VR4102_AIU_ADDR		0x0b000000	/* XXX */
#define VR4102_KIU_ADDR		0x0b000180
#define VR4102_DSIU_ADDR	0x0b0001a0
#define VR4102_LED_ADDR		0x0b000240
#define VR4102_SIU_ADDR		0x0c000000
#define VR4102_HSP_ADDR		0x0c000020
#define VR4102_FIR_ADDR		0x0b000000	/* XXX */
#define	VR4102_SCU_ARR		VRIP_NO_ADDR	/* XXX: no register */
#define VR4102_SDRAMU_ADDR	VRIP_NO_ADDR	/* XXX: no register */
#define VR4102_PCI_ADDR		VRIP_NO_ADDR	/* XXX: no register */
#define VR4102_PCICONF_ADDR	VRIP_NO_ADDR	/* XXX: no register */
#define VR4102_CSI_ADDR		VRIP_NO_ADDR	/* XXX: no register */

/*
 * VR4122 registers
 */
#define VR4122_BCU_ADDR		0x0f000000
#define VR4122_DMAAU_ADDR	0x0f000020
#define VR4122_DCU_ADDR		0x0f000040
#define VR4122_CMU_ADDR		0x0f000060
#define VR4122_ICU_ADDR		0x0f000080
#define VR4122_PMU_ADDR		0x0f000100
#define VR4122_RTC_ADDR		0x0f000140
#define VR4122_DSU_ADDR		VRIP_NO_ADDR	/* XXX: no register */
#define VR4122_GIU_ADDR		0x0f000140
#define VR4122_PIU_ADDR		VRIP_NO_ADDR	/* XXX: no register */
#define VR4122_AIU_ADDR		VRIP_NO_ADDR	/* XXX: no register */
#define VR4122_KIU_ADDR		VRIP_NO_ADDR	/* XXX: no register */
#define VR4122_DSIU_ADDR	0x0f000820
#define VR4122_LED_ADDR		0x0f000180
#define VR4122_SIU_ADDR		0x0f000800
#define VR4122_HSP_ADDR		VRIP_NO_ADDR	/* XXX: no register */
#define VR4122_FIR_ADDR		0x0f000840	/* XXX */
#define	VR4122_SCU_ARR		0x0f001000
#define VR4122_SDRAMU_ADDR	0x00000400
#define VR4122_PCI_ADDR		0x00000c00
#define VR4122_PCICONF_ADDR	0x00000d00
#define VR4122_CSI_ADDR		0x000001a0

/*
 * VRIP base address
 *
 * REQUIRE: opt_vr41xx.h, vrcpudef.h
 */
#include "opt_vr41xx.h"
#include <hpcmips/vr/vrcpudef.h>

#if !defined SINGLE_VRIP_BASE

#error currently missconfiguration.
#error NEED switch VRIP_BASE_ADDR by vr cpu type.

#else

#if defined VRGROUP_4181
#define VRIP_BASE_ADDR		0x0a000000
#endif /* VRGROUP_4181 */

#if defined VRGROUP_4122
#define VRIP_BASE_ADDR		0x0f000000

#define VRIP_BCU_ADDR		VR4122_BCU_ADDR	
#define VRIP_DMAAU_ADDR		VR4122_DMAAU_ADDR
#define VRIP_DCU_ADDR		VR4122_DCU_ADDR	
#define VRIP_CMU_ADDR		VR4122_CMU_ADDR	
#define VRIP_ICU_ADDR		VR4122_ICU_ADDR	
#define VRIP_PMU_ADDR		VR4122_PMU_ADDR	
#define VRIP_RTC_ADDR		VR4122_RTC_ADDR	
#define VRIP_DSU_ADDR		VR4122_DSU_ADDR	
#define VRIP_GIU_ADDR		VR4122_GIU_ADDR	
#define VRIP_PIU_ADDR		VR4122_PIU_ADDR	
#define VRIP_AIU_ADDR		VR4122_AIU_ADDR
#define VRIP_KIU_ADDR		VR4122_KIU_ADDR	
#define VRIP_DSIU_ADDR		VR4122_DSIU_ADDR
#define VRIP_LED_ADDR		VR4122_LED_ADDR	
#define VRIP_SIU_ADDR		VR4122_SIU_ADDR	
#define VRIP_HSP_ADDR		VR4122_HSP_ADDR	
#define VRIP_FIR_ADDR		VR4122_FIR_ADDR
#define	VRIP_SCU_ARR		VR4122_SCU_ARR		/* XXX: no register */
#define VRIP_SDRAMU_ADDR	VR4122_SDRAMU_ADDR	/* XXX: no register */
#define VRIP_PCI_ADDR		VR4122_PCI_ADDR		/* XXX: no register */
#define VRIP_PCICONF_ADDR	VR4122_PCICONF_ADDR	/* XXX: no register */
#define VRIP_CSI_ADDR		VR4122_CSI_ADDR		/* XXX: no register */

#endif /* VRGROUP_4122 */

#if defined VRGROUP_4102_4121
#define VRIP_BASE_ADDR		0x0b000000

#define VRIP_BCU_ADDR		VR4102_BCU_ADDR	
#define VRIP_DMAAU_ADDR		VR4102_DMAAU_ADDR
#define VRIP_DCU_ADDR		VR4102_DCU_ADDR	
#define VRIP_CMU_ADDR		VR4102_CMU_ADDR	
#define VRIP_ICU_ADDR		VR4102_ICU_ADDR	
#define VRIP_PMU_ADDR		VR4102_PMU_ADDR	
#define VRIP_RTC_ADDR		VR4102_RTC_ADDR	
#define VRIP_DSU_ADDR		VR4102_DSU_ADDR	
#define VRIP_GIU_ADDR		VR4102_GIU_ADDR	
#define VRIP_PIU_ADDR		VR4102_PIU_ADDR	
#define VRIP_AIU_ADDR		VR4102_AIU_ADDR
#define VRIP_KIU_ADDR		VR4102_KIU_ADDR	
#define VRIP_DSIU_ADDR		VR4102_DSIU_ADDR
#define VRIP_LED_ADDR		VR4102_LED_ADDR	
#define VRIP_SIU_ADDR		VR4102_SIU_ADDR	
#define VRIP_HSP_ADDR		VR4102_HSP_ADDR	
#define VRIP_FIR_ADDR		VR4102_FIR_ADDR
#define	VRIP_SCU_ARR		VR4102_SCU_ARR		/* XXX: no register */
#define VRIP_SDRAMU_ADDR	VR4102_SDRAMU_ADDR	/* XXX: no register */
#define VRIP_PCI_ADDR		VR4102_PCI_ADDR		/* XXX: no register */
#define VRIP_PCICONF_ADDR	VR4102_PCICONF_ADDR	/* XXX: no register */
#define VRIP_CSI_ADDR		VR4102_CSI_ADDR		/* XXX: no register */

#endif /* VRGROUP_4102_4121 */

#endif /* SINGLE_VRIP_BASE */

/*
 * ICU interrupt level
 */
/* reserved 			62-31 */
#define VRIP_INTR_BCU		25
#define VRIP_INTR_CSI		24
#define VRIP_INTR_SCU		23
#define VRIP_INTR_PCI		22
#define VRIP_INTR_DSIU		21
#define VRIP_INTR_FIR		20
#define VRIP_INTR_TCLK		19
#define VRIP_INTR_HSP		18
#define VRIP_INTR_LED		17
#define VRIP_INTR_RTCL2		16
/* reserved 			15,14 */
#define VRIP_INTR_DOZEPIU	13
#define VRIP_INTR_CLKRUN	12
#define VRIP_INTR_SOFT		11
#define VRIP_INTR_WRBERR	10
#define VRIP_INTR_SIU		9
#define VRIP_INTR_GIU		8
#define VRIP_INTR_KIU		7
#define VRIP_INTR_AIU		6
#define VRIP_INTR_PIU		5
/* reserved 			4	VRC4171 use this ??? */
#define VRIP_INTR_ETIMER	3
#define VRIP_INTR_RTCL1		2
#define VRIP_INTR_POWER		1
#define VRIP_INTR_BAT		0
