/*	$NetBSD: zs_pcc.c,v 1.7.8.2 2000/12/08 09:28:34 bouyer Exp $	*/

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

#include <mvme68k/dev/mainbus.h>
#include <mvme68k/dev/pccreg.h>
#include <mvme68k/dev/pccvar.h>
#include <mvme68k/dev/zsvar.h>


/* Definition of the driver for autoconfig. */
static int	zsc_pcc_match  __P((struct device *, struct cfdata *, void *));
static void	zsc_pcc_attach __P((struct device *, struct device *, void *));

struct cfattach zsc_pcc_ca = {
	sizeof(struct zsc_softc), zsc_pcc_match, zsc_pcc_attach
};

extern struct cfdriver zsc_cd;

cons_decl(zsc_pcc);


/*
 * Is the zs chip present?
 */
static int
zsc_pcc_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct pcc_attach_args *pa = aux;

	if (strcmp(pa->pa_name, zsc_cd.cd_name))
		return (0);

	pa->pa_ipl = cf->pcccf_ipl;
	if (pa->pa_ipl == -1)
		pa->pa_ipl = ZSHARD_PRI;
	return (1);
}

/*
 * Attach a found zs.
 *
 * Match slave number to zs unit number, so that misconfiguration will
 * not set up the keyboard as ttya, etc.
 */
static void
zsc_pcc_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct zsc_softc *zsc = (void *) self;
	struct pcc_attach_args *pa = aux;
	struct zsdevice zs;
	bus_space_handle_t bush;
	int zs_level, ir;
	static int didintr;

	/* Map the device's registers */
	bus_space_map(pa->pa_bust, pa->pa_offset, 4, 0, &bush);

	zs_level = pa->pa_ipl;

	/* XXX: This is a gross hack. I need to bus-space zs.c ... */
	zs.zs_chan_b.zc_csr = (volatile u_char *) bush;
	zs.zs_chan_b.zc_data = (volatile u_char *) bush + 1;
	zs.zs_chan_a.zc_csr = (volatile u_char *) bush + 2;
	zs.zs_chan_a.zc_data = (volatile u_char *) bush + 3;

	/*
	 * Do common parts of SCC configuration.
	 * Note that the vector is not actually used by the ZS chip on
	 * MVME-147. We set up the PCC so that it provides the vector.
	 * This is just here so the real vector is printed at config time.
	 */
	zs_config(zsc, &zs, PCC_VECBASE + PCCV_ZS, PCLK_147);

	/*
	 * Now safe to install interrupt handlers.  Note the arguments
	 * to the interrupt handlers aren't used.  Note, we only do this
	 * once since both SCCs interrupt at the same level and vector.
	 */
	if (didintr == 0) {
		didintr = 1;
		pccintr_establish(PCCV_ZS, zshard_shared, zs_level, zsc);
	}

	/* Sanity check the interrupt levels. */
	ir = pcc_reg_read(sys_pcc, PCCREG_SERIAL_INTR_CTRL);
	if (((ir & PCC_IMASK) != 0) &&
	    ((ir & PCC_IMASK) != zs_level))
		panic("zs_pcc_attach: zs configured at different IPLs");

	/*
	 * Set master interrupt enable. Vector is supplied by the PCC.
	 */
	pcc_reg_write(sys_pcc, PCCREG_SERIAL_INTR_CTRL,
	    zs_level | PCC_IENABLE | PCC_ZSEXTERN);
	zs_write_reg(zsc->zsc_cs[0], 2, PCC_VECBASE + PCCV_ZS);
	zs_write_reg(zsc->zsc_cs[0], 9, zs_init_reg[9]);
}

/****************************************************************
 * Console support functions (MVME PCC specific!)
 ****************************************************************/

/*
 * Check for SCC console.  The MVME-147 always uses unit 0 chan 0.
 */
void
zsc_pcccnprobe(cp)
	struct consdev *cp;
{
	if (machineid != MVME_147) {
		cp->cn_pri = CN_DEAD;
		return;
	}

	/* Initialize required fields. */
	cp->cn_dev = makedev(zs_major, 0);
	cp->cn_pri = CN_NORMAL;
}

void
zsc_pcccninit(cp)
	struct consdev *cp;
{
	bus_space_handle_t bush;
	struct zsdevice zs;

	bus_space_map(&_mainbus_space_tag,
	    intiobase_phys + MAINBUS_PCC_OFFSET + PCC_ZS0_OFF, 4, 0, &bush);

	/* XXX: This is a gross hack. I need to bus-space zs.c ... */
	zs.zs_chan_b.zc_csr = (volatile u_char *) bush;
	zs.zs_chan_b.zc_data = (volatile u_char *) bush + 1;
	zs.zs_chan_a.zc_csr = (volatile u_char *) bush + 2;
	zs.zs_chan_a.zc_data = (volatile u_char *) bush + 3;

	/* Do common parts of console init. */
	zs_cnconfig(0, 0, &zs, PCLK_147);
}
