/*	$NetBSD: zs_pcctwo.c,v 1.5.8.1 2002/05/19 07:56:36 gehenna Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross, Jason R. Thorpe and Steve C. Woodford.
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
 * Zilog Z8530 Dual UART driver (machine-dependent part)
 *
 * Runs two serial lines per chip using slave drivers.
 * Plain tty/async lines use the zs_async slave.
 *
 * Modified for NetBSD/mvme68k by Jason R. Thorpe <thorpej@NetBSD.ORG>
 *
 * Modified to attach to the PCCchip2/MCchip backend by Steve Woodford.
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
#include <machine/bus.h>

#include <dev/mvme/pcctworeg.h>
#include <dev/mvme/pcctwovar.h>

#include <mvme68k/dev/mainbus.h>
#include <mvme68k/dev/zsvar.h>


/* Definition of the driver for autoconfig. */
static int	zsc_pcctwo_match(struct device *, struct cfdata *, void *);
static void	zsc_pcctwo_attach(struct device *, struct device *, void *);

struct cfattach zsc_pcctwo_ca = {
	sizeof(struct zsc_softc), zsc_pcctwo_match, zsc_pcctwo_attach
};

extern struct cfdriver zsc_cd;

cons_decl(zsc_pcctwo);


/*
 * Is the zs chip present?
 */
static int
zsc_pcctwo_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct pcctwo_attach_args *pa = aux;

	if (strcmp(pa->pa_name, zsc_cd.cd_name) ||
	    (machineid != MVME_162 && machineid != MVME_172))
		return (0);

	pa->pa_ipl = cf->pcctwocf_ipl;
	if (pa->pa_ipl == -1)
		pa->pa_ipl = ZSHARD_PRI;
	return (1);
}

/*
 * Attach a found zs.
 */
static void
zsc_pcctwo_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct zsc_softc *zsc = (void *) self;
	struct pcctwo_attach_args *pa = aux;
	struct zsdevice zs;
	bus_space_handle_t bush;
	int zs_level;
	static int vector = MCCHIPV_ZS0;

	/* Map the device's registers */
	bus_space_map(pa->pa_bust, pa->pa_offset, 8, 0, &bush);

	zs_level = pa->pa_ipl;

	/* XXX: This is a gross hack. I need to bus-space zs.c ... */
	zs.zs_chan_b.zc_csr = (volatile u_char *) bush + 1;
	zs.zs_chan_b.zc_data = (volatile u_char *) bush + 3;
	zs.zs_chan_a.zc_csr = (volatile u_char *) bush + 5;
	zs.zs_chan_a.zc_data = (volatile u_char *) bush + 7;

	/* Do common parts of SCC configuration. */
	zs_config(zsc, &zs, vector + PCCTWO_VECBASE, PCLK_162);

	evcnt_attach_dynamic(&zsc->zsc_evcnt, EVCNT_TYPE_INTR,
	    pcctwointr_evcnt(zs_level), "rs232", zsc->zsc_dev.dv_xname);

	/*
	 * Now safe to install interrupt handlers.
	 */
	pcctwointr_establish(vector++, zshard_unshared, zs_level, zsc, NULL);

	/*
	 * Set master interrupt enable.
	 */
	zs_write_reg(zsc->zsc_cs[0], 9, zs_init_reg[9]);
}

/****************************************************************
 * Console support functions (MVME PCC specific!)
 ****************************************************************/

/*
 * Check for SCC console.  The MVME-1x2 always uses unit 0 chan 0.
 */
void
zsc_pcctwocnprobe(cp)
	struct consdev *cp;
{
	extern const struct cdevsw zstty_cdevsw;

	if (machineid != MVME_162 && machineid != MVME_172) {
		cp->cn_pri = CN_DEAD;
		return;
	}

	/* Initialize required fields. */
	cp->cn_dev = makedev(cdevsw_lookup_major(&zstty_cdevsw), 0);
	cp->cn_pri = CN_NORMAL;
}

void
zsc_pcctwocninit(cp)
	struct consdev *cp;
{
	bus_space_handle_t bush;
	struct zsdevice zs;

	bus_space_map(&_mainbus_space_tag,
	    intiobase_phys + MAINBUS_PCCTWO_OFFSET + MCCHIP_ZS0_OFF, 8, 0,&bush);

	/* XXX: This is a gross hack. I need to bus-space zs.c ... */
	zs.zs_chan_b.zc_csr = (volatile u_char *) bush + 1;
	zs.zs_chan_b.zc_data = (volatile u_char *) bush + 3;
	zs.zs_chan_a.zc_csr = (volatile u_char *) bush + 5;
	zs.zs_chan_a.zc_data = (volatile u_char *) bush + 7;

	/* Do common parts of console init. */
	zs_cnconfig(0, 0, &zs, PCLK_162);
}
