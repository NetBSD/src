/* $NetBSD: tegra_pcie.c,v 1.4 2015/10/15 09:06:04 jmcneill Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "locators.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tegra_pcie.c,v 1.4 2015/10/15 09:06:04 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/extent.h>
#include <sys/queue.h>
#include <sys/mutex.h>
#include <sys/kmem.h>

#include <arm/cpufunc.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_pciereg.h>
#include <arm/nvidia/tegra_var.h>

static int	tegra_pcie_match(device_t, cfdata_t, void *);
static void	tegra_pcie_attach(device_t, device_t, void *);

struct tegra_pcie_ih {
	int			(*ih_callback)(void *);
	void			*ih_arg;
	int			ih_ipl;
	TAILQ_ENTRY(tegra_pcie_ih) ih_entry;
};

struct tegra_pcie_softc {
	device_t		sc_dev;
	bus_dma_tag_t		sc_dmat;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh_afi;
	bus_space_handle_t	sc_bsh_a1;
	bus_space_handle_t	sc_bsh_a2;
	int			sc_intr;

	struct arm32_pci_chipset sc_pc;

	void			*sc_ih;

	kmutex_t		sc_lock;

	TAILQ_HEAD(, tegra_pcie_ih) sc_intrs;
	u_int			sc_intrgen;
};

static int	tegra_pcie_intr(void *);
static void	tegra_pcie_init(pci_chipset_tag_t, void *);
static void	tegra_pcie_enable(struct tegra_pcie_softc *);

static void	tegra_pcie_attach_hook(device_t, device_t,
				       struct pcibus_attach_args *);
static int	tegra_pcie_bus_maxdevs(void *, int);
static pcitag_t	tegra_pcie_make_tag(void *, int, int, int);
static void	tegra_pcie_decompose_tag(void *, pcitag_t, int *, int *, int *);
static pcireg_t	tegra_pcie_conf_read(void *, pcitag_t, int);
static void	tegra_pcie_conf_write(void *, pcitag_t, int, pcireg_t);
static int	tegra_pcie_conf_hook(void *, int, int, int, pcireg_t);
static void	tegra_pcie_conf_interrupt(void *, int, int, int, int, int *);

static int	tegra_pcie_intr_map(const struct pci_attach_args *,
				    pci_intr_handle_t *);
static const char *tegra_pcie_intr_string(void *, pci_intr_handle_t,
					  char *, size_t);
const struct evcnt *tegra_pcie_intr_evcnt(void *, pci_intr_handle_t);
static void *	tegra_pcie_intr_establish(void *, pci_intr_handle_t,
					 int, int (*)(void *), void *);
static void	tegra_pcie_intr_disestablish(void *, void *);

CFATTACH_DECL_NEW(tegra_pcie, sizeof(struct tegra_pcie_softc),
	tegra_pcie_match, tegra_pcie_attach, NULL, NULL);

static int
tegra_pcie_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
tegra_pcie_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_pcie_softc * const sc = device_private(self);
	struct tegraio_attach_args * const tio = aux;
	const struct tegra_locators * const loc = &tio->tio_loc;
	struct extent *memext, *pmemext;
	struct pcibus_attach_args pba;
	int error;

	sc->sc_dev = self;
#if notyet
	sc->sc_dmat = tio->tio_coherent_dmat;
#else
	sc->sc_dmat = tio->tio_dmat;
#endif
	sc->sc_bst = tio->tio_bst;
	sc->sc_intr = loc->loc_intr;
	if (bus_space_map(sc->sc_bst, TEGRA_PCIE_AFI_BASE, TEGRA_PCIE_AFI_SIZE,
	    0, &sc->sc_bsh_afi) != 0)
		panic("couldn't map PCIE AFI");
	if (bus_space_map(sc->sc_bst, TEGRA_PCIE_A1_BASE, TEGRA_PCIE_A1_SIZE,
	    0, &sc->sc_bsh_a1) != 0)
		panic("couldn't map PCIE A1");
	if (bus_space_map(sc->sc_bst, TEGRA_PCIE_A2_BASE, TEGRA_PCIE_A2_SIZE,
	    0, &sc->sc_bsh_a2) != 0)
		panic("couldn't map PCIE A2");

	TAILQ_INIT(&sc->sc_intrs);
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);

	aprint_naive("\n");
	aprint_normal(": PCIE\n");

	sc->sc_ih = intr_establish(loc->loc_intr, IPL_VM, IST_LEVEL,
	    tegra_pcie_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		    loc->loc_intr);
		return;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n", loc->loc_intr);

	tegra_pcie_init(&sc->sc_pc, sc);

	memext = extent_create("pcimem", TEGRA_PCIE_MEM_BASE,
	    TEGRA_PCIE_MEM_BASE + TEGRA_PCIE_MEM_SIZE - 1,
	    NULL, 0, EX_NOWAIT);
	pmemext = extent_create("pcipmem", TEGRA_PCIE_PMEM_BASE,
	    TEGRA_PCIE_PMEM_BASE + TEGRA_PCIE_PMEM_SIZE - 1,
	    NULL, 0, EX_NOWAIT);

	error = pci_configure_bus(&sc->sc_pc, NULL, memext, pmemext, 0,
	    arm_dcache_align);

	extent_destroy(memext);
	extent_destroy(pmemext);

	if (error) {
		aprint_error_dev(self, "configuration failed (%d)\n",
		    error);
		return;
	}

	tegra_pcie_enable(sc);

	memset(&pba, 0, sizeof(pba));
	pba.pba_flags = PCI_FLAGS_MRL_OKAY |
			PCI_FLAGS_MRM_OKAY |
			PCI_FLAGS_MWI_OKAY |
			PCI_FLAGS_MEM_OKAY;
	pba.pba_memt = sc->sc_bst;
	pba.pba_dmat = sc->sc_dmat;
	pba.pba_pc = &sc->sc_pc;
	pba.pba_bus = 0;

	config_found_ia(self, "pcibus", &pba, pcibusprint);
}

static int
tegra_pcie_legacy_intr(struct tegra_pcie_softc *sc)
{
	const uint32_t msg = bus_space_read_4(sc->sc_bst, sc->sc_bsh_afi,
	    AFI_MSG_REG);
	struct tegra_pcie_ih *pcie_ih;
	int rv = 0;

	if (msg & (AFI_MSG_INT0|AFI_MSG_INT1)) {
		mutex_enter(&sc->sc_lock);
		const u_int lastgen = sc->sc_intrgen;
		TAILQ_FOREACH(pcie_ih, &sc->sc_intrs, ih_entry) {
			int (*callback)(void *) = pcie_ih->ih_callback;
			void *arg = pcie_ih->ih_arg;
			mutex_exit(&sc->sc_lock);
			rv += callback(arg);
			mutex_enter(&sc->sc_lock);
			if (lastgen != sc->sc_intrgen)
				break;
		}
		mutex_exit(&sc->sc_lock);
	} else if (msg & (AFI_MSG_PM_PME0|AFI_MSG_PM_PME1)) {
		device_printf(sc->sc_dev, "PM PME message; AFI_MSG=%08x\n",
		    msg);
	} else {
		bus_space_write_4(sc->sc_bst, sc->sc_bsh_afi, AFI_MSG_REG, msg);
		rv = 1;
	}

	return rv;
}

static int
tegra_pcie_intr(void *priv)
{
	struct tegra_pcie_softc *sc = priv;

	const uint32_t code = bus_space_read_4(sc->sc_bst, sc->sc_bsh_afi,
	    AFI_INTR_CODE_REG);
	const uint32_t sig = bus_space_read_4(sc->sc_bst, sc->sc_bsh_afi,
	    AFI_INTR_SIGNATURE_REG);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh_afi, AFI_INTR_CODE_REG, 0);

	switch (__SHIFTOUT(code, AFI_INTR_CODE_INT_CODE)) {
	case AFI_INTR_CODE_SM_MSG:
		return tegra_pcie_legacy_intr(sc);
	default:
		device_printf(sc->sc_dev, "intr: code %#x sig %#x\n",
		    code, sig);
		return 1;
	}
}

static void
tegra_pcie_enable(struct tegra_pcie_softc *sc)
{
	/* disable MSI */
	bus_space_write_4(sc->sc_bst, sc->sc_bsh_afi,
	    AFI_MSI_BAR_SZ_REG, 0);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh_afi,
	    AFI_MSI_FPCI_BAR_ST_REG, 0);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh_afi,
	    AFI_MSI_AXI_BAR_ST_REG, 0);

	bus_space_write_4(sc->sc_bst, sc->sc_bsh_afi,
	    AFI_SM_INTR_ENABLE_REG, 0xffffffff);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh_afi,
	    AFI_AFI_INTR_ENABLE_REG, 0);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh_afi, AFI_INTR_CODE_REG, 0);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh_afi,
	    AFI_INTR_MASK_REG, AFI_INTR_MASK_INT);
}

void
tegra_pcie_init(pci_chipset_tag_t pc, void *priv)
{
	pc->pc_conf_v = priv;
	pc->pc_attach_hook = tegra_pcie_attach_hook;
	pc->pc_bus_maxdevs = tegra_pcie_bus_maxdevs;
	pc->pc_make_tag = tegra_pcie_make_tag;
	pc->pc_decompose_tag = tegra_pcie_decompose_tag;
	pc->pc_conf_read = tegra_pcie_conf_read;
	pc->pc_conf_write = tegra_pcie_conf_write;
	pc->pc_conf_hook = tegra_pcie_conf_hook;
	pc->pc_conf_interrupt = tegra_pcie_conf_interrupt;

	pc->pc_intr_v = priv;
	pc->pc_intr_map = tegra_pcie_intr_map;
	pc->pc_intr_string = tegra_pcie_intr_string;
	pc->pc_intr_evcnt = tegra_pcie_intr_evcnt;
	pc->pc_intr_establish = tegra_pcie_intr_establish;
	pc->pc_intr_disestablish = tegra_pcie_intr_disestablish;
}

static void
tegra_pcie_attach_hook(device_t parent, device_t self,
    struct pcibus_attach_args *pba)
{
}

static int
tegra_pcie_bus_maxdevs(void *v, int busno)
{
	return busno == 0 ? 2 : 32;
}

static pcitag_t
tegra_pcie_make_tag(void *v, int b, int d, int f)
{
	return (b << 16) | (d << 11) | (f << 8);
}

static void
tegra_pcie_decompose_tag(void *v, pcitag_t tag, int *bp, int *dp, int *fp)
{
	if (bp)
		*bp = (tag >> 16) & 0xff;
	if (dp)
		*dp = (tag >> 11) & 0x1f;
	if (fp)
		*fp = (tag >> 8) & 0x7;
}

static pcireg_t
tegra_pcie_conf_read(void *v, pcitag_t tag, int offset)
{
	struct tegra_pcie_softc *sc = v;
	bus_space_handle_t bsh;
	int b, d, f;
	u_int reg;

	if ((unsigned int)offset >= PCI_EXTCONF_SIZE)
		return (pcireg_t) -1;

	tegra_pcie_decompose_tag(v, tag, &b, &d, &f);

	if (b == 0) {
		reg = d * 0x1000 + offset;
		bsh = sc->sc_bsh_a1;
	} else {
		reg = tag | offset;
		bsh = sc->sc_bsh_a2;
	}

	return bus_space_read_4(sc->sc_bst, bsh, reg);
}

static void
tegra_pcie_conf_write(void *v, pcitag_t tag, int offset, pcireg_t val)
{
	struct tegra_pcie_softc *sc = v;
	bus_space_handle_t bsh;
	int b, d, f;
	u_int reg;

	if ((unsigned int)offset >= PCI_EXTCONF_SIZE)
		return;

	tegra_pcie_decompose_tag(v, tag, &b, &d, &f);

	if (b == 0) {
		reg = d * 0x1000 + offset;
		bsh = sc->sc_bsh_a1;
	} else {
		reg = tag | offset;
		bsh = sc->sc_bsh_a2;
	}

	bus_space_write_4(sc->sc_bst, bsh, reg, val);
}

static int
tegra_pcie_conf_hook(void *v, int b, int d, int f, pcireg_t id)
{
	return PCI_CONF_ENABLE_MEM | PCI_CONF_MAP_MEM | PCI_CONF_ENABLE_BM;
}

static void
tegra_pcie_conf_interrupt(void *v, int bus, int dev, int ipin, int swiz,
    int *ilinep)
{
	*ilinep = 5;
}

static int
tegra_pcie_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ih)
{
	if (pa->pa_intrpin == 0)
		return EINVAL;
	*ih = pa->pa_intrpin;
	return 0;
}
				    
static const char *
tegra_pcie_intr_string(void *v, pci_intr_handle_t ih, char *buf, size_t len)
{
	struct tegra_pcie_softc *sc = v;

	if (ih == PCI_INTERRUPT_PIN_NONE)
		return NULL;

	snprintf(buf, len, "irq %d", sc->sc_intr);
	return buf;
}

const struct evcnt *
tegra_pcie_intr_evcnt(void *v, pci_intr_handle_t ih)
{
	return NULL;
}

static void *
tegra_pcie_intr_establish(void *v, pci_intr_handle_t ih, int ipl,
    int (*callback)(void *), void *arg)
{
	struct tegra_pcie_softc *sc = v;
	struct tegra_pcie_ih *pcie_ih;

	if (ih == 0)
		return NULL;

	pcie_ih = kmem_alloc(sizeof(*pcie_ih), KM_SLEEP);
	pcie_ih->ih_callback = callback;
	pcie_ih->ih_arg = arg;
	pcie_ih->ih_ipl = ipl;

	mutex_enter(&sc->sc_lock);
	TAILQ_INSERT_TAIL(&sc->sc_intrs, pcie_ih, ih_entry);
	sc->sc_intrgen++;
	mutex_exit(&sc->sc_lock);

	return pcie_ih;
}

static void
tegra_pcie_intr_disestablish(void *v, void *vih)
{
	struct tegra_pcie_softc *sc = v;
	struct tegra_pcie_ih *pcie_ih = vih;

	mutex_enter(&sc->sc_lock);
	TAILQ_REMOVE(&sc->sc_intrs, pcie_ih, ih_entry);
	mutex_exit(&sc->sc_lock);

	kmem_free(pcie_ih, sizeof(*pcie_ih));
}
