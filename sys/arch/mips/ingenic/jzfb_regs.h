/*	$NetBSD: jzfb_regs.h,v 1.2.8.2 2017/12/03 11:36:28 jdolecek Exp $ */

/*-
 * Copyright (c) 2015 Michael Lorenz
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

#ifndef JZFB_REGS_H
#define JZFB_REGS_H

#define JZ_LCDCFG		0x0000
#define JZ_LCDCTRL		0x0030
#define JZ_LCDSTATE		0x0034
#define JZ_LCDOSDC		0x0100
#define JZ_LCDOSDCTRL		0x0104
#define JZ_LCDOSDS		0x0108
#define JZ_LCDBGC0		0x010C
#define JZ_LCDBGC1		0x02C4
#define JZ_LCDKEY0		0x0110
#define JZ_LCDKEY1		0x0114
#define JZ_LCDALPHA		0x0118
#define JZ_LCDIPUR		0x011C
#define JZ_LCDRGBC		0x0090
#define JZ_LCDVAT		0x000C
#define JZ_LCDDAH		0x0010
#define JZ_LCDDAV		0x0014
#define JZ_LCDXYP0		0x0120
#define JZ_LCDXYP1		0x0124
#define JZ_LCDSIZE0		0x0128
#define JZ_LCDSIZE1		0x012C
#define JZ_LCDVSYNC		0x0004
#define JZ_LCDHSYNC		0x0008
#define JZ_LCDPS		0x0018
#define JZ_LCDCLS		0x001C
#define JZ_LCDSPL		0x0020
#define JZ_LCDREV		0x0024
#define JZ_LCDIID		0x0038
#define JZ_LCDDA0		0x0040
#define JZ_LCDSA0		0x0044
#define JZ_LCDFID0		0x0048
#define JZ_LCDCMD0		0x004C
#define JZ_LCDDA1		0x0050
#define JZ_LCDSA1		0x0054
#define JZ_LCDOFFS0		0x0060
#define JZ_LCDPW0		0x0064
#define JZ_LCDCNUM0		0x0068
#define JZ_LCDPOS0		0x0068
#define JZ_LCDDESSIZE0		0x006C
#define JZ_LCDFID1		0x0058
#define JZ_LCDCMD1		0x005C
#define JZ_LCDOFFS1		0x0070
#define JZ_LCDPW1		0x0074
#define JZ_LCDCNUM1		0x0078
#define JZ_LCDPOS1		0x0078
#define JZ_LCDDESSIZE1		0x007C
#define JZ_LCDPCFG		0x02C0
#define JZ_LCDDUALCTRL		0x02C8
#define JZ_LCDENH_CFG		0x0400
#define JZ_LCDENH_CSCCFG	0x0404
#define JZ_LCDENH_LUMACFG	0x0408
#define JZ_LCDENH_CHROCFG0	0x040C
#define JZ_LCDENH_CHROCFG1	0x0410
#define JZ_LCDENH_DITHERCFG	0x0414
#define JZ_LCDENH_STATUS	0x0418
#define JZ_LCDENH_GAMMA		0x0800
#define JZ_LCDENH_VEE		0x1000

#endif /* JZFB_REGS_H */
