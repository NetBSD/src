/*	$NetBSD: rpckbdvar.h,v 1.1 2001/10/05 22:27:42 reinoud Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 * rpckbdvar.h
 *
 * Created      : 04/03/01
 */

/*
 * softc structure for the rpckbd device
 */

struct rpckbd_softc {
	struct device	sc_device;	/* device node */
	void		*sc_ih;		/* interrupt pointer */

	bus_space_tag_t	   sc_iot;	/* bus tag */
	bus_space_handle_t sc_ioh;	/* bus handle */

	vaddr_t	data_port;
	vaddr_t	cmd_port;

	int t_lastchar;
	int t_flags;
	int t_resend;
	int t_ack;

	int t_isconsole;

	int sc_enabled;
	int sc_ledstate;

	struct device *sc_wskbddev;

	int rawkbd;
};


/* function prototypes used by attach routines */

extern int rpckbd_reset		__P((struct rpckbd_softc *sc));
extern int rpckbd_intr		__P((void *arg));
extern int rpckbd_init		__P((struct device *self, int isconsole, vaddr_t, vaddr_t));
extern int rpckbd_cnattach	__P((struct device *self));

/* End of rpckbdvar.h */

