/*	$NetBSD: clkctrl.c,v 1.3.80.1 2008/05/16 02:23:12 yamt Exp $	*/

/*
 * Copyright (c) 2005 Michael Lorenz
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
__KERNEL_RCSID(0, "$NetBSD: clkctrl.c,v 1.3.80.1 2008/05/16 02:23:12 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>

#include <sparc/sparc/vaddrs.h>

static int clkctrl_match(struct device *, struct cfdata *, void *);
static void clkctrl_attach(struct device *, struct device *, void *);

CFATTACH_DECL(clkctrl, sizeof(struct device),
     clkctrl_match, clkctrl_attach, NULL, NULL);

static void tadpole_cpu_sleep(struct cpu_info *);
volatile uint8_t *clkctrl_reg = NULL;

static int
clkctrl_match(struct device *parent, struct cfdata *cf, void *aux)
{
	union obio_attach_args *uoba = aux;

	if ((uoba->uoba_isobio4 != 0) || (clkctrl_reg != NULL))
		return (0);

	return (strcmp("clk-ctrl", uoba->uoba_sbus.sa_name) == 0);
}

static void
clkctrl_attach(struct device *parent, struct device *self, void *aux)
{
	union obio_attach_args *uoba = aux;
	struct sbus_attach_args *sa = &uoba->uoba_sbus;
	bus_space_handle_t bh;
	struct cpu_info *cur;

	if (clkctrl_reg != NULL) {
		aprint_error("unable to attach more than once\n");
		return;
	}
	if (sbus_bus_map(sa->sa_bustag,
			 sa->sa_slot, sa->sa_offset,
			 sizeof(long),
			 BUS_SPACE_MAP_LINEAR, &bh) != 0) {
		aprint_error("unable to map register\n");
		return;
	}

	clkctrl_reg = (volatile uint8_t *)bus_space_vaddr(sa->sa_bustag, bh);

#ifdef DEBUG
	printf(" reg: %x", (uint32_t)clkctrl_reg);
#endif
	cur = curcpu();
	cur->idlespin = tadpole_cpu_sleep;

	printf("\n");
}

/* ARGSUSED */
static void
tadpole_cpu_sleep(struct cpu_info *ci)
{
	if (clkctrl_reg == 0)
		return;
	*clkctrl_reg = 0;
}
