/*	$NetBSD: pci_gio.c,v 1.5.64.1 2010/01/20 09:04:32 matt Exp $	*/

/*
 * Copyright (c) 2006 Stephen M. Rumble
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pci_gio.c,v 1.5.64.1 2010/01/20 09:04:32 matt Exp $");

/*
 * Glue for PCI devices that are connected to the GIO bus by various little
 * GIO<->PCI ASICs.
 *
 * We presently support the following boards:
 *	o Phobos G100/G130/G160	(if_tlp, lxtphy)
 *	o Set Engineering GFE	(if_tl, nsphy)
 */

#include "opt_pci.h"
#include "pci.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/extent.h>

#include <machine/bus.h>
#include <machine/machtype.h>

#include <sgimips/gio/giovar.h>
#include <sgimips/gio/gioreg.h>
#include <sgimips/gio/giodevs.h>

#include <sgimips/dev/imcvar.h>

#include <mips/cache.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciconf.h>

int giopci_debug = 0;
#define DPRINTF(_x)	if (giopci_debug) printf _x

struct giopci_softc {
	struct device			sc_dev;
	struct sgimips_pci_chipset	sc_pc;
	int				sc_slot;
	int				sc_gprid;
	uint32_t			sc_pci_len;
	bus_space_tag_t			sc_iot;
	bus_space_handle_t		sc_ioh;
};

static int	giopci_match(struct device *, struct cfdata *, void *);
static void	giopci_attach(struct device *, struct device *, void *);
static int	giopci_bus_maxdevs(pci_chipset_tag_t, int);
static pcireg_t	giopci_conf_read(pci_chipset_tag_t, pcitag_t, int);
static void	giopci_conf_write(pci_chipset_tag_t, pcitag_t, int, pcireg_t);
static int	giopci_conf_hook(pci_chipset_tag_t, int, int, int, pcireg_t);
static int	giopci_intr_map(struct pci_attach_args *, pci_intr_handle_t *);
static const char *
		giopci_intr_string(pci_chipset_tag_t, pci_intr_handle_t);
static void    *giopci_intr_establish(int, int, int (*)(void *), void *);
static void	giopci_intr_disestablish(void *);

#define PHOBOS_PCI_OFFSET	0x00100000
#define PHOBOS_PCI_LENGTH	128		/* ~arbitrary */
#define PHOBOS_TULIP_START	0x00101000
#define PHOBOS_TULIP_END	0x001fffff

#define SETENG_MAGIC_OFFSET	0x00020000
#define SETENG_MAGIC_VALUE	0x00001000
#define SETENG_PCI_OFFSET	0x00080000
#define SETENG_PCI_LENGTH	128		/* ~arbitrary */
#define SETENG_TLAN_START	0x00100000
#define SETENG_TLAN_END		0x001fffff

CFATTACH_DECL(giopci, sizeof(struct giopci_softc),
    giopci_match, giopci_attach, NULL, NULL);

static int
giopci_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct gio_attach_args *ga = aux;
	int gprid;

	/*
	 * I think that these cards are all GIO32-bis or GIO64. Thus
	 * they work in either Indigo2/Challenge M or
	 * Indy/Challenge S/Indigo R4k, according to form factor. However,
	 * there are some exceptions (e.g. my Indigo R4k won't power 
	 * on with the Set Engineering card installed).
	 */
	if (mach_type != MACH_SGI_IP20 && mach_type != MACH_SGI_IP22)
		return (0);

	gprid = GIO_PRODUCT_PRODUCTID(ga->ga_product);
	if (gprid == PHOBOS_G100 || gprid == PHOBOS_G130 ||
	    gprid == PHOBOS_G160 || gprid == SETENG_GFE)
		return (1);

	return (0);
}

static void 
giopci_attach(struct device *parent, struct device *self, void *aux)
{
	struct giopci_softc *sc = (void *)self;
	pci_chipset_tag_t pc = &sc->sc_pc;
	struct gio_attach_args *ga = aux;
	uint32_t pci_off, pci_len, arb;
	struct pcibus_attach_args pba;
	u_long m_start, m_end;
#ifdef PCI_NETBSD_CONFIGURE
	extern int pci_conf_debug;

	pci_conf_debug = giopci_debug;
#endif

	sc->sc_iot	= ga->ga_iot;
	sc->sc_slot	= ga->ga_slot;
	sc->sc_gprid	= GIO_PRODUCT_PRODUCTID(ga->ga_product);

	if (mach_type == MACH_SGI_IP22 &&
	    mach_subtype == MACH_SGI_IP22_FULLHOUSE)
		arb = GIO_ARB_RT | GIO_ARB_MST | GIO_ARB_PIPE;
	else
		arb = GIO_ARB_RT | GIO_ARB_MST;

	if (gio_arb_config(ga->ga_slot, arb)) {
		printf(": failed to configure GIO bus arbiter\n");
		return;
	}

#if (NIMC > 0)
	imc_disable_sysad_parity();
#endif

	switch (sc->sc_gprid) {
	case PHOBOS_G100:
	case PHOBOS_G130:
	case PHOBOS_G160:
		pci_off = PHOBOS_PCI_OFFSET;
		pci_len = PHOBOS_PCI_LENGTH;
		m_start = MIPS_KSEG1_TO_PHYS(ga->ga_addr + PHOBOS_TULIP_START);
		m_end = MIPS_KSEG1_TO_PHYS(ga->ga_addr + PHOBOS_TULIP_END);
		break;

	case SETENG_GFE:
		/*
		 * NB: The SetEng board does not allow the ThunderLAN's DMA
		 *     engine to properly transfer segments that span page
		 *     boundaries. See sgimips/autoconf.c where we catch a
		 *     tl(4) device attachment and create an appropriate
		 *     proplib entry to enable the workaround.
		 */
		pci_off = SETENG_PCI_OFFSET;
		pci_len = SETENG_PCI_LENGTH;
		m_start = MIPS_KSEG1_TO_PHYS(ga->ga_addr + SETENG_TLAN_START);
		m_end = MIPS_KSEG1_TO_PHYS(ga->ga_addr + SETENG_TLAN_END);
		bus_space_write_4(ga->ga_iot, ga->ga_ioh,
		    SETENG_MAGIC_OFFSET, SETENG_MAGIC_VALUE);
		break;

	default:
		panic("giopci_attach: unsupported GIO product id 0x%02x",
		    sc->sc_gprid);
	}

	if (bus_space_subregion(ga->ga_iot, ga->ga_ioh, pci_off, pci_len,
	    &sc->sc_ioh)) {
		printf("%s: unable to map PCI registers\n",sc->sc_dev.dv_xname);
		return;
	}
	sc->sc_pci_len = pci_len;

	pc->pc_bus_maxdevs	= giopci_bus_maxdevs;
	pc->pc_conf_read	= giopci_conf_read;
	pc->pc_conf_write	= giopci_conf_write;
	pc->pc_conf_hook	= giopci_conf_hook;
	pc->pc_intr_map		= giopci_intr_map;
	pc->pc_intr_string	= giopci_intr_string;
	pc->intr_establish	= giopci_intr_establish;
	pc->intr_disestablish	= giopci_intr_disestablish;
	pc->iot			= ga->ga_iot;
	pc->ioh			= ga->ga_ioh;
	pc->cookie		= sc;

	printf(": %s\n", gio_product_string(sc->sc_gprid));

#ifdef PCI_NETBSD_CONFIGURE
	pc->pc_memext = extent_create("giopcimem", m_start, m_end,
	    M_DEVBUF, NULL, 0, EX_NOWAIT);
	pci_configure_bus(pc, NULL, pc->pc_memext, NULL, 0,
	    mips_cache_info.mci_dcache_align);
#endif

	memset(&pba, 0, sizeof(pba));
	pba.pba_memt	= SGIMIPS_BUS_SPACE_MEM;
	pba.pba_dmat	= ga->ga_dmat;
	pba.pba_pc	= pc;
	pba.pba_flags	= PCI_FLAGS_MEM_ENABLED;
	/* NB: do not set PCI_FLAGS_{MRL,MRM,MWI}_OKAY  -- true ?! */

	config_found_ia(self, "pcibus", &pba, pcibusprint);
}

static int
giopci_bus_maxdevs(pci_chipset_tag_t pc, int busno)
{
	
	return (busno == 0);
}

static pcireg_t
giopci_conf_read(pci_chipset_tag_t pc, pcitag_t tag, int reg)
{
	struct giopci_softc *sc = pc->cookie;
	int bus, dev, func;
	pcireg_t data;

	pci_decompose_tag(pc, tag, &bus, &dev, &func);
	if (bus != 0 || dev != 0 || func != 0)
		return (0);

	/* XXX - should just use bus_space_peek */
	if (reg >= sc->sc_pci_len) {
		DPRINTF(("giopci_conf_read: reg 0x%x out of bounds\n", reg));
		return (0);
	}

	DPRINTF(("giopci_conf_read: reg 0x%x = 0x", reg));
	data = bus_space_read_4(sc->sc_iot, sc->sc_ioh, reg);
	DPRINTF(("%08x\n", data));

	return (data);
}

static void
giopci_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t data)
{
	struct giopci_softc *sc = pc->cookie;
	int bus, dev, func;

	pci_decompose_tag(pc, tag, &bus, &dev, &func);
	if (bus != 0 || dev != 0 || func != 0)
		return;

	/* XXX - should just use bus_space_poke */
	if (reg >= sc->sc_pci_len) {
		DPRINTF(("giopci_conf_write: reg 0x%x out of bounds "
		    "(val = 0x%08x)\n", reg, data));
		return;
	}

	DPRINTF(("giopci_conf_write: reg 0x%x = 0x%08x\n", reg, data));
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, reg, data);
}

static int
giopci_conf_hook(pci_chipset_tag_t pc, int bus, int device, int function,
    pcireg_t id)
{

	/* All devices use memory accesses only. */
	return (PCI_CONF_MAP_MEM | PCI_CONF_ENABLE_MEM | PCI_CONF_ENABLE_BM);
}

static int
giopci_intr_map(struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	struct giopci_softc *sc = pa->pa_pc->cookie;

	*ihp = sc->sc_slot;

	return (0);
}

static const char *
giopci_intr_string(pci_chipset_tag_t pc, pci_intr_handle_t ih)
{
	static char str[10];

	snprintf(str, sizeof(str), "slot %s",
	    (ih == GIO_SLOT_EXP0) ? "EXP0" :
	    (ih == GIO_SLOT_EXP1) ? "EXP1" : "GFX");
	return (str);
}

static void *
giopci_intr_establish(int slot, int level, int (*func)(void *), void *arg)
{

	return (gio_intr_establish(slot, level, func, arg));
}

static void
giopci_intr_disestablish(void *cookie)
{

	panic("giopci_intr_disestablish: impossible.");
}
