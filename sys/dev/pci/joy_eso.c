/*	$NetBSD: joy_eso.c,v 1.5 2003/07/14 15:47:25 lukem Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (augustss@netbsd.org).
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
__KERNEL_RCSID(0, "$NetBSD: joy_eso.c,v 1.5 2003/07/14 15:47:25 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/select.h>
#include <sys/audioio.h>

#include <machine/bus.h>

#include <dev/audio_if.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <dev/pci/esovar.h>

#include <dev/ic/joyvar.h>

static int	joy_eso_match __P((struct device *, struct cfdata *, void *));
static void	joy_eso_attach __P((struct device *, struct device *, void *));

CFATTACH_DECL(joy_eso, sizeof (struct joy_softc),
    joy_eso_match, joy_eso_attach, NULL, NULL);

static int
joy_eso_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct audio_attach_args *aa = (struct audio_attach_args *)aux;

	if (aa->type != AUDIODEV_TYPE_AUX)
		return (0);
	return (1);
}

static void
joy_eso_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct eso_softc *esc = (struct eso_softc *)parent;
	struct joy_softc *sc = (struct joy_softc *)self;

	printf("\n");

	sc->sc_ioh = esc->sc_game_ioh;
	sc->sc_iot = esc->sc_game_iot;

	joyattach(sc);
}
