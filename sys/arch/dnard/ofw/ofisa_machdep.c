/*	$NetBSD: ofisa_machdep.c,v 1.1.2.1 2002/02/11 20:07:39 jdolecek Exp $	*/

/*
 * Copyright 1998
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/ofw/openfirm.h>
#include <dev/isa/isavar.h>
#include <dev/ofisa/ofisavar.h>

int
ofisa_get_isabus_data(phandle, iba)
	int phandle;
	struct isabus_attach_args *iba;
{

	iba->iba_iot = &isa_io_bs_tag;
	iba->iba_memt = &isa_mem_bs_tag;
	iba->iba_dmat = &isa_bus_dma_tag;
	iba->iba_ic = &isa_chipset_tag;

	return (0);
}

int
ofisa_ignore_child(pphandle, cphandle)
	int pphandle, cphandle;
{
	
	return (0);
}

#ifdef COMPAT_OLD_OFW

int
ofisa_md_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct ofbus_attach_args *oba = aux;
	char type[8];
	char name[8];
	int rv;

	rv = 0;
	if (1) {		/* XXX old firmware compat enabled */
		/* match type and name properties */
		if (OF_getprop(oba->oba_phandle, "device_type", type,
		      sizeof(type)) > 0 &&
		    strcmp(type, "isa") == 0 &&
		    OF_getprop(oba->oba_phandle, "name", name,
		      sizeof(name)) > 0 &&
		    strcmp(name, "isa") == 0)
			rv = 4;
	}
	return (rv);
}

#endif /* COMPAT_OLD_OFW */
