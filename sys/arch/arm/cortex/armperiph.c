/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "locators.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: armperiph.c,v 1.2.2.2 2017/12/03 11:35:52 jdolecek Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/lwp.h>

#include "ioconf.h"

#include <arm/mainbus/mainbus.h>
#include <arm/cortex/mpcore_var.h>
#include <arm/cortex/gtmr_intr.h>

static int armperiph_match(device_t, cfdata_t, void *);
static void armperiph_attach(device_t, device_t, void *);

static bool attached;

struct armperiph_softc {
	device_t sc_dev;
	bus_space_tag_t sc_memt;
	bus_space_handle_t sc_memh;
};

struct armperiph_info {
	const char pi_name[12];
	bus_size_t pi_off1;
	bus_size_t pi_off2;
};

#ifdef CPU_CORTEXA5
static const struct armperiph_info a5_devices[] = {
	{ "armscu", 0x0000, 0 },
	{ "armgic", 0x1000, 0x0100 },
	{ "a9tmr",  0x0200, 0 },
	{ "a9wdt",   0x0600, 0 },
	{ "arml2cc", 0, 0 },	/* external; needs "offset" property */
	{ "", 0, 0 },
};
#endif

#ifdef CPU_CORTEXA7
static const struct armperiph_info a7_devices[] = {
	{ "armgic",  0x1000, 0x2000 },
	{ "armgtmr", 0, 0 },
	{ "", 0, 0 },
};
#endif

#ifdef CPU_CORTEXA9
static const struct armperiph_info a9_devices[] = {
	{ "armscu",  0x0000, 0 },
	{ "arml2cc", 0x2000, 0 },
	{ "armgic",  0x1000, 0x0100 },
	{ "a9tmr",   0x0200, 0 },
	{ "a9wdt",   0x0600, 0 },
	{ "", 0, 0 },
};
#endif

#ifdef CPU_CORTEXA15
static const struct armperiph_info a15_devices[] = {
	{ "armgic",  0x1000, 0x2000 },
	{ "armgtmr", 0, 0 },
	{ "", 0, 0 },
};
#endif

#ifdef CPU_CORTEXA17
static const struct armperiph_info a17_devices[] = {
	{ "armgic",  0x1000, 0x2000 },
	{ "armgtmr", 0, 0 },
	{ "", 0, 0 },
};
#endif

#ifdef CPU_CORTEXA57
static const struct armperiph_info a57_devices[] = {
	{ "armgic",  0x1000, 0x2000 },
	{ "armgtmr", 0, 0 },
	{ "", 0, 0 },
};
#endif


static const struct mpcore_config {
	const struct armperiph_info *cfg_devices;
	uint32_t cfg_cpuid;
	uint32_t cfg_cbar_size;
} configs[] = {
#ifdef CPU_CORTEXA5
	{ a5_devices, 0x410fc050, 2*4096 },
#endif
#ifdef CPU_CORTEXA7
	{ a7_devices, 0x410fc070, 8*4096 },
#endif
#ifdef CPU_CORTEXA9
	{ a9_devices, 0x410fc090, 3*4096 },
#endif
#ifdef CPU_CORTEXA15
	{ a15_devices, 0x410fc0f0, 8*4096 },
#endif
#ifdef CPU_CORTEXA17
	{ a17_devices, 0x410fc0e0, 8*4096 },
#endif
#ifdef CPU_CORTEXA57
	{ a57_devices, 0x410fd070, 8*4096 },
#endif
};

static const struct mpcore_config *
armperiph_find_config(void)
{
	const uint32_t arm_cpuid = curcpu()->ci_arm_cpuid & 0xff0ff0f0;
	for (size_t i = 0; i < __arraycount(configs); i++) {
		if (arm_cpuid == configs[i].cfg_cpuid) {
			return configs + i;
		}
	}

	return NULL;
}

CFATTACH_DECL_NEW(armperiph, sizeof(struct armperiph_softc),
    armperiph_match, armperiph_attach, NULL, NULL);

static int
armperiph_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args * const mb = aux;
	const int base = cf->cf_loc[MAINBUSCF_BASE];
	const int size = cf->cf_loc[MAINBUSCF_SIZE];
	const int dack = cf->cf_loc[MAINBUSCF_DACK];
	const int irq = cf->cf_loc[MAINBUSCF_IRQ];
	const int intrbase = cf->cf_loc[MAINBUSCF_INTRBASE];

	if (attached)
		return 0;

	if (base != MAINBUSCF_BASE_DEFAULT || base != mb->mb_iobase
	    || size != MAINBUSCF_SIZE_DEFAULT || size != mb->mb_iosize
	    || dack != MAINBUSCF_DACK_DEFAULT || dack != mb->mb_drq
	    || irq != MAINBUSCF_IRQ_DEFAULT || irq != mb->mb_irq
	    || intrbase != MAINBUSCF_INTRBASE_DEFAULT
	    || intrbase != mb->mb_intrbase)
		return 0;

	if (!CPU_ID_CORTEX_P(curcpu()->ci_arm_cpuid))
		return 0;

	if (armreg_cbar_read() == 0)
		return 0;

	if (armperiph_find_config() == NULL)
		return 0;

	return 1;
}

static void
armperiph_attach(device_t parent, device_t self, void *aux)
{
	struct armperiph_softc * const sc = device_private(self);
	struct mainbus_attach_args * const mb = aux;
	bus_addr_t cbar = armreg_cbar_read();
	const struct mpcore_config * const cfg = armperiph_find_config();
	prop_dictionary_t prop = device_properties(self);
	uint32_t cbar_override;

	if (prop_dictionary_get_uint32(prop, "cbar", &cbar_override))
		cbar = (bus_addr_t)cbar_override;

	/*
	 * The normal mainbus bus space will not work for us so the port's
	 * device_register must have replaced it with one that will work.
	 */
	sc->sc_dev = self;
	sc->sc_memt = mb->mb_iot;

	int error = bus_space_map(sc->sc_memt, cbar, cfg->cfg_cbar_size, 0,
	    &sc->sc_memh);
	if (error) {
		aprint_normal(": error mapping registers at %#lx: %d\n",
		    cbar, error);
		return;
	}
	aprint_normal("\n");

	/*
	 * Let's try to attach any children we may have.
	 */
	for (size_t i = 0; cfg->cfg_devices[i].pi_name[0] != 0; i++) {
		struct mpcore_attach_args mpcaa = {
			.mpcaa_name = cfg->cfg_devices[i].pi_name,
			.mpcaa_memt = sc->sc_memt,
			.mpcaa_memh = sc->sc_memh,
			.mpcaa_off1 = cfg->cfg_devices[i].pi_off1,
			.mpcaa_off2 = cfg->cfg_devices[i].pi_off2,
		};
#if defined(CPU_CORTEXA7) || defined(CPU_CORTEXA15) || defined(CPU_CORTEXA57)
		if (strcmp(mpcaa.mpcaa_name, "armgtmr") == 0) {
			mpcaa.mpcaa_irq = IRQ_GTMR_PPI_VTIMER;
		}
#endif

		config_found(self, &mpcaa, NULL);
	}
}
