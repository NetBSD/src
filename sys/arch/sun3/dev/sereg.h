/*	$NetBSD: sereg.h,v 1.5.108.1 2008/05/16 02:23:17 yamt Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross.
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
 * Sun3/E SCSI/Ethernet board.  This is a VME board with some memory,
 * an Intel Ether, and an NCR5380 SCSI with a cheap DMA engine.
 */

/*****************************************************************
 * Register definitions for the SCSI portion.
 * (size=0x20)
 */
struct se_regs {
	u_char			ncrregs[8];
	u_short			unused1;
	u_short			dma_addr;	/* DMA offset register	*/
	u_short			unused2;
	u_short			dma_cntr;	/* DMA count down register */
	u_short			unused3[5];
	u_short			se_csr;		/* control/status register */
	u_short			unused4;
	u_short			se_ivec;	/* interrupt vector	*/
};

/*
 * SCSI Control and Status Register.
 * Note:
 *	(ro) 	indicates bit is read only.
 *	(rw)	indicates bit is read or write.
 */
#define SE_CSR_SBC_IP		0x0200	/* (ro) sbc interrupt pending */
#define SE_CSR_SEND 		0x0008	/* (rw) DMA dir, 1=to device */
#define SE_CSR_INTR_EN		0x0004	/* (rw) interrupts enable */
#define	SE_CSR_VCC  		0x0002	/* (ro) power signal to the chip */
#define SE_CSR_SCSI_RES		0x0001	/* (rw) reset sbc and udc, 0=reset */


/*****************************************************************
 * Register definitions for the Ethernet portion.
 * (size=0x100)
 */
struct ie_regs {
	u_short			ie_pad0;
	u_short			ie_csr;
	u_short			ie_pad1[7];
	u_short			ie_ivec;	/* interrupt vector */
	u_short			ie_pad3[128-10];
};

/*
 * Ether Control and Status Register.
 */
#define IE_CSR_RESET		0x8000	/* board reset */
#define IE_CSR_NOLOOP		0x4000	/* loopback disable */
#define IE_CSR_ATTEN		0x2000	/* channel attention */
#define IE_CSR_IENAB		0x1000	/* interrupt enable */
#define IE_CSR_IPEND		0x0100	/* interrupt pending */


/*****************************************************************
 * Register definitions for the entire SCSI/Ethernet board.
 * I had the impression that there were overlaps in this map,
 * which was the reason for existence of the "sebuf" driver.
 * Now it looks like the "sebuf" driver was unnecessary. XXX
 */

#define SE_NCRBUFSIZE	0x10000
#define SE_IEBUFSIZE	0x20000
struct sebuf_regs {
	char	se_scsi_buf[SE_NCRBUFSIZE];
	struct se_regs se_scsi_regs;
	char	se_pad[0x10000 - 0x120];
	struct ie_regs se_eth_regs;
	char	se_eth_buf[SE_IEBUFSIZE];
};	/* 128KB total */
