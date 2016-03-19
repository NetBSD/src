/*	$NetBSD: pcf8584var.h,v 1.4.38.1 2016/03/19 11:30:09 skrll Exp $	*/
/*	$OpenBSD: pcf8584var.h,v 1.5 2007/10/20 18:46:21 kettenis Exp $ */

/*
 * Copyright (c) 2006 David Gwynne <dlg@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

struct pcfiic_softc {
	device_t		sc_dev;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_space_handle_t	sc_ioh2;
	int			sc_master;
	u_int8_t		sc_addr;
	u_int8_t		sc_clock;
	u_int8_t		sc_regmap[2];

	int			sc_poll;

	struct i2c_controller	sc_i2c;
	krwlock_t		sc_lock;
};

void	pcfiic_attach(struct pcfiic_softc *, i2c_addr_t, u_int8_t, int);
int	pcfiic_intr(void *);
