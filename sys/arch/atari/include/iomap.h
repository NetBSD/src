/*	$NetBSD: iomap.h,v 1.7 1999/12/06 16:06:41 leo Exp $	*/

/*
 * Copyright (c) 1995 Leo Weppelman.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Leo Weppelman.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#ifndef _MACHINE_IOMAP_H
#define _MACHINE_IOMAP_H
/*
 * Atari TT hardware:
 * I/O Address maps
 */
#ifdef _KERNEL
vaddr_t	stio_addr;		/* Where the st io-area is mapped	*/
#define	AD_STIO	(stio_addr)	/* .. see atari_init.c			*/

/*
 * PCI KVA addresses. These are determined in atari_init.c. Except for
 * the config-space, they should be used for a PCI-console only. Other
 * cards should use the bus-functions to map io & mem spaces.
 * Each card gets an config area of NBPG  bytes.
 */
vaddr_t	pci_conf_addr;		/* KVA base of PCI config space		*/
vaddr_t	pci_io_addr;		/* KVA base of PCI io-space		*/
vaddr_t	pci_mem_addr;		/* KVA base of PCI mem-space		*/
#endif /* _KERNEL */

#define	PCI_CONFB_PHYS	(0xA0000000L)
#define	PCI_CONFM_PHYS	(0x00010000L)
#define	PCI_IO_PHYS	(0xB0000000L)
#define	PCI_MEM_PHYS	(0x80000000L)
#define PCI_VGA_PHYS	(0x800a0000L)

#define PCI_CONF_SIZE	(4 * NBPG)
#define PCI_IO_SIZE	(NBPG)
#define PCI_VGA_SIZE	(32 * 1024)

#define	BOOTM_VA_POOL	(32 * 8192)	/* Pre-allocated VA-space	*/

#define	AD_RAM		(0x000000L)	/* main memory			*/
#define	AD_CART		(0xFA0000L)	/* expansion cartridge		*/
#define	AD_ROM		(0xFC0000L)	/* system ROM			*/
#define	STIO_SIZE	(0x8000L)	/* Size of mapped I/O devices	*/

/*
 * Physical address of I/O area. Use only for pte initialisation!
 */
#define	STIO_PHYS	((machineid & ATARI_HADES)	\
				? 0xffff8000L		\
				: 0x00ff8000L)

/*
 * I/O addresses in the STIO area:
 */
#define	AD_RAMCFG	(AD_STIO + 0x0000)	/* ram configuration	*/
#define AD_FAL_MON_TYPE	(AD_STIO + 0x0006)	/* Falcon monitor type	*/
#define	AD_VIDEO	(AD_STIO + 0x0200)	/* video controller	*/
#define AD_RESERVED	(AD_STIO + 0x0400)	/* reserved		*/
#define	AD_DMA		(AD_STIO + 0x0600)	/* DMA device access	*/
#define	AD_SCSI_DMA	(AD_STIO + 0x0700)	/* SCSI DMA registers	*/
#define	AD_NCR5380	(AD_STIO + 0x0780)	/* SCSI controller	*/
#define	AD_SOUND	(AD_STIO + 0x0800)	/* YM-2149		*/
#define	AD_RTC		(AD_STIO + 0x0960)	/* TT realtime clock	*/
#define	AD_SCC		(AD_STIO + 0x0C80)	/* SCC 8530		*/
#define	AD_SCU		(AD_STIO + 0x0E00)	/* System Control Unit	*/

#define	AD_MFP		(AD_STIO + 0x7A00)	/* 68901		*/
#define	AD_MFP2		(AD_STIO + 0x7A80)	/* 68901-TT		*/
#define	AD_ACIA		(AD_STIO + 0x7C00)	/* 2 * 6850		*/
#endif /* _MACHINE_IOMAP_H */
