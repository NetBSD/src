/*	$NetBSD: mkclock_ap.c,v 1.1 2003/10/25 04:07:28 tsutsui Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mkclock_ap.c,v 1.1 2003/10/25 04:07:28 tsutsui Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <dev/clock_subr.h>
#include <dev/ic/mk48txxreg.h>

#include <newsmips/apbus/apbusvar.h>

#define MKCLOCK_AP_STRIDE	2
#define MKCLOCK_AP_OFFSET	\
    ((MK48T02_CLKOFF + MK48TXX_ICSR) << MKCLOCK_AP_STRIDE)

int  mkclock_ap_match(struct device *, struct cfdata  *, void *);
void mkclock_ap_attach(struct device *, struct device *, void *);
static u_int8_t mkclock_ap_nvrd(bus_space_tag_t bt, bus_space_handle_t, int);
static void mkclock_ap_nvwr(bus_space_tag_t, bus_space_handle_t, int, u_int8_t);

CFATTACH_DECL(mkclock_ap, sizeof(struct device),
    mkclock_ap_match, mkclock_ap_attach, NULL, NULL);

extern struct cfdriver mkclock_cd;

int
mkclock_ap_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct apbus_attach_args *apa = aux;

	if (strcmp("clock", apa->apa_name) != 0)
		return 0;

	return 1;
}

void
mkclock_ap_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct apbus_attach_args *apa = aux;
	bus_space_tag_t bst;
	bus_space_handle_t bsh;
	todr_chip_handle_t handle;

	printf(" slot%d addr 0x%lx", apa->apa_slotno, apa->apa_hwbase);
	if (bus_space_map(bst, apa->apa_hwbase - MKCLOCK_AP_OFFSET,
	    MK48T02_CLKSZ, 0, &bsh) != 0)
		printf("can't map device space\n");

	handle = mk48txx_attach(bst, bsh, "mk48t02", 1900,
	    mkclock_ap_nvrd, mkclock_ap_nvwr);
	if (handle == NULL)
		panic("can't attach tod clock");

	printf("\n");

	handle->bus_cookie = NULL;
	handle->todr_setwen = NULL;

        todr_attach(handle);
}

static u_int8_t
mkclock_ap_nvrd(bt, bh, off)
	bus_space_tag_t bt;
	bus_space_handle_t bh;
	int off;
{
	u_int8_t rv;

	rv = bus_space_read_4(bt, bh, off << MKCLOCK_AP_STRIDE);
	return rv;
}

static void
mkclock_ap_nvwr(bt, bh, off, v)
	bus_space_tag_t bt;
	bus_space_handle_t bh;
	int off;
	u_int8_t v;
{

	bus_space_write_4(bt, bh, off << MKCLOCK_AP_STRIDE, v);
}
