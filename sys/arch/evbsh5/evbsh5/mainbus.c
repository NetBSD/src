/*	$NetBSD: mainbus.c,v 1.5 2003/07/15 01:37:40 lukem Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* SH5 Evaluation Board Mainbus */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.5 2003/07/15 01:37:40 lukem Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <sh5/sh5/mainbus.h>

static int mainbusmatch(struct device *, struct cfdata *, void *);
static void mainbusattach(struct device *, struct device *, void *);
static int mainbusprint(void *, const char *);

CFATTACH_DECL(mainbus, sizeof(struct device),
    mainbusmatch, mainbusattach, NULL, NULL);
extern struct cfdriver mainbus_cd;

/*
 * Devices which hang off the Peripheral Bridge
 */
static const char *mainbus_devs[] = {"superhyway", NULL};


/*ARGSUSED*/
static int
mainbusmatch(struct device *parent, struct cfdata *cf, void *args)
{
	static int mainbus_matched;

	if (mainbus_matched)
		return (0);

	mainbus_matched = 1;
	return (1);
}

/*ARGSUSED*/
static void
mainbusattach(struct device *parent, struct device *self, void *args)
{
	struct mainbus_attach_args ma;
	int i;

	printf("\n");

	/*
	 * Attach configured children
	 */
	for (i = 0; mainbus_devs[i] != NULL; i++) {
		ma.ma_bust = &_sh5_bus_space_tag;
		ma.ma_dmat = &_sh5_bus_dma_tag;
		ma.ma_name = mainbus_devs[i];

		(void) config_found(self, &ma, mainbusprint);
	}
}

static int
mainbusprint(void *arg, const char *cp)
{
	struct mainbus_attach_args *ma = arg;

	if (cp)
		aprint_normal("%s at %s", ma->ma_name, cp);

	return (UNCONF);
}
