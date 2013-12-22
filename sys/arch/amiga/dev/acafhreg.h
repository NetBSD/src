/*	$NetBSD: acafhreg.h,v 1.1 2013/12/22 02:21:51 rkujawa Exp $ */

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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

#ifndef _AMIGA_ACAFHREG_H_

#define GAYLE_IDE_BASE		0xDA0000 /* ACA500 has Gayle-compatible IDE */

#define ACAFH_MSB_SHIFT		0xF
#define ACAFH_MSB_MASK		0x8000

#define ACAFH_BASE		0xB00000
#define ACAFH_FIRST_REG_OFF	0x3000
#define ACAFH_END		0xB3B002
/* registers have stride of 16kB */
#define ACAFH_CF_DETECT_BOOT		0x0
#define ACAFH_CF_DETECT_AUX		0x1
#define ACAFH_CF_IRQ_BOOT		0x2
#define ACAFH_CF_IRQ_AUX		0x3

#define ACAFH_VERSION_BIT3		0x4
#define ACAFH_VERSION_BIT2		0x5
#define ACAFH_VERSION_BIT1		0x6
#define ACAFH_VERSION_BIT0		0x7

#define ACAFH_MAPROM			0x8
#define ACAFH_CHIPMAP			0x9
#define ACAFH_FLASH_WRITE		0xA
#define ACAFH_VBR_MOVE			0xB
#define ACAFH_MEMPROBE_AUXIRQ		0xC
#define ACAFH_POWERUP			0xD
#define ACAFH_C0WIPE			0xE

#endif /* _AMIGA_ACAFHREG_H_ */

