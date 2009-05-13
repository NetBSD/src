/*	$NetBSD: oiocreg.h,v 1.1.6.2 2009/05/13 17:18:19 jym Exp $	*/

/*
 * Copyright (c) 2009 Stephen M. Rumble
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
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
 */

#ifndef _ARCH_SGIMIPS_DEV_OIOCREG_H_
#define	_ARCH_SGIMIPS_DEV_OIOCREG_H_

/* Registers below are relative to: */
#define OIOC_BASE_ADDRESS	0x1f900000

/*
 * The IOC SCSI DMA engine consists of 257 16-bit address registers, which can
 * point to 256 4096-byte buffers.
 *
 * IOC_SCSI_DMA_LOW contains the first 12 bits of a starting offset within
 * the first page of a virtually contiguous buffer. The MSB indicates DMA
 * transfer direction. 
 *
 * There are 256 high address registers starting at IOC_SCSI_DMA_HIGH_BASE
 * and occurring every 4 bytes. This provides 28 address bits - more than
 * sufficient for these old machines.
 */
#define OIOC_SCSI_REGS		0x00000000	/* SCSI registers offset */
#define OIOC_SCSI_REGS_SIZE	0x00200102	/* SCSI length */

#define OIOC_SCSI_DMA_LOW	0x00000002	/* 16-bit */
#define OIOC_SCSI_DMA_HIGH_BASE	0x00020002	/* 16-bit */
#define OIOC_SCSI_DMA_FLUSH	0x00040000	/* 32-bit; write 0 to flush */

#define OIOC_SCSI_DMA_NSEGS		256
#define OIOC_SCSI_DMA_HIGH(_x)		(OIOC_SCSI_DMA_HIGH_BASE + ((_x) << 2))
#define OIOC_SCSI_DMA_HIGH_SHFT		12
#define OIOC_SCSI_DMA_LOW_ADDR_MASK	0x0fff
#define OIOC_SCSI_DMA_LOW_ADDR_DMADIR	0x8000	/* if set: SCSI->MEM */

#define OIOC_SCSI_RESET_OFF	0x00180000	/* 32-bit; read to set _RESET */
#define OIOC_SCSI_RESET_ON	0x00180004	/* 32-bit; read to clr _RESET */

#define OIOC_WD33C93_ASR	0x00200001	/*  8-bit; scsi asr register */
#define OIOC_WD33C93_ASR_SIZE	1
#define OIOC_WD33C93_DATA	0x00200101	/*  8-bit; scsi data register */
#define OIOC_WD33C93_DATA_SIZE	1

/*
 * IOC has 64 (I think) 16-bit page remap registers occurring every 4 bytes
 * starting at 0xbf920802. Each register takes a physical page number, N, which
 * maps physical memory page N into LANCE's 24-bit address space at offset
 * (N * 4096).
 */
#define OIOC_ENET_PGMAP_BASE	0x00020802		/* 16-bit */
#define OIOC_ENET_NPGMAPS	64			/* 64 pages */
#define OIOC_ENET_PGMAP_SIZE	(OIOC_ENET_NPGMAPS * 4)
#define OIOC_ENET_PGMAP_OFF(n)	((n) << 2)		/* every 4 bytes */

#define OIOC_ENET_RESET_OFF	0x00060000	/* 8-bit; read to set _RESET */
#define OIOC_ENET_RESET_ON	0x00060004	/* 8-bit; read to clr _RESET */
#define OIOC_LANCE_RDP		0x00050000	/* 16-bit; le reg. data port */
#define OIOC_LANCE_RDP_SIZE	2
#define OIOC_LANCE_RAP		0x00050100	/* 16-bit; le reg. access port*/
#define OIOC_LANCE_RAP_SIZE	2

#define OIOC2_CONFIG		0x00180008	/* 32-bit; IOC2 (IP6/10) only */

/* OIOC2_CONFIG bits; only BURST, NOSYNC and HIWAT are writable. */
#define OIOC2_CONFIG_HIWAT_MASK		0x0000000f
#define OIOC2_CONFIG_HIWAT_SHFT		0x00000000
#define OIOC2_CONFIG_ID_MASK		0x00000030
#define OIOC2_CONFIG_ID_SHFT		0x00000004
#define OIOC2_CONFIG_NOSYNC_MASK	0x00000040
#define OIOC2_CONFIG_NOSYNC_SHFT	0x00000006
#define OIOC2_CONFIG_BURST_MASK		0x00000080
#define OIOC2_CONFIG_BURST_SHFT		0x00000007
#define OIOC2_CONFIG_COUNT_MASK		0x00007f00
#define OIOC2_CONFIG_COUNT_SHFT		0x00000008
#define OIOC2_CONFIG_RSRVD_MASK		0x00008000
#define OIOC2_CONFIG_RSRVD_SHFT		0x0000000f
#define OIOC2_CONFIG_SCP_MASK		0x003f0000
#define OIOC2_CONFIG_SCP_SHFT		0x00000010
#define OIOC2_CONFIG_RSRVD2_MASK 	0x0fc00000	
#define OIOC2_CONFIG_RSRVD2_SHFT 	0x00000016
#define OIOC2_CONFIG_IOP_MASK		0xf0000000
#define OIOC2_CONFIG_IOP_SHFT		0x0000001c

#endif /* _ARCH_SGIMIPS_DEV_OIOCREG_H_ */
