/*	$NetBSD: hb.c,v 1.2 1999/12/21 08:37:36 tsutsui Exp $	*/

/*-
 * Copyright (C) 1999 Izumi Tsutsui.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>

#include <news68k/news68k/isr.h>
#include <news68k/dev/hbvar.h>

static int	hb_match __P((struct device *, struct cfdata *, void *));
static void	hb_attach __P((struct device *, struct device *, void *));
static int	hb_search __P((struct device *, struct cfdata *, void *));
static int	hb_print __P((void *, const char *));

struct cfattach hb_ca = {
	sizeof(struct device), hb_match, hb_attach
};

extern struct cfdriver hb_cd;

static int
hb_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	char *ma_name = aux;

	if (strcmp(ma_name, hb_cd.cd_name) != 0)
		return 0;

	return 1;
}

static void
hb_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct hb_attach_args ha;

	printf("\n");
	bzero(&ha, sizeof(ha));

	config_search(hb_search, self, &ha);
}

static int
hb_search(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct hb_attach_args *ha = aux;

	ha->ha_name = cf->cf_driver->cd_name;
	ha->ha_address = cf->cf_addr;
	ha->ha_ipl = cf->cf_ipl;
	ha->ha_vect = cf->cf_vect;

	if ((*cf->cf_attach->ca_match)(parent, cf, ha) > 0)
		config_attach(parent, cf, ha, hb_print);

	return 0;
}

/*
 * Print out the confargs.  The (parent) name is non-NULL
 * when there was no match found by config_found().
 */
static int
hb_print(args, name)
	void *args;
	const char *name;
{
	struct hb_attach_args *ha = args;

#if 0
	if (ha->ha_addr > 0)
#endif
		printf (" addr 0x%08lx", ha->ha_address);
	if (ha->ha_ipl > 0)
		printf (" ipl %d", ha->ha_ipl);
	if (ha->ha_vect > 0) {
		printf (" vect %d", ha->ha_vect);
	}

	return (QUIET);
}

/*
 * hb_intr_establish: establish hb interrupt
 */
void
hb_intr_establish(hbvect, hand, ipl, arg)
	int hbvect;
	int (*hand) __P((void *)), ipl;
	void *arg;
{

	if ((ipl < 1) || (ipl > 7)) {
		printf("hb: illegal interrupt level: %d\n", ipl);
		panic("hb_intr_establish");
	}

	if ((hbvect < 0) || (hbvect > 255)) {
		printf("hb: illegal vector offset: 0x%x\n", hbvect);
		panic("hb_intr_establish");
	}

	isrlink_vectored(hand, arg, ipl, hbvect);
}

void
hb_intr_disestablish(hbvect)
	int hbvect;
{

	if ((hbvect < 0) || (hbvect > 255)) {
		printf("hb: illegal vector offset: 0x%x\n", hbvect);
		panic("hb_intr_disestablish");
	}

	isrunlink_vectored(hbvect);
}
