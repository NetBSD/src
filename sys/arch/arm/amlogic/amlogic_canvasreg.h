/* $NetBSD: amlogic_canvasreg.h,v 1.1.20.2 2017/12/03 11:35:51 jdolecek Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _ARM_AMLOGIC_CANVASREG_H
#define _ARM_AMLOGIC_CANVASREG_H

#define CANVAS_REG(n)	((n) << 2)

#define DC_CAV_LUT_DATAL_REG	CANVAS_REG(0x12)
#define DC_CAV_LUT_DATAH_REG	CANVAS_REG(0x13)
#define DC_CAV_LUT_ADDR_REG	CANVAS_REG(0x14)

#define DC_CAV_LUT_DATAL_FBADDR		__BITS(28,0)
#define DC_CAV_LUT_DATAL_WIDTH_L	__BITS(31,29)

#define DC_CAV_LUT_DATAH_BLKMODE	__BITS(25,24)
#define DC_CAV_LUT_DATAH_BLKMODE_LINEAR	0
#define DC_CAV_LUT_DATAH_BLKMODE_32X32	1
#define DC_CAV_LUT_DATAH_BLKMODE_64X64	2
#define DC_CAV_LUT_DATAH_YWRAP		__BIT(23)
#define DC_CAV_LUT_DATAH_XWRAP		__BIT(22)
#define DC_CAV_LUT_DATAH_HEIGHT		__BITS(21,9)
#define DC_CAV_LUT_DATAH_WIDTH_H	__BITS(8,0)

#define DC_CAV_LUT_ADDR_WR_EN		__BIT(9)
#define DC_CAV_LUT_ADDR_RD_EN		__BIT(8)
#define DC_CAV_LUT_ADDR_INDEX		__BITS(2,0)

#endif /* _ARM_AMLOGIC_CANVASREG_H */
