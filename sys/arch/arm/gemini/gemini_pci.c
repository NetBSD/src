/*	$NetBSD: gemini_pci.c,v 1.8 2010/01/05 13:14:56 mbalmer Exp $	*/

/* adapted from:
 *	NetBSD: i80312_pci.c,v 1.9 2005/12/11 12:16:51 christos Exp
 */

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
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
 * PCI configuration support for i80312 Companion I/O chip.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gemini_pci.c,v 1.8 2010/01/05 13:14:56 mbalmer Exp $");

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <arm/pic/picvar.h>

#include <arm/gemini/gemini_reg.h>
#include <arm/gemini/gemini_pcivar.h>
#include <arm/gemini/gemini_obiovar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciconf.h>

#include <machine/pci_machdep.h>

#include "opt_gemini.h"
#include "opt_pci.h"
#include "pci.h"

void		gemini_pci_attach_hook(struct device *, struct device *,
		    struct pcibus_attach_args *);
int		gemini_pci_bus_maxdevs(void *, int);
pcitag_t	gemini_pci_make_tag(void *, int, int, int);
void		gemini_pci_decompose_tag(void *, pcitag_t, int *, int *,
		    int *);
pcireg_t	gemini_pci_conf_read(void *, pcitag_t, int);
void		gemini_pci_conf_write(void *, pcitag_t, int, pcireg_t);
int		gemini_pci_conf_hook(pci_chipset_tag_t, int, int, int,
		    pcireg_t);

int		gemini_pci_intr_map(struct pci_attach_args *,
		    pci_intr_handle_t *);
const char	*gemini_pci_intr_string(void *, pci_intr_handle_t);
const struct evcnt *gemini_pci_intr_evcnt(void *, pci_intr_handle_t);
void		*gemini_pci_intr_establish(void *, pci_intr_handle_t,
		    int, int (*)(void *), void *);
void		gemini_pci_intr_disestablish(void *, void *);
int		gemini_pci_intr_handler(void *v);

#define	PCI_CONF_LOCK(s)	(s) = disable_interrupts(I32_bit)
#define	PCI_CONF_UNLOCK(s)	restore_interrupts((s))

struct gemini_pci_intrq {
	SIMPLEQ_ENTRY(gemini_pci_intrq) iq_q;
	int (*iq_func)(void *);
	void *iq_arg;
	void *iq_ih;
};

static SIMPLEQ_HEAD(, gemini_pci_intrq) gemini_pci_intrq =
	SIMPLEQ_HEAD_INITIALIZER(gemini_pci_intrq);

static inline int
gemini_pci_intrq_empty(void)
{
	return SIMPLEQ_EMPTY(&gemini_pci_intrq);
}

static inline void *
gemini_pci_intrq_insert(void *ih, int (*func)(void *), void *arg)
{
	struct gemini_pci_intrq *iqp;

        iqp = malloc(sizeof(*iqp), M_DEVBUF, M_NOWAIT|M_ZERO);
        if (iqp == NULL) {
		printf("gemini_pci_intrq_insert: malloc failed\n");
		return NULL;
	}

        iqp->iq_func = func;
        iqp->iq_arg = arg;
        iqp->iq_ih = ih;
        SIMPLEQ_INSERT_TAIL(&gemini_pci_intrq, iqp, iq_q);

	return (void *)iqp;
}

static inline void
gemini_pci_intrq_remove(void *cookie)
{
	struct gemini_pci_intrq *iqp;

	SIMPLEQ_FOREACH(iqp, &gemini_pci_intrq, iq_q) {
		if ((void *)iqp == cookie) {
			SIMPLEQ_REMOVE(&gemini_pci_intrq,
				iqp, gemini_pci_intrq, iq_q);
			free(iqp, M_DEVBUF);
			return;
		}
	}
}

static inline int
gemini_pci_intrq_dispatch(void)
{
	struct gemini_pci_intrq *iqp;

	SIMPLEQ_FOREACH(iqp, &gemini_pci_intrq, iq_q) {
		(*iqp->iq_func)(iqp->iq_arg);
	}

	return 1;
}

void
gemini_pci_init(pci_chipset_tag_t pc, void *cookie)
{
#if NPCI > 0 && defined(PCI_NETBSD_CONFIGURE)
	struct obio_softc *sc = cookie;
	struct extent *ioext, *memext;
#endif

	pc->pc_conf_v = cookie;
	pc->pc_attach_hook = gemini_pci_attach_hook;
	pc->pc_bus_maxdevs = gemini_pci_bus_maxdevs;
	pc->pc_make_tag = gemini_pci_make_tag;
	pc->pc_decompose_tag = gemini_pci_decompose_tag;
	pc->pc_conf_read = gemini_pci_conf_read;
	pc->pc_conf_write = gemini_pci_conf_write;

	pc->pc_intr_v = cookie;
	pc->pc_intr_map = gemini_pci_intr_map;
	pc->pc_intr_string = gemini_pci_intr_string;
	pc->pc_intr_evcnt = gemini_pci_intr_evcnt;
	pc->pc_intr_establish = gemini_pci_intr_establish;
	pc->pc_intr_disestablish = gemini_pci_intr_disestablish;

	pc->pc_conf_hook = gemini_pci_conf_hook;

	/*
	 * initialize copy of CFG_CMD
	 */
	sc->sc_pci_chipset.pc_cfg_cmd =
		gemini_pci_conf_read(sc, 0, GEMINI_PCI_CFG_CMD); 

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

	aprint_normal("%s: configuring Secondary PCI bus\n",
		device_xname(sc->sc_dev));

	/*
	 * XXX PCI IO addr should be inherited ?
	 */
	ioext  = extent_create("pciio",
		GEMINI_PCIIO_BASE,
		GEMINI_PCIIO_BASE + GEMINI_PCIIO_SIZE - 1,
		M_DEVBUF, NULL, 0, EX_NOWAIT);

	/*
	 * XXX PCI mem addr should be inherited ?
	 */
	memext = extent_create("pcimem",
		GEMINI_PCIMEM_BASE,
		GEMINI_PCIMEM_BASE + GEMINI_PCIMEM_SIZE - 1,
		M_DEVBUF, NULL, 0, EX_NOWAIT);

	pci_configure_bus(pc, ioext, memext, NULL, 0, arm_dcache_align);

	gemini_pci_conf_write(sc, 0, GEMINI_PCI_CFG_REG_MEM1,
		PCI_CFG_REG_MEM_BASE((GEMINI_DRAM_BASE + (GEMINI_BUSBASE * 1024 * 1024)))
		| gemini_pci_cfg_reg_mem_size(MEMSIZE * 1024 * 1024));

	extent_destroy(ioext);
	extent_destroy(memext);
#endif
}

void
pci_conf_interrupt(pci_chipset_tag_t pc, int a, int b, int c, int d, int *p)
{
}

int
gemini_pci_conf_hook(pci_chipset_tag_t pc, int bus, int device, int function, pcireg_t id)
{
	int rv;

	rv = PCI_CONF_ALL;

	return rv;
}

void
gemini_pci_attach_hook(struct device *parent, struct device *self,
	struct pcibus_attach_args *pba)
{
	/* Nothing to do. */
}

int
gemini_pci_bus_maxdevs(void *v, int busno)
{
	return (32);
}

pcitag_t
gemini_pci_make_tag(void *v, int b, int d, int f)
{
	return ((b << 16) | (d << 11) | (f << 8));
}

void
gemini_pci_decompose_tag(void *v, pcitag_t tag, int *bp, int *dp, int *fp)
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
gemini_pci_conf_setup(struct obio_softc *sc, pcitag_t tag, int offset,
	struct pciconf_state *ps)
{
	gemini_pci_decompose_tag(sc, tag, &ps->ps_b, &ps->ps_d, &ps->ps_f);

	ps->ps_addr_val =
		  PCI_CFG_CMD_ENB
		| PCI_CFG_CMD_BUSn(ps->ps_b)
		| PCI_CFG_CMD_DEVn(ps->ps_d)
		| PCI_CFG_CMD_FUNCn(ps->ps_f)
		| PCI_CFG_CMD_REGn(offset);

	return (0);
}

pcireg_t
gemini_pci_conf_read(void *v, pcitag_t tag, int offset)
{
	struct obio_softc *sc = v;
	struct pciconf_state ps;
	vaddr_t va;
	pcireg_t rv;
	u_int s;

	if (gemini_pci_conf_setup(sc, tag, offset, &ps))
		return ((pcireg_t) -1);

	PCI_CONF_LOCK(s);

	if (sc->sc_pci_chipset.pc_cfg_cmd != ps.ps_addr_val) {
		sc->sc_pci_chipset.pc_cfg_cmd = ps.ps_addr_val;
		bus_space_write_4(sc->sc_iot, sc->sc_pcicfg_ioh,
			GEMINI_PCI_CFG_CMD, ps.ps_addr_val);
	}

	va = (vaddr_t) bus_space_vaddr(sc->sc_iot, sc->sc_pcicfg_ioh);
	if (badaddr_read((void *) (va + GEMINI_PCI_CFG_DATA), sizeof(rv), &rv)) {
		/*
		 * XXX Clear the Master Abort 
		 */
#if 1
		printf("conf_read: %d/%d/%d bad address\n",
			ps.ps_b, ps.ps_d, ps.ps_f);
#endif
		rv = (pcireg_t) -1;
	}

	PCI_CONF_UNLOCK(s);

	return (rv);
}

void
gemini_pci_conf_write(void *v, pcitag_t tag, int offset, pcireg_t val)
{
	struct obio_softc *sc = v;
	struct pciconf_state ps;
	u_int s;

	if (gemini_pci_conf_setup(sc, tag, offset, &ps))
		return;

	PCI_CONF_LOCK(s);

	if (sc->sc_pci_chipset.pc_cfg_cmd != ps.ps_addr_val) {
		sc->sc_pci_chipset.pc_cfg_cmd = ps.ps_addr_val;
		bus_space_write_4(sc->sc_iot, sc->sc_pcicfg_ioh,
			GEMINI_PCI_CFG_CMD, ps.ps_addr_val);
	}

	bus_space_write_4(sc->sc_iot, sc->sc_pcicfg_ioh,
		GEMINI_PCI_CFG_DATA, val);

	PCI_CONF_UNLOCK(s);
}

int
gemini_pci_intr_map(struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	int irq;

	irq = 8;

	*ihp = irq;
	return 0;
}

const char *
gemini_pci_intr_string(void *v, pci_intr_handle_t ih)
{
	const char *name = "pci";

	return (name);
}

const struct evcnt *
gemini_pci_intr_evcnt(void *v, pci_intr_handle_t ih)
{
	return NULL;
}

void *
gemini_pci_intr_establish(void *v, pci_intr_handle_t pci_ih, int ipl,
	int (*func)(void *), void *arg)
{
	pcireg_t r;
	void *ih=NULL;
	int irq;
	void *cookie;

	irq = (int)pci_ih;

	r = gemini_pci_conf_read(v, 0, GEMINI_PCI_CFG_REG_CTL2); 
	r |= CFG_REG_CTL2_INTMASK_INT_ABCD;
	gemini_pci_conf_write(v, 0, GEMINI_PCI_CFG_REG_CTL2, r);

	if (gemini_pci_intrq_empty())
		ih = intr_establish(irq, ipl, IST_LEVEL_HIGH,
			gemini_pci_intr_handler, v);

	cookie = gemini_pci_intrq_insert(ih, func, arg);
	if (cookie == NULL) {
		if (gemini_pci_intrq_empty())
			intr_disestablish(ih);
	}

	return cookie;
}

void
gemini_pci_intr_disestablish(void *v, void *cookie)
{
	pcireg_t r;
	struct gemini_pci_intrq *iqp = (struct gemini_pci_intrq *)cookie;
	void *ih = iqp->iq_ih;

	gemini_pci_intrq_remove(cookie);
	if (gemini_pci_intrq_empty()) {
		r = gemini_pci_conf_read(v, 0, GEMINI_PCI_CFG_REG_CTL2); 
		r &= ~CFG_REG_CTL2_INTMASK_INT_ABCD;
		gemini_pci_conf_write(v, 0, GEMINI_PCI_CFG_REG_CTL2, r);
		intr_disestablish(ih);
	}
}

int
gemini_pci_intr_handler(void *v)
{
	pcireg_t r;
	int rv;

	/*
	 * dispatch PCI device interrupt handlers
	 */
	rv = gemini_pci_intrq_dispatch();

	/*
	 * ack Gemini PCI interrupts
	 */
	r = gemini_pci_conf_read(v, 0, GEMINI_PCI_CFG_REG_CTL2); 
	gemini_pci_conf_write(v, 0, GEMINI_PCI_CFG_REG_CTL2, r);

	return rv;
}

