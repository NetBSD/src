/*	$NetBSD: deq.c,v 1.8.18.1 2017/12/03 11:36:25 jdolecek Exp $	*/

/*-
 * Copyright (C) 2005 Michael Lorenz
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * a dummy device to attach to OF's deq node marking the TAS3004 audio mixer /
 * equalizer chip, needed by snapper
 */
 
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: deq.c,v 1.8.18.1 2017/12/03 11:36:25 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <dev/ofw/openfirm.h>
#include <dev/i2c/i2cvar.h>

#include <machine/autoconf.h>
#include <macppc/dev/deqvar.h>

static void deq_attach(device_t, device_t, void *);
static int deq_match(device_t, struct cfdata *, void *);

CFATTACH_DECL_NEW(deq, sizeof(struct deq_softc),
    deq_match, deq_attach, NULL, NULL);

static const char * deq_compats[] = {
	"deq",
	"tas3004",
	"pcm3052",
	"cs8416",
	"codec",
	NULL
};

int
deq_match(device_t parent, struct cfdata *cf, void *aux)
{
	struct i2c_attach_args *ia = aux;
	
	if (ia->ia_name) {
		if (ia->ia_ncompat > 0) {
			if (iic_compat_match(ia, deq_compats))
				return 1;
		}
		if (strcmp(ia->ia_name, "deq") == 0)
			return 1;
	}
	return 0;
}

void
deq_attach(device_t parent, device_t self, void *aux)
{
	struct deq_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	char name[256];

	sc->sc_dev = self;
	sc->sc_node = ia->ia_cookie;
	sc->sc_parent = parent;
	sc->sc_address = (ia->ia_addr & 0x7f);
	sc->sc_i2c = ia->ia_tag;
	if (OF_getprop(sc->sc_node, "compatible", name, 256) <= 0) {
		/* deq has no 'compatible' on my iBook G4 */
		switch (sc->sc_address) {
			case 0x35:
				strcpy(name, "tas3004");
				break;
			default:
				strcpy(name, "unknown");
		}
	}
	aprint_normal(" Audio Codec (%s)\n", name);
}
