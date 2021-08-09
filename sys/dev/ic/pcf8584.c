/*	$NetBSD: pcf8584.c,v 1.19.2.1 2021/08/09 01:29:52 thorpej Exp $	*/
/*	$OpenBSD: pcf8584.c,v 1.9 2007/10/20 18:46:21 kettenis Exp $ */

/*
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/bus.h>

#include <dev/i2c/i2cvar.h>

#include <dev/ic/pcf8584var.h>
#include <dev/ic/pcf8584reg.h>

#include "locators.h"

/* Internal registers */
#define PCF8584_S0		0x00
#define PCF8584_S1		0x01
#define PCF8584_S2		0x02
#define PCF8584_S3		0x03

void		pcfiic_init(struct pcfiic_softc *);
int		pcfiic_i2c_exec(void *, i2c_op_t, i2c_addr_t, const void *,
		    size_t, void *, size_t, int);

int		pcfiic_xmit(struct pcfiic_softc *, u_int8_t, const u_int8_t *,
		    size_t, const u_int8_t *, size_t);
int		pcfiic_recv(struct pcfiic_softc *, u_int8_t, u_int8_t *,
		    size_t);

u_int8_t	pcfiic_read(struct pcfiic_softc *, bus_size_t);
void		pcfiic_write(struct pcfiic_softc *, bus_size_t, u_int8_t);
int		pcfiic_wait_BBN(struct pcfiic_softc *);
int		pcfiic_wait_pin(struct pcfiic_softc *, volatile u_int8_t *);

void
pcfiic_init(struct pcfiic_softc *sc)
{
	/* init S1 */
	pcfiic_write(sc, PCF8584_S1, PCF8584_CTRL_PIN);
	/* own address */
	pcfiic_write(sc, PCF8584_S0, sc->sc_addr);

	/* select clock reg */
	pcfiic_write(sc, PCF8584_S1, PCF8584_CTRL_PIN | PCF8584_CTRL_ES1);
	pcfiic_write(sc, PCF8584_S0, sc->sc_clock);

	pcfiic_write(sc, PCF8584_S1, PCF8584_CMD_IDLE);

	delay(200000);	/* Multi-Master mode, wait for longest i2c message */
}

void
pcfiic_attach(struct pcfiic_softc *sc, i2c_addr_t addr, uint8_t clock,
    int swapregs)
{
	struct i2cbus_attach_args iba;
	struct pcfiic_channel *ch;
	int i;

	if (swapregs) {
		sc->sc_regmap[PCF8584_S1] = PCF8584_S0;
		sc->sc_regmap[PCF8584_S0] = PCF8584_S1;
	} else {
		sc->sc_regmap[PCF8584_S0] = PCF8584_S0;
		sc->sc_regmap[PCF8584_S1] = PCF8584_S1;
	}
	sc->sc_clock = clock;
	sc->sc_addr = addr;

	pcfiic_init(sc);

	printf("\n");

	if (sc->sc_channels == NULL) {
		KASSERT(sc->sc_nchannels == 0);
		ch = kmem_alloc(sizeof(*sc->sc_channels), KM_SLEEP);
		ch->ch_channel = 0;
		ch->ch_devhandle = device_handle(sc->sc_dev);

		sc->sc_channels = ch;
		sc->sc_nchannels = 1;
	} else {
		KASSERT(sc->sc_nchannels != 0);
	}

	for (i = 0; i < sc->sc_nchannels; i++) {
		int locs[I2CBUSCF_NLOCS];

		ch = &sc->sc_channels[i];
		ch->ch_sc = sc;
		iic_tag_init(&ch->ch_i2c);
		ch->ch_i2c.ic_cookie = ch;
		ch->ch_i2c.ic_exec = pcfiic_i2c_exec;
		ch->ch_i2c.ic_acquire_bus = sc->sc_acquire_bus;
		ch->ch_i2c.ic_release_bus = sc->sc_release_bus;

		locs[I2CBUSCF_BUS] = ch->ch_i2c.ic_channel;

		memset(&iba, 0, sizeof(iba));
		iba.iba_tag = &ch->ch_i2c;
		config_found(sc->sc_dev, &iba,
		    sc->sc_nchannels == 1 ? iicbus_print : iicbus_print_multi,
		    CFARGS(.submatch = config_stdsubmatch,
			   .locators = locs,
			   .devhandle = ch->ch_devhandle));
	}
}

int
pcfiic_intr(void *arg)
{
	return (0);
}

int
pcfiic_i2c_exec(void *arg, i2c_op_t op, i2c_addr_t addr,
    const void *cmdbuf, size_t cmdlen, void *buf, size_t len, int flags)
{
	struct pcfiic_softc	*sc = arg;
	int			ret = 0;

#if 0
        printf("%s: exec op: %d addr: 0x%x cmdlen: %d len: %d flags 0x%x\n",
            device_xname(sc->sc_dev), op, addr, (int)cmdlen, (int)len, flags);
#endif

	if (sc->sc_poll)
		flags |= I2C_F_POLL;

	/*
	 * If we are writing, write address, cmdbuf, buf.
	 * If we are reading, write address, cmdbuf, then read address, buf.
	 */
	if (I2C_OP_WRITE_P(op)) {
		ret = pcfiic_xmit(sc, addr & 0x7f, cmdbuf, cmdlen, buf, len);
	} else {
		if (pcfiic_xmit(sc, addr & 0x7f, cmdbuf, cmdlen, NULL, 0) != 0)
			return (1);
		ret = pcfiic_recv(sc, addr & 0x7f, buf, len);
	}
	return (ret);
}

int
pcfiic_xmit(struct pcfiic_softc *sc, u_int8_t addr, const u_int8_t *cmdbuf,
    size_t cmdlen, const u_int8_t *buf, size_t len)
{
	int			i, err = 0;
	volatile u_int8_t	r;

	if (pcfiic_wait_BBN(sc) != 0)
		return (1);

	pcfiic_write(sc, PCF8584_S0, addr << 1);
	pcfiic_write(sc, PCF8584_S1, PCF8584_CMD_START);

	for (i = 0; i <= cmdlen + len; i++) {
		if (pcfiic_wait_pin(sc, &r) != 0) {
			pcfiic_write(sc, PCF8584_S1, PCF8584_CMD_STOP);
			return (1);
		}

		if (r & PCF8584_STATUS_LRB) {
			err = 1;
			break;
		}

		if (i < cmdlen)
			pcfiic_write(sc, PCF8584_S0, cmdbuf[i]);
		else if (i < cmdlen + len)
			pcfiic_write(sc, PCF8584_S0, buf[i - cmdlen]);
	}
	pcfiic_write(sc, PCF8584_S1, PCF8584_CMD_STOP);
	return (err);
}

int
pcfiic_recv(struct pcfiic_softc *sc, u_int8_t addr, u_int8_t *buf, size_t len)
{
	int			i = 0, err = 0;
	volatile u_int8_t	r;

	if (pcfiic_wait_BBN(sc) != 0)
		return (1);

	pcfiic_write(sc, PCF8584_S0, (addr << 1) | 0x01);
	pcfiic_write(sc, PCF8584_S1, PCF8584_CMD_START);

	for (i = 0; i <= len; i++) {
		if (pcfiic_wait_pin(sc, &r) != 0) {
			pcfiic_write(sc, PCF8584_S1, PCF8584_CMD_STOP);
			return (1);
		}

		if ((i != len) && (r & PCF8584_STATUS_LRB)) {
			pcfiic_write(sc, PCF8584_S1, PCF8584_CMD_STOP);
			return (1);
		}

		if (i == len - 1) {
			pcfiic_write(sc, PCF8584_S1, PCF8584_CMD_NAK);
		} else if (i == len) {
			pcfiic_write(sc, PCF8584_S1, PCF8584_CMD_STOP);
		}

		r = pcfiic_read(sc, PCF8584_S0);
		if (i > 0)
			buf[i - 1] = r;
	}
	return (err);
}

u_int8_t
pcfiic_read(struct pcfiic_softc *sc, bus_size_t r)
{
	bus_space_barrier(sc->sc_iot, sc->sc_ioh, sc->sc_regmap[r], 1,
	    BUS_SPACE_BARRIER_READ);
	return (bus_space_read_1(sc->sc_iot, sc->sc_ioh, sc->sc_regmap[r]));
}

void
pcfiic_write(struct pcfiic_softc *sc, bus_size_t r, u_int8_t v)
{
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, sc->sc_regmap[r], v);
	(void)bus_space_read_1(sc->sc_iot, sc->sc_ioh, PCF8584_S1);
}

int
pcfiic_wait_BBN(struct pcfiic_softc *sc)
{
	int		i;

	for (i = 0; i < 1000; i++) {
		if (pcfiic_read(sc, PCF8584_S1) & PCF8584_STATUS_BBN)
			return (0);
		delay(1000);
	}
	return (1);
}

int
pcfiic_wait_pin(struct pcfiic_softc *sc, volatile u_int8_t *r)
{
	int		i;

	for (i = 0; i < 1000; i++) {
		*r = pcfiic_read(sc, PCF8584_S1);
		if ((*r & PCF8584_STATUS_PIN) == 0)
			return (0);
		delay(1000);
	}
	return (1);
}
