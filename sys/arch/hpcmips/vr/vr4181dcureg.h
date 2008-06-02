/* $NetBSD: vr4181dcureg.h,v 1.1.104.1 2008/06/02 13:22:11 mjf Exp $ */

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Naoto Shimazaki of YOKOGAWA Electric Corporation.
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

/*
 *	VR4181 DCU (DMA Control Unit) Registers definitions.
 *		dcu1 at 0x0a000020-0x0a000047
 *		dcu2 at 0x0a000650-0x0a000667
 *
 */

/* dcu1 registers */
#define DCU_MICDEST1REG1_W	0x00	/* microphone destination 1 low */
#define DCU_MICDEST1REG2_W	0x02	/* microphone destination 1 high */
#define DCU_MICDEST2REG1_W	0x04	/* microphone destination 2 low */
#define DCU_MICDEST2REG2_W	0x06	/* microphone destination 2 high */
#define DCU_SPKRSRC1REG1_W	0x08	/* speaker destination 1 low */
#define DCU_SPKRSRC1REG2_W	0x0a	/* speaker destination 1 high */
#define DCU_SPKRSRC2REG1_W	0x0c	/* speaker destination 2 low */
#define DCU_SPKRSRC2REG2_W	0x0e	/* speaker destination 2 high */
#define DCU_DMARST_REG_W	0x20	/* DMA reset */
#define  DCU_DMARST		0x0001	/* DMA reset */
#define DCU_AIUDMAMSK_REG_W	0x26	/* audio DMA mask */
#define  DCU_ENABLE_MIC		0x0008	/* enable microphone */
#define  DCU_ENABLE_SPK		0x0004	/* enable speaker */

/* dcu2 registers */
#define DCU_MICRCLEN_REG_W	0x08	/* microphone record length */
#define DCU_SPKRCLEN_REG_W	0x0a	/* speaker record length */
#define DCU_MICDMACFG_REG_W	0x0e	/* microphone DMA configuration */
#define  DCU_MICLOAD		0x0100
#define DCU_SPKDMACFG_REG_W	0x10	/* speaker DMA configuration */
#define  DCU_SPKLOAD		0x0001
#define DCU_DMAITRQ_REG_W	0x12	/* DMA interrupt request */
#define  DCU_SPKEOP		0x20
#define  DCU_MICEOP		0x10
#define DCU_DMACTL_REG_W	0x14	/* DMA control */
#define  DCU_SPKCNT_MSK		0xc000
#define  DCU_SPKCNT_INC		0x0000
#define  DCU_SPKCNT_DEC		0x4000
#define  DCU_MICCNT_MSK		0x3000
#define  DCU_MICCNT_INC		0x0000
#define  DCU_MICCNT_DEC		0x1000
#define DCU_DMAITMK_REG_W	0x16	/* DMA interrupt mask */
#define  DCU_SPKEOP_ENABLE	0x0020
#define  DCU_MICEOP_ENABLE	0x0010
