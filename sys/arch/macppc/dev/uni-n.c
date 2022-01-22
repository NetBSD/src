/*	$NetBSD: uni-n.c,v 1.12 2022/01/22 11:49:16 thorpej Exp $	*/

/*-
 * Copyright (C) 2005 Michael Lorenz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
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

/*
 * a driver to match the uni_n node in OF's device tree and attach its children 
 */
 
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uni-n.c,v 1.12 2022/01/22 11:49:16 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <dev/ofw/openfirm.h>
#include <dev/ofw/ofw_pci.h>

#include <machine/autoconf.h>

#include "fcu.h"

static void uni_n_attach(device_t, device_t, void *);
static int uni_n_match(device_t, cfdata_t, void *);
static int uni_n_print(void *, const char *);

struct uni_n_softc {
	device_t sc_dev;
	struct powerpc_bus_space sc_memt;
	int sc_node;
};

CFATTACH_DECL_NEW(uni_n, sizeof(struct uni_n_softc),
    uni_n_match, uni_n_attach, NULL, NULL);

#if NFCU > 0
/* storage for CPUID SEEPROM contents found on some G5 */
static uint8_t eeprom[2][160];
#endif

int
uni_n_match(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;
	char compat[32];
	if ((strcmp(ca->ca_name, "uni-n") != 0) &&
	    (strcmp(ca->ca_name, "u4") != 0) &&
	    (strcmp(ca->ca_name, "u3") != 0))
		return 0;

	memset(compat, 0, sizeof(compat));
#if 0
	OF_getprop(ca->ca_node, "compatible", compat, sizeof(compat));
	if (strcmp(compat, "uni-north") != 0)
		return 0;
#endif
	return 1;
}

/*
 * Attach all the sub-devices we can find
 */
void
uni_n_attach(device_t parent, device_t self, void *aux)
{
	struct uni_n_softc *sc = device_private(self);
	struct confargs *our_ca = aux;
	struct confargs ca;
	int node, child, namelen;
#if NFCU > 0
	int cpuid;
#endif
	u_int reg[20];
	int intr[6];
	char name[32];

	sc->sc_dev = self;
	node = our_ca->ca_node;
	sc->sc_node = node;
	printf(" address 0x%08x\n",
	    our_ca->ca_reg[our_ca->ca_nreg > 8 ? 1 : 0]);

#if NFCU > 0
	/*
	 * zero out eeprom blocks, then see if we have valid data
	 * doing this here because the EEPROMs are dangling from out i2c bus
	 * but we can get all the data just from looking at the properties
	 */
	memset(eeprom, 0, sizeof(eeprom));
	cpuid = OF_finddevice("/u3/i2c/cpuid@a0");
	OF_getprop(cpuid, "cpuid", eeprom[0], sizeof(eeprom[0]));
	if (eeprom[0][1] != 0)
		aprint_normal_dev(self, "found EEPROM data for CPU 0\n");
	cpuid = OF_finddevice("/u3/i2c/cpuid@a2");
	OF_getprop(cpuid, "cpuid", eeprom[1], sizeof(eeprom[1]));
	if (eeprom[1][1] != 0)
		aprint_normal_dev(self, "found EEPROM data for CPU 1\n");
#endif

	memset(&sc->sc_memt, 0, sizeof(struct powerpc_bus_space));
	sc->sc_memt.pbs_flags = _BUS_SPACE_LITTLE_ENDIAN|_BUS_SPACE_MEM_TYPE;
	if (ofwoea_map_space(RANGE_TYPE_MACIO, RANGE_MEM, node, &sc->sc_memt,
	    "uni-n mem-space") != 0) {
		panic("Can't init uni-n mem tag");
	}

	devhandle_t selfh = device_handle(self);
	for (child = OF_child(node); child; child = OF_peer(child)) {
		namelen = OF_getprop(child, "name", name, sizeof(name));
		if (namelen < 0)
			continue;
		if (namelen >= sizeof(name))
			continue;

		name[namelen] = 0;
		ca.ca_name = name;
		ca.ca_node = child;
		ca.ca_tag = &sc->sc_memt;
		ca.ca_nreg  = OF_getprop(child, "reg", reg, sizeof(reg));
		ca.ca_nintr = OF_getprop(child, "AAPL,interrupts", intr,
				sizeof(intr));
		if (ca.ca_nintr == -1)
			ca.ca_nintr = OF_getprop(child, "interrupts", intr,
					sizeof(intr));

		ca.ca_reg = reg;
		ca.ca_intr = intr;
		config_found(self, &ca, uni_n_print,
		    CFARGS(.devhandle = devhandle_from_of(selfh, child)));
	}
}

int
uni_n_print(void *aux, const char *uni_n)
{
	struct confargs *ca = aux;
	if (uni_n)
		aprint_normal("%s at %s", ca->ca_name, uni_n);

	if (ca->ca_nreg > 0)
		aprint_normal(" address 0x%x", ca->ca_reg[0]);

	return UNCONF;
}

#if NFCU > 0
int
get_cpuid(int cpu, uint8_t *buf)
{
	if ((cpu < 0) || (cpu > 1)) return -1;
	if (eeprom[cpu][1] == 0) return 0;
	memcpy(buf, eeprom[cpu], sizeof(eeprom[cpu]));
	return sizeof(eeprom[cpu]);
}
#endif
