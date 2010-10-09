/*	$NetBSD: pci_machdep.c,v 1.1.2.2 2010/10/09 03:31:40 yamt Exp $	*/
/*
 * Copyright (c) 2008 KIYOHARA Takashi
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pci_machdep.c,v 1.1.2.2 2010/10/09 03:31:40 yamt Exp $");

#include "opt_mvsoc.h"
#include "gtpci.h"
#include "mvpex.h"
#include "pci.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/extent.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>

#include <arm/marvell/mvsocreg.h>
#include <arm/marvell/mvsocvar.h>
#include <arm/marvell/mvsocgppvar.h>
#if NGTPCI > 0
#include <dev/marvell/gtpcireg.h>
#include <dev/marvell/gtpcivar.h>
#endif
#if NMVPEX > 0
#include <dev/marvell/mvpexreg.h>
#include <dev/marvell/mvpexvar.h>
#endif

#include <machine/pci_machdep.h>

#if defined(ORION)
#include <arm/marvell/orionreg.h>
#endif
#if defined(KIRKWOOD)
#include <arm/marvell/kirkwoodreg.h>
#endif
#include <dev/marvell/marvellreg.h>


#if NGTPCI > 0
#if NGTPCI_MBUS > 0
static pcireg_t gtpci_mbus_conf_read(void *, pcitag_t, int);
static void gtpci_mbus_conf_write(void *, pcitag_t, int, pcireg_t);
#endif
static int gtpci_gpp_intr_map(struct pci_attach_args *, pci_intr_handle_t *);
static const char *gtpci_gpp_intr_string(void *, pci_intr_handle_t);
static const struct evcnt *gtpci_gpp_intr_evcnt(void *, pci_intr_handle_t);
static void *gtpci_gpp_intr_establish(void *, pci_intr_handle_t, int, int (*)(void *), void *);
static void gtpci_gpp_intr_disestablish(void *, void *);

struct arm32_pci_chipset arm32_gtpci_chipset = {
	NULL,	/* conf_v */
	gtpci_attach_hook,
	gtpci_bus_maxdevs,
	gtpci_make_tag,
	gtpci_decompose_tag,
#if NGTPCI_MBUS > 0
	gtpci_mbus_conf_read,		/* XXXX: always this functions */
	gtpci_mbus_conf_write,
#else
	gtpci_conf_read,
	gtpci_conf_write,
#endif
	NULL,	/* intr_v */
	gtpci_gpp_intr_map,
	gtpci_gpp_intr_string,
	gtpci_gpp_intr_evcnt,
	gtpci_gpp_intr_establish,
	gtpci_gpp_intr_disestablish,
#ifdef __HAVE_PCI_CONF_HOOK
	gtpci_conf_hook,
#endif
};
#endif

#if NMVPEX > 0
#if NMVPEX_MBUS > 0
static pcireg_t mvpex_mbus_conf_read(void *, pcitag_t, int);
#endif

struct arm32_pci_chipset arm32_mvpex0_chipset = {
	NULL,	/* conf_v */
	mvpex_attach_hook,
	mvpex_bus_maxdevs,
	mvpex_make_tag,
	mvpex_decompose_tag,
#if NMVPEX_MBUS > 0
	mvpex_mbus_conf_read,		/* XXXX: always this functions */
#else
	mvpex_conf_read,
#endif
	mvpex_conf_write,
	NULL,	/* intr_v */
	mvpex_intr_map,
	mvpex_intr_string,
	mvpex_intr_evcnt,
	mvpex_intr_establish,
	mvpex_intr_disestablish,
#ifdef __HAVE_PCI_CONF_HOOK
	mvpex_conf_hook,
#endif
};
struct arm32_pci_chipset arm32_mvpex1_chipset = {
	NULL,	/* conf_v */
	mvpex_attach_hook,
	mvpex_bus_maxdevs,
	mvpex_make_tag,
	mvpex_decompose_tag,
#if NMVPEX_MBUS > 0
	mvpex_mbus_conf_read,		/* XXXX: always this functions */
#else
	mvpex_conf_read,
#endif
	mvpex_conf_write,
	NULL,	/* intr_v */
	mvpex_intr_map,
	mvpex_intr_string,
	mvpex_intr_evcnt,
	mvpex_intr_establish,
	mvpex_intr_disestablish,
#ifdef __HAVE_PCI_CONF_HOOK
	mvpex_conf_hook,
#endif
};
#endif


void
pci_conf_interrupt(pci_chipset_tag_t v, int bus, int dev, int pin, int swiz,
		   int *iline)
{

	/* nothing */
}


#if NGTPCI > 0
#if NGTPCI_MBUS > 0
#define GTPCI_MBUS_CA		0x0c78	/* Configuration Address */
#define GTPCI_MBUS_CD		0x0c7c	/* Configuration Data */

static pcireg_t
gtpci_mbus_conf_read(void *v, pcitag_t tag, int reg)
{
	struct gtpci_softc *sc = v;
	const pcireg_t addr = tag | reg;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GTPCI_MBUS_CA,
	    addr | GTPCI_CA_CONFIGEN);
	if ((addr | GTPCI_CA_CONFIGEN) !=
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, GTPCI_MBUS_CA))
		return -1;

	return bus_space_read_4(sc->sc_iot, sc->sc_ioh, GTPCI_MBUS_CD);
}

static void
gtpci_mbus_conf_write(void *v, pcitag_t tag, int reg, pcireg_t data)
{
	struct gtpci_softc *sc = v;
	pcireg_t addr = tag | (reg & 0xfc);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GTPCI_MBUS_CA,
	    addr | GTPCI_CA_CONFIGEN);
	if ((addr | GTPCI_CA_CONFIGEN) !=
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, GTPCI_MBUS_CA))
		return;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GTPCI_MBUS_CD, data);
}
#endif	/* NGTPCI_MBUS */

/*
 * We assume to use GPP interrupt as PCI interrupts.
 *   pci_intr_map() shall returns number of GPP between 0 and 31.  However
 *   returns 0xff, because we do not know the connected pin number for GPP
 *   of your board.
 *   pci_intr_string() shall returns string "gpp <num>".
 *   pci_intr_establish() established interrupt in the pin of all GPP.
 *   Moreover, the return value will be disregarded.  For instance, the
 *   setting for interrupt is not done.
 */

/* ARGSUSED */
static int
gtpci_gpp_intr_map(struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{

	*ihp = pa->pa_intrpin;
	return 0;
}

/* ARGSUSED */
static const char *
gtpci_gpp_intr_string(void *v, pci_intr_handle_t pin)
{
	struct gtpci_softc *sc = v;
	prop_array_t int2gpp;
	prop_object_t gpp;
	static char intrstr[8];

	int2gpp = prop_dictionary_get(device_properties(sc->sc_dev), "int2gpp");
	gpp = prop_array_get(int2gpp, pin);
	sprintf(intrstr, "gpp %d", (int)prop_number_integer_value(gpp));

	return intrstr;
}

/* ARGSUSED */
static const struct evcnt *
gtpci_gpp_intr_evcnt(void *v, pci_intr_handle_t pin)
{

	return NULL;
}

static void *
gtpci_gpp_intr_establish(void *v, pci_intr_handle_t int_pin, int ipl,
		         int (*intrhand)(void *), void *intrarg)
{
	struct gtpci_softc *sc = v;
	prop_array_t int2gpp;
	prop_object_t gpp;
	int gpp_pin;

	int2gpp = prop_dictionary_get(device_properties(sc->sc_dev), "int2gpp");
	gpp = prop_array_get(int2gpp, int_pin);
	gpp_pin = prop_number_integer_value(gpp);
	return mvsocgpp_intr_establish(gpp_pin, ipl, 0, intrhand, intrarg);
}

static void
gtpci_gpp_intr_disestablish(void *v, void *ih)
{

	mvsocgpp_intr_disestablish(ih);
}
#endif

#if NMVPEX_MBUS > 0
static pcireg_t
mvpex_mbus_conf_read(void *v, pcitag_t tag, int reg)
{
	struct mvpex_softc *sc = v;
	pcireg_t addr, data, pci_cs;
	uint32_t stat;
	int bus, dev, func, pexbus, pexdev;

	mvpex_decompose_tag(v, tag, &bus, &dev, &func);

	stat = bus_space_read_4(sc->sc_iot, sc->sc_ioh, MVPEX_STAT);
	pexbus = MVPEX_STAT_PEXBUSNUM(stat);
	pexdev = MVPEX_STAT_PEXDEVNUM(stat);
	if (bus != pexbus || dev != pexdev)
		if (stat & MVPEX_STAT_DLDOWN)
			return -1;

	if (bus == pexbus) {
		if (pexdev == 0) {
			if (dev != 1 && dev != pexdev)
				return -1;
		} else {
			if (dev != 0 && dev != pexdev)
				return -1;
		}
		if (func != 0)
			return -1;
	}

	addr = ((reg & 0xf00) << 24)  | tag | (reg & 0xfc);

#if defined(ORION)
	/*
	 * Guideline (GL# PCI Express-1) Erroneous Read Data on Configuration
	 * This guideline is relevant for all devices except of the following
	 * devices:
	 *     88F5281-BO and above, and 88F5181L-A0 and above
	 */
	if ((bus != pexbus || dev != pexdev) &&
	    !(sc->sc_model == MARVELL_ORION_2_88F5281 && sc->sc_rev == 1) &&
	    !(sc->sc_model == MARVELL_ORION_1_88F5181 && sc->sc_rev == 8)) {

		/* PCI-Express configuration read work-around */
		/*
		 * We will use one of the Punit (AHBToMbus) windows to
		 * access the xbar and read the data from there
		 *
		 * Need to configure the 2 free Punit (AHB to MBus bridge)
		 * address decoding windows:
		 * Configure the flash Window to handle Configuration space
		 * requests for PEX0/1:
		 *
		 * Configuration transactions from the CPU should write/read
		 * the data to/from address of the form:
		 *	addr[31:28]: 0x5 (for PEX0) or 0x6 (for PEX1)
		 *	addr[27:24]: extended register number
		 *	addr[23:16]: bus number
		 *	addr[15:11]: device number
		 *	addr[10: 8]: function number
		 *	addr[ 7: 0]: register number
		 */

		struct mvsoc_softc *soc =
		    device_private(device_parent(sc->sc_dev));;
		bus_space_handle_t pcicfg_ioh;
		uint32_t remapl, remaph, wc, pcicfg_addr, pcicfg_size;
		int window, target, attr, base, size, s;
		const int pex_pcicfg_tag =
		    (sc->sc_model == MARVELL_ORION_1_88F1181) ?
		    ORION_TAG_FLASH_CS : ORION_TAG_PEX0_MEM;

		window = mvsoc_target(pex_pcicfg_tag,
		    &target, &attr, &base, &size);
		if (window >= nwindow) {
			aprint_error_dev(sc->sc_dev,
			    "can't read pcicfg space\n");
			return -1;
		}

		s = splhigh();

		remapl = remaph = 0;
		if (window == 0 || window == 1) {
			remapl = read_mlmbreg(MVSOC_MLMB_WRLR(window));
			remaph = read_mlmbreg(MVSOC_MLMB_WRHR(window));
		}

		wc =
		    MVSOC_MLMB_WCR_WINEN			|
		    MVSOC_MLMB_WCR_ATTR(ORION_ATTR_PEX_CFG)	|
		    MVSOC_MLMB_WCR_TARGET((soc->sc_addr + sc->sc_offset) >> 16);
		if (sc->sc_model == MARVELL_ORION_1_88F1181) {
			pcicfg_addr = base;
			pcicfg_size = size;
		} else if (sc->sc_model == MARVELL_ORION_1_88F5182) {
#define PEX_PCICFG_RW_WA_BASE		0x50000000
#define PEX_PCICFG_RW_WA_5182_BASE	0xf0000000
#define PEX_PCICFG_RW_WA_SIZE		(16 * 1024 * 1024)
			pcicfg_addr = PEX_PCICFG_RW_WA_5182_BASE;
			pcicfg_size = PEX_PCICFG_RW_WA_SIZE;
		} else {
			pcicfg_addr = PEX_PCICFG_RW_WA_BASE;
			pcicfg_size = PEX_PCICFG_RW_WA_SIZE;
		}
		write_mlmbreg(MVSOC_MLMB_WCR(window),
		    wc | MVSOC_MLMB_WCR_SIZE(pcicfg_size));
		write_mlmbreg(MVSOC_MLMB_WBR(window), pcicfg_addr);

		if (window == 0 || window == 1) {
			write_mlmbreg(MVSOC_MLMB_WRLR(window), pcicfg_addr);
			write_mlmbreg(MVSOC_MLMB_WRHR(window), 0);
		}

		if (bus_space_map(sc->sc_iot, pcicfg_addr, pcicfg_size, 0,
		    &pcicfg_ioh) == 0) {
			data = bus_space_read_4(sc->sc_iot, pcicfg_ioh, addr);
			bus_space_unmap(sc->sc_iot, pcicfg_ioh, pcicfg_size);
		} else
			data = -1;

		write_mlmbreg(MVSOC_MLMB_WCR(window),
		    MVSOC_MLMB_WCR_WINEN		|
		    MVSOC_MLMB_WCR_ATTR(attr)		|
		    MVSOC_MLMB_WCR_TARGET(target)	|
		    MVSOC_MLMB_WCR_SIZE(size));
		write_mlmbreg(MVSOC_MLMB_WBR(window), base);
		if (window == 0 || window == 1) {
			write_mlmbreg(MVSOC_MLMB_WRLR(window), remapl);
			write_mlmbreg(MVSOC_MLMB_WRHR(window), remaph);
		}

		splx(s);
#else
	if (0) {
#endif
	} else {
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVPEX_CA,
		    addr | MVPEX_CA_CONFIGEN);
		if ((addr | MVPEX_CA_CONFIGEN) !=
		    bus_space_read_4(sc->sc_iot, sc->sc_ioh, MVPEX_CA))
			return -1;

		pci_cs = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
		    PCI_COMMAND_STATUS_REG);
		bus_space_write_4(sc->sc_iot, sc->sc_ioh,
		    PCI_COMMAND_STATUS_REG, pci_cs | PCI_STATUS_MASTER_ABORT);

		data = bus_space_read_4(sc->sc_iot, sc->sc_ioh, MVPEX_CD);
	}

	return data;
}
#endif
