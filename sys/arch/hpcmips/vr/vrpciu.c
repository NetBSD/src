/*	$NetBSD: vrpciu.c,v 1.1 2001/06/13 07:32:48 enami Exp $	*/

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

#define	_HPCMIPS_BUS_DMA_PRIVATE	/* XXX */
#include <machine/bus.h>

#include <dev/pci/pcivar.h>

#include <hpcmips/vr/icureg.h>
#include <hpcmips/vr/vripvar.h>
#include <hpcmips/vr/vrpciureg.h>
#include <hpcmips/vr/vrc4173bcuvar.h>

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
static pcitag_t	vrpciu_make_tag(pci_chipset_tag_t, int, int, int);
static void	vrpciu_decompose_tag(pci_chipset_tag_t, pcitag_t, int *, int *,
		    int *);
static pcireg_t	vrpciu_conf_read(pci_chipset_tag_t, pcitag_t, int); 
static void	vrpciu_conf_write(pci_chipset_tag_t, pcitag_t, int, pcireg_t);
static void	*vrpciu_intr_establish(pci_chipset_tag_t, pci_intr_handle_t,
		    int, int (*)(void *), void *);
static void	vrpciu_intr_disestablish(pci_chipset_tag_t, void *);
static void	*vrpciu_vrcintr_establish(pci_chipset_tag_t, int,
		    int (*)(void *), void *);
static void	vrpciu_vrcintr_disestablish(pci_chipset_tag_t, void *);

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
	bus_space_tag_t iot;
	u_int32_t reg;
#if NPCI > 0
	struct pcibus_attach_args pba;
#endif

	sc->sc_vc = va->va_vc;
	sc->sc_iot = va->va_iot;
	if (bus_space_map(sc->sc_iot, va->va_addr, va->va_size, 0,
	    &sc->sc_ioh)) {
		printf(": couldn't map io space\n");
		return;
	}

	sc->sc_ih = vrip_intr_establish(va->va_vc, va->va_intr, IPL_TTY,
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
	pc->pc_bus_devorder = vrc4173bcu_pci_bus_devorder;
	pc->pc_make_tag = vrpciu_make_tag;
	pc->pc_decompose_tag = vrpciu_decompose_tag;
	pc->pc_conf_read = vrpciu_conf_read;
	pc->pc_conf_write = vrpciu_conf_write;
	pc->pc_intr_map = vrc4173bcu_pci_intr_map;
	pc->pc_intr_string = vrc4173bcu_pci_intr_string;
	pc->pc_intr_evcnt = vrc4173bcu_pci_intr_evcnt;
	pc->pc_intr_establish = vrpciu_intr_establish;
	pc->pc_intr_disestablish = vrpciu_intr_disestablish;
	pc->pc_vrcintr_establish = vrpciu_vrcintr_establish;
	pc->pc_vrcintr_disestablish = vrpciu_vrcintr_disestablish;

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

	pba.pba_iot = iot = hpcmips_alloc_bus_space_tag();
	reg = vrpciu_read(sc, VRPCIU_MIOAWREG);
	iot->t_base = VRPCIU_MAW_ADDR(reg);
	iot->t_size = VRPCIU_MAW_SIZE(reg);
	snprintf(iot->t_name, sizeof(iot->t_name), "%s/iot",
	    sc->sc_dev.dv_xname);
	hpcmips_init_bus_space_extent(iot);

	/*
	 * Just use system bus space tag.  It works since WinCE maps
	 * PCI bus space at same offset.  But this isn't right thing
	 * of course.  XXX.
	 */
	pba.pba_memt = sc->sc_iot;
	pba.pba_dmat = &hpcmips_default_bus_dma_tag;
	pba.pba_bus = 0;
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
	u_int32_t isr;

	isr = vrpciu_read(sc, VRPCIU_INTCNTSTAREG);
	printf("%s: vrpciu_intr 0x%08x\n", sc->sc_dev.dv_xname, isr);
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

void *
vrpciu_intr_establish(pci_chipset_tag_t pc, pci_intr_handle_t ih, int level,
    int (*func)(void *), void *arg)
{
	struct vrpciu_softc *sc = (struct vrpciu_softc *)pc->pc_dev;

	if (ih == -1)
		return (NULL);
	DPRINTF(("vrpciu_intr_establish: %p\n", sc));
	return (vrc4173bcu_intr_establish(sc->sc_bcu, ih, func, arg));
}

void
vrpciu_intr_disestablish(pci_chipset_tag_t pc, void *cookie)
{
	struct vrpciu_softc *sc = (struct vrpciu_softc *)pc->pc_dev;

	DPRINTF(("vrpciu_intr_disestablish: %p\n", sc));
	vrc4173bcu_intr_disestablish(sc->sc_bcu, cookie);
}

void *
vrpciu_vrcintr_establish(pci_chipset_tag_t pc, int port,
    int (*func)(void *), void *arg)
{
	struct vrpciu_softc *sc = (struct vrpciu_softc *)pc->pc_dev;
	struct vrip_softc *vsc = (struct vrip_softc *)sc->sc_vc;
	void *ih;

	sc->sc_bcu = arg;
	ih = hpcio_intr_establish(vsc->sc_gpio_chips[VRIP_IOCHIP_VRGIU],
	    port, HPCIO_INTR_LEVEL | HPCIO_INTR_LOW | HPCIO_INTR_HOLD,
	    func, arg);

	return (ih);
}

void
vrpciu_vrcintr_disestablish(pci_chipset_tag_t pc, void *ih)
{
	struct vrpciu_softc *sc = (struct vrpciu_softc *)pc->pc_dev;
	struct vrip_softc *vsc = (struct vrip_softc *)sc->sc_vc;

	return (vrip_intr_disestablish(vsc->sc_gpio_chips[VRIP_IOCHIP_VRGIU],
	    ih));
}
