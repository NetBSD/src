/*	$NetBSD: cpu.c,v 1.4.14.1 2017/12/03 11:36:12 jdolecek Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.4.14.1 2017/12/03 11:36:12 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/cpu.h>

#include <mips/locore.h>

#include <machine/autoconf.h>

#include "ioconf.h"

static int cpumatch(device_t, cfdata_t, void *);
static void cpuattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(cpu, 0,
    cpumatch, cpuattach, NULL, NULL);

static int cpu_attached;

int
cpumatch(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	/* make sure that we're looking for a CPU. */
	if (strcmp(ma->ma_name, cpu_cd.cd_name) != 0)
		return 0;

	return !cpu_attached;
}

void
cpuattach(device_t parent, device_t self, void *aux)
{

	struct cpu_info * const ci = curcpu();

	ci->ci_dev = self;
	self->dv_private = ci;

	aprint_normal(": ");
	cpu_attached = 1;

	cpu_identify(self);
}
