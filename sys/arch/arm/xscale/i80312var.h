/*	$NetBSD: i80312var.h,v 1.8 2003/10/06 16:06:05 thorpej Exp $	*/

/*
 * Copyright (c) 2001, 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARM_XSCALE_I80312VAR_H_
#define	_ARM_XSCALE_I80312VAR_H_

#include <machine/bus.h>

#include <dev/pci/pcivar.h>

struct i80312_softc {
	struct device sc_dev;		/* generic device glue */

	int sc_is_host;			/* indicates if we're a host or
					   plugged into another host */

	/*
	 * This is the bus_space and handle used to access the
	 * i80312 itself.  This is filled in by the board-specific
	 * front-end.
	 */
	bus_space_tag_t sc_st;
	bus_space_handle_t sc_sh;

	/* Handles for the various subregions. */
	bus_space_handle_t sc_ppb_sh;
	bus_space_handle_t sc_atu_sh;
	bus_space_handle_t sc_mem_sh;
	bus_space_handle_t sc_intc_sh;

	/*
	 * Secondary IDSEL Select bits for providing a private
	 * PCI device space.
	 */
	uint16_t sc_sisr;

	/*
	 * We expect the board-specific front-end to have already mapped
	 * the PCI I/O spaces .. they're only 64K each, and I/O mappings
	 * tend to be smaller than a page size, so it's generally more
	 * efficient to map them all into virtual space in one fell swoop.
	 */
	vaddr_t	sc_piow_vaddr;		/* primary I/O window vaddr */
	vaddr_t sc_siow_vaddr;		/* secondary I/O window vaddr */

	/*
	 * Variables that define the Primary Inbound window.  The base
	 * address is configured by a host via BAR #0.  The xlate variable
	 * defines the start of the local address space that it maps to.
	 * The size variable defines the byte size.
	 *
	 * This window is used for incoming PCI memory read/write cycles
	 * from a host.
	 *
	 * ...unless we're a host, in which case we make the Primary
	 * Inbound window work like the Secondary Inbound window, so
	 * that PCI devices on that bus can talk to our local RAM.
	 */
	uint32_t sc_pin_base;
	uint32_t sc_pin_xlate;
	uint32_t sc_pin_size;

	/*
	 * Variables that define the Secondary Inbound window.  The
	 * base variable indicates the PCI base address of the window.
	 * The xlate variable defines the start of the local address
	 * space that it maps to.  The size variable defines the byte
	 * size.
	 *
	 * This window is used for DMA with devices on the secondary bus.
	 */
	uint32_t sc_sin_base;
	uint32_t sc_sin_xlate;
	uint32_t sc_sin_size;

	/*
	 * This is the PCI address that the Primary Outbound Memory
	 * window maps to.
	 */
	uint32_t sc_pmemout_base;
	uint32_t sc_pmemout_size;

	/*
	 * This is the PCI address that the Primary Outbound I/O
	 * window maps to.
	 */
	uint32_t sc_pioout_base;
	uint32_t sc_pioout_size;

	/*
	 * This is the PCI address that the Secondary Outbound Memory
	 * window maps to.
	 */
	uint32_t sc_smemout_base;
	uint32_t sc_smemout_size;

	/*
	 * This is the PCI address that the Secondary Outbound I/O
	 * window maps to.
	 */
	uint32_t sc_sioout_base;
	uint32_t sc_sioout_size;

	/*
	 * This defines the private I/O and Memory spaces on the
	 * Secondary bus.
	 */
	uint32_t sc_privio_base;
	uint32_t sc_privio_size;
	uint32_t sc_privmem_base;
	uint32_t sc_privmem_size;

	uint8_t sc_sder;	/* secondary decode enable register */

	/* Bus space, DMA, and PCI tags for the PCI bus (private devices). */
	struct bus_space sc_pci_iot;
	struct bus_space sc_pci_memt;
	struct arm32_bus_dma_tag sc_pci_dmat;
	struct arm32_pci_chipset sc_pci_chipset;

	/* DMA window info for PCI DMA. */
	struct arm32_dma_range sc_pci_dma_range;

	/* GPIO state */
	uint8_t sc_gpio_dir;	/* GPIO pin direction (1 == output) */
	uint8_t sc_gpio_val;	/* GPIO output pin value */

	/* DMA tag for local devices. */
	struct arm32_bus_dma_tag sc_local_dmat;
};

/*
 * Arguments used to attach IOP built-ins.
 */
struct iopxs_attach_args {
	const char *ia_name;	/* name of device */
	bus_space_tag_t ia_st;	/* space tag */
	bus_space_handle_t ia_sh;/* handle of IOP base */
	bus_dma_tag_t ia_dmat;	/* DMA tag */
	bus_addr_t ia_offset;	/* offset of device from IOP base */
	bus_size_t ia_size;	/* size of sub-device */
};

extern struct bus_space i80312_bs_tag;
extern struct i80312_softc *i80312_softc;

void	i80312_sdram_bounds(bus_space_tag_t, bus_space_handle_t,
	    paddr_t *, psize_t *);

void	i80312_attach(struct i80312_softc *);

void	i80312_bs_init(bus_space_tag_t, void *);
void	i80312_io_bs_init(bus_space_tag_t, void *);
void	i80312_mem_bs_init(bus_space_tag_t, void *);

void	i80312_gpio_set_direction(uint8_t, uint8_t);
void	i80312_gpio_set_val(uint8_t, uint8_t);
uint8_t	i80312_gpio_get_val(void);

void	i80312_pci_init(pci_chipset_tag_t, void *);

#endif /* _ARM_XSCALE_I80312VAR_H_ */
