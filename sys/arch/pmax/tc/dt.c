/*	$NetBSD: dt.c,v 1.1.2.2 2002/03/15 16:48:31 ad Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dt.c,v 1.1.2.2 2002/03/15 16:48:31 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <dev/dec/lk201.h>

#include <dev/tc/tcvar.h>
#include <dev/tc/ioasicreg.h>
#include <dev/tc/ioasicvar.h>

#include <machine/conf.h>

#include <pmax/pmax/maxine.h>

#include <pmax/tc/dtreg.h>
#include <pmax/tc/dtvar.h>

#define	DT_BUF_CNT		16
#define	DT_ESC_CHAR		0xf8
#define	DT_MAX_POLL		0x70000		/* about half a sec */

#define	DT_GET_BYTE(data)	(((*(data)) >> 8) & 0xff)
#define	DT_PUT_BYTE(data,c)	{ *(data) = (c) << 8; }

#define	DT_RX_AVAIL(poll)	((*(poll) & 1) != 0)
#define	DT_TX_AVAIL(poll)	((*(poll) & 2) != 0)

int	dt_match(struct device *, struct cfdata *, void *);
void	dt_attach(struct device *, struct device *, void *);
int	dt_intr(void *);
int	dt_null_handler(struct device *, struct dt_msg *, int);
int	dt_print(void *, const char *);
void	dt_strvis(u_char *, u_char *, int);
int	dt_msg_put(struct dt_msg *);

int	dt_kbd_addr = DT_ADDR_KBD;

struct	dt_state dt_state;

struct 	cfattach dt_ca = {
	sizeof(struct dt_softc), dt_match, dt_attach
};

int
dt_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct ioasicdev_attach_args *d;

	d = aux;

	if (strcmp(d->iada_modname, "dtop") != 0)
		return (0);

	if (badaddr((caddr_t)(d->iada_addr), 2))
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

	SLIST_INIT(&sc->sc_free);
	for (i = 0; i < DT_BUF_CNT; i++, msg++)
		SLIST_INSERT_HEAD(&sc->sc_free, msg, chain.slist);

	ioasic_intr_establish(parent, d->iada_cookie, TC_IPL_TTY, dt_intr, sc);
	printf("\n");

	for (i = DT_ADDR_FIRST; i <= DT_ADDR_LAST; i += 2) {
		dta.dta_addr = i;
		config_found(self, &dta, dt_print);
	}
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
dt_establish_handler(struct dt_softc *sc, int devno, struct device *dv,
		     void (*hdlr)(void *))
{
	struct dt_device *dtdv;

	devno = DT_DEVICE_NO(devno);
	if (devno < 0 || devno > DT_MAX_DEVICES)
		return (EINVAL);

	dtdv = &sc->sc_dtdv[devno];
	SIMPLEQ_INIT(&dtdv->dtdv_queue);

	dtdv->dtdv_sih = softintr_establish(IPL_SOFTSERIAL, hdlr, dtdv);
	if (dtdv->dtdv_sih == NULL)
		return (ENOMEM);

	dtdv->dtdv_dv = dv;
	return (0);
}

int
dt_intr(void *cookie)
{
	struct dt_device *dtdv;
	struct dt_softc *sc;
	struct dt_msg *msg, *pend;
	int devno;

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
		sc->sc_msg.code.val.P = 0;
		sc->sc_msg.code.val.sub = 0;
		sc->sc_msg.code.val.len = 1;
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

	devno = DT_DEVICE_NO(sc->sc_msg.src);
	if (devno < 0 || devno > DT_MAX_DEVICES) {
#ifdef DIAGNOSTIC
		printf("%s: received message from unknown device 0x%x\n",
		    sc->sc_dv.dv_xname, msg->src);
#endif
		return (1);
	}

	dtdv = &sc->sc_dtdv[devno];
	if (dtdv->dtdv_sih == NULL) {
#ifdef DIAGNOSTIC
		printf("%s: received message from unknown device 0x%x\n",
		    sc->sc_dv.dv_xname, msg->src);
#endif
		return (1);
	}

	if ((msg = SLIST_FIRST(&sc->sc_free)) == NULL) {
#ifdef DIAGNOSTIC
		printf("%s: input overflow\n", sc->sc_dv.dv_xname);
#endif
		return (1);
	}

	SLIST_REMOVE_HEAD(&sc->sc_free, chain.slist);
	memcpy(msg, &sc->sc_msg, sizeof(*msg));

	pend = SIMPLEQ_FIRST(&dtdv->dtdv_queue);
	SIMPLEQ_INSERT_TAIL(&dtdv->dtdv_queue, msg, chain.simpleq);
	if (pend == NULL)
		softintr_schedule(dtdv->dtdv_sih);

	return (1);
}

int
dt_msg_get(struct dt_msg *msg, int intr)
{
	volatile u_int *poll, *data;
	u_int8_t c;
	int max;

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
		max = DT_MAX_POLL;

		while (!DT_RX_AVAIL(poll)) {
			if (intr)
				return (DT_GET_NOTYET);
			if (max-- <= 0)
				break;
			DELAY(1);
		}

		if (max <= 0) {
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
			msg->code.bits = c;
			dt_state.ds_state = 2;
			dt_state.ds_len = msg->code.val.len + 1;
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

int
dt_msg_put(struct dt_msg *msg)
{
	volatile u_int *poll, *data;
	u_int8_t *p;
	int max, len;

	poll = dt_state.ds_poll;
	data = dt_state.ds_data;

	len = msg->code.val.len + 3;
	p = (u_int8_t *)&msg;

	for (; len != 0; len--, p++) {
		max = DT_MAX_POLL;

		while (!DT_TX_AVAIL(poll)) {
			if (max-- <= 0)
				break;
			DELAY(1);
		}

		if (max <= 0)
			return (-1);

		DT_PUT_BYTE(data, *p);
	}

	return (0);
}

void
dt_strvis(u_char *i, u_char *o, int len)
{

	memcpy(o, i, len);
	o[len] = '\0';
}

int
dt_identify(int addr, struct dt_ident *ident)
{
	struct dt_msg msg;
	u_char buf[16];

	msg.dst = addr;
	msg.src = DT_ADDR_HOST;
	msg.code.val.P = 1;
	msg.code.val.sub = 0;
	msg.code.val.len = 1;
	msg.body[0] = DT_MSG_ID_REQUEST;

	if (dt_msg_put(&msg) != 0) {
		printf("dt_identify: msg_put timed out\n");
		return (-1);
	}

	DELAY(5000);

	for (;;) {
		if (dt_msg_get(&msg, 0) != DT_GET_DONE) {
			printf("dt_identify: msg_get timed out\n");
			return (-1);
		}
		if (msg.src == addr) {
			if (msg.code.val.P != 0)
				break;
			else
				printf("dt_identify: received junk\n");
		} else
			printf("dt_identify: message not for us\n");
	}

	if (msg.code.val.len < sizeof(*ident) - 3) {
		printf("dt_identify: short response\n");
		return (-1);
	}

	if (msg.code.val.len > sizeof(*ident)) {
		printf("dt_identify: long response\n");
		return (-1);
	}

	dt_strvis(ident->vendor, buf, sizeof(ident->vendor));
	printf("dt_identify(%x): vendor <%s>\n", addr, buf);

	dt_strvis(ident->module, buf, sizeof(ident->module));
	printf("dt_identify(%x): module <%s>\n", addr, buf);

	dt_strvis(ident->revision, buf, sizeof(ident->revision));
	printf("dt_identify(%x): revision <%s>\n", addr, buf);

	return (0);
}
