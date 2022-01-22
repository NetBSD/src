/*	$NetBSD: ofisa.c,v 1.35 2022/01/22 11:49:17 thorpej Exp $	*/

/*
 * Copyright 1997, 1998
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
__KERNEL_RCSID(0, "$NetBSD: ofisa.c,v 1.35 2022/01/22 11:49:17 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/bus.h>
#include <sys/intr.h>

#include <dev/ofw/openfirm.h>
#include <dev/isa/isavar.h>
#include <dev/ofisa/ofisavar.h>

#include "isadma.h"

#define	OFW_MAX_STACK_BUF_SIZE	256

static int	ofisamatch(device_t, cfdata_t, void *);
static void	ofisaattach(device_t, device_t, void *);

static int	ofisa_subclass_match(device_t, cfdata_t, void *);

CFATTACH_DECL_NEW(ofisa, 0,
    ofisamatch, ofisaattach, NULL, NULL);

CFATTACH_DECL_NEW(ofisa_subclass, 0,
    ofisa_subclass_match, ofisaattach, NULL, NULL);

extern struct cfdriver ofisa_cd;

int
ofisaprint(void *aux, const char *pnp)
{
	struct ofbus_attach_args *oba = aux;
	char name[64];

	(void)of_packagename(oba->oba_phandle, name, sizeof name);
	if (pnp)
		aprint_normal("%s at %s", name, pnp);
	else
		aprint_normal(" (%s)", name);
	return UNCONF;
}

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "pnpPNP,a00" },
	DEVICE_COMPAT_EOL
};

int
ofisamatch(device_t parent, cfdata_t cf, void *aux)
{
	struct ofbus_attach_args *oba = aux;
	int rv;

	rv = of_compatible_match(oba->oba_phandle, compat_data) ? 5 : 0;
#ifdef _OFISA_MD_MATCH
	if (!rv)
		rv = ofisa_md_match(parent, cf, aux);
#endif

	return (rv);
}

int
ofisa_subclass_match(device_t parent, cfdata_t cf, void *aux)
{
	/* We're attaching "ofisa" to something that knows what it's doing. */
	return 5;
}

void
ofisaattach(device_t parent, device_t self, void *aux)
{
	struct ofbus_attach_args *oba = aux;
	struct isabus_attach_args iba;
	struct ofisa_attach_args aa;
	int child;

	if (ofisa_get_isabus_data(oba->oba_phandle, &iba) < 0) {
		printf(": couldn't get essential bus data\n");
		return;
	}

	printf("\n");

#if NISADMA > 0
	/*
	 * Initialize our DMA state.
	 */
	isa_dmainit(iba.iba_ic, iba.iba_iot, iba.iba_dmat, self);
#endif

	devhandle_t selfh = device_handle(self);
	for (child = OF_child(oba->oba_phandle); child;
	    child = OF_peer(child)) {
		if (ofisa_ignore_child(oba->oba_phandle, child))
			continue;

		memset(&aa, 0, sizeof aa);

		aa.oba.oba_busname = "ofisa";
		aa.oba.oba_phandle = child;
		aa.iot = iba.iba_iot;
		aa.memt = iba.iba_memt;
		aa.dmat = iba.iba_dmat;
		aa.ic = iba.iba_ic;

		config_found(self, &aa, ofisaprint,
		    CFARGS(.devhandle = devhandle_from_of(selfh, child)));
	}
}

int
ofisa_reg_count(int phandle)
{
	int len;

	len = OF_getproplen(phandle, "reg");

	/* nonexistent or obviously malformed "reg" property */
	if (len < 0 || (len % 12) != 0)
		return (-1);
	return (len / 12);
}

int
ofisa_reg_get(int phandle, struct ofisa_reg_desc *descp, int ndescs)
{
	char *buf, *bp, small[OFW_MAX_STACK_BUF_SIZE];
	int i, proplen, rv;

	i = ofisa_reg_count(phandle);
	if (i < 0)
		return (-1);
	proplen = i * 12;
	ndescs = uimin(ndescs, i);

	i = ndescs * 12;
	if (i > OFW_MAX_STACK_BUF_SIZE) {
		buf = malloc(i, M_TEMP, M_WAITOK);
	} else {
		buf = small;
	}

	if (OF_getprop(phandle, "reg", buf, i) != proplen) {
		rv = -1;
		goto out;
	}

	for (i = 0, bp = buf; i < ndescs; i++, bp += 12) {
		if (of_decode_int(&bp[0]) & 1)
			descp[i].type = OFISA_REG_TYPE_IO;
		else
			descp[i].type = OFISA_REG_TYPE_MEM;
		descp[i].addr = of_decode_int(&bp[4]);
		descp[i].len = of_decode_int(&bp[8]);
	}
	rv = i;		/* number of descriptors processed (== ndescs) */

out:
	if (buf != small)
		free(buf, M_TEMP);
	return (rv);
}

void
ofisa_reg_print(struct ofisa_reg_desc *descp, int ndescs)
{
	int i;

	if (ndescs == 0) {
		printf("none");
		return;
	}

	for (i = 0; i < ndescs; i++) {
		printf("%s%s 0x%lx/%ld", i ? ", " : "",
		    descp[i].type == OFISA_REG_TYPE_IO ? "io" : "mem",
		    (long)descp[i].addr, (long)descp[i].len);
	}
}

int
ofisa_intr_count(int phandle)
{
	int len;

	len = OF_getproplen(phandle, "interrupts");

	/* nonexistent or obviously malformed "reg" property */
	if (len < 0 || (len % 8) != 0)
		return (-1);
	return (len / 8);
}

int
ofisa_intr_get(int phandle, struct ofisa_intr_desc *descp, int ndescs)
{
	char *buf, *bp, small[OFW_MAX_STACK_BUF_SIZE];
	int i, proplen, rv;

	i = ofisa_intr_count(phandle);
	if (i < 0)
		return (-1);
	proplen = i * 8;
	ndescs = uimin(ndescs, i);

	i = ndescs * 8;
	if (i > OFW_MAX_STACK_BUF_SIZE) {
		buf = malloc(i, M_TEMP, M_WAITOK);
	} else {
		buf = small;
	}

	if (OF_getprop(phandle, "interrupts", buf, i) != proplen) {
		rv = -1;
		goto out;
	}

	for (i = 0, bp = buf; i < ndescs; i++, bp += 8) {
		descp[i].irq = of_decode_int(&bp[0]);
		switch (of_decode_int(&bp[4])) {
		case 0:
		case 1:
			descp[i].share = IST_LEVEL;
			break;
		case 2:
		case 3:
			descp[i].share = IST_EDGE;
			break;
#ifdef DIAGNOSTIC
		default:
			/* Dunno what to do, so fail. */
			printf("ofisa_intr_get: unknown interrupt type %d\n",
			    of_decode_int(&bp[4]));
			rv = -1;
			goto out;
#endif
		}
	}
	rv = i;		/* number of descriptors processed (== ndescs) */

out:
	if (buf != small)
		free(buf, M_TEMP);
	return (rv);
}

void
ofisa_intr_print(struct ofisa_intr_desc *descp, int ndescs)
{
	int i;

	if (ndescs == 0) {
		printf("none");
		return;
	}

	for (i = 0; i < ndescs; i++) {
		printf("%s%d (%s)", i ? ", " : "", descp[i].irq,
		    descp[i].share == IST_LEVEL ? "level" : "edge");
	}
}

int
ofisa_dma_count(int phandle)
{
	int len;

	len = OF_getproplen(phandle, "dma");

	/* nonexistent or obviously malformed "reg" property */
	if (len < 0 || (len % 20) != 0)
		return (-1);
	return (len / 20);
}

int
ofisa_dma_get(int phandle, struct ofisa_dma_desc *descp, int ndescs)
{
	char *buf, *bp, small[OFW_MAX_STACK_BUF_SIZE];
	int i, proplen, rv;

	i = ofisa_dma_count(phandle);
	if (i < 0)
		return (-1);
	proplen = i * 20;
	ndescs = uimin(ndescs, i);

	i = ndescs * 20;
	if (i > OFW_MAX_STACK_BUF_SIZE) {
		buf = malloc(i, M_TEMP, M_WAITOK);
	} else {
		buf = small;
	}

	if (OF_getprop(phandle, "dma", buf, i) != proplen) {
		rv = -1;
		goto out;
	}

	for (i = 0, bp = buf; i < ndescs; i++, bp += 20) {
		descp[i].drq = of_decode_int(&bp[0]);
		descp[i].mode = of_decode_int(&bp[4]);
		descp[i].width = of_decode_int(&bp[8]);
		descp[i].countwidth = of_decode_int(&bp[12]);
		descp[i].busmaster = of_decode_int(&bp[16]);
	}
	rv = i;		/* number of descriptors processed (== ndescs) */

out:
	if (buf != small)
		free(buf, M_TEMP);
	return (rv);
}

void
ofisa_dma_print(struct ofisa_dma_desc *descp, int ndescs)
{
	char unkmode[16];
	const char *modestr;
	int i;

	if (ndescs == 0) {
		printf("none");
		return;
	}

	for (i = 0; i < ndescs; i++) {
		switch (descp[i].mode) {
		case OFISA_DMA_MODE_COMPAT:
			modestr = "compat";
			break;
		case OFISA_DMA_MODE_A:
			modestr = "A";
			break;
		case OFISA_DMA_MODE_B:
			modestr = "B";
			break;
		case OFISA_DMA_MODE_F:
			modestr = "F";
			break;
		case OFISA_DMA_MODE_C:
			modestr = "C";
			break;
		default:
			snprintf(unkmode, sizeof(unkmode), "??? (%d)",
			    descp[i].mode);
			modestr = unkmode;
			break;
		}

		printf("%s%d %s mode %d-bit (%d-bit count)%s", i ? ", " : "",
		    descp[i].drq, modestr, descp[i].width,
		    descp[i].countwidth,
		    descp[i].busmaster ? " busmaster" : "");

	}
}

void
ofisa_print_model(device_t self, int phandle)
{
	char *model, small[OFW_MAX_STACK_BUF_SIZE];
        int n = OF_getproplen(phandle, "model");

        if (n <= 0)
		return;

	if (n > OFW_MAX_STACK_BUF_SIZE) {
		model = malloc(n, M_TEMP, M_WAITOK);
	} else {
		model = small;
	}

	if (OF_getprop(phandle, "model", model, n) != n)
		goto out;
		
	aprint_normal(": %s\n", model);
	if (self)
		aprint_normal_dev(self, "");
out:
	if (model != small)
		free(model, M_TEMP);
}
