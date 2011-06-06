/*	$NetBSD: hpcvar.h,v 1.11.76.1 2011/06/06 09:06:40 jruoho Exp $	*/

/*
 * Copyright (c) 2001 Rafal K. Boni
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
 */

#ifndef _ARCH_SGIMIPS_HPC_HPCVAR_H_
#define	_ARCH_SGIMIPS_HPC_HPCVAR_H_

#define HPCDEV_IP12		(1U << 0)	/* Indigo R3k, 4D/3x */
#define HPCDEV_IP20		(1U << 1)	/* Indigo R4k */
#define HPCDEV_IP22		(1U << 2)	/* Indigo2 */
#define HPCDEV_IP24		(1U << 3)	/* Indy */

/* HPC 1.5/3 differ a bit, thus we need an abstraction layer */

struct hpc_values {     
	int		revision;
        uint32_t	scsi0_regs;
        uint32_t	scsi0_regs_size; 
        uint32_t	scsi0_cbp;
        uint32_t	scsi0_ndbp;
        uint32_t	scsi0_bc;
        uint32_t	scsi0_ctl;
        uint32_t	scsi0_gio;
        uint32_t	scsi0_dev;
        uint32_t	scsi0_dmacfg;
        uint32_t	scsi0_piocfg;
        uint32_t	scsi1_regs;
        uint32_t	scsi1_regs_size;
        uint32_t	scsi1_cbp;
        uint32_t	scsi1_ndbp;
        uint32_t	scsi1_bc;
        uint32_t	scsi1_ctl;
        uint32_t	scsi1_gio;
        uint32_t	scsi1_dev;
        uint32_t	scsi1_dmacfg;
        uint32_t	scsi1_piocfg;
        uint32_t	enet_regs;
        uint32_t	enet_regs_size;
        uint32_t	enet_intdelay;
        uint32_t	enet_intdelayval;
        uint32_t	enetr_cbp;
        uint32_t	enetr_ndbp;
        uint32_t	enetr_bc;
        uint32_t	enetr_ctl;
        uint32_t	enetr_ctl_active;
        uint32_t	enetr_reset;
        uint32_t	enetr_dmacfg;
        uint32_t	enetr_piocfg;
        uint32_t	enetx_cbp;
        uint32_t	enetx_ndbp;
        uint32_t	enetx_bc;
        uint32_t	enetx_ctl;
        uint32_t	enetx_ctl_active;
        uint32_t	enetx_dev;
        uint32_t	enetr_fifo;
        uint32_t	enetr_fifo_size;
        uint32_t	enetx_fifo;
        uint32_t	enetx_fifo_size;
        uint32_t	scsi0_devregs_size;
        uint32_t	scsi1_devregs_size;
        uint32_t	enet_devregs;
        uint32_t	enet_devregs_size;
        uint32_t	pbus_fifo;
        uint32_t	pbus_fifo_size;
        uint32_t	pbus_bbram;
        uint32_t	scsi_max_xfer;
	uint32_t	scsi_dma_segs;
        uint32_t	scsi_dma_segs_size;
        uint32_t	scsi_dma_datain_cmd;
        uint32_t	scsi_dma_dataout_cmd;
        uint32_t	scsi_dmactl_flush;
        uint32_t	scsi_dmactl_active;
        uint32_t	scsi_dmactl_reset;
};

struct hpc_attach_args {
	const char		*ha_name;	/* name of device */
	bus_addr_t		ha_devoff;	/* offset of device */
	bus_addr_t		ha_dmaoff;	/* offset of DMA regs */
	int			ha_irq;		/* interrupt line */

	bus_space_tag_t		ha_st;		/* HPC space tag */
	bus_space_handle_t	ha_sh;		/* HPC space handle XXX */
	bus_dma_tag_t		ha_dmat;	/* HPC DMA tag */

	struct hpc_values	*hpc_regs;	/* HPC register definitions */

	uint8_t			hpc_eeprom[256];/* HPC eeprom contents */
};

#endif	/* _ARCH_SGIMIPS_HPC_HPCVAR_H_ */
