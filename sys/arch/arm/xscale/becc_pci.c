/*	$NetBSD: becc_pci.c,v 1.3 2003/05/23 05:21:26 briggs Exp $	*/

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

/*
 * PCI configuration support for the ADI Engineering Big Endian Companion
 * Chip.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>

#include <arm/xscale/beccreg.h>
#include <arm/xscale/beccvar.h>

#include <dev/pci/ppbreg.h>
#include <dev/pci/pciconf.h>

#include "opt_pci.h"
#include "pci.h"

void		becc_pci_attach_hook(struct device *, struct device *,
		    struct pcibus_attach_args *);
int		becc_pci_bus_maxdevs(void *, int);
pcitag_t	becc_pci_make_tag(void *, int, int, int);
void		becc_pci_decompose_tag(void *, pcitag_t, int *, int *,
		    int *);
pcireg_t	becc_pci_conf_read(void *, pcitag_t, int);
void		becc_pci_conf_write(void *, pcitag_t, int, pcireg_t);

int		becc_pci_intr_map(struct pci_attach_args *,
		    pci_intr_handle_t *);
const char	*becc_pci_intr_string(void *, pci_intr_handle_t);
const struct evcnt *becc_pci_intr_evcnt(void *, pci_intr_handle_t);
void		*becc_pci_intr_establish(void *, pci_intr_handle_t,
		    int, int (*)(void *), void *);
void		becc_pci_intr_disestablish(void *, void *);

#define	PCI_CONF_LOCK(s)	(s) = disable_interrupts(I32_bit)
#define	PCI_CONF_UNLOCK(s)	restore_interrupts((s))

#if 0
#define DPRINTF(x) printf(x)
#else
#define DPRINTF(x)
#endif

void
becc_pci_init(pci_chipset_tag_t pc, void *cookie)
{
#if NPCI > 0 && defined(PCI_NETBSD_CONFIGURE)
	struct becc_softc *sc = cookie;
	struct extent *ioext, *memext;
#endif

	pc->pc_conf_v = cookie;
	pc->pc_attach_hook = becc_pci_attach_hook;
	pc->pc_bus_maxdevs = becc_pci_bus_maxdevs;
	pc->pc_make_tag = becc_pci_make_tag;
	pc->pc_decompose_tag = becc_pci_decompose_tag;
	pc->pc_conf_read = becc_pci_conf_read;
	pc->pc_conf_write = becc_pci_conf_write;

	pc->pc_intr_v = cookie;
	pc->pc_intr_map = becc_pci_intr_map;
	pc->pc_intr_string = becc_pci_intr_string;
	pc->pc_intr_evcnt = becc_pci_intr_evcnt;
	pc->pc_intr_establish = becc_pci_intr_establish;
	pc->pc_intr_disestablish = becc_pci_intr_disestablish;

#if NPCI > 0 && defined(PCI_NETBSD_CONFIGURE)
	/*
	 * Configure the PCI bus.
	 *
	 * XXX We need to revisit this.  We only configure the Secondary
	 * bus (and its children).  The bus configure code needs changes
	 * to support how the busses are arranged on this chip.  We also
	 * need to only configure devices in the private device space on
	 * the Secondary bus.
	 */

	/* Reserve the bottom 32K of the PCI address space. */
	ioext  = extent_create("pciio", sc->sc_ioout_xlate + (32 * 1024),
	    sc->sc_ioout_xlate + (64 * 1024) - 1,
	    M_DEVBUF, NULL, 0, EX_NOWAIT);
	memext = extent_create("pcimem", sc->sc_owin_xlate[0],
	    sc->sc_owin_xlate[0] + BECC_PCI_MEM1_SIZE - 1,
	    M_DEVBUF, NULL, 0, EX_NOWAIT);

	aprint_normal("%s: configuring PCI bus\n", sc->sc_dev.dv_xname);
	pci_configure_bus(pc, ioext, memext, NULL, 0, arm_dcache_align);

	extent_destroy(ioext);
	extent_destroy(memext);
#endif
}

void
pci_conf_interrupt(pci_chipset_tag_t pc, int a, int b, int c, int d, int *p)
{
}

void
becc_pci_attach_hook(struct device *parent, struct device *self,
    struct pcibus_attach_args *pba)
{

	/* Nothing to do. */
}

int
becc_pci_bus_maxdevs(void *v, int busno)
{

	return (32);
}

pcitag_t
becc_pci_make_tag(void *v, int b, int d, int f)
{

	return ((b << 16) | (d << 11) | (f << 8));
}

void
becc_pci_decompose_tag(void *v, pcitag_t tag, int *bp, int *dp, int *fp)
{

	if (bp != NULL)
		*bp = (tag >> 16) & 0xff;
	if (dp != NULL)
		*dp = (tag >> 11) & 0x1f;
	if (fp != NULL)
		*fp = (tag >> 8) & 0x7;
}

struct pciconf_state {
	uint32_t ps_offset;

	int ps_b, ps_d, ps_f;
	int ps_type;
};

static int
becc_pci_conf_setup(struct becc_softc *sc, pcitag_t tag, int offset,
    struct pciconf_state *ps)
{

	becc_pci_decompose_tag(sc, tag, &ps->ps_b, &ps->ps_d, &ps->ps_f);

	/*
	 * If the bus # is the same as our own, then use Type 0 cycles,
	 * else use Type 1.
	 */
	if (ps->ps_b == 0) {
		/* XXX This is a platform-specific parameter. */
		if (ps->ps_d > (14 - BECC_IDSEL_BIT))
			return (1);
		ps->ps_offset = (1U << (ps->ps_d + BECC_IDSEL_BIT)) |
		    (ps->ps_f << 8) | offset;
		ps->ps_type = 0;
	} else {
		/* The tag is already in the correct format. */
		ps->ps_offset = tag | offset | 1;
		ps->ps_type = 1;
	}

	return (0);
}

static int becc_pci_conf_cleanup(struct becc_softc *sc);
static int
becc_pci_conf_cleanup(struct becc_softc *sc)
{
	uint32_t reg;
	int	err=0;

	BECC_CSR_WRITE(BECC_POCR, 0);

	reg = becc_pcicore_read(sc, PCI_COMMAND_STATUS_REG);
	if (reg & 0xf9000000) {
		DPRINTF((" ** pci status error: %08x (%08x) **\n",
		    reg, reg & 0xf9000000));

		err = 1;
		becc_pcicore_write(sc, PCI_COMMAND_STATUS_REG,
		    reg & 0xf900ffff);
		reg = becc_pcicore_read(sc, PCI_COMMAND_STATUS_REG);

		DPRINTF((" ** pci status after clearing: %08x (%08x) **\n",
		    reg, reg & 0xf9000000));
	}
	reg = BECC_CSR_READ(BECC_PMISR);
	if (reg & 0x000f000d) {
		DPRINTF((" ** pci master isr: %08x (%08x) **\n",
		    reg, reg & 0x000f000d));

		err = 1;
		BECC_CSR_WRITE(BECC_PMISR, reg & 0x000f000d);
		reg = BECC_CSR_READ(BECC_PMISR);

		DPRINTF((" ** pci master isr after clearing: %08x (%08x) **\n",
		    reg, reg & 0x000f000d));
	}
	reg = BECC_CSR_READ(BECC_PSISR);
	if (reg & 0x000f0210) {
		DPRINTF((" ** pci slave isr: %08x (%08x) **\n",
		    reg, reg & 0x000f0210));

		err = 1;
		BECC_CSR_WRITE(BECC_PSISR, reg & 0x000f0210);
		reg = BECC_CSR_READ(BECC_PSISR);

		DPRINTF((" ** pci slave isr after clearing: %08x (%08x) **\n",
		    reg, reg & 0x000f0210));
	}

	return err;
}

pcireg_t
becc_pci_conf_read(void *v, pcitag_t tag, int offset)
{
	struct becc_softc *sc = v;
	struct pciconf_state ps;
	vaddr_t va;
	pcireg_t rv;
	u_int s;

	if (becc_pci_conf_setup(sc, tag, offset, &ps))
		return ((pcireg_t) -1);

	/*
	 * Skip device 0 (the BECC itself).  We don't want it
	 * to appear as part of the PCI device space.
	 */
	if (ps.ps_b == 0 && ps.ps_d == 0)
		return ((pcireg_t) -1);

	PCI_CONF_LOCK(s);

	va = sc->sc_pci_cfg_base + ps.ps_offset;
	BECC_CSR_WRITE(BECC_POCR, ps.ps_type);

	if (badaddr_read((void *) va, sizeof(rv), &rv)) {
		/* XXX Check master/target abort? */
#if 0
		printf("conf_read: %d/%d/%d bad address\n",
		    ps.ps_b, ps.ps_d, ps.ps_f);
#endif
		rv = (pcireg_t) -1;
	}

	if (becc_pci_conf_cleanup(sc))
		rv = (pcireg_t) -1;

	PCI_CONF_UNLOCK(s);

	return (rv);
}

void
becc_pci_conf_write(void *v, pcitag_t tag, int offset, pcireg_t val)
{
	struct becc_softc *sc = v;
	struct pciconf_state ps;
	vaddr_t va;
	u_int s;

	if (becc_pci_conf_setup(sc, tag, offset, &ps))
		return;

	PCI_CONF_LOCK(s);
	BECC_CSR_WRITE(BECC_POCR, ps.ps_type);

	va = sc->sc_pci_cfg_base + ps.ps_offset;

	*(__volatile pcireg_t *)va = val;

	becc_pci_conf_cleanup(sc);

	PCI_CONF_UNLOCK(s);
}

int
becc_pci_intr_map(struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	int irq;

	if (pa->pa_bus == 0) {
		switch (pa->pa_device) {
		case 1: irq = ICU_PCI_INTB; break; /* Ethernet #0 */
		case 2: irq = ICU_PCI_INTC; break; /* Ethernet #1 */
		case 3:				   /* Card slot */
			switch (pa->pa_intrpin) {
			case 1:		irq = ICU_PCI_INTA; break;
			case 2:		irq = ICU_PCI_INTB; break;
			case 3:		irq = ICU_PCI_INTC; break;
			case 4:		irq = ICU_PCI_INTD; break;
			default:
				printf("becc_pci_intr_map: bogus pin: %d\n",
				    pa->pa_intrpin);
				return (1);
			}
			break;
		default:
			break;
		}
	} else {
		switch (pa->pa_intrpin) {
		case 1:		irq = ICU_PCI_INTA; break;
		case 2:		irq = ICU_PCI_INTB; break;
		case 3:		irq = ICU_PCI_INTC; break;
		case 4:		irq = ICU_PCI_INTD; break;
		default:
			printf("becc_pci_intr_map: bogus pin: %d\n",
			    pa->pa_intrpin);
			return (1);
		}
	}

	*ihp = irq;
	return (0);
}

const char *
becc_pci_intr_string(void *v, pci_intr_handle_t ih)
{

	return (becc_irqnames[ih]);
}

const struct evcnt *
becc_pci_intr_evcnt(void *v, pci_intr_handle_t ih)
{

	/* XXX For now. */
	return (NULL);
}

void *
becc_pci_intr_establish(void *v, pci_intr_handle_t ih, int ipl,
    int (*func)(void *), void *arg)
{

	return (becc_intr_establish(ih, ipl, func, arg));
}

void
becc_pci_intr_disestablish(void *v, void *cookie)
{

	becc_intr_disestablish(cookie);
}
