/*	$NetBSD: cpu_sh.c,v 1.1 2002/07/05 13:31:52 scw Exp $	*/

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
 * SH-5 CPU Module
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <sh5/dev/superhywayvar.h>

#include "locators.h"

static int cpu_shmatch(struct device *, struct cfdata *, void *);
static void cpu_shattach(struct device *, struct device *, void *);

struct cfattach cpu_sh_ca = {
	sizeof(struct device), cpu_shmatch, cpu_shattach
};
extern struct cfdriver cpu_cd;

#define	CPU_SH_MODULE_ID	0x51e2

/*ARGSUSED*/
static int
cpu_shmatch(struct device *parent, struct cfdata *cf, void *args)
{
	struct superhyway_attach_args *sa = args;
	bus_space_handle_t bh;
	u_int64_t vcr;

	if (strcmp(sa->sa_name, cpu_cd.cd_name))
		return (0);

	sa->sa_pport = 0;

	bus_space_map(sa->sa_bust,
	    SUPERHYWAY_PPORT_TO_BUSADDR(cf->cf_loc[SUPERHYWAYCF_PPORT]),
	    SUPERHYWAY_REG_SZ, 0, &bh);
	vcr = bus_space_read_8(sa->sa_bust, bh, SUPERHYWAY_REG_VCR);
	bus_space_unmap(sa->sa_bust, bh, SUPERHYWAY_REG_SZ);

	if (SUPERHYWAY_VCR_MOD_ID(vcr) != CPU_SH_MODULE_ID)
		return (0);

	sa->sa_pport = cf->cf_loc[SUPERHYWAYCF_PPORT];

	return (1);
}

/*ARGSUSED*/
static void
cpu_shattach(struct device *parent, struct device *self, void *args)
{
	struct superhyway_attach_args *sa = args;
	bus_space_handle_t bh;
	u_int64_t vcr;

	bus_space_map(sa->sa_bust, SUPERHYWAY_PPORT_TO_BUSADDR(sa->sa_pport),
	    SUPERHYWAY_REG_SZ, 0, &bh);
	vcr = bus_space_read_8(sa->sa_bust, bh, SUPERHYWAY_REG_VCR);
	bus_space_unmap(sa->sa_bust, bh, SUPERHYWAY_REG_SZ);

	printf(": SH-5 CPU, Version 0x%x\n",
	    (int) SUPERHYWAY_VCR_MOD_VERS(vcr));
}
