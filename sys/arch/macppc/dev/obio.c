/*	$NetBSD: obio.c,v 1.30 2010/10/20 18:52:33 phx Exp $	*/

/*-
 * Copyright (C) 1998	Internet Research Institute, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by
 *	Internet Research Institute, Inc.
 * 4. The name of the author may not be used to endorse or promote products
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: obio.c,v 1.30 2010/10/20 18:52:33 phx Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/sysctl.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/ofw/openfirm.h>

#include <machine/autoconf.h>

#include <macppc/dev/obiovar.h>

#include <powerpc/cpu.h>

#include "opt_obio.h"

#ifdef OBIO_DEBUG
# define DPRINTF printf
#else
# define DPRINTF while (0) printf
#endif

static void obio_attach(struct device *, struct device *, void *);
static int obio_match(struct device *, struct cfdata *, void *);
static int obio_print(void *, const char *);

struct obio_softc {
	struct device sc_dev;
	bus_space_tag_t sc_tag;
	bus_space_handle_t sc_bh;
	int sc_node;
#ifdef OBIO_SPEED_CONTROL
	int sc_voltage;
	int sc_busspeed;
#endif
};

static struct obio_softc *obio0 = NULL;

#ifdef OBIO_SPEED_CONTROL
static void obio_setup_gpios(struct obio_softc *, int);
static void obio_set_cpu_speed(struct obio_softc *, int);
static int  obio_get_cpu_speed(struct obio_softc *);
static int  sysctl_cpuspeed_temp(SYSCTLFN_ARGS);

static const char *keylargo[] = {"Keylargo",
				 "AAPL,Keylargo",
				 NULL};

#endif

CFATTACH_DECL(obio, sizeof(struct obio_softc),
    obio_match, obio_attach, NULL, NULL);

int
obio_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_APPLE)
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_APPLE_GC:
		case PCI_PRODUCT_APPLE_OHARE:
		case PCI_PRODUCT_APPLE_HEATHROW:
		case PCI_PRODUCT_APPLE_PADDINGTON:
		case PCI_PRODUCT_APPLE_KEYLARGO:
		case PCI_PRODUCT_APPLE_PANGEA_MACIO:
		case PCI_PRODUCT_APPLE_INTREPID:
		case PCI_PRODUCT_APPLE_K2:
			return 1;
		}

	return 0;
}

/*
 * Attach all the sub-devices we can find
 */
void
obio_attach(struct device *parent, struct device *self, void *aux)
{
	struct obio_softc *sc = (struct obio_softc *)self;
	struct pci_attach_args *pa = aux;
	struct confargs ca;
	bus_space_handle_t bsh;
	int node, child, namelen, error;
	u_int reg[20];
	int intr[6], parent_intr = 0, parent_nintr = 0;
	char name[32];
	char compat[32];

#ifdef OBIO_SPEED_CONTROL
	sc->sc_voltage = -1;
	sc->sc_busspeed = -1;
#endif

	switch (PCI_PRODUCT(pa->pa_id)) {

	case PCI_PRODUCT_APPLE_GC:
	case PCI_PRODUCT_APPLE_OHARE:
	case PCI_PRODUCT_APPLE_HEATHROW:
	case PCI_PRODUCT_APPLE_PADDINGTON:
	case PCI_PRODUCT_APPLE_KEYLARGO:
	case PCI_PRODUCT_APPLE_PANGEA_MACIO:
	case PCI_PRODUCT_APPLE_INTREPID:
		node = pcidev_to_ofdev(pa->pa_pc, pa->pa_tag);
		if (node == -1)
			node = OF_finddevice("mac-io");
			if (node == -1)
				node = OF_finddevice("/pci/mac-io");
		break;
	case PCI_PRODUCT_APPLE_K2:
		node = OF_finddevice("mac-io");
		break;

	default:
		node = -1;
		break;
	}
	if (node == -1)
		panic("macio not found or unknown");

	sc->sc_node = node;

#if defined (PMAC_G5)
	if (OF_getprop(node, "assigned-addresses", reg, sizeof(reg)) < 20)
	{
		return;
	}
#else
	if (OF_getprop(node, "assigned-addresses", reg, sizeof(reg)) < 12)
		return;
#endif /* PMAC_G5 */

	/*
	 * XXX
	 * This relies on the primary obio always attaching first which is
	 * true on the PowerBook 3400c and similar machines but may or may
	 * not work on others. We can't rely on the node name since Apple
	 * didn't follow anything remotely resembling a consistent naming
	 * scheme.
	 */
	if (obio0 == NULL)
		obio0 = sc;

	ca.ca_baseaddr = reg[2];
	ca.ca_tag = pa->pa_memt;
	sc->sc_tag = pa->pa_memt;
	error = bus_space_map (pa->pa_memt, ca.ca_baseaddr, 0x80, 0, &bsh);
	if (error)
		panic(": failed to map mac-io %#x", ca.ca_baseaddr);
	sc->sc_bh = bsh;

	printf(": addr 0x%x\n", ca.ca_baseaddr);

	/* Enable internal modem (KeyLargo) */
	if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_KEYLARGO) {
		aprint_normal("%s: enabling KeyLargo internal modem\n",
		    self->dv_xname);
		bus_space_write_4(ca.ca_tag, bsh, 0x40, 
		    bus_space_read_4(ca.ca_tag, bsh, 0x40) & ~(1<<25));
	}

	/* Enable internal modem (Pangea) */
	if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_PANGEA_MACIO) {
		/* set reset */
		bus_space_write_1(ca.ca_tag, bsh, 0x006a + 0x03, 0x04);
		/* power modem on */
		bus_space_write_1(ca.ca_tag, bsh, 0x006a + 0x02, 0x04);
		/* unset reset */
		bus_space_write_1(ca.ca_tag, bsh, 0x006a + 0x03, 0x05);
	}

	/* Gatwick and Paddington use same product ID */
	namelen = OF_getprop(node, "compatible", compat, sizeof(compat));

	if (strcmp(compat, "gatwick") == 0) {
		parent_nintr = OF_getprop(node, "AAPL,interrupts", intr,
					sizeof(intr));
		parent_intr = intr[0];
	} else {
  		/* Enable CD and microphone sound input. */
		if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_PADDINGTON)
			bus_space_write_1(ca.ca_tag, bsh, 0x37, 0x03);
	}

	for (child = OF_child(node); child; child = OF_peer(child)) {
		namelen = OF_getprop(child, "name", name, sizeof(name));
		if (namelen < 0)
			continue;
		if (namelen >= sizeof(name))
			continue;

#ifdef OBIO_SPEED_CONTROL
		if (strcmp(name, "gpio") == 0) {

			obio_setup_gpios(sc, child);
			continue;
		}
#endif

		name[namelen] = 0;
		ca.ca_name = name;
		ca.ca_node = child;
		ca.ca_tag = pa->pa_memt;

		ca.ca_nreg = OF_getprop(child, "reg", reg, sizeof(reg));

		if (strcmp(compat, "gatwick") != 0) {
			ca.ca_nintr = OF_getprop(child, "AAPL,interrupts", intr,
					sizeof(intr));
			if (ca.ca_nintr == -1)
				ca.ca_nintr = OF_getprop(child, "interrupts", intr,
						sizeof(intr));
		} else {
			intr[0] = parent_intr;
			ca.ca_nintr = parent_nintr;
		}
		ca.ca_reg = reg;
		ca.ca_intr = intr;

		config_found(self, &ca, obio_print);
	}
}

static const char * const skiplist[] = {
	"interrupt-controller",
	"gpio",
	"escc-legacy",
	"timer",
	"i2c",
	"power-mgt",
	"escc",
	"battery",
	"backlight"
	
};

#define N_LIST (sizeof(skiplist) / sizeof(skiplist[0]))

int
obio_print(void *aux, const char *obio)
{
	struct confargs *ca = aux;
	int i;

	for (i = 0; i < N_LIST; i++)
		if (strcmp(ca->ca_name, skiplist[i]) == 0)
			return QUIET;

	if (obio)
		aprint_normal("%s at %s", ca->ca_name, obio);

	if (ca->ca_nreg > 0)
		aprint_normal(" offset 0x%x", ca->ca_reg[0]);

	return UNCONF;
}

void obio_write_4(int offset, uint32_t value)
{
	if (obio0 == NULL)
		return;
	bus_space_write_4(obio0->sc_tag, obio0->sc_bh, offset, value);
}

void obio_write_1(int offset, uint8_t value)
{
	if (obio0 == NULL)
		return;
	bus_space_write_1(obio0->sc_tag, obio0->sc_bh, offset, value);
}

uint32_t obio_read_4(int offset)
{
	if (obio0 == NULL)
		return 0xffffffff;
	return bus_space_read_4(obio0->sc_tag, obio0->sc_bh, offset);
}

uint8_t obio_read_1(int offset)
{
	if (obio0 == NULL)
		return 0xff;
	return bus_space_read_1(obio0->sc_tag, obio0->sc_bh, offset);
}

#ifdef OBIO_SPEED_CONTROL

static void
obio_setup_gpios(struct obio_softc *sc, int node)
{
	uint32_t reg[6];
	struct sysctlnode *sysctl_node;
	char name[32];
	int gpio_base, child, use_dfs;

	if (of_compatible(sc->sc_node, keylargo) == -1)
		return;

	if (OF_getprop(node, "reg", reg, sizeof(reg)) < 4)
		return;

	gpio_base = reg[0];
	DPRINTF("gpio_base: %02x\n", gpio_base);

	/* now look for voltage and bus speed gpios */
	use_dfs = 0;
	for (child = OF_child(node); child; child = OF_peer(child)) {

		if (OF_getprop(child, "name", name, sizeof(name)) < 1)
			continue;

		if (OF_getprop(child, "reg", reg, sizeof(reg)) < 4)
			continue;

		if (strcmp(name, "frequency-gpio") == 0) {
			DPRINTF("found frequency_gpio at %02x\n", reg[0]);
			sc->sc_busspeed = gpio_base + reg[0];
		}
		if (strcmp(name, "voltage-gpio") == 0) {
			DPRINTF("found voltage_gpio at %02x\n", reg[0]);
			sc->sc_voltage = gpio_base + reg[0];
		}
		if (strcmp(name, "cpu-vcore-select") == 0) {
			DPRINTF("found cpu-vcore-select at %02x\n", reg[0]);
			sc->sc_voltage = gpio_base + reg[0];
			/* frequency gpio is not needed, we use cpu's DFS */
			use_dfs = 1;
		}
	}

	if ((sc->sc_voltage < 0) || (sc->sc_busspeed < 0 && !use_dfs))
		return;

	printf("%s: enabling Intrepid CPU speed control\n",
	    sc->sc_dev.dv_xname);

	sysctl_node = NULL;
	sysctl_createv(NULL, 0, NULL, 
	    (const struct sysctlnode **)&sysctl_node, 
	    CTLFLAG_READWRITE | CTLFLAG_OWNDESC | CTLFLAG_IMMEDIATE,
	    CTLTYPE_INT, "cpu_speed", "CPU speed", sysctl_cpuspeed_temp, 
	    (unsigned long)sc, NULL, 0, CTL_MACHDEP, CTL_CREATE, CTL_EOL);
	if (sysctl_node != NULL) {
		sysctl_node->sysctl_data = (void *)sc;
	}
}

static void
obio_set_cpu_speed(struct obio_softc *sc, int fast)
{

	if (sc->sc_voltage < 0)
		return;

	if (sc->sc_busspeed >= 0) {
		/* set voltage and speed via gpio */
		if (fast) {
			bus_space_write_1(sc->sc_tag, sc->sc_bh,
			    sc->sc_voltage, 5);
			bus_space_write_1(sc->sc_tag, sc->sc_bh,
			    sc->sc_busspeed, 5);
		} else {
			bus_space_write_1(sc->sc_tag, sc->sc_bh,
			    sc->sc_busspeed, 4);
			bus_space_write_1(sc->sc_tag, sc->sc_bh,
			    sc->sc_voltage, 4);
		}
	}
	else {
		/* set voltage via gpio and speed via the 7447A's DFS bit */
		if (fast) {
			bus_space_write_1(sc->sc_tag, sc->sc_bh,
			    sc->sc_voltage, 5);
			DELAY(1000);
		}

		/* set DFS for all cpus */
		cpu_set_dfs(fast ? 1 : 2);
		DELAY(100);

		if (!fast) {
			bus_space_write_1(sc->sc_tag, sc->sc_bh,
			    sc->sc_voltage, 4);
			DELAY(1000);
		}
	}
}

static int
obio_get_cpu_speed(struct obio_softc *sc)
{
	
	if (sc->sc_voltage < 0)
		return 0;

	if (sc->sc_busspeed >= 0) {
		if (bus_space_read_1(sc->sc_tag, sc->sc_bh, sc->sc_busspeed)
		    & 1)
		return 1;
	}
	else
		return cpu_get_dfs() == 1;

	return 0;
}

static int
sysctl_cpuspeed_temp(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct obio_softc *sc = node.sysctl_data;
	const int *np = newp;
	int speed, nd = 0;

	speed = obio_get_cpu_speed(sc);	
	node.sysctl_idata = speed;
	if (np) {
		/* we're asked to write */	
		nd = *np;
		node.sysctl_data = &speed;
		if (sysctl_lookup(SYSCTLFN_CALL(&node)) == 0) {
			int new_reg;

			new_reg = (max(0, min(1, node.sysctl_idata)));
			obio_set_cpu_speed(sc, new_reg);
			return 0;
		}
		return EINVAL;
	} else {
		node.sysctl_size = 4;
		return(sysctl_lookup(SYSCTLFN_CALL(&node)));
	}
}

#endif /* OBIO_SPEEDCONTROL */
