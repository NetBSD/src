/*	$NetBSD: com_dio.c,v 1.4.16.1 2006/06/18 05:35:00 gdamore Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
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
 * 3. Neither the name of the University nor the names of its contributors
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: com_dio.c,v 1.4.16.1 2006/06/18 05:35:00 gdamore Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/termios.h>
#include <sys/ttydefaults.h>

#include <machine/bus.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <hp300/dev/diovar.h>
#include <hp300/dev/diodevs.h>
#include <hp300/dev/com_dioreg.h>
#include <hp300/dev/com_diovar.h>

struct com_dio_softc {
	struct	com_softc sc_com;	/* real "com" softc */

	/* DIO-specific goo. */
	struct	bus_space_tag sc_tag;	/* device specific bus space tag */
	void	*sc_ih;			/* interrupt handler */
};

static int	com_dio_match(struct device *, struct cfdata *, void *);
static void	com_dio_attach(struct device *, struct device *, void *);

CFATTACH_DECL(com_dio, sizeof(struct com_dio_softc),
    com_dio_match, com_dio_attach, NULL, NULL);

static int com_dio_speed = TTYDEF_SPEED;
static struct bus_space_tag comcntag;

#define CONMODE	((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */

static int
com_dio_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct dio_attach_args *da = aux;

	switch (da->da_id) {
	case DIO_DEVICE_ID_DCA0:
	case DIO_DEVICE_ID_DCA0REM:
	case DIO_DEVICE_ID_DCA1:
	case DIO_DEVICE_ID_DCA1REM:
		return 1;
	}

	return 0;
}

static void
com_dio_attach(struct device *parent, struct device *self, void *aux)
{
	struct com_dio_softc *dsc = (void *)self;
	struct com_softc *sc = &dsc->sc_com;
	bus_space_tag_t iot;
	bus_space_handle_t iohdca, iohcom;
	struct dio_attach_args *da = aux;
	int isconsole;

	isconsole = com_is_console(&comcntag, da->da_addr + DCA_COM_OFFSET,
	    &iohcom);

	if (isconsole) {
		iot = &comcntag;
		bus_space_unmap(iot, iohcom, COM_NPORTS);
	} else {
		iot = &dsc->sc_tag;
		memcpy(iot, da->da_bst, sizeof(struct bus_space_tag));
		dio_set_bus_space_oddbyte(iot);
	}

	if (bus_space_map(iot, da->da_addr, DCA_SIZE, 0, &iohdca) ||
	    bus_space_subregion(iot, iohdca, DCA_COM_OFFSET,
	    COM_NPORTS << 1, &iohcom)) {
		printf(": can't map i/o space\n");
		return;
	}

	if (!isconsole) {
		bus_space_write_1(iot, iohdca, DCA_RESET, 0xff);
		DELAY(1000);
	}

	COM_INIT_REGS(sc->sc_regs, iot, iohcom,
	    da->da_addr + DCA_COM_OFFSET);

	sc->sc_frequency = COM_DIO_FREQ;

	com_attach_subr(sc);

	dsc->sc_ih = dio_intr_establish(comintr, sc, da->da_ipl,
	    ((sc->sc_hwflags & COM_HW_FIFO) != 0) ? IPL_TTY : IPL_TTYNOBUF);

	/* Enable interrupts. */
	bus_space_write_1(iot, iohdca, DCA_IC, IC_IE);
}

int
com_dio_cnattach(bus_space_tag_t bst, bus_addr_t addr, int scode)
{
	bus_space_tag_t iot = &comcntag;
	bus_space_handle_t iohdca;
	uint8_t id;

	memcpy(iot, bst, sizeof(struct bus_space_tag));
	dio_set_bus_space_oddbyte(iot);

	if (bus_space_map(iot, addr, DCA_SIZE, 0, &iohdca))
		return 1;
	bus_space_write_1(iot, iohdca, DCA_RESET, 0xff);
	DELAY(100);
	bus_space_write_1(iot, iohdca, DCA_IC, IC_IE);

	id = bus_space_read_1(iot, iohdca, DCA_ID);
	bus_space_unmap(iot, iohdca, DCA_SIZE);

	switch (id) {
#ifdef CONSCODE
	case DCAID0:
	case DCAID1:
#endif
	case DCAREMID0:
	case DCAREMID1:
		break;
	default:
		return 1;
	}

	comcnattach(iot, addr + DCA_COM_OFFSET, com_dio_speed, COM_DIO_FREQ,
	    COM_TYPE_NORMAL, CONMODE);

	return 0;
}
