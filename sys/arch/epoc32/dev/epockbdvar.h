/*	$NetBSD: epockbdvar.h,v 1.1 2013/04/28 12:11:25 kiyohara Exp $	*/
/*
 * Copyright (c) 2013 KIYOHARA Takashi
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

struct epockbd_softc {
	device_t sc_dev;
	device_t sc_wskbddev;

	int sc_kbd_ncolumn;
	int sc_kbd_nrow;
	bus_space_tag_t sc_scant;
	bus_space_handle_t sc_scanh;
	bus_space_tag_t sc_datat;
	bus_space_handle_t sc_datah;

	callout_t sc_c;

	uint8_t sc_state[8]; 

	int sc_flags;
#define EPOCKBD_FLAG_ENABLE	(1 << 0)
#define EPOCKBD_FLAG_RAW	(1 << 1)
};

void epockbd_attach(struct epockbd_softc *);

void epockbd_cnattach(void);
