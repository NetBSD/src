/*	$NetBSD: mainbus.c,v 1.7.14.1 2001/01/18 09:22:54 bouyer Exp $	*/

/*
 * Copyright (c) 1997 Matthias Pfaller.
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
 *	This product includes software developed by Matthias Pfaller.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/conf.h>
#include <machine/icu.h>

static int	mbprobe __P((struct device *, struct cfdata *, void *));
static void	mbattach __P((struct device *, struct device *, void *));
static int	mbsearch __P((struct device *, struct cfdata *, void *));
static int	mbprint __P((void *, const char *));

struct cfattach mainbus_ca = {
	sizeof(struct device), mbprobe, mbattach
};

static int
mbprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return(strcmp(cf->cf_driver->cd_name, "mainbus") == 0);
}

static void
mbattach(parent, self, aux)
	struct device *parent, *self;
 	void *aux;
{
	int clk, net;
	static u_char icu_table[] = {
		/*
		 * The ICU is initialized by the Monitor as follows:
		 *	CCTL=0x15 MCTL=0 LCSV=13 IPS=0 PDIR=0xfe PDAT=0xfe
		 *
		 * G0 is used SWAP RAM/ROM, G1-G6 for interrupts and
		 * G7 to select between the ncr or the aic SCSI controller.
		 */
		IPS,	0x7e,	/* 0=i/o, 1=int_req */
		PDIR,	0x7e,	/* 1=in, 0=out */
		PDAT,	0xfe,	/* keep ROM at high mem */
		0xff
	};

	printf("\n");
	icu_init(icu_table);

	/* Set up the interrupt system. */
	intr_init();

	/* Allocate softclock at IPL_NET as splnet() has to block softclock. */
	clk = intr_establish(SOFTINT, softclock, NULL,
				"softclock", IPL_NET, IPL_NET, 0);
	net = intr_establish(SOFTINT, softnet, NULL,
				"softnet", IPL_NET, IPL_NET, 0);
	if (clk != SIR_CLOCK || net != SIR_NET)
		panic("Wrong clock or net softint allocated");

	config_search(mbsearch, self, NULL);
}

static int
mbprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	char *delim;
	struct confargs *ca = aux;

	if (ca->ca_addr != -1) {
		printf(" addr 0x%x", ca->ca_addr);
		delim = ",";
	} else
		delim = "";

	if (ca->ca_irq != -1) {
		printf("%s irq %d", delim, ca->ca_irq & 15);
		if (ca->ca_irq & 0xf0)
			printf(", %d", ca->ca_irq >> 4);
	}

	return(UNCONF);
}

static int
mbsearch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct confargs ca;

	ca.ca_addr  = cf->cf_addr;
	ca.ca_irq   = cf->cf_irq;
	ca.ca_flags = cf->cf_flags;

	while ((*cf->cf_attach->ca_match)(parent, cf, &ca) > 0) {
		config_attach(parent, cf, &ca, mbprint);
		if (cf->cf_fstate != FSTATE_STAR)
			break;
	}
	return (0);
}

void
icu_init(p)
	u_char *p;
{
	di();
	while (*p != 0xff) {
		ICUB(p[0]) = p[1];
		p += 2;
	}
	ei();
}
