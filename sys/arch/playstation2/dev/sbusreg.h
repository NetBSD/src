/*	$NetBSD: sbusreg.h,v 1.4.6.2 2014/08/20 00:03:17 tls Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

#define SBUS_SMFLG_REG			MIPS_PHYS_TO_KSEG1(0x1000f230)
#define   SMFLG_PCMCIA_INT	0x00000100
#define   SMFLG_USB_INT		0x00000400

#define SBUS_AIF_INTSR_REG16		MIPS_PHYS_TO_KSEG1(0x18000004)
#define SBUS_AIF_INTEN_REG16		MIPS_PHYS_TO_KSEG1(0x18000006)

#define SBUS_PCMCIA_EXC1_REG16		MIPS_PHYS_TO_KSEG1(0x1f801476)
#define SBUS_PCMCIA_CSC1_REG16		MIPS_PHYS_TO_KSEG1(0x1f801464)
#define SBUS_PCMCIA_IMR1_REG16		MIPS_PHYS_TO_KSEG1(0x1f801468)
#define SBUS_PCMCIA_TIMR_REG16		MIPS_PHYS_TO_KSEG1(0x1f80147e)
#define SBUS_PCMCIA3_TIMR_REG16		MIPS_PHYS_TO_KSEG1(0x1f801466)
