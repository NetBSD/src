/*	$NetBSD: vme.c,v 1.2 1998/02/04 00:38:34 pk Exp $	*/
/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <vm/vm.h>

#include <dev/vme/vmevar.h>

#include "locators.h"

int		vmeprint __P((void *, const char *));

int
vmeprint(args, name)
	void *args;
	const char *name;
{
	struct vme_attach_args *va = args;

	if (name)
		printf("[%s at %s]", "???", name);

	printf(" addr %lx", va->vma_reg[0]);
	printf(" pri %x", va->vma_pri);
	printf(" vec %x", va->vma_vec);
	return (UNCONF);
}


int
vmesearch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct vme_busattach_args *vba = (struct vme_busattach_args *)aux;
	struct vme_attach_args vma;
	int i;

	vma.vma_bustag = vba->vba_bustag;
	vma.vma_dmatag = vba->vba_dmatag;
	vma.vma_chipset_tag = vba->vba_chipset_tag;

	vma.vma_nreg = 1;
	for (i = 0; i < vma.vma_nreg; i++)
		/* XXX - or whatever shape this config enhancement takes on */
		vma.vma_reg[0] = cf->cf_loc[VMECF_ADDR + i];

	vma.vma_vec = cf->cf_loc[VMECF_VEC];
	vma.vma_pri = cf->cf_loc[VMECF_PRI];

	if ((*cf->cf_attach->ca_match)(parent, cf, &vma) == 0)
		return (0);

	config_attach(parent, cf, &vma, vmeprint);
	return (0);
}
