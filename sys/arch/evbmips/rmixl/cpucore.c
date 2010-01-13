/*	$NetBSD: cpucore.c,v 1.1.2.1 2010/01/13 09:40:35 cliff Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge for Wasabi Systems, Inc.
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

#include "locators.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpucore.c,v 1.1.2.1 2010/01/13 09:40:35 cliff Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <evbmips/rmixl/autoconf.h>
#include <evbmips/rmixl/cpucorevar.h>

typedef struct cpucore_softc {
	device_t	sc_dev;
	bool		sc_attached;
} cpucore_softc_t;

static int	cpucore_match(struct device *, struct cfdata *, void *);
static void	cpucore_attach(struct device *, struct device *, void *);
static int	cpucore_print(void *, const char *);

CFATTACH_DECL(cpucore, sizeof(struct device),
    cpucore_match, cpucore_attach, NULL, NULL);

static int
cpucore_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct mainbus_attach_args *aa = aux;
	int core = cf->cf_loc[MAINBUSCF_CORE];

	if (strncmp(aa->ma_name, cf->cf_name, strlen(cf->cf_name)) == 0)
#ifndef MULTIPROCESSOR
	    if (aa->ma_core == 0)
#endif
		if ((core == MAINBUSCF_CORE_DEFAULT)
		||  (core == aa->ma_core))
			return 1;

	return 0;
}

static void
cpucore_attach(struct device *parent, struct device *self, void *aux)
{
	struct cpucore_attach_args aa;
	u_int nthreads;

	aprint_normal(": %lu.%02luMHz (hz cycles = %lu, delay divisor = %lu)\n",
	    curcpu()->ci_cpu_freq / 1000000,
	    (curcpu()->ci_cpu_freq % 1000000) / 10000,
	    curcpu()->ci_cycles_per_hz, curcpu()->ci_divisor_delay);

	aprint_normal("%s: ", device_xname(self));
	cpu_identify(self);

	nthreads = MIPS_CIDFL_RMI_NTHREADS(mycpu->cpu_cidflags);
	aprint_normal("%s: %d %s on core\n", device_xname(self), nthreads,
		nthreads == 1 ? "thread" : "threads");

	/*
	 * Attach CPU (RMI thread) devices
	 */
	for (int i=0; i < nthreads; i++) {
		aa.ca_name = "cpu";
		aa.ca_thread = i;
		config_found(self, &aa, cpucore_print);
	}
}

static int
cpucore_print(void *aux, const char *pnp)
{
	struct cpucore_attach_args *aa = aux;

	if (pnp != NULL)
		aprint_normal("%s: ", pnp);
	aprint_normal(" thread %d", aa->ca_thread);

	return (UNCONF);
}

