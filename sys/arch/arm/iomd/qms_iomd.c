/*	$NetBSD: qms_iomd.c,v 1.8 2003/07/15 00:24:46 lukem Exp $	*/

/*
 * Copyright (c) Scott Stevens 1995 All rights reserved
 * Copyright (c) Melvin Tang-Richardson 1995 All rights reserved
 * Copyright (c) Mark Brinicombe 1995 All rights reserved
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
 *	This product includes software developed by the RiscBSD team.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

/*
 * Quadrature mouse driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: qms_iomd.c,v 1.8 2003/07/15 00:24:46 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/select.h>
#include <sys/tty.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <arm/iomd/iomdvar.h>
#include <arm/iomd/qmsvar.h>

extern int iomd_found;

#define TIMER1_COUNT 40000		/* 50Hz */

static int  qms_iomd_probe     __P((struct device *, struct cfdata *, void *));
static void qms_iomd_attach    __P((struct device *, struct device *, void *));
static void qms_iomd_intenable __P((struct qms_softc *sc, int enable));

CFATTACH_DECL(qms_iomd, sizeof(struct qms_softc),
    qms_iomd_probe, qms_iomd_attach, NULL, NULL);

static int
qms_iomd_probe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct qms_attach_args *qa = aux;

	if (strcmp(qa->qa_name, "qms") == 0)
		return(1);

	return(0);
}


static void
qms_iomd_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct qms_softc *sc = (void *)self;
	struct qms_attach_args *qa = aux;

	sc->sc_iot = qa->qa_iot;
	sc->sc_ioh = qa->qa_ioh;
	sc->sc_butioh = qa->qa_ioh_but;
	sc->sc_irqnum = qa->qa_irq;

	sc->sc_intenable = qms_iomd_intenable;

/*	if (sc->sc_irqnum == IRQ_TIMER1) {
		WriteByte(IOMD_T1LOW, (TIMER1_COUNT >> 0) & 0xff);
		WriteByte(IOMD_T1HIGH, (TIMER1_COUNT >> 8) & 0xff);
		WriteByte(IOMD_T1GO, 0);
	}
*/
	qmsattach(sc);
	printf("\n");
}

static void
qms_iomd_intenable(sc, enable)
	struct qms_softc *sc;
	int enable;
{
	if (enable) {
		sc->sc_ih = intr_claim(sc->sc_irqnum, IPL_TTY, "qms", qmsintr, sc);
		if (!sc->sc_ih)
			panic("%s: Cannot claim interrupt", sc->sc_device.dv_xname);
	} else {
		if (intr_release(sc->sc_ih) != 0)
			panic("%s: Cannot release IRQ", sc->sc_device.dv_xname);
	}
}

#ifdef DIAGNOSTIC
#include <arm/iomd/iomdreg.h>
#include <arm/arm32/katelib.h>
void
qms_console_freeze()
{
	if (!iomd_found) return;

	/* Middle mouse button freezes the display while active */
	while ((ReadByte(IO_MOUSE_BUTTONS) & MOUSE_BUTTON_MIDDLE) == 0);

	/* Left mouse button slows down the display speed */

	if ((ReadByte(IO_MOUSE_BUTTONS) & MOUSE_BUTTON_LEFT) == 0)
		delay(5000);
}
#endif /* DIAGNOSTIC */

/* End of qms_iomd.c */
