/*	$NetBSD: intreg.c,v 1.26 2006/10/10 17:45:43 tsutsui Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass and Gordon W. Ross.
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

/*
 * This handles multiple attach of autovectored interrupts,
 * and the handy software interrupt request register.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intreg.c,v 1.26 2006/10/10 17:45:43 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/vmmeter.h>

#include <uvm/uvm_extern.h>

#include <m68k/asm_single.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/mon.h>

#include <sun3/sun3/interreg.h>
#include <sun3/sun3/machdep.h>

struct intreg_softc {
	struct device sc_dev;
	volatile u_char *sc_reg;
};

static int  intreg_match(struct device *, struct cfdata *, void *);
static void intreg_attach(struct device *, struct device *, void *);

CFATTACH_DECL(intreg, sizeof(struct intreg_softc),
    intreg_match, intreg_attach, NULL, NULL);

volatile u_char *interrupt_reg;
int intreg_attached;

/* called early (by internal_configure) */
void 
intreg_init(void)
{
	vaddr_t va;

	if (find_prom_map(IREG_ADDR, PMAP_OBIO, 1, &va) != 0) {
		mon_printf("intreg_init\n");
		sunmon_abort();
	}
	interrupt_reg = (void *)va;

	/* Turn off all interrupts until clock_attach */
	*interrupt_reg = 0;
}


static int 
intreg_match(struct device *parent, struct cfdata *cf, void *args)
{
	struct confargs *ca = args;

	/* This driver only supports one instance. */
	if (intreg_attached)
		return (0);

	/* Validate the given address. */
	if (ca->ca_paddr != IREG_ADDR)
		return (0);

	return (1);
}


static void 
intreg_attach(struct device *parent, struct device *self, void *args)
{
	struct intreg_softc *sc = (void *)self;

	printf("\n");

	sc->sc_reg = interrupt_reg;

	intreg_attached = 1;
}


void 
isr_soft_request(int level)
{
	u_char bit;

	if ((level < _IPL_SOFT_LEVEL_MIN) || (level > _IPL_SOFT_LEVEL_MAX))
		return;

	bit = 1 << level;
	single_inst_bset_b(*interrupt_reg, bit);
}

void 
isr_soft_clear(int level)
{
	u_char bit;

	if ((level < _IPL_SOFT_LEVEL_MIN) || (level > _IPL_SOFT_LEVEL_MAX))
		return;

	bit = 1 << level;
	single_inst_bclr_b(*interrupt_reg, bit);
}

