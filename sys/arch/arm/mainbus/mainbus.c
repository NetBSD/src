/* $NetBSD: mainbus.c,v 1.9 2003/07/15 00:24:47 lukem Exp $ */

/*
 * Copyright (c) 1994,1995 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * mainbus.c
 *
 * mainbus configuration
 *
 * Created      : 15/12/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.9 2003/07/15 00:24:47 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/device.h>

#if defined(arm32)		/* XXX */
#include <machine/io.h>
#endif
#include <machine/bus.h>
#include <arm/mainbus/mainbus.h>
#include "locators.h"

/*
 * mainbus is a root device so we a bus space tag to pass to children
 *
 * The tag is provided by mainbus_io.c and mainbus_io_asm.S
 */

extern struct bus_space mainbus_bs_tag;

/* Prototypes for functions provided */

static int  mainbusmatch  __P((struct device *, struct cfdata *, void *));
static void mainbusattach __P((struct device *, struct device *, void *));
static int  mainbusprint  __P((void *aux, const char *mainbus));
static int  mainbussearch __P((struct device *, struct cfdata *, void *));

/* attach and device structures for the device */

CFATTACH_DECL(mainbus, sizeof(struct device),
    mainbusmatch, mainbusattach, NULL, NULL);

/*
 * int mainbusmatch(struct device *parent, struct cfdata *cf, void *aux)
 *
 * Always match for unit 0
 */

static int
mainbusmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return (1);
}

/*
 * int mainbusprint(void *aux, const char *mainbus)
 *
 * print routine used during config of children
 */

static int
mainbusprint(aux, mainbus)
	void *aux;
	const char *mainbus;
{
	struct mainbus_attach_args *mb = aux;

	if (mb->mb_iobase != MAINBUSCF_BASE_DEFAULT)
		aprint_normal(" base 0x%x", mb->mb_iobase);
	if (mb->mb_iosize > 1)
		aprint_normal("-0x%x", mb->mb_iobase + mb->mb_iosize - 1);
	if (mb->mb_irq != -1)
		aprint_normal(" irq %d", mb->mb_irq);
	if (mb->mb_drq != -1)
		aprint_normal(" drq 0x%08x", mb->mb_drq);

/* XXXX print flags */
	return (QUIET);
}

/*
 * int mainbussearch(struct device *parent, struct device *self, void *aux)
 *
 * search routine used during the config of children
 */

static int
mainbussearch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mainbus_attach_args mb;
	int tryagain;

	do {
		if (cf->cf_loc[MAINBUSCF_BASE] == MAINBUSCF_BASE_DEFAULT) {
			mb.mb_iobase = MAINBUSCF_BASE_DEFAULT;
			mb.mb_iosize = 0;
			mb.mb_drq = MAINBUSCF_DACK_DEFAULT;
			mb.mb_irq = MAINBUSCF_IRQ_DEFAULT;
		} else {    
			mb.mb_iobase = cf->cf_loc[MAINBUSCF_BASE];
#if defined(arm32) /* XXX */
			mb.mb_iobase += IO_CONF_BASE;
#endif
			mb.mb_iosize = 0;
			mb.mb_drq = cf->cf_loc[MAINBUSCF_DACK];
			mb.mb_irq = cf->cf_loc[MAINBUSCF_IRQ];
		}
		mb.mb_iot = &mainbus_bs_tag;

		tryagain = 0;
		if (config_match(parent, cf, &mb) > 0) {
			config_attach(parent, cf, &mb, mainbusprint);
/*			tryagain = (cf->cf_fstate == FSTATE_STAR);*/
		}
	} while (tryagain);

	return (0);
}

/*
 * void mainbusattach(struct device *parent, struct device *self, void *aux)
 *
 * probe and attach all children
 */

static void
mainbusattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	aprint_naive("\n");
	aprint_normal("\n");

	config_search(mainbussearch, self, NULL);
}

/* End of mainbus.c */
