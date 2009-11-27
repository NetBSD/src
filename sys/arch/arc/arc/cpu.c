/*	$NetBSD: cpu.c,v 1.17 2009/11/27 03:23:04 rmind Exp $	*/
/*	$OpenBSD: cpu.c,v 1.8 1997/04/19 17:19:41 pefo Exp $ */

/*
 * Copyright (c) 1997 Per Fogelstrom
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
 *	This product includes software developed under OpenBSD by
 *	Per Fogelstrom.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.17 2009/11/27 03:23:04 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/autoconf.h>

#include "ioconf.h"

/* Definition of the driver for autoconfig. */
static int	cpumatch(device_t, cfdata_t, void *);
static void	cpuattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(cpu, 0,
    cpumatch, cpuattach, NULL, NULL);

static int
cpumatch(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;

	/* make sure that we're looking for a CPU. */
	if (strcmp(ca->ca_name, cpu_cd.cd_name) != 0)
		return 0;

	return 1;
}

static void
cpuattach(device_t parent, device_t self, void *aux)
{

	aprint_normal(": ");

#if 1
	cpu_identify();
#else /* XXX - before do this, fix pmax, newsmips */
	cpu_identify(dev);
#endif
}
