/*	$NetBSD: com_mca.c,v 1.1.2.3 2001/04/21 17:48:50 bouyer Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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

/*-
 * Copyright (c) 1991 The Regents of the University of California.
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
 *	@(#)com.c	7.5 (Berkeley) 5/16/91
 */

/*
 * This driver attaches serial port boards and internal modems.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/device.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <dev/mca/mcavar.h>
#include <dev/mca/mcadevs.h>

struct com_mca_softc {
	struct	com_softc sc_com;	/* real "com" softc */

	/* MCA-specific goo. */
	void	*sc_ih;			/* interrupt handler */
};

int com_mca_probe __P((struct device *, struct cfdata *, void *));
void com_mca_attach __P((struct device *, struct device *, void *));
void com_mca_cleanup __P((void *));
static int ibm_modem_getcfg __P((struct mca_attach_args *, int *, int *));

struct cfattach com_mca_ca = {
	sizeof(struct com_mca_softc), com_mca_probe, com_mca_attach
};

static const struct com_mca_product {
	u_int32_t	cp_prodid;	/* MCA product ID */
	const char	*cp_name;	/* device name */
	int (*cp_getcfg) __P((struct mca_attach_args *, int *iobase, int *irq));
					/* get device i/o base and irq */
} com_mca_products[] = {
	{ MCA_PRODUCT_IBM_MOD,	"IBM Internal Modem",	ibm_modem_getcfg },
	{ 0,			NULL,			NULL },
};

static const struct com_mca_product *com_mca_lookup __P((int));

static const struct com_mca_product *
com_mca_lookup(ma_id)
	int ma_id;
{
	const struct com_mca_product *cpp;

	for (cpp = com_mca_products; cpp->cp_name != NULL; cpp++)
		if (cpp->cp_prodid == ma_id)
			return (cpp);

	return (NULL);
}

int
com_mca_probe(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct mca_attach_args *ma = aux;

	if (com_mca_lookup(ma->ma_id))
		return (1);

	return (0);
}

void
com_mca_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct com_mca_softc *isc = (void *)self;
	struct com_softc *sc = &isc->sc_com;
	int iobase, irq;
	struct mca_attach_args *ma = aux;
	const struct com_mca_product *cpp;

	cpp = com_mca_lookup(ma->ma_id);

	/* get iobase and irq */
	if ((*cpp->cp_getcfg)(ma, &iobase, &irq)) {
		printf(": com_mca_attach: could not get config\n");
		return;
	}	

	printf(" slot %d i/o %#x-%#x irq %d: %s\n", ma->ma_slot + 1,
		iobase, iobase + COM_NPORTS - 1,
		irq, cpp->cp_name);

	printf("%s", sc->sc_dev.dv_xname);

	if (bus_space_map(ma->ma_iot, iobase, COM_NPORTS, 0, &sc->sc_ioh)) {
		printf(": can't map i/o space\n");
		return;
	}

	sc->sc_frequency = COM_FREQ;
	sc->sc_iot = ma->ma_iot;
	sc->sc_iobase = iobase;

	com_attach_subr(sc);

	isc->sc_ih = mca_intr_establish(ma->ma_mc, irq, IPL_SERIAL,
			comintr, sc);
	if (isc->sc_ih == NULL) {
                printf("%s: couldn't establish interrupt handler\n",
                    sc->sc_dev.dv_xname);
                return;
        }

	/*
	 * Shutdown hook for buggy BIOSs that don't recognize the UART
	 * without a disabled FIFO.
	 * XXX is this necessary on MCA ? --- jdolecek
	 */
	if (shutdownhook_establish(com_mca_cleanup, sc) == NULL)
		panic("com_mca_attach: could not establish shutdown hook");
}

void
com_mca_cleanup(arg)
	void *arg;
{
	struct com_softc *sc = arg;

	if (ISSET(sc->sc_hwflags, COM_HW_FIFO))
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, com_fifo, 0);
}

/* map serial_X to iobase and irq */
static const struct {
	int iobase;
	int irq;
} MCA_SERIAL[] = {
	{ 0x03f8,	4 },	/* SERIAL_1 */
	{ 0x02f8,	3 },	/* SERIAL_2 */
	{ 0x3220,	3 },	/* SERIAL_3 */
	{ 0x3228,	3 },	/* SERIAL_4 */
	{ 0x4220,	3 },	/* SERIAL_5 */
	{ 0x4228,	3 },	/* SERIAL_6 */
	{ 0x5220,	3 },	/* SERIAL_7 */
	{ 0x5228,	3 },	/* SERIAL_8 */
};

/*
 * Get config for IBM Internal Modem (ID 0xEDFF). This beast doesn't
 * seem to support even AT commands, it's good as example for adding
 * other stuff though.
 */
static int
ibm_modem_getcfg(ma, iobasep, irqp)
	struct mca_attach_args *ma;
	int *iobasep, *irqp;
{
	int pos2;
	int snum;

	pos2 = mca_conf_read(ma->ma_mc, ma->ma_slot, 2);

	/*
	 * POS register 2: (adf pos0)
	 * 7 6 5 4 3 2 1 0
	 *         \__/  \__ enable: 0=adapter disabled, 1=adapter enabled
	 *            \_____ Serial Configuration: XX=SERIAL_XX 
	 */ 
	
	snum = (pos2 & 0x0e) >> 1;

	*iobasep = MCA_SERIAL[snum].iobase;
	*irqp = MCA_SERIAL[snum].irq;

	return (0);
}
