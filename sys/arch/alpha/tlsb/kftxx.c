/* $NetBSD: kftxx.c,v 1.3 1997/04/06 20:09:14 cgd Exp $ */

/*
 * Copyright (c) 1997 by Matthew Jacob
 * NASA AMES Research Center.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * KFTIA and KFTHA Bus Adapter Node for I/O hoses
 * found on AlphaServer 8200 and 8400 systems.
 *
 * i.e., handler for all TLSB I/O nodes.
 */

#include <machine/options.h>		/* Pull in config options headers */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <machine/rpb.h>
#include <machine/pte.h>

#include <alpha/tlsb/tlsbreg.h>
#include <alpha/tlsb/tlsbvar.h>
#include <alpha/tlsb/kftxxreg.h>
#include <alpha/tlsb/kftxxvar.h>
#include <alpha/pci/dwlpxvar.h>

struct kft_softc {
	struct device	sc_dev;
	int		sc_node;	/* TLSB node */
	u_int16_t	sc_dtype;	/* device type */
};

#define KV(_addr)	((caddr_t)ALPHA_PHYS_TO_K0SEG((_addr)))

static int	kftmatch __P((struct device *, struct cfdata *, void *));
static void	kftattach __P((struct device *, struct device *, void *));
struct cfattach kft_ca = {
	sizeof(struct kft_softc), kftmatch, kftattach
};

struct cfdriver	kft_cd = {
	NULL, "kft", DV_DULL,
};

static int	kftprint __P((void *, const char *));
void	kft_intr_establish __P((struct confargs *, int (*)(void *), void *));
void	kft_intr_disestablish __P((struct confargs *));
caddr_t	kft_cvtaddr __P((struct confargs *));
int	kft_matchname __P((struct confargs *, char *));

static int
kftprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	register struct kft_dev_attach_args *ka = aux;
	if (pnp)
		 printf("%s at %s", ka->ka_name, pnp);
	return (UNCONF);
}

static int
kftmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct tlsb_dev_attach_args *ta = aux;
	if (TLDEV_ISIOPORT(ta->ta_dtype))
		return (1);
	return (0);
}

static void
kftattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct tlsb_dev_attach_args *ta = aux;
	struct kft_softc *sc = (struct kft_softc *)self;
	struct kft_dev_attach_args ka;
	int hoseno;

	sc->sc_dev = *self;;
	sc->sc_node = ta->ta_node;
	sc->sc_dtype = ta->ta_dtype;
	printf("\n");

	for (hoseno = 0; hoseno < MAXHOSE; hoseno++) {
		u_int32_t value =
		    TLSB_GET_NODEREG(sc->sc_node, KFT_IDPNSEX(hoseno));
		if (value & 0x0E000000) {
			printf("%s: Hose %d IDPNSE has %x\n", self->dv_xname,
			    hoseno, value);
			continue;
		}
		if ((value & 0x1) != 0x0) {
			printf("%s: Hose %d has a Bad Cable (0x%x)\n",
			    self->dv_xname, hoseno, value);
			continue;
		}
		if ((value & 0x6) != 0x6) {
			if (value)
				printf("%s: Hose %d is missing PWROK (0x%x)\n",
				    self->dv_xname, hoseno, value);
			continue;
		}
		ka.ka_name = "dwlpx";
		ka.ka_node = sc->sc_node;
		ka.ka_dtype = sc->sc_dtype;
		ka.ka_hosenum = hoseno;
		config_found(self, &ka, kftprint);
	}
}
