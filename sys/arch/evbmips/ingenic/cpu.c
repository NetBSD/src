/*	$NetBSD: cpu.c,v 1.1.2.3 2016/10/05 20:55:27 skrll Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.1.2.3 2016/10/05 20:55:27 skrll Exp $");

#include "opt_ingenic.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/cpu.h>

#include <mips/locore.h>
#include <mips/asm.h>
#include <mips/ingenic/ingenic_regs.h>

static int	cpu_match(device_t, cfdata_t, void *);
static void	cpu_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(cpu, 0,
    cpu_match, cpu_attach, NULL, NULL);

struct cpu_info *startup_cpu_info;
extern void *ingenic_wakeup;

static int
cpu_match(device_t parent, cfdata_t match, void *aux)
{
	struct mainbusdev {
		const char *md_name;
	} *aa = aux;
	if (strcmp(aa->md_name, "cpu") == 0) return 1;
	return 0;
}

static void
cpu_attach(device_t parent, device_t self, void *aux)
{
	struct cpu_info *ci = curcpu();
	int unit;

	if ((unit = device_unit(self)) > 0) {
#ifdef MULTIPROCESSOR
		uint32_t vec, reg;
		int bail = 10000;

		startup_cpu_info = cpu_info_alloc(NULL, unit, 0, unit, 0);
		startup_cpu_info->ci_cpu_freq = ci->ci_cpu_freq;
		ci = startup_cpu_info;
		wbflush();
		vec = (uint32_t)&ingenic_wakeup;
		reg = MFC0(12, 4);	/* reset entry reg */
		reg &= ~REIM_ENTRY_M;
		reg |= vec;
		MTC0(reg, 12, 4);
		reg = MFC0(12, 2);	/* core control reg */
		reg |= CC_RPC1;		/* use our exception vector */
		reg &= ~CC_SW_RST1;	/* get core 1 out of reset */
		MTC0(reg, 12, 2);
		while ((!kcpuset_isset(cpus_hatched, cpu_index(startup_cpu_info))) && (bail > 0)) {
			delay(1000);
			bail--;
		}
		if (!kcpuset_isset(cpus_hatched, cpu_index(startup_cpu_info))) {
			aprint_error_dev(self, "did not hatch\n");
			return;
		}
#else
		aprint_normal_dev(self,
		    "processor off-line; "
		    "multiprocessor support not present in kernel\n");
		return;
#endif

	}
	ci->ci_dev = self;
	self->dv_private = ci;

	aprint_normal(": %lu.%02luMHz (hz cycles = %lu, delay divisor = %lu)\n",
	    ci->ci_cpu_freq / 1000000,
	    (ci->ci_cpu_freq % 1000000) / 10000,
	    ci->ci_cycles_per_hz, ci->ci_divisor_delay);

	aprint_normal_dev(self, "");
	cpu_identify(self);
	cpu_attach_common(self, ci);
}
