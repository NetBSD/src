/*	$NetBSD: if_cs_ofisa_machdep.c,v 1.1.2.2 2002/02/28 04:11:54 nathanw Exp $	*/

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

/*
 * WARNING: THIS FILE IS VERY SHARK-SPECIFIC!
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/ofw/openfirm.h>
#include <dev/isa/isavar.h>
#include <dev/ofisa/ofisavar.h>

#include <dev/ic/cs89x0reg.h>
#include <dev/ic/cs89x0var.h>

#ifdef COMPAT_OLD_OFW

int
cs_ofisa_md_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct ofisa_attach_args *aa = aux;
	char type[64];
	char name[64];
	char model[64];
	char compatible[64];
	int rv;

	rv = 0;
	if (1) {		/* XXX old firmware compat enabled */
		/* At a minimum, must match type and name properties. */
		if (OF_getprop(aa->oba.oba_phandle, "device_type", type,
		    sizeof(type)) < 0 || strcmp(type, "network") != 0 ||
		    OF_getprop(aa->oba.oba_phandle, "name", name,
		    sizeof(name)) < 0 || strcmp(name, "ethernet") != 0)
			return (0);

		/* Full match on model. */
		if (OF_getprop(aa->oba.oba_phandle, "model", model,
		    sizeof(model)) > 0 && strcmp(model, "CS8900") == 0)
			rv = 3;

		/* Check for compatible match. */
		if (OF_getprop(aa->oba.oba_phandle, "compatible", compatible,
		    sizeof(compatible)) > 0 && pmatch(compatible, "*CS8900*",
		    NULL) > 0)
			rv = 2;
	}
	return (rv);
}

int
cs_ofisa_md_reg_fixup(parent, self, aux, descp, ndescs, ndescsfilled)
	struct device *parent, *self;
	void *aux;
	struct ofisa_reg_desc *descp;
	int ndescs, ndescsfilled;
{

	if (1) {		/* XXX old firmware compat enabled */
		/* We can't provide it. */
		if (ndescs != 2)
			return (ndescsfilled);

		/* Firmware provided it. */
		if (ndescsfilled == 2)
			return (ndescsfilled);

		descp[0].type = OFISA_REG_TYPE_IO;
		descp[0].addr = 0x300;
		descp[0].len  = CS8900_IOSIZE;

		descp[1].type = OFISA_REG_TYPE_MEM;
		descp[1].addr = 0xd0000;
		descp[1].len  = 4096;
	}
	return (ndescsfilled);
}

int
cs_ofisa_md_intr_fixup(parent, self, aux, descp, ndescs, ndescsfilled)
	struct device *parent, *self;
	void *aux;
	struct ofisa_intr_desc *descp;
	int ndescs, ndescsfilled;
{

	if (1)			/* XXX old firmware compat enabled */
	if (ndescs > 0 && ndescsfilled > 0)
                descp[0].share = IST_LEVEL;
	return (ndescsfilled);
}

int *
cs_ofisa_md_media_fixup(parent, self, aux, media, nmediap, defmediap)
	struct device *parent, *self;
	void *aux;
	int *media, *nmediap, *defmediap;
{

	if (1) {		/* XXX old firmware compat enabled */
		if (media == NULL) {
			media = malloc(2 * sizeof(int), M_TEMP, M_NOWAIT);
			if (media == NULL)
				return (NULL);
			media[0] = IFM_ETHER|IFM_10_T;
			media[1] = IFM_ETHER|IFM_10_T|IFM_FDX;
			*nmediap = 2;
			*defmediap = media[0];
		}
	}
	return (media);
}

int
cs_ofisa_md_dma_fixup(parent, self, aux, descp, ndescs, ndescsfilled)
	struct device *parent, *self;
	void *aux;
	struct ofisa_dma_desc *descp;
	int ndescs, ndescsfilled;
{
	struct ofisa_attach_args *aa = aux;

	if (ndescs > 0 && ndescsfilled > 0) {
		if (OF_getproplen(aa->oba.oba_phandle, "no-dma") >= 0)
			descp[0].drq = ISACF_DRQ_DEFAULT;
	}
	return (ndescsfilled);
}

#endif /* COMPAT_OLD_OFW */

int
cs_ofisa_md_cfgflags_fixup(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{

	return (CFGFLG_USE_SA|CFGFLG_IOCHRDY|CFGFLG_NOT_EEPROM);
}
