/*	$NetBSD: vme.c,v 1.6 1997/03/19 16:24:41 gwr Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross and Jason R. Thorpe.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Generic VME support for the Motorola MVME series of computers.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>

#include <mvme68k/dev/vmevar.h>

int	vmes_match __P((struct device *, struct cfdata *, void *));
int	vmel_match __P((struct device *, struct cfdata *, void *));

void	vme_attach __P((struct device *, struct device *, void *));

int	vme_search __P((struct device *, struct cfdata *, void *));
int	vme_print __P((void *, const char *));

int	vmechip_print __P((void *, const char *));

struct cfdriver vmechip_cd = {
	NULL, "vmechip", DV_DULL
};

struct cfattach vmes_ca = {
	sizeof(struct device), vmes_match, vme_attach
};

struct cfdriver vmes_cd = {
	NULL, "vmes", DV_DULL
};

struct cfattach vmel_ca = {
	sizeof(struct device), vmel_match, vme_attach
};

struct cfdriver vmel_cd = {
	NULL, "vmel", DV_DULL
};

/* Pointer to the system vmechip softc. */
struct	vmechip_softc *sys_vmechip;

void
vme_config(sc)
	struct vmechip_softc *sc;
{
	struct vme_attach_args va;

	/* We can only do this once. */
	if (sys_vmechip)
		panic("vme_config: more than one VME bus?");

	sys_vmechip = sc;

	/* Attach D16 space. */
	bzero(&va, sizeof(va));
	va.va_bustype = VME_D16;
	(void)config_found(&sc->sc_dev, &va, vmechip_print);

	/* Attach D32 space. */
	bzero(&va, sizeof(va));
	va.va_bustype = VME_D32;
	(void)config_found(&sc->sc_dev, &va, vmechip_print);
}

int
vmechip_print(aux, cp)
	void *aux;
	const char *cp;
{
	struct vme_attach_args *va = aux;
	char *busname;

	if (cp) {
		switch (va->va_bustype) {
		case VME_D16:
			busname = vmes_cd.cd_name;
			break;

		case VME_D32:
			busname = vmel_cd.cd_name;
			break;

		default:
			panic("vmechip_print: impossible bustype");
		}

		printf("%s at %s", busname, cp);
	}

	return (UNCONF);
}

int
vmes_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct vme_attach_args *va = aux;

	if (va->va_bustype != VME_D16)
		return (0);

	return (1);
}

int
vmel_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct vme_attach_args *va = aux;

	if (va->va_bustype != VME_D32)
		return (0);

	return (1);
}

void
vme_attach(parent, self, args)
	struct device *parent;
	struct device *self;
	void *args;
{
	printf("\n");

	/* We know va_bustype == VME_Dxx */
	(void) config_search(vme_search, self, args);
}

int
vme_search(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct vme_attach_args *va = aux;
	char *name;
	int unit, reject = 0;

	name = cf->cf_driver->cd_name;
	unit = cf->cf_unit;

	/* Send in the clones. */
	if (cf->cf_fstate == FSTATE_STAR) {
		printf("vme_search: device `%s' is cloned\n", name);
		panic("clone devices not allowed on VME");
	}

	/* Fill in vme_attach_args */
	va->va_atype = cf->vmecf_atype;
	va->va_addr = cf->vmecf_addr;
	va->va_ipl = cf->vmecf_ipl;
	va->va_vec = cf->vmecf_vec;

	/*
	 * No wildcards allowed.
	 * XXX Should check for a wildcard on addr, too, but we don't
	 * XXX in (u_int32_t)-1 isn't 0xffffffff on some weird machine.
	 */

	/* atype checked later */

	if (va->va_ipl == -1) {
		printf("vme_search: device `%s%d' has wildcarded ipl\n",
		    name, unit);
		reject = 1;
	}
	if (va->va_vec == -1) {
		printf("vme_search: device `%s%d' has wildcarded vec\n",
		    name, unit);
		reject = 1;
	}

	/* Sanity check the atype locator. */
	if (reject == 0)
	switch (va->va_atype) {
	case VME_A16:
	case VME_A24:
	case VME_A32:
		break;	/* Just fine. */

	case -1:	/* wildcarded */
		printf("vme_search: device `%s%d' has wildcarded atype\n",
		    name, unit);
		reject = 1;
		break;

	default:
		printf("vme_search: device `%s%d' has invalid atype `%d'\n",
		    name, unit, va->va_atype);
		reject = 1;
		break;
	}

	if (reject) {
		printf("vme_search: rejecting device `%s%d'\n", name, unit);
		return (0);
	}

	/* Attempt to match.  If we win, attach the device. */
	if ((*cf->cf_attach->ca_match)(parent, cf, va) > 0) {
		config_attach(parent, cf, va, vme_print);
		return (1);
	}

	return (0);
}

int
vme_print(aux, cp)
	void *aux;
	const char *cp;
{
	struct vme_attach_args *va = aux;

	if (cp)
		printf("device at %s", cp);

	printf(" atype %d addr 0x%lx ipl %d vec 0x%x", va->va_atype,
	    va->va_addr, va->va_ipl, va->va_vec);

	return (UNCONF);
}

void
vmeintr_establish(func, arg, ipl, vec)
	int (*func) __P((void *));
	void *arg;
	int ipl, vec;
{

	isrlink_vectored(func, arg, ipl, vec);

	/* Enable VME IRQ. */
	sys_vmechip->sc_irqref[ipl]++;
	(*sys_vmechip->sc_chip->vme_intrline_enable)(ipl);
}

void
vmeintr_disestablish(ipl, vec)
	int ipl, vec;
{

	isrunlink_vectored(vec);

	/* Disable VME IRQ if possible. */
	switch (sys_vmechip->sc_irqref[ipl]) {
	case 0:
		printf("vmeintr_disestablish: nothing using IRQ %d\n", ipl);
		panic("vmeintr_disestablish");
		/* NOTREACHED */

	case 1:
		(*sys_vmechip->sc_chip->vme_intrline_disable)(ipl);
		/* FALLTHROUGH */

	default:
		sys_vmechip->sc_irqref[ipl]--;
	}
}

void *
vmemap(bpa, size, bustype, atype)
	u_long bpa;
	size_t size;
	int bustype, atype;
{
	u_long pa;
	void *rval;

	/* Translate address. */
	if ((*sys_vmechip->sc_chip->vme_translate_addr)(bpa, size,
	    bustype, atype, &pa))
		return (NULL);

	return (iomap(pa, size));
}

void
vmeunmap(kva, size)
	void *kva;
	size_t size;
{

	iounmap(kva, size);
}
