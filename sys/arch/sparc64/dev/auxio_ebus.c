/*	$NetBSD: auxio_ebus.c,v 1.4 2000/04/13 09:53:49 mrg Exp $	*/

/*
 * Copyright (c) 2000 Matthew R. Green
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * AUXIO registers support on the Ebus2.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>

#include <sparc64/dev/ebusreg.h>
#include <sparc64/dev/ebusvar.h>
#include <sparc64/dev/auxioreg.h>
#include <sparc64/dev/auxiovar.h>

#define	AUXIO_ROM_NAME		"auxio"

int	auxio_ebus_match __P((struct device *, struct cfdata *, void *));
void	auxio_ebus_attach __P((struct device *, struct device *, void *));

struct cfattach auxio_ebus_ca = {
	sizeof(struct device), auxio_ebus_match, auxio_ebus_attach
};

/*
 * We export this structure so that anyone who wishes to fiddle the
 * AUXIO registers can.
 *
 * XXX we need to better design the access here for when there are
 * multiple Ebus2's in one system..
 */
struct auxio_registers auxio_registers;

int
auxio_ebus_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct ebus_attach_args *ea = aux;

	return (strcmp(AUXIO_ROM_NAME, ea->ea_name) == 0);
}

void
auxio_ebus_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct ebus_attach_args *ea = aux;
	int i, first = 1;

	/*
	 * for each of the registers found, if we know the matching
	 * AUXIO register, set it up.
	 */
	for (i = 0; i < ea->ea_nregs; i++) {

#define	SHOWIT(s) do { \
	if (first) { \
		first = 0; \
		printf(": found"); \
	} \
	printf(" %s", s); \
} while (0)

		if (ea->ea_regs[i].lo == AUXIO_FD) {
			SHOWIT("fd");
			auxio_registers.auxio_fd.lo = ea->ea_regs[i].lo;
			auxio_registers.auxio_fd.hi = ea->ea_regs[i].hi;
		} else if (ea->ea_regs[i].lo == AUXIO_AUDIO) {
			SHOWIT("audio");
			auxio_registers.auxio_audio.lo = ea->ea_regs[i].lo;
			auxio_registers.auxio_audio.hi = ea->ea_regs[i].hi;
		} else if (ea->ea_regs[i].lo == AUXIO_POWER) {
			SHOWIT("power");
			auxio_registers.auxio_power.lo = ea->ea_regs[i].lo;
			auxio_registers.auxio_power.hi = ea->ea_regs[i].hi;
		} else if (ea->ea_regs[i].lo == AUXIO_LED) {
			SHOWIT("led");
			auxio_registers.auxio_led.lo = ea->ea_regs[i].lo;
			auxio_registers.auxio_led.hi = ea->ea_regs[i].hi;
		} else if (ea->ea_regs[i].lo == AUXIO_PCI) {
			SHOWIT("pci");
			auxio_registers.auxio_pci.lo = ea->ea_regs[i].lo;
			auxio_registers.auxio_pci.hi = ea->ea_regs[i].hi;
		} else if (ea->ea_regs[i].lo == AUXIO_FREQ) {
			SHOWIT("freq");
			auxio_registers.auxio_freq.lo = ea->ea_regs[i].lo;
			auxio_registers.auxio_freq.hi = ea->ea_regs[i].hi;
		} else if (ea->ea_regs[i].lo == AUXIO_SCSI) {
			SHOWIT("scsi");
			auxio_registers.auxio_scsi.lo = ea->ea_regs[i].lo;
			auxio_registers.auxio_scsi.hi = ea->ea_regs[i].hi;
		} else if (ea->ea_regs[i].lo == AUXIO_TEMP) {
			SHOWIT("temp");
			auxio_registers.auxio_temp.lo = ea->ea_regs[i].lo;
			auxio_registers.auxio_temp.hi = ea->ea_regs[i].hi;
		} else {
			printf(": unknown auxio register %x.%x",
			    ea->ea_regs[i].hi, ea->ea_regs[i].lo);
		}
	}
	printf("\n");
}
