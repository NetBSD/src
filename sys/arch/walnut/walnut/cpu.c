/*	$NetBSD: cpu.c,v 1.1 2001/06/13 06:02:01 simonb Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Eduardo Horvath and Simon Burge for Wasabi Systems, Inc.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/walnut.h>

#define IBM405GP	0x4011

struct cputab {
	int version;
	char *name;
};
static struct cputab models[] = {
	{ IBM405GP, "405GP" },
	{ 0,	    NULL }
};

static int	cpumatch(struct device *, struct cfdata *, void *);
static void	cpuattach(struct device *, struct device *, void *);

struct cfattach cpu_ca = {
	sizeof(struct device), cpumatch, cpuattach
};

int ncpus;

struct cpu_info cpu_info_store;

int cpufound = 0;

static int
cpumatch(struct device *parent, struct cfdata *cf, void *aux)
{
	union mainbus_attach_args *maa = aux;

	/* make sure that we're looking for a CPU */
	if (strcmp(maa->mba_rmb.rmb_name, cf->cf_driver->cd_name) != 0)
                return (0);

	return !cpufound;
}

static void
cpuattach(struct device *parent, struct device *self, void *aux)
{
	int pvr, cpu;
	struct cputab *cp = models;

	cpufound++;
	ncpus++;

	asm ("mfpvr %0" : "=r"(pvr));
	cpu = pvr >> 16;

	while (cp->name) {
		if (cp->version == cpu)
			break;
		cp++;
	}
	if (cp->name)
		strcpy(cpu_model, cp->name);
	else
		sprintf(cpu_model, "Version 0x%x", cpu);
	sprintf(cpu_model + strlen(cpu_model), " (Revision %d.%d)",
		(pvr >> 8) & 0xff, pvr & 0xff);

	printf(": %dMHz %s\n", board_data.processor_speed / 1000 / 1000,
	    cpu_model);
}
