/*	$NetBSD: bcm2835_bsc.c,v 1.9 2017/12/28 22:42:36 christos Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: bcm2835_bsc.c,v 1.9 2017/12/28 22:42:36 christos Exp $");

#if defined(_KERNEL_OPT)
#include "opt_kernhist.h"
#endif

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/kernhist.h>
#include <sys/intr.h>
#include <sys/mutex.h>
#include <sys/once.h>
#include <sys/systm.h>

#include <dev/i2c/i2cvar.h>

#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm2835_bscreg.h>

#include <dev/fdt/fdtvar.h>

KERNHIST_DEFINE(bsciichist);

struct bsciic_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	struct i2c_controller sc_i2c;
	kmutex_t sc_buslock;
	void *sc_inth;

	struct clk *sc_clk;
	u_int sc_frequency;
	u_int sc_clkrate;
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
bsciic_init(void)
{

	KERNHIST_INIT(bsciichist, 512);

	return 0;
}

static int
bsciic_match(device_t parent, cfdata_t match, void *aux)
{
	const char * const compatible[] = { "brcm,bcm2835-i2c", NULL };
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
bsciic_attach(device_t parent, device_t self, void *aux)
{
	struct bsciic_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	prop_dictionary_t prop = device_properties(self);
	struct i2cbus_attach_args iba;
	bool disable = false;

	static ONCE_DECL(control);
	RUN_ONCE(&control, bsciic_init);

	bus_addr_t addr;
	bus_size_t size;

	sc->sc_dev = self;
	sc->sc_iot = faa->faa_bst;

	int error = fdtbus_get_reg(phandle, 0, &addr, &size);
	if (error) {
		aprint_error(": unable to get device registers\n");
		return;
	}

	prop_dictionary_get_bool(prop, "disable", &disable);
	if (disable) {
		aprint_naive(": disabled\n");
		aprint_normal(": disabled\n");
		return;
	}

	/* Enable clock */
	sc->sc_clk = fdtbus_clock_get_index(phandle, 0);
	if (sc->sc_clk == NULL) {
		aprint_error(": couldn't acquire clock\n");
		return;
	}

	if (clk_enable(sc->sc_clk) != 0) {
		aprint_error(": failed to enable clock\n");
		return;
	}

	sc->sc_frequency = clk_get_rate(sc->sc_clk);

	if (of_getprop_uint32(phandle, "clock-frequency",
	    &sc->sc_clkrate) != 0) {
		sc->sc_clkrate = 100000;
	}

	if (bus_space_map(sc->sc_iot, addr, size, 0, &sc->sc_ioh)) {
		aprint_error_dev(sc->sc_dev, "unable to map device\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Broadcom Serial Controller\n");

	mutex_init(&sc->sc_buslock, MUTEX_DEFAULT, IPL_NONE);

	/* clear FIFO, disable controller */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_C, BSC_C_CLEAR_CLEAR);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_S, BSC_S_CLKT |
	    BSC_S_ERR | BSC_S_DONE);

	u_int divider = howmany(sc->sc_frequency, sc->sc_clkrate);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_DIV,
	   __SHIFTIN(divider, BSC_DIV_CDIV));

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

	KERNHIST_LOG(bsciichist, "C %08jx S %08jx D %08jx A %08jx",
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
	uint32_t s __diagused;

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

	if (isread && cmdlen == 0)
		goto only_read;

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
	KERNHIST_LOG(bsciichist, "flood top %#jx %ju",
	    (uintptr_t)buf, len, 0, 0);
	j = 10000000;
	for (pos = 0; pos < len; ) {
		if (--j == 0)
			return -1;
		s = bus_space_read_4(sc->sc_iot, sc->sc_ioh, BSC_S);
		KERNHIST_LOG(bsciichist, "w s %08jx", s, 0, 0, 0);
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
		KERNHIST_LOG(bsciichist, "w %#jx %#jx %02jx",
		    (uintptr_t)buf, (uintptr_t)&buf[pos],
		    buf[pos], 0);
		pos++;
	}
	KERNHIST_LOG(bsciichist, "flood bot %#jx %ju",
	    (uintptr_t)buf, len, 0, 0);

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

	if (error == 0 && (s & (BSC_S_CLKT|BSC_S_ERR)) != 0)
		error = EIO;

	if (!isread)
		goto done;

only_read:
	c |= BSC_C_READ;

	buf = databuf;
	len = datalen;
	dlen = datalen;

	dlen = __SHIFTIN(dlen, BSC_DLEN_DLEN);
#if 0
	KASSERT(dlen >= 1);
#endif
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_DLEN, dlen);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_A, a);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_C, c | BSC_C_ST);

	do {
		s = bus_space_read_4(sc->sc_iot, sc->sc_ioh, BSC_S);
	} while ((s & BSC_S_TA) == 0);

	KERNHIST_LOG(bsciichist, "drain top %#jx %ju",
	    (uintptr_t)buf, len, 0, 0);
	j = 10000000;
	for (pos = 0; pos < len; ) {
		if (--j == 0)
			return -1;
		s = bus_space_read_4(sc->sc_iot, sc->sc_ioh, BSC_S);
		KERNHIST_LOG(bsciichist, "r s %08jx", s, 0, 0, 0);
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
		KERNHIST_LOG(bsciichist, "r %#jx %#jx %02jx",
		    (uintptr_t)buf, (uintptr_t)&buf[pos],
		    buf[pos], 0);
		pos++;
	}
	KERNHIST_LOG(bsciichist, "drain bot %#jx %ju", (uintptr_t)buf, len,
	    0, 0);

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

	if (error == 0 && (s & (BSC_S_CLKT|BSC_S_ERR)) != 0)
		error = EIO;

	return error;
}
