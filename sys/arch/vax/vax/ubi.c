/*	$NetBSD: ubi.c,v 1.1 2009/01/22 18:49:03 christos Exp $ */
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed at Ludd, University of 
 *	Lule}, Sweden and its contributors.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
__KERNEL_RCSID(0, "$NetBSD: ubi.c,v 1.1 2009/01/22 18:49:03 christos Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/bus.h>
#include <machine/nexus.h>
#include <machine/cpu.h>
#include <machine/sid.h>
#include <machine/ka730.h>

static	int ubi_print(void *, const char *);
static	int ubi_match(struct device *, struct cfdata *, void *);
static	void ubi_attach(struct device *, struct device *, void*);

CFATTACH_DECL_NEW(ubi, sizeof(struct device),
    ubi_match, ubi_attach, NULL, NULL);

int
ubi_print(void *aux, const char *name)
{
	struct sbi_attach_args *sa = (struct sbi_attach_args *)aux;

	if (name)
		aprint_normal("unknown device 0x%x at %s", sa->sa_type, name);

	aprint_normal(" tr%d", sa->sa_nexnum);
	return (UNCONF);
}


int
ubi_match(struct device *parent, struct cfdata *cf, void *aux)
{
	if (vax_bustype == VAX_UNIBUS)
		return 1;
	return 0;
}

void
ubi_attach(struct device *parent, struct device *self, void *aux)
{
	struct	sbi_attach_args sa;

	printf("\n");

#define NEXPAGES (sizeof(struct nexus) / VAX_NBPG)

#if 0
	/*
	 * Probe for memory, can be in the first 4 slots.
	 */

	for (sa.sa_nexnum = 0; sa.sa_nexnum < 4; sa.sa_nexnum++) {
		sa.sa_ioh = vax_map_physmem(NEX730 +
		    sizeof(struct nexus) * sa.sa_nexnum, NEXPAGES);
		if (badaddr((caddr_t)sa.sa_ioh, 4)) {
			vax_unmap_physmem((vaddr_t)sa.sa_ioh, NEXPAGES);
		} else {
			sa.sa_type = NEX_MEM16;
			config_found(self, (void*)&sa, ubi_print);
		}
	}
#endif

	/* VAX 730 fixed configuration */

	/* memory */
	sa.sa_nexnum = 0;
	sa.sa_ioh = vax_map_physmem((int)NEX730 +
	   sizeof(struct nexus) * sa.sa_nexnum, NEXPAGES);
	sa.sa_type = NEX_MEM16;
	config_found(self, (void*)&sa, ubi_print);

	printf("\n");

	/* generic UBA */
	sa.sa_nexnum = 3;
	sa.sa_ioh = vax_map_physmem((int)NEX730 +
	    sizeof(struct nexus) * sa.sa_nexnum, NEXPAGES);
	sa.sa_type = NEX_UBA0;
	config_found(self, (void*)&sa, ubi_print);
}
