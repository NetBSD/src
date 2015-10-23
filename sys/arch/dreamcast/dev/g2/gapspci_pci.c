/*	$NetBSD: gapspci_pci.c,v 1.17 2015/10/23 08:40:08 knakahara Exp $	*/

/*-
 * Copyright (c) 2001 Marcus Comstedt.
 * Copyright (c) 2001 Jason R. Thorpe.
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
 *	This product includes software developed by Marcus Comstedt.
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
 * PCI configuraiton space implementation for the SEGA GAPS PCI bridge.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: gapspci_pci.c,v 1.17 2015/10/23 08:40:08 knakahara Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <machine/cpu.h>
#include <machine/sysasicvar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include <dreamcast/dev/g2/gapspcivar.h>

void		gaps_attach_hook(device_t, device_t,
		    struct pcibus_attach_args *);
int		gaps_bus_maxdevs(void *, int);
pcitag_t	gaps_make_tag(void *, int, int, int);
void		gaps_decompose_tag(void *, pcitag_t, int *, int *, int *);
pcireg_t	gaps_conf_read(void *, pcitag_t, int);
void		gaps_conf_write(void *, pcitag_t, int, pcireg_t);

int		gaps_intr_map(const struct pci_attach_args *, pci_intr_handle_t *);
const char	*gaps_intr_string(void *, pci_intr_handle_t,
		    char *buf, size_t len);
void		*gaps_intr_establish(void *, pci_intr_handle_t,
		    int, int (*)(void *), void *);
void		gaps_intr_disestablish(void *, void *);

void
gaps_pci_init(struct gaps_softc *sc)
{
	pci_chipset_tag_t pc = &sc->sc_pc;

	memset(pc, 0, sizeof(*pc));

	pc->pc_attach_hook = gaps_attach_hook;
	pc->pc_bus_maxdevs = gaps_bus_maxdevs;
	pc->pc_make_tag = gaps_make_tag;
	pc->pc_decompose_tag = gaps_decompose_tag;
	pc->pc_conf_read = gaps_conf_read;
	pc->pc_conf_write = gaps_conf_write;
	pc->pc_conf_v = sc;

	pc->pc_intr_map = gaps_intr_map;
	pc->pc_intr_string = gaps_intr_string;
	pc->pc_intr_establish = gaps_intr_establish;
	pc->pc_intr_disestablish = gaps_intr_disestablish;

	if (bus_space_map(sc->sc_memt, 0x01001600, 0x100,
	    0, &sc->sc_pci_memh) != 0)
		panic("gaps_pci_init: can't map PCI configuration space");
}

#define	GAPS_PCITAG_MAGIC	0x022473

void
gaps_attach_hook(device_t parent, device_t pci, struct pcibus_attach_args *pba)
{
	struct gaps_softc *sc = device_private(parent);

	/*
	 * Now that we know there's a bus configured, go ahead and
	 * program the BAR on the device.
	 */
	pci_conf_write(&sc->sc_pc, GAPS_PCITAG_MAGIC,
	    PCI_MAPREG_START + 4, 0x01000000);
	pci_conf_write(&sc->sc_pc, GAPS_PCITAG_MAGIC, PCI_COMMAND_STATUS_REG,
	    pci_conf_read(&sc->sc_pc, 0, PCI_COMMAND_STATUS_REG) |
	    PCI_COMMAND_MEM_ENABLE | PCI_COMMAND_MASTER_ENABLE);
}

int
gaps_bus_maxdevs(void *v, int bus)
{

	return 1;
}

pcitag_t
gaps_make_tag(void *v, int bus, int dev, int func)
{

	if (bus == 0 && dev == 0 && func == 0)
		return GAPS_PCITAG_MAGIC;

	return 0;
}

void
gaps_decompose_tag(void *v, pcitag_t tag, int *bp, int *dp, int *fp)
{
	int b, d, f;

	if (tag == GAPS_PCITAG_MAGIC)
		b = d = f = 0;
	else {
		/*
		 * Invalid for GAPS.  These values ensure that a valid
		 * tag cannot be built.
		 */
		b = 0xff;
		d = 0x1f;
		f = 0x7;
	}

	if (bp != NULL)
		*bp = b;
	if (dp != NULL)
		*dp = d;
	if (fp != NULL)
		*fp = f;
}

pcireg_t
gaps_conf_read(void *v, pcitag_t tag, int reg)
{
	struct gaps_softc *sc = v;

	if (tag != GAPS_PCITAG_MAGIC)
		return -1;

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return -1;

	if (reg == (PCI_MAPREG_START + 4)) {
		/*
		 * We fake the BAR -- just return the physical address
		 * to which the device is mapped.
		 */
		return 0x01001700;
	}

	return bus_space_read_4(sc->sc_memt, sc->sc_pci_memh, reg);
}

void
gaps_conf_write(void *v, pcitag_t tag, int reg, pcireg_t val)
{
	struct gaps_softc *sc = v;

	if (tag != GAPS_PCITAG_MAGIC)
		return;

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return;

	/* Disallow writing to the "BAR" ... it doesn't actually exist. */
	if (reg == (PCI_MAPREG_START + 4) && val != 0x01000000)
		return;

	bus_space_write_4(sc->sc_memt, sc->sc_pci_memh, reg, val);
}

int
gaps_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{

	*ihp = SYSASIC_EVENT_EXT;
	return 0;
}

const char *
gaps_intr_string(void *v, pci_intr_handle_t ih,
    char *buf, size_t len)
{

	strlcpy(buf, sysasic_intr_string(SYSASIC_IRL11), len);
	return buf;
}

void *
gaps_intr_establish(void *v, pci_intr_handle_t ih, int level,
    int (*func)(void *), void *arg)
{

	return sysasic_intr_establish(ih, level, SYSASIC_IRL11, func, arg);
}

void
gaps_intr_disestablish(void *v, void *ih)
{

	return sysasic_intr_disestablish(ih);
}
