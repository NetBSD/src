/*	$NetBSD: cpu_mainbus.c,v 1.13.2.2 2017/12/03 11:35:54 jdolecek Exp $	*/

/*
 * Copyright (c) 1995 Mark Brinicombe.
 * Copyright (c) 1995 Brini.
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
 * cpu.c
 *
 * Probing and configuration for the master cpu
 *
 * Created      : 10/10/95
 */

#include "locators.h"
#include "opt_multiprocessor.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu_mainbus.c,v 1.13.2.2 2017/12/03 11:35:54 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <arm/mainbus/mainbus.h>

/*
 * Prototypes
 */
static int cpu_mainbus_match(device_t, cfdata_t, void *);
static void cpu_mainbus_attach(device_t, device_t, void *);
 
/*
 * int cpumatch(device_t parent, cfdata_t cf, void *aux)
 *
 * Probe for the main cpu. Currently all this does is return 1 to
 * indicate that the cpu was found.
 */ 
#ifdef MULTIPROCESSOR
extern u_int arm_cpu_max;
#else
#define	arm_cpu_max		1
#endif
 
static int
cpu_mainbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args * const mb = aux;
	int id = mb->mb_core;

	if (id != MAINBUSCF_CORE_DEFAULT) {
		if (id == 0)
			return cpu_info_store.ci_dev == NULL;
		if (id >= arm_cpu_max)
			return 0;
#ifdef MULTIPROCESSOR
		if (cpu_info[id] != NULL)
			return 0;
#endif
		return 1;
	}

	if (cpu_info_store.ci_dev == NULL) {
		mb->mb_core = 0;
		return 1;
	}

#ifdef MULTIPROCESSOR
	for (id = 1; id < arm_cpu_max; id++) {
		if (cpu_info[id] != NULL)
			continue;
		mb->mb_core = id;
		return 1;
	}
#endif

	return 0;
}

/*
 * void cpusattach(device_t parent, device_t dev, void *aux)
 *
 * Attach the main cpu
 */
  
static void
cpu_mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args * const mb = aux;

	cpu_attach(self, mb->mb_core);
}

CFATTACH_DECL_NEW(cpu_mainbus, 0,
    cpu_mainbus_match, cpu_mainbus_attach, NULL, NULL);
