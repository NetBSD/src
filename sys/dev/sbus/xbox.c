/*	$NetBSD: xbox.c,v 1.4.6.1 2001/10/01 12:46:20 fvdl Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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

/*
 * Sbus expansion box.
 */

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <dev/sbus/sbusvar.h>
#include <dev/sbus/xboxvar.h>
#include <machine/autoconf.h>

/*
 * Xbox registers definitions.
 *
 * The xbox device can operate in two mode: "opaque" and "transparent".
 * In opaque mode, all accesses to the xbox address space are directed
 * to the xbox itself. In transparent mode, all accesses are mapped to
 * the SBus cards in the expansion box.
 *
 * To access the xbox registers in transparent mode, you must write
 * to the "write0" register. The layout of this register appears to
 * be as follows:
 *
 *	bit 31-24: xbox key (identifies device when you cascade them)
 *	bit 23-12: offset of register to access
 *	bit 11-0:  value to write
 *
 * For instance, to switch to opaque mode:
 *	(*sc->write0_reg) = (sc->sc_key << 24) | XAC_CTL1_OFFSET;
 *
 * (note we're not currently using any of this)
 */
#define WRITE0_OFFSET		0

#define XAC_ERR_OFFSET		0x2000
#define XAC_CTL0_OFFSET		0x10000
#define XAC_CTL1_OFFSET		0x11000
#define XAC_ELUA_OFFSET		0x12000
#define XAC_ELLA_OFFSET		0x13000
#define XAC_ELE_OFFSET		0x14000

#define XBC_ERR_OFFSET		0x42000
#define XBC_CTL0_OFFSET		0x50000
#define XBC_CTL1_OFFSET		0x51000
#define XBC_ELUA_OFFSET		0x52000
#define XBC_ELLA_OFFSET		0x53000
#define XBC_ELE_OFFSET		0x54000

#define XBOX_NREG		13

struct xbox_softc {
	struct device	sc_dev;		/* base device */
	int		sc_key;		/* this xbox's unique key */
};

/* autoconfiguration driver */
int	xbox_match __P((struct device *, struct cfdata *, void *));
void	xbox_attach __P((struct device *, struct device *, void *));
int	xbox_print __P(( void *, const char *));

struct cfattach xbox_ca = {
	sizeof(struct xbox_softc), xbox_match, xbox_attach
};

int
xbox_print(args, busname)
	void *args;
	const char *busname;
{
	struct xbox_attach_args *xa = args;

	if (busname)
		printf("%s at %s", xa->xa_name, busname);
	return (UNCONF);
}

int
xbox_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct sbus_attach_args *sa = aux;

	return (strcmp("SUNW,xbox", sa->sa_name) == 0);
}

/*
 * Attach an Xbox.
 */
void
xbox_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct xbox_softc *sc = (struct xbox_softc *)self;
	struct sbus_attach_args *sa = aux;
	int node = sa->sa_node;
	struct xbox_attach_args xa;
	char *cp;

	sc->sc_key = PROM_getpropint(node, "write0-key", -1);

	cp = PROM_getpropstring(node, "model");
	printf(": model %s", cp);

	cp = PROM_getpropstring(node, "child-present");
	if (strcmp(cp, "true") != 0) {
		printf(": no sbus devices\n");
		return;
	}

	printf("\n");

	/*
	 * Now pretend to be another Sbus.
	 */
	bzero(&xa, sizeof xa);
	xa.xa_name = "sbus";
	xa.xa_node = node;
	xa.xa_bustag = sa->sa_bustag;
	xa.xa_dmatag = sa->sa_dmatag;

	(void) config_found(&sc->sc_dev, (void *)&xa, xbox_print);
}
