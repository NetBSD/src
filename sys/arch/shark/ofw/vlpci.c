/*	$NetBSD: vlpci.c,v 1.3 2017/02/19 14:34:40 jakllsch Exp $	*/

/*
 * Copyright (c) 2017 Jonathan A. Kollasch
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vlpci.c,v 1.3 2017/02/19 14:34:40 jakllsch Exp $");

#include "opt_pci.h"
#include "pci.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/mutex.h>

#include <dev/isa/isavar.h>

#include <dev/ofw/openfirm.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>
#include <arm/pci_machdep.h>

static int	vlpci_match(device_t, struct cfdata *, void *);
static void	vlpci_attach(device_t, device_t, void *);

static void	vlpci_pc_attach_hook(device_t, device_t,
    struct pcibus_attach_args *);
static int	vlpci_pc_bus_maxdevs(void *, int);
static pcitag_t	vlpci_pc_make_tag(void *, int, int, int);
static void	vlpci_pc_decompose_tag(void *, pcitag_t, int *, int *, int *);
static pcireg_t	vlpci_pc_conf_read(void *, pcitag_t, int);
static void	vlpci_pc_conf_write(void *, pcitag_t, int, pcireg_t);
#ifdef __HAVE_PCI_CONF_HOOK
static int	vlpci_pc_conf_hook(void *, int, int, int, pcireg_t);
#endif
static void	vlpci_pc_conf_interrupt(void *, int, int, int, int, int *);

struct vlpci_softc {
	device_t			sc_dev;
	kmutex_t			sc_lock;
	bus_space_handle_t		sc_conf_ioh;
	bus_space_handle_t		sc_reg_ioh;
	struct arm32_pci_chipset	sc_pc;
};

CFATTACH_DECL_NEW(vlpci, sizeof(struct vlpci_softc),
    vlpci_match, vlpci_attach, NULL, NULL);

static const char * const compat_strings[] = { "via,vt82c505", NULL };

static void
regwrite_1(struct vlpci_softc * const sc, uint8_t off, uint8_t val)
{
	mutex_spin_enter(&sc->sc_lock);
	bus_space_write_1(&isa_io_bs_tag, sc->sc_reg_ioh, 0, off);
	bus_space_write_1(&isa_io_bs_tag, sc->sc_reg_ioh, 1, val);
	mutex_spin_exit(&sc->sc_lock);
}

static int
vlpci_match(device_t parent, struct cfdata *match, void *aux)
{
	struct ofbus_attach_args * const oba = aux;

	if (of_compatible(oba->oba_phandle, compat_strings) < 0)
		return 0;

	return 2;	/* beat generic ofbus */
}

static void
vlpci_attach(device_t parent, device_t self, void *aux)
{
	struct vlpci_softc * const sc = device_private(self);
	pci_chipset_tag_t const pc = &sc->sc_pc;
	struct pcibus_attach_args pba;

	aprint_normal("\n");

	sc->sc_dev = self;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_HIGH);
	memset(&pba, 0, sizeof(pba));

	if (bus_space_map(&isa_io_bs_tag, 0xa8, 0x2,
	    0, &sc->sc_reg_ioh) != 0) {
		aprint_error_dev(self, "failed to map 0xa8-9\n");
		return;
	}
	if (bus_space_map(&isa_io_bs_tag, 0xcf8, 0x8,
	    0, &sc->sc_conf_ioh) != 0) {
		aprint_error_dev(self, "failed to map 0xcf8-f\n");
		return;
	}

	/* Enable VLB/PCI bridge */
	regwrite_1(sc, 0x96, 0x18); /* Undocumented by VIA */
	regwrite_1(sc, 0x93, 0xd0);

	pc->pc_conf_v = sc;
	pc->pc_attach_hook = vlpci_pc_attach_hook;
	pc->pc_bus_maxdevs = vlpci_pc_bus_maxdevs;
	pc->pc_make_tag = vlpci_pc_make_tag;
	pc->pc_decompose_tag = vlpci_pc_decompose_tag;
	pc->pc_conf_read = vlpci_pc_conf_read;
	pc->pc_conf_write = vlpci_pc_conf_write;
#ifdef __HAVE_PCI_CONF_HOOK
	pc->pc_conf_hook = vlpci_pc_conf_hook;
#endif
	pc->pc_conf_interrupt = vlpci_pc_conf_interrupt;

	pc->pc_intr_v = sc;

	pba.pba_flags = PCI_FLAGS_IO_OKAY; /* XXX test this, implement more */
	pba.pba_pc = &sc->sc_pc;
	pba.pba_bus = 0;

	config_found_ia(self, "pcibus", &pba, pcibusprint);
}

static void
vlpci_pc_attach_hook(device_t parent, device_t self,
    struct pcibus_attach_args *pba)
{
}

static int
vlpci_pc_bus_maxdevs(void *v, int busno)
{
	return busno == 0 ? 32 : 0;
}

static pcitag_t
vlpci_pc_make_tag(void *v, int b, int d, int f)
{
	return (b << 16) | (d << 11) | (f << 8);
}

static void
vlpci_pc_decompose_tag(void *v, pcitag_t tag, int *bp, int *dp, int *fp)
{
	if (bp)
		*bp = (tag >> 16) & 0xff;
	if (dp)
		*dp = (tag >> 11) & 0x1f;
	if (fp)
		*fp = (tag >> 8) & 0x7;
}

static pcireg_t
vlpci_pc_conf_read(void *v, pcitag_t tag, int offset)
{
	struct vlpci_softc * const sc = v;
	pcireg_t ret;

	KASSERT((offset & 3) == 0);

	if (offset >= PCI_CONF_SIZE)
		return 0xffffffff;

	mutex_spin_enter(&sc->sc_lock);
	bus_space_write_4(&isa_io_bs_tag, sc->sc_conf_ioh, 0,
	    0x80000000UL|tag|offset);
	ret = bus_space_read_4(&isa_io_bs_tag, sc->sc_conf_ioh, 4);
	mutex_spin_exit(&sc->sc_lock);

#if 0
	device_printf(sc->sc_dev, "%s tag %x offset %x ret %x\n",
	    __func__, (unsigned int)tag, offset, ret);
#endif

	return ret;
}

static void
vlpci_pc_conf_write(void *v, pcitag_t tag, int offset, pcireg_t val)
{
	struct vlpci_softc * const sc = v;

	KASSERT((offset & 3) == 0);

	if (offset >= PCI_CONF_SIZE)
		return;

#if 0
	device_printf(sc->sc_dev, "%s tag %x offset %x val %x\n",
	    __func__, (unsigned int)tag, offset, val);
#endif

	mutex_spin_enter(&sc->sc_lock);
	bus_space_write_4(&isa_io_bs_tag, sc->sc_conf_ioh, 0,
	    0x80000000UL|tag|offset);
	bus_space_write_4(&isa_io_bs_tag, sc->sc_conf_ioh, 4, val);
	mutex_spin_exit(&sc->sc_lock);
}

#ifdef __HAVE_PCI_CONF_HOOK
static int
vlpci_pc_conf_hook(void *v, int b, int d, int f, pcireg_t id)
{
	return PCI_CONF_DEFAULT & ~PCI_CONF_ENABLE_BM;
}
#endif

static void
vlpci_pc_conf_interrupt(void *v, int bus, int dev, int ipin, int swiz,
    int *ilinep)
{
	*ilinep = 0xff; /* XXX */
}
