/*	$NetBSD: vrc4173bcu.c,v 1.1.4.2 2002/02/11 20:08:13 jdolecek Exp $	*/

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <machine/platid.h>
#include <machine/platid_mask.h>
#include <machine/config_hook.h>

#include <hpcmips/vr/vrc4173bcuvar.h>
#include <hpcmips/vr/vrc4173icureg.h>
#include <hpcmips/vr/vrc4173cmureg.h>

#define	VRC4173BCU_BADR		0x10
#ifdef DEBUG
#define	DPRINTF(args)	printf args
#else
#define	DPRINTF(args)
#endif

#define USE_WINCE_CLKMASK	(~0)

static int	vrc4173bcu_match(struct device *, struct cfdata *, void *);
static void	vrc4173bcu_attach(struct device *, struct device *, void *);
static int	vrc4173bcu_print(void *, const char *);
static int	vrc4173bcu_intr(void *);

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

struct vrc4173bcu_softc {
	struct device sc_dev;

	pci_chipset_tag_t sc_pc;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_size_t sc_size;

	bus_space_handle_t sc_icuh;	/* I/O handle for ICU. */
	bus_space_handle_t sc_cmuh;	/* I/O handle for CMU. */
	void *sc_ih;
#define VRC4173BCU_NINTRS	16
	config_call_tag sc_calltags[VRC4173BCU_NINTRS];
	int sc_intrmask;

	struct vrc4173bcu_platdep *sc_platdep;
};

struct cfattach vrc4173bcu_ca = {
	sizeof(struct vrc4173bcu_softc), vrc4173bcu_match, vrc4173bcu_attach,
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

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo);
	printf(": %s (rev. 0x%02x)\n", devinfo, PCI_REVISION(pa->pa_class));

#if 0
	printf("%s: ", sc->sc_dev.dv_xname);
	pci_conf_print(pa->pa_pc, pa->pa_tag, NULL);
#endif

	csr = pci_conf_read(pc, tag, VRC4173BCU_BADR);
	DPRINTF(("%s: base addr = 0x%08x\n", sc->sc_dev.dv_xname, csr));

	sc->sc_platdep = platid_search(&platid, platdep_table,
	    sizeof(platdep_table)/sizeof(*platdep_table),
	    sizeof(*platdep_table));

	/* Map I/O registers */
	if (pci_mapreg_map(pa, VRC4173BCU_BADR, PCI_MAPREG_TYPE_IO, 0,
	    &sc->sc_iot, &sc->sc_ioh, NULL, &sc->sc_size)) {
		printf("%s: can't map mem space\n", sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_pc = pc;

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
	 * set interrupt mask
	 */
	sc->sc_intrmask = sc->sc_platdep->intrmask;
	bus_space_write_2(sc->sc_iot, sc->sc_icuh, VRC4173ICU_MSYSINT1,
	    sc->sc_intrmask);

	/*
	 * install pci intr hooks
	 */
	memset(sc->sc_calltags, 0, sizeof(sc->sc_calltags));
	pci_decompose_tag(pc, pa->pa_intrtag, &bus, &device, &function);
	/* USB unit */
	if (sc->sc_intrmask & (1 << VRC4173ICU_USBINTR))
		sc->sc_calltags[VRC4173ICU_USBINTR] =
		    config_connect(CONFIG_HOOK_PCIINTR,
			CONFIG_HOOK_PCIINTR_ID(bus, device, 2));
	/* PC card unit 1 */
	if (sc->sc_intrmask & (1 << VRC4173ICU_PCMCIA1INTR))
		sc->sc_calltags[VRC4173ICU_USBINTR] =
		    config_connect(CONFIG_HOOK_PCIINTR,
			CONFIG_HOOK_PCIINTR_ID(bus, 1, 0));
	/* PC card unit 2 */
	if (sc->sc_intrmask & (1 << VRC4173ICU_PCMCIA2INTR))
		sc->sc_calltags[VRC4173ICU_USBINTR] =
		    config_connect(CONFIG_HOOK_PCIINTR,
			CONFIG_HOOK_PCIINTR_ID(bus, 2, 0));

	/*
	 * Attach sub units found in vrc4173.  XXX.
	 */
	config_found(self, "vrc4173cmu", vrc4173bcu_print);
	config_found(self, "vrc4173giu", vrc4173bcu_print);
	config_found(self, "vrc4173piu", vrc4173bcu_print);
	config_found(self, "vrc4173kiu", vrc4173bcu_print);
	config_found(self, "vrc4173aiu", vrc4173bcu_print);
	config_found(self, "vrc4173ps2u", vrc4173bcu_print);
}

int
vrc4173bcu_print(void *aux, const char *pnp)
{
	const char *name = aux;

	if (pnp)
		printf("%s at %s", name, pnp);

	return (UNCONF);
}

int
vrc4173bcu_intr(void *arg)
{
	struct vrc4173bcu_softc *sc = (struct vrc4173bcu_softc *)arg;
	u_int16_t reg;
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
	for (i = 0; i < VRC4173BCU_NINTRS; i++)
		if (reg & (1 << i))
			config_connected_call(sc->sc_calltags[i], NULL);

	return (1);
}
