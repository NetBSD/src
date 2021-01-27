/* $NetBSD: ssdfb_spi.c,v 1.5 2021/01/27 02:32:31 thorpej Exp $ */

/*
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tobias Nygren.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ssdfb_spi.c,v 1.5 2021/01/27 02:32:31 thorpej Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>
#include <dev/spi/spivar.h>
#include <dev/ic/ssdfbvar.h>

struct bs_state {
	uint8_t	*base;
	uint8_t	*cur;
	uint8_t	mask;
};

struct ssdfb_spi_softc {
	struct ssdfb_softc	sc;
	struct spi_handle	*sc_sh;
	bool			sc_3wiremode;
};

static int	ssdfb_spi_match(device_t, cfdata_t, void *);
static void	ssdfb_spi_attach(device_t, device_t, void *);

static int	ssdfb_spi_cmd_3wire(void *, uint8_t *, size_t, bool);
static int	ssdfb_spi_xfer_rect_3wire_ssd1322(void *, uint8_t, uint8_t,
		    uint8_t, uint8_t, uint8_t *, size_t, bool);

static int	ssdfb_spi_cmd_4wire(void *, uint8_t *, size_t, bool);
static int	ssdfb_spi_xfer_rect_4wire_ssd1322(void *, uint8_t, uint8_t,
		    uint8_t, uint8_t, uint8_t *, size_t, bool);

static void	ssdfb_bitstream_init(struct bs_state *, uint8_t *);
static void	ssdfb_bitstream_append(struct bs_state *, uint8_t, uint8_t);
static void	ssdfb_bitstream_append_cmd(struct bs_state *, uint8_t);
static void	ssdfb_bitstream_append_data(struct bs_state *, uint8_t *,
		    size_t);
static void	ssdfb_bitstream_final(struct bs_state *);

CFATTACH_DECL_NEW(ssdfb_spi, sizeof(struct ssdfb_spi_softc),
    ssdfb_spi_match, ssdfb_spi_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "solomon,ssd1322" },
	DEVICE_COMPAT_EOL
};

static int
ssdfb_spi_match(device_t parent, cfdata_t match, void *aux)
{
	struct spi_attach_args *sa = aux;
	int res;

	res = spi_compatible_match(sa, match, compat_data);
	if (!res)
		return res;

	/*
	 * SSD1306 and SSD1322 data sheets specify 100ns cycle time.
	 */
	if (spi_configure(sa->sa_handle, SPI_MODE_0, 10000000))
		res = 0;

	return res;
}

static void
ssdfb_spi_attach(device_t parent, device_t self, void *aux)
{
	struct ssdfb_spi_softc *sc = device_private(self);
	struct cfdata *cf = device_cfdata(self);
	struct spi_attach_args *sa = aux;
	int flags = cf->cf_flags;

	sc->sc.sc_dev = self;
	sc->sc_sh = sa->sa_handle;
	sc->sc.sc_cookie = (void *)sc;
	if ((flags & SSDFB_ATTACH_FLAG_PRODUCT_MASK) == SSDFB_PRODUCT_UNKNOWN)
		flags |= SSDFB_PRODUCT_SSD1322_GENERIC;
	/*
	 * Note on interface modes.
	 *
	 * 3 wire mode sends 9 bit sequences over the MOSI, MSB contains
	 * the bit that determines if the lower 8 bits are command or data.
	 *
	 * 4 wire mode sends 8 bit sequences and requires an auxiliary GPIO
	 * pin for the command/data bit. But in other to allocate a GPIO pin
	 * we need to use fdt, so only support 3 wire mode in this frontend,
	 * at least for now.
	 */
	sc->sc_3wiremode = true;

	switch (flags & SSDFB_ATTACH_FLAG_PRODUCT_MASK) {
	case SSDFB_PRODUCT_SSD1322_GENERIC:
		if (sc->sc_3wiremode) {
			sc->sc.sc_transfer_rect =
			    ssdfb_spi_xfer_rect_3wire_ssd1322;
		} else {
			sc->sc.sc_transfer_rect =
			    ssdfb_spi_xfer_rect_4wire_ssd1322;
		}
		break;
	default:
		panic("ssdfb_spi_attach: product not implemented");
	}
	if (sc->sc_3wiremode) {
		sc->sc.sc_cmd = ssdfb_spi_cmd_3wire;
	} else {
		sc->sc.sc_cmd = ssdfb_spi_cmd_4wire;
	}
	
	ssdfb_attach(&sc->sc, flags);

	device_printf(sc->sc.sc_dev, "%d-wire SPI interface\n",
	    sc->sc_3wiremode == true ? 3 : 4);
}

static int
ssdfb_spi_cmd_3wire(void *cookie, uint8_t *cmd, size_t len, bool usepoll)
{
	struct ssdfb_spi_softc *sc = (struct ssdfb_spi_softc *)cookie;
	uint8_t bitstream[16 * 9 / 8];
	struct bs_state s;

	KASSERT(len > 0 && len <= 16);
	ssdfb_bitstream_init(&s, bitstream);
	ssdfb_bitstream_append_cmd(&s, *cmd);
	cmd++;
	len--;
	ssdfb_bitstream_append_data(&s, cmd, len);
	ssdfb_bitstream_final(&s);

	return spi_send(sc->sc_sh, s.cur - s.base, bitstream);
}

static int
ssdfb_spi_xfer_rect_3wire_ssd1322(void *cookie, uint8_t fromcol, uint8_t tocol,
    uint8_t fromrow, uint8_t torow, uint8_t *p, size_t stride, bool usepoll)
{
	struct ssdfb_spi_softc *sc = (struct ssdfb_spi_softc *)cookie;
	uint8_t bitstream[128 * 9 / 8];
	struct bs_state s;
	uint8_t row;
	size_t rlen = (tocol + 1 - fromcol) * 2;
	int error;

	/*
	 * Unlike iic(4), there is no way to force spi(4) to use polling.
	 */
	if (usepoll && !cold)
		return 0;

	ssdfb_bitstream_init(&s, bitstream);
	ssdfb_bitstream_append_cmd(&s, SSD1322_CMD_SET_ROW_ADDRESS);
	ssdfb_bitstream_append_data(&s, &fromrow, 1);
	ssdfb_bitstream_append_data(&s, &torow, 1);
	ssdfb_bitstream_append_cmd(&s, SSD1322_CMD_SET_COLUMN_ADDRESS);
	ssdfb_bitstream_append_data(&s, &fromcol, 1);
	ssdfb_bitstream_append_data(&s, &tocol, 1);
	ssdfb_bitstream_append_cmd(&s, SSD1322_CMD_WRITE_RAM);
	ssdfb_bitstream_final(&s);
	error = spi_send(sc->sc_sh, s.cur - s.base, bitstream);
	if (error)
		return error;

	KASSERT(rlen <= 128);
	for (row = fromrow; row <= torow; row++) {
		ssdfb_bitstream_init(&s, bitstream);
		ssdfb_bitstream_append_data(&s, p, rlen);
		ssdfb_bitstream_final(&s);
		error = spi_send(sc->sc_sh, s.cur - s.base, bitstream);
		if (error)
			return error;
		p += stride;
	}

	return 0;
}

static void
ssdfb_bitstream_init(struct bs_state *s, uint8_t *dst)
{
	s->base = s->cur = dst;
	s->mask = 0x80;
}

static void
ssdfb_bitstream_append(struct bs_state *s, uint8_t b, uint8_t srcmask)
{
	while(srcmask) {
		if (b & srcmask)
			*s->cur |= s->mask;
		else
			*s->cur &= ~s->mask;
		srcmask >>= 1;
		s->mask >>= 1;
		if (!s->mask) {
			s->mask = 0x80;
			s->cur++;
		}
	}
}

static void
ssdfb_bitstream_append_cmd(struct bs_state *s, uint8_t cmd)
{
	ssdfb_bitstream_append(s, 0, 1);
	ssdfb_bitstream_append(s, cmd, 0x80);
}

static void
ssdfb_bitstream_append_data(struct bs_state *s, uint8_t *data, size_t len)
{
	while(len--) {
		ssdfb_bitstream_append(s, 1, 1);
		ssdfb_bitstream_append(s, *data++, 0x80);
	}
}

static void
ssdfb_bitstream_final(struct bs_state *s)
{
	uint8_t padding_cmd = SSD1322_CMD_WRITE_RAM;
	/* padding_cmd = SSDFB_NOP_CMD; */

	while (s->mask != 0x80) {
		ssdfb_bitstream_append_cmd(s, padding_cmd);
	}
}

static void
ssdfb_spi_4wire_set_dc(struct ssdfb_spi_softc *sc, int value)
{
	/* TODO: this should toggle an auxilliary GPIO pin */
	panic("ssdfb_spi_4wire_set_dc");
}

static int
ssdfb_spi_cmd_4wire(void *cookie, uint8_t *cmd, size_t len, bool usepoll)
{
	struct ssdfb_spi_softc *sc = (struct ssdfb_spi_softc *)cookie;
	int error;

	ssdfb_spi_4wire_set_dc(sc, 0);
	error = spi_send(sc->sc_sh, 1, cmd);
	if (error)
		return error;
	if (len > 1) {
		ssdfb_spi_4wire_set_dc(sc, 1);
		len--;
		cmd++;
		error = spi_send(sc->sc_sh, len, cmd);
		if (error)
			return error;
	}

	return 0;
}

static int
ssdfb_spi_xfer_rect_4wire_ssd1322(void *cookie, uint8_t fromcol, uint8_t tocol,
    uint8_t fromrow, uint8_t torow, uint8_t *p, size_t stride, bool usepoll)
{
	struct ssdfb_spi_softc *sc = (struct ssdfb_spi_softc *)cookie;
	uint8_t row;
	size_t rlen = (tocol + 1 - fromcol) * 2;
	int error;
	uint8_t cmd;
	uint8_t data[2];

	/*
	 * Unlike iic(4), there is no way to force spi(4) to use polling.
	 */
	if (usepoll && !cold)
		return 0;

	ssdfb_spi_4wire_set_dc(sc, 0);
	cmd = SSD1322_CMD_SET_ROW_ADDRESS;
	error = spi_send(sc->sc_sh, sizeof(cmd), &cmd);
	if (error)
		return error;
	ssdfb_spi_4wire_set_dc(sc, 1);
	data[0] = fromrow;
	data[1] = torow;
	error = spi_send(sc->sc_sh, sizeof(data), data);
	if (error)
		return error;

	ssdfb_spi_4wire_set_dc(sc, 0);
	cmd = SSD1322_CMD_SET_COLUMN_ADDRESS;
	error = spi_send(sc->sc_sh, sizeof(cmd), &cmd);
	if (error)
		return error;
	ssdfb_spi_4wire_set_dc(sc, 1);
	data[0] = fromcol;
	data[1] = tocol;
	error = spi_send(sc->sc_sh, sizeof(data), data);
	if (error)
		return error;

	ssdfb_spi_4wire_set_dc(sc, 0);
	cmd = SSD1322_CMD_WRITE_RAM;
	error = spi_send(sc->sc_sh, sizeof(cmd), &cmd);
	if (error)
		return error;

	ssdfb_spi_4wire_set_dc(sc, 1);
	for (row = fromrow; row <= torow; row++) {
		error = spi_send(sc->sc_sh, rlen, p);
		if (error)
			return error;
		p += stride;
	}

	return 0;
}
