/*	$NetBSD: ibm_6050.c,v 1.1 2001/06/20 14:35:25 nonaka Exp $	*/

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

#include <sys/param.h>

#include <uvm/uvm_extern.h>

#include <machine/intr.h>
#include <machine/psl.h>
#include <machine/platform.h>

#include <dev/isa/isavar.h>

static void ext_intr_ibm_6050(void);

struct platform platform_ibm_6050 = {
	"IBM PPS Model 6050/6070 (E)",		/* model */
	platform_generic_match,			/* match */
	pci_intr_fixup_ibm_6050,		/* pci_intr_fixup */
	ext_intr_ibm_6050,			/* ext_intr */
	cpu_setup_ibm_generic,			/* cpu_setup */
	reset_ibm_generic,			/* reset */
};

void
pci_intr_fixup_ibm_6050(int bus, int dev, int *line)
{
	if (bus != 0)
		return;

	switch (dev) {
	case 12:
	case 13:
	case 16:
	case 18:
	case 22:
		*line = 15;
		break;
	}
}

static void
ext_intr_ibm_6050(void)
{
	u_int8_t irq;
	int r_imen;
	int pcpl;
	struct intrhand *ih;

	/* what about enabling external interrupt in here? */
	pcpl = splhigh();	/* Turn off all */

	irq = *((u_char *)prep_intr_reg + INTR_VECTOR_REG);
	intrcnt2[irq]++;

	r_imen = 1 << irq;

	if ((pcpl & r_imen) != 0) {
		ipending |= r_imen;	/* Masked! Mark this as pending */
		imen |= r_imen;
		isa_intr_mask(imen);
	} else {
		ih = intrhand[irq];
		if (ih == NULL)
			printf("spurious interrupt %d\n", irq);
		while (ih) {
			(*ih->ih_fun)(ih->ih_arg);
			ih = ih->ih_next;
		}

		isa_intr_clr(irq);

		uvmexp.intrs++;
		intrcnt[irq]++;
	}

	splx(pcpl);	/* Process pendings. */
}
