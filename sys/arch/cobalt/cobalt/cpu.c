/*	$NetBSD: cpu.c,v 1.2 2001/06/27 08:44:24 nisimura Exp $	*/

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

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <mips/locore.h>

#include <machine/cpu.h>
#include <machine/autoconf.h>

static int	cpu_match(struct device *, struct cfdata *, void *);
static void	cpu_attach(struct device *, struct device *, void *);

struct cfattach cpu_ca = {
	sizeof(struct device), cpu_match, cpu_attach
};

static int
cpu_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	return 1;
}

static void
cpu_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	char *cpu_name, *fpu_name;
	extern void rm52xx_idle(void);	/* power saving when idle */

	printf("\n");
	switch (MIPS_PRID_IMPL(cpu_id)) {
	case MIPS_RM5200:
		cpu_name = "QED RM5200 CPU";
		fpu_name = "builtin FPA";
		break;
	default:
		cpu_name = "Unknown CPU";
		fpu_name = "Unknown FPA";
		break;
	}

	printf("%s: %s (0x%04x) with %s (0x%04x)\n",
	    self->dv_xname, cpu_name, cpu_id, fpu_name, fpu_id);

	printf("%s: L1 cache: ", self->dv_xname);
	printf("%dKB Instruction, %dKB Data, 2way set associative\n",
	    mips_L1ICacheSize/1024, mips_L1DCacheSize/1024);
	printf("%s: ", self->dv_xname);

	if (!mips_L2CachePresent)
		printf("no L2 cache\n");
	else
		printf("L2 cache: %dKB/%dB %s, %s\n",
		    mips_L2CacheSize/1024, mips_L2CacheLSize,
		    mips_L2CacheMixed ? "mixed" : "separated",
		    mips_L2CacheIsSnooping? "snooping" : "no snooping");

	/* XXX probably should not be here XXX */
	CPU_IDLE = (long *) rm52xx_idle;
}
