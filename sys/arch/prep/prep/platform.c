/*	$NetBSD: platform.c,v 1.12 2005/12/24 22:45:36 perry Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by NONAKA Kimihiro.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: platform.c,v 1.12 2005/12/24 22:45:36 perry Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <dev/pci/pcivar.h>

#include <machine/intr.h>
#include <machine/platform.h>
#include <machine/residual.h>

static struct platform platform_unknown = {
	"",					/* model */
	platform_generic_match,			/* match */
	prep_pci_get_chipset_tag_indirect,	/* pci_setup */
	pci_intr_nofixup,			/* pci_intr_fixup */
	init_intr,				/* init_intr */
	cpu_setup_unknown,			/* cpu_setup */
	reset_unknown,				/* reset */
	obiodevs_nodev,				/* obiodevs */
};

static struct plattab plattab_unknown = {
	NULL,	0
};

const char *obiodevs_nodev[] = {
	NULL
};

struct platform *platform = &platform_unknown;

int
ident_platform(void)
{
	struct plattab *p = &plattab_unknown;
	int matched = -1, match = 0;
	int i, rv;

	if (res == NULL)
		return 0;

	if (strncmp(res->VitalProductData.PrintableModel,
	    "IBM", 3) == 0)
		p = &plattab_ibm;
	else if (strncmp(res->VitalProductData.PrintableModel,
	    "MOT", 3) == 0)
		p = &plattab_mot;
	else if (strncmp(res->VitalProductData.PrintableModel,
	    "BULL ESTRELLA (e0)         (e0)", 31) == 0) /* XXX */
		p = &plattab_mot;

	for (i = 0; i < p->num; i++) {
		rv = (*p->platform[i]->match)(p->platform[i]);
		if (rv > match) {
			match = rv;
			matched = i;
		}
	}
	if (match)
		platform = p->platform[matched];
	return match;
}

int
platform_generic_match(struct platform *p)
{

	if (p == NULL || p->model == NULL)
		return 0;
	if (strcmp(res->VitalProductData.PrintableModel, p->model) != 0)
    		return 0;
	return 1;
}

/* ARGUSED */
void
pci_intr_nofixup(int busno, int device, int swiz, int *intr)
{
}

/* ARGUSED */
void
cpu_setup_unknown(struct device *dev)
{
}

void
reset_unknown(void)
{
}

void
reset_prep_generic(void)
{
	int msr;
	u_char reg;

	__asm volatile("mfmsr %0" : "=r"(msr));
	msr |= PSL_IP;
	__asm volatile("mtmsr %0" :: "r"(msr));

	reg = *(volatile u_char *)(PREP_BUS_SPACE_IO + 0x92);
	reg &= ~1UL;
	*(volatile u_char *)(PREP_BUS_SPACE_IO + 0x92) = reg;
	reg = *(volatile u_char *)(PREP_BUS_SPACE_IO + 0x92);
	reg |= 1;
	*(volatile u_char *)(PREP_BUS_SPACE_IO + 0x92) = reg;
}
