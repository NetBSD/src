/* $NetBSD: universe_pci.c,v 1.1 2000/02/25 18:22:39 drochner Exp $ */

/*
 * Copyright (c) 1999
 * 	Matthias Drochner.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Common functions for PCI-VME-interfaces using the
 * Newbridge/Tundra Universe II chip (CA91C142).
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
/*#include <dev/pci/pcidevs.h>*/

#include <machine/bus.h>

#include <dev/vme/vmereg.h>
#include <dev/vme/vmevar.h>

#include <dev/ic/universereg.h>
#include <dev/pci/universe_pci_var.h>

int univ_pci_intr __P((void *));

#define read_csr_4(d, reg) \
  bus_space_read_4(d->csrt, d->csrh, offsetof(struct universereg, reg))
#define write_csr_4(d, reg, val) \
  bus_space_write_4(d->csrt, d->csrh, offsetof(struct universereg, reg), val)

#define _pso(i) offsetof(struct universereg, __CONCAT(pcislv, i))
static int pcislvoffsets[8] = {
	_pso(0), _pso(1), _pso(2), _pso(3),
	_pso(4), _pso(5), _pso(6), _pso(7)
};
#undef _pso

#define read_pcislv(d, idx, reg) \
  bus_space_read_4(d->csrt, d->csrh, \
   pcislvoffsets[idx] + offsetof(struct universe_pcislvimg, reg))
#define write_pcislv(d, idx, reg, val) \
  bus_space_write_4(d->csrt, d->csrh, \
   pcislvoffsets[idx] + offsetof(struct universe_pcislvimg, reg), val)

int
univ_pci_attach(d, pa)
	struct univ_pci_data *d;
	struct pci_attach_args *pa;
{
	pci_chipset_tag_t pc = pa->pa_pc;
	pci_intr_handle_t ih;
	const char *intrstr = NULL;
	u_int32_t reg;

	d->pc = pc;

	if (pci_mapreg_map(pa, 0x10,
			   PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_32BIT,
			   0, &d->csrt, &d->csrh, NULL, NULL) &&
	    pci_mapreg_map(pa, 0x14,
			   PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_32BIT,
			   0, &d->csrt, &d->csrh, NULL, NULL) &&
	    pci_mapreg_map(pa, 0x10,
			   PCI_MAPREG_TYPE_IO,
			   0, &d->csrt, &d->csrh, NULL, NULL) &&
	    pci_mapreg_map(pa, 0x14,
			   PCI_MAPREG_TYPE_IO,
			   0, &d->csrt, &d->csrh, NULL, NULL))
		return (-1);

	reg = read_csr_4(d, misc_ctl);
	printf("univ_pci_attach: misc_ctl=%x\n", reg);

	/* enable DMA */
	pci_conf_write(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
	    pci_conf_read(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG) |
	    PCI_COMMAND_MASTER_ENABLE);

	/* Map and establish the interrupt. */
	if (pci_intr_map(pc, pa->pa_intrtag, pa->pa_intrpin,
	    pa->pa_intrline, &ih)) {
		printf("univ_pci_attach: couldn't map interrupt\n");
		return (-1);
	}
	intrstr = pci_intr_string(pc, ih);
	/*
	 * Use a low interrupt level (the lowest?).
	 * We will raise before calling a subdevice's handler.
	 */
	d->ih = pci_intr_establish(pc, ih, IPL_BIO, univ_pci_intr, d);
	if (d->ih == NULL) {
		printf("univ_pci_attach: couldn't establish interrupt");
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return (-1);
	}
	printf("univ_pci: interrupting at %s\n", intrstr);

	return (0);
}

int
univ_pci_mapvme(d, wnd, vmebase, len, am, datawidth, pcibase)
	struct univ_pci_data *d;
	int wnd;
	vme_addr_t vmebase;
	u_int32_t len;
	vme_am_t am;
	vme_datasize_t datawidth;
	u_int32_t pcibase;
{
	u_int32_t ctl = 0x80000000;

	switch (am & VME_AM_ADRSIZEMASK) {
	case VME_AM_A32:
		ctl |= 0x00020000;
		break;
	case VME_AM_A24:
		ctl |= 0x00010000;
		break;
	case VME_AM_A16:
		break;
	default:
		return (EINVAL);
	}
	if (am & VME_AM_SUPER)
		ctl |= 0x00001000;
	if ((am & VME_AM_MODEMASK) == VME_AM_PRG)
		ctl |= 0x00004000;
	if (datawidth & VME_D32)
		ctl |= 0x00800000;
	else if (datawidth & VME_D16)
		ctl |= 0x00400000;
	else if (!(datawidth & VME_D8))
		return (EINVAL);

#ifdef UNIV_DEBUG
	printf("univ_pci_mapvme: wnd %d, map %x-%x to %x, ctl=%x\n",
	       wnd, pcibase, pcibase + len, vmebase, ctl);
#endif

	write_pcislv(d, wnd, lsi_bs, pcibase);
	write_pcislv(d, wnd, lsi_bd, pcibase + len);
	write_pcislv(d, wnd, lsi_to, vmebase - pcibase);
	write_pcislv(d, wnd, lsi_ctl, ctl);
	return (0);
}

void
univ_pci_unmapvme(d, wnd)
	struct univ_pci_data *d;
	int wnd;
{
#ifdef UNIV_DEBUG
	printf("univ_pci_unmapvme: wnd %d\n", wnd);
#endif
	write_pcislv(d, wnd, lsi_ctl, 0);
}

int
univ_pci_intr(v)
	void *v;
{
	struct univ_pci_data *d = v;
	u_int32_t intcsr;

	intcsr = read_csr_4(d, lint_stat) & 0xffffff;

	if (!intcsr)
		return (0);

	/* ack everything */
	write_csr_4(d, lint_stat, intcsr);

	printf("univ_pci_intr: lint_stat=%x\n", intcsr);

	return (1);
}
