/*	$NetBSD: lpt_ofisa_machdep.c,v 1.4.94.1 2009/05/13 17:18:23 jym Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lpt_ofisa_machdep.c,v 1.4.94.1 2009/05/13 17:18:23 jym Exp $");

#include "opt_compat_old_ofw.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/ofw/openfirm.h>
#include <dev/isa/isavar.h>
#include <dev/ofisa/ofisavar.h>

#ifdef COMPAT_OLD_OFW

extern int i87307PrinterConfig(bus_space_tag_t, u_int);	/* XXX */

int
lpt_ofisa_md_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct ofisa_attach_args *aa = aux;
	char type[9];
	char name[9];
	int rv;

	rv = 0;
	if (1) {		/* XXX old firmware compat enabled */
		/* match type and name properties */
		if (OF_getprop(aa->oba.oba_phandle, "device_type",
		      type, sizeof(type)) > 0 &&
		    strcmp(type, "parallel") == 0 &&
		    OF_getprop(aa->oba.oba_phandle, "name", name,
		      sizeof(name)) > 0 &&
		    strcmp(name, "parallel") == 0)
			rv = 4;
	}
	return (rv);
}

int
lpt_ofisa_md_intr_fixup(struct device *parent, struct device *self, void *aux, struct ofisa_intr_desc *descp, int ndescs, int ndescsfilled)
{

	if (1)			/* XXX old firmware compat enabled */
	if (ndescs > 0 && ndescsfilled > 0) {
                i87307PrinterConfig(&isa_io_bs_tag, descp[0].irq);
                descp[0].share = IST_LEVEL;
	}
	return (ndescsfilled);
}

#endif /* COMPAT_OLD_OFW */
