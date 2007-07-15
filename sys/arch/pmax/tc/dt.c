/*	$NetBSD: dt.c,v 1.7.2.1 2007/07/15 22:20:26 ad Exp $	*/

/*-
 * Copyright (c) 2002, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell and Rick Macklem.
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
 *	@(#)dtop.c	8.2 (Berkeley) 11/30/93
 */

/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */
/*
 * 	Author: Alessandro Forin, Carnegie Mellon University
 *
 *	Hardware-level operations for the Desktop serial line
 *	bus (i2c aka ACCESS).
 */
/************************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

********************************************************/

/*
 * ACCESS.bus device support for the Personal DECstation.  This code handles
 * only the keyboard and mouse, and will likely not work if other ACCESS.bus
 * devices are physically attached to the system.
 *
 * Since we do not know how to drive the hardware (the only reference being
 * Mach), we can't identify which devices are connected to the system by
 * sending idenfication requests.  With only a mouse and keyboard attached
 * to the system, we do know which two slave addresses will be in use. 
 * However, we don't know which is the mouse, and which is the keyboard. 
 * So, we resort to inspecting device reports and making an educated guess
 * as to which is which.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dt.c,v 1.7.2.1 2007/07/15 22:20:26 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/intr.h>

#include <dev/dec/lk201.h>

#include <dev/tc/tcvar.h>
#include <dev/tc/ioasicreg.h>
#include <dev/tc/ioasicvar.h>

#include <pmax/pmax/maxine.h>

#include <pmax/tc/dtreg.h>
#include <pmax/tc/dtvar.h>

#define	DT_BUF_CNT		16
#define	DT_ESC_CHAR		0xf8
#define	DT_XMT_OK		0xfb
#define	DT_MAX_POLL		0x70000		/* about half a sec */

#define	DT_GET_BYTE(data)	(((*(data)) >> 8) & 0xff)
#define	DT_PUT_BYTE(data,c)	{ *(data) = (c) << 8; wbflush(); }

#define	DT_RX_AVAIL(poll)	((*(poll) & 1) != 0)
#define	DT_TX_AVAIL(poll)	((*(poll) & 2) != 0)

int	dt_match(struct device *, struct cfdata *, void *);
void	dt_attach(struct device *, struct device *, void *);
int	dt_intr(void *);
int	dt_null_handler(struct device *, struct dt_msg *, int);
int	dt_print(void *, const char *);
void	dt_strvis(uint8_t *, char *, int);
void	dt_dispatch(void *);

int	dt_kbd_addr = DT_ADDR_KBD;
struct	dt_device dt_kbd_dv;
int	dt_ms_addr = DT_ADDR_MOUSE;
struct	dt_device dt_ms_dv;
struct	dt_state dt_state;

CFATTACH_DECL(dt, sizeof(struct dt_softc),
    dt_match, dt_attach, NULL, NULL);

int
dt_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct ioasicdev_attach_args *d;

	d = aux;

	if (strcmp(d->iada_modname, "dtop") != 0)
		return (0);

	if (badaddr((void *)(d->iada_addr), 2))
		return (0);

	return (1);
}

void
dt_attach(struct device *parent, struct device *self, void *aux)
{
	struct ioasicdev_attach_args *d;
	struct dt_attach_args dta;
	struct dt_softc *sc;
	struct dt_msg *msg;
	int i;

	d = aux;
	sc = (struct dt_softc*)self;

	dt_cninit();

	msg = malloc(sizeof(*msg) * DT_BUF_CNT, M_DEVBUF, M_NOWAIT);
	if (msg == NULL) {
		printf("%s: memory exhausted\n", sc->sc_dv.dv_xname);
		return;
	}

	sc->sc_sih = softint_establish(SOFTINT_SERIAL, dt_dispatch, sc);
	if (sc->sc_sih == NULL) {
		printf("%s: memory exhausted\n", sc->sc_dv.dv_xname);
		free(msg, M_DEVBUF);
	}

	SIMPLEQ_INIT(&sc->sc_queue);
	SLIST_INIT(&sc->sc_free);
	for (i = 0; i < DT_BUF_CNT; i++, msg++)
		SLIST_INSERT_HEAD(&sc->sc_free, msg, chain.slist);

	ioasic_intr_establish(parent, d->iada_cookie, TC_IPL_TTY, dt_intr, sc);
	printf("\n");

	dta.dta_addr = DT_ADDR_KBD;
	config_found(self, &dta, dt_print);
	dta.dta_addr = DT_ADDR_MOUSE;
	config_found(self, &dta, dt_print);
}

void
dt_cninit(void)
{

	dt_state.ds_poll = (volatile u_int *)
	    MIPS_PHYS_TO_KSEG1(XINE_REG_INTR);
	dt_state.ds_data = (volatile u_int *)
	    MIPS_PHYS_TO_KSEG1(XINE_PHYS_TC_3_START + 0x280000);
}

int
dt_print(void *aux, const char *pnp)
{

	return (QUIET);
}

int
dt_establish_handler(struct dt_softc *sc, struct dt_device *dtdv,
    struct device *dv, void (*hdlr)(void *, struct dt_msg *))
{

	dtdv->dtdv_dv = dv;
	dtdv->dtdv_handler = hdlr;
	return (0);
}

int
dt_intr(void *cookie)
{
	struct dt_softc *sc;
	struct dt_msg *msg, *pend;

	sc = cookie;

	switch (dt_msg_get(&sc->sc_msg, 1)) {
	case DT_GET_ERROR:
		/*
		 * Ugh! The most common occurrence of a data overrun is upon
		 * a key press and the result is a software generated "stuck
		 * key".  All I can think to do is fake an "all keys up"
		 * whenever a data overrun occurs.
		 */
		sc->sc_msg.src = dt_kbd_addr;
		sc->sc_msg.ctl = DT_CTL(1, 0, 0);
		sc->sc_msg.body[0] = DT_KBD_EMPTY;
#ifdef DIAGNOSTIC
		printf("%s: data overrun or stray interrupt\n",
		    sc->sc_dv.dv_xname);
#endif
		break;

	case DT_GET_DONE:
		break;

	case DT_GET_NOTYET:
		return (1);
	}

	if ((msg = SLIST_FIRST(&sc->sc_free)) == NULL) {
		printf("%s: input overflow\n", sc->sc_dv.dv_xname);
		return (1);
	}
	SLIST_REMOVE_HEAD(&sc->sc_free, chain.slist);
	memcpy(msg, &sc->sc_msg, sizeof(*msg));

	pend = SIMPLEQ_FIRST(&sc->sc_queue);
	SIMPLEQ_INSERT_TAIL(&sc->sc_queue, msg, chain.simpleq);
	if (pend == NULL)
		softint_schedule(sc->sc_sih);

	return (1);
}

void
dt_dispatch(void *cookie)
{
	struct dt_softc *sc;
	struct dt_msg *msg;
	int s;
	struct dt_device *dtdv;

	sc = cookie;
	msg = NULL;

	for (;;) {
		s = spltty();
		if (msg != NULL)
			SLIST_INSERT_HEAD(&sc->sc_free, msg, chain.slist);
		msg = SIMPLEQ_FIRST(&sc->sc_queue);
		if (msg != NULL)
			SIMPLEQ_REMOVE_HEAD(&sc->sc_queue, chain.simpleq);
		splx(s);
		if (msg == NULL)
			break;

		if (msg->src != DT_ADDR_MOUSE && msg->src != DT_ADDR_KBD) {
			printf("%s: message from unknown dev 0x%x\n",
			    sc->sc_dv.dv_xname, sc->sc_msg.src);
			dt_msg_dump(msg);
			continue;
		}
		if (DT_CTL_P(msg->ctl) != 0) {
			printf("%s: received control message\n",
			    sc->sc_dv.dv_xname);
			dt_msg_dump(msg);
			continue;
		}

		/*
		 * 1. Mouse should have no more than eight buttons, so first
		 *    8 bits of body will be zero.
		 * 2. Mouse should always send full locator report.
		 *    Note:  my mouse does not send 'z' data, so the size
		 *    did not match the size of struct dt_locator_msg - mhitch
		 * 3. Keyboard should never report all-up (0x00) in
		 *    a packet with size > 1.
		 */
		if (DT_CTL_LEN(msg->ctl) >= 6 &&
		    msg->body[0] == 0 && msg->src != dt_ms_addr) {
			dt_kbd_addr = dt_ms_addr;
			dt_ms_addr = msg->src;
		} else if (DT_CTL_LEN(msg->ctl) < 6 && msg->body[0] != 0 &&
		    msg->src != dt_kbd_addr) {
			dt_ms_addr = dt_kbd_addr;
			dt_kbd_addr = msg->src;
		}

		if (msg->src == dt_kbd_addr)
			dtdv = &dt_kbd_dv;
		else
			dtdv = &dt_ms_dv;

		if (dtdv->dtdv_handler != NULL)
			(*dtdv->dtdv_handler)(dtdv->dtdv_dv, msg);
	}
}

int
dt_msg_get(struct dt_msg *msg, int intr)
{
	volatile u_int *poll, *data;
	uint8_t c;
	int max_polls;

	poll = dt_state.ds_poll;
	data = dt_state.ds_data;

	/*
	 * The interface does not hand us the first byte, which is our
	 * address and cannot ever be anything else but 0x50.
	 */
	if (dt_state.ds_state == 0) {
		dt_state.ds_escaped = 0;
		dt_state.ds_ptr = 0;
	}

	for (;;) {
		max_polls = DT_MAX_POLL;

		while (!DT_RX_AVAIL(poll)) {
			if (intr)
				return (DT_GET_NOTYET);
			if (max_polls-- <= 0)
				break;
			DELAY(1);
		}

		if (max_polls <= 0) {
			if (dt_state.ds_state != 0) {
				dt_state.ds_bad_pkts++;
				dt_state.ds_state = 0;
			}
			return (DT_GET_ERROR);
		}

		c = DT_GET_BYTE(data);

		if (dt_state.ds_escaped) {
			switch (c) {
			case 0xe8:
			case 0xe9:
			case 0xea:
			case 0xeb:
				c += 0x10;
				break;
			}
			if (c == 'O') {
				dt_state.ds_bad_pkts++;
				dt_state.ds_state = 0;
				return (DT_GET_ERROR);
			}
			dt_state.ds_escaped = 0;
		} else if (c == DT_ESC_CHAR) {
			dt_state.ds_escaped = 1;
			continue;
		}

		if (dt_state.ds_state == 0) {
			msg->src = c;
			dt_state.ds_state = 1;
		} else if (dt_state.ds_state == 1) {
			msg->ctl = c;
			dt_state.ds_state = 2;
			dt_state.ds_len = DT_CTL_LEN(msg->ctl) + 1;
			if (dt_state.ds_len > sizeof(msg->body))
				printf("dt_msg_get: msg truncated: %d\n",
				    dt_state.ds_len);
		} else /* if (dt_state.ds_state == 2) */ {
			if (dt_state.ds_ptr < sizeof(msg->body))
				msg->body[dt_state.ds_ptr++] = c;
			if (dt_state.ds_ptr >= dt_state.ds_len)
				break;
		}
	}

	msg->dst = DT_ADDR_HOST;
	dt_state.ds_state = 0;
	return (DT_GET_DONE);
}

void
dt_msg_dump(struct dt_msg *msg)
{
	int i, l;

	l = DT_CTL_LEN(msg->ctl);

	printf("hdr: dst=%02x src=%02x p=%02x sub=%02x len=%02x\n",
	   msg->dst, msg->src, DT_CTL_P(msg->ctl), DT_CTL_SUBADDR(msg->ctl),
	   l);

	printf("body: ");	   
	for (i = 0; i < l && i < 20; i++)
		printf("%02x ", msg->body[i]);
	if (i < l) {
		printf("\n");
		for (; i < l; i++)
			printf("%02x ", msg->body[i]);
	}
	printf("\n");
}
