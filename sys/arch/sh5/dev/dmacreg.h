/*	$NetBSD: dmacreg.h,v 1.1.2.2 2005/02/04 11:44:56 skrll Exp $	*/

/*-
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _SH5_DMACREG_H
#define _SH5_DMACREG_H

#define	DMAC_MODULE_ID		0x0183

/*
 * Offsets to the register areas of dma controller.
 */
#define	DMAC_COMMON		0x08
#define	DMAC_SAR(c)		(0x10 + (0x28 * (c)))
#define	DMAC_DAR(c)		(0x18 + (0x28 * (c)))
#define	DMAC_COUNT(c)		(0x20 + (0x28 * (c)))
#define	DMAC_CTRL(c)		(0x28 + (0x28 * (c)))
#define	DMAC_STATUS(c)		(0x30 + (0x28 * (c)))
#define	DMAC_DMAEXG		0xc0
#define	DMAC_REG_SZ		0xc8

#define	DMAC_N_CHANNELS		4

/*
 * Bit definitions for DMAC_COMMON
 */
#define	DMAC_COMMON_PRIORITY_ROUND_ROBIN	(1u << 0)
#define	DMAC_COMMON_MASTER_ENABLE		(1u << 3)
#define	DMAC_COMMON_NMI_FLAG			(1u << 4)
#define	DMAC_COMMON_ERROR_RESPONSE(c)		(1u << (7 + (c)))
#define	DMAC_COMMON_ADDRESS_ALIGNMENT_ERROR(c)	(1u << (11 + (c)))

/*
 * Masks for DMAC_SAR/DMAC_DAR/DMAC_COUNT
 */
#define	DMAC_SAR_MASK(a)			((a) & 0x00000000ffffffffull)
#define	DMAC_DAR_MASK(a)			((a) & 0x00000000ffffffffull)
#define	DMAC_COUNT_MASK(a)			((a) & 0x00000000ffffffffull)

/*
 * Bit definitions for DMAC_CTRL
 */
#define	DMAC_CTRL_TRANSFER_SIZE_MASK		0x0018
#define	DMAC_CTRL_TRANSFER_SIZE_1		0x0
#define	DMAC_CTRL_TRANSFER_SIZE_2		0x1
#define	DMAC_CTRL_TRANSFER_SIZE_4		0x2
#define	DMAC_CTRL_TRANSFER_SIZE_8		0x3
#define	DMAC_CTRL_TRANSFER_SIZE_16		0x4
#define	DMAC_CTRL_TRANSFER_SIZE_32		0x5
#define	DMAC_CTRL_SOURCE_INCREMENT_MASK		0x0018
#define	DMAC_CTRL_SOURCE_INCREMENT_INC		0x0000
#define	DMAC_CTRL_SOURCE_INCREMENT_DEC		0x0008
#define	DMAC_CTRL_SOURCE_INCREMENT_HOLD		0x0010
#define	DMAC_CTRL_DESTINATION_INCREMENT_MASK	0x0060
#define	DMAC_CTRL_DESTINATION_INCREMENT_INC	0x0000
#define	DMAC_CTRL_DESTINATION_INCREMENT_DEC	0x0020
#define	DMAC_CTRL_DESTINATION_INCREMENT_HOLD	0x0040
#define	DMAC_CTRL_RESOURCE_SELECT_MASK		0x0780
#define	DMAC_CTRL_RESOURCE_SELECT_AUTO		0x0000
#define	DMAC_CTRL_RESOURCE_SELECT_PERIPH_0	0x0080
#define	DMAC_CTRL_RESOURCE_SELECT_PERIPH_1	0x0100
#define	DMAC_CTRL_RESOURCE_SELECT_PERIPH_2	0x0180
#define	DMAC_CTRL_RESOURCE_SELECT_PERIPH_3	0x0200
#define	DMAC_CTRL_INTERRUPT_ENABLE		(1u << 11)
#define	DMAC_CTRL_TRANSFER_ENABLE		(1u << 12)

/*
 * Bit definitions for DMAC_STATUS
 */
#define	DMAC_STATUS_TRANSFER_END		(1u << 0)
#define	DMAC_STATUS_ADDRESS_ALIGN_ERROR		(1u << 1)

/*
 * Bit definitions for DMAC_DMAEXG
 */
#define	DMAC_DMAEXG_DACK_READWRITE(c)			(1u << (c))
#define	DMAC_DMAEXG_DREQ_ATTRIBUTE(c)			(1u << ((c) + 4)
#define	DMAC_DMAEXG_DRACK_ATTRIBUTE(c)			(1u << ((c) + 8)
#define	DMAC_DMAEXG_DACK_ATTRIBUTE(c)			(1u << ((c) + 12)
#define	DMAC_DMAEXG_REQUEST_MODE(c)			(1u << ((c) + 16)
#define	DMAC_DMAEXG_REQUEST_QUEUE_DEPTH(c)		(1u << ((c) + 20)
#define	DMAC_DMAEXG_REQUEST_QUEUE_CLEAR_ENABLE(c)	(1u << ((c) + 24)

#endif /* _SH5_DMACREG_H */
