/*	$NetBSD: vme_machdep.c,v 1.7 2000/01/19 13:13:18 leo Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>

#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/iomap.h>
#include <machine/mfp.h>

#include <atari/atari/device.h>
#include <atari/vme/vmevar.h>

static int	vmebusprint __P((void *auxp, const char *));
static int	vmebusmatch __P((struct device *, struct cfdata *, void *));
static void	vmebusattach __P((struct device *, struct device *, void *));

struct cfattach avmebus_ca = {
	sizeof(struct device), vmebusmatch, vmebusattach
};

int
vmebusmatch(pdp, cfp, auxp)
struct device	*pdp;
struct cfdata	*cfp;
void		*auxp;
{
	if(atari_realconfig == 0)
		return (0);
	if (strcmp((char *)auxp, "avmebus") || cfp->cf_unit != 0)
		return(0);
	return(machineid & ATARI_FALCON ? 0 : 1);
}

void
vmebusattach(pdp, dp, auxp)
struct device	*pdp, *dp;
void		*auxp;
{
	struct vmebus_attach_args	vba;

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
	config_found(dp, &vba, vmebusprint);
}

int
vmebusprint(auxp, name)
void		*auxp;
const char	*name;
{
	if(name == NULL)
		return(UNCONF);
	return(QUIET);
}
