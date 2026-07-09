/*	$NetBSD: pcf8584var.h,v 1.9 2026/07/09 14:48:42 thorpej Exp $	*/
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

#ifndef _DEV_IC_PCF8584VAR_H_
#define	_DEV_IC_PCF8584VAR_H_

struct pcfiic_softc {
	device_t		sc_dev;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_space_handle_t	sc_mux_ioh;
	u_int8_t		sc_addr;
	u_int8_t		sc_clock;
	u_int8_t		sc_regmap[2];

	bool			sc_has_mux;

	int			sc_poll;
	int			sc_delay;

	struct i2c_controller	sc_i2c;
};

/*
 * The PCF8584 has only a single address input pin.  These are
 * indices into the sc_regmap[] that contain the offsets from
 * the base io handle needed to drive that pin according to the
 * desired register access.
 */
#define	PCF8584_S0		0
#define	PCF8584_S1		1

void	pcfiic_attach(struct pcfiic_softc *, i2c_addr_t, u_int8_t);
int	pcfiic_intr(void *);

#endif /* _DEV_IC_PCF8584VAR_H_ */
