/*	$NetBSD: superhyway.c,v 1.1 2002/07/05 13:31:55 scw Exp $	*/

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

/*
 * SH-5 SuperHyway Bus Abstraction
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <sh5/sh5/mainbus.h>
#include <sh5/dev/superhywayvar.h>


static int superhywaymatch(struct device *, struct cfdata *, void *);
static void superhywayattach(struct device *, struct device *, void *);
static int superhywayprint(void *, const char *);

struct cfattach superhyway_ca = {
	sizeof(struct device), superhywaymatch, superhywayattach
};
extern struct cfdriver superhyway_cd;

static const char *superhyway_devices[] = {
	"cpu", "pbridge", "emi", "dmac", "femi", "pchb", NULL
};

/*ARGSUSED*/
static int
superhywaymatch(struct device *parent, struct cfdata *cf, void *args)
{
	struct mainbus_attach_args *ma = args;

	return (strcmp(ma->ma_name, superhyway_cd.cd_name) == 0);
}

/*ARGSUSED*/
static void
superhywayattach(struct device *parent, struct device *self, void *args)
{
	struct mainbus_attach_args *ma = args;
	struct superhyway_attach_args sa;
	int i;

	printf(": SuperHyway bus\n");

	/*
	 * Attach configured children
	 */
	for (i = 0; superhyway_devices[i] != NULL; i++) {
		sa.sa_name = superhyway_devices[i];
		sa.sa_bust = ma->ma_bust;
		sa.sa_dmat = ma->ma_dmat;
		sa.sa_pport = (u_int)-1;

		(void) config_found(self, &sa, superhywayprint);
	}
}

static int
superhywayprint(void *arg, const char *cp)
{
	struct superhyway_attach_args *sa = arg;
	int pport = (int)sa->sa_pport;

	if (pport != 0 && cp)
		printf("%s at %s", sa->sa_name, cp);

	if (pport > 0)
		printf(" p-port 0x%02x", pport);

	return ((pport == 0) ? QUIET : UNCONF);
}
