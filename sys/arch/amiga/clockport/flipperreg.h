/*      $NetBSD: flipperreg.h,v 1.1.4.2 2013/01/16 05:32:40 yamt Exp $ */

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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

#ifndef _AMIGA_FLIPPERREG_H_
#define _AMIGA_FLIPPERREG_H_

#define DELFINA_1200		0xB5
#define DELFINA_FLIPPER		0xB6

#define FLIPPER_REGSIZE		0x8

#define FLIPPER_DSP_ICR		0x0
#define FLIPPER_DSP_ISR		0x1
#define FLIPPER_HOSTCTL		0x2
#define FLIPPER_HOSTCTL_CODECSEL	__BIT(0)
#define FLIPPER_HOSTCTL_IRQDIS		__BIT(4)
#define FLIPPER_HOSTCTL_QUERYVER	__BIT(5)
#define FLIPPER_HOSTCTL_HENCLK		__BIT(6)
#define FLIPPER_HOSTCTL_RESET		__BIT(7)
#define FLIPPER_DSP_DM		0x3
#define FLIPPER_DSP_CVR		0x4
#define FLIPPER_DSP_IVR		0x5
#define FLIPPER_DSP_DH		0x6
#define FLIPPER_DSP_DL		0x7

#endif /* _AMIGA_FLIPPERREG_H_ */

