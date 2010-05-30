/*	$NetBSD: cpu.c,v 1.7.4.1 2010/05/30 05:16:55 rmind Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.7.4.1 2010/05/30 05:16:55 rmind Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpivar.h>


struct cpu_info cpu_info_primary __aligned(CACHE_LINE_SIZE);

static int cpu_match(device_t, cfdata_t, void *);
static void cpu_attach(device_t, device_t, void *);

struct cpu_softc {
	device_t sc_dev;		/* device tree glue */
	struct cpu_info *sc_info;	/* pointer to CPU info */
};

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

	return;
}
