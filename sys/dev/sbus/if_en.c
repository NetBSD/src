/*	$NetBSD: if_en.c,v 1.8 1999/03/29 12:04:43 cjs Exp $	*/

/*
 *
 * Copyright (c) 1996 Charles D. Cranor and Washington University.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Charles D. Cranor and
 *	Washington University.
 * 4. The name of the author may not be used to endorse or promote products
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *
 * i f _ e n _ s b u s . c  
 *
 * author: Chuck Cranor <chuck@ccrc.wustl.edu>
 * started: spring, 1996.
 *
 * SBUS glue for the eni155s card.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>

#include <net/if.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/cpu.h>

#include <dev/sbus/sbusvar.h>

#include <dev/ic/midwayreg.h>
#include <dev/ic/midwayvar.h>


/*
 * local structures
 */

struct en_sbus_softc {
	/* bus independent stuff */
	struct en_softc	esc;		/* includes "device" structure */

	/* sbus glue */
	struct sbusdev	sc_sd;		/* sbus device */
};

/*
 * local defines (SBUS specific stuff)
 */

#define EN_IPL 5

/*
 * prototypes
 */

static	int en_sbus_match __P((struct device *, struct cfdata *, void *));
static	void en_sbus_attach __P((struct device *, struct device *, void *));

/*
 * SBUS autoconfig attachments
 */

struct cfattach en_sbus_ca = {
	sizeof(struct en_sbus_softc), en_sbus_match, en_sbus_attach,
};

/***********************************************************************/

/*
 * autoconfig stuff
 */

static int
en_sbus_match(parent, cf, aux)
	struct device *parent;
        struct cfdata *cf;
	void *aux;

{
	struct sbus_attach_args *sa = aux;

	if (strcmp("ENI-155s", sa->sa_name) == 0)  {
		if (CPU_ISSUN4M) {
#ifdef DEBUG
			printf("%s: sun4m DMA not supported yet\n",
			    sa->sa_name);
#endif
			return (0);
		}
		return (1);
	} else {
		return (0);
	}
}


static void
en_sbus_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;

{
	struct sbus_attach_args *sa = aux;
	struct en_softc *sc = (void *)self;
	struct en_sbus_softc *scs = (void *)self;

	printf("\n");

	if (bus_space_map2(sa->sa_bustag, sa->sa_slot,
			 sa->sa_offset,
			 4*1024*1024,
			 0, 0, &sc->en_base) != 0) {
		printf("%s: cannot map registers\n", self->dv_xname);
		return;
	}

	/* Establish interrupt channel */
	(void)bus_intr_establish(sa->sa_bustag, sa->sa_pri, 0, en_intr, sc);

	sc->ipl = sa->sa_pri;	/* appropriate? */

	sbus_establish(&scs->sc_sd, &sc->sc_dev);

	/*
	 * done SBUS specific stuff
	 */
	en_attach(sc);
}
