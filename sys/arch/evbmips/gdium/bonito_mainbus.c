/*	$NetBSD: bonito_mainbus.c,v 1.1.2.2 2009/08/19 18:46:12 yamt Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
__KERNEL_RCSID(0, "$NetBSD: bonito_mainbus.c,v 1.1.2.2 2009/08/19 18:46:12 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/reboot.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <mips/bonito/bonitoreg.h>

#include <evbmips/gdium/gdiumvar.h>

static int	bonito_mainbus_match(device_t, cfdata_t, void *);
static void	bonito_mainbus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(bonito_mainbus, 0,
    bonito_mainbus_match, bonito_mainbus_attach, NULL, NULL);

extern struct cfdriver bonito_cd;

int
bonito_mainbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args * const maa = aux;

	if (strcmp(maa->maa_name, bonito_cd.cd_name) == 0)
		return (1);

	return (0);
}

void
bonito_mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct pcibus_attach_args pba;
	struct gdium_config * const gc = &gdium_configuration;
	pcireg_t rev;

	self->dv_private = &gc->gc_bonito;

	/*
	 * There is only one PCI controller on a Loongson chip.
	 */

	rev = PCI_REVISION(REGVAL(BONITO_PCICLASS));

	printf(": BONITO Memory and PCI controller, %s rev. %d.%d\n",
	    BONITO_REV_FPGA(rev) ? "FPGA" : "ASIC",
	    BONITO_REV_MAJOR(rev), BONITO_REV_MINOR(rev));

	pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED;
	pba.pba_bus = 0;
	pba.pba_bridgetag = NULL;

	pba.pba_iot = &gc->gc_iot;
	pba.pba_memt = &gc->gc_memt;
	pba.pba_dmat = &gc->gc_pci_dmat;
	pba.pba_dmat64 = NULL;
	pba.pba_pc = &gc->gc_pc;

	(void) config_found_ia(self, "pcibus", &pba, pcibusprint);
}
