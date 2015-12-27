/*	$NetBSD: vrc4172pci.c,v 1.17.6.1 2015/12/27 12:09:36 skrll Exp $	*/

/*-
 * Copyright (c) 2002 TAKEMURA Shin
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vrc4172pci.c,v 1.17.6.1 2015/12/27 12:09:36 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/bus_space_hpcmips.h>
#include <machine/bus_dma_hpcmips.h>
#include <machine/config_hook.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciidereg.h>

#include <hpcmips/vr/icureg.h>
#include <hpcmips/vr/vripif.h>
#include <hpcmips/vr/vrc4172pcireg.h>

#include "pci.h"
#include "opt_vrc4172pci.h"

#ifdef DEBUG
#define	DPRINTF(args)	printf args
#else
#define	DPRINTF(args)	while (0) {}
#endif

struct vrc4172pci_softc {
	device_t sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	struct hpcmips_pci_chipset sc_pc;
#ifdef VRC4172PCI_MCR700_SUPPORT
	pcireg_t sc_fake_baseaddr;
	hpcio_chip_t sc_iochip;
#if 0
	hpcio_intr_handle_t sc_ih;
#endif
#endif /* VRC4172PCI_MCR700_SUPPORT */
};

static int	vrc4172pci_match(device_t, cfdata_t, void *);
static void	vrc4172pci_attach(device_t, device_t, void *);
static void	vrc4172pci_attach_hook(device_t, device_t,
		    struct pcibus_attach_args *);
static int	vrc4172pci_bus_maxdevs(pci_chipset_tag_t, int);
static int	vrc4172pci_bus_devorder(pci_chipset_tag_t, int, uint8_t *, int);
static pcitag_t	vrc4172pci_make_tag(pci_chipset_tag_t, int, int, int);
static void	vrc4172pci_decompose_tag(pci_chipset_tag_t, pcitag_t, int *,
		    int *, int *);
static pcireg_t	vrc4172pci_conf_read(pci_chipset_tag_t, pcitag_t, int); 
static void	vrc4172pci_conf_write(pci_chipset_tag_t, pcitag_t, int,
		    pcireg_t);
static int	vrc4172pci_intr_map(const struct pci_attach_args *,
		    pci_intr_handle_t *);
static const char *vrc4172pci_intr_string(pci_chipset_tag_t,pci_intr_handle_t,
		    char *, size_t);
static const struct evcnt *vrc4172pci_intr_evcnt(pci_chipset_tag_t,
		    pci_intr_handle_t);
static void	*vrc4172pci_intr_establish(pci_chipset_tag_t,
		    pci_intr_handle_t, int, int (*)(void *), void *);
static void	vrc4172pci_intr_disestablish(pci_chipset_tag_t, void *);
#ifdef VRC4172PCI_MCR700_SUPPORT
#if 0
static int	vrc4172pci_mcr700_intr(void *arg);
#endif
#endif

CFATTACH_DECL_NEW(vrc4172pci, sizeof(struct vrc4172pci_softc),
    vrc4172pci_match, vrc4172pci_attach, NULL, NULL);

static inline void
vrc4172pci_write(struct vrc4172pci_softc *sc, int offset, u_int32_t val)
{

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, offset, val);
}

static inline u_int32_t
vrc4172pci_read(struct vrc4172pci_softc *sc, int offset)
{
	u_int32_t res;

	if (bus_space_peek(sc->sc_iot, sc->sc_ioh, offset, 4, &res) < 0) {
		res = 0xffffffff;
	}

	return (res);
}

static int
vrc4172pci_match(device_t parent, cfdata_t match, void *aux)
{

	return (1);
}

static void
vrc4172pci_attach(device_t parent, device_t self, void *aux)
{
	struct vrc4172pci_softc *sc = device_private(self);
	pci_chipset_tag_t pc = &sc->sc_pc;
	struct vrip_attach_args *va = aux;
#if NPCI > 0
	struct pcibus_attach_args pba;
#endif

	sc->sc_dev = self;
	sc->sc_iot = va->va_iot;
	if (bus_space_map(sc->sc_iot, va->va_addr, va->va_size, 0,
	    &sc->sc_ioh)) {
		printf(": couldn't map io space\n");
		return;
	}
	printf("\n");

#ifdef VRC4172PCI_MCR700_SUPPORT
	if (platid_match(&platid, &platid_mask_MACH_NEC_MCR_700) ||
	    platid_match(&platid, &platid_mask_MACH_NEC_MCR_700A) ||
	    platid_match(&platid, &platid_mask_MACH_NEC_MCR_730) ||
	    platid_match(&platid, &platid_mask_MACH_NEC_MCR_730A)) {
		/* power USB controller on MC-R700 */
		sc->sc_iochip = va->va_gpio_chips[VRIP_IOCHIP_VRGIU];
		hpcio_portwrite(sc->sc_iochip, 45, 1);
		sc->sc_fake_baseaddr = 0x0afe0000;
#if 0
		sc->sc_ih = hpcio_intr_establish(sc->sc_iochip, 1,
		    HPCIO_INTR_EDGE|HPCIO_INTR_HOLD,
		    vrc4172pci_mcr700_intr, sc);
#endif
	}
#endif /* VRC4172PCI_MCR700_SUPPORT */

	pc->pc_dev = sc->sc_dev;
	pc->pc_attach_hook = vrc4172pci_attach_hook;
	pc->pc_bus_maxdevs = vrc4172pci_bus_maxdevs;
	pc->pc_bus_devorder = vrc4172pci_bus_devorder;
	pc->pc_make_tag = vrc4172pci_make_tag;
	pc->pc_decompose_tag = vrc4172pci_decompose_tag;
	pc->pc_conf_read = vrc4172pci_conf_read;
	pc->pc_conf_write = vrc4172pci_conf_write;
	pc->pc_intr_map = vrc4172pci_intr_map;
	pc->pc_intr_string = vrc4172pci_intr_string;
	pc->pc_intr_evcnt = vrc4172pci_intr_evcnt;
	pc->pc_intr_establish = vrc4172pci_intr_establish;
	pc->pc_intr_disestablish = vrc4172pci_intr_disestablish;

#if 0
	{
		int i;

		for (i = 0; i < 2; i++)
			printf("%s: ID_REG(0, 0, %d) = 0x%08x\n",
			    device_xname(self), i,
			    pci_conf_read(pc, pci_make_tag(pc, 0, 0, i),
				PCI_ID_REG));
	}
#endif

#if NPCI > 0
	memset(&pba, 0, sizeof(pba));
	pba.pba_iot = sc->sc_iot;
	pba.pba_memt = sc->sc_iot;
	pba.pba_dmat = &hpcmips_default_bus_dma_tag.bdt;
	pba.pba_dmat64 = NULL;
	pba.pba_bus = 0;
	pba.pba_bridgetag = NULL;
	pba.pba_flags = PCI_FLAGS_IO_OKAY | PCI_FLAGS_MEM_OKAY |
	    PCI_FLAGS_MRL_OKAY;
	pba.pba_pc = pc;

	config_found_ia(self, "pcibus", &pba, pcibusprint);
#endif
}

void
vrc4172pci_attach_hook(device_t parent, device_t self,
    struct pcibus_attach_args *pba)
{

	return;
}

int
vrc4172pci_bus_maxdevs(pci_chipset_tag_t pc, int busno)
{

	return (1);	/* Vrc4172 has only one device */
}

int
vrc4172pci_bus_devorder(pci_chipset_tag_t pc, int busno, uint8_t *devs,
    int maxdevs)
{
	if (maxdevs <= 0)
		return 0;
	devs[0] = 0;
	return 1;
}

pcitag_t
vrc4172pci_make_tag(pci_chipset_tag_t pc, int bus, int device, int function)
{

	return ((bus << 16) | (device << 11) | (function << 8));
}

void
vrc4172pci_decompose_tag(pci_chipset_tag_t pc, pcitag_t tag, int *bp, int *dp,
    int *fp)
{

	if (bp != NULL)
		*bp = (tag >> 16) & 0xff;
	if (dp != NULL)
		*dp = (tag >> 11) & 0x1f;
	if (fp != NULL)
		*fp = (tag >> 8) & 0x07;
}

pcireg_t
vrc4172pci_conf_read(pci_chipset_tag_t pc, pcitag_t tag, int reg)
{
	struct vrc4172pci_softc *sc = device_private(pc->pc_dev);
	u_int32_t val;

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return ((pcireg_t) -1);

#ifdef VRC4172PCI_MCR700_SUPPORT
	if (sc->sc_fake_baseaddr != 0 &&
	    tag == vrc4172pci_make_tag(pc, 0, 0, 1) &&
	    reg == PCI_MAPREG_START) {
		val = sc->sc_fake_baseaddr;
		goto out;
	}
#endif /*  VRC4172PCI_MCR700_SUPPORT */

	tag |= VRC4172PCI_CONFADDR_CONFIGEN;

	vrc4172pci_write(sc, VRC4172PCI_CONFAREG, tag | reg);
	val = vrc4172pci_read(sc, VRC4172PCI_CONFDREG);

#ifdef VRC4172PCI_MCR700_SUPPORT
 out:
#endif
	DPRINTF(("%s: conf_read: tag = 0x%08x, reg = 0x%x, val = 0x%08x\n",
	    device_xname(sc->sc_dev), (u_int32_t)tag, reg, val));

	return (val);
}

void
vrc4172pci_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg,
    pcireg_t data)
{
	struct vrc4172pci_softc *sc = device_private(pc->pc_dev);

	DPRINTF(("%s: conf_write: tag = 0x%08x, reg = 0x%x, val = 0x%08x\n",
	    device_xname(sc->sc_dev), (u_int32_t)tag, reg, (u_int32_t)data));

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return;

#ifdef VRC4172PCI_MCR700_SUPPORT
	if (sc->sc_fake_baseaddr != 0 &&
	    tag == vrc4172pci_make_tag(pc, 0, 0, 1) &&
	    reg == PCI_MAPREG_START) {
		sc->sc_fake_baseaddr = (data & 0xfffff000);
		return;
	}
#endif /*  VRC4172PCI_MCR700_SUPPORT */

	tag |= VRC4172PCI_CONFADDR_CONFIGEN;

	vrc4172pci_write(sc, VRC4172PCI_CONFAREG, tag | reg);
	vrc4172pci_write(sc, VRC4172PCI_CONFDREG, data);
}

int
vrc4172pci_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	pci_chipset_tag_t pc = pa->pa_pc;
	pcitag_t intrtag = pa->pa_intrtag;
	int bus, dev, func;

	pci_decompose_tag(pc, intrtag, &bus, &dev, &func);
	DPRINTF(("%s(%d, %d, %d): line = %d, pin = %d\n", device_xname(pc->pc_dev),
	    bus, dev, func, pa->pa_intrline, pa->pa_intrpin));

	*ihp = CONFIG_HOOK_PCIINTR_ID(bus, dev, func);

	return (0);
}

const char *
vrc4172pci_intr_string(pci_chipset_tag_t pc, pci_intr_handle_t ih, char *buf,
    size_t len)
{
	snprintf(buf, len, "pciintr %d:%d:%d",
	    CONFIG_HOOK_PCIINTR_BUS((int)ih),
	    CONFIG_HOOK_PCIINTR_DEVICE((int)ih),
	    CONFIG_HOOK_PCIINTR_FUNCTION((int)ih));

	return buf;
}

const struct evcnt *
vrc4172pci_intr_evcnt(pci_chipset_tag_t pc, pci_intr_handle_t ih)
{

	return (NULL);
}

void *
vrc4172pci_intr_establish(pci_chipset_tag_t pc, pci_intr_handle_t ih,
    int level, int (*func)(void *), void *arg)
{

	if (ih == -1)
		return (NULL);
	DPRINTF(("vrc4172pci_intr_establish: %lx\n", ih));

	return (config_hook(CONFIG_HOOK_PCIINTR, ih, CONFIG_HOOK_EXCLUSIVE,
	    (int (*)(void *, int, long, void *))func, arg));
}

void
vrc4172pci_intr_disestablish(pci_chipset_tag_t pc, void *cookie)
{

	DPRINTF(("vrc4172pci_intr_disestablish: %p\n", cookie));
	config_unhook(cookie);
}

#ifdef VRC4172PCI_MCR700_SUPPORT
#if 0
int
vrc4172pci_mcr700_intr(void *arg)
{
	struct vrc4172pci_softc *sc = arg;

	hpcio_intr_clear(sc->sc_iochip, sc->sc_ih);
	printf("USB port %s\n", hpcio_portread(sc->sc_iochip, 1) ? "ON" : "OFF");
	hpcio_portwrite(sc->sc_iochip, 45, hpcio_portread(sc->sc_iochip, 1));

	return (0);
}
#endif
#endif /* VRC4172PCI_MCR700_SUPPORT */
