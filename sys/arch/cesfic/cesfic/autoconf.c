/*	$NetBSD: autoconf.c,v 1.8 2003/04/01 23:57:01 thorpej Exp $	*/

/*
 * Copyright (c) 1997, 1999
 *	Matthias Drochner.  All rights reserved.
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
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/queue.h>
#include <sys/reboot.h>
#include <sys/tty.h>
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/vmparam.h>
#include <machine/cpu.h>
#include <machine/pmap.h>
#include <machine/pte.h>
#include <m68k/cacheops.h>

#include <cesfic/cesfic/isr.h>

struct	device *booted_device;
int	booted_partition;

u_int	bootdev;

struct evcnt evcnt_fpsp_unimp, evcnt_fpsp_unsupp;

int	mainbusmatch __P((struct device *, struct cfdata *, void *));
void	mainbusattach __P((struct device *, struct device *, void *));
int	mainbussearch __P((struct device *, struct cfdata *, void *));

CFATTACH_DECL(mainbus, sizeof(struct device),
    mainbusmatch, mainbusattach, NULL, NULL);

int
mainbusmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	static int mainbus_matched = 0;

	/* Allow only one instance. */
	if (mainbus_matched)
		return (0);

	mainbus_matched = 1;
	return (1);
}

void
mainbusattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{

	printf("\n");

#ifdef FPSP /* XXX this shouldn't be here but in a "cpu" node */
	evcnt_attach(self, "fpuni", &evcnt_fpsp_unimp);
	evcnt_attach(self, "fpund", &evcnt_fpsp_unsupp);
#endif

	/* Search for and attach children. */
	config_search(mainbussearch, self, NULL);
}

int
mainbussearch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{

	if (config_match(parent, cf, NULL) > 0)
		config_attach(parent, cf, NULL, NULL);
	return (0);
}

int
mainbus_map(physaddr, size, cacheable, virtaddr)
	u_long physaddr;
	int size;
	int cacheable;
	void ** virtaddr;
{

	u_long pa, endpa;
	vm_offset_t va;

	pa = m68k_trunc_page(physaddr);
	endpa = m68k_round_page(physaddr + size);

#ifdef DIAGNOSTIC
	if (endpa <= pa)
		panic("bus_mem_add_mapping: overflow");
#endif

	va = uvm_km_valloc(kernel_map, endpa - pa);
	if (va == 0)
		return (ENOMEM);

	*virtaddr = (void*)(va + (physaddr & PGOFSET));
	for (; pa < endpa; pa += PAGE_SIZE, va += PAGE_SIZE) {
		pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE);
		if (!cacheable) {
			pt_entry_t *pte = kvtopte(va);
			*pte |= PG_CI;
			*pte &= ~PG_CCB;
		}
	}
	TBIAS();

	return (0);
}

void
cpu_configure()
{

	isrinit();

	(void)splhigh();
	if (config_rootfound("mainbus", "mainbus") == NULL)
		panic("no mainbus found");

	(void)spl0();
	cold = 0;

	isrprintlevels();
}

void
cpu_rootconf()
{
	setroot(0, 0);
}
