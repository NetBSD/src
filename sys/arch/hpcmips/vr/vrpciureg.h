/*	$NetBSD: vrpciureg.h,v 1.1 2001/06/13 07:32:48 enami Exp $	*/

/*-
 * Copyright (c) 2001 Enami Tsugutomo.
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
 */

#define	VRPCIU_BASE		0x0f000c00 /* vr4122 */

/*
 * Master Memory/IO Address Window.
 */
#define	VRPCIU_MMAW1REG		0x0000
#define	VRPCIU_MMAW2REG		0x0004
#define	VRPCIU_MIOAWREG		0x0010

#define	VRPCIU_MAW_IBAMASK	0xff000000 /* Internal Bus Base Address */
#define	VRPCIU_MAW_ADDRMASK(reg) \
	((((reg) >> 13) & 0x7f) << 24)	/* Address Mask */
#define	VRPCIU_MAW_ADDR(reg)	\
	(((reg) & VRPCIU_MAW_IBAMASK) & VRPCIU_MAW_ADDRMASK(reg))
#define	VRPCIU_MAW_SIZE(reg)	(~(VRPCIU_MAW_ADDRMASK(reg) | 0x80000000))
#define	VRPCIU_MAW_WINEN	(1 << 12) /* PCI access is enabled */
#define	VRPCIU_MAW_PCIADDR(reg) (((reg) & 0xff) << 24)	/* PCI Address */
#define	VRPCIU_MAW(start, size)	/* XXX */

/*
 * Target Address Window.
 */
#define	VRPCIU_TAW1REG		0x0008
#define	VRPCIU_TAW2REG		0x000c
#define	VRPCIU_TAW_ADDRMASK(reg) \
	((((reg) >> 13) & 0x7f) << 21)	/* Address Mask */
#define	VRPCIU_TAW_WINEN	(1 << 12) /* PCI access is enabled */
#define	VRPCIU_TAW_IBA(reg) (((reg) & 0x7ff) << 21) /* Internal Bus Address */

#define	VRPCIU_CONFDREG		0x0014
#define	VRPCIU_CONFAREG		0x0018
#define	VRPCIU_MAILREG		0x001c
#define	VRPCIU_BUSERRADREG	0x0024
#define	VRPCIU_INTCNTSTAREG	0x0028
#define	VRPCIU_EXACCREG		0x002c
#define	VRPCIU_RECONTREG	0x0030
#define	VRPCIU_ENREG		0x0034
#define	VRPCIU_CLKSELREG	0x0038
#define	VRPCIU_TRDYVREG		0x003c
#define	VRPCIU_CLKRUNREG	0x0060

#define	VRPCIU_CONF_TYPE1	0x1
#define	VRPCIU_CONF_BASE	(0x0f000d00 - VRPCIU_BASE)
#define	VRPCIU_CONF_MAILREG	0x10
#define	VRPCIU_CONF_MBA1REG	0x14
#define	VRPCIU_CONF_MBA2REG	0x18
