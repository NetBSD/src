/*	$NetBSD: mvsocvar.h,v 1.1.4.2 2010/10/22 09:23:12 uebayasi Exp $	*/
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

#include <machine/bus.h>

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
void * mvsoc_bridge_intr_establish(int, int, int (*)(void *), void *);

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
	KIRKWOOD_TAG_NAND,
	KIRKWOOD_TAG_SPI,
	KIRKWOOD_TAG_BOOTROM,
	KIRKWOOD_TAG_CRYPT,
};
int mvsoc_target(int, uint32_t *, uint32_t *, uint32_t *, uint32_t *);

void orion_getclks(bus_addr_t);
void orion_intr_bootstrap(void);

void kirkwood_getclks(bus_addr_t);
void kirkwood_intr_bootstrap(void);

#endif	/* _MVSOCVAR_H_ */
