/* $NetBSD: tlsbmem.c,v 1.10.10.1 2011/06/23 14:18:56 cherry Exp $ */

/*
 * Copyright (c) 1997 by Matthew Jacob
 * NASA AMES Research Center.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Dummy Node for TLSB Memory Modules found on
 * AlphaServer 8200 and 8400 systems.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: tlsbmem.c,v 1.10.10.1 2011/06/23 14:18:56 cherry Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <machine/rpb.h>
#include <machine/pte.h>

#include <alpha/tlsb/tlsbreg.h>
#include <alpha/tlsb/tlsbvar.h>

struct tlsbmem_softc {
	device_t	sc_dev;
	int		sc_node;	/* TLSB node */
	uint16_t	sc_dtype;	/* device type */
};

static int	tlsbmemmatch(device_t, cfdata_t, void *);
static void	tlsbmemattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(tlsbmem, sizeof (struct tlsbmem_softc),
    tlsbmemmatch, tlsbmemattach, NULL, NULL);

static int
tlsbmemmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct tlsb_dev_attach_args *ta = aux;
	if (TLDEV_ISMEM(ta->ta_dtype))
		return (1);
	return (0);
}

static void
tlsbmemattach(device_t parent, device_t self, void *aux)
{
	struct tlsb_dev_attach_args *ta = aux;
	struct tlsbmem_softc *sc = device_private(self);

	sc->sc_dev = self;
	sc->sc_node = ta->ta_node;
	sc->sc_dtype = ta->ta_dtype;

	aprint_normal("\n");
}
