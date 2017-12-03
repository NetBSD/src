/*	$NetBSD: cmi.c,v 1.13.18.1 2017/12/03 11:36:48 jdolecek Exp $ */
/*
 * Copyright (c) 1999 Ludd, University of Lule}, Sweden.
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cmi.c,v 1.13.18.1 2017/12/03 11:36:48 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <machine/nexus.h>
#include <machine/sid.h>
#include <machine/ka750.h>
#include <machine/mainbus.h>

static	int cmi_print(void *, const char *);
static	int cmi_match(device_t, cfdata_t, void *);
static	void cmi_attach(device_t, device_t, void*);

CFATTACH_DECL_NEW(cmi, 0,
    cmi_match, cmi_attach, NULL, NULL);

int
cmi_print(void *aux, const char *name)
{
	struct sbi_attach_args * const sa = aux;

	if (name)
		aprint_normal("unknown device 0x%x at %s", sa->sa_type, name);

	aprint_normal(" tr%d", sa->sa_nexnum);
	return (UNCONF);
}


int
cmi_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args * const ma = aux;

	return !strcmp("cmi", ma->ma_type);
}

void
cmi_attach(device_t parent, device_t self, void *aux)
{
	struct sbi_attach_args sa;

        sa.sa_base = NEX750;

	aprint_normal("\n");
	/*
	 * Probe for memory, can be in the first 4 slots.
	 */
#define NEXPAGES (sizeof(struct nexus) / VAX_NBPG)
	for (sa.sa_nexnum = 0; sa.sa_nexnum < 4; sa.sa_nexnum++) {
		sa.sa_ioh = vax_map_physmem(NEX750 +
		    sizeof(struct nexus) * sa.sa_nexnum, NEXPAGES);
		if (badaddr((void *)sa.sa_ioh, 4)) {
			vax_unmap_physmem((vaddr_t)sa.sa_ioh, NEXPAGES);
		} else {
			sa.sa_type = NEX_MEM16;
			config_found(self, (void*)&sa, cmi_print);
		}
	}

	/*
	 * Probe for mba's, can be in slot 4 - 7.
	 */
	for (sa.sa_nexnum = 4; sa.sa_nexnum < 7; sa.sa_nexnum++) {
		sa.sa_ioh = vax_map_physmem(NEX750 +
		    sizeof(struct nexus) * sa.sa_nexnum, NEXPAGES);
		if (badaddr((void *)sa.sa_ioh, 4)) {
			vax_unmap_physmem((vaddr_t)sa.sa_ioh, NEXPAGES);
		} else {
			sa.sa_type = NEX_MBA;
			config_found(self, (void*)&sa, cmi_print);
		}
	}

	/*
	 * There are always one generic UBA, and maybe an optional.
	 */
	sa.sa_nexnum = 8;
	sa.sa_ioh = vax_map_physmem(NEX750 +
	    sizeof(struct nexus) * sa.sa_nexnum, NEXPAGES);
	sa.sa_type = NEX_UBA0;
	config_found(self, (void*)&sa, cmi_print);

	sa.sa_nexnum = 9;
	sa.sa_ioh = vax_map_physmem(NEX750 +
	    sizeof(struct nexus) * sa.sa_nexnum, NEXPAGES);
	sa.sa_type = NEX_UBA1;
	if (badaddr((void *)sa.sa_ioh, 4))
		vax_unmap_physmem((vaddr_t)sa.sa_ioh, NEXPAGES);
	else
		config_found(self, (void*)&sa, cmi_print);
}
