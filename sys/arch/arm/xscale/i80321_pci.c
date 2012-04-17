/*	$NetBSD: i80321_pci.c,v 1.10.2.1 2012/04/17 00:06:07 yamt Exp $	*/

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
 * PCI configuration support for i80321 I/O Processor chip.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i80321_pci.c,v 1.10.2.1 2012/04/17 00:06:07 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <sys/bus.h>

#include <arm/xscale/i80321reg.h>
#include <arm/xscale/i80321var.h>

#include <dev/pci/ppbreg.h>
#include <dev/pci/pciconf.h>

#include "opt_pci.h"
#include "opt_i80321.h"
#include "pci.h"

void		i80321_pci_attach_hook(struct device *, struct device *,
		    struct pcibus_attach_args *);
int		i80321_pci_bus_maxdevs(void *, int);
pcitag_t	i80321_pci_make_tag(void *, int, int, int);
void		i80321_pci_decompose_tag(void *, pcitag_t, int *, int *,
		    int *);
pcireg_t	i80321_pci_conf_read(void *, pcitag_t, int);
void		i80321_pci_conf_write(void *, pcitag_t, int, pcireg_t);

#define	PCI_CONF_LOCK(s)	(s) = disable_interrupts(I32_bit)
#define	PCI_CONF_UNLOCK(s)	restore_interrupts((s))

void
i80321_pci_init(pci_chipset_tag_t pc, void *cookie)
{
#if NPCI > 0 && defined(PCI_NETBSD_CONFIGURE)
	struct i80321_softc *sc = cookie;
	struct extent *ioext, *memext;
	uint32_t busno;
#endif

	pc->pc_conf_v = cookie;
	pc->pc_attach_hook = i80321_pci_attach_hook;
	pc->pc_bus_maxdevs = i80321_pci_bus_maxdevs;
	pc->pc_make_tag = i80321_pci_make_tag;
	pc->pc_decompose_tag = i80321_pci_decompose_tag;
	pc->pc_conf_read = i80321_pci_conf_read;
	pc->pc_conf_write = i80321_pci_conf_write;

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

	busno = bus_space_read_4(sc->sc_st, sc->sc_atu_sh, ATU_PCIXSR);
	busno = PCIXSR_BUSNO(busno);
	if (busno == 0xff)
		busno = 0;

	ioext  = extent_create("pciio",
	    sc->sc_ioout_xlate + sc->sc_ioout_xlate_offset,
	    sc->sc_ioout_xlate + VERDE_OUT_XLATE_IO_WIN_SIZE - 1,
	    NULL, 0, EX_NOWAIT);

#ifdef I80321_USE_DIRECT_WIN
	memext = extent_create("pcimem", VERDE_OUT_DIRECT_WIN_BASE + VERDE_OUT_DIRECT_WIN_SKIP,
	    VERDE_OUT_DIRECT_WIN_BASE + VERDE_OUT_DIRECT_WIN_SIZE- 1,
	    NULL, 0, EX_NOWAIT);
#else
	memext = extent_create("pcimem", sc->sc_owin[0].owin_xlate_lo,
	    sc->sc_owin[0].owin_xlate_lo + VERDE_OUT_XLATE_MEM_WIN_SIZE - 1,
	    NULL, 0, EX_NOWAIT);
#endif

	aprint_normal_dev(sc->sc_dev, "configuring PCI bus\n");
	pci_configure_bus(pc, ioext, memext, NULL, busno, arm_dcache_align);

	extent_destroy(ioext);
	extent_destroy(memext);
#endif
}

void
pci_conf_interrupt(pci_chipset_tag_t pc, int a, int b, int c, int d, int *p)
{
}

void
i80321_pci_attach_hook(struct device *parent, struct device *self,
    struct pcibus_attach_args *pba)
{

	/* Nothing to do. */
}

int
i80321_pci_bus_maxdevs(void *v, int busno)
{

	return (32);
}

pcitag_t
i80321_pci_make_tag(void *v, int b, int d, int f)
{

	return ((b << 16) | (d << 11) | (f << 8));
}

void
i80321_pci_decompose_tag(void *v, pcitag_t tag, int *bp, int *dp, int *fp)
{

	if (bp != NULL)
		*bp = (tag >> 16) & 0xff;
	if (dp != NULL)
		*dp = (tag >> 11) & 0x1f;
	if (fp != NULL)
		*fp = (tag >> 8) & 0x7;
}

struct pciconf_state {
	uint32_t ps_addr_val;

	int ps_b, ps_d, ps_f;
};

static int
i80321_pci_conf_setup(struct i80321_softc *sc, pcitag_t tag, int offset,
    struct pciconf_state *ps)
{
	uint32_t busno;

	i80321_pci_decompose_tag(sc, tag, &ps->ps_b, &ps->ps_d, &ps->ps_f);

	busno = bus_space_read_4(sc->sc_st, sc->sc_atu_sh, ATU_PCIXSR);
	busno = PCIXSR_BUSNO(busno);
	if (busno == 0xff)
		busno = 0;

	/*
	 * If the bus # is the same as our own, then use Type 0 cycles,
	 * else use Type 1.
	 *
	 * XXX We should filter out all non-private devices here!
	 * XXX How does private space interact with PCI-PCI bridges?
	 */
	if (ps->ps_b == busno) {
		if (ps->ps_d > (31 - 16))
			return (1);
		/*
		 * NOTE: PCI-X requires that that devices updated their
		 * PCIXSR on every config write with the device number
		 * specified in AD[15:11].  If we don't set this field,
		 * each device could end of thinking it is at device 0,
		 * which can cause a number of problems.  Doing this
		 * unconditionally should be OK when only PCI devices
		 * are present.
		 */
		ps->ps_addr_val = (1U << (ps->ps_d + 16)) |
		    (ps->ps_d << 11) | (ps->ps_f << 8) | offset;
	} else {
		/* The tag is already in the correct format. */
		ps->ps_addr_val = tag | offset | 1;
	}

	return (0);
}

pcireg_t
i80321_pci_conf_read(void *v, pcitag_t tag, int offset)
{
	struct i80321_softc *sc = v;
	struct pciconf_state ps;
	vaddr_t va;
	uint32_t isr;
	pcireg_t rv;
	u_int s;

	if (i80321_pci_conf_setup(sc, tag, offset, &ps))
		return ((pcireg_t) -1);

	PCI_CONF_LOCK(s);

	bus_space_write_4(sc->sc_st, sc->sc_atu_sh, ATU_OCCAR,
	    ps.ps_addr_val);

	va = (vaddr_t) bus_space_vaddr(sc->sc_st, sc->sc_atu_sh);
	if (badaddr_read((void *) (va + ATU_OCCDR), sizeof(rv), &rv)) {
		isr = bus_space_read_4(sc->sc_st, sc->sc_atu_sh, ATU_ATUISR);
		bus_space_write_4(sc->sc_st, sc->sc_atu_sh, ATU_ATUISR,
		    isr & (ATUISR_P_SERR_DET|ATUISR_PMA|ATUISR_PTAM|
			   ATUISR_PTAT|ATUISR_PMPE));
#if 0
		printf("conf_read: %d/%d/%d bad address\n",
		    ps.ps_b, ps.ps_d, ps.ps_f);
#endif
		rv = (pcireg_t) -1;
	}

	PCI_CONF_UNLOCK(s);

	return (rv);
}

void
i80321_pci_conf_write(void *v, pcitag_t tag, int offset, pcireg_t val)
{
	struct i80321_softc *sc = v;
	struct pciconf_state ps;
	u_int s;

	if (i80321_pci_conf_setup(sc, tag, offset, &ps))
		return;

	PCI_CONF_LOCK(s);

	bus_space_write_4(sc->sc_st, sc->sc_atu_sh, ATU_OCCAR,
	    ps.ps_addr_val);
	bus_space_write_4(sc->sc_st, sc->sc_atu_sh, ATU_OCCDR, val);

	PCI_CONF_UNLOCK(s);
}
