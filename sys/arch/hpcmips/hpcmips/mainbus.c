/*	$NetBSD: mainbus.c,v 1.11 2001/09/15 11:13:21 uch Exp $	*/

/*-
 * Copyright (c) 1999
 *         Shin Takemura and PocketBSD Project. All rights reserved.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include "opt_vr41xx.h"
#include "opt_tx39xx.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

struct mainbus_softc {
	struct	device sc_dv;
};

/* Definition of the mainbus driver. */
static int	mbmatch(struct device *, struct cfdata *, void *);
static void	mbattach(struct device *, struct device *, void *);
static int	mbprint(void *, const char *);
bus_space_tag_t mb_bus_space_init(void);

bus_space_tag_t	mb_bus_space_init(void);

struct cfattach mainbus_ca = {
	sizeof(struct mainbus_softc), mbmatch, mbattach
};

/* There can be only one. */
static int mainbus_found;

static int
mbmatch(struct device *parent, struct cfdata *cf, void *aux)
{

	if (mainbus_found)
		return (0);

	return (1);
}

int ncpus = 0;	/* only support uniprocessors, for now */
bus_space_tag_t system_bus_iot; /* Serial console requires this */

bus_space_tag_t
mb_bus_space_init()
{
	bus_space_tag_t iot;

	iot = hpcmips_alloc_bus_space_tag();
	strcpy(iot->t_name, "System internal");
	iot->t_base = 0x0;
	iot->t_size = 0xffffffff;
	iot->t_extent = 0; /* No extent for bootstraping */
	system_bus_iot = iot;

	return (iot);
}

static void
mbattach(struct device *parent, struct device *self, void *aux)
{
	int i;
	register struct device *mb = self;
	struct mainbus_attach_args ma;
	char *devnames[] = {
		"txsim", "vrip", "bivideo", "btnmgr", "hpcapm",
	};

	mainbus_found = 1;

	printf("\n");

	/* Attach CPU */
	ma.ma_name = "cpu";
	config_found(mb, &ma, mbprint);


#if defined TX39XX && defined VR41XX
/* XXX: currently, the case defined TX39XX && defined VR41XX don't work */
#error misconfiguration
#endif /* defined TX39XX && defined VR41XX */

	/* Platform Specific Function Hooks */
#ifdef VR41XX
#ifdef TX39XX
	if (platid_match(&platid, &platid_mask_CPU_MIPS_VR_41XX))
#endif /* TX39XX */
	{
		if (!system_bus_iot) 
			mb_bus_space_init();
		/* Now prepare extent */
		hpcmips_init_bus_space_extent(system_bus_iot);
		ma.ma_iot = system_bus_iot;
	}
#endif /* VR41XX */
#ifdef TX39XX
#ifdef VR41XX
	if (platid_match(&platid, &platid_mask_CPU_MIPS_TX_3900)
	    || platid_match(&platid, &platid_mask_CPU_MIPS_TX_3920))
#endif /* VR41XX */
	{
		; /* do nothing.. */
	}
#endif /* TX39XX */

	/* Attach devices */
	for (i = 0; i < sizeof(devnames)/sizeof(*devnames); i++) {
		ma.ma_name = devnames[i];
		config_found(mb, &ma, mbprint);
	}
}


static int
mbprint(void *aux, const char *pnp)
{

	if (pnp)
		return (QUIET);

	return (UNCONF);
}
