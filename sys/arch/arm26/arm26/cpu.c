/* $NetBSD: cpu.c,v 1.1.6.5 2001/01/05 17:34:00 bouyer Exp $ */

/*-
 * Copyright (c) 2000 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/* This file is part of NetBSD/arm26 -- a port of NetBSD to ARM2/3 machines. */
/*
 * cpu.c - high-level CPU detection etc
 */

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.1.6.5 2001/01/05 17:34:00 bouyer Exp $");

#include <sys/device.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/user.h>
#include <machine/armreg.h>
#include <machine/machdep.h>
#include <machine/pcb.h>

#include <arch/arm26/arm26/cpuvar.h>

#include "opt_cputypes.h"

static int cpu_match(struct device *, struct cfdata *, void *);
static void cpu_attach(struct device *, struct device *, void *);
static int cpu_search(struct device *, struct cfdata *, void *);
static register_t cpu_identify(void);
#ifdef CPU_ARM3
static void cpu_arm3_setup(struct device *, int);
#endif
static void cpu_delay_calibrate(struct device *);

register_t cpu_type;

struct cpu_softc {
	struct device sc_dev;
};

struct cfattach cpu_ca = {
	sizeof(struct cpu_softc), cpu_match, cpu_attach
};

/* cf_flags bits */
#define CFF_NOCACHE	0x00000001

static int
cpu_match(struct device *parent, struct cfdata *cf, void *aux)
{

	if (cf->cf_unit == 0)
		return 1;
	return 0;
}

static void
cpu_attach(struct device *parent, struct device *self, void *aux)
{
	int supported;

	printf(": ");
	cpu_type = cpu_identify();
	supported = 0;
	switch (cpu_type & CPU_ID_CPU_MASK) {
	case CPU_ID_ARM2:
		printf("ARM2");
#ifdef CPU_ARM2
		supported = 1;
#endif
		break;
	case CPU_ID_ARM250:
		printf("ARM250");
#ifdef CPU_ARM250
		supported = 1;
#endif
		break;
	case CPU_ID_ARM3:
		printf("ARM3 (rev. %d)", cpu_type & CPU_ID_REVISION_MASK);
#ifdef CPU_ARM3
		supported = 1;
		cpu_arm3_setup(self, self->dv_cfdata->cf_flags);
#endif
		break;
	default:
		printf("Unknown type, ID=0x%08x", cpu_type);
		break;
	}
	printf("\n");
	if (!supported)
		printf("%s: WARNING: CPU type not supported by kernel\n",
		       self->dv_xname);
	config_interrupts(self, cpu_delay_calibrate);
	config_search(cpu_search, self, NULL);
}

static int
cpu_search(struct device *parent, struct cfdata *cf, void *aux)
{
	
	if ((cf->cf_attach->ca_match)(parent, cf, NULL) > 0)
		config_attach(parent, cf, NULL, NULL);

	return 0;
}

static register_t
cpu_identify()
{
	label_t here;
	register_t dummy;
	volatile register_t id;

	if (setjmp(&here) == 0) {
		curproc->p_addr->u_pcb.pcb_onundef_lj = &here;
		id = CPU_ID_ARM2;
		/* ARM250 and ARM3 support SWP. */
		asm volatile ("swp r0, r0, [%0]" : : "r" (&dummy) : "r0");
		id = CPU_ID_ARM250;
		/* ARM3 has an internal coprocessor 15 with an ID register. */
		asm volatile ("mrc 15, 0, %0, cr0, cr0" : "=r" (id));
	}
	curproc->p_addr->u_pcb.pcb_onundef_lj = NULL;
	return id;
}

#ifdef CPU_ARM3

#define ARM3_READ(reg, var) \
	asm ("mrc 15, 0, %0, cr" __STRING(reg) ", cr0" : "=r" (var))
#define ARM3_WRITE(reg, val) \
	asm ("mcr 15, 0, %0, cr" __STRING(reg) ", cr0" : : "r" (val))

static void
cpu_arm3_setup(struct device *self, int flags)
{

	/* Disable the cache while we set things up. */
	ARM3_WRITE(ARM3_CP15_CONTROL, ARM3_CTL_SHARED);
	if (flags & CFF_NOCACHE) {
		printf(", cache disabled");
		return;
	}
	/* All RAM and ROM is cacheable. */
	ARM3_WRITE(ARM3_CP15_CACHEABLE, 0xfcffffff);
	/* All RAM is updateable. */
	ARM3_WRITE(ARM3_CP15_UPDATEABLE, 0x00ffffff);
	/* Nothing is disruptive.  We'll do cache flushing manually. */
	ARM3_WRITE(ARM3_CP15_DISRUPTIVE, 0x00000000);
	/* Flush the cache and turn it on. */
	ARM3_WRITE(ARM3_CP15_FLUSH, 0);
	ARM3_WRITE(ARM3_CP15_CONTROL, ARM3_CTL_CACHE_ON | ARM3_CTL_SHARED);
	printf(", cache enabled");
	cpu_delay_factor = 8;
}
#endif

/* XXX This should be inlined. */
void
cpu_cache_flush(void)
{

#ifdef CPU_ARM3
#if defined(CPU_ARM2) || defined(CPU_ARM250)
	if ((cpu_type & CPU_ID_CPU_MASK) == CPU_ID_ARM3)
#endif
		ARM3_WRITE(ARM3_CP15_FLUSH, 0);
#endif
}

int cpu_delay_factor = 1;

static void
cpu_delay_calibrate(struct device *self)
{
	struct timeval start, end, diff;

	microtime(&start);
	cpu_delayloop(10000);
	microtime(&end);
	timersub(&end, &start, &diff);
	cpu_delay_factor = 10000 / diff.tv_usec + 1;
	printf("%s: 10000 loops in %ld microseconds, delay factor = %d\n",
	       self->dv_xname, diff.tv_usec, cpu_delay_factor);
}
