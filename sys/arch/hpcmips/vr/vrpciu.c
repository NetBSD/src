/*	$NetBSD: vrpciu.c,v 1.1.10.5 2002/04/17 00:03:10 nathanw Exp $	*/

/*-
 * Copyright (c) 2001 Enami Tsugutomo.
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
#include <hpcmips/vr/vrpciureg.h>

#include "pci.h"

#ifdef DEBUG
#define	DPRINTF(args)	printf args
#else
#define	DPRINTF(args)
#endif

struct vrpciu_softc {
	struct device sc_dev;

	vrip_chipset_tag_t sc_vc;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	void *sc_ih;

	struct vrc4173bcu_softc *sc_bcu; /* vrc4173bcu */

	struct hpcmips_pci_chipset sc_pc;
};

static void	vrpciu_write(struct vrpciu_softc *, int, u_int32_t);
static u_int32_t
		vrpciu_read(struct vrpciu_softc *, int);
#ifdef DEBUG
static void	vrpciu_write_2(struct vrpciu_softc *, int, u_int16_t)
	__attribute__((unused));
static u_int16_t
		vrpciu_read_2(struct vrpciu_softc *, int);
#endif
static int	vrpciu_match(struct device *, struct cfdata *, void *);
static void	vrpciu_attach(struct device *, struct device *, void *);
#if NPCI > 0
static int	vrpciu_print(void *, const char *);
#endif
static int	vrpciu_intr(void *);
static void	vrpciu_attach_hook(struct device *, struct device *,
		    struct pcibus_attach_args *);
static int	vrpciu_bus_maxdevs(pci_chipset_tag_t, int);
static int	vrpciu_bus_devorder(pci_chipset_tag_t, int, char *);
static pcitag_t	vrpciu_make_tag(pci_chipset_tag_t, int, int, int);
static void	vrpciu_decompose_tag(pci_chipset_tag_t, pcitag_t, int *, int *,
		    int *);
static pcireg_t	vrpciu_conf_read(pci_chipset_tag_t, pcitag_t, int); 
static void	vrpciu_conf_write(pci_chipset_tag_t, pcitag_t, int, pcireg_t);
static int	vrpciu_intr_map(struct pci_attach_args *, pci_intr_handle_t *);
static const char *vrpciu_intr_string(pci_chipset_tag_t, pci_intr_handle_t);
static const struct evcnt *vrpciu_intr_evcnt(pci_chipset_tag_t,
		    pci_intr_handle_t);
static void	*vrpciu_intr_establish(pci_chipset_tag_t, pci_intr_handle_t,
		    int, int (*)(void *), void *);
static void	vrpciu_intr_disestablish(pci_chipset_tag_t, void *);

struct cfattach vrpciu_ca = {
	sizeof(struct vrpciu_softc), vrpciu_match, vrpciu_attach
};

static void
vrpciu_write(struct vrpciu_softc *sc, int offset, u_int32_t val)
{

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, offset, val);
}

static u_int32_t
vrpciu_read(struct vrpciu_softc *sc, int offset)
{

	return (bus_space_read_4(sc->sc_iot, sc->sc_ioh, offset));
}

#ifdef DEBUG
static void
vrpciu_write_2(struct vrpciu_softc *sc, int offset, u_int16_t val)
{

	bus_space_write_2(sc->sc_iot, sc->sc_ioh, offset, val);
}

static u_int16_t
vrpciu_read_2(struct vrpciu_softc *sc, int offset)
{

	return (bus_space_read_2(sc->sc_iot, sc->sc_ioh, offset));
}
#endif

static int
vrpciu_match(struct device *parent, struct cfdata *match, void *aux)
{

	return (1);
}

static void
vrpciu_attach(struct device *parent, struct device *self, void *aux)
{
	struct vrpciu_softc *sc = (struct vrpciu_softc *)self;
	pci_chipset_tag_t pc = &sc->sc_pc;
	struct vrip_attach_args *va = aux;
#if defined(DEBUG) || NPCI > 0
	u_int32_t reg;
#endif
#if NPCI > 0
	struct bus_space_tag_hpcmips *iot;
	char tmpbuf[16];
	struct pcibus_attach_args pba;
#endif

	sc->sc_vc = va->va_vc;
	sc->sc_iot = va->va_iot;
	if (bus_space_map(sc->sc_iot, va->va_addr, va->va_size, 0,
	    &sc->sc_ioh)) {
		printf(": couldn't map io space\n");
		return;
	}

	sc->sc_ih = vrip_intr_establish(va->va_vc, va->va_unit, 0, IPL_TTY,
	    vrpciu_intr, sc);
	if (sc->sc_ih == NULL) {
		printf(": couldn't establish interrupt\n");
		return;
	}

	/* Enable level 2 interrupt */
	vrip_intr_setmask2(va->va_vc, sc->sc_ih, PCIINT_INT0, 1);

	printf("\n");

#ifdef DEBUG
#define	DUMP_MAW(sc, name, reg) do {					\
	printf("%s: %s =\t0x%08x\n", (sc)->sc_dev.dv_xname,		\
	    (name), (reg));						\
	printf("%s:\tIBA/MASK =\t0x%08x/0x%08x (0x%08x - 0x%08x)\n",	\
	    (sc)->sc_dev.dv_xname,					\
	    reg & VRPCIU_MAW_IBAMASK, VRPCIU_MAW_ADDRMASK(reg),		\
	    VRPCIU_MAW_ADDR(reg),					\
	    VRPCIU_MAW_ADDR(reg) + VRPCIU_MAW_SIZE(reg));		\
	printf("%s:\tWINEN =\t0x%08x\n", (sc)->sc_dev.dv_xname,		\
	    reg & VRPCIU_MAW_WINEN);					\
	printf("%s:\tPCIADR =\t0x%08x\n", (sc)->sc_dev.dv_xname,	\
	    VRPCIU_MAW_PCIADDR(reg));					\
} while (0)
#define	DUMP_TAW(sc, name, reg) do {				\
	printf("%s: %s =\t\t0x%08x\n", (sc)->sc_dev.dv_xname,	\
	    (name), (reg));					\
	printf("%s:\tMASK =\t0x%08x\n", (sc)->sc_dev.dv_xname,	\
	    VRPCIU_TAW_ADDRMASK(reg));				\
	printf("%s:\tWINEN =\t0x%08x\n", (sc)->sc_dev.dv_xname,	\
	    reg & VRPCIU_TAW_WINEN);				\
	printf("%s:\tIBA =\t0x%08x\n", (sc)->sc_dev.dv_xname,	\
	    VRPCIU_TAW_IBA(reg));				\
} while (0)
	reg = vrpciu_read(sc, VRPCIU_MMAW1REG);
	DUMP_MAW(sc, "MMAW1", reg);
	reg = vrpciu_read(sc, VRPCIU_MMAW2REG);
	DUMP_MAW(sc, "MMAW2", reg);
	reg = vrpciu_read(sc, VRPCIU_TAW1REG);
	DUMP_TAW(sc, "TAW1", reg);
	reg = vrpciu_read(sc, VRPCIU_TAW2REG);
	DUMP_TAW(sc, "TAW2", reg);
	reg = vrpciu_read(sc, VRPCIU_MIOAWREG);
	DUMP_MAW(sc, "MIOAW", reg);
	printf("%s: BUSERRAD =\t0x%08x\n", sc->sc_dev.dv_xname,
	    vrpciu_read(sc, VRPCIU_BUSERRADREG));
	printf("%s: INTCNTSTA =\t0x%08x\n", sc->sc_dev.dv_xname,
	    vrpciu_read(sc, VRPCIU_INTCNTSTAREG));
	printf("%s: EXACC =\t0x%08x\n", sc->sc_dev.dv_xname,
	    vrpciu_read(sc, VRPCIU_EXACCREG));
	printf("%s: RECONT =\t0x%08x\n", sc->sc_dev.dv_xname,
	    vrpciu_read(sc, VRPCIU_RECONTREG));
	printf("%s: PCIEN =\t0x%08x\n", sc->sc_dev.dv_xname,
	    vrpciu_read(sc, VRPCIU_ENREG));
	printf("%s: CLOCKSEL =\t0x%08x\n", sc->sc_dev.dv_xname,
	    vrpciu_read(sc, VRPCIU_CLKSELREG));
	printf("%s: TRDYV =\t0x%08x\n", sc->sc_dev.dv_xname,
	    vrpciu_read(sc, VRPCIU_TRDYVREG));
	printf("%s: CLKRUN =\t0x%08x\n", sc->sc_dev.dv_xname,
	    vrpciu_read_2(sc, VRPCIU_CLKRUNREG));
	printf("%s: IDREG =\t0x%08x\n", sc->sc_dev.dv_xname,
	    vrpciu_read(sc, VRPCIU_CONF_BASE + PCI_ID_REG));
	reg = vrpciu_read(sc, VRPCIU_CONF_BASE + PCI_COMMAND_STATUS_REG);
	printf("%s: CSR =\t\t0x%08x\n", sc->sc_dev.dv_xname, reg);
	vrpciu_write(sc, VRPCIU_CONF_BASE + PCI_COMMAND_STATUS_REG, reg);
	printf("%s: CSR =\t\t0x%08x\n", sc->sc_dev.dv_xname,
	    vrpciu_read(sc, VRPCIU_CONF_BASE + PCI_COMMAND_STATUS_REG));
	printf("%s: CLASS =\t0x%08x\n", sc->sc_dev.dv_xname,
	    vrpciu_read(sc, VRPCIU_CONF_BASE + PCI_CLASS_REG));
	printf("%s: BHLC =\t\t0x%08x\n", sc->sc_dev.dv_xname,
	    vrpciu_read(sc, VRPCIU_CONF_BASE + PCI_BHLC_REG));
	printf("%s: MAIL =\t\t0x%08x\n", sc->sc_dev.dv_xname,
	    vrpciu_read(sc, VRPCIU_CONF_BASE + VRPCIU_CONF_MAILREG));
	printf("%s: MBA1 =\t\t0x%08x\n", sc->sc_dev.dv_xname,
	    vrpciu_read(sc, VRPCIU_CONF_BASE + VRPCIU_CONF_MBA1REG));
	printf("%s: MBA2 =\t\t0x%08x\n", sc->sc_dev.dv_xname,
	    vrpciu_read(sc, VRPCIU_CONF_BASE + VRPCIU_CONF_MBA2REG));
	printf("%s: INTR =\t\t0x%08x\n", sc->sc_dev.dv_xname,
	    vrpciu_read(sc, VRPCIU_CONF_BASE + PCI_INTERRUPT_REG));
#if 0
	vrpciu_write(sc, VRPCIU_CONF_BASE + PCI_INTERRUPT_REG,
	    vrpciu_read(sc, VRPCIU_CONF_BASE + PCI_INTERRUPT_REG) | 0x01);
	printf("%s: INTR =\t\t0x%08x\n", sc->sc_dev.dv_xname,
	    vrpciu_read(sc, VRPCIU_CONF_BASE + PCI_INTERRUPT_REG));
#endif
#endif

	pc->pc_dev = &sc->sc_dev;
	pc->pc_attach_hook = vrpciu_attach_hook;
	pc->pc_bus_maxdevs = vrpciu_bus_maxdevs;
	pc->pc_bus_devorder = vrpciu_bus_devorder;
	pc->pc_make_tag = vrpciu_make_tag;
	pc->pc_decompose_tag = vrpciu_decompose_tag;
	pc->pc_conf_read = vrpciu_conf_read;
	pc->pc_conf_write = vrpciu_conf_write;
	pc->pc_intr_map = vrpciu_intr_map;
	pc->pc_intr_string = vrpciu_intr_string;
	pc->pc_intr_evcnt = vrpciu_intr_evcnt;
	pc->pc_intr_establish = vrpciu_intr_establish;
	pc->pc_intr_disestablish = vrpciu_intr_disestablish;

#if 0
	{
		int i;

		for (i = 0; i < 8; i++)
			printf("%s: ID_REG(0, 0, %d) = 0x%08x\n",
			    sc->sc_dev.dv_xname, i,
			    pci_conf_read(pc, pci_make_tag(pc, 0, 0, i),
				PCI_ID_REG));
	}
#endif

#if NPCI > 0
	memset(&pba, 0, sizeof(pba));
	pba.pba_busname = "pci";

	/* For now, just inherit window mappings set by WinCE.  XXX. */

	iot = hpcmips_alloc_bus_space_tag();
	reg = vrpciu_read(sc, VRPCIU_MIOAWREG);
	snprintf(tmpbuf, sizeof(tmpbuf), "%s/iot",
	    sc->sc_dev.dv_xname);
	hpcmips_init_bus_space(iot, (struct bus_space_tag_hpcmips *)sc->sc_iot,
	    tmpbuf, VRPCIU_MAW_ADDR(reg), VRPCIU_MAW_SIZE(reg));
	pba.pba_iot = &iot->bst;

	/*
	 * Just use system bus space tag.  It works since WinCE maps
	 * PCI bus space at same offset.  But this isn't right thing
	 * of course.  XXX.
	 */
	pba.pba_memt = sc->sc_iot;
	pba.pba_dmat = &hpcmips_default_bus_dma_tag.bdt;
	pba.pba_bus = 0;

	if (platid_match(&platid, &platid_mask_MACH_LASER5_L_BOARD)) {
		/*
		 * fix PCI device configration for L-Router.
		 */
		/* change IDE controller to native mode */
		reg = pci_conf_read(pc, pci_make_tag(pc, 0, 16, 0),
				    PCI_CLASS_REG);
		reg |= PCIIDE_INTERFACE_PCI(0) << PCI_INTERFACE_SHIFT;
		reg |= PCIIDE_INTERFACE_PCI(1) << PCI_INTERFACE_SHIFT;
		pci_conf_write(pc, pci_make_tag(pc, 0, 16, 0), PCI_CLASS_REG,
			       reg);
		/* fix broken BAR setting of fxp0, fxp1 */
		pci_conf_write(pc, pci_make_tag(pc, 0, 0, 0), PCI_MAPREG_START,
			       0x11100000);
		pci_conf_write(pc, pci_make_tag(pc, 0, 1, 0), PCI_MAPREG_START,
			       0x11200000);
	}

	pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED |
	    PCI_FLAGS_MRL_OKAY;
	pba.pba_pc = pc;

	config_found(self, &pba, vrpciu_print);
#endif
}

#if NPCI > 0
static int
vrpciu_print(void *aux, const char *pnp)
{
	struct pcibus_attach_args *pba = aux;

	if (pnp != NULL)
		printf("%s at %s", pba->pba_busname, pnp);
	else
		printf(" bus %d", pba->pba_bus);

	return (UNCONF);
}
#endif

/*
 * Handle PCI error interrupts.
 */
int
vrpciu_intr(void *arg)
{
	struct vrpciu_softc *sc = (struct vrpciu_softc *)arg;
	u_int32_t isr, baddr;

	isr = vrpciu_read(sc, VRPCIU_INTCNTSTAREG);
	baddr = vrpciu_read(sc, VRPCIU_BUSERRADREG);
	printf("%s: status=0x%08x  bad addr=0x%08x\n",
	    sc->sc_dev.dv_xname, isr, baddr);
	return ((isr & 0x0f) ? 1 : 0);
}

void
vrpciu_attach_hook(struct device *parent, struct device *self,
    struct pcibus_attach_args *pba)
{

	return;
}

int
vrpciu_bus_maxdevs(pci_chipset_tag_t pc, int busno)
{

	return (32);
}

int
vrpciu_bus_devorder(pci_chipset_tag_t pc, int busno, char *devs)
{
	int i, dev;
	char priorities[32];
	static pcireg_t ids[] = {
		/* these devices should be attached first */
		PCI_ID_CODE(PCI_VENDOR_NEC, PCI_PRODUCT_NEC_VRC4173_BCU),
	};

	/* scan PCI devices and check the id table */
	memset(priorities, 0, sizeof(priorities));
	for (dev = 0; dev < 32; dev++) {
		pcireg_t id;
		id = pci_conf_read(pc, pci_make_tag(pc, 0, dev, 0),PCI_ID_REG);
		for (i = 0; i < sizeof(ids)/sizeof(*ids); i++)
			if (id == ids[i])
				priorities[dev] = 1;
	}

	/* fill order array */
	for (i = 1; 0 <= i; i--)
		for (dev = 0; dev < 32; dev++)
			if (priorities[dev] == i)
				*devs++ = dev;

	return (32);
}

pcitag_t
vrpciu_make_tag(pci_chipset_tag_t pc, int bus, int device, int function)
{

	return ((bus << 16) | (device << 11) | (function << 8));
}

void
vrpciu_decompose_tag(pci_chipset_tag_t pc, pcitag_t tag, int *bp, int *dp,
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
vrpciu_conf_read(pci_chipset_tag_t pc, pcitag_t tag, int reg)
{
	struct vrpciu_softc *sc = (struct vrpciu_softc *)pc->pc_dev;
	u_int32_t val;
	int bus, device, function;

	pci_decompose_tag(pc, tag, &bus, &device, &function);
	if (bus == 0) {
		if (device > 21)
			return ((pcitag_t)-1);
		tag = (1 << (device + 11)) | (function << 8); /* Type 0 */
	} else
		tag |= VRPCIU_CONF_TYPE1;

	vrpciu_write(sc, VRPCIU_CONFAREG, tag | reg);
	val = vrpciu_read(sc, VRPCIU_CONFDREG);
#if 0
	printf("%s: conf_read: tag = 0x%08x, reg = 0x%x, val = 0x%08x\n",
	    sc->sc_dev.dv_xname, (u_int32_t)tag, reg, val);
#endif
	return (val);
}

void
vrpciu_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg,
    pcireg_t data)
{
	struct vrpciu_softc *sc = (struct vrpciu_softc *)pc->pc_dev;
	int bus, device, function;

#if 0
	printf("%s: conf_write: tag = 0x%08x, reg = 0x%x, val = 0x%08x\n",
	    sc->sc_dev.dv_xname, (u_int32_t)tag, reg, (u_int32_t)data);
#endif
	vrpciu_decompose_tag(pc, tag, &bus, &device, &function);
	if (bus == 0) {
		if (device > 21)
			return;
		tag = (1 << (device + 11)) | (function << 8); /* Type 0 */
	} else
		tag |= VRPCIU_CONF_TYPE1;

	vrpciu_write(sc, VRPCIU_CONFAREG, tag | reg);
	vrpciu_write(sc, VRPCIU_CONFDREG, data);
}

int
vrpciu_intr_map(struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	pci_chipset_tag_t pc = pa->pa_pc;
	pcitag_t intrtag = pa->pa_intrtag;
	int bus, dev, func;
#ifdef DEBUG
	int line = pa->pa_intrline;
	int pin = pa->pa_intrpin;
#endif

	pci_decompose_tag(pc, intrtag, &bus, &dev, &func);
	DPRINTF(("%s(%d, %d, %d): line = %d, pin = %d\n", pc->pc_dev->dv_xname,
	    bus, dev, func, line, pin));

	*ihp = CONFIG_HOOK_PCIINTR_ID(bus, dev, func);

	return (0);
}

const char *
vrpciu_intr_string(pci_chipset_tag_t pc, pci_intr_handle_t ih)
{
	static char irqstr[sizeof("pciintr") + 16];

	snprintf(irqstr, sizeof(irqstr), "pciintr %d:%d:%d",
	    CONFIG_HOOK_PCIINTR_BUS((int)ih),
	    CONFIG_HOOK_PCIINTR_DEVICE((int)ih),
	    CONFIG_HOOK_PCIINTR_FUNCTION((int)ih));

	return (irqstr);
}

const struct evcnt *
vrpciu_intr_evcnt(pci_chipset_tag_t pc, pci_intr_handle_t ih)
{

	/* XXX for now, no evcnt parent reported */

	return (NULL);
}

void *
vrpciu_intr_establish(pci_chipset_tag_t pc, pci_intr_handle_t ih, int level,
    int (*func)(void *), void *arg)
{

	if (ih == -1)
		return (NULL);
	DPRINTF(("vrpciu_intr_establish: %lx\n", ih));

	return (config_hook(CONFIG_HOOK_PCIINTR, ih, CONFIG_HOOK_EXCLUSIVE,
	    (int (*)(void *, int, long, void *))func, arg));
}

void
vrpciu_intr_disestablish(pci_chipset_tag_t pc, void *cookie)
{

	DPRINTF(("vrpciu_intr_disestablish: %p\n", cookie));
	config_unhook(cookie);
}
