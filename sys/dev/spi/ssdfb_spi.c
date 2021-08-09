/* $NetBSD: ssdfb_spi.c,v 1.9.2.1 2021/08/09 00:30:09 thorpej Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: ssdfb_spi.c,v 1.9.2.1 2021/08/09 00:30:09 thorpej Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>

#include <dev/spi/spivar.h>
#include <dev/ic/ssdfbvar.h>
#include "opt_fdt.h"
#ifdef FDT
#include <dev/fdt/fdtvar.h>
#endif /* FDT */

struct bs_state {
	uint8_t	*base;
	uint8_t	*cur;
	uint8_t	mask;
};

struct ssdfb_spi_softc {
	struct ssdfb_softc	sc;
	struct spi_handle	*sc_sh;
#ifdef FDT
	struct fdtbus_gpio_pin	*sc_gpio_dc;
	struct fdtbus_gpio_pin	*sc_gpio_res;
#endif /* FDT */
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
static int	ssdfb_spi_xfer_rect_4wire_ssd1353(void *, uint8_t, uint8_t,
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
	{ .compat = "solomon,ssd1306",	.value = SSDFB_PRODUCT_SSD1306_GENERIC },
	{ .compat = "solomon,ssd1322",	.value = SSDFB_PRODUCT_SSD1322_GENERIC },
	{ .compat = "solomon,ssd1353",	.value = SSDFB_PRODUCT_SSD1353_GENERIC },
	{ .compat = "dep160128a",	.value = SSDFB_PRODUCT_DEP_160128A_RGB },
	DEVICE_COMPAT_EOL
};

static int
ssdfb_spi_match(device_t parent, cfdata_t match, void *aux)
{
	struct spi_attach_args *sa = aux;

	return spi_compatible_match(sa, match, compat_data);
}

#ifdef FDT
static void
ssdfb_spi_gpio_fdt(struct ssdfb_spi_softc *sc)
{
	devhandle_t devhandle = device_handle(sc->sc.sc_dev);
	int phandle = devhandle_to_of(devhandle);

	sc->sc_gpio_dc = fdtbus_gpio_acquire(phandle, "dc-gpio",
	    GPIO_PIN_OUTPUT);
	if (sc->sc_gpio_dc == NULL) {
		sc->sc_gpio_dc = fdtbus_gpio_acquire(phandle, "cd-gpio",
		    GPIO_PIN_OUTPUT);
	}
	if (sc->sc_gpio_dc != NULL) {
		sc->sc_3wiremode = false;
	}

	sc->sc_gpio_res = fdtbus_gpio_acquire(phandle, "res-gpio",
	    GPIO_PIN_OUTPUT);
	if (sc->sc_gpio_res) {
		fdtbus_gpio_write_raw(sc->sc_gpio_res, 0);
		DELAY(100);
		fdtbus_gpio_write_raw(sc->sc_gpio_res, 1);
		DELAY(100);
	}
}
#endif /* FDT */

static void
ssdfb_spi_attach(device_t parent, device_t self, void *aux)
{
	struct ssdfb_spi_softc *sc = device_private(self);
	devhandle_t devhandle = device_handle(self);
	struct cfdata *cf = device_cfdata(self);
	struct spi_attach_args *sa = aux;
	int flags = cf->cf_flags;
	int error;

	sc->sc.sc_dev = self;
	sc->sc_sh = sa->sa_handle;
	sc->sc.sc_cookie = (void *)sc;

	/*
	 * SSD1306 and SSD1322 data sheets specify 100ns cycle time.
	 */
	error = spi_configure(sa->sa_handle, SPI_MODE_0, 10000000);
	if (error) {
		aprint_error(": spi_configure failed (error = %d)\n",
		    error);
		return;
	}

	if ((flags & SSDFB_ATTACH_FLAG_PRODUCT_MASK) == SSDFB_PRODUCT_UNKNOWN) {
		const struct device_compatible_entry *dce =
			device_compatible_lookup(sa->sa_compat, sa->sa_ncompat,
						 compat_data);
		if (dce)
			flags |= (int)dce->value;
		else
			flags |= SSDFB_PRODUCT_SSD1322_GENERIC;
	}

	/*
	 * Note on interface modes.
	 *
	 * 3 wire mode sends 9 bit sequences over the MOSI, MSB contains
	 * the bit that determines if the lower 8 bits are command or data.
	 *
	 * 4 wire mode sends 8 bit sequences and requires an auxiliary GPIO
	 * pin for the command/data bit.
	 *
	 * Default to 3 wire mode.  If the device tree specifies a
	 * D/C GPIO pin, then we will use 4 wire mode.
	 */
	sc->sc_3wiremode = true;
	switch (devhandle_type(devhandle)) {
#ifdef FDT
	case DEVHANDLE_TYPE_OF:
		ssdfb_spi_gpio_fdt(sc);
		break;
#endif /* FDT */
	default:
		break;
	}

	sc->sc.sc_cmd = sc->sc_3wiremode
	    ? ssdfb_spi_cmd_3wire
	    : ssdfb_spi_cmd_4wire;

	switch (flags & SSDFB_ATTACH_FLAG_PRODUCT_MASK) {
	case SSDFB_PRODUCT_SSD1322_GENERIC:
		sc->sc.sc_transfer_rect = sc->sc_3wiremode
		    ? ssdfb_spi_xfer_rect_3wire_ssd1322
		    : ssdfb_spi_xfer_rect_4wire_ssd1322;
		break;
	case SSDFB_PRODUCT_SSD1353_GENERIC:
	case SSDFB_PRODUCT_DEP_160128A_RGB:
		sc->sc.sc_transfer_rect = sc->sc_3wiremode
		    ? NULL /* not supported here */
		    : ssdfb_spi_xfer_rect_4wire_ssd1353;
		break;
	}

	if (!sc->sc.sc_transfer_rect) {
		aprint_error(": sc_transfer_rect not implemented\n");
		return;
	}

	ssdfb_attach(&sc->sc, flags);

	aprint_normal_dev(self, "%d-wire SPI interface\n",
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
	/* TODO: refactor this if we ever support more that just FDT. */

#ifdef FDT
	KASSERT(sc->sc_gpio_dc != NULL);
	fdtbus_gpio_write(sc->sc_dc_gpio, value);
#else
	/* TODO: this should toggle an auxilliary GPIO pin */
	panic("ssdfb_spi_4wire_set_dc");
#endif /* FDT */
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

static int
ssdfb_spi_xfer_rect_4wire_ssd1353(void *cookie, uint8_t fromcol, uint8_t tocol,
    uint8_t fromrow, uint8_t torow, uint8_t *p, size_t stride, bool usepoll)
{
	struct ssdfb_spi_softc *sc = (struct ssdfb_spi_softc *)cookie;
	uint8_t row;
	size_t rlen = (tocol + 1 - fromcol) * 3;
	uint8_t bitstream[160 * 3];
	uint8_t *dstp, *srcp, *endp;
	int error;
	uint8_t cmd;
	uint8_t data[2];

	/*
	 * Unlike iic(4), there is no way to force spi(4) to use polling.
	 */
	if (usepoll && !cold)
		return 0;

	ssdfb_spi_4wire_set_dc(sc, 0);
	cmd = SSD1353_CMD_SET_ROW_ADDRESS;
	error = spi_send(sc->sc_sh, sizeof(cmd), &cmd);
	if (error)
		return error;
	ssdfb_spi_4wire_set_dc(sc, 1);
	data[0] = fromrow;
	data[1] = torow;
	if (sc->sc.sc_upsidedown) {
		/* fix picture outside frame on 160x128 panel */
		data[0] += 132 - sc->sc.sc_p->p_height;
		data[1] += 132 - sc->sc.sc_p->p_height;
	}
	error = spi_send(sc->sc_sh, sizeof(data), data);
	if (error)
		return error;

	ssdfb_spi_4wire_set_dc(sc, 0);
	cmd = SSD1353_CMD_SET_COLUMN_ADDRESS;
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
	cmd = SSD1353_CMD_WRITE_RAM;
	error = spi_send(sc->sc_sh, sizeof(cmd), &cmd);
	if (error)
		return error;

	ssdfb_spi_4wire_set_dc(sc, 1);
	KASSERT(rlen <= sizeof(bitstream));
	for (row = fromrow; row <= torow; row++) {
		/* downconvert each row from 32bpp rgba to 18bpp panel format */
		dstp = bitstream;
		endp = dstp + rlen;
		srcp = p;
		while (dstp < endp) {
			*dstp++ = (*srcp++) >> 2;
			*dstp++ = (*srcp++) >> 2;
			*dstp++ = (*srcp++) >> 2;
			srcp++;
		}
		error = spi_send(sc->sc_sh, rlen, bitstream);
		if (error)
			return error;
		p += stride;
	}

	return 0;
}
