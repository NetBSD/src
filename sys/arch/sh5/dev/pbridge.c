/*	$NetBSD: pbridge.c,v 1.9 2003/07/15 03:35:58 lukem Exp $	*/

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
 * SH-5 Peripheral Bridge Controller
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pbridge.c,v 1.9 2003/07/15 03:35:58 lukem Exp $");

#include "opt_sh5_debug.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <sh5/dev/superhywayvar.h>
#include <sh5/dev/pbridgevar.h>
#include <sh5/dev/pbridgereg.h>

#include "locators.h"

static int pbridgematch(struct device *, struct cfdata *, void *);
static void pbridgeattach(struct device *, struct device *, void *);
static int pbridgeprint(void *, const char *);

CFATTACH_DECL(pbridge, sizeof(struct device),
    pbridgematch, pbridgeattach, NULL, NULL);
extern struct cfdriver pbridge_cd;

/*
 * Devices which hang off the Peripheral Bridge
 */
struct pbridge_device {
	const char *pd_name;
	bus_addr_t pd_offset;
};

/*
 * Note: "intc" MUST be first here, as sh5_intr_establish() won't
 * work without it. This also means that "pbridge" must be attached
 * very early on during autoconf.
 */
static struct pbridge_device pbridge_devices[] = {
	{"intc", PBRIDGE_OFFSET_BASE + PBRIDGE_OFFSET_INTC},
	{"cprc", PBRIDGE_OFFSET_BASE + PBRIDGE_OFFSET_CPRC},
	{"tmu", PBRIDGE_OFFSET_BASE + PBRIDGE_OFFSET_TMU},
	{"scif", PBRIDGE_OFFSET_BASE + PBRIDGE_OFFSET_SCIF},
	{"rtc", PBRIDGE_OFFSET_BASE + PBRIDGE_OFFSET_RTC},
	{NULL, 0}
};

/*ARGSUSED*/
static int
pbridgematch(struct device *parent, struct cfdata *cf, void *args)
{
	struct superhyway_attach_args *sa = args;
#ifndef SH5_SIM
	bus_space_handle_t bh;
	u_int64_t vcr;
#endif
	if (strcmp(sa->sa_name, pbridge_cd.cd_name))
		return (0);
#ifndef SH5_SIM
	sa->sa_pport = 0;

	bus_space_map(sa->sa_bust,
	    SUPERHYWAY_PPORT_TO_BUSADDR(cf->cf_loc[SUPERHYWAYCF_PPORT]),
	    SUPERHYWAY_REG_SZ, 0, &bh);
	vcr = bus_space_read_8(sa->sa_bust, bh, SUPERHYWAY_REG_VCR);
	bus_space_unmap(sa->sa_bust, bh, SUPERHYWAY_REG_SZ);

	if (SUPERHYWAY_VCR_MOD_ID(vcr) != PBRIDGE_MODULE_ID)
		return (0);
#endif
	sa->sa_pport = cf->cf_loc[SUPERHYWAYCF_PPORT];

	return (1);
}

/*ARGSUSED*/
static void
pbridgeattach(struct device *parent, struct device *self, void *args)
{
	struct superhyway_attach_args *sa = args;
	struct pbridge_attach_args pa;
	bus_space_handle_t bh;
	u_int64_t vcr;
	int i;

	pa._pa_base = SUPERHYWAY_PPORT_TO_BUSADDR(sa->sa_pport);

	bus_space_map(sa->sa_bust, pa._pa_base, SUPERHYWAY_REG_SZ, 0, &bh);
	vcr = bus_space_read_8(sa->sa_bust, bh, SUPERHYWAY_REG_VCR);
	bus_space_unmap(sa->sa_bust, bh, SUPERHYWAY_REG_SZ);

	printf(": Peripheral Bridge, Version 0x%x\n",
	    (int)SUPERHYWAY_VCR_MOD_VERS(vcr));

	/*
	 * Attach configured children
	 */
	for (i = 0; pbridge_devices[i].pd_name != NULL; i++) {
		pa.pa_name = pbridge_devices[i].pd_name;
		pa.pa_offset = pbridge_devices[i].pd_offset + pa._pa_base;
		pa.pa_bust = sa->sa_bust;
		pa.pa_dmat = sa->sa_dmat;
		pa.pa_ipl = -1;
		pa.pa_intevt = -1;

		(void) config_found(self, &pa, pbridgeprint);
	}
}

static int
pbridgeprint(void *arg, const char *cp)
{
	struct pbridge_attach_args *pa = arg;

	if (cp)
		aprint_normal("%s at %s", pa->pa_name, cp);

	aprint_normal(" offset 0x%x", pa->pa_offset - pa->_pa_base);

	if (pa->pa_ipl != -1)
		aprint_normal(" ipl %d", pa->pa_ipl);
	if (pa->pa_intevt != -1)
		aprint_normal(" intevt 0x%x", pa->pa_intevt);

	return (UNCONF);
}
