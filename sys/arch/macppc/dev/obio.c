/*	$NetBSD: obio.c,v 1.53 2022/12/28 07:34:42 macallan Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: obio.c,v 1.53 2022/12/28 07:34:42 macallan Exp $");

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
#include <sys/cpufreq.h>

#include "opt_obio.h"

#ifdef OBIO_DEBUG
# define DPRINTF printf
#else
# define DPRINTF while (0) printf
#endif

static void obio_attach(device_t, device_t, void *);
static int obio_match(device_t, cfdata_t, void *);
static int obio_print(void *, const char *);

struct obio_softc {
	device_t sc_dev;
	bus_space_tag_t sc_tag;
	bus_space_handle_t sc_bh;
	int sc_node;
#ifdef OBIO_SPEED_CONTROL
	int sc_voltage;
	int sc_busspeed;
	int sc_spd_hi, sc_spd_lo;
	struct cpufreq sc_cf;
#endif
};

static struct obio_softc *obio0 = NULL;

#ifdef OBIO_SPEED_CONTROL
static void obio_setup_gpios(struct obio_softc *, int);
static void obio_set_cpu_speed(struct obio_softc *, int);
static int  obio_get_cpu_speed(struct obio_softc *);
static int  sysctl_cpuspeed_temp(SYSCTLFN_ARGS);
static int  sysctl_cpuspeed_cur(SYSCTLFN_ARGS);
static int  sysctl_cpuspeed_available(SYSCTLFN_ARGS);
static void obio_get_freq(void *, void *);
static void obio_set_freq(void *, void *);
static const char *keylargo[] = {"Keylargo",
				 "AAPL,Keylargo",
				 NULL};

#endif

CFATTACH_DECL_NEW(obio, sizeof(struct obio_softc),
    obio_match, obio_attach, NULL, NULL);

int
obio_match(device_t parent, cfdata_t cf, void *aux)
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
		case PCI_PRODUCT_APPLE_SHASTA:
			return 1;
		}

	return 0;
}

/*
 * Attach all the sub-devices we can find
 */
void
obio_attach(device_t parent, device_t self, void *aux)
{
	struct obio_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	struct confargs ca;
	bus_space_handle_t bsh;
	int node, child, namelen, error;
	u_int reg[20];
	int intr[6], parent_intr = 0, parent_nintr = 0;
	int map_size = 0x1000;
	char name[32];
	char compat[32];

	sc->sc_dev = self;
#ifdef OBIO_SPEED_CONTROL
	sc->sc_voltage = -1;
	sc->sc_busspeed = -1;
	sc->sc_spd_lo = 600;
	sc->sc_spd_hi = 800;
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
	case PCI_PRODUCT_APPLE_SHASTA:
		node = OF_finddevice("mac-io");
		map_size = 0x10000;
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
	error = bus_space_map (pa->pa_memt, ca.ca_baseaddr, map_size, 0, &bsh);
	if (error)
		panic(": failed to map mac-io %#x", ca.ca_baseaddr);
	sc->sc_bh = bsh;

	printf(": addr 0x%x\n", ca.ca_baseaddr);

	/* Enable internal modem (KeyLargo) */
	if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_KEYLARGO) {
		aprint_normal("%s: enabling KeyLargo internal modem\n",
		    device_xname(self));
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

	devhandle_t selfh = device_handle(self);
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

		config_found(self, &ca, obio_print,
		    CFARGS(.devhandle = devhandle_from_of(selfh, child)));
	}
}

static const char * const skiplist[] = {
	"interrupt-controller",
	"chrp,open-pic",
	"open-pic",
	"mpic",
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

int
obio_space_map(bus_addr_t addr, bus_size_t size, bus_space_handle_t *bh)
{
	if (obio0 == NULL)
		return 0xff;
	return bus_space_subregion(obio0->sc_tag, obio0->sc_bh,
	    addr & 0xfffff, size, bh);
}
	
#ifdef OBIO_SPEED_CONTROL

static void
obio_setup_cpufreq(device_t dev)
{
	struct obio_softc *sc = device_private(dev);
	int ret;

	ret = cpufreq_register(&sc->sc_cf);
	if (ret != 0)
		aprint_error_dev(sc->sc_dev, "cpufreq_register() failed, error %d\n", ret);
}

static void
obio_setup_gpios(struct obio_softc *sc, int node)
{
	uint32_t gpio_base, reg[6];
	const struct sysctlnode *sysctl_node, *me, *freq;
	struct cpufreq *cf = &sc->sc_cf;
	char name[32];
	int child, use_dfs, cpunode, hiclock;

	if (! of_compatible(sc->sc_node, keylargo))
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

		/*
		 * These register offsets either have to be added to the obio
		 * base address or to the gpio base address. This differs
		 * even in the same OF-tree! So we guess the offset is
		 * based on obio when it is larger than the gpio_base.
		 */
		if (reg[0] >= gpio_base)
			reg[0] -= gpio_base;

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
	    device_xname(sc->sc_dev));

	sc->sc_spd_lo = curcpu()->ci_khz / 1000;
	hiclock = 0;
	cpunode = OF_finddevice("/cpus/@0");
	OF_getprop(cpunode, "clock-frequency", &hiclock, 4);
	if (hiclock != 0)
		sc->sc_spd_hi = (hiclock + 500000) / 1000000;
	printf("hiclock: %d\n", sc->sc_spd_hi);
	if (use_dfs) sc->sc_spd_lo = sc->sc_spd_hi / 2;

	sysctl_node = NULL;

	if (sysctl_createv(NULL, 0, NULL, 
	    &me, 
	    CTLFLAG_READWRITE, CTLTYPE_NODE, "cpu", NULL, NULL,
	    0, NULL, 0, CTL_MACHDEP, CTL_CREATE, CTL_EOL) != 0)
		printf("couldn't create 'cpu' node\n");
	
	if (sysctl_createv(NULL, 0, NULL, 
	    &freq, 
	    CTLFLAG_READWRITE, CTLTYPE_NODE, "frequency", NULL, NULL,
	    0, NULL, 0, CTL_MACHDEP, me->sysctl_num, CTL_CREATE, CTL_EOL) != 0)
		printf("couldn't create 'frequency' node\n");

	if (sysctl_createv(NULL, 0, NULL, 
	    &sysctl_node, 
	    CTLFLAG_READWRITE | CTLFLAG_OWNDESC,
	    CTLTYPE_INT, "target", "CPU speed", sysctl_cpuspeed_temp, 
	    0, (void *)sc, 0, CTL_MACHDEP, me->sysctl_num, freq->sysctl_num, 
	    CTL_CREATE, CTL_EOL) == 0) {
	} else
		printf("couldn't create 'target' node\n");

	if (sysctl_createv(NULL, 0, NULL, 
	    &sysctl_node, 
	    CTLFLAG_READWRITE,
	    CTLTYPE_INT, "current", NULL, sysctl_cpuspeed_cur, 
	    1, (void *)sc, 0, CTL_MACHDEP, me->sysctl_num, freq->sysctl_num, 
	    CTL_CREATE, CTL_EOL) == 0) {
	} else
		printf("couldn't create 'current' node\n");

	if (sysctl_createv(NULL, 0, NULL, 
	    &sysctl_node, 
	    CTLFLAG_READWRITE,
	    CTLTYPE_STRING, "available", NULL, sysctl_cpuspeed_available, 
	    2, (void *)sc, 0, CTL_MACHDEP, me->sysctl_num, freq->sysctl_num, 
	    CTL_CREATE, CTL_EOL) == 0) {
	} else
		printf("couldn't create 'available' node\n");
	printf("speed: %d\n", curcpu()->ci_khz);

	/* support cpufreq */
	snprintf(cf->cf_name, CPUFREQ_NAME_MAX, "Intrepid");
	cf->cf_state[0].cfs_freq = sc->sc_spd_hi;
	cf->cf_state[1].cfs_freq = sc->sc_spd_lo;
	cf->cf_state_count = 2;
	cf->cf_mp = FALSE;
	cf->cf_cookie = sc;
	cf->cf_get_freq = obio_get_freq;
	cf->cf_set_freq = obio_set_freq;
	/* 
	 * XXX
	 * cpufreq_register() calls xc_broadcast() which relies on kthreads
	 * running so we need to postpone it
	 */
	config_interrupts(sc->sc_dev, obio_setup_cpufreq);
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

static void
obio_get_freq(void *cookie, void *spd)
{
	struct obio_softc *sc = cookie;
	uint32_t *freq;

	freq = spd;
	if (obio_get_cpu_speed(sc) == 0) {
		*freq = sc->sc_spd_lo;
	} else
		*freq = sc->sc_spd_hi;
}

static void
obio_set_freq(void *cookie, void *spd)
{
	struct obio_softc *sc = cookie;
	uint32_t *freq;

	freq = spd;
	if (*freq == sc->sc_spd_lo) {
		obio_set_cpu_speed(sc, 0);
	} else if (*freq == sc->sc_spd_hi) {
		obio_set_cpu_speed(sc, 1);
	} else
		aprint_error_dev(sc->sc_dev, "%s(%d) bogus CPU speed\n", __func__, *freq);
}

static int
sysctl_cpuspeed_temp(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct obio_softc *sc = node.sysctl_data;
	int speed, mhz;

	speed = obio_get_cpu_speed(sc);	
	switch (speed) {
		case 0:
			mhz = sc->sc_spd_lo;
			break;
		case 1:
			mhz = sc->sc_spd_hi;
			break;
		default:
			speed = -1;
	}
	node.sysctl_data = &mhz;
	if (sysctl_lookup(SYSCTLFN_CALL(&node)) == 0) {
		int new_reg;

		new_reg = *(int *)node.sysctl_data;
		if (new_reg == sc->sc_spd_lo) {
			obio_set_cpu_speed(sc, 0);
		} else if (new_reg == sc->sc_spd_hi) {
			obio_set_cpu_speed(sc, 1);
		} else {
			printf("%s: new_reg %d\n", __func__, new_reg);
			return EINVAL;
		}
		return 0;
	}
	return EINVAL;
}

static int
sysctl_cpuspeed_cur(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct obio_softc *sc = node.sysctl_data;
	int speed, mhz;

	speed = obio_get_cpu_speed(sc);
	switch (speed) {
		case 0:
			mhz = sc->sc_spd_lo;
			break;
		case 1:
			mhz = sc->sc_spd_hi;
			break;
		default:
			speed = -1;
	}
	node.sysctl_data = &mhz;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
}

static int
sysctl_cpuspeed_available(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct obio_softc *sc = node.sysctl_data;
	char buf[128];

	snprintf(buf, 128, "%d %d", sc->sc_spd_lo, sc->sc_spd_hi);	
	node.sysctl_data = buf;
	return(sysctl_lookup(SYSCTLFN_CALL(&node)));
}

SYSCTL_SETUP(sysctl_ams_setup, "sysctl obio subtree setup")
{

	sysctl_createv(NULL, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "machdep", NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CTL_EOL);
}

#endif /* OBIO_SPEEDCONTROL */
