/*	$NetBSD: kbvar.h,v 1.1.128.2 2008/06/02 13:22:28 mjf Exp $	*/

/*-
 * Copyright (c) 2001 Izumi Tsutsui.  All rights reserved.
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

struct console_softc {
	u_long	cs_nintr;
	u_long	cs_nkeyevents;
	int	cs_isconsole;
	int	cs_polling;
	int	cs_key;
	u_int	cs_type;
	int	cs_val;
};

struct kb_softc {
	device_t sc_dev;
	device_t sc_wskbddev;
	struct console_softc *sc_conssc;
	bus_space_tag_t sc_bt;
	bus_space_handle_t sc_bh;
	bus_size_t sc_offset; /* kb data port offset */
};

extern struct wskbd_accessops kb_accessops;
extern struct wskbd_consops kb_consops;
extern struct wskbd_mapdata kb_keymapdata;

void	kb_intr(struct kb_softc *);
int	kb_cnattach(struct console_softc *);
