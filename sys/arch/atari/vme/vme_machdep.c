/*	$NetBSD: vme_machdep.c,v 1.18.4.2 2011/06/12 00:23:54 rmind Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vme_machdep.c,v 1.18.4.2 2011/06/12 00:23:54 rmind Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/iomap.h>
#include <machine/mfp.h>

#include <atari/atari/device.h>
#include <atari/vme/vmevar.h>

static int	vmebusprint(void *, const char *);
static int	vmebusmatch(device_t, cfdata_t, void *);
static void	vmebusattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(avmebus, 0,
    vmebusmatch, vmebusattach, NULL, NULL);

int vmebus_attached;

int
vmebusmatch(device_t parent, cfdata_t cf, void *aux)
{

	if (atari_realconfig == 0)
		return 0;
	if (strcmp((char *)aux, "avmebus") || vmebus_attached)
		return 0;
	return (machineid & ATARI_FALCON) ? 0 : 1;
}

void
vmebusattach(device_t parent, device_t self, void *aux)
{
	struct vmebus_attach_args	vba;

	vmebus_attached = 1;

	vba.vba_busname = "vme";
	vba.vba_iot     = beb_alloc_bus_space_tag(NULL);
	vba.vba_memt    = beb_alloc_bus_space_tag(NULL);
	if ((vba.vba_iot == NULL) || (vba.vba_memt == NULL)) {
		printf("beb_alloc_bus_space_tag failed!\n");
		return;
	}

	/*
	 * XXX: Should we use zero or the actual (phys) start of VME memory.
	 */
	vba.vba_iot->base  = 0;
	vba.vba_memt->base = 0;

	printf("\n");
	config_found(self, &vba, vmebusprint);
}

int
vmebusprint(void *aux, const char *name)
{

	if (name == NULL)
		return UNCONF;
	return QUIET;
}
