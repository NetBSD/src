/*	$NetBSD: cpu.c,v 1.10.12.1 2013/02/25 00:28:44 tls Exp $	*/

/*
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 *
 * Author:
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.10.12.1 2013/02/25 00:28:44 tls Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpivar.h>

#define MHz	1000000L
#define GHz	(1000L * MHz)

struct cpu_info cpu_info_primary __aligned(CACHE_LINE_SIZE);
struct cpu_info *cpu_info_list = &cpu_info_primary;

struct cpu_softc {
	device_t sc_dev;		/* device tree glue */
	struct cpu_info *sc_info;	/* pointer to CPU info */
};

char cpu_model[64];

static int cpu_match(device_t, cfdata_t, void *);
static void cpu_attach(device_t, device_t, void *);

static void identifycpu(struct cpu_softc *);

CFATTACH_DECL_NEW(cpu, sizeof(struct cpu_softc),
    cpu_match, cpu_attach, NULL, NULL);


static int
cpu_match(device_t parent, cfdata_t match, void *aux)
{

	return 1;
}

static void
cpu_attach(device_t parent, device_t self, void *aux)
{
	struct cpu_softc *sc = device_private(self);
	ACPI_MADT_LOCAL_SAPIC *sapic = (ACPI_MADT_LOCAL_SAPIC *)aux;
	struct cpu_info *ci;
	uint64_t lid;
	int id, eid;

	aprint_naive("\n");
	aprint_normal(": ProcessorID %d, Id %d, Eid %d%s\n",
	    sapic->ProcessorId, sapic->Id, sapic->Eid,
	    sapic->LapicFlags & ACPI_MADT_ENABLED ? "" : " (disabled)");

	/* Get current CPU Id */
	lid = ia64_get_lid();
	id = (lid & 0x00000000ff000000) >> 24;
	eid = (lid & 0x0000000000ff0000) >> 16;

	sc->sc_dev = self;
	if (id == sapic->Id && eid == sapic->Eid)
		ci = curcpu();
	else {
		ci = (struct cpu_info *)kmem_zalloc(sizeof(*ci), KM_NOSLEEP);
		if (ci == NULL) {
			aprint_error_dev(self, "memory alloc failed\n");
			return;
		}
	}
	sc->sc_info = ci;

	ci->ci_cpuid = sapic->ProcessorId;
	ci->ci_intrdepth = -1;			/* need ? */
	ci->ci_dev = self;

	identifycpu(sc);

	return;
}


static void
identifycpu(struct cpu_softc *sc)
{
	uint64_t vendor[3];
	const char *family_name, *model_name;
	uint64_t features, tmp;
	int number, revision, model, family, archrev;
	char bitbuf[32];
	extern uint64_t processor_frequency;

	/*
	 * Assumes little-endian.
	 */
	vendor[0] = ia64_get_cpuid(0);
	vendor[1] = ia64_get_cpuid(1);
	vendor[2] = '\0';

	tmp = ia64_get_cpuid(3);
	number = (tmp >> 0) & 0xff;
	revision = (tmp >> 8) & 0xff;
	model = (tmp >> 16) & 0xff;
	family = (tmp >> 24) & 0xff;
	archrev = (tmp >> 32) & 0xff;

	family_name = model_name = "unknown";
	switch (family) {
	case 0x07:
		family_name = "Itanium";
		model_name = "Merced";
		break;
	case 0x1f:
		family_name = "Itanium 2";
		switch (model) {
		case 0x00:
			model_name = "McKinley";
			break;
		case 0x01:
			/*
			 * Deerfield is a low-voltage variant based on the
			 * Madison core. We need circumstantial evidence
			 * (i.e. the clock frequency) to identify those.
			 * Allow for roughly 1% error margin.
			 */
			tmp = processor_frequency >> 7;
			if ((processor_frequency - tmp) < 1*GHz &&
			    (processor_frequency + tmp) >= 1*GHz)
				model_name = "Deerfield";
			else
				model_name = "Madison";
			break;
		case 0x02:
			model_name = "Madison II";
			break;
		}
		break;
	}
	snprintf(cpu_model, sizeof(cpu_model), "%s", model_name);

	features = ia64_get_cpuid(4);

	aprint_normal_dev(sc->sc_dev, "%s (", model_name);
	if (processor_frequency) {
		aprint_normal("%ld.%02ld-MHz ",
		    (processor_frequency + 4999) / MHz,
		    ((processor_frequency + 4999) / (MHz/100)) % 100);
	}
	aprint_normal("%s)\n", family_name);
	aprint_normal_dev(sc->sc_dev, "Origin \"%s\",  Revision %d\n",
	    (char *)vendor, revision);

#define IA64_FEATURES_BITMASK "\177\020"				\
    "b\0LB\0"	/* 'brl' instruction is implemented */			\
    "b\1SD\0"	/* Processor implements sportaneous deferral */		\
    "b\2AO\0"	/* Processor implements 16-byte atomic operations */	\
    "\0"
	snprintb(bitbuf, sizeof(bitbuf), IA64_FEATURES_BITMASK, features);
	aprint_normal_dev(sc->sc_dev, "Features %s\n", bitbuf);
}
