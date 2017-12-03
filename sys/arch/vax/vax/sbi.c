/*	$NetBSD: sbi.c,v 1.37.14.1 2017/12/03 11:36:48 jdolecek Exp $ */
/*
 * Copyright (c) 1994 Ludd, University of Lule}, Sweden.
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

/*
 * ABus code added by Johnny Billquist 2010
 */

/*
 * Still to do: Write all SBI error handling.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sbi.c,v 1.37.14.1 2017/12/03 11:36:48 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <machine/sid.h>
#include <machine/nexus.h>
#include <machine/mainbus.h>
#include <machine/ioa.h>

static int sbi_print(void *, const char *);
#if VAX780 || VAXANY
static int sbi_mainbus_match(device_t, cfdata_t, void *);
static void sbi_mainbus_attach(device_t, device_t, void*);
#endif
#if VAX8600 || VAXANY
static int sbi_abus_match(device_t, cfdata_t, void *);
static void sbi_abus_attach(device_t, device_t, void*);
#endif

int
sbi_print(void *aux, const char *name)
{
	struct sbi_attach_args *sa = aux;
	bool unsupp = false;

	if (name) {
		switch (sa->sa_type) {
		case NEX_MBA:
			aprint_naive("mba at %s", name);
			aprint_normal("mba at %s", name);
			break;
		case NEX_CI:
			aprint_naive("ci at %s", name);
			aprint_normal("ci at %s", name);
			unsupp = true;
			break;
		default:
			aprint_naive("unknown device 0x%x at %s",
			    sa->sa_type, name);
			aprint_normal("unknown device 0x%x at %s",
			    sa->sa_type, name);
			unsupp = true;
		}		
	}
	aprint_naive(" tr%d", sa->sa_nexnum);
	aprint_normal(" tr%d", sa->sa_nexnum);
	return (unsupp ? UNSUPP : UNCONF);
}

#if VAX780 || VAXANY
int
sbi_mainbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args * const ma = aux;

	return !strcmp("sbi", ma->ma_type);
}

void
sbi_mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args * const ma = aux;
	struct sbi_attach_args sa;
	u_int nexnum;

	aprint_naive("\n");
	aprint_normal("\n");

	sa.sa_iot = ma->ma_iot;
	sa.sa_dmat = ma->ma_dmat;

#define NEXPAGES (sizeof(struct nexus) / VAX_NBPG)
        sa.sa_sbinum = 0;

	for (nexnum = 0; nexnum < NNEXSBI; nexnum++) {
		struct nexus *nexusP = 0;
		volatile int tmp;

		nexusP = (struct nexus *)vax_map_physmem((paddr_t)(NEX780) +
		    sizeof(struct nexus) * nexnum, NEXPAGES);
		if (badaddr((void *)nexusP, 4)) {
			vax_unmap_physmem((vaddr_t)nexusP, NEXPAGES);
		} else {
			tmp = nexusP->nexcsr.nex_csr; /* no byte reads */
			sa.sa_type = tmp & 255;

			sa.sa_nexnum = nexnum;
			sa.sa_ioh = (vaddr_t)nexusP;
                        sa.sa_base = (bus_addr_t)NEX780;
			config_found(self, (void*)&sa, sbi_print);
		}
	}
}

CFATTACH_DECL_NEW(sbi_mainbus, 0,
    sbi_mainbus_match, sbi_mainbus_attach, NULL, NULL);

#endif

#if VAX8600 || VAXANY
int
sbi_abus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct abus_attach_args * const aa = aux;

	return ((aa->aa_type & IOA_TYPMSK) == IOA_SBIA);
}

void
sbi_abus_attach(device_t parent, device_t self, void *aux)
{
	struct abus_attach_args * const aa = aux;
	struct sbi_attach_args sa;
	u_int nexnum, minnex;

	aprint_naive(": SBIA Rev. %d, base address %#x\n", aa->aa_type & 0xf, (unsigned int)aa->aa_base);
	aprint_normal(": SBIA Rev. %d, base address %#x\n", aa->aa_type & 0xf, (unsigned int)aa->aa_base);

	sa.sa_iot = aa->aa_iot;
	sa.sa_dmat = aa->aa_dmat;
        sa.sa_base = aa->aa_base;

#define NEXPAGES (sizeof(struct nexus) / VAX_NBPG)
        minnex = aa->aa_num * NNEXSBI;
        sa.sa_sbinum = aa->aa_num;

	for (nexnum = minnex; nexnum < minnex + NNEXSBI; nexnum++) {
		struct nexus *nexusP = 0;
		volatile int tmp;

		nexusP = (struct nexus *)vax_map_physmem(sa.sa_base +
		    sizeof(struct nexus) * nexnum, NEXPAGES);
		if (badaddr((void *)nexusP, 4)) {
			vax_unmap_physmem((vaddr_t)nexusP, NEXPAGES);
		} else {
			tmp = nexusP->nexcsr.nex_csr; /* no byte reads */
			sa.sa_type = tmp & 255;

			sa.sa_nexnum = nexnum;
			sa.sa_ioh = (vaddr_t)nexusP;
			config_found(self, (void*)&sa, sbi_print);
		}
	}
}


CFATTACH_DECL_NEW(sbi_abus, 0,
                  sbi_abus_match, sbi_abus_attach, NULL, NULL);

#endif
