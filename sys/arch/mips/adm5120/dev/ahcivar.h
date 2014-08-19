/*	$NetBSD: ahcivar.h,v 1.3.14.2 2014/08/20 00:03:12 tls Exp $	*/

/*-
 * Copyright (c) 2007 Ruslan Ermilov and Vsevolod Lobko.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The names of the authors may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tetsuya Isaki.
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
#ifndef	_AHCIVAR_H
#define	_AHCIVAR_H

/*
 * ADM5120 USB Host Controller
 */

#define MS_TO_TICKS(ms) ((ms) * hz / 1000)
#define delay_ms(X) \
	usb_delay_ms(&sc->sc_bus, (X));

struct ahci_xfer {
	usbd_xfer_handle sx_xfer;
	callout_t sx_callout_t;
};

struct ahci_softc {
	struct usbd_bus		 sc_bus;
	bus_space_tag_t		 sc_st;
	bus_space_handle_t	 sc_ioh;
	bus_dma_tag_t		 sc_dmat;
	void *sc_ih;			/* interrupt cookie */

	kmutex_t		 sc_lock;
	kmutex_t		 sc_intr_lock;

	void				(*sc_enable_power)(void *, int);
	void				(*sc_enable_intr)(void *, int);
	void				*sc_arg;
	int				 sc_powerstat;
#define POWER_ON	(1)
#define POWER_OFF	(0)
#define INTR_ON 	(1)
#define INTR_OFF	(0)

	device_t		 sc_child;

	device_t		 sc_parent;	/* parent device */

	u_int8_t		 sc_addr;	/* device address of root hub */
	u_int8_t		 sc_conf;
	SIMPLEQ_HEAD(, usbd_xfer) sc_free_xfers;

	/* Information for the root hub interrupt pipe */
	int			 sc_interval;
	usbd_xfer_handle	 sc_intr_xfer;
	callout_t		 sc_poll_handle;

	int				 sc_flags;
#define AHCDF_RESET	(0x01)
#define AHCDF_INSERT	(0x02)

	/* Root HUB specific members */
	int				sc_fullspeed;
	int				sc_connect;	/* XXX */
	int				sc_change;
};

int  ahci_intr(void *);
#endif	/* _AHCIVAR_H */
