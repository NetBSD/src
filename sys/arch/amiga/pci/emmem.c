/*	$NetBSD: emmem.c,v 1.2.4.2 2013/02/25 00:28:22 tls Exp $ */

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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

/*
 * Handle (optional) PCI memory space on Elbox Mediator bridges.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/extent.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <amiga/dev/zbusvar.h>
#include <amiga/pci/empbreg.h>
#include <amiga/pci/emmemvar.h>

static int	emmem_match(device_t, cfdata_t, void *);
static void	emmem_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(emmem, sizeof(struct emmem_softc),
    emmem_match, emmem_attach, NULL, NULL);

static int
emmem_match(device_t parent, cfdata_t cf, void *aux)
{
	struct zbus_args *zap;

	zap = aux;

	if (zap->manid != ZORRO_MANID_ELBOX)
		return 0;

	switch (zap->prodid) {
	case ZORRO_PRODID_MED1K2_MEM:	/* Mediator 1200 family */
	case ZORRO_PRODID_MED1K2SX_MEM:
	case ZORRO_PRODID_MED1K2LT2_MEM:
	case ZORRO_PRODID_MED1K2LT4_MEM:
	case ZORRO_PRODID_MED1K2TX_MEM:
	case ZORRO_PRODID_MEDZIV_MEM:	/* Mediator ZIV, not really yet */
	case ZORRO_PRODID_MED4K_MEM:	/* Mediator 4000 family */
	case ZORRO_PRODID_MED4KMKII_MEM:
		return 1;
	}

	return 0;
}

static void
emmem_attach(device_t parent, device_t self, void *aux)
{
	struct zbus_args *zap;
	struct emmem_softc *sc;

	sc = device_private(self); 
	zap = aux;

	sc->sc_dev = self;
	sc->sc_base = zap->va;
	sc->sc_size = zap->size;

	aprint_normal(": ELBOX Mediator PCI memory window, %d kB\n",
	    zap->size / 1024);

	/*
	 * Do nothing here, empb or em4k should find the emmem devices
	 * and do the right(tm) thing. 
	 */

}

