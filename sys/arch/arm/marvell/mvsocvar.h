/*	$NetBSD: mvsocvar.h,v 1.3.2.3 2017/12/03 11:35:54 jdolecek Exp $	*/
/*
 * Copyright (c) 2007, 2010 KIYOHARA Takashi
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _MVSOCVAR_H_
#define _MVSOCVAR_H_

#include <sys/bus.h>

struct mvsoc_softc {
        device_t sc_dev;

	bus_addr_t sc_addr;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_dma_tag_t sc_dmat;
};

typedef int (*mvsoc_irq_handler_t)(void *);

extern uint32_t mvPclk, mvSysclk, mvTclk;
extern vaddr_t mlmb_base;
extern int nwindow, nremap;
extern int gpp_npins, gpp_irqbase;
extern struct bus_space mvsoc_bs_tag;
extern struct arm32_bus_dma_tag mvsoc_bus_dma_tag;

#define read_mlmbreg(o)		(*(volatile uint32_t *)(mlmb_base + (o)))
#define write_mlmbreg(o, v)	(*(volatile uint32_t *)(mlmb_base + (o)) = (v))

void mvsoc_bootstrap(bus_addr_t);
uint16_t mvsoc_model(void);
uint8_t mvsoc_rev(void);
void *mvsoc_bridge_intr_establish(int, int, int (*)(void *), void *);

#include <dev/marvell/marvellvar.h>

enum mvsoc_tags {
	MVSOC_TAG_INTERNALREG  = MARVELL_TAG_MAX,

	ORION_TAG_PEX0_MEM,
	ORION_TAG_PEX0_IO,
	ORION_TAG_PEX1_MEM,
	ORION_TAG_PEX1_IO,
	ORION_TAG_PCI_MEM,
	ORION_TAG_PCI_IO,
	ORION_TAG_DEVICE_CS0,
	ORION_TAG_DEVICE_CS1,
	ORION_TAG_DEVICE_CS2,
	ORION_TAG_FLASH_CS,
	ORION_TAG_DEVICE_BOOTCS,
	ORION_TAG_CRYPT,

	KIRKWOOD_TAG_PEX_MEM,
	KIRKWOOD_TAG_PEX_IO,
	KIRKWOOD_TAG_PEX1_MEM,
	KIRKWOOD_TAG_PEX1_IO,
	KIRKWOOD_TAG_NAND,
	KIRKWOOD_TAG_SPI,
	KIRKWOOD_TAG_BOOTROM,
	KIRKWOOD_TAG_CRYPT,

	MV78XX0_TAG_DEVICE_CS0,
	MV78XX0_TAG_DEVICE_CS1,
	MV78XX0_TAG_DEVICE_CS2,
	MV78XX0_TAG_DEVICE_CS3,
	MV78XX0_TAG_DEVICE_BOOTCS,
	MV78XX0_TAG_SPI,
	MV78XX0_TAG_PEX0_MEM,
	MV78XX0_TAG_PEX01_MEM,
	MV78XX0_TAG_PEX02_MEM,
	MV78XX0_TAG_PEX03_MEM,
	MV78XX0_TAG_PEX0_IO,
	MV78XX0_TAG_PEX01_IO,
	MV78XX0_TAG_PEX02_IO,
	MV78XX0_TAG_PEX03_IO,
	MV78XX0_TAG_PEX1_MEM,
	MV78XX0_TAG_PEX11_MEM,
	MV78XX0_TAG_PEX12_MEM,
	MV78XX0_TAG_PEX13_MEM,
	MV78XX0_TAG_PEX1_IO,
	MV78XX0_TAG_PEX11_IO,
	MV78XX0_TAG_PEX12_IO,
	MV78XX0_TAG_PEX13_IO,
	MV78XX0_TAG_CRYPT,

	DOVE_TAG_PEX0_MEM,
	DOVE_TAG_PEX0_IO,
	DOVE_TAG_PEX1_MEM,
	DOVE_TAG_PEX1_IO,
	DOVE_TAG_CRYPT,
	DOVE_TAG_SPI0,
	DOVE_TAG_SPI1,
	DOVE_TAG_BOOTROM,
	DOVE_TAG_NAND,
	DOVE_TAG_PMU,

	ARMADAXP_TAG_PEX00_MEM,
	ARMADAXP_TAG_PEX00_IO,
	ARMADAXP_TAG_PEX01_MEM,
	ARMADAXP_TAG_PEX01_IO,
	ARMADAXP_TAG_PEX02_MEM,
	ARMADAXP_TAG_PEX02_IO,
	ARMADAXP_TAG_PEX03_MEM,
	ARMADAXP_TAG_PEX03_IO,
	ARMADAXP_TAG_PEX10_MEM,
	ARMADAXP_TAG_PEX10_IO,
	ARMADAXP_TAG_PEX11_MEM,
	ARMADAXP_TAG_PEX11_IO,
	ARMADAXP_TAG_PEX12_MEM,
	ARMADAXP_TAG_PEX12_IO,
	ARMADAXP_TAG_PEX13_MEM,
	ARMADAXP_TAG_PEX13_IO,
	ARMADAXP_TAG_PEX2_MEM,
	ARMADAXP_TAG_PEX2_IO,
	ARMADAXP_TAG_PEX3_MEM,
	ARMADAXP_TAG_PEX3_IO,
	ARMADAXP_TAG_CRYPT0,
	ARMADAXP_TAG_CRYPT1,
};
int mvsoc_target(int, uint32_t *, uint32_t *, uint32_t *, uint32_t *);
int mvsoc_target_dump(struct mvsoc_softc *);
int mvsoc_attr_dump(struct mvsoc_softc *, uint32_t, uint32_t);

extern void (*mvsoc_intr_init)(void);
extern int (*mvsoc_clkgating)(struct marvell_attach_args *);

void orion_bootstrap(vaddr_t);
void kirkwood_bootstrap(vaddr_t);
void mv78xx0_bootstrap(vaddr_t);
void dove_bootstrap(vaddr_t);
void armadaxp_bootstrap(vaddr_t, bus_addr_t);

#endif	/* _MVSOCVAR_H_ */
