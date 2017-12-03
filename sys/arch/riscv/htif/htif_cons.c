/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

__RCSID("$NetBSD: htif_cons.c,v 1.1.18.2 2017/12/03 11:36:39 jdolecek Exp $");

#include <sys/param.h>
#include <sys/device.h>

#include <riscv/htif/htif_var.h>

static int htif_cons_match(device_t, cfdata_t, void *);
static void htif_cons_attach(device_t, device_t, void *);

struct htif_cons_softc {
	device_t sc_dev;
};

CFATTACH_DECL_NEW(htif_cons, sizeof(struct htif_cons_softc),
    htif_cons_match, htif_cons_attach, NULL, NULL);

int
htif_cons_match(device_t parent, cfdata_t cf, void *aux)
{
	struct htif_attach_args * const haa = aux;

	return !strcmp(haa->haa_name, cf->cf_name);
}

static void
htif_cons_attach(device_t parent, device_t self, void *aux)
{
	//struct htif_attach_args * const haa = aux;
	struct htif_cons_softc * const sc = device_private(self);

	sc->sc_dev = self;
}
