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

__KERNEL_RCSID(1, "$NetBSD: armperiph.c,v 1.2.6.2 2012/11/28 22:40:26 matt Exp $");

#include <sys/param.h>
#include <sys/device.h>

#include "ioconf.h"

#include <arm/mainbus/mainbus.h>
#include <arm/cortex/mpcore_var.h>

static int armperiph_match(device_t, cfdata_t, void *);
static void armperiph_attach(device_t, device_t, void *);

static bool attached;

struct armperiph_softc {
	device_t sc_dev;
	bus_space_tag_t sc_memt;
	bus_space_handle_t sc_memh;
};

#ifdef CPU_CORTEXA5
static const char * const a5_devices[] = {
	"armscu", "armgic", NULL
};
#endif

#ifdef CPU_CORTEXA7
static const char * const a7_devices[] = {
	"armgic", NULL
};
#endif

#ifdef CPU_CORTEXA9
static const char * const a9_devices[] = {
	"armscu", "arml2cc", "armgic", "a9tmr", "a9wdt", NULL
};
#endif

static const struct mpcore_config {
	const char * const *cfg_devices;
	uint32_t cfg_cpuid;
	uint32_t cfg_cbar_size;
} configs[] = {
#ifdef CPU_CORTEXA5
	{ a5_devices, 0x410fc050, 8192 },
#endif
#ifdef CPU_CORTEXA7
	{ a7_devices, 0x410fc070, 32768 },
#endif
#ifdef CPU_CORTEXA9
	{ a9_devices, 0x410fc090, 3*4096 },
#endif
#ifdef CPU_CORTEXA15
	{ a15_devices, 0x410fc0f0, 32768 },
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
	for (size_t i = 0; cfg->cfg_devices[i] != NULL; i++) {
		struct mpcore_attach_args mpcaa = {
			.mpcaa_name = cfg->cfg_devices[i],
			.mpcaa_memt = sc->sc_memt,
			.mpcaa_memh = sc->sc_memh,
		};

		config_found(self, &mpcaa, NULL);
	}
}
