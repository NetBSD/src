/* $NetBSD: wsqmsvar.h,v 1.1 2001/10/05 22:27:44 reinoud Exp $ */

/*-
 * Copyright (c) 2001 Reinoud Zandijk
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Reinoud Zandijk
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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
 *
 *
 * Quadratic mouse driver variable for the wscons as used in the IOMD but is in
 * principle more generic.
 *
 * wsqmsvar.h
 */


/* softc structure for the wsqms device */

struct wsqms_softc {
	struct device  sc_device;	/* remember our own device as attached */
	struct device *sc_wsmousedev;	/* remember our wsmouse device... */

	bus_space_tag_t sc_iot;		/* bus tag */
	bus_space_handle_t sc_ioh;	/* bus handle for XY */
	bus_space_handle_t sc_butioh;	/* bus handle for buttons */

	/* interupt handler switch function + goo */
	void (*sc_intenable) __P((struct wsqms_softc *, int));
	void *sc_ih;			/* interrupt pointer */
	int   sc_irqnum; 		/* IRQ number */

#define WSQMS_ENABLED 0x01
	int sc_flags;

	int lastx;
	int lasty;
	int lastb;
};


/* function prototypes */
extern void wsqms_attach	__P((struct wsqms_softc *sc, struct device *));
extern int  wsqms_intr		__P((void *arg));


/* End of wsqmsvar.h */
