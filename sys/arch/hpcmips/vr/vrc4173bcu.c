/*	$NetBSD: vrc4173bcu.c,v 1.15 2004/04/24 15:49:00 kleink Exp $	*/

/*-
 * Copyright (c) 2001,2002 Enami Tsugutomo.
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
__KERNEL_RCSID(0, "$NetBSD: vrc4173bcu.c,v 1.15 2004/04/24 15:49:00 kleink Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <machine/platid.h>
#include <machine/platid_mask.h>
#include <machine/config_hook.h>

#include <hpcmips/vr/vripunit.h>
#include <hpcmips/vr/vripif.h>
#include <hpcmips/vr/vrc4173bcuvar.h>
#include <hpcmips/vr/vrc4173icureg.h>
#include <hpcmips/vr/vrc4173cmureg.h>

#include "locators.h"

#ifdef VRC4173BCU_DEBUG
#define DPRINTF_ENABLE
#define DPRINTF_DEBUG	vrc4173bcu_debug
#endif
#define USE_HPC_DPRINTF
#include <machine/debug.h>

#define	VRC4173BCU_BADR		0x10
#define USE_WINCE_CLKMASK	(~0)

static int	vrc4173bcu_match(struct device *, struct cfdata *, void *);
static void	vrc4173bcu_attach(struct device *, struct device *, void *);
static int	vrc4173bcu_intr(void *);
static int	vrc4173bcu_print(void *, const char *);
static int	vrc4173bcu_search(struct device *, struct cfdata *cf, void *);
static int	vrc4173bcu_pci_intr(void *);
#ifdef VRC4173BCU_DEBUG
static void	vrc4173bcu_dump_level2mask(vrip_chipset_tag_t,
		    vrip_intr_handle_t);
#endif

int __vrc4173bcu_power(vrip_chipset_tag_t, int, int);
vrip_intr_handle_t __vrc4173bcu_intr_establish(vrip_chipset_tag_t, int, int,
    int, int(*)(void*), void*);
void __vrc4173bcu_intr_disestablish(vrip_chipset_tag_t, vrip_intr_handle_t);
void __vrc4173bcu_intr_setmask1(vrip_chipset_tag_t, vrip_intr_handle_t, int);
void __vrc4173bcu_intr_setmask2(vrip_chipset_tag_t, vrip_intr_handle_t,
    u_int32_t, int);
void __vrc4173bcu_intr_getstatus2(vrip_chipset_tag_t, vrip_intr_handle_t,
    u_int32_t*);
void __vrc4173bcu_register_cmu(vrip_chipset_tag_t, vrcmu_chipset_tag_t);
void __vrc4173bcu_register_gpio(vrip_chipset_tag_t, hpcio_chip_t);
void __vrc4173bcu_register_dmaau(vrip_chipset_tag_t, vrdmaau_chipset_tag_t);
void __vrc4173bcu_register_dcu(vrip_chipset_tag_t, vrdcu_chipset_tag_t);
int __vrc4173bcu_clock(vrcmu_chipset_tag_t, u_int16_t, int);

/*
 * machine dependent info
 */
static struct vrc4173bcu_platdep {
	platid_mask_t *platidmask;
	u_int32_t clkmask;
	int intrmask;
} platdep_table[] = {
	{
		&platid_mask_MACH_VICTOR_INTERLINK_MPC303,
		USE_WINCE_CLKMASK,		/* clock mask */
		(1 << VRC4173ICU_USBINTR)|	/* intrrupts */
		(1 << VRC4173ICU_PCMCIA1INTR)|
		(1 << VRC4173ICU_PCMCIA2INTR),
	},
	{
		&platid_mask_MACH_VICTOR_INTERLINK_MPC304,
		USE_WINCE_CLKMASK,		/* clock mask */
		(1 << VRC4173ICU_USBINTR)|	/* intrrupts */
		(1 << VRC4173ICU_PCMCIA1INTR)|
		(1 << VRC4173ICU_PCMCIA2INTR),
	},
	{
		&platid_mask_MACH_NEC_MCR_SIGMARION2,
		USE_WINCE_CLKMASK,		/* clock mask */
		(1 << VRC4173ICU_USBINTR),	/* intrrupts */
	},
	{
		&platid_wild,
		USE_WINCE_CLKMASK,	/* XXX */
		-1,
	},
};

struct vrc4173bcu_unit {
	char	*vu_name;
	int	vu_intr[2];
	int	vu_clkmask;
	bus_addr_t	vu_lreg;
	bus_addr_t	vu_mlreg;
	bus_addr_t	vu_hreg;
	bus_addr_t	vu_mhreg;
};

struct vrc4173bcu_softc {
	struct device sc_dev;
	struct vrip_chipset_tag sc_chipset;
	struct vrcmu_chipset_tag sc_cmuchip;

	pci_chipset_tag_t sc_pc;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_size_t sc_size;

	bus_space_handle_t sc_icuh;	/* I/O handle for ICU. */
	bus_space_handle_t sc_cmuh;	/* I/O handle for CMU. */
	void *sc_ih;
#define VRC4173BCU_NINTRS	16
	int sc_intrmask;
	struct vrc4173bcu_intrhand {
		int	(*ih_fun)(void *);
		void	*ih_arg;
		const struct vrc4173bcu_unit *ih_unit;
	} sc_intrhands[32];

	struct vrc4173bcu_unit *sc_units;
	int sc_nunits;
	int sc_pri;

	struct vrc4173bcu_platdep *sc_platdep;
};

#define VALID_UNIT(sc, unit)	(0 <= (unit) && (unit) < (sc)->sc_nunits)

static struct vrc4173bcu_unit vrc4173bcu_units[] = {
	[VRIP_UNIT_KIU] = {
		"kiu",
		{ VRC4173ICU_KIUINTR,	},
		VRC4173CMU_CLKMSK_KIU,
		VRC4173ICU_KIUINT,	VRC4173ICU_MKIUINT,
	},
	[VRIP_UNIT_PIU] = {
		"piu",
		{ VRC4173ICU_PIUINTR, VRC4173ICU_DOZEPIUINTR},
		VRC4173CMU_CLKMSK_PIU,
		VRC4173ICU_PIUINT,	VRC4173ICU_MPIUINT,
	},
	[VRIP_UNIT_AIU] = {
		"aiu",
		{ VRC4173ICU_AIUINTR,	},
		VRC4173CMU_CLKMSK_AIU,
		VRC4173ICU_AIUINT,	VRC4173ICU_MAIUINT,
	},
	[VRIP_UNIT_GIU] = {
		"giu",
		{ VRC4173ICU_GIUINTR,	},
		0,
		VRC4173ICU_GIULINT,	VRC4173ICU_MGIULINT,
		VRC4173ICU_GIUHINT,	VRC4173ICU_MGIUHINT,
	},
	[VRIP_UNIT_PS2U0] = {
		"PS/2-Ch1",
		{ VRC4173ICU_PS2CH1INTR,	},
		VRC4173CMU_CLKMSK_PS2CH1,
	},
	[VRIP_UNIT_PS2U1] = {
		"PS/2-Ch2",
		{ VRC4173ICU_PS2CH2INTR,	},
		VRC4173CMU_CLKMSK_PS2CH2,
	},
	[VRIP_UNIT_USBU] = {
		"usbu",
		{ VRC4173ICU_USBINTR,	},
		VRC4173CMU_CLKMSK_USB,
	},
	[VRIP_UNIT_CARDU0] = {
		"cardu0",
		{ VRC4173ICU_PCMCIA1INTR,	},
		VRC4173CMU_CLKMSK_CARD1,
	},
	[VRIP_UNIT_CARDU1] = {
		"cardu1",
		{ VRC4173ICU_PCMCIA2INTR,	},
		VRC4173CMU_CLKMSK_CARD2,
	},
};

CFATTACH_DECL(vrc4173bcu, sizeof(struct vrc4173bcu_softc),
    vrc4173bcu_match, vrc4173bcu_attach, NULL, NULL);

static const struct vrip_chipset_tag vrc4173bcu_chipset_methods = {
	.vc_power		= __vrc4173bcu_power,
	.vc_intr_establish	= __vrc4173bcu_intr_establish,
	.vc_intr_disestablish	= __vrc4173bcu_intr_disestablish,
	.vc_intr_setmask1	= __vrc4173bcu_intr_setmask1,
	.vc_intr_setmask2	= __vrc4173bcu_intr_setmask2,
	.vc_intr_getstatus2	= __vrc4173bcu_intr_getstatus2,
	.vc_register_cmu	= __vrc4173bcu_register_cmu,
	.vc_register_gpio	= __vrc4173bcu_register_gpio,
	.vc_register_dmaau	= __vrc4173bcu_register_dmaau,
	.vc_register_dcu	= __vrc4173bcu_register_dcu,
};

int
vrc4173bcu_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_NEC &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_NEC_VRC4173_BCU)
		return (1);
 
	return (0);
}

void
vrc4173bcu_attach(struct device *parent, struct device *self, void *aux)
{
	struct vrc4173bcu_softc *sc = (struct vrc4173bcu_softc *)self;
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	pcitag_t tag = pa->pa_tag;
	pcireg_t csr;
	char devinfo[256];
	u_int16_t reg;
	pci_intr_handle_t ih;
	const char *intrstr;
	int bus, device, function;
#ifdef DEBUG
	char buf[80];
#endif

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	printf(": %s (rev. 0x%02x)\n", devinfo, PCI_REVISION(pa->pa_class));

#if 0
	printf("%s: ", sc->sc_dev.dv_xname);
	pci_conf_print(pa->pa_pc, pa->pa_tag, NULL);
#endif

	sc->sc_pc = pc;
	sc->sc_cmuchip.cc_sc = sc;
	sc->sc_cmuchip.cc_clock = __vrc4173bcu_clock;
	sc->sc_units = vrc4173bcu_units;
	sc->sc_nunits = sizeof(vrc4173bcu_units)/sizeof(struct vrc4173bcu_unit);
	sc->sc_chipset = vrc4173bcu_chipset_methods; /* structure assignment */
	sc->sc_chipset.vc_sc = sc;

	sc->sc_platdep = platid_search(&platid, platdep_table,
	    sizeof(platdep_table)/sizeof(*platdep_table),
	    sizeof(*platdep_table));

	/* Map I/O registers */
	if (pci_mapreg_map(pa, VRC4173BCU_BADR, PCI_MAPREG_TYPE_IO, 0,
	    &sc->sc_iot, &sc->sc_ioh, NULL, &sc->sc_size)) {
		printf("%s: can't map mem space\n", sc->sc_dev.dv_xname);
		return;
	}

	/* Enable the device. */
	csr = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
	DPRINTF(("%s: csr = 0x%08x", sc->sc_dev.dv_xname, csr));
	pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG,
	    csr | PCI_COMMAND_MASTER_ENABLE | PCI_COMMAND_IO_ENABLE);
	csr = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
	DPRINTF((" -> 0x%08x\n", csr));

	csr = pci_conf_read(pc, tag, VRC4173BCU_BADR);
	DPRINTF(("%s: base addr = %x@0x%08x\n", sc->sc_dev.dv_xname,
	    (int)sc->sc_size, csr));
	DPRINTF(("%s: iot = 0x%08x, ioh = 0x%08x\n", sc->sc_dev.dv_xname,
	    (int)sc->sc_iot, (int)sc->sc_ioh));

	/*
	 * Map I/O space for ICU.
	 */
	if (bus_space_subregion(sc->sc_iot, sc->sc_ioh,
	    VRC4173ICU_IOBASE, VRC4173ICU_IOSIZE, &sc->sc_icuh)) {
		printf(": can't map ICU i/o space\n");
		return;
	}

	/*
	 * Map I/O space for CMU.
	 */
	if (bus_space_subregion(sc->sc_iot, sc->sc_ioh,
	    VRC4173CMU_IOBASE, VRC4173CMU_IOSIZE, &sc->sc_cmuh)) {
		printf(": can't map CMU i/o space\n");
		return;
	}

	/* machine dependent setup */
	if (sc->sc_platdep->clkmask == USE_WINCE_CLKMASK) {
		/* XXX, You can nothing! */
		reg = bus_space_read_2(sc->sc_iot, sc->sc_cmuh,
		    VRC4173CMU_CLKMSK);
		printf("%s: default clock mask is %04x\n",
		    sc->sc_dev.dv_xname, reg);
	} else {
		/* assert all reset bits */
		bus_space_write_2(sc->sc_iot, sc->sc_cmuh, VRC4173CMU_SRST,
		    VRC4173CMU_SRST_AC97 | VRC4173CMU_SRST_USB |
		    VRC4173CMU_SRST_CARD2 | VRC4173CMU_SRST_CARD1);
		/* set clock mask */
		bus_space_write_2(sc->sc_iot, sc->sc_cmuh,
		    VRC4173CMU_CLKMSK, sc->sc_platdep->clkmask);
		/* clear reset bit */
		bus_space_write_2(sc->sc_iot, sc->sc_cmuh, VRC4173CMU_SRST, 0);
	}

#ifdef DEBUG
	reg = bus_space_read_2(sc->sc_iot, sc->sc_icuh, VRC4173ICU_SYSINT1);
	bitmask_snprintf(reg,
	    "\20\1USB\2PCMCIA2\3PCMCIA1\4PS2CH2\5PS2CH1\6PIU\7AIU\10KIU"
	    "\11GIU\12AC97\13AC97-1\14B11\15B12\16DOZEPIU\17B14\20B15",
	    buf, sizeof(buf));
	printf("%s: SYSINT1 = 0x%s\n", sc->sc_dev.dv_xname, buf);

	reg = bus_space_read_2(sc->sc_iot, sc->sc_icuh, VRC4173ICU_MKIUINT);
	bitmask_snprintf(reg,
	    "\20\1SCANINT\2KDATRDY\3KDATLOST\4B3\5B4\6B5\7B6\10B7"
	    "\11B8\12B9\13B10\14B11\15B12\16B13\17B14\20B15",
	    buf, sizeof(buf));
	printf("%s: MKIUINT = 0x%s\n", sc->sc_dev.dv_xname, buf);

	reg = bus_space_read_2(sc->sc_iot, sc->sc_icuh, VRC4173ICU_MSYSINT1);
	bitmask_snprintf(reg,
	    "\20\1USB\2PCMCIA2\3PCMCIA1\4PS2CH2\5PS2CH1\6PIU\7AIU\10KIU"
	    "\11GIU\12AC97\13AC97-1\14B11\15B12\16DOZEPIU\17B14\20B15",
	    buf, sizeof(buf));
	printf("%s: MSYSINT1 = 0x%s\n", sc->sc_dev.dv_xname, buf);

#if 1
	reg = VRC4173ICU_USBINTR | VRC4173ICU_PIUINTR | VRC4173ICU_KIUINTR |
	    VRC4173ICU_DOZEPIUINTR;
	bus_space_write_2(sc->sc_iot, sc->sc_icuh, VRC4173ICU_MSYSINT1, reg);

	reg = bus_space_read_2(sc->sc_iot, sc->sc_icuh, VRC4173ICU_MSYSINT1);
	bitmask_snprintf(reg,
	    "\20\1USB\2PCMCIA2\3PCMCIA1\4PS2CH2\5PS2CH1\6PIU\7AIU\10KIU"
	    "\11GIU\12AC97\13AC97-1\14B11\15B12\16DOZEPIU\17B14\20B15",
	    buf, sizeof(buf));
	printf("%s: MSYSINT1 = 0x%s\n", sc->sc_dev.dv_xname, buf);
#endif
#endif

	/*
	 * set interrupt mask
	 */
	sc->sc_intrmask = sc->sc_platdep->intrmask;
	bus_space_write_2(sc->sc_iot, sc->sc_icuh, VRC4173ICU_MSYSINT1,
	    sc->sc_intrmask);

	/*
	 * install interrupt handler
	 */
	if (pci_intr_map(pa, &ih)) {
		printf("%s: couldn't map interrupt\n", sc->sc_dev.dv_xname);
		return;
	}
	intrstr = pci_intr_string(pc, ih);
	sc->sc_ih = pci_intr_establish(pc, ih, IPL_NET, vrc4173bcu_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt",
		    sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	printf("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);

	/*
	 * install pci intr hooks
	 */
	pci_decompose_tag(pc, pa->pa_intrtag, &bus, &device, &function);
	/* USB unit */
	if (sc->sc_intrmask & (1 << VRC4173ICU_USBINTR))
		vrip_intr_establish(&sc->sc_chipset, VRIP_UNIT_USBU, 0,
		    IPL_NET, vrc4173bcu_pci_intr,
		    config_connect(CONFIG_HOOK_PCIINTR,
			CONFIG_HOOK_PCIINTR_ID(bus, device, 2)));
	/* PC card unit 1 */
	if (sc->sc_intrmask & (1 << VRC4173ICU_PCMCIA1INTR))
		vrip_intr_establish(&sc->sc_chipset, VRIP_UNIT_CARDU0, 0,
		    IPL_NET, vrc4173bcu_pci_intr,
		    config_connect(CONFIG_HOOK_PCIINTR,
			CONFIG_HOOK_PCIINTR_ID(bus, 1, 0)));
	/* PC card unit 2 */
	if (sc->sc_intrmask & (1 << VRC4173ICU_PCMCIA2INTR))
		vrip_intr_establish(&sc->sc_chipset, VRIP_UNIT_CARDU1, 0,
		    IPL_NET, vrc4173bcu_pci_intr,
		    config_connect(CONFIG_HOOK_PCIINTR,
			CONFIG_HOOK_PCIINTR_ID(bus, 2, 0)));

	/*
	 *  Attach each devices
	 *  sc->sc_pri = 2~1
	 */
	for (sc->sc_pri = 2; 0 < sc->sc_pri; sc->sc_pri--)
		config_search(vrc4173bcu_search, self, vrc4173bcu_print);
}

int
vrc4173bcu_print(void *aux, const char *hoge)
{
	struct vrip_attach_args *va = (struct vrip_attach_args*)aux;

	if (va->va_addr != VRIPIFCF_ADDR_DEFAULT)
		aprint_normal(" addr 0x%04lx", va->va_addr);
	if (va->va_size != VRIPIFCF_SIZE_DEFAULT)
		aprint_normal("-%04lx",
		    (va->va_addr + va->va_size - 1) & 0xffff);
	if (va->va_addr2 != VRIPIFCF_ADDR2_DEFAULT)
		aprint_normal(", 0x%04lx", va->va_addr2);
	if (va->va_size2 != VRIPIFCF_SIZE2_DEFAULT)
		aprint_normal("-%04lx",
		    (va->va_addr2 + va->va_size2 - 1) & 0xffff);

	return (UNCONF);
}

int
vrc4173bcu_search(struct device *parent, struct cfdata *cf, void *aux)
{
	struct vrc4173bcu_softc *sc = (struct vrc4173bcu_softc *)parent;
	struct vrip_attach_args va;

	memset(&va, 0, sizeof(va));
	va.va_vc = &sc->sc_chipset;
	va.va_iot = sc->sc_iot;
	va.va_parent_ioh = sc->sc_ioh;
	va.va_unit = cf->cf_loc[VRIPIFCF_UNIT];
	va.va_addr = cf->cf_loc[VRIPIFCF_ADDR];
	va.va_size = cf->cf_loc[VRIPIFCF_SIZE];
	va.va_addr2 = cf->cf_loc[VRIPIFCF_ADDR2];
	va.va_size2 = cf->cf_loc[VRIPIFCF_SIZE2];
	va.va_gpio_chips = NULL;	/* XXX */
	va.va_cc = sc->sc_chipset.vc_cc;
	va.va_ac = sc->sc_chipset.vc_ac;
	va.va_dc = sc->sc_chipset.vc_dc;
	if ((config_match(parent, cf, &va) == sc->sc_pri))
		config_attach(parent, cf, &va, vrc4173bcu_print);

	return (0);
}

int
vrc4173bcu_intr(void *arg)
{
	struct vrc4173bcu_softc *sc = (struct vrc4173bcu_softc *)arg;
	u_int16_t reg;
	struct vrc4173bcu_intrhand *ih;
	int i;

	reg = bus_space_read_2(sc->sc_iot, sc->sc_icuh, VRC4173ICU_SYSINT1);
	reg &= sc->sc_intrmask;
	if (reg == 0)
		return (0);

#if 0
    {
	char buf[80];
	bitmask_snprintf(reg,
	    "\20\1USB\2PCMCIA2\3PCMCIA1\4PS2CH2\5PS2CH1\6PIU\7AIU\10KIU"
	    "\11GIU\12AC97\13AC97-1\14B11\15B12\16DOZEPIU\17B14\20B15",
	    buf, sizeof(buf));
	printf("%s: %s\n", sc->sc_dev.dv_xname, buf);
    }
#endif
	for (ih = sc->sc_intrhands, i = 0; i < VRC4173BCU_NINTRS; i++, ih++)
		if ((reg & (1 << i)) && ih->ih_fun != NULL)
			ih->ih_fun(ih->ih_arg);

	return (1);
}

static int
vrc4173bcu_pci_intr(void *arg)
{
	config_call_tag ct = (config_call_tag)arg;
	config_connected_call(ct, NULL);

	return (0);
}

#ifdef VRC4173BCU_DEBUG
static void
vrc4173bcu_dump_level2mask(vrip_chipset_tag_t vc, vrip_intr_handle_t handle)
{
	struct vrc4173bcu_softc *sc = vc->vc_sc;
	struct vrc4173bcu_intrhand *ih = handle;
	const struct vrc4173bcu_unit *vu = ih->ih_unit;
	u_int32_t reg;
    
	if (vu->vu_mlreg) {
		DPRINTF(("level1[%d] level2 mask:", vu->vu_intr[0]));
		reg = bus_space_read_2(sc->sc_iot, sc->sc_icuh, vu->vu_mlreg);
		if (vu->vu_mhreg) {
			reg |= (bus_space_read_2(sc->sc_iot, sc->sc_icuh,
			    vu->vu_mhreg) << 16);
			dbg_bit_print(reg);
		} else
			dbg_bit_print(reg);
	}
}
#endif

int
__vrc4173bcu_power(vrip_chipset_tag_t vc, int unit, int onoff)
{
	struct vrc4173bcu_softc *sc = vc->vc_sc;
	const struct vrc4173bcu_unit *vu;

	if (sc->sc_chipset.vc_cc == NULL)
		return (0);	/* You have no clock mask unit yet. */
	if (!VALID_UNIT(sc, unit))
		return (0);
	vu = &sc->sc_units[unit];

	return (*sc->sc_chipset.vc_cc->cc_clock)(sc->sc_chipset.vc_cc, 
	    vu->vu_clkmask, onoff);
}

vrip_intr_handle_t
__vrc4173bcu_intr_establish(vrip_chipset_tag_t vc, int unit, int line,
    int level, int (*ih_fun)(void *), void *ih_arg)
{
	struct vrc4173bcu_softc *sc = vc->vc_sc;
	const struct vrc4173bcu_unit *vu;
	struct vrc4173bcu_intrhand *ih;

	if (!VALID_UNIT(sc, unit))
		return (NULL);
	vu = &sc->sc_units[unit];
	ih = &sc->sc_intrhands[vu->vu_intr[line]];
	if (ih->ih_fun) /* Can't share level 1 interrupt */
		return (NULL);
	ih->ih_fun = ih_fun;
	ih->ih_arg = ih_arg;
	ih->ih_unit = vu;
    
	/* Mask level 2 interrupt mask register. (disable interrupt) */
	vrip_intr_setmask2(vc, ih, ~0, 0);
	/* Unmask  Level 1 interrupt mask register (enable interrupt) */
	vrip_intr_setmask1(vc, ih, 1);

	return ((void *)ih);
}

void
__vrc4173bcu_intr_disestablish(vrip_chipset_tag_t vc, vrip_intr_handle_t handle)
{
	struct vrc4173bcu_intrhand *ih = handle;

	/* Mask  Level 1 interrupt mask register (disable interrupt) */
	vrip_intr_setmask1(vc, ih, 0);
	/* Mask level 2 interrupt mask register(if any). (disable interrupt) */
	vrip_intr_setmask2(vc, ih, ~0, 0);
	ih->ih_fun = NULL;
	ih->ih_arg = NULL;
}

/* Set level 1 interrupt mask. */
void
__vrc4173bcu_intr_setmask1(vrip_chipset_tag_t vc, vrip_intr_handle_t handle,
    int enable)
{
	struct vrc4173bcu_softc *sc = vc->vc_sc;
	struct vrc4173bcu_intrhand *ih = handle;
	int level1 = ih - sc->sc_intrhands;

	DPRINTF(("__vrc4173bcu_intr_setmask1: SYSINT: %s %d\n",
		 enable ? "enable" : "disable", level1));
	if (enable)
		sc->sc_intrmask |= (1 << level1);
	else
		sc->sc_intrmask &= ~(1 << level1);	
	bus_space_write_2 (sc->sc_iot, sc->sc_icuh, VRC4173ICU_MSYSINT1,
	    sc->sc_intrmask);
#ifdef VRC4173BCU_DEBUG
	if (vrc4173bcu_debug)
		dbg_bit_print(sc->sc_intrmask);
#endif
    
	return;
}

/* Get level 2 interrupt status */
void
__vrc4173bcu_intr_getstatus2(vrip_chipset_tag_t vc, vrip_intr_handle_t handle,
    u_int32_t *status /* Level 2 status */)
{
	struct vrc4173bcu_softc *sc = vc->vc_sc;
	struct vrc4173bcu_intrhand *ih = handle;
	const struct vrc4173bcu_unit *vu = ih->ih_unit;
	u_int32_t reg;

	reg = bus_space_read_2(sc->sc_iot, sc->sc_icuh, vu->vu_lreg);
	reg |= (bus_space_read_2(sc->sc_iot, sc->sc_icuh,  vu->vu_hreg) << 16);
	*status = reg;
}

/* Set level 2 interrupt mask. */
void
__vrc4173bcu_intr_setmask2(vrip_chipset_tag_t vc, vrip_intr_handle_t handle,
    u_int32_t mask /* Level 2 mask */, int onoff)
{
	struct vrc4173bcu_softc *sc = vc->vc_sc;
	struct vrc4173bcu_intrhand *ih = handle;
	const struct vrc4173bcu_unit *vu = ih->ih_unit;
	u_int16_t reg;

	DPRINTF(("vrc4173bcu_intr_setmask2:\n"));
#ifdef VRC4173BCU_DEBUG
	if (vrc4173bcu_debug)
		vrc4173bcu_dump_level2mask(vc, handle);
#endif
	if (vu->vu_mlreg) {
		reg = bus_space_read_2(sc->sc_iot, sc->sc_icuh, vu->vu_mlreg);
		if (onoff)
			reg |= (mask & 0xffff);
		else
			reg &= ~(mask & 0xffff);
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, vu->vu_mlreg, reg);
	}
	if (vu->vu_mhreg) {
		reg = bus_space_read_2(sc->sc_iot, sc->sc_icuh, vu->vu_mhreg);
		if (onoff)
			reg |= ((mask >> 16) & 0xffff);
		else
			reg &= ~((mask >> 16) & 0xffff);
		bus_space_write_2(sc->sc_iot, sc->sc_icuh, vu->vu_mhreg, reg);
	}
#ifdef VRC4173BCU_DEBUG
	if (vrc4173bcu_debug)
		vrc4173bcu_dump_level2mask(vc, handle);
#endif

	return;
}

int
__vrc4173bcu_clock(vrcmu_chipset_tag_t cc, u_int16_t mask, int onoff)
{
	struct vrc4173bcu_softc *sc = cc->cc_sc;
	u_int16_t reg;

	reg = bus_space_read_2(sc->sc_iot, sc->sc_cmuh, VRC4173CMU_CLKMSK);
#if 0
	printf("cmu register(enter):");
	dbg_bit_print(reg);
#endif
	if (onoff)
		reg |= mask;
	else
		reg &= ~mask;
	bus_space_write_2(sc->sc_iot, sc->sc_cmuh, VRC4173CMU_CLKMSK, reg);
#if 0
	printf("cmu register(exit) :");
	dbg_bit_print(reg);
#endif
	return (0);
}

void
__vrc4173bcu_register_cmu(vrip_chipset_tag_t vc, vrcmu_chipset_tag_t cmu)
{
	vc->vc_cc = cmu;
}

void
__vrc4173bcu_register_gpio(vrip_chipset_tag_t vc, hpcio_chip_t chip)
{
	/* XXX, not implemented yet */
}

void
__vrc4173bcu_register_dmaau(vrip_chipset_tag_t vc, vrdmaau_chipset_tag_t dmaau)
{

	vc->vc_ac = dmaau;
}

void
__vrc4173bcu_register_dcu(vrip_chipset_tag_t vc, vrdcu_chipset_tag_t dcu)
{

	vc->vc_dc = dcu;
}
