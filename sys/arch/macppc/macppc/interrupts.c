/*	$NetBSD: interrupts.c,v 1.1.2.3 2007/05/04 02:49:38 macallan Exp $ */

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
 * 3. Neither the name of The NetBSD Foundation nor the names of its
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
__KERNEL_RCSID(0, "$NetBSD: interrupts.c,v 1.1.2.3 2007/05/04 02:49:38 macallan Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kernel.h>

#include <machine/autoconf.h>
#include <arch/powerpc/pic/picvar.h>
#include <dev/ofw/openfirm.h>

#include "opt_interrupt.h"

#ifdef PIC_OPENPIC
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

	printf("macio: %08x\n", macio);

	pic = OF_child(macio);
	while ((pic != 0) && (of_compatible(pic, compat) == -1))
		pic = OF_peer(pic);

	printf("pic: %08x\n", pic);
	if ((pic == -1) || (pic == 0))
		return FALSE;

	if (OF_getprop(macio, "assigned-addresses", reg, sizeof(reg)) != 20) 
		return FALSE;

	obio_base = reg[2];
	printf("obio-base: %08x\n", obio_base);

	if (OF_getprop(pic, "reg", reg, 8) < 8) 
		return FALSE;

	pic_base = obio_base + reg[0];
	printf("pic-base: %08x\n", pic_base);

	aprint_normal("found openpic PIC at %08x\n", pic_base);
	setup_openpic((void *)pic_base, pass_through);

	return TRUE;
}

#endif /* PIC_OPENPIC */

void
init_interrupt(void)
{

	pic_init();
#ifdef PIC_OHARE
	if (init_ohare())
		goto done;
#endif
#ifdef PIC_GRANDCENTRAL
	if (init_grandcentral())
		goto done;
#endif
#ifdef PIC_HEATHROW
	if (init_heathrow())
		goto done;
#endif
#ifdef PIC_OPENPIC
	if (init_openpic(0))
		goto done;
#endif
	panic("%s: no supported interrupt controller found", __func__);
done:
	oea_install_extint(pic_ext_intr);
}
