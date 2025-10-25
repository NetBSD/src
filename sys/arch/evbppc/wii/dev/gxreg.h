/* $NetBSD: gxreg.h,v 1.1 2025/10/25 15:02:56 jmcneill Exp $ */

/*-
 * Copyright (c) 2025 Jared McNeill <jmcneill@invisible.ca>
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

#ifndef _WII_DEV_GXREG_H        
#define _WII_DEV_GXREG_H

#define LOWER_16_BITS(x)	((x) & 0xffff)
#define UPPER_16_BITS(x)	(((x) >> 16) & 0xffff)

#define _REG16(x)	((x) / 2)
#define _REG32(x)	((x) / 4)

#define CP_SR			_REG16(0x00)
#define  CP_SR_IDLE_CMDS	0x0008
#define  CP_SR_IDLE_READ	0x0004
#define CP_CR			_REG16(0x02)
#define  CP_CR_READ_ENABLE	0x0001
#define  CP_CR_GP_LINK_ENABLE	0x0010
#define CP_CLEAR		_REG16(0x04)
#define  CP_CLEAR_OVERFLOW	0x0001
#define  CP_CLEAR_UNDERFLOW	0x0002
#define CP_FIFO_BASE_LO		_REG16(0x20)
#define CP_FIFO_BASE_HI		_REG16(0x22)
#define CP_FIFO_END_LO		_REG16(0x24)
#define CP_FIFO_END_HI		_REG16(0x26)
#define CP_FIFO_HIWAT_LO	_REG16(0x28)
#define CP_FIFO_HIWAT_HI	_REG16(0x2a)
#define CP_FIFO_LOWAT_LO	_REG16(0x2c)
#define CP_FIFO_LOWAT_HI	_REG16(0x2e)
#define CP_FIFO_RW_DIST_LO	_REG16(0x30)
#define CP_FIFO_RW_DIST_HI	_REG16(0x32)
#define CP_FIFO_WRITE_PTR_LO	_REG16(0x34)
#define CP_FIFO_WRITE_PTR_HI	_REG16(0x36)
#define CP_FIFO_READ_PTR_LO	_REG16(0x38)
#define CP_FIFO_READ_PTR_HI	_REG16(0x3a)

#define PE_ZCONF		_REG16(0x00)
#define  PE_ZCONF_UPD_ENABLE	0x0010
#define  PE_ZCONF_FUNC_ALWAYS	0x000e
#define  PE_ZCONF_COMP_ENABLE	0x0001
#define PE_ALPHA_CONF		_REG16(0x02)
#define  PE_ALPHA_CONF_OP_SET	0xf000
#define  PE_ALPHA_CONF_SRC_1	0x0100
#define  PE_ALPHA_CONF_DST_0	0x0000
#define  PE_ALPHA_CONF_UPD_A	0x0010
#define  PE_ALPHA_CONF_UPD_C	0x0008
#define PE_ALPHA_DEST		_REG16(0x04)
#define  PE_ALPHA_DEST_ENABLE	0x0100
#define PE_ALPHA_MODE		_REG16(0x06)
#define  PE_ALPHA_MODE_ALWAYS	0x0700
#define PE_ALPHA_READ		_REG16(0x08)
#define  PE_ALPHA_READ_UNK	0x0004
#define  PE_ALPHA_READ_FF	0x0001
#define PE_ISR			_REG16(0x0a)
#define  PE_ISR_FINISH		0x0008
#define  PE_ISR_TOKEN		0x0004
#define  PE_ISR_FINISH_ENABLE	0x0002
#define  PE_IST_TOKEN_ENABLE	0x0001
#define PE_TOKEN		_REG16(0x0e)

#define PI_FIFO_BASE_START	_REG32(0x0c)
#define PI_FIFO_BASE_END	_REG32(0x10)
#define PI_FIFO_WRITE_PTR	_REG32(0x14)

#define GX_XF_VIEWPORT_X0	0x101a
#define GX_XF_VIEWPORT_Y0	0x101b
#define GX_XF_VIEWPORT_Z	0x101c
#define GX_XF_VIEWPORT_X1	0x101d
#define GX_XF_VIEWPORT_Y1	0x101e
#define GX_XF_VIEWPORT_FP	0x101f
#define GX_XF_PROJ_0		0x1020
#define GX_XF_PROJ_1		0x1021
#define GX_XF_PROJ_2		0x1022
#define GX_XF_PROJ_3		0x1023
#define GX_XF_PROJ_4		0x1024
#define GX_XF_PROJ_5		0x1025
#define GX_XF_PROJ_MODE		0x1026

#define GX_FIFO_ALIGN		32
#define GX_FIFO_HIWAT(_size)	((_size) - (16 * 1024))
#define GX_FIFO_LOWAT(_size)	(((_size) >> 1) & ~0x1f)
#define GX_XY(x, y)		(((uint32_t)(y) << 10) | (x))

#define GX_STRICT_ORDER(x)			\
	do {					\
		x;				\
		asm volatile("" ::: "memory");	\
	} while (0)

typedef union gx_wgpipe {
	volatile int8_t		s8;
	volatile uint8_t	u8;
	volatile int16_t	s16;
	volatile uint16_t	u16;
	volatile int16_t	s32;
	volatile uint32_t	u32;
} gx_wgpipe_t;

#endif /* _WII_DEV_GXREG_H */
