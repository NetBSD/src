/*	$NetBSD: if_ie.c,v 1.1.2.1 1999/01/30 21:58:41 scw Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/protosw.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <vm/vm.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <dev/ic/i82586var.h>

#include <mvme68k/dev/pccvar.h>
#include <mvme68k/dev/pcctworeg.h>


#ifdef IE_DEBUG
#define DPRINTF(x)	printf x
#else
#define DPRINTF(x)
#endif

struct ie_pcctwo_softc {
	struct ie_softc sc_ie;
	struct pcctwo	*sc_pcctwo;
};

/* Functions required by the i82586 MI driver */
static void 	ie_reset __P((struct ie_softc *, int));
static void 	ie_atten __P((struct ie_softc *));

static void	ie_copyin __P((struct ie_softc *, void *, int, size_t));
static void	ie_copyout __P((struct ie_softc *, const void *, int, size_t));

static u_int16_t ie_read_16 __P((struct ie_softc *, int));
static void	ie_write_16 __P((struct ie_softc *, int, u_int16_t));
static void	ie_write_24 __P((struct ie_softc *, int, int));

int ie_pcctwo_match __P((struct device *, struct cfdata *, void *));
void ie_pcctwo_attach __P((struct device *, struct device *, void *));


struct cfattach ie_pcctwo_ca = {
	sizeof(struct ie_pcctwo_softc), ie_pcctwo_match, ie_pcctwo_attach
};

extern struct cfdriver ie_cd;


/*
 * i82596 Support Routines for MVME1[67]7 Boards
 */
/* ARGSUSED */
static void
ie_reset(sc, why)
	struct ie_softc *sc;
	int why;
{
	/* No way to reset the chip */
}

static void
ie_atten(sc)
	struct ie_softc *sc;
{
}

static void
ie_copyin (sc, dst, offset, size)
        struct ie_softc *sc;
        void *dst;
        int offset;
        size_t size;
{
}

static void
ie_copyout (sc, src, offset, size)
        struct ie_softc *sc;
        const void *src;
        int offset;
        size_t size;
{
}

static u_int16_t
ie_read_16 (sc, offset)
        struct ie_softc *sc;
        int offset;
{
}

static void
ie_write_16 (sc, offset, value)
        struct ie_softc *sc;
        int offset;
        u_int16_t value;
{
}

static void
ie_write_24 (sc, offset, addr)
        struct ie_softc *sc;
        int offset, addr;
{
}

int
ie_pcctwo_match(parent, cf, args)
	struct device *parent;
	struct cfdata *cf;
	void *args;
{
	struct pcc_attach_args *pa = args;

	if ( strcmp(pa->pa_name, ie_cd.cd_name) )
		return 0;

	return 1;
}

void
ie_pcctwo_attach(parent, self, args)
	struct device *parent;
	struct device *self;
	void   *args;
{
	struct ie_pcctwo_softc *isc = (void *)self;
	struct ie_softc *sc = &isc->sc_ie;
	struct pcc_attach_args *pa = args;
	u_int8_t ethaddr[ETHER_ADDR_LEN];

	i82586_attach(sc, "hello", ethaddr, NULL, 0, 0);
}
