/*	$NetBSD: interrupts.c,v 1.9 2022/12/28 06:00:26 macallan Exp $ */

/*-
 * Copyright (c) 2007 Michael Lorenz
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
__KERNEL_RCSID(0, "$NetBSD: interrupts.c,v 1.9 2022/12/28 06:00:26 macallan Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kernel.h>

#include <prop/proplib.h>

#include <machine/intr.h>
#include <machine/autoconf.h>
#include <powerpc/pic/picvar.h>
#include <powerpc/pic/ipivar.h>
#include <dev/ofw/openfirm.h>
#include <powerpc/oea/cpufeat.h>

#include "opt_interrupt.h"
#include "pic_openpic.h"
#include "pic_ohare.h"
#include "pic_heathrow.h"
#include "pic_u3_ht.h"
#include "opt_ipi.h"
#include "ipi_openpic.h"
#include "ipi_hammerhead.h"

#if NPIC_OPENPIC > 0
static int init_openpic(int);

const char *compat[] = {
	"chrp,open-pic",
	"open-pic",
	"openpic",
	NULL
};

static int 
init_openpic(int pass_through)
{
	uint32_t reg[5];
	uint32_t obio_base, pic_base;
	int      pic, macio;

	macio = OF_finddevice("/pci/mac-io");
	if (macio == -1)
		macio = OF_finddevice("mac-io");
	if (macio == -1)
		macio = OF_finddevice("/ht/pci/mac-io");
	if (macio == -1)
		return FALSE;

	aprint_debug("macio: %08x\n", macio);

	pic = OF_child(macio);
	while ((pic != 0) && !of_compatible(pic, compat))
		pic = OF_peer(pic);

	aprint_debug("pic: %08x\n", pic);
	if ((pic == -1) || (pic == 0))
		return FALSE;

	if (OF_getprop(macio, "assigned-addresses", reg, sizeof(reg)) != 20) 
		return FALSE;

	obio_base = reg[2];
	aprint_debug("obio-base: %08x\n", obio_base);

	if (OF_getprop(pic, "reg", reg, 8) < 8) 
		return FALSE;

	pic_base = obio_base + reg[0];
	aprint_debug("pic-base: %08x\n", pic_base);

	aprint_normal("found openpic PIC at %08x\n", pic_base);
	setup_openpic(oea_mapiodev(pic_base, 0x40000), pass_through);

	return TRUE;
}

#endif /* NPIC_OPENPIC > 0 */

/*
 * look for supported interrupt controllers
 * there are several possible cases:
 * - Ohare and Grand Central models have the same interrupt controller, Ohare
 *   can have a 2nd cascaded one ( Powerbook 3400c with ethernet for example )
 * - Heathrow is more or less Ohare with all the registers doubled to allow 64
 *   IRQs
 * - openpic covers all OpenPICs built into various mac-io found in some G3,
 *   all G4 and most G5 models
 * - u3_ht is an OpenPIC built into the CPC 9x5 bridges / memory controllers
 *   found in G5s, it's got enough quirks to require its own driver. Some
 *   models have both openpic and u3_ht, on those openpic handles IPIs and
 *   normal IRQs while the u3_ht is cascaded and can be used for MSI. On G5s
 *   that have no openpic the u3_ht handles all interrupts, IPIs and MSI
 */
void
init_interrupt(void)
{
	int ok = 0;

	pic_init();
#if NPIC_OHARE > 0
	if (init_ohare())
		goto done;
#endif
#if NPIC_HEATHROW > 0
	if (init_heathrow())
		goto done;
#endif
#if NPIC_OPENPIC > 0
	if (init_openpic(0)) {
#ifdef MULTIPROCESSOR
		setup_openpic_ipi();
#endif
		ok = 1;
	}
#endif
#if NPIC_U3_HT > 0
	if (init_u3_ht())
		ok = 1;
#endif
	if (ok == 0)
	    panic("%s: no supported interrupt controller found", __func__);
#if NPIC_OHARE + NPIC_HEATHROW > 0
done:
#endif
	oea_install_extint(pic_ext_intr);

#ifdef MULTIPROCESSOR
#if (NPIC_OHARE + NPIC_HEATHROW) > 0
	if (OF_finddevice("/hammerhead") != -1)
		setup_hammerhead_ipi();
#endif
#endif /*MULTIPROCESSOR*/
}
