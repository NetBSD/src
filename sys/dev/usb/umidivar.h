/*	$NetBSD: umidivar.h,v 1.13.14.1 2010/11/06 08:08:39 uebayasi Exp $	*/
/*
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Takuya SHIOZAKI (tshiozak@NetBSD.org).
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

#define UMIDI_PACKET_SIZE 4

/*
 * hierarchie
 *
 * <-- parent	       child -->
 *
 * umidi(sc) -> endpoint -> jack   <- (dynamically assignable) - mididev
 *	   ^	 |    ^	    |
 *	   +-----+    +-----+
 */

/* midi device */
struct umidi_mididev {
	struct umidi_softc	*sc;
	device_t		mdev;
	/* */
	struct umidi_jack	*in_jack;
	struct umidi_jack	*out_jack;
	char			*label;
	/* */
	int			opened;
	int			flags;
};

/* Jack Information */
struct umidi_jack {
	struct umidi_endpoint	*endpoint;
	/* */
	int			cable_number;
	void			*arg;
	int			binded;
	int			opened;
	unsigned char		*midiman_ppkt;
	union {
		struct {
			void			(*intr)(void *);
		} out;
		struct {
			void			(*intr)(void *, int);
		} in;
	} u;
};

#define UMIDI_MAX_EPJACKS	16
typedef unsigned char (*umidi_packet_bufp)[UMIDI_PACKET_SIZE];
/* endpoint data */
struct umidi_endpoint {
	struct umidi_softc	*sc;
	/* */
	int			addr;
	usbd_pipe_handle	pipe;
	usbd_xfer_handle	xfer;
	umidi_packet_bufp	buffer;
	umidi_packet_bufp	next_slot;
	u_int32_t               buffer_size;
	int			num_scheduled;
	int			num_open;
	int			num_jacks;
	int			soliciting;
	void			*solicit_cookie;
	int			armed;
	struct umidi_jack	*jacks[UMIDI_MAX_EPJACKS];
	u_int16_t		this_schedule; /* see UMIDI_MAX_EPJACKS */
	u_int16_t		next_schedule;
};

/* software context */
struct umidi_softc {
	device_t		sc_dev;
	usbd_device_handle	sc_udev;
	usbd_interface_handle	sc_iface;
	const struct umidi_quirk	*sc_quirk;

	int			sc_dying;

	int			sc_out_num_jacks;
	struct umidi_jack	*sc_out_jacks;
	int			sc_in_num_jacks;
	struct umidi_jack	*sc_in_jacks;
	struct umidi_jack	*sc_jacks;

	int			sc_num_mididevs;
	struct umidi_mididev	*sc_mididevs;

	int			sc_out_num_endpoints;
	struct umidi_endpoint	*sc_out_ep;
	int			sc_in_num_endpoints;
	struct umidi_endpoint	*sc_in_ep;
	struct umidi_endpoint	*sc_endpoints;
	int			cblnums_global;
};
