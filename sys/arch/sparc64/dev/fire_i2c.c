/* $NetBSD: fire_i2c.c,v 1.3 2026/06/05 09:08:55 jdc Exp $ */

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman.
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
__KERNEL_RCSID(0, "$NetBSD: fire_i2c.c,v 1.3 2026/06/05 09:08:55 jdc Exp $");

#include <sys/param.h>
#include <sys/condvar.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/mutex.h>

#include <machine/autoconf.h>
#include <machine/openfirm.h>

#include <dev/i2c/i2cvar.h>
#include <arch/sparc64/dev/fire_i2creg.h>

#define FIREI2C_DEBUG 0
#if FIREI2C_DEBUG > 0
#define DPRINTF printf
#else
#define DPRINTF if (0) printf
#endif

#define FIREI2C_SCL_FREQ	100000
#define FIREI2C_OWN_ADDR	0x08

#define FIREI2C_CLK_FULL	0
#define FIREI2C_CLK_HALF	1
#define FIREI2C_CLK_TT		2
#define FIREI2C_CLK_LEN		3

struct firei2c_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bustag;
	bus_space_handle_t sc_regh;
	int sc_node;
	void *sc_inth;
	uint8_t sc_i2c_clks[FIREI2C_CLK_LEN];
	struct i2c_controller sc_i2c;
	kmutex_t sc_mutex;
	kcondvar_t sc_cv;
};

static int firei2c_match(device_t, cfdata_t, void *);
static void firei2c_attach(device_t, device_t, void *);
static uint8_t firei2c_clock(int);
static int firei2c_stop(struct firei2c_softc *);
static int firei2c_wait(struct firei2c_softc *, int);
int firei2c_exec(void *, i2c_op_t, i2c_addr_t, const void *,
    size_t, void *, size_t, int);
static int firei2c_write(struct firei2c_softc *, const uint8_t *, size_t,
    uint8_t *, size_t, int, int);
static int firei2c_read(struct firei2c_softc *, uint8_t *, size_t, int, int);
int firei2c_intr(void *);
static uint8_t firei2c_reg_read(struct firei2c_softc *, bus_size_t);
static void firei2c_reg_write(struct firei2c_softc *, bus_size_t, uint8_t);

CFATTACH_DECL_NEW(firei2c, sizeof(struct firei2c_softc),
	firei2c_match, firei2c_attach, NULL, NULL);

static int
firei2c_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;
	char *compat;

	if (strcmp(ma->ma_name, "i2c"))
		return 0;

	compat = prom_getpropstring(ma->ma_node, "compatible");
	if (!strcmp(compat, "fire-i2c"))
		return 1;

	return 0;
}

static void
firei2c_attach(device_t parent, device_t self, void *aux)
{
	struct firei2c_softc *sc = device_private(self);
	struct mainbus_attach_args *ma = aux;
	int  sysclk;

	sc->sc_bustag = ma->ma_bustag;
	sc->sc_node = ma->ma_node;
	sc->sc_dev = self;

	if (ma->ma_nreg != 1) {
		aprint_error(": register count error (%d != 1)\n",
		    ma->ma_nreg);
		return;
	}
	if (ma->ma_reg[0].ur_len < FIREI2C_SRST) {
		aprint_error(": register length error (%lld < %d\n",
		    (long long int) ma->ma_reg[0].ur_len, FIREI2C_SRST);
		return;
	}

	if (bus_space_map(sc->sc_bustag, ma->ma_reg[0].ur_paddr,
	    ma->ma_reg[0].ur_len, 0, &sc->sc_regh)) {
		aprint_error(": failed to map registers\n");
		return;
	}

	sc->sc_inth = NULL;
/* XXX need to set up interrupt mapping before we can enable them here
   Both i2c interrupts must go to the same Pyro leaf.

	sc->sc_inth = bus_intr_establish(sc->sc_bustag, *ma->ma_interrupts,
	    IPL_VM, firei2c_intr, sc);

	if (sc->sc_inth == NULL) {
		aprint_error(": failed to establish interrupt\n");
		return;
	}
*/

	aprint_normal(": addr %" PRIx64 ": Fire/MI2C i2c controller\n",
	    ma->ma_reg[0].ur_paddr);
/* XXX interrupts
	aprint_normal_dev(self, "interrupting at %x\n", *ma->ma_interrupts);
*/

	/* Calculate clock, software reset, set our addr, enable interrupts */
	sysclk = prom_getpropint(findroot(), "clock-frequency", 0);
	sc->sc_i2c_clks[FIREI2C_CLK_FULL] = firei2c_clock(sysclk);
	sc->sc_i2c_clks[FIREI2C_CLK_HALF] = firei2c_clock(sysclk / 2);
	sc->sc_i2c_clks[FIREI2C_CLK_TT] = firei2c_clock(sysclk / 32);
	firei2c_reg_write(sc, FIREI2C_SRST, FIREI2C_SRST_RST);
	delay(1000);
	firei2c_reg_write(sc, FIREI2C_CCR, sc->sc_i2c_clks[FIREI2C_CLK_FULL]);
	firei2c_reg_write(sc, FIREI2C_ADDR,
	    FIREI2C_OWN_ADDR << FIREI2C_ADDR_SHIFT);

	/* i2c setup */
	iic_tag_init(&sc->sc_i2c);
	sc->sc_i2c.ic_cookie = sc;
	sc->sc_i2c.ic_exec = firei2c_exec;

	/* Synchronisation between exec and intr */
	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, IPL_VM);
	cv_init(&sc->sc_cv, "firei2c");

	iicbus_attach(sc->sc_dev, &sc->sc_i2c);
	return;
}

/*
 * Sampling frequency: input clock divided by 2^N (N is bits 0:2)
 * SCL frequency: sampling frequency divided by (M + 1) * 10 (bits 3:6)
 * i2c SCL is max 100kHz, so find the closest frequency at or below that.
 *
 * If the system (JBus) clock frequency changes, we must change our clock.
 * If increasing: after the JBus change, if decreasing: before the change.
 * This is to avoid running the i2c clock faster than the specification.
 */
static uint8_t
firei2c_clock(int sysclk)
{
	int p, f, best_p, best_f;
	uint32_t sample, scl;
	uint32_t min = FIREI2C_SCL_FREQ;
	uint8_t clkval;

	best_p = 0;
	best_f = 0;
	for (p = 0; p <= FIREI2C_CCR_POW2_MAX; p++) {
		if (p)
			sample = sysclk / (2 << (p - 1));
		else
			sample = sysclk;
		for (f = 0; f <= FIREI2C_CCR_FACT_MAX; f++) {
			scl = sample / ((f + 1) * 10);
			if (scl <= FIREI2C_SCL_FREQ) {
				if (FIREI2C_SCL_FREQ - scl < min) {
					best_p = p;
					best_f = f;
					min = FIREI2C_SCL_FREQ - scl;
				}
				break;	/* Already at or below SCL_FREQ */
			}
		}
		if (!min)	/* Exactly SCL_FREQ */
			break;
	}
	clkval = best_p + (best_f << FIREI2C_CCR_FACT_SHIFT);
	return clkval;
}

/* No interrupts for stop, so wait for idle */
static int
firei2c_stop(struct firei2c_softc *sc)
{
	int i;
	uint8_t ctrl, val;

	ctrl = FIREI2C_CTRL_ENAB | FIREI2C_CTRL_STP;
	firei2c_reg_write(sc, FIREI2C_CTRL, ctrl);
	for (i = 0; i < 500; i++) {
		val = firei2c_reg_read(sc, FIREI2C_STAT);
		if (val == FIREI2C_STAT_IDLE)
			return 0;
		delay(1000);
	}
	/* Clear the interrupt flag if we timed out */
	ctrl = FIREI2C_CTRL_ENAB;
	firei2c_reg_write(sc, FIREI2C_CTRL, ctrl);
	return 1;
}


static int
firei2c_wait(struct firei2c_softc *sc, int flags)
{
	volatile uint8_t ctrl;
	int i;

	if (sc->sc_inth == NULL || flags & I2C_F_POLL) {
		for (i = 0; i < 500; i++) {
			ctrl = firei2c_reg_read(sc, FIREI2C_CTRL);
			if (ctrl & FIREI2C_CTRL_IFLG)
				return 0;
			delay(1000);
		}
		return 1;
	} else {
		mutex_enter(&sc->sc_mutex);
		if (cv_timedwait(&sc->sc_cv, &sc->sc_mutex, hz / 2)) {
			mutex_exit(&sc->sc_mutex);
			return 1;
		}
		mutex_exit(&sc->sc_mutex);
		return 0;
	}
}

/*
 * Flow for a "simple" read/write:
 *   1.  Send start
 *   2.  Send addr + R or W
 *   3.  Receive ACK
 *   4.  (nothing)
 *   5.  (nothing)
 *   6R. Receive data			6W.  Send data
 *   7R. Send ACK or NAK		7W.  Receive ACK or NAK
 *       either receive more (6R) or	     either write more (6W) or
 *   8.  Send stop
 * 
 * Flow for read/write following a "register" write:
 *   1.  Send start
 *   2.  Send addr + W
 *   3.  Receive ACK
 *   4.  Send register
 *   5R. Send repeat start + addr + R	5W.  (nothing to do)
 *   6R. Receive data			6W.  Send data
 *   7R. Send ACK or NAK		7W.  Receive ACK or NAK
 *       either receive more (6R) or	     either write more (6W) or
 *   8.  Send stop
 *
 * After each step, we receive an interrupt notifying us of the result.
 * We check the status, set up the next step, then clear the interrupt flag.
 */
int
firei2c_exec(void *arg, i2c_op_t op, i2c_addr_t addr, const void *cmd,
    size_t cmdlen, void *vbuf, size_t buflen, int flags)
{
	struct firei2c_softc *sc = arg;
	uint8_t *buf = vbuf;
	uint8_t ctrl, val, ack, nack;
	int err;

	DPRINTF("%s: exec op: %d addr: 0x%x cmdlen: %d buflen: %d flags 0x%x\n",
	    device_xname(sc->sc_dev), op, addr,
	    (int) cmdlen, (int) buflen, flags);

	/*
	 * Send start
 	 * If we are writing, write addr, cmd, buf.
 	 * If we are reading, either:
 	 *   write addr, cmd, repeat-start, then read addr to buf, or:
 	 *   read addr to buf.
	 * Send stop
 	 */

	/* Control defaults - bus enabled, interrupts enabled */
	ctrl = FIREI2C_CTRL_ENAB;
	if (sc->sc_inth != NULL && !(flags & I2C_F_POLL))
		ctrl |= FIREI2C_CTRL_IEN;

	val = firei2c_reg_read(sc, FIREI2C_STAT);
	if (val != FIREI2C_STAT_IDLE) {
		/* Try sending stop */
		DPRINTF("%s: not idle, sending stop\n",
		    device_xname(sc->sc_dev));
		err = firei2c_stop(sc);
		if (err) {
			printf("%s: not idle (0x%02x)\n",
			    device_xname(sc->sc_dev), val);
			return 1;
		}
	}

	/* Send start */
	val = ctrl | FIREI2C_CTRL_STA;
	firei2c_reg_write(sc, FIREI2C_CTRL, val);
	err = firei2c_wait(sc, flags);
	val = firei2c_reg_read(sc, FIREI2C_STAT);
	if (err) {
		printf("%s: start timeout 0x%x\n",
		    device_xname(sc->sc_dev), val);
		goto stop;
	}
	if (val != FIREI2C_STAT_STA) {
		printf("%s: start error 0x%x\n",
		    device_xname(sc->sc_dev), val);
		err = 1;
		goto stop;
	}

	/* Address + r/w in data, then clear interrupt flag in ctrl */
	val = (addr & 0x7f) << FIREI2C_DATA_SHIFT;
	if (I2C_OP_WRITE_P(op) || cmdlen > 0) {
		ack = FIREI2C_STAT_AWR_ACK;
		nack = FIREI2C_STAT_AWR_NAK;
	} else {
		val |= 0x01;
		ack = FIREI2C_STAT_ARE_ACK;
		nack = FIREI2C_STAT_ARE_NAK;
	}
	firei2c_reg_write(sc, FIREI2C_DATA, val);
	firei2c_reg_write(sc, FIREI2C_CTRL, ctrl);
	err = firei2c_wait(sc, flags);
	val = firei2c_reg_read(sc, FIREI2C_STAT);
	if (err) {
		printf("%s: addr timeout 0x%x\n",
		    device_xname(sc->sc_dev), val);
		goto stop;
	}
	if (val != ack) {
		if (val != nack)	/* Don't print for NACK */
			printf("%s: addr error 0x%x\n",
			    device_xname(sc->sc_dev), val);
		err = 1;
		goto stop;
	}

	err = 0;
	if (I2C_OP_READ_P(op)) {
		if (cmdlen > 0) {
			/* Write register */
			err = firei2c_write(sc, cmd, cmdlen,
			    NULL, 0, ctrl, flags);
			if (err)
				goto stop;

			/* Send repeat start */
			val = ctrl | FIREI2C_CTRL_STA;
			firei2c_reg_write(sc, FIREI2C_CTRL, val);
			err = firei2c_wait(sc, flags);
			val = firei2c_reg_read(sc, FIREI2C_STAT);
			if (err) {
				printf("%s: repeat start timeout 0x%x\n",
				    device_xname(sc->sc_dev), val);
				goto stop;
			}
			if (val != FIREI2C_STAT_REPSTA) {
				printf("%s: repeat start error 0x%x\n",
				    device_xname(sc->sc_dev), val);
				err = 1;
				goto stop;
			}

			/* Address + r in data, then clear intr flag in ctrl */
			val = (addr & 0x7f) << FIREI2C_DATA_SHIFT;
			val |= 0x01;
			ack = FIREI2C_STAT_ARE_ACK;
			nack = FIREI2C_STAT_ARE_NAK;
			firei2c_reg_write(sc, FIREI2C_DATA, val);
			firei2c_reg_write(sc, FIREI2C_CTRL, ctrl);
			err = firei2c_wait(sc, flags);
			val = firei2c_reg_read(sc, FIREI2C_STAT);
			if (err) {
				printf("%s: addr timeout 0x%x\n",
				    device_xname(sc->sc_dev), val);
				goto stop;
			}
			if (val != ack) {
				if (val != nack)
					printf("%s: addr error 0x%x\n",
					    device_xname(sc->sc_dev), val);
				err = 1;
				goto stop;
			}
		}

		/* Read data */
		err = firei2c_read(sc, buf, buflen, ctrl, flags);
	} else
		/* Write data or register + data */
		err = firei2c_write(sc, cmd, cmdlen, buf, buflen, ctrl, flags);

stop:
	err |= firei2c_stop(sc);

	return err;
}

static int
firei2c_write(struct firei2c_softc *sc, const uint8_t *cmd, size_t cmdlen,
    uint8_t *buf, size_t buflen, int ctrl, int flags)
{
	uint8_t val;
	int i, err;

	for (i = 0; i < cmdlen + buflen; i++) {
		/* Bytes in data, then clear interrupt flag in ctrl */
		if (i < cmdlen)
			val = cmd[i];
		else
			val = buf[i - cmdlen];
		firei2c_reg_write(sc, FIREI2C_DATA, val);
		firei2c_reg_write(sc, FIREI2C_CTRL, ctrl);
		err = firei2c_wait(sc, flags);
		val = firei2c_reg_read(sc, FIREI2C_STAT);
		if (err) {
			printf("%s: write timeout 0x%x\n",
			    device_xname(sc->sc_dev), val);
			return 1;
		}
		if (val != FIREI2C_STAT_DAT_ACK) {
			if (val != FIREI2C_STAT_DAT_NAK)
				printf("%s: write error 0x%x\n",
				    device_xname(sc->sc_dev), val);
			return 1;
		}

	}
	return 0;
}

static int
firei2c_read(struct firei2c_softc *sc, uint8_t *buf, size_t buflen,
    int ctrl, int flags)
{
	uint8_t val, ack;
	int i, err;

	for (i = 0; i < buflen; i++) {
		/* Send ACK on all but last byte */
		if (i < buflen - 1) {
			val = ctrl | FIREI2C_CTRL_AAK;
			ack = FIREI2C_STAT_MDAT_ACK;
		} else {
			val = ctrl;
			ack = FIREI2C_STAT_MDAT_NAK;
		}
		/* Clear interrupt flag in ctrl, bytes in data */
		firei2c_reg_write(sc, FIREI2C_CTRL, val);
		err = firei2c_wait(sc, flags);
		val = firei2c_reg_read(sc, FIREI2C_STAT);
		if (err) {
			printf("%s: read timeout 0x%x\n",
			    device_xname(sc->sc_dev), val);
			return 1;
		}
		if (val != ack) {
			printf("%s: read error 0x%x\n",
			    device_xname(sc->sc_dev), val);
			return 1;
		}
		buf[i] = firei2c_reg_read(sc, FIREI2C_DATA);
	}
	return 0;
}

int
firei2c_intr(void *arg)
{
	struct firei2c_softc *sc = arg;
	uint8_t ctrl;

	mutex_enter(&sc->sc_mutex);
	ctrl = firei2c_reg_read(sc, FIREI2C_CTRL);
	if (ctrl & FIREI2C_CTRL_IFLG) {
		/* Disable interrupts */
		firei2c_reg_write(sc, FIREI2C_CTRL, ctrl & ~FIREI2C_CTRL_IEN);
		cv_signal(&sc->sc_cv);
		mutex_exit(&sc->sc_mutex);
		return 1;	/* Handled */
	}
	mutex_exit(&sc->sc_mutex);
	return 0;
}

static uint8_t
firei2c_reg_read(struct firei2c_softc *sc, bus_size_t reg)
{
	uint32_t val;
	bus_space_barrier(sc->sc_bustag, sc->sc_regh, reg, 8,
	    BUS_SPACE_BARRIER_READ);
	val = bus_space_read_8(sc->sc_bustag, sc->sc_regh, reg);
	return val & 0xff;
}

static void
firei2c_reg_write(struct firei2c_softc *sc, bus_size_t reg, uint8_t val)
{
	bus_space_write_8(sc->sc_bustag, sc->sc_regh, reg, val);
	bus_space_barrier(sc->sc_bustag, sc->sc_regh, reg, 8,
	    BUS_SPACE_BARRIER_WRITE);
}
