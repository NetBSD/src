/*	$NetBSD: jziic.c,v 1.1 2015/04/04 12:28:52 macallan Exp $ */

/*-
 * Copyright (c) 2015 Michael Lorenz
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
__KERNEL_RCSID(0, "$NetBSD: jziic.c,v 1.1 2015/04/04 12:28:52 macallan Exp $");

/*
 * a preliminary driver for JZ4780's on-chip SMBus controllers
 * - needs more error handling and interrupt support
 * - transfers can't be more than the chip's FIFO, supposedly 16 bytes per 
 *   direction
 * so, good enough for RTCs but not much else yet
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/mutex.h>
#include <sys/bus.h>
#include <sys/mutex.h>

#include <mips/ingenic/ingenic_var.h>
#include <mips/ingenic/ingenic_regs.h>

#include <dev/i2c/i2cvar.h>

#include "opt_ingenic.h"

#ifdef JZIIC_DEBUG
#define DPRINTF aprint_error
#else
#define DPRINTF while (0) printf
#endif
static int jziic_match(device_t, struct cfdata *, void *);
static void jziic_attach(device_t, device_t, void *);

struct jziic_softc {
	device_t 		sc_dev;
	bus_space_tag_t 	sc_memt;
	bus_space_handle_t 	sc_memh;
	struct i2c_controller 	sc_i2c;
	kmutex_t		sc_buslock;
	uint32_t		sc_pclk;
};

CFATTACH_DECL_NEW(jziic, sizeof(struct jziic_softc),
    jziic_match, jziic_attach, NULL, NULL);

static int jziic_enable(struct jziic_softc *);
static void jziic_disable(struct jziic_softc *);
static int jziic_i2c_acquire_bus(void *, int);
static void jziic_i2c_release_bus(void *, int);
static int jziic_i2c_exec(void *, i2c_op_t, i2c_addr_t, const void *, size_t,
		    void *, size_t, int);


/* ARGSUSED */
static int
jziic_match(device_t parent, struct cfdata *match, void *aux)
{
	struct apbus_attach_args *aa = aux;

	if (strcmp(aa->aa_name, "jziic") != 0)
		return 0;

	return 1;
}

/* ARGSUSED */
static void
jziic_attach(device_t parent, device_t self, void *aux)
{
	struct jziic_softc *sc = device_private(self);
	struct apbus_attach_args *aa = aux;
	struct i2cbus_attach_args iba;
	int error;
#ifdef JZIIC_DEBUG
	int i;
	uint8_t in[1] = {0}, out[16];
#endif

	sc->sc_dev = self;
	sc->sc_pclk = aa->aa_pclk;
	sc->sc_memt = aa->aa_bst;

	error = bus_space_map(aa->aa_bst, aa->aa_addr, 0x100, 0, &sc->sc_memh);
	if (error) {
		aprint_error_dev(self,
		    "can't map registers for %s: %d\n", aa->aa_name, error);
		return;
	}

	mutex_init(&sc->sc_buslock, MUTEX_DEFAULT, IPL_NONE);

	aprint_naive(": SMBus controller\n");
	aprint_normal(": SMBus controller\n");

#if notyet
	ih = evbmips_intr_establish(aa->aa_irq, ohci_intr, sc);

	if (ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		     aa->aa_irq);
		goto fail;
	}
#endif

#ifdef JZIIC_DEBUG
	if (jziic_i2c_exec(sc, I2C_OP_READ_WITH_STOP, 0x51, in, 1, out, 9, 0)
	    >= 0) {
		for (i = 0; i < 9; i++)
			printf(" %02x", out[i]);
		printf("\n");
		delay(1000000);
		jziic_i2c_exec(sc, I2C_OP_READ_WITH_STOP,
		    0x51, in, 1, out, 9, 0);
		for (i = 0; i < 9; i++)
			printf(" %02x", out[i]);
		printf("\n");
		delay(1000000);
		jziic_i2c_exec(sc, I2C_OP_READ_WITH_STOP,
		    0x51, in, 1, out, 9, 0);
		for (i = 0; i < 9; i++)
			printf(" %02x", out[i]);
		printf("\n");
	}
#endif

	/* fill in the i2c tag */
	sc->sc_i2c.ic_cookie = sc;
	sc->sc_i2c.ic_acquire_bus = jziic_i2c_acquire_bus;
	sc->sc_i2c.ic_release_bus = jziic_i2c_release_bus;
	sc->sc_i2c.ic_send_start = NULL;
	sc->sc_i2c.ic_send_stop = NULL;
	sc->sc_i2c.ic_initiate_xfer = NULL;
	sc->sc_i2c.ic_read_byte = NULL;
	sc->sc_i2c.ic_write_byte = NULL;
	sc->sc_i2c.ic_exec = jziic_i2c_exec;

	iba.iba_tag = &sc->sc_i2c;
	(void) config_found_ia(sc->sc_dev, "i2cbus", &iba, iicbus_print);


	return;

#if notyet
fail:
	if (ih) {
		evbmips_intr_disestablish(ih);
	}
	bus_space_unmap(sc->sc_memt, sc->sc_memh, 0x100);
#endif
}

static int
jziic_enable(struct jziic_softc *sc)
{
	int bail = 100000;
	uint32_t reg;

	bus_space_write_4(sc->sc_memt, sc->sc_memh, JZ_SMBENB, JZ_ENABLE);
	reg = bus_space_read_4(sc->sc_memt, sc->sc_memh, JZ_SMBENBST);
	DPRINTF("status: %02x\n", reg);
	while ((bail > 0) && (reg == 0)) {
		bail--;
		reg = bus_space_read_4(sc->sc_memt, sc->sc_memh, JZ_SMBENBST);
	}
	DPRINTF("bail: %d\n", bail);
	return (reg != 0);
}

static void
jziic_disable(struct jziic_softc *sc)
{
	int bail = 100000;
	uint32_t reg;

	bus_space_write_4(sc->sc_memt, sc->sc_memh, JZ_SMBENB, 0);
	reg = bus_space_read_4(sc->sc_memt, sc->sc_memh, JZ_SMBENBST);
	DPRINTF("status: %02x\n", reg);
	while ((bail > 0) && (reg != 0)) {
		bail--;
		reg = bus_space_read_4(sc->sc_memt, sc->sc_memh, JZ_SMBENBST);
	}
	DPRINTF("bail: %d\n", bail);
}

static int
jziic_i2c_acquire_bus(void *cookie, int flags)
{
	struct jziic_softc *sc = cookie;

	mutex_enter(&sc->sc_buslock);
	return 0;
}

static void
jziic_i2c_release_bus(void *cookie, int flags)
{
	struct jziic_softc *sc = cookie;

	mutex_exit(&sc->sc_buslock);
}

static int
jziic_wait(struct jziic_softc *sc)
{
	uint32_t reg;
	int bail = 10000;
	reg = bus_space_read_4(sc->sc_memt, sc->sc_memh, JZ_SMBST);
	while ((reg & JZ_MSTACT) && (bail > 0)) {
		delay(100);
		reg = bus_space_read_4(sc->sc_memt, sc->sc_memh, JZ_SMBST);
		bail--;
	}  
	return ((reg & JZ_MSTACT) == 0);
}

int
jziic_i2c_exec(void *cookie, i2c_op_t op, i2c_addr_t addr, const void *vcmd,
    size_t cmdlen, void *vbuf, size_t buflen, int flags)
{
	struct jziic_softc *sc = cookie;
	int ticks, hcnt, lcnt, hold, setup;
	int i, bail = 10000, ret = 0;
	uint32_t abort;
	uint8_t *rx, data;
	const uint8_t *tx;

	tx = vcmd;
	rx = vbuf;

	DPRINTF("%s: 0x%02x %d %d\n", __func__, addr, cmdlen, buflen);

	jziic_disable(sc);

	/* set speed and such */

	/* PCLK ticks per SMBus cycle */
	ticks = sc->sc_pclk / 100; /* assuming 100kHz for now */
	hcnt = (ticks * 40 / (40 + 47)) - 8;
	lcnt = (ticks * 47 / (40 + 47)) - 1;
	hold = sc->sc_pclk * 4 / 10000 - 1; /* ... * 400 / 1000000 ... */
	hold = max(1, hold);
	hold |= JZ_HDENB;
	setup = sc->sc_pclk * 3 / 10000 + 1; /* ... * 300 / 1000000 ... */
	DPRINTF("hcnt %d lcnt %d hold %d\n", hcnt, lcnt, hold);
	bus_space_write_4(sc->sc_memt, sc->sc_memh, JZ_SMBSHCNT, hcnt);
	bus_space_write_4(sc->sc_memt, sc->sc_memh, JZ_SMBSLCNT, lcnt);
	bus_space_write_4(sc->sc_memt, sc->sc_memh, JZ_SMBSDAHD, hold);
	bus_space_write_4(sc->sc_memt, sc->sc_memh, JZ_SMBSDASU, setup);
	bus_space_write_4(sc->sc_memt, sc->sc_memh, JZ_SMBCON,
	    JZ_SLVDIS | JZ_STPHLD | JZ_REST | JZ_SPD_100KB | JZ_MD);
	(void)bus_space_read_4(sc->sc_memt, sc->sc_memh, JZ_SMBCINT);
	jziic_wait(sc);
	/* try to talk... */

	if (!jziic_enable(sc)) {
		ret = -1;
		goto bork;
	}

	bus_space_write_4(sc->sc_memt, sc->sc_memh, JZ_SMBTAR, addr);
	jziic_wait(sc);
	DPRINTF("st: %02x\n",
	    bus_space_read_4(sc->sc_memt, sc->sc_memh, JZ_SMBST));
	DPRINTF("wr int: %02x\n",
	    bus_space_read_4(sc->sc_memt, sc->sc_memh, JZ_SMBINTST));
	abort = bus_space_read_4(sc->sc_memt, sc->sc_memh, JZ_SMBABTSRC);
	DPRINTF("abort: %02x\n", abort);
	if ((abort != 0)) {
		ret = -1;
		goto bork;
	}

	do {
		bail--;
		delay(100);
	} while (((bus_space_read_4(sc->sc_memt, sc->sc_memh, JZ_SMBST) &
	           JZ_TFE) == 0) && (bail > 0));

	if (cmdlen != 0) {
		for (i = 0; i < cmdlen; i++) {
			bus_space_write_4(sc->sc_memt, sc->sc_memh,
			    JZ_SMBDC, *tx);
			tx++;
		}
	}

	if (I2C_OP_READ_P(op)) {
		/* now read */
		for (i = 0; i < (buflen + 1); i++) {
			bus_space_write_4(sc->sc_memt, sc->sc_memh,
			    JZ_SMBDC, JZ_CMD);
		}
		wbflush();
		DPRINTF("rd st: %02x\n",
		    bus_space_read_4(sc->sc_memt, sc->sc_memh, JZ_SMBST));
		DPRINTF("rd int: %02x\n",
		    bus_space_read_4(sc->sc_memt, sc->sc_memh, JZ_SMBINTST));
		DPRINTF("abort: %02x\n",
		    bus_space_read_4(sc->sc_memt, sc->sc_memh, JZ_SMBABTSRC));
		for (i = 0; i < buflen; i++) {
			bail = 10000;
			while (((bus_space_read_4(sc->sc_memt, sc->sc_memh,
				  JZ_SMBST) & JZ_RFNE) == 0) && (bail > 0)) {
				bail--;
				delay(100);
			} 
			if (bail == 0) {
				ret = -1;
				goto bork;
			}
			data = bus_space_read_4(sc->sc_memt, sc->sc_memh,
			    JZ_SMBDC);
			DPRINTF("rd st: %02x %d\n",
			  bus_space_read_4(sc->sc_memt, sc->sc_memh, JZ_SMBST),
			  bail);
			DPRINTF("rd int: %02x\n",
			  bus_space_read_4(sc->sc_memt, sc->sc_memh,
			   JZ_SMBINTST));
			DPRINTF("abort: %02x\n", abort);
			DPRINTF("rd data: %02x\n", data);
			*rx = data;
			rx++;
		}
	} else {
		tx = vbuf;
		for (i = 0; i < buflen; i++) {
			DPRINTF("wr data: %02x\n", *tx);
			bus_space_write_4(sc->sc_memt, sc->sc_memh,
			    JZ_SMBDC, *tx);
			wbflush();
			tx++;
		}
		jziic_wait(sc);
		abort = bus_space_read_4(sc->sc_memt, sc->sc_memh,
		    JZ_SMBABTSRC);
		DPRINTF("abort: %02x\n", abort);
		if ((abort != 0)) {
			ret = -1;
			goto bork;
		}

		DPRINTF("st: %02x %d\n",
		    bus_space_read_4(sc->sc_memt, sc->sc_memh, JZ_SMBST), bail);
		DPRINTF("wr int: %02x\n",
		    bus_space_read_4(sc->sc_memt, sc->sc_memh, JZ_SMBINTST));
	}
	bus_space_write_4(sc->sc_memt, sc->sc_memh, JZ_SMBCON,
	    JZ_SLVDIS | JZ_REST | JZ_SPD_100KB | JZ_MD);
bork:
	jziic_disable(sc);
	return ret;
}
