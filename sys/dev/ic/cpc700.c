/*	$NetBSD: cpc700.c,v 1.1.4.2 2002/05/30 14:45:32 gehenna Exp $	*/

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at Sandburst Corp.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * The IBM CPC700 is a bridge chip for the PowerPC.  It contains
 *  - CPU interface
 *  - DRAM controller
 *  - PCI bus master & slave controller
 *  - interrupt controller
 *  - timer
 *  - two UARTs
 *  - two IIC ports
 *
 *  This driver handles the overall device and enumeration of the
 *  supported subdevices.  NetBSD knows how to handle:
 *  - PCI master
 *  - interrupt controller
 *  - UARTs
 *  Skeleton drivers are provided for the timer and IIC.
 *
 * XXX This driver assumes that there is only one instance of it.
 */

#include "pci.h"
#include "opt_pci.h"

#include <sys/param.h>
#include <sys/extent.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <machine/bus.h>
#include "locators.h"

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pciconf.h>

#include <dev/ic/cpc700reg.h>
#include <dev/ic/cpc700var.h>
#include <dev/ic/cpc700uic.h>

union attach_args {
	const char *busname;		/* first elem of all */
	struct pcibus_attach_args pba;
	struct cpcbus_attach_args cba;
};


void
cpc_attach(struct device *self, pci_chipset_tag_t pc, bus_space_tag_t mem,
	   bus_space_tag_t pciio, bus_dma_tag_t tag, int attachpci,
	   uint freq);

static bus_space_tag_t the_cpc_tag;
static bus_space_handle_t the_cpc_handle;
#define INL(a) bus_space_read_stream_4(the_cpc_tag, the_cpc_handle, (a))
#define OUTL(a, d) bus_space_write_stream_4(the_cpc_tag, the_cpc_handle, (a), d)

static int
cpc_print(void *aux, const char *pnp)
{
	struct cpcbus_attach_args *caa = aux;

	if (pnp)
		printf("%s at %s", caa->cpca_name, pnp);

	printf(" addr 0x%08x", caa->cpca_addr);
	if (caa->cpca_irq != CPCBUSCF_IRQ_DEFAULT)
		printf(" irq %d", caa->cpca_irq);

	return (UNCONF);
}

static int
cpcpci_print(void *aux, const char *pnp)
{
	union attach_args *aa = aux;

	if (pnp)
		printf("%s at %s", aa->busname, pnp);

	return (UNCONF);
}

static int
cpc_submatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct cpcbus_attach_args *caa = aux;

	if (cf->cf_loc[CPCBUSCF_ADDR] != caa->cpca_addr)
		return (0);

	return ((*cf->cf_attach->ca_match)(parent, cf, aux));
}

/*
 * Attach the cpc.
 */
void
cpc_attach(struct device *self, pci_chipset_tag_t pc, bus_space_tag_t mem,
	   bus_space_tag_t pciio, bus_dma_tag_t dma, int attachpci,
	   uint freq)
{
	union attach_args aa;
	int i;
	pcitag_t tag; 
	pcireg_t erren;
	static struct {
		const char *name;
		bus_addr_t addr;
		int irq;
	} devs[] = {
		{ "com", CPC_COM0, CPC_IB_UART_0 },
		{ "com", CPC_COM1, CPC_IB_UART_1 },
		{ "timer", CPC_TIMER, CPCBUSCF_IRQ_DEFAULT },
		{ "iic", CPC_IIC0, CPC_IB_IIC_0 },
		{ "iic", CPC_IIC1, CPC_IB_IIC_1 },
		{ NULL, 0 }
	};
#if NPCI > 0 && defined(PCI_NETBSD_CONFIGURE)
	struct extent *ioext, *memext;
#ifdef PCI_CONFIGURE_VERBOSE
	extern int pci_conf_debug;

	pci_conf_debug = 1;
#endif
#endif

	printf(": IBM CPC700\n");

	the_cpc_tag = mem;
	if (bus_space_map(mem, CPC_UIC_BASE, CPC_UIC_SIZE, 0,
			  &the_cpc_handle)) {
		printf("%s: can't map i/o space\n", self->dv_xname);
		return;
	}

	aa.cba.cpca_tag = mem;
	aa.cba.cpca_freq = freq;
	for (i = 0; devs[i].name; i++) {
		aa.cba.cpca_name = devs[i].name;
		aa.cba.cpca_addr = devs[i].addr;
		aa.cba.cpca_irq = devs[i].irq;
		config_found_sm(self, &aa.cba, cpc_print, cpc_submatch);
	}

	tag = pci_make_tag(pc, 0, 0, 0);

	aa.pba.pba_busname = "pci";
	aa.pba.pba_iot = pciio;
	aa.pba.pba_memt = mem;
	aa.pba.pba_dmat = dma;
	aa.pba.pba_pc = 0;
	aa.pba.pba_flags = PCI_FLAGS_MEM_ENABLED | PCI_FLAGS_IO_ENABLED;
	aa.pba.pba_bus = 0;

	/* Save PCI error condition reg. */
	erren = pci_conf_read(pc, tag, CPC_PCI_BRDGERR);
	pci_conf_write(pc, tag, CPC_PCI_BRDGERR, 0);

#if NPCI > 0 && defined(PCI_NETBSD_CONFIGURE)
	ioext  = extent_create("pciio",  CPC_PCI_IO_START, CPC_PCI_IO_END,
	    M_DEVBUF, NULL, 0, EX_NOWAIT);
	memext = extent_create("pcimem", CPC_PCI_MEM_BASE, CPC_PCI_MEM_END,
	    M_DEVBUF, NULL, 0, EX_NOWAIT);

	printf("WARNING: PCI_NETBSD_CONFIGURE is broken\n");
	pci_configure_bus(0, ioext, memext, NULL, 0, 32);

	extent_destroy(ioext);
	extent_destroy(memext);
#endif

	config_found(self, &aa.pba, cpcpci_print);

	/* Restore error triggers, and clear errors */
	pci_conf_write(pc, tag, CPC_PCI_BRDGERR, erren | CPC_PCI_CLEARERR);
}

/***************************************************************************/

/*
 * Interrupt controller.
 */

void
cpc700_init_intr(bus_space_tag_t bt, bus_space_handle_t bh,
		 u_int32_t active, u_int32_t level)
{
	/* XXX */
	the_cpc_tag = bt;
	the_cpc_handle = bh;
	/* 
	 * See CPC700 manual for information about what
	 * interrupts have which properties.
	 */
	OUTL(CPC_UIC_SR, 0xffffffff);    /* clear all intrs */
	OUTL(CPC_UIC_ER, 0x00000000);    /* disable all intrs */
	OUTL(CPC_UIC_CR, 0xffffffff);    /* gen INT not MCP */
	OUTL(CPC_UIC_PR, 0xffff8000 | active);    /* 0 = active low */
	OUTL(CPC_UIC_TR, 0xc0000000 | level);    /* 0 = level intr */
	OUTL(CPC_UIC_VR, CPC_UIC_CVR_PRI); /* intr 0 is highest */
}

int
cpc700_read_irq(void)
{
	int irq;
	u_int32_t irqs;

	irqs = INL(CPC_UIC_MSR);
	for (irq = 0; irq < ICU_LEN; irq++) {
		if (irqs & CPC_INTR_MASK(irq))
			return (irq);
	}
	return (-1);
}

void
cpc700_eoi(int irq)
{
	OUTL(CPC_UIC_SR, CPC_INTR_MASK(irq));
}

void
cpc700_disable_irq(int irq)
{
	u_int32_t reg;

	reg = INL(CPC_UIC_ER);
	reg &= ~CPC_INTR_MASK(irq);
	OUTL(CPC_UIC_ER, reg);
}

void
cpc700_enable_irq(int irq)
{
	u_int32_t reg;

	reg = INL(CPC_UIC_ER);
	reg |= CPC_INTR_MASK(irq);
	OUTL(CPC_UIC_ER, reg);
}
