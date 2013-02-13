/*	$NetBSD: bcm2835_bsc.c,v 1.1.4.2 2013/02/13 01:36:14 riz Exp $	*/

/*
 * Copyright (c) 2012 Jonathan A. Kollasch
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bcm2835_bsc.c,v 1.1.4.2 2013/02/13 01:36:14 riz Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/mutex.h>
#include <sys/bus.h>
#include <sys/intr.h>

#include <dev/i2c/i2cvar.h>

#include <arm/broadcom/bcm_amba.h>
#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm2835_bscreg.h>
#include <arm/broadcom/bcm2835_gpio_subr.h>

#if defined(_KERNEL_OPT)
#include "opt_kernhist.h"
#endif
#include <sys/kernhist.h>

KERNHIST_DECL(bsciichist);

struct bsciic_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_size_t sc_ios;
	struct i2c_controller sc_i2c;
	kmutex_t sc_buslock;
	void *sc_inth;
};

static int bsciic_match(device_t, cfdata_t, void *);
static void bsciic_attach(device_t, device_t, void *);

void bsciic_dump_regs(struct bsciic_softc * const);

static int  bsciic_acquire_bus(void *, int);
static void bsciic_release_bus(void *, int);
static int  bsciic_exec(void *, i2c_op_t, i2c_addr_t, const void *, size_t,
    void *, size_t, int);

CFATTACH_DECL_NEW(bsciic, sizeof(struct bsciic_softc),
    bsciic_match, bsciic_attach, NULL, NULL);

static int
bsciic_match(device_t parent, cfdata_t match, void *aux)
{
	struct amba_attach_args * const aaa = aux;

	if (strcmp(aaa->aaa_name, "bcmbsc") != 0)
		return 0;

	return 1;
}

static void
bsciic_attach(device_t parent, device_t self, void *aux)
{
	struct bsciic_softc * const sc = device_private(self);
	struct amba_attach_args * const aaa = aux;
	struct i2cbus_attach_args iba;
	u_int bscunit = ~0;

	switch (aaa->aaa_addr) {
	case BCM2835_BSC0_BASE:
		bscunit = 0;
		break;
	case BCM2835_BSC1_BASE:
		bscunit = 1;
		break;
	}

	aprint_naive("\n");
	aprint_normal(": BSC%u\n", bscunit);

	KERNHIST_INIT(bsciichist, 512);

	sc->sc_dev = self;

	mutex_init(&sc->sc_buslock, MUTEX_DEFAULT, IPL_NONE);

	sc->sc_iot = aaa->aaa_iot;
	if (bus_space_map(aaa->aaa_iot, aaa->aaa_addr, aaa->aaa_size, 0,
	    &sc->sc_ioh) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to map device\n");
		return;
	}
	sc->sc_ios = aaa->aaa_size;

	switch (aaa->aaa_addr) {
	case BCM2835_BSC0_BASE:
		/* SDA0 on GPIO0, SCL0 on GPIO1 */
		bcm2835gpio_function_select(0, BCM2835_GPIO_ALT0);
		bcm2835gpio_function_select(1, BCM2835_GPIO_ALT0);
		break;
	case BCM2835_BSC1_BASE:
		/* SDA1 on GPIO2, SCL1 on GPIO3 */
		bcm2835gpio_function_select(2, BCM2835_GPIO_ALT0);
		bcm2835gpio_function_select(3, BCM2835_GPIO_ALT0);
		break;
	}

	/* clear FIFO, disable controller */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_C, BSC_C_CLEAR_CLEAR);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_S, BSC_S_CLKT |
	    BSC_S_ERR | BSC_S_DONE);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_DIV,
	   __SHIFTIN(250000000/100000, BSC_DIV_CDIV)); // XXX may not be this

	sc->sc_i2c.ic_cookie = sc;
	sc->sc_i2c.ic_acquire_bus = bsciic_acquire_bus;
	sc->sc_i2c.ic_release_bus = bsciic_release_bus;
	sc->sc_i2c.ic_exec = bsciic_exec;

	memset(&iba, 0, sizeof(iba));

	iba.iba_tag = &sc->sc_i2c;
	iba.iba_type = 0;
	config_found_ia(self, "i2cbus", &iba, iicbus_print);
}

void
bsciic_dump_regs(struct bsciic_softc * const sc)
{
	KERNHIST_FUNC(__func__);
	KERNHIST_CALLED(bsciichist);

	KERNHIST_LOG(bsciichist, "C %08x S %08x D %08x A %08x",
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, BSC_C),
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, BSC_S),
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, BSC_DLEN),
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, BSC_A)
	    );
}

static int
bsciic_acquire_bus(void *v, int flags)
{
	struct bsciic_softc * const sc = v;
	uint32_t s;

	mutex_enter(&sc->sc_buslock);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_S, BSC_S_CLKT |
	    BSC_S_ERR | BSC_S_DONE);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_C, BSC_C_I2CEN |
	    BSC_C_CLEAR_CLEAR);
	s = bus_space_read_4(sc->sc_iot, sc->sc_ioh, BSC_S);
	KASSERT((s & BSC_S_TA) == 0);

	return 0;
}

static void
bsciic_release_bus(void *v, int flags)
{
	struct bsciic_softc * const sc = v;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_C, BSC_C_CLEAR_CLEAR);

	mutex_exit(&sc->sc_buslock);
}

static int
bsciic_exec(void *v, i2c_op_t op, i2c_addr_t addr, const void *cmdbuf,
    size_t cmdlen, void *databuf, size_t datalen, int flags)
{
	KERNHIST_FUNC(__func__); KERNHIST_CALLED(bsciichist);
	struct bsciic_softc * const sc = v;
	uint32_t c, s, dlen, a;
	uint32_t j;
	uint8_t *buf;
	size_t len;
	size_t pos;
	int error = 0;
	const bool isread = I2C_OP_READ_P(op);

	flags |= I2C_F_POLL;

#if 0
	device_printf(sc->sc_dev, "exec: op %d, addr 0x%x, cmdbuf %p, "
	    "cmdlen %zu, databuf %p, datalen %zu, flags 0x%x\n",
	    op, addr, cmdbuf, cmdlen, databuf, datalen, flags);
#endif

	a = __SHIFTIN(addr, BSC_A_ADDR);
	c = BSC_C_I2CEN | BSC_C_CLEAR_CLEAR;
#if notyet
	c |= BSC_C_INTR | BSC_C_INTT | BSC_C_INTD;
#endif

	buf = __UNCONST(cmdbuf);
	len = cmdlen;

	if (isread)
		dlen = cmdlen;
	else
		dlen = cmdlen + datalen;
	dlen = __SHIFTIN(dlen, BSC_DLEN_DLEN);

	s = bus_space_read_4(sc->sc_iot, sc->sc_ioh, BSC_S);
	if ((s & BSC_S_TA) != 0)
		bsciic_dump_regs(sc);
	KASSERT((s & BSC_S_TA) == 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_DLEN, dlen);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_A, a);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_C, c | BSC_C_ST);

	do {
		s = bus_space_read_4(sc->sc_iot, sc->sc_ioh, BSC_S);
	} while ((s & BSC_S_TA) == 0);

flood_again:
	KERNHIST_LOG(bsciichist, "flood top %p %zu", buf, len, 0, 0);
	j = 10000000;
	for (pos = 0; pos < len; ) {
		if (--j == 0)
			return -1;
		s = bus_space_read_4(sc->sc_iot, sc->sc_ioh, BSC_S);
		KERNHIST_LOG(bsciichist, "w s %08x", s, 0, 0, 0);
		if ((s & BSC_S_CLKT) != 0) {
			error = EIO;
			goto done;
		}
		if ((s & BSC_S_ERR) != 0) {
			error = EIO;
			goto done;
		}
		if ((s & BSC_S_DONE) != 0)
			break;
		if ((s & BSC_S_TXD) == 0)
			continue;
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_FIFO, buf[pos]);
		KERNHIST_LOG(bsciichist, "w %p %p %02x", buf, &buf[pos],
		    buf[pos], 0);
		pos++;
	}
	KERNHIST_LOG(bsciichist, "flood bot %p %zu", buf, len, 0, 0);

	if (buf == cmdbuf && !isread) {
		buf = databuf;
		len = datalen;
		goto flood_again;
	}

	do {
		s = bus_space_read_4(sc->sc_iot, sc->sc_ioh, BSC_S);
	} while ((s & BSC_S_TA) != 0);

	s = bus_space_read_4(sc->sc_iot, sc->sc_ioh, BSC_S);
	s &= BSC_S_CLKT|BSC_S_ERR|BSC_S_DONE;
	KASSERT((s & BSC_S_DONE) != 0);
	if (s != 0)
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_S, s);

	if (!isread)
		goto done;

	c |= BSC_C_READ;

	buf = databuf;
	len = datalen;
	dlen = datalen;

	dlen = __SHIFTIN(dlen, BSC_DLEN_DLEN);
	KASSERT(dlen >= 1);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_DLEN, dlen);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_A, a);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_C, c | BSC_C_ST);

	do {
		s = bus_space_read_4(sc->sc_iot, sc->sc_ioh, BSC_S);
	} while ((s & BSC_S_TA) == 0);

	KERNHIST_LOG(bsciichist, "drain top %p %zu", buf, len, 0, 0);
	j = 10000000;
	for (pos = 0; pos < len; ) {
		if (--j == 0)
			return -1;
		s = bus_space_read_4(sc->sc_iot, sc->sc_ioh, BSC_S);
		KERNHIST_LOG(bsciichist, "r s %08x", s, 0, 0, 0);
		if ((s & BSC_S_CLKT) != 0) {
			error = EIO;
			goto done;
		}
		if ((s & BSC_S_ERR) != 0) {
			error = EIO;
			goto done;
		}
		if ((s & BSC_S_DONE) != 0)
			break;
		if ((s & BSC_S_RXD) == 0)
			continue;
		j = 10000000;
		buf[pos] = bus_space_read_4(sc->sc_iot, sc->sc_ioh, BSC_FIFO);
		KERNHIST_LOG(bsciichist, "r %p %p %02x", buf, &buf[pos],
		    buf[pos], 0);
		pos++;
	}
	KERNHIST_LOG(bsciichist, "drain bot %p %zu", buf, len, 0, 0);

	do {
		s = bus_space_read_4(sc->sc_iot, sc->sc_ioh, BSC_S);
	} while ((s & BSC_S_TA) != 0);

	s = bus_space_read_4(sc->sc_iot, sc->sc_ioh, BSC_S);
	s &= BSC_S_CLKT|BSC_S_ERR|BSC_S_DONE;
	KASSERT((s & BSC_S_DONE) != 0);
	if (s != 0)
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_S, s);

done:
	bsciic_dump_regs(sc);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_DLEN, 0);

	do {
		s = bus_space_read_4(sc->sc_iot, sc->sc_ioh, BSC_S);
	} while ((s & BSC_S_TA) != 0);

	bsciic_dump_regs(sc);

	s = bus_space_read_4(sc->sc_iot, sc->sc_ioh, BSC_S);
	s &= BSC_S_CLKT|BSC_S_ERR|BSC_S_DONE;
	if (s != 0)
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_S, s);

	bsciic_dump_regs(sc);

	return error;
}
