/*	$NetBSD: vsbus.h,v 1.19.16.1 2017/08/28 17:51:54 skrll Exp $ */
/*
 * Copyright (c) 1996 Ludd, University of Lule}, Sweden.
 * All rights reserved.
 *
 * This code is derived from software contributed to Ludd by Bertram Barth.
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

/*
 * Generic definitions for the (virtual) vsbus. contains common info
 * used by all VAXstations.
 */

#ifndef _VAX_VSBUS_H_
#define _VAX_VSBUS_H_

#include <machine/bus.h>
#include <machine/sgmap.h>

struct	vsbus_attach_args {
	vaddr_t	va_addr;		/* virtual CSR address */
	paddr_t	va_paddr;		/* physical CSR address */
	short	va_br;			/* Interrupt level */
	short	va_cvec;		/* Interrupt vector address */
	u_char	va_maskno;		/* Interrupt vector in mask */
	vaddr_t	va_dmaaddr;		/* DMA area address */
	vsize_t	va_dmasize;		/* DMA area size */
	bus_space_tag_t va_memt;
	bus_dma_tag_t va_dmat;
};

/*
 * Some chip addresses and constants, same on all VAXstations.
 */
#define VS_CFGTST	0x20020000      /* config register */
#define VS_REGS         0x20080000      /* Misc CPU internal regs */
#define VS_REGS_KA49	0x25c00000      /* ... same, on 512KB ROM systems */
#define NI_ADDR         0x20090000      /* Ethernet address */
#define DZ_CSR          0x200a0000      /* DZ11-compatible chip csr */
#define DZ_CSR_KA49	0x25000000      /* ... same, on 512KB ROM systems */
#define VS_CLOCK        0x200b0000      /* clock chip address */
#define SCA_REGS        0x200c0000      /* disk device addresses */
#define SCA_REGS_KA49	0x26000000      /* ... same, on 512KB ROM systems */
#define NI_BASE         0x200e0000      /* LANCE CSRs */
#define NI_IOSIZE       (128 * VAX_NBPG)    /* IO address size */

#define	KA49_SCSIMAP	0x27000000	/* KA49 SCSI SGMAP */
/*
 * Small monochrome graphics framebuffer, present on all machines.
 */
#define	SMADDR		0x30000000
#define	SMSIZE		0x20000		/* Actually 256k, only 128k used */

struct	vsbus_softc {
	device_t sc_dev;
	u_char	*sc_intmsk;	/* Mask register */
	u_char	*sc_intclr;	/* Clear interrupt register */
	u_char	*sc_intreq;	/* Interrupt request register */
	u_char	sc_mask;	/* Interrupts to enable after autoconf */
	vaddr_t	sc_vsregs;	/* Where the VS_REGS are mapped */
	vaddr_t sc_dmaaddr;	/* Mass storage virtual DMA area */
	vsize_t sc_dmasize;	/* Size of the DMA area */

	bus_space_tag_t sc_iot;
	struct vax_bus_dma_tag sc_dmatag;
	struct vax_sgmap sc_sgmap;
};

struct vsbus_dma {
	SIMPLEQ_ENTRY(vsbus_dma) vd_q;
	void (*vd_go)(void *);
	void *vd_arg;
};

#ifdef _KERNEL
void	vsbus_dma_init(struct vsbus_softc *, unsigned ptecnt);
u_char	vsbus_setmask(int);
void	vsbus_clrintr(int);
void	vsbus_copytoproc(struct proc *, void *, void *, int);
void	vsbus_copyfromproc(struct proc *, void *, void *, int);
void	vsbus_dma_start(struct vsbus_dma *);
void	vsbus_dma_intr(void);
#endif
#endif /* _VAX_VSBUS_H_ */
