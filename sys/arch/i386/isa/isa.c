/*-
 * Copyright (c) 1993, 1994 Charles Hannum.
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)isa.c	7.2 (Berkeley) 5/13/91
 *	$Id: isa.c,v 1.55.2.2 1994/10/11 09:46:36 mycroft Exp $
 */

/*
 * Code to manage AT bus
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <machine/pio.h>

#include <i386/isa/isareg.h>
#include <i386/isa/isa_device.h>
#include <i386/isa/isavar.h>
#include <i386/isa/ic/i8042.h>

/* sorry, has to be here, no place else really suitable */
#include <machine/pc/display.h>
u_short *Crtat = (u_short *)MONO_BUF;

/*
 * Configure all ISA devices
 *
 * XXX This code is a hack.  It wants to be new config, but can't be until the
 * interrupt system is redone.  For now, we do some gross hacks to make it look
 * 99% like new config.
 */
static char *msgs[3] = { "", " not configured\n", " unsupported\n" };

struct cfdata *
config_search(fn, parent, aux)
	cfmatch_t fn;
	struct device *parent;
	void *aux;
{
	struct cfdata *cf = 0;
	struct device *dv = 0;
	size_t devsize;
	struct cfdriver *cd;
	struct isa_device *id,
	    *idp =  parent ? (void *)parent->dv_cfdata->cf_loc : 0;

	for (id = isa_devtab; id->id_driver; id++) {
		if (id->id_state == FSTATE_FOUND)
			continue;
		if (id->id_parent != idp)
			continue;
		cd = id->id_driver;
		if (id->id_unit < cd->cd_ndevs) {
			if (cd->cd_devs[id->id_unit] != 0)
				continue;
		} else {
			int old = cd->cd_ndevs, new;
			void **nsp;

			if (old == 0)
				new = MINALLOCSIZE / sizeof(void *);
			else
				new = old * 2;
			while (new <= id->id_unit)
				new *= 2;
			cd->cd_ndevs = new;
			nsp = malloc(new * sizeof(void *), M_DEVBUF, M_NOWAIT);
			if (nsp == 0)
				panic("config_search: %sing dev array",
				    old != 0 ? "expand" : "creat");
			bzero(nsp + old, (new - old) * sizeof(void *));
			if (old != 0) {
				bcopy(cd->cd_devs, nsp, old * sizeof(void *));
				free(cd->cd_devs, M_DEVBUF);
			}
			cd->cd_devs = nsp;
		}
		if (!cf) {
			cf = malloc(sizeof(struct cfdata), M_DEVBUF, M_NOWAIT);
			if (!cf)
				panic("config_search: creating cfdata");
		}
		cf->cf_driver = cd;
		cf->cf_unit = id->id_unit;
		cf->cf_fstate = 0;
		cf->cf_loc = (void *)id;
		cf->cf_flags = id->id_flags;
		cf->cf_parents = 0;
		cf->cf_ivstubs = 0;
		if (dv && devsize != cd->cd_devsize) {
			free(dv, M_DEVBUF);
			dv = 0;
		}
		if (!dv) {
			devsize = cd->cd_devsize;
			dv = malloc(devsize, M_DEVBUF, M_NOWAIT);
			if (!dv)
				panic("config_search: creating softc");
		}
		bzero(dv, cd->cd_devsize);
		dv->dv_class = cd->cd_class;
		dv->dv_cfdata = cf;
		dv->dv_unit = id->id_unit;
		sprintf(dv->dv_xname, "%s%d", cd->cd_name, id->id_unit);
		dv->dv_parent = parent;
		cd->cd_devs[id->id_unit] = dv;
		if (fn) {
			if ((*fn)(parent, dv, aux))
				return cf;
		} else {
			if ((*cd->cd_match)(parent, dv, aux))
				return cf;
		}
		cd->cd_devs[id->id_unit] = 0;
	}
	if (cf)
		free(cf, M_DEVBUF);
	if (dv)
		free(dv, M_DEVBUF);
	return 0;
}

void
config_attach(parent, cf, aux, print)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
	cfprint_t print;
{
	struct isa_device *id = (void *)cf->cf_loc;
	struct cfdriver *cd = cf->cf_driver;
	struct device *dv = cd->cd_devs[id->id_unit];

	cf->cf_fstate = id->id_state = FSTATE_FOUND;
	printf("%s at %s", dv->dv_xname, parent ? parent->dv_xname : "isa0");
	if (print)
		(void) (*print)(aux, (char *)0);
	(*cd->cd_attach)(parent, dv, aux);
}

int
config_found(parent, aux, print)
	struct device *parent;
	void *aux;
	cfprint_t print;
{
	struct cfdata *cf;

	if ((cf = config_search((cfmatch_t)NULL, parent, aux)) != NULL) {
		config_attach(parent, cf, aux, print);
		return 1;
	}
	if (print)
		printf(msgs[(*print)(aux, parent->dv_xname)]);
	return 0;
}

int
isaprint(aux, isa)
	void *aux;
	char *isa;
{
	struct isa_attach_args *ia = aux;

	if (ia->ia_iosize)
		printf(" port 0x%x", ia->ia_iobase);
	if (ia->ia_iosize > 1)
		printf("-0x%x", ia->ia_iobase + ia->ia_iosize - 1);
	if (ia->ia_msize)
		printf(" iomem 0x%x", ia->ia_maddr - atdevbase + 0xa0000);
	if (ia->ia_msize > 1)
		printf("-0x%x",
		    ia->ia_maddr - atdevbase + 0xa0000 + ia->ia_msize - 1);
	if (ia->ia_irq)
		printf(" irq %d", ffs(ia->ia_irq) - 1);
	if (ia->ia_drq != (u_short)-1)
		printf(" drq %d", ia->ia_drq);
	/* XXXX print flags */
	return QUIET;
}

int
isasubmatch(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct isa_device *id = (void *)self->dv_cfdata->cf_loc;
	struct isa_attach_args ia;

	ia.ia_iobase = id->id_iobase;
	ia.ia_iosize = 0x666;
	ia.ia_irq = id->id_irq;
	ia.ia_drq = id->id_drq;
	ia.ia_maddr = id->id_maddr - 0xa0000 + atdevbase;
	ia.ia_msize = id->id_msize;

	if (!(*id->id_driver->cd_match)(parent, self, &ia)) {
		/*
		 * If we don't do this, isa_configure() will repeatedly try to
		 * probe devices that weren't found.  But we need to be careful
		 * to do it only for the ISA bus, or we would cause things like
		 * `com0 at ast? slave ?' to not probe on the second ast.
		 */
		if (!parent)
			id->id_state = FSTATE_FOUND;

		return 0;
	}

	config_attach(parent, self->dv_cfdata, &ia, isaprint);

	return 1;
}

void
isa_configure()
{

	startrtclock();

	while (config_search(isasubmatch, NULL, NULL));

	printf("biomask %x netmask %x ttymask %x\n",
	    (u_short)imask[IPL_BIO], (u_short)imask[IPL_NET],
	    (u_short)imask[IPL_TTY]);

	spl0();
}
