/* $NetBSD: cpu.c,v 1.2.18.1 2017/12/03 11:36:01 jdolecek Exp $ */

/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.2.18.1 2017/12/03 11:36:01 jdolecek Exp $");

#include "ioconf.h"

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <mips/locore.h>

#include <machine/autoconf.h>

static int	cpumatch(device_t, cfdata_t, void *);
static void	cpuattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(cpu, 0,
    cpumatch, cpuattach, NULL, NULL);

static int
cpumatch(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	/* make sure that we're looking for a CPU. */
	if (strcmp(ma->ma_name, cpu_cd.cd_name) != 0) {
		return (0);
	}
	return (1);
}

static void
cpuattach(device_t parent, device_t self, void *aux)
{

	printf(": ");
	cpu_identify(self);
}
