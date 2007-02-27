/*	$NetBSD: pxa2x0cpu.h,v 1.3.2.1 2007/02/27 16:49:41 yamt Exp $ */

/*
 * Copyright (c) 2005  Genetec Corporation.  All rights reserved.
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
 *
 */

/*
 * In kernel config file, users can have options
 *    CPU_XSCALE_PXA250 and/or CPU_XSCALE_PXA270.
 *
 * If kernel is configured to support PXA250 and PXA270, CPU type is
 * determined run-time by reading a co-processor register.
 */

#ifndef	_ARM_XSCALE_PXA2X0CPU_H
#define	_ARM_XSCALE_PXA2X0CPU_H

#ifdef	_KERNEL_OPT
#include	"opt_cputypes.h" /* User's choice of CPU */
#endif

#if !defined(CPU_XSCALE_PXA250) && !defined(CPU_XSCALE_PXA270)
#error neither CPU_XSCALE_PXA250 nor CPU_XSCALE_PXA270 is defined.
#endif

#if defined(CPU_XSCALE_PXA250) || defined(CPU_XSCALE_PXA270)
# define  __CPU_XSCALE_PXA2XX
#endif

#define	CPU_ID_PXA_MASK	(CPU_ID_IMPLEMETOR_MASK|CPU_ID_VARIANT_MASK|\
			 CPU_ID_ARCH_MASK|CPU_ID_XSCALE_COREGEN_MASK)

#define	__CPU_IS_PXA250	((cpufunc_id() & CPU_ID_XSCALE_COREGEN_MASK) == 0x2000)
#define	__CPU_IS_PXA270	((cpufunc_id() & CPU_ID_XSCALE_COREGEN_MASK) == 0x4000)

# if defined(CPU_XSCALE_PXA250) && defined(CPU_XSCALE_PXA270)
#define	CPU_IS_PXA250	__CPU_IS_PXA250
#define	CPU_IS_PXA270	__CPU_IS_PXA270
#elif defined(CPU_XSCALE_PXA250) && !defined(CPU_XSCALE_PXA270)
#define	CPU_IS_PXA250	(1)
#define	CPU_IS_PXA270	(0)
#elif !defined(CPU_XSCALE_PXA250) && defined(CPU_XSCALE_PXA270)
#define	CPU_IS_PXA250	(0)
#define	CPU_IS_PXA270	(1)
#elif !defined(CPU_XSCALE_PXA250) && !defined(CPU_XSCALE_PXA270)
#define	CPU_IS_PXA250	(0)
#define	CPU_IS_PXA270	(0)
#endif

#include <arm/xscale/pxa2x0reg.h>

#ifdef	CPU_XSCALE_PXA270
#define	PXA2X0_GPIO_SIZE	PXA270_GPIO_SIZE
#define	GPIO_REG		PXA270_GPIO_REG
#define	GPIO_NPINS		PXA270_GPIO_NPINS
#define	PXA2X0_MEMCTL_SIZE	PXA270_MEMCTL_SIZE
#define	PXA2X0_USBDC_SIZE	PXA270_USBDC_SIZE
#define	PXA2X0_RTC_SIZE		PXA270_RTC_SIZE
#else
#define	PXA2X0_GPIO_SIZE	PXA250_GPIO_SIZE
#define	GPIO_REG		PXA250_GPIO_REG
#define	GPIO_NPINS		PXA250_GPIO_NPINS
#define	PXA2X0_MEMCTL_SIZE	PXA250_MEMCTL_SIZE
#define	PXA2X0_USBDC_SIZE	PXA250_USBDC_SIZE
#define	PXA2X0_RTC_SIZE		PXA250_RTC_SIZE
#endif

#endif	/* _ARM_XSCALE_PXA2X0CPU_H */
