/*	$NetBSD: zs_pcc.c,v 1.1 2001/05/14 18:23:07 drochner Exp $	*/

/*
 * Copyright (c) 1997, 1999
 *	Matthias Drochner.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/syslog.h>

#include <dev/cons.h>
#include <dev/ic/z8530reg.h>
#include <machine/z8530var.h>

#include <machine/cpu.h>
#include <machine/autoconf.h>

#include <cesfic/cesfic/isr.h>

#include <cesfic/dev/zsvar.h>

extern void sic_enable_int __P((int, int, int, int, int));

static int	zsc_pcc_match  __P((struct device *, struct cfdata *, void *));
static void	zsc_pcc_attach __P((struct device *, struct device *, void *));

static char *zsbase;

struct cfattach zsc_pcc_ca = {
	sizeof(struct zsc_softc), zsc_pcc_match, zsc_pcc_attach
};

static int
zsc_pcc_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return (1);
}

static void
zsc_pcc_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct zsc_softc *zsc = (void *) self;
	static int didintr;

	if (!zsbase)
		mainbus_map(0x58000000, 0x10000, 0, (void **)&zsbase);

	/* Do common parts of SCC configuration. */
	zs_config(zsc, zsbase);

	/*
	 * Now safe to install interrupt handlers.  Note the arguments
	 * to the interrupt handlers aren't used.  Note, we only do this
	 * once since both SCCs interrupt at the same level and vector.
	 */
	if (didintr == 0) {
		didintr = 1;
		(void) isrlink(zshard, zsc, 4, ISRPRI_TTY);
		sic_enable_int(19, 0, 4, 4, 0);
	}

	zs_write_reg(zsc->zsc_cs[0], 2, 0x18 + ZSHARD_PRI);
	zs_write_reg(zsc->zsc_cs[0], 9, ZSWR9_MASTER_IE);
}

void
zs_cnattach(base)
	void *base;
{
	zsbase = base;

	zs_cninit(zsbase);
}
