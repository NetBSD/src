/*	$NetBSD: ti_iicreg.h,v 1.3 2019/10/31 10:21:29 jmcneill Exp $	*/

/*
 * Copyright (c) 2013 Manuel Bouyer.  All rights reserved.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _TI_IICREG_H
#define _TI_IICREG_H

#define		I2C_SYSC_CLKACTIVITY_OCP	0x0010
#define		I2C_SYSC_CLKACTIVITY_SYSTEM	0x0020
#define		I2C_SYSC_IDLE_MASK		0x0018
#define		I2C_SYSC_IDLE_FORCE		0x0000
#define		I2C_SYSC_IDLE_SMART		0x0010
#define		I2C_SYSC_IDLE_NONE		0x0008
#define		I2C_SYSC_ENAWAKEUP		0x0004
#define		I2C_SYSC_SRST			0x0002
#define		I2C_SYSC_AUTOIDLE		0x0001

#define		I2C_IRQSTATUS_XDR		0x4000
#define		I2C_IRQSTATUS_RDR		0x2000
#define		I2C_IRQSTATUS_BB		0x1000
#define		I2C_IRQSTATUS_ROVR		0x0800
#define		I2C_IRQSTATUS_XUDF		0x0400
#define		I2C_IRQSTATUS_AAS		0x0200
#define		I2C_IRQSTATUS_BF		0x0100
#define		I2C_IRQSTATUS_AERR		0x0080
#define		I2C_IRQSTATUS_STC		0x0040
#define		I2C_IRQSTATUS_GC		0x0020
#define		I2C_IRQSTATUS_XRDY		0x0010
#define		I2C_IRQSTATUS_RRDY		0x0008
#define		I2C_IRQSTATUS_ARDY		0x0004
#define		I2C_IRQSTATUS_NACK		0x0002
#define		I2C_IRQSTATUS_AL		0x0001

#define		I2C_DMARXENABLE			0x0001

#define		I2C_DMATXENABLE			0x0001

#define		I2C_SYSS_RDONE			0x0001

#define		I2C_BUF_RDMA_EN			0x8000
#define		I2C_BUF_RXFIFO_CLR		0x4000
#define		I2C_BUF_RXTRSH_MASK		0x3f00
#define		I2C_BUF_RXTRSH(x)		((x) << 8)
#define		I2C_BUF_XDMA_EN			0x0080
#define		I2C_BUF_TXFIFO_CLR		0x0040
#define		I2C_BUF_TXTRSH_MASK		0x003f
#define		I2C_BUF_TXTRSH(x)		((x) << 0)

#define		I2C_CNT_MASK			0xffff

#define		I2C_DATA_MASK			0x00ff

#define		I2C_CON_EN			0x8000
#define		I2C_CON_STB			0x0800
#define		I2C_CON_MST			0x0400
#define		I2C_CON_TRX			0x0200
#define		I2C_CON_XSA			0x0100
#define		I2C_CON_XOA0			0x0080
#define		I2C_CON_XOA1			0x0040
#define		I2C_CON_XOA2			0x0020
#define		I2C_CON_XOA3			0x0010
#define		I2C_CON_STP			0x0002
#define		I2C_CON_STT			0x0001

#define		I2C_OA_MASK			0x03ff

#define		I2C_SA_MASK			0x03ff

#define		I2C_PSC_MASK			0x000f

#define		I2C_SCLL_MASK			0x000f

#define		I2C_SCLH_MASK			0x000f

#define		I2C_BUFSTAT_FIFODEPTH(x)	(((x) >> 14) & 0x03)
#define		I2C_BUFSTAT_RXSTAT(x)		(((x) >>  8) & 0x3f)
#define		I2C_BUFSTAT_TXSTAT(x)		(((x) >>  0) & 0x3f)

#define		I2C_ACTOA_OA3_ACT		0x0008
#define		I2C_ACTOA_OA2_ACT		0x0004
#define		I2C_ACTOA_OA1_ACT		0x0002
#define		I2C_ACTOA_OA0_ACT		0x0001

#endif /* _TI_IICREG_H */
