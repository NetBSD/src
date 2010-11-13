/*	$NetBSD: imxssireg.h,v 1.1 2010/11/13 07:11:03 bsh Exp $	*/
/*
 * Copyright (c) 2009  Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARM_IMX_IMXSSIREG_H
#define	_ARM_IMX_IMXSSIREG_H

#define	SSI_STX0	0x0000
#define	SSI_STX1	0x0004
#define	SSI_SRX0	0x0008
#define	SSI_SRX1	0x000C
#define	SSI_SCR		0x0010
#define	 SCR_CLK_IST		__BIT(9)
#define	 SCR_TCH_EN		__BIT(8)
#define	 SCR_SYS_CLK_EN		__BIT(7)
#define	 SCR_I2SMODE_MASK	((0x3)<<5)
#define	 SCR_I2SMODE(n)		((n)<<5)
#define	 I2SMODE_NORMAL		(0)
#define	 I2SMODE_MASTER		(1)
#define	 I2SMODE_SLAVE		(2)
#define	 SCR_SYN		__BIT(4)
#define	 SCR_NET		__BIT(3)
#define	 SCR_RE			__BIT(2)
#define	 SCR_TE			__BIT(1)
#define	 SCR_SSIEN		__BIT(0)
#define	SSI_SISR	0x0014
#define	SSI_SIER	0x0018
#define	 SI_RDMAE		__BIT(22)
#define	 SI_RIE			__BIT(21)
#define	 SI_TDMAE		__BIT(20)
#define	 SI_TIE			__BIT(19)
#define	 SI_CMDAU_EN		__BIT(18)
#define	 SI_CMDDU_EN		__BIT(17)
#define	 SI_RXT_EN		__BIT(16)
#define	 SI_RDR1_EN		__BIT(15)
#define	 SI_RDR0_EN		__BIT(14)
#define	 SI_TDE1_EN		__BIT(13)
#define	 SI_TDE0_EN		__BIT(12)
#define	 SI_ROE1_EN		__BIT(11)
#define	 SI_ROE0_EN		__BIT(10)
#define	 SI_TUE1_EN		__BIT(9)
#define	 SI_TUE0_EN		__BIT(8)
#define	 SI_RFS_EN		__BIT(7)
#define	 SI_TFS_EN		__BIT(6)
#define	 SI_RLS_EN		__BIT(5)
#define	 SI_TLS_EN		__BIT(4)
#define	 SI_RFF1_EN		__BIT(3)
#define	 SI_RFF0_EN		__BIT(2)
#define	 SI_TFE1_EN		__BIT(1)
#define	 SI_TFE0_EN		__BIT(0)
#define	SSI_STCR	0x001C
#define	SSI_SRCR	0x0020
#define	 CR_XEX			__BIT(10)
#define	 CR_XBIT		__BIT(9)
#define	 CR_FEN1		__BIT(8)
#define	 CR_FEN0		__BIT(7)
#define	 CR_FDIR		__BIT(6)
#define	 CR_XDIR		__BIT(5)
#define	 CR_SHFD		__BIT(4)
#define	 CR_SHFD_MSB	        (0<<4)
#define	 CR_SHFD_LSB		CR_SHFD
#define	 CR_SCKP		__BIT(3)
#define	 CR_FSI			__BIT(2)
#define	 CR_FSL			__BIT(1)
#define	 CR_EFS			__BIT(0)
#define	SSI_STCCR	0x0024
#define	SSI_SRCCR	0x0028
#define	 WL_SHIFT		13
#define	 WL_MASK		(0xf << 13)
#define	 DC_SHIFT		8
#define	 DC_MASK		(0xf << 8)
#define	SSI_SFCSR	0x002C
#define	 SFCSR_RFCNT1_MASK	(0xf << 28)
#define	 SFCSR_TFCNT1_MASK	(0xf << 24)
#define	 SFCSR_RFWM1_MASK	(0xf << 20)
#define	 SFCSR_TFWM1_MASK	(0xf << 16)
#define	 SFCSR_RFCNT0_MASK	(0xf << 12)
#define	 SFCSR_TFCNT0_MASK	(0xf << 8)
#define	 SFCSR_RFWM0_MASK	(0xf << 4)
#define	 SFCSR_TFWM0_MASK	(0xf << 0)
#define	 SFCSR_RFWM1(x)		(((x) & 0xf) << 20)
#define	 SFCSR_TFWM1(x)		(((x) & 0xf) << 16)
#define	 SFCSR_RFWM0(x)		(((x) & 0xf) << 4)
#define	 SFCSR_TFWM0(x)		(((x) & 0xf) << 0)
#define	SSI_SACNT	0x0038
#define	SSI_SACSDD	0x003C
#define	SSI_SACDAT	0x0040
#define	SSI_SATAG	0x0044
#define	SSI_STMSK	0x0048
#define	SSI_SRMSK	0x004C


#define	SSI_SIZE	0x100


#endif	/* _ARM_IMX_IMXSSIREG_H */
