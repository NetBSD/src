/* $NetBSD: haltworeg.h,v 1.1 2003/09/25 16:35:50 lonewolf Exp $ */

/*
 * Copyright (c) 2003 Ilpo Ruotsalainen
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * 
 * <<Id: LICENSE_GC,v 1.1 2001/10/01 23:24:05 cgd Exp>>
 */

#ifndef _ARCH_SGIMIPS_HPC_HALTWOREG_H_
#define _ARCH_SGIMIPS_HPC_HALTWOREG_H_

/* HAL2 direct-addressable registers, PBUS PIO channel 0 */
#define HAL2_REG_CTL_ISR		0x10

#define HAL2_REG_CTL_REV		0x20
#define  HAL2_REV_AUDIO_PRESENT_N	0x8000

#define HAL2_REG_CTL_IAR		0x30
#define  HAL2_IAR_READ			0x0080

#define HAL2_REG_CTL_IDR0		0x40
#define HAL2_REG_CTL_IDR1		0x50
#define HAL2_REG_CTL_IDR2		0x60
#define HAL2_REG_CTL_IDR3		0x70

#define HAL2_ISR_TSTATUS		0x01
#define  HAL2_ISR_GLOBAL_RESET_N	0x08
#define  HAL2_ISR_CODEC_RESET_N		0x10

/* HAL2 direct-addressable registers, PBUS PIO channel 2 */
#define HAL2_REG_VOL_RIGHT		0x00
#define HAL2_REG_VOL_LEFT		0x04

/* HAL2 indirect-addressable registers */
#define HAL2_IREG_RELAY_C		0x9100
#define  HAL2_RELAY_C_STATE		0x01

#define HAL2_IREG_DMA_PORT_EN		0x9104
#define  HAL2_DMA_PORT_EN_CODECTX	0x08

#define HAL2_IREG_DMA_END		0x9108
#define  HAL2_DMA_END_CODECTX		0x08

#define HAL2_IREG_DMA_DRV		0x910C

#define HAL2_IREG_BRES1_C1		0x2104
#define HAL2_IREG_BRES1_C2		0x2108

#define HAL2_IREG_DAC_C1		0x1404
#define  HAL2_C1_DMA_SHIFT		0
#define  HAL2_C1_CLKID_SHIFT		3
#define  HAL2_C1_DATAT_SHIFT		8

#define HAL2_IREG_DAC_C2		0x1408

#endif
