/* $NetBSD: apple_pcie.c,v 1.6 2022/04/27 08:03:06 skrll Exp $ */
/*	$OpenBSD: aplpcie.c,v 1.13 2022/04/06 18:59:26 naddy Exp $	*/

/*-
 * Copyright (c) 2021 Jared McNeill <jmcneill@invisible.ca>
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

/*
 * Copyright (c) 2021 Mark Kettenis <kettenis@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: apple_pcie.c,v 1.6 2022/04/27 08:03:06 skrll Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/bitops.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>

#include <dev/fdt/fdtvar.h>

#include <arm/pci/pci_msi_machdep.h>
#include <arm/fdt/pcihost_fdtvar.h>

#define PCIE_CORE_LANE_CONF(port)	(0x84000 + (port) * 0x4000)
#define  PCIE_CORE_LANE_CONF_REFCLK0REQ	__BIT(0)
#define  PCIE_CORE_LANE_CONF_REFCLK1REQ	__BIT(1)
#define  PCIE_CORE_LANE_CONF_REFCLK0ACK	__BIT(2)
#define  PCIE_CORE_LANE_CONF_REFCLK1ACK	__BIT(3)
#define  PCIE_CORE_LANE_CONF_REFCLK0EN	__BIT(9)
#define  PCIE_CORE_LANE_CONF_REFCLK1EN	__BIT(10)
#define PCIE_CORE_LANE_CTRL(port)	(0x84004 + (port) * 0x4000)
#define  PCIE_CORE_LANE_CTRL_CFGACC	__BIT(15)

#define PCIE_PORT_LTSSM_CTRL		0x0080
#define  PCIE_PORT_LTSSM_CTRL_START	__BIT(0)
#define	PCIE_PORT_MSI_CTRL		0x0124
#define	 PCIE_PORT_MSI_CTRL_EN		__BIT(0)
#define	 PCIE_PORT_MSI_CTRL_32		__SHIFTIN(5U, __BITS(7,4))
#define	PCIE_PORT_MSI_REMAP		0x0128
#define	PCIE_PORT_MSI_DOORBELL		0x0168
#define PCIE_PORT_LINK_STAT		0x0208
#define  PCIE_PORT_LINK_STAT_UP		__BIT(0)
#define PCIE_PORT_APPCLK		0x0800
#define  PCIE_PORT_APPCLK_EN		__BIT(0)
#define  PCIE_PORT_APPCLK_CGDIS		__BIT(8)
#define PCIE_PORT_STAT			0x0804
#define  PCIE_PORT_STAT_READY		__BIT(0)
#define PCIE_PORT_REFCLK		0x0810
#define  PCIE_PORT_REFCLK_EN		__BIT(0)
#define  PCIE_PORT_REFCLK_CGDIS		__BIT(8)
#define PCIE_PORT_PERST			0x0814
#define  PCIE_PORT_PERST_DIS		__BIT(0)

extern struct bus_space arm_generic_bs_tag;

struct apple_pcie_softc {
	struct pcihost_softc	sc_pcihost;

	bus_space_tag_t		sc_rc_bst;
	bus_space_handle_t	sc_rc_bsh;

	int			sc_phandle;
	struct arm_pci_msi	sc_msi;
	u_int			sc_msi_start;
	u_int			sc_nmsi;
	struct pci_attach_args	**sc_msi_pa;
	void			**sc_msi_ih;
	uint64_t		sc_msi_addr;
};

static int	apple_pcie_match(device_t, cfdata_t, void *);
static void	apple_pcie_attach(device_t, device_t, void *);

static void	apple_pcie_attach_hook(device_t, device_t,
				       struct pcibus_attach_args *);
static int	apple_pcie_msi_init(struct apple_pcie_softc *);

CFATTACH_DECL_NEW(apple_pcie, sizeof(struct apple_pcie_softc),
	apple_pcie_match, apple_pcie_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "apple,pcie" },
	DEVICE_COMPAT_EOL
};

#define RREAD4(sc, reg)						\
    (bus_space_read_4((sc)->sc_rc_bst, (sc)->sc_rc_bsh, (reg)))
#define RWRITE4(sc, reg, val)					\
    bus_space_write_4((sc)->sc_rc_bst, (sc)->sc_rc_bsh, (reg), (val))
#define RSET4(sc, reg, bits)				\
    RWRITE4((sc), (reg), RREAD4((sc), (reg)) | (bits))
#define RCLR4(sc, reg, bits)				\
    RWRITE4((sc), (reg), RREAD4((sc), (reg)) & ~(bits))


static void
apple_pcie_setup_port(struct apple_pcie_softc *sc, int phandle)
{
	const bus_space_tag_t bst = sc->sc_pcihost.sc_bst;
	const device_t dev = sc->sc_pcihost.sc_dev;
	const int parent = sc->sc_pcihost.sc_phandle;
	char regname[sizeof("portX")];
	bus_space_handle_t bsh;
	bus_addr_t addr;
	bus_size_t size;
	int error;
	int timo;
	int len;

	const u_int *reg = fdtbus_get_prop(phandle, "reg", &len);
	if (len != 5 * sizeof(uint32_t)) {
		aprint_error(": couldn't get port number\n");
	}

	u_int portno = __SHIFTOUT(be32toh(reg[0]), __BITS(13,11));
	snprintf(regname, sizeof(regname), "port%u", portno);

	if (fdtbus_get_reg_byname(parent, regname, &addr, &size) != 0) {
		aprint_error(": couldn't get %s regs\n", regname);
		return;
	}
	error = bus_space_map(bst, addr, size, 0, &bsh);
	if (error != 0) {
		aprint_error(": couldn't map %s regs\n", regname);
		return;
	}

#define PREAD4(bst, bsh, reg)					\
    bus_space_read_4((bst), (bsh), (reg))
#define PWRITE4(bst, bsh, reg, val)				\
    bus_space_write_4((bst), (bsh), (reg), (val))
#define PSET4(bst, bsh, reg, bits)				\
    PWRITE4((bst), (bsh), (reg), PREAD4((bst), (bsh), (reg)) | (bits))
#define PCLR4(bst, bsh, reg, bits)				\
    PWRITE4((bst), (bsh), (reg), PREAD4((bst), (bsh), (reg)) & ~(bits))

	/* Doorbell address must be below 4GB */
	KASSERT((sc->sc_msi_addr & ~0xffffffffUL) == 0);

	int pwren_gpiolen, reset_gpiolen;

	pwren_gpiolen = OF_getproplen(phandle, "pwren-gpios");
	reset_gpiolen = OF_getproplen(phandle, "reset-gpios");
	if (reset_gpiolen <= 0)
		return;

	/*
	 * Set things up such that we can share the 32 available MSIs
	 * across all ports.
	 */
	PWRITE4(bst, bsh, PCIE_PORT_MSI_CTRL,
	    PCIE_PORT_MSI_CTRL_32 | PCIE_PORT_MSI_CTRL_EN);
	PWRITE4(bst, bsh, PCIE_PORT_MSI_REMAP, 0);
	PWRITE4(bst, bsh, PCIE_PORT_MSI_DOORBELL,
	    __SHIFTOUT(sc->sc_msi_addr, __BITS(31, 0)));

	/* Check if the link is already up. */
	uint32_t stat = PREAD4(bst, bsh, PCIE_PORT_LINK_STAT);
	if (stat & PCIE_PORT_LINK_STAT_UP) {
		aprint_debug_dev(dev, "link already up\n");
		return;
	}
	aprint_debug_dev(dev, "bringing link up\n");

	PSET4(bst, bsh, PCIE_PORT_APPCLK, PCIE_PORT_APPCLK_EN);

	struct fdtbus_gpio_pin *gpio_reset = fdtbus_gpio_acquire(phandle,
	    "reset-gpios", GPIO_PIN_OUTPUT);

        if (gpio_reset == NULL) {
		aprint_debug_dev(dev, "failed to get reset-gpios\n");
		return;
	}

	fdtbus_gpio_write(gpio_reset, 1);

	/* Power up the device if necessary. */
	if (pwren_gpiolen > 0) {
		struct fdtbus_gpio_pin *gpio_pwren = fdtbus_gpio_acquire(phandle,
		    "pwren-gpios", GPIO_PIN_OUTPUT);

		if (gpio_pwren == NULL) {
			aprint_debug_dev(dev, "failed to get pwren-gpios\n");
			return;
		}

		fdtbus_gpio_write(gpio_pwren, 1);
	}

	/* Setup Refclk. */
	RSET4(sc, PCIE_CORE_LANE_CTRL(portno), PCIE_CORE_LANE_CTRL_CFGACC);
	RSET4(sc, PCIE_CORE_LANE_CONF(portno), PCIE_CORE_LANE_CONF_REFCLK0REQ);
	for (timo = 500; timo > 0; timo--) {
		stat = RREAD4(sc, PCIE_CORE_LANE_CONF(portno));
		if (stat & PCIE_CORE_LANE_CONF_REFCLK0ACK)
			break;
		delay(100);
	}
	RSET4(sc, PCIE_CORE_LANE_CONF(portno), PCIE_CORE_LANE_CONF_REFCLK1REQ);
	for (timo = 500; timo > 0; timo--) {
		stat = RREAD4(sc, PCIE_CORE_LANE_CONF(portno));
		if (stat & PCIE_CORE_LANE_CONF_REFCLK1ACK)
			break;
		delay(100);
	}
	RCLR4(sc, PCIE_CORE_LANE_CTRL(portno), PCIE_CORE_LANE_CTRL_CFGACC);
	RSET4(sc, PCIE_CORE_LANE_CONF(portno),
	    PCIE_CORE_LANE_CONF_REFCLK0EN | PCIE_CORE_LANE_CONF_REFCLK1EN);
	PSET4(bst, bsh, PCIE_PORT_REFCLK, PCIE_PORT_REFCLK_EN);

	/*
	 * PERST# must remain asserted for at least 100us after the
	 * reference clock becomes stable.  But also has to remain
	 * active at least 100ms after power up.
	 */
	if (pwren_gpiolen > 0)
		delay(100000);
	else
		delay(100);

	/* Deassert PERST#. */
	PSET4(bst, bsh, PCIE_PORT_PERST, PCIE_PORT_PERST_DIS);
	fdtbus_gpio_write(gpio_reset, 0);

	for (timo = 2500; timo > 0; timo--) {
		stat = PREAD4(bst, bsh, PCIE_PORT_STAT);
		if (stat & PCIE_PORT_STAT_READY)
			break;
		delay(100);
	}
	if ((stat & PCIE_PORT_STAT_READY) == 0) {
		aprint_debug_dev(dev, "link up\n");
		return;
	}

	PCLR4(bst, bsh, PCIE_PORT_REFCLK, PCIE_PORT_REFCLK_CGDIS);
	PCLR4(bst, bsh, PCIE_PORT_APPCLK, PCIE_PORT_APPCLK_CGDIS);

	/* Bring up the link. */
	PWRITE4(bst, bsh, PCIE_PORT_LTSSM_CTRL, PCIE_PORT_LTSSM_CTRL_START);
	for (timo = 1000; timo > 0; timo--) {
		stat = PREAD4(bst, bsh, PCIE_PORT_LINK_STAT);
		if (stat & PCIE_PORT_LINK_STAT_UP)
			break;
		delay(100);
	}

#undef PREAD4
#undef PWRITE4
#undef PCLR4
#undef PSET4

	bus_space_unmap(bst, bsh, size);
}

static int
apple_pcie_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
apple_pcie_attach(device_t parent, device_t self, void *aux)
{
	struct apple_pcie_softc * const asc = device_private(self);
	struct pcihost_softc * const sc = &asc->sc_pcihost;
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t cs_addr, rc_addr;
	bus_size_t cs_size, rc_size;
	int error;

	if (fdtbus_get_reg_byname(phandle, "config", &cs_addr, &cs_size) != 0) {
		aprint_error(": couldn't get registers (%s)\n", "config");
		return;
	}

	if (fdtbus_get_reg_byname(phandle, "rc", &rc_addr, &rc_size) != 0) {
		aprint_error(": couldn't get registers (%s)\n", "rc");
		return;
	}

	sc->sc_dev = self;
	sc->sc_dmat = faa->faa_dmat;
	sc->sc_bst = asc->sc_rc_bst = faa->faa_bst;
	/*
	 * Create a new bus tag for PCIe devices that does not inherit the
	 * nonposted MMIO flag from the host controller.
	 */
	sc->sc_pci_bst = &arm_generic_bs_tag;
	sc->sc_phandle = phandle;
	error = bus_space_map(faa->faa_bst, cs_addr, cs_size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map registers (%s): %d\n", "config",
		    error);
		return;
	}
	error = bus_space_map(asc->sc_rc_bst, rc_addr, rc_size, 0,
	    &asc->sc_rc_bsh);
	if (error) {
		aprint_error(": couldn't map registers (%s): %d\n", "rc",
		    error);
		return;
	}
	sc->sc_type = PCIHOST_ECAM;

	if (apple_pcie_msi_init(asc) == 0) {
		sc->sc_pci_flags |= PCI_FLAGS_MSI_OKAY;
#if notyet
		sc->sc_pci_flags |= PCI_FLAGS_MSIX_OKAY;
#endif
	}

	aprint_naive("\n");
	aprint_normal(": Apple PCIe host controller\n");

	for (int node = OF_child(phandle); node; node = OF_peer(node))
		apple_pcie_setup_port(asc, node);

	/*
	 * Must wait at least 100ms after link training completes
	 * before sending a configuration request to a device
	 * immediately below a port.
	 */
	delay(100000);

	pcihost_init(&sc->sc_pc, sc);

	sc->sc_pc.pc_attach_hook = apple_pcie_attach_hook;
	pcihost_init2(sc);
}



static void
apple_pcie_attach_hook(device_t parent, device_t self,
    struct pcibus_attach_args *pba)
{
	struct apple_pcie_softc *sc = pba->pba_pc->pc_conf_v;
	const int phandle = sc->sc_pcihost.sc_phandle;
	bus_dma_tag_t dmat;

	KASSERT(device_is_a(sc->sc_pcihost.sc_dev, "applepcie"));

	/* XXX this should be per-device, not per-bus */
	const uint32_t rid = pba->pba_bus << 8;

	dmat = fdtbus_iommu_map_pci(phandle, rid, sc->sc_pcihost.sc_dmat);

	pba->pba_dmat = pba->pba_dmat64 = dmat;
}

static int
apple_pcie_msi_alloc_msi(struct apple_pcie_softc *sc, int count,
    const struct pci_attach_args *pa)
{
	struct pci_attach_args *new_pa;
	int msi, n;

	for (msi = 0; msi < sc->sc_nmsi; msi += n) {
		/* Look for first empty slot */
		if (sc->sc_msi_pa[msi] != NULL) {
			/* skip the used entry */
			n = 1;
			continue;
		}

		/* Now check that 'count' entries are also empty */
		for (n = 1; n < count && msi + n < sc->sc_nmsi; n++) {
			if (sc->sc_msi_pa[msi + n] != NULL) {
				break;
			}
		}
		/*
		 * If 'count' empty entries weren't found then the search
		 * continues.
		 */
		if (n != count)
			continue;
		for (n = 0; n < count; n++) {
			new_pa = kmem_alloc(sizeof(*new_pa), KM_SLEEP);
			memcpy(new_pa, pa, sizeof(*new_pa));
			sc->sc_msi_pa[msi + n] = new_pa;
		}

		return msi;
	}

	return -1;
}

static void
apple_pcie_msi_free_msi(struct apple_pcie_softc *sc, int msi)
{
	struct pci_attach_args *pa;

	pa = sc->sc_msi_pa[msi];
	sc->sc_msi_pa[msi] = NULL;

	if (pa != NULL) {
		kmem_free(pa, sizeof(*pa));
	}
}

static int
apple_pcie_msi_available_msi(struct apple_pcie_softc *sc)
{
	int msi, n;

	for (n = 0, msi = 0; msi < sc->sc_nmsi; msi++) {
		if (sc->sc_msi_pa[msi] == NULL) {
			n++;
		}
	}

	return n;
}

static void
apple_pcie_msi_msi_enable(struct apple_pcie_softc *sc, int msi, int count)
{
	const struct pci_attach_args *pa = sc->sc_msi_pa[msi];
	pci_chipset_tag_t pc = pa->pa_pc;
	pcitag_t tag = pa->pa_tag;
	pcireg_t ctl;
	int off;

	if (!pci_get_capability(pc, tag, PCI_CAP_MSI, &off, NULL))
		panic("apple_pcie_msi_msi_enable: device is not MSI-capable");

	ctl = pci_conf_read(pc, tag, off + PCI_MSI_CTL);
	ctl &= ~PCI_MSI_CTL_MSI_ENABLE;
	pci_conf_write(pc, tag, off + PCI_MSI_CTL, ctl);

	ctl = pci_conf_read(pc, tag, off + PCI_MSI_CTL);
	ctl &= ~PCI_MSI_CTL_MME_MASK;
	ctl |= __SHIFTIN(ilog2(count), PCI_MSI_CTL_MME_MASK);
	pci_conf_write(pc, tag, off + PCI_MSI_CTL, ctl);

	const uint64_t addr = sc->sc_msi_addr;
	const uint32_t data = msi;

	ctl = pci_conf_read(pc, tag, off + PCI_MSI_CTL);
	if (ctl & PCI_MSI_CTL_64BIT_ADDR) {
		pci_conf_write(pc, tag, off + PCI_MSI_MADDR64_LO,
		    __SHIFTOUT(addr, __BITS(31, 0)));
		pci_conf_write(pc, tag, off + PCI_MSI_MADDR64_HI,
		    __SHIFTOUT(addr, __BITS(63, 32)));
		pci_conf_write(pc, tag, off + PCI_MSI_MDATA64, data);
	} else {
		pci_conf_write(pc, tag, off + PCI_MSI_MADDR,
		    __SHIFTOUT(addr, __BITS(31, 0)));
		pci_conf_write(pc, tag, off + PCI_MSI_MDATA, data);
	}
	ctl |= PCI_MSI_CTL_MSI_ENABLE;
	pci_conf_write(pc, tag, off + PCI_MSI_CTL, ctl);
}

static void
apple_pcie_msi_msi_disable(struct apple_pcie_softc *sc, int msi)
{
	const struct pci_attach_args *pa = sc->sc_msi_pa[msi];
	pci_chipset_tag_t pc = pa->pa_pc;
	pcitag_t tag = pa->pa_tag;
	pcireg_t ctl;
	int off;

	if (!pci_get_capability(pc, tag, PCI_CAP_MSI, &off, NULL))
		panic("apple_pcie_msi_msi_disable: device is not MSI-capable");

	ctl = pci_conf_read(pc, tag, off + PCI_MSI_CTL);
	ctl &= ~PCI_MSI_CTL_MSI_ENABLE;
	pci_conf_write(pc, tag, off + PCI_MSI_CTL, ctl);
}

static void
apple_pcie_msi_msix_enable(struct apple_pcie_softc *sc, int msi, int msix_vec,
    bus_space_tag_t bst, bus_space_handle_t bsh)
{
	const struct pci_attach_args *pa = sc->sc_msi_pa[msi];
	pci_chipset_tag_t pc = pa->pa_pc;
	pcitag_t tag = pa->pa_tag;
	pcireg_t ctl;
	uint32_t val;
	int off;

	if (!pci_get_capability(pc, tag, PCI_CAP_MSIX, &off, NULL))
		panic("apple_pcie_msi_msix_enable: device is not MSI-X-capable");

	ctl = pci_conf_read(pc, tag, off + PCI_MSIX_CTL);
	ctl &= ~PCI_MSIX_CTL_ENABLE;
	pci_conf_write(pc, tag, off + PCI_MSIX_CTL, ctl);

	const uint64_t addr = sc->sc_msi_addr;
	const uint32_t data = msi;
	const uint64_t entry_base = PCI_MSIX_TABLE_ENTRY_SIZE * msix_vec;
	bus_space_write_4(bst, bsh, entry_base + PCI_MSIX_TABLE_ENTRY_ADDR_LO,
	    __SHIFTOUT(addr, __BITS(31, 0)));
	bus_space_write_4(bst, bsh, entry_base + PCI_MSIX_TABLE_ENTRY_ADDR_HI,
	    __SHIFTOUT(addr, __BITS(63, 32)));
	bus_space_write_4(bst, bsh, entry_base + PCI_MSIX_TABLE_ENTRY_DATA,
	    data);
	val = bus_space_read_4(bst, bsh,
	    entry_base + PCI_MSIX_TABLE_ENTRY_VECTCTL);
	val &= ~PCI_MSIX_VECTCTL_MASK;
	bus_space_write_4(bst, bsh, entry_base + PCI_MSIX_TABLE_ENTRY_VECTCTL,
	    val);

	ctl = pci_conf_read(pc, tag, off + PCI_MSIX_CTL);
	ctl |= PCI_MSIX_CTL_ENABLE;
	pci_conf_write(pc, tag, off + PCI_MSIX_CTL, ctl);
}

static void
apple_pcie_msi_msix_disable(struct apple_pcie_softc *sc, int msi)
{
	const struct pci_attach_args *pa = sc->sc_msi_pa[msi];
	pci_chipset_tag_t pc = pa->pa_pc;
	pcitag_t tag = pa->pa_tag;
	pcireg_t ctl;
	int off;

	if (!pci_get_capability(pc, tag, PCI_CAP_MSIX, &off, NULL))
		panic("apple_pcie_msi_msix_disable: device is not MSI-X-capable");

	ctl = pci_conf_read(pc, tag, off + PCI_MSIX_CTL);
	ctl &= ~PCI_MSIX_CTL_ENABLE;
	pci_conf_write(pc, tag, off + PCI_MSIX_CTL, ctl);
}

static pci_intr_handle_t *
apple_pcie_msi_msi_alloc(struct arm_pci_msi *msi, int *count,
    const struct pci_attach_args *pa, bool exact)
{
	struct apple_pcie_softc * const sc = msi->msi_priv;
	pci_intr_handle_t *vectors;
	int n, off;

	if (!pci_get_capability(pa->pa_pc, pa->pa_tag, PCI_CAP_MSI, &off, NULL))
		return NULL;

	const int avail = apple_pcie_msi_available_msi(sc);
	if (avail == 0)
		return NULL;

	if (exact && *count > avail)
		return NULL;

	while (*count > avail) {
		(*count) >>= 1;
	}
	if (*count == 0)
		return NULL;

	const int msi_base = apple_pcie_msi_alloc_msi(sc, *count, pa);
	if (msi_base == -1)
		return NULL;

	vectors = kmem_alloc(sizeof(*vectors) * *count, KM_SLEEP);
	for (n = 0; n < *count; n++) {
		const int msino = msi_base + n;
		vectors[n] = ARM_PCI_INTR_MSI |
		    __SHIFTIN(msino, ARM_PCI_INTR_IRQ) |
		    __SHIFTIN(n, ARM_PCI_INTR_MSI_VEC) |
		    __SHIFTIN(msi->msi_id, ARM_PCI_INTR_FRAME);
	}

	apple_pcie_msi_msi_enable(sc, msi_base, *count);

	return vectors;
}

static pci_intr_handle_t *
apple_pcie_msi_msix_alloc(struct arm_pci_msi *msi, u_int *table_indexes,
    int *count, const struct pci_attach_args *pa, bool exact)
{
	struct apple_pcie_softc * const sc = msi->msi_priv;
	pci_intr_handle_t *vectors;
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	bus_size_t bsz;
	uint32_t table_offset, table_size;
	int n, off, bar, error;
	pcireg_t tbl;

	if (!pci_get_capability(pa->pa_pc, pa->pa_tag, PCI_CAP_MSIX, &off, NULL))
		return NULL;

	const int avail = apple_pcie_msi_available_msi(sc);
	if (exact && *count > avail)
		return NULL;

	while (*count > avail) {
		(*count) >>= 1;
	}
	if (*count == 0)
		return NULL;

	tbl = pci_conf_read(pa->pa_pc, pa->pa_tag, off + PCI_MSIX_TBLOFFSET);
	bar = PCI_BAR0 + (4 * (tbl & PCI_MSIX_TBLBIR_MASK));
	table_offset = tbl & PCI_MSIX_TBLOFFSET_MASK;
	table_size = pci_msix_count(pa->pa_pc, pa->pa_tag) * PCI_MSIX_TABLE_ENTRY_SIZE;
	if (table_size == 0)
		return NULL;

	error = pci_mapreg_submap(pa, bar, pci_mapreg_type(pa->pa_pc, pa->pa_tag, bar),
	    BUS_SPACE_MAP_LINEAR, roundup(table_size, PAGE_SIZE), table_offset,
	    &bst, &bsh, NULL, &bsz);
	if (error)
		return NULL;

	const int msi_base = apple_pcie_msi_alloc_msi(sc, *count, pa);
	if (msi_base == -1) {
		bus_space_unmap(bst, bsh, bsz);
		return NULL;
	}

	vectors = kmem_alloc(sizeof(*vectors) * *count, KM_SLEEP);
	for (n = 0; n < *count; n++) {
		const int msino = msi_base + n;
		const int msix_vec = table_indexes ? table_indexes[n] : n;
		vectors[msix_vec] = ARM_PCI_INTR_MSIX |
		    __SHIFTIN(msino, ARM_PCI_INTR_IRQ) |
		    __SHIFTIN(msix_vec, ARM_PCI_INTR_MSI_VEC) |
		    __SHIFTIN(msi->msi_id, ARM_PCI_INTR_FRAME);

		apple_pcie_msi_msix_enable(sc, msino, msix_vec, bst, bsh);
	}

	bus_space_unmap(bst, bsh, bsz);

	return vectors;
}

static void *
apple_pcie_msi_intr_establish(struct arm_pci_msi *msi,
    pci_intr_handle_t ih, int ipl, int (*func)(void *), void *arg, const char *xname)
{
	struct apple_pcie_softc * const sc = msi->msi_priv;

	const int msino = __SHIFTOUT(ih, ARM_PCI_INTR_IRQ);
	const int mpsafe = (ih & ARM_PCI_INTR_MPSAFE) ? FDT_INTR_MPSAFE : 0;

	KASSERT(sc->sc_msi_ih[msino] == NULL);
	sc->sc_msi_ih[msino] = intr_establish_xname(sc->sc_msi_start + msino,
	    ipl, IST_LEVEL | (mpsafe ? IST_MPSAFE : 0), func, arg, xname);

	return sc->sc_msi_ih[msino];
}

static void
apple_pcie_msi_intr_release(struct arm_pci_msi *msi, pci_intr_handle_t *pih,
    int count)
{
	struct apple_pcie_softc * const sc = msi->msi_priv;
	int n;

	for (n = 0; n < count; n++) {
		const int msino = __SHIFTOUT(pih[n], ARM_PCI_INTR_IRQ);
		if (pih[n] & ARM_PCI_INTR_MSIX)
			apple_pcie_msi_msix_disable(sc, msino);
		if (pih[n] & ARM_PCI_INTR_MSI)
			apple_pcie_msi_msi_disable(sc, msino);
		apple_pcie_msi_free_msi(sc, msino);
		if (sc->sc_msi_ih[msino] != NULL) {
			intr_disestablish(sc->sc_msi_ih[msino]);
			sc->sc_msi_ih[msino] = NULL;
		}
	}
}

static int
apple_pcie_msi_init(struct apple_pcie_softc *sc)
{
	struct arm_pci_msi *msi = &sc->sc_msi;
	const int phandle = sc->sc_pcihost.sc_phandle;
	int len;

	const u_int *data = fdtbus_get_prop(phandle, "msi-ranges", &len);
	switch (len) {
	case 8:
		/* two cells: start and count */
		sc->sc_msi_start = be32toh(data[0]);
		sc->sc_nmsi = be32toh(data[1]);
		break;
	case 20:
		/* 5 cells: xref, specifier (3 cells), and count */
		sc->sc_msi_start = be32toh(data[2]);
		sc->sc_nmsi = be32toh(data[4]);
		break;
	default:
		aprint_error_dev(sc->sc_pcihost.sc_dev,
		    "WARNING: bad msi-ranges property, MSI not enabled!\n");
		return ENXIO;
	}
	sc->sc_msi_pa = kmem_zalloc(sizeof(*sc->sc_msi_pa) * sc->sc_nmsi,
	    KM_SLEEP);
	sc->sc_msi_ih = kmem_zalloc(sizeof(*sc->sc_msi_ih) * sc->sc_nmsi,
	    KM_SLEEP);

	if (of_getprop_uint64(phandle, "msi-doorbell", &sc->sc_msi_addr)) {
		sc->sc_msi_addr = 0xffff000ULL;
	}

	msi->msi_dev = sc->sc_pcihost.sc_dev;
	msi->msi_priv = sc;
	msi->msi_alloc = apple_pcie_msi_msi_alloc;
	msi->msix_alloc = apple_pcie_msi_msix_alloc;
	msi->msi_intr_establish = apple_pcie_msi_intr_establish;
	msi->msi_intr_release = apple_pcie_msi_intr_release;

	return arm_pci_msi_add(msi);
}
