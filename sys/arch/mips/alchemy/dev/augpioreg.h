/* $NetBSD: augpioreg.h,v 1.3.8.2 2006/04/22 11:37:41 simonb Exp $ */

/*-
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Written by Garrett D'Amore for Itronix Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */ 

#ifndef _MIPS_ALCHEMY_DEV_AUGPIOREG_H
#define	_MIPS_ALCHEMY_DEV_AUGPIOREG_H

#define	AUGPIO_NPINS			64

/*
 * SYS_GPIO registers -- offset from GPIO_BASE.
 *
 * This excludes SYS_PINFUNC, that has to be included in the SYS_BASE
 * registers -- it is surrounded by other non-GPIO related regs.
 */
#define	AUGPIO_TRIOUTRD		0x00
#define	AUGPIO_TRIOUTCLR	0x00
#define	AUGPIO_OUTPUTRD		0x08
#define	AUGPIO_OUTPUTSET	0x08
#define	AUGPIO_OUTPUTCLR	0x0C
#define	AUGPIO_PINSTATERD	0x10
#define	AUGPIO_PININPUTEN	0x10
#define	AUGPIO_SIZE		0x14

/* GPIO2 registers -- offset from GPIO2_BASE */
#define	AUGPIO2_DIR		0x00
#define	AUGPIO2_OUTPUT		0x08
#define	AUGPIO2_PINSTATE	0x0C
#define	AUGPIO2_INTEN		0x10
#define	AUGPIO2_ENABLE		0x14
#define	AUGPIO2_SIZE		0x18

#define	AUGPIO_GPIO2_ENABLE_CE		0x1
#define	AUGPIO_GPIO2_ENABLE_MR		0x2

#endif	/* _MIPS_ALCHEMY_DEV_AUPCIREG_H */
