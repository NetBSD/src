/*	$NetBSD: mainbus.c,v 1.7.6.1 2006/04/22 11:38:09 simonb Exp $	*/
/*	NetBSD: mainbus.c,v 1.53 2003/10/27 14:11:47 junyoung Exp 	*/

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
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
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.7.6.1 2006/04/22 11:38:09 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include "hypervisor.h"

#include "opt_xen.h"

#include <machine/cpuvar.h>
#include <machine/i82093var.h>

#ifdef XEN
#include <machine/xen.h>
#include <machine/hypervisor.h>
#endif

int	mainbus_match(struct device *, struct cfdata *, void *);
void	mainbus_attach(struct device *, struct device *, void *);

CFATTACH_DECL(mainbus, sizeof(struct device),
    mainbus_match, mainbus_attach, NULL, NULL);

int	mainbus_print(void *, const char *);

union mainbus_attach_args {
	const char *mba_busname;		/* first elem of all */
	struct cpu_attach_args mba_caa;
#if NHYPERVISOR > 0
	struct hypervisor_attach_args mba_haa;
#endif
};

#ifndef XEN
/*
 * This is set when the ISA bus is attached.  If it's not set by the
 * time it's checked below, then mainbus attempts to attach an ISA.
 */
int	isa_has_been_seen;
struct x86_isa_chipset x86_isa_chipset;
#if XXXNISA > 0
struct isabus_attach_args mba_iba = {
	"isa",
	X86_BUS_SPACE_IO, X86_BUS_SPACE_MEM,
	&isa_bus_dma_tag,
	&x86_isa_chipset
};
#endif /* XXXNISA */

/*
 * Same as above, but for EISA.
 */
int	eisa_has_been_seen;
#endif /* !XEN */

#if defined(MPBIOS) || defined(MPACPI)
struct mp_bus *mp_busses;
int mp_nbus;
struct mp_intr_map *mp_intrs;
int mp_nintr;
 
int mp_isa_bus = -1;            /* XXX */
int mp_eisa_bus = -1;           /* XXX */

#ifdef MPVERBOSE
int mp_verbose = 1;
#else
int mp_verbose = 0;
#endif
#endif


/*
 * Probe for the mainbus; always succeeds.
 */
int
mainbus_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{

	return 1;
}

/*
 * Attach the mainbus.
 */
void
mainbus_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	union mainbus_attach_args mba;

	printf("\n");

	memset(&mba.mba_caa, 0, sizeof(mba.mba_caa));
	mba.mba_caa.caa_name = "cpu";
	mba.mba_caa.cpu_number = 0;
	mba.mba_caa.cpu_role = CPU_ROLE_SP;
	mba.mba_caa.cpu_func = 0;
	config_found_ia(self, "cpubus", &mba.mba_caa, mainbus_print);

#if NHYPERVISOR > 0
	mba.mba_haa.haa_busname = "hypervisor";
	config_found_ia(self, "hypervisorbus", &mba.mba_haa, mainbus_print);
#endif
}

int
mainbus_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	union mainbus_attach_args *mba = aux;

	if (pnp)
		aprint_normal("%s at %s", mba->mba_busname, pnp);
	return (UNCONF);
}
