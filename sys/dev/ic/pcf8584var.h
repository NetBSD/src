/*	$NetBSD: pcf8584var.h,v 1.6.22.1 2021/08/09 01:29:52 thorpej Exp $	*/
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

struct pcfiic_channel {
	struct i2c_controller	ch_i2c;
	struct pcfiic_softc	*ch_sc;
	devhandle_t		ch_devhandle;
	int			ch_channel;
};

struct pcfiic_softc {
	device_t		sc_dev;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	uint8_t			sc_addr;
	uint8_t			sc_clock;
	uint8_t			sc_regmap[2];

	int			sc_poll;

	/*
	 * Some Sun clones of the this i2c controller support
	 * multiple channels.  The specific attachment will
	 * initialize these fields for controllers that support
	 * this.  If not, the core driver will assume a single
	 * channel.
	 */
	struct pcfiic_channel	*sc_channels;
	int			sc_nchannels;
	int			(*sc_acquire_bus)(void *, int);
	void			(*sc_release_bus)(void *, int);
};

void	pcfiic_attach(struct pcfiic_softc *, i2c_addr_t, uint8_t, int);
int	pcfiic_intr(void *);

#endif /* _DEV_IC_PCF8584VAR_H_ */
