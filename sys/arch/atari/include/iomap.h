/*	$NetBSD: iomap.h,v 1.10 2001/05/28 06:43:20 leo Exp $	*/

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

#include "opt_mbtype.h"
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
 * 'pci_mem_uncached' is used by the Milan interrupt handler that frobs
 * with the PLX. Also, the Milan uses the first page of 'pci_io_addr' for
 * access to some of it's ISA I/O devices (RTC, Interrupt controller, etc.)
 */
vaddr_t	pci_conf_addr;		/* KVA base of PCI config space		*/
vaddr_t	pci_io_addr;		/* KVA base of PCI io-space		*/
vaddr_t	pci_mem_addr;		/* KVA base of PCI mem-space		*/
vaddr_t	pci_mem_uncached;	/* KVA base of an uncached PCI mem-page	*/
#endif /* _KERNEL */

#define	PCI_CONFB_PHYS		(0xA0000000L)
#define	PCI_CONFM_PHYS		(0x00010000L)

#if defined(_ATARIHW_)
#define	PCI_IO_PHYS		(0xB0000000L)
#define	PCI_MEM_PHYS		(0x80000000L)
#define PCI_VGA_PHYS		(0x800a0000L)
#define	ISA_IOSTART		(0xfff30000L) /* XXX: With byte frobbing */
#define	ISA_MEMSTART		(0xff000000L)
#endif /* defined(_ATARIHW_) */

#if defined(_MILANHW_)
#define	PCI_IO_PHYS		(0x80000000L)
#define	PCI_MEM_PHYS		(0x40000000L)
#define PCI_VGA_PHYS		(0x400a0000L)
#define	ISA_IOSTART		(0x80000000L) /* !NO! byte frobbing	 */
#define	ISA_MEMSTART		(0x40000000L)
#endif	/* defined(_MILANHW_) */

/*
 * Pre-allocated PCI-memory regions (atari_init.c). We need those in the
 * boot-stages.
 * XXX: Can probably be reduced to only PCI_CONF_SIZE (Leo).
 */
#define PCI_CONF_SIZE	(4 * NBPG)
#define PCI_IO_SIZE	(NBPG)
#define PCI_MEM_SIZE	(NBPG)

#define	PCI_VGA_SIZE	(32 * 1024) /* XXX Leo: Only used by grfabs_et now. */

/*
 * See bootm_init()/bootm_alloc() in bus.c for the usage of this pool
 * of pre-allocated VA-space.
 */
#define	BOOTM_VA_POOL	(32 * 8192)	/* Pre-allocated VA-space	*/

#define	AD_RAM		(0x000000L)	/* main memory			*/
#define	AD_CART		(0xFA0000L)	/* expansion cartridge		*/
#define	AD_ROM		(0xFC0000L)	/* system ROM			*/
#define	STIO_SIZE	(0x8000L)	/* Size of mapped I/O devices	*/

/*
 * Physical address of I/O area. Use only for pte initialisation!
 */
#define	STIO_PHYS	((machineid & (ATARI_HADES | ATARI_MILAN))	\
				? 0xffff8000L				\
				: 0x00ff8000L)

#if defined(_ATARIHW_)

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
#define	AD_CFG_SWITCH	(AD_STIO + 0x1200)	/* Config switches	*/
#define	AD_MFP		(AD_STIO + 0x7A00)	/* 68901		*/
#define	AD_MFP2		(AD_STIO + 0x7A80)	/* 68901-TT		*/
#define	AD_ACIA		(AD_STIO + 0x7C00)	/* 2 * 6850		*/
#endif /* defined(_ATARIHW_) */

#if defined(_MILANHW_)
/*
 * Milan onboard I/O
 */
#define	AD_MFP		(AD_STIO + 0x4100)	/* 68901		*/
#define	AD_PLX		(AD_STIO + 0x4200)	/* PLX9080		*/

/*
 * Milan special locations in the first page of the PCI I/O space.
 */
#define	AD_8259_MASTER	(pci_io_addr + 0x20)	/* Master int. contr.	*/
#define	AD_8259_SLAVE	(pci_io_addr + 0xA0)	/* Slave int. contr.	*/
#define	AD_RTC		(pci_io_addr + 0x70)	/* MC146818 compat. RTC	*/

#endif /* defined(_MILANHW_) */

#endif /* _MACHINE_IOMAP_H */
