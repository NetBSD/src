/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: platinumfbreg.h,v 1.1.18.2 2017/12/03 11:36:25 jdolecek Exp $");

#ifndef PLATINUMFBREG_H
#define PLATINUMFBREG_H

/*
 * platinum register address offsets.
 * registers are each 32 bits with 12*8bits of padding,
 * 128 registers total
 */
#define PLATINUM_REG_OFFSET_ADDR(x) (x * 0x10)
#define PLATINUM_REG_COUNT          128

/*
 * colormap register addresses.
 * registers are each 8 bits with 15*8 bits of padding
 */
#define PLATINUM_CMAP_BASE_ADDR   0xf301b000 /* XXX not in ofw ? */
#define PLATINUM_CMAP_ADDR_OFFSET 0x00000000
#define PLATINUM_CMAP_D1_OFFSET   0x00000010
#define PLATINUM_CMAP_D2_OFFSET   0x00000020
#define PLATINUM_CMAP_LUT_OFFSET  0x00000030
#define PLATINUM_CMAP_SIZE        0x1000     /* XXX ofw says 0x800? */

/* framebuffer */
#define PLATINUM_FB_BANK_SIZE 0x100000
#define PLATINUM_FB_MIN_SIZE (1 * PLATINUM_FB_BANK_SIZE)
#define PLATINUM_FB_MAX_SIZE (4 * PLATINUM_FB_BANK_SIZE)

/* depth/colormodes */
#define PLATINUM_CMODE_8  0
#define PLATINUM_CMODE_16 1
#define PLATINUM_CMODE_32 2

/* DACula types */
#define PLATINUM_DAC_0 0x84
#define PLATINUM_DAC_1 0x3c

#endif /* PLATINUMFBREG_H */
