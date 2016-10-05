/* $NetBSD: tegra_pcie.c,v 1.2.2.4 2016/10/05 20:55:25 skrll Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tegra_pcie.c,v 1.2.2.4 2016/10/05 20:55:25 skrll Exp $");

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

#include <dev/fdt/fdtvar.h>

static int	tegra_pcie_match(device_t, cfdata_t, void *);
static void	tegra_pcie_attach(device_t, device_t, void *);

#define TEGRA_PCIE_NBUS 256
#define TEGRA_PCIE_ECFB (1<<(12 - 8))	/* extended conf frags per bus */

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
	bus_space_handle_t	sc_bsh_rpconf;
	int			sc_phandle;

	struct arm32_pci_chipset sc_pc;

	void			*sc_ih;

	kmutex_t		sc_lock;

	TAILQ_HEAD(, tegra_pcie_ih) sc_intrs;
	u_int			sc_intrgen;

	bus_space_handle_t	sc_bsh_extc[TEGRA_PCIE_NBUS-1][TEGRA_PCIE_ECFB];
};

static int	tegra_pcie_intr(void *);
static void	tegra_pcie_init(pci_chipset_tag_t, void *);
static void	tegra_pcie_enable(struct tegra_pcie_softc *);
static void	tegra_pcie_setup(struct tegra_pcie_softc * const);
static void	tegra_pcie_conf_frag_map(struct tegra_pcie_softc * const,
					 uint, uint);
static void	tegra_pcie_conf_map_bus(struct tegra_pcie_softc * const, uint);
static void	tegra_pcie_conf_map_buses(struct tegra_pcie_softc * const);

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
	const char * const compatible[] = { "nvidia,tegra124-pcie", NULL };
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
tegra_pcie_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_pcie_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	struct extent *ioext, *memext, *pmemext;
	struct pcibus_attach_args pba;
	bus_addr_t afi_addr, cs_addr;
	bus_size_t afi_size, cs_size;
	char intrstr[128];
	int error;

	if (fdtbus_get_reg(faa->faa_phandle, 1, &afi_addr, &afi_size) != 0) {
		aprint_error(": couldn't get afi registers\n");
		return;
	}
#if notyet
	if (fdtbus_get_reg(faa->faa_phandle, 2, &cs_addr, &cs_size) != 0) {
		aprint_error(": couldn't get cs registers\n");
		return;
	}
#else
	cs_addr = TEGRA_PCIE_RPCONF_BASE;
	cs_size = TEGRA_PCIE_RPCONF_SIZE;
#endif

	sc->sc_dev = self;
	sc->sc_dmat = faa->faa_dmat;
	sc->sc_bst = faa->faa_bst;
	sc->sc_phandle = faa->faa_phandle;
	error = bus_space_map(sc->sc_bst, afi_addr, afi_size, 0,
	    &sc->sc_bsh_afi);
	if (error) {
		aprint_error(": couldn't map afi registers: %d\n", error);
		return;
	}
	error = bus_space_map(sc->sc_bst, cs_addr, cs_size, 0,
	    &sc->sc_bsh_rpconf);
	if (error) {
		aprint_error(": couldn't map cs registers: %d\n", error);
		return;
	}

	tegra_pcie_conf_map_buses(sc);

	TAILQ_INIT(&sc->sc_intrs);
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);

	aprint_naive("\n");
	aprint_normal(": PCIE\n");

	if (!fdtbus_intr_str(faa->faa_phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	sc->sc_ih = fdtbus_intr_establish(faa->faa_phandle, 0, IPL_VM, 0,
	    tegra_pcie_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	tegra_pcie_setup(sc);

	tegra_pcie_init(&sc->sc_pc, sc);

	ioext = extent_create("pciio", TEGRA_PCIE_IO_BASE,
	    TEGRA_PCIE_IO_BASE + TEGRA_PCIE_IO_SIZE - 1,
	    NULL, 0, EX_NOWAIT);
	memext = extent_create("pcimem", TEGRA_PCIE_MEM_BASE,
	    TEGRA_PCIE_MEM_BASE + TEGRA_PCIE_MEM_SIZE - 1,
	    NULL, 0, EX_NOWAIT);
	pmemext = extent_create("pcipmem", TEGRA_PCIE_PMEM_BASE,
	    TEGRA_PCIE_PMEM_BASE + TEGRA_PCIE_PMEM_SIZE - 1,
	    NULL, 0, EX_NOWAIT);

	error = pci_configure_bus(&sc->sc_pc, ioext, memext, pmemext, 0,
	    arm_dcache_align);

	extent_destroy(ioext);
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
			PCI_FLAGS_MEM_OKAY |
			PCI_FLAGS_IO_OKAY;
	pba.pba_iot = sc->sc_bst;
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
	int rv;

	const uint32_t code = bus_space_read_4(sc->sc_bst, sc->sc_bsh_afi,
	    AFI_INTR_CODE_REG);
	const uint32_t sig = bus_space_read_4(sc->sc_bst, sc->sc_bsh_afi,
	    AFI_INTR_SIGNATURE_REG);

	switch (__SHIFTOUT(code, AFI_INTR_CODE_INT_CODE)) {
	case AFI_INTR_CODE_SM_MSG:
		rv = tegra_pcie_legacy_intr(sc);
		break;
	default:
		device_printf(sc->sc_dev, "intr: code %#x sig %#x\n",
		    code, sig);
		rv = 1;
		break;
	}

	bus_space_write_4(sc->sc_bst, sc->sc_bsh_afi, AFI_INTR_CODE_REG, 0);

	return rv;
}

static void
tegra_pcie_setup(struct tegra_pcie_softc * const sc)
{
	size_t i;

	/*
	 * Map PCI address spaces into ARM address space via
	 * HyperTransport-like "FPCI".
	 */
	static const struct { uint32_t size, base, fpci; } pcie_init_table[] = {
		/*
		 * === BEWARE ===
		 *
		 * We depend on our TEGRA_PCIE_IO window overlaping the
		 * TEGRA_PCIE_A1 window to allow us to use the same
		 * bus_space_tag for both PCI IO and Memory spaces.
		 *
		 * 0xfdfc000000-0xfdfdffffff is the FPCI/HyperTransport
		 * mapping for 0x0000000-0x1ffffff of PCI IO space.
		 */
		{ TEGRA_PCIE_IO_SIZE >> 12, TEGRA_PCIE_IO_BASE,
		  (0xfdfc000000 + TEGRA_PCIE_IO_BASE) >> 8 | 0, },

		/* HyperTransport Technology Type 1 Address Format */
		{ TEGRA_PCIE_CONF_SIZE >> 12, TEGRA_PCIE_CONF_BASE,
		  0xfdff000000 >> 8 | 0, },

		/* 1:1 MMIO mapping */
		{ TEGRA_PCIE_MEM_SIZE >> 12, TEGRA_PCIE_MEM_BASE,
		  TEGRA_PCIE_MEM_BASE >> 8 | 1, },

		/* Extended HyperTransport Technology Type 1 Address Format */
		{ TEGRA_PCIE_EXTC_SIZE >> 12, TEGRA_PCIE_EXTC_BASE,
		  0xfe10000000 >> 8 | 0, },

		/* 1:1 prefetchable MMIO mapping */
		{ TEGRA_PCIE_PMEM_SIZE >> 12, TEGRA_PCIE_PMEM_BASE,
		  TEGRA_PCIE_PMEM_BASE >> 8 | 1, },
	};

	for (i = 0; i < AFI_AXI_NBAR; i++) {
		bus_space_write_4(sc->sc_bst, sc->sc_bsh_afi,
		    AFI_AXI_BARi_SZ(i), 0);
		bus_space_write_4(sc->sc_bst, sc->sc_bsh_afi,
		    AFI_AXI_BARi_START(i), 0);
		bus_space_write_4(sc->sc_bst, sc->sc_bsh_afi,
		    AFI_FPCI_BARi(i), 0);
	}

	for (i = 0; i < __arraycount(pcie_init_table); i++) {
		bus_space_write_4(sc->sc_bst, sc->sc_bsh_afi,
		    AFI_AXI_BARi_START(i), pcie_init_table[i].base);
		bus_space_write_4(sc->sc_bst, sc->sc_bsh_afi,
		    AFI_FPCI_BARi(i), pcie_init_table[i].fpci);
		bus_space_write_4(sc->sc_bst, sc->sc_bsh_afi,
		    AFI_AXI_BARi_SZ(i), pcie_init_table[i].size);
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

static void
tegra_pcie_conf_frag_map(struct tegra_pcie_softc * const sc, uint bus,
    uint frg)
{
	bus_addr_t a;

	KASSERT(bus >= 1);
	KASSERT(bus < TEGRA_PCIE_NBUS);
	KASSERT(frg < TEGRA_PCIE_ECFB);

	if (sc->sc_bsh_extc[bus-1][frg] != 0) {
		device_printf(sc->sc_dev, "bus %u fragment %#x already "
		    "mapped\n", bus, frg);
		return;
	}

	a = TEGRA_PCIE_EXTC_BASE + (bus << 16) + (frg << 24);
	if (bus_space_map(sc->sc_bst, a, 1 << 16, 0,
	    &sc->sc_bsh_extc[bus-1][frg]) != 0)
		device_printf(sc->sc_dev, "couldn't map PCIE "
		    "configuration for bus %u fragment %#x", bus, frg);
}

/* map non-non-extended configuration space for full bus range */
static void
tegra_pcie_conf_map_bus(struct tegra_pcie_softc * const sc, uint bus)
{
	uint i;

	for (i = 1; i < TEGRA_PCIE_ECFB; i++) {
		tegra_pcie_conf_frag_map(sc, bus, i);
	}
}

/* map non-extended configuration space for full bus range */
static void
tegra_pcie_conf_map_buses(struct tegra_pcie_softc * const sc)
{
	uint b;

	for (b = 1; b < TEGRA_PCIE_NBUS; b++) {
		tegra_pcie_conf_frag_map(sc, b, 0);
	}
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
	const pci_chipset_tag_t pc = pba->pba_pc;
	struct tegra_pcie_softc * const sc = pc->pc_conf_v;

	if (pba->pba_bus >= 1) {
		tegra_pcie_conf_map_bus(sc, pba->pba_bus);
	}
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

	if (b >= TEGRA_PCIE_NBUS)
		return (pcireg_t) -1;

	if (b == 0) {
		if (d >= 2 || f != 0)
			return (pcireg_t) -1;
		reg = d * 0x1000 + offset;
		bsh = sc->sc_bsh_rpconf;
	} else {
		reg = (d << 11) | (f << 8) | (offset & 0xff);
		bsh = sc->sc_bsh_extc[b-1][(offset >> 8) & 0xf];
		if (bsh == 0)
			return (pcireg_t) -1;
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

	if (b >= TEGRA_PCIE_NBUS)
		return;

	if (b == 0) {
		if (d >= 2 || f != 0)
			return;
		reg = d * 0x1000 + offset;
		bsh = sc->sc_bsh_rpconf;
	} else {
		reg = (d << 11) | (f << 8) | (offset & 0xff);
		bsh = sc->sc_bsh_extc[b-1][(offset >> 8) & 0xf];
		if (bsh == 0)
			return;
	}

	bus_space_write_4(sc->sc_bst, bsh, reg, val);
}

static int
tegra_pcie_conf_hook(void *v, int b, int d, int f, pcireg_t id)
{
	return PCI_CONF_DEFAULT & ~PCI_CONF_ENABLE_BM;
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

	if (!fdtbus_intr_str(sc->sc_phandle, 0, buf, len))
		return NULL;

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
