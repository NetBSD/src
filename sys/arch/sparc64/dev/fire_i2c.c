/* $NetBSD: fire_i2c.c,v 1.5 2026/09/25 07:16:27 jdc Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: fire_i2c.c,v 1.5 2026/09/25 07:16:27 jdc Exp $");

#include <sys/param.h>
#include <sys/condvar.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/mutex.h>

#include <machine/autoconf.h>
#include <machine/openfirm.h>

#include <dev/pci/pcivar.h>

#include <dev/i2c/i2cvar.h>
#include <sparc64/dev/fire_i2creg.h>
#include <sparc64/dev/iommureg.h>
#include <sparc64/dev/iommuvar.h>
#include <sparc64/dev/pyrovar.h>

#include "ioconf.h"

#define FIREI2C_DEBUG 0
#if FIREI2C_DEBUG > 0
int dlvl = 1;		/* Set to 2 for verbose i2c ops output */
#define DPRINTF(n) if (dlvl >= n) printf
#else
#define DPRINTF(n) if (0) printf
#endif

#define FIREI2C_SCL_FREQ	100000	/* Target i2c clock frequency */
#define FIREI2C_OWN_ADDR	0x08	/* Our i2c slave address */

/* i2c clocks to match possible system clock frequencies (1/1, 1/2, 1/32) */
#define FIREI2C_CLK_FULL	0
#define FIREI2C_CLK_HALF	1
#define FIREI2C_CLK_TT		2
#define FIREI2C_CLK_LEN		3

/* i2c operations */
#define FIREI2C_OP_READ		0x01
#define FIREI2C_OP_WRITE	0x02
#define FIREI2C_OP_WR_RD	0x04

/* i2c stages */
#define FIREI2C_STAGE_IDLE	0x00
#define FIREI2C_STAGE_START	0x01
#define FIREI2C_STAGE_WR_ADDR	0x02
#define FIREI2C_STAGE_RE_ADDR	0x04
#define FIREI2C_STAGE_WRITE	0x08
#define FIREI2C_STAGE_REPSTART	0x10
#define FIREI2C_STAGE_READ	0x20
#define FIREI2C_STAGE_STOP	0x40

/* mainbus intr map/mask are only used for i2c interrupts, so put them here. */
struct mainbus_interrupt_map {
	uint64_t paddr;		/* phys addr mask */
	uint32_t intr;		/* interrupt mask */
	int32_t cnode;		/* child node */
	uint32_t cintr;		/* child interrupt */
} __packed;

struct mainbus_interrupt_map_mask {
	uint64_t paddr;		/* phys addr mask */
	uint32_t intr;		/* interrupt */
} __packed;

struct firei2c_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bustag;
	bus_space_handle_t sc_regh;
	int sc_node;
	void *sc_inth;

	uint8_t sc_i2c_clks[FIREI2C_CLK_LEN];	/* Clocks for sys bus speeds */

	struct i2c_controller sc_i2c;

	kmutex_t sc_mutex;		/* Interrupt mutex ... */
	kcondvar_t sc_cv;		/* ... and condvar */

	int sc_op;			/* Operation to run */
	int sc_stage;			/* Exec stage */
	i2c_addr_t sc_target;		/* Saved exec args */
	const u_int8_t *sc_cmdbuf;	/* ... */
	size_t sc_cmdlen;		/* ... */
	u_int8_t *sc_buf;		/* ... */
	size_t sc_buflen;		/* ... */
	int sc_i;			/* How far through buf */
	int sc_err;			/* Error during transaction */

};

static int firei2c_match(device_t, cfdata_t, void *);
static void firei2c_attach(device_t, device_t, void *);
static void firei2c_intr_establish(struct firei2c_softc *,
    struct mainbus_attach_args *);
static uint8_t firei2c_clock(int);
static int firei2c_stop(struct firei2c_softc *);
static int firei2c_wait(struct firei2c_softc *, int);
int firei2c_exec(void *, i2c_op_t, i2c_addr_t, const void *,
    size_t, void *, size_t, int);
static int firei2c_write(struct firei2c_softc *, const uint8_t *, size_t,
    uint8_t *, size_t, int);
static int firei2c_read(struct firei2c_softc *, uint8_t *, size_t, int);
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
		aprint_error(": register length error (%" PRId64 " < %d\n",
		    ma->ma_reg[0].ur_len, FIREI2C_SRST);
		return;
	}

	if (bus_space_map(sc->sc_bustag, ma->ma_reg[0].ur_paddr,
	    ma->ma_reg[0].ur_len, 0, &sc->sc_regh)) {
		aprint_error(": failed to map registers\n");
		return;
	}

	aprint_normal(": addr %" PRIx64 ": Fire/MI2C i2c controller\n",
	    ma->ma_reg[0].ur_paddr);

	firei2c_intr_establish(sc, ma);

	/* Calculate clock, software reset, set our address */
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
	sc->sc_stage = FIREI2C_STAGE_IDLE;

	iicbus_attach(sc->sc_dev, &sc->sc_i2c);
	return;
}

/*
 * Use the mainbus (root) interrupt map/mask to find our interrupt number
 * (ino 60 or 61) and pyro leaf.
 * Both i2c interrupts must go to the same pyro leaf, but we assume that
 * OFW has set up the ino and the leaf mapping for us.
 */
#define IMASK_LEN 3
#define IMAP_LEN 10
static void
firei2c_intr_establish(struct firei2c_softc *sc,
    struct mainbus_attach_args *ma)
{
	struct mainbus_interrupt_map *mb_imap;
	struct mainbus_interrupt_map_mask *mb_imask;
	struct pyro_softc *psc = NULL;
	int64_t paddr;
	u_int intr;
	int ihandle, root, size, i;

	sc->sc_inth = NULL;	/* Default */
	ihandle = 0;

	/* Find our mapped interrupt number */
	root = findroot();
	mb_imask = NULL;
	if (prom_getprop(root, "interrupt-map-mask",
	    sizeof(int32_t), &size, (void **) &mb_imask) ||
	    size != IMASK_LEN) {
		aprint_error(": no interrupt-map-mask\n");
		return;
	}

	mb_imap = NULL;
	if (prom_getprop(root, "interrupt-map",
	    sizeof(int32_t), &size, (void **) &mb_imap) ||
	    size != IMAP_LEN) {
		aprint_error(": no interrupt-map\n");
		return;
	}

	intr = ma->ma_interrupts[0] && mb_imask->intr;
	DPRINTF(1)("%s: intr %x masked to %x\n",
	    device_xname(sc->sc_dev), ma->ma_interrupts[0], intr);
	paddr = ma->ma_reg[0].ur_paddr & mb_imask->paddr;
	DPRINTF(1)("%s: reg 0x%" PRIx64 " masked to 0x%" PRIx64 "\n",
	    device_xname(sc->sc_dev), ma->ma_reg[0].ur_paddr, paddr);
	for (i = 0; i < 2; i++) {
		DPRINTF(1)("%s: checking paddr 0x%" PRIx64 ", intr %x\n",
		    device_xname(sc->sc_dev), mb_imap->paddr, mb_imap->intr);
		if (paddr == mb_imap->paddr && intr == mb_imap->intr) {
			ihandle = mb_imap->cintr;
			DPRINTF(1)("%s: found cintr %d, cnode 0x%x\n",
			    device_xname(sc->sc_dev), ihandle, mb_imap->cnode);
			break;
		}
		mb_imap++;
	}
	if (ihandle == 0) {
		aprint_error(": failed to match interrupt\n");
		return;
	}

	/* Find the pyro leaf with the matching cnode ) */
	for (i = 0; i < pyro_cd.cd_ndevs; i++) {
		device_t dt = device_lookup(&pyro_cd, i);
		psc = device_private(dt);
		if (psc && psc->sc_node == mb_imap->cnode) {
			DPRINTF(1)("%s: matched pyro %d (0x%x)\n",
			    device_xname(sc->sc_dev), i, psc->sc_node);
			ihandle |= psc->sc_ign;
			break;
		}
	}
	if (psc == NULL) {
		aprint_error(": failed to match pyro leaf\n");
		return;
	}

	sc->sc_inth = psc->intr_establish_sc(psc,
	    ihandle, IPL_BIO, firei2c_intr, sc, NULL);

	if (sc->sc_inth != NULL) {
		aprint_normal_dev(sc->sc_dev,
		    "interrupting at ivec %x on pyro%d\n", ihandle, i);
	}

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
		if (val == FIREI2C_STAT_IDLE) {
			mutex_enter(&sc->sc_mutex);
			sc->sc_stage = FIREI2C_STAGE_IDLE;
			mutex_exit(&sc->sc_mutex);
			return 0;
		}
		delay(1000);
	}
	/* Clear the interrupt flag if we timed out */
	ctrl = FIREI2C_CTRL_ENAB;
	firei2c_reg_write(sc, FIREI2C_CTRL, ctrl);
	/* Set the stage to idle, even for timeout */
	mutex_enter(&sc->sc_mutex);
	sc->sc_stage = FIREI2C_STAGE_IDLE;
	mutex_exit(&sc->sc_mutex);
	return 1;
}


static int
firei2c_wait(struct firei2c_softc *sc, int flags)
{
	volatile uint8_t ctrl;
	int i;

	for (i = 0; i < 500; i++) {
		ctrl = firei2c_reg_read(sc, FIREI2C_CTRL);
		if (ctrl & FIREI2C_CTRL_IFLG)
			return 0;
		delay(1000);
	}
	return 1;
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
 * We clear the interrupt flag, check the status, and set up the next step.
 * If we're polling, we check the flag in a loop instead.
 */
int
firei2c_exec(void *arg, i2c_op_t op, i2c_addr_t addr, const void *cmd,
    size_t cmdlen, void *vbuf, size_t buflen, int flags)
{
	struct firei2c_softc *sc = arg;
	const uint8_t *cmdbuf = cmd;
	uint8_t *buf = vbuf;
	uint8_t ctrl, val, ack, nack;
	unsigned deadline, rem;
	int err;

	DPRINTF(1)("%s: exec op: %d addr: 0x%x "
	    "cmdlen: %d buflen: %d flags 0x%x, intr %s, poll %s\n",
	    device_xname(sc->sc_dev), op, addr,
	    (int) cmdlen, (int) buflen, flags,
	    sc->sc_inth == NULL ? "no" : "yes",
	    flags & I2C_F_POLL ? "yes" : "no");
	val = firei2c_reg_read(sc, FIREI2C_STAT);
	if (val != FIREI2C_STAT_IDLE) {
		/* Try sending stop */
		DPRINTF(1)("%s: not idle, sending stop\n",
		    device_xname(sc->sc_dev));
		err = firei2c_stop(sc);
		if (err) {
			printf("%s: not idle (0x%02x)\n",
			    device_xname(sc->sc_dev), val);
			return 1;
		}
	}

	/* Control defaults - bus enabled, interrupts enabled */
	ctrl = FIREI2C_CTRL_ENAB;
	if (sc->sc_inth != NULL && !(flags & I2C_F_POLL)) {
		ctrl |= FIREI2C_CTRL_IEN;
		sc->sc_target = addr;
		sc->sc_cmdbuf = cmdbuf;
		sc->sc_cmdlen = cmdlen;
		sc->sc_buf = buf;
		sc->sc_buflen = buflen;
		sc->sc_i = 0;
		sc->sc_err = 0;
		if (I2C_OP_READ_P(op))
			if (cmdlen > 0)
				sc->sc_op = FIREI2C_OP_WR_RD;
			else
				sc->sc_op = FIREI2C_OP_READ;
		else
			sc->sc_op = FIREI2C_OP_WRITE;
		mutex_enter(&sc->sc_mutex);
		sc->sc_stage = FIREI2C_STAGE_START;
		mutex_exit(&sc->sc_mutex);
	}

	/* Send start */
	firei2c_reg_write(sc, FIREI2C_CTRL, ctrl | FIREI2C_CTRL_STA);

	/* Interrupt-driven */
	if (sc->sc_inth != NULL && !(flags & I2C_F_POLL)) {
		/* Wait for the transfer to complete (stop). */
		deadline = getticks() +
		    /*timeout*/ hz / 10 + 10 * (cmdlen + buflen);
		mutex_enter(&sc->sc_mutex);
		while (sc->sc_stage != FIREI2C_STAGE_STOP) {
			rem = deadline - getticks();
			if (!rem || rem >= INT_MAX) {
				printf("%s: intr timeout\n",
				    device_xname(sc->sc_dev));
				sc->sc_stage = FIREI2C_STAGE_STOP;
				err = sc->sc_err | 1;
				break;
			}
			cv_timedwait(&sc->sc_cv, &sc->sc_mutex, rem);
		}
		mutex_exit(&sc->sc_mutex);
		err = sc->sc_err;
		goto stop;
	}

	/*
	 * If we are writing, write addr, cmd, buf.
	 * If we are reading, either:
	 *   write addr, cmd, repeat-start, then read addr to buf, or:
	 *   read addr to buf.
	 * Send stop
	 */
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

	/* Address + r/w in data, then send ctrl */
	val = addr << FIREI2C_DATA_SHIFT;
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
			    NULL, 0, flags);
			if (err)
				goto stop;

			/* Send repeat start */
			firei2c_reg_write(sc, FIREI2C_CTRL,
			    ctrl | FIREI2C_CTRL_STA);
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

			/* Address + r in data, then send ctrl */
			val = addr << FIREI2C_DATA_SHIFT;
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
		err = firei2c_read(sc, buf, buflen, flags);
	} else
		/* Write data or register + data */
		err = firei2c_write(sc, cmd, cmdlen, buf, buflen, flags);

stop:
	err |= firei2c_stop(sc);

	return err;
}

static int
firei2c_write(struct firei2c_softc *sc, const uint8_t *cmd, size_t cmdlen,
    uint8_t *buf, size_t buflen, int flags)
{
	uint8_t ctrl, val;
	int i, err;

	DPRINTF(2)("%s: write %ld\n",
	    device_xname(sc->sc_dev), cmdlen + buflen);
	ctrl = FIREI2C_CTRL_ENAB;
	for (i = 0; i < cmdlen + buflen; i++) {
		/* Bytes in data, then send ctrl */
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
firei2c_read(struct firei2c_softc *sc, uint8_t *buf, size_t buflen, int flags)
{
	uint8_t ctrl, val, ack;
	int i, err;

	DPRINTF(2)("%s: read %ld\n", device_xname(sc->sc_dev), buflen);
	for (i = 0; i < buflen; i++) {
		/* Send ACK on all but last byte */
		if (i < buflen - 1) {
			ctrl = FIREI2C_CTRL_ENAB | FIREI2C_CTRL_AAK;
			ack = FIREI2C_STAT_MDAT_ACK;
		} else {
			ctrl = FIREI2C_CTRL_ENAB;
			ack = FIREI2C_STAT_MDAT_NAK;
		}
		/* Send ctrl, then bytes in data */
		firei2c_reg_write(sc, FIREI2C_CTRL, ctrl);
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

/*
 * When we receive an interrupt, check the status versus the step
 * and set up the next stage.
 * See the comments before firei2c_exec() for the list of steps.
 */
int
firei2c_intr(void *arg)
{
	struct firei2c_softc *sc = arg;
	uint8_t ctrl, val;

	mutex_enter(&sc->sc_mutex);
	ctrl = firei2c_reg_read(sc, FIREI2C_CTRL);
	if (ctrl & FIREI2C_CTRL_IFLG) {
		/* Disable interrupts */
		firei2c_reg_write(sc, FIREI2C_CTRL, ctrl & ~FIREI2C_CTRL_IEN);
	}

	if (sc->sc_stage == FIREI2C_STAGE_IDLE) {
		DPRINTF(1)("%s: intr and idle\n", device_xname(sc->sc_dev));
		mutex_exit(&sc->sc_mutex);
		return 0;
	}

	ctrl = FIREI2C_CTRL_ENAB | FIREI2C_CTRL_IEN;
	val = firei2c_reg_read(sc, FIREI2C_STAT);

	switch (sc->sc_stage) {
	case FIREI2C_STAGE_START:
		DPRINTF(2)("%s: intr start, op %x\n",
		    device_xname(sc->sc_dev), sc->sc_op);
		if (val != FIREI2C_STAT_STA) {
			printf("%s: intr start error 0x%02x\n",
			    device_xname(sc->sc_dev), val);
			sc->sc_err = 1;
			sc->sc_stage = FIREI2C_STAGE_STOP;
			break;
		}

		/* Address + r/w in data, then send ctrl */
		val = sc->sc_target << FIREI2C_DATA_SHIFT;
		if (sc->sc_op == FIREI2C_OP_READ) {
			val |= 0x01;
			sc->sc_stage = FIREI2C_STAGE_RE_ADDR;
		} else
			sc->sc_stage = FIREI2C_STAGE_WR_ADDR;
		firei2c_reg_write(sc, FIREI2C_DATA, val);
		firei2c_reg_write(sc, FIREI2C_CTRL, ctrl);
		break;

	case FIREI2C_STAGE_WR_ADDR:
		DPRINTF(2)("%s: intr addr(w) 0x%02x = 0x%02x\n",
		    device_xname(sc->sc_dev), sc->sc_target, val);
		if (val != FIREI2C_STAT_AWR_ACK) {
			if (val != FIREI2C_STAT_AWR_NAK)
				printf("%s: intr addr(w) error 0x%02x\n",
				    device_xname(sc->sc_dev), val);
			sc->sc_err = 1;
			sc->sc_stage = FIREI2C_STAGE_STOP;
			break;
		}

		sc->sc_stage = FIREI2C_STAGE_WRITE;
		val = FIREI2C_STAT_DAT_ACK;	/* For write */
		/* Fallthrough */

	case FIREI2C_STAGE_WRITE:
		DPRINTF(2)("%s: intr write 0x%02x = 0x%02x\n",
		    device_xname(sc->sc_dev), sc->sc_target, val);
		if (val != FIREI2C_STAT_DAT_ACK) {
			if (val != FIREI2C_STAT_DAT_NAK)
				printf("%s: intr write error 0x%02x\n",
				    device_xname(sc->sc_dev), val);
			sc->sc_err = 1;
			sc->sc_stage = FIREI2C_STAGE_STOP;
			break;
		}

		/* Start the write from cmd */
		if (sc->sc_i < sc->sc_cmdlen) {
			firei2c_reg_write(sc, FIREI2C_DATA,
			    sc->sc_cmdbuf[sc->sc_i]);
			firei2c_reg_write(sc, FIREI2C_CTRL, ctrl);
			sc->sc_i++;
			break;
		}

		/* If a read OP, switch with repeat start after writing cmd */
		if (sc->sc_op == FIREI2C_OP_WR_RD) {
			sc->sc_i = 0;
			sc->sc_stage = FIREI2C_STAGE_REPSTART;
			ctrl |= FIREI2C_CTRL_STA;
			firei2c_reg_write(sc, FIREI2C_CTRL, ctrl);
			break;
		}

		/* If a write OP, continue the write, now from buf */
		if (sc->sc_i < sc->sc_cmdlen + sc->sc_buflen) {
			firei2c_reg_write(sc, FIREI2C_DATA,
			    sc->sc_buf[sc->sc_i - sc->sc_cmdlen]);
			firei2c_reg_write(sc, FIREI2C_CTRL, ctrl);
			sc->sc_i++;
		} else {
			sc->sc_stage = FIREI2C_STAGE_STOP;
		}
		break;

	case FIREI2C_STAGE_REPSTART:
		DPRINTF(2)("%s: intr repstart 0x%02x = 0x%02x\n",
		    device_xname(sc->sc_dev), sc->sc_target, val);
		if (val != FIREI2C_STAT_REPSTA) {
			printf("%s: intr repstart error 0x%02x\n",
			    device_xname(sc->sc_dev), val);
			sc->sc_err = 1;
			sc->sc_stage = FIREI2C_STAGE_STOP;
			break;
		}

		/* Address + r in data, then send ctrl */
		val = (sc->sc_target << FIREI2C_DATA_SHIFT) | 0x01;
		sc->sc_stage = FIREI2C_STAGE_RE_ADDR;
		firei2c_reg_write(sc, FIREI2C_DATA, val);
		firei2c_reg_write(sc, FIREI2C_CTRL, ctrl);
		break;

	case FIREI2C_STAGE_RE_ADDR:
		DPRINTF(2)("%s: intr addr(r) 0x%02x = 0x%02x\n",
		    device_xname(sc->sc_dev), sc->sc_target, val);
		if (val != FIREI2C_STAT_ARE_ACK) {
			if (val != FIREI2C_STAT_ARE_NAK)
				printf("%s: intr addr(r) error 0x%02x\n",
				    device_xname(sc->sc_dev), val);
			sc->sc_err = 1;
			sc->sc_stage = FIREI2C_STAGE_STOP;
			break;
		}

		sc->sc_stage = FIREI2C_STAGE_READ;
		val = FIREI2C_STAT_MDAT_NAK;	/* For read */
		/* Fallthrough */

	case FIREI2C_STAGE_READ:
		DPRINTF(2)("%s: intr read 0x%02x = 0x%02x\n",
		    device_xname(sc->sc_dev), sc->sc_target, val);
		if (sc->sc_i > 0 && sc->sc_i < sc->sc_buflen) {
			if (val != FIREI2C_STAT_MDAT_ACK) {
				printf("%s: intr read error 0x%02x\n",
				    device_xname(sc->sc_dev), val);
				sc->sc_err = 1;
				sc->sc_stage = FIREI2C_STAGE_STOP;
				break;
			}
		} else {
			if (val != FIREI2C_STAT_MDAT_NAK) {
				printf("%s: intr read error 0x%02x\n",
				    device_xname(sc->sc_dev), val);
				sc->sc_err = 1;
				sc->sc_stage = FIREI2C_STAGE_STOP;
				break;
			}
		}

		/* Data read is 1 cycle behind ctrl  */
		if (sc->sc_i > 0 && sc->sc_i <= sc->sc_buflen) {
			sc->sc_buf[sc->sc_i - 1] =
			   firei2c_reg_read(sc, FIREI2C_DATA);
		}

		if (sc->sc_i == sc->sc_buflen) {
			/* Last data was read */
			sc->sc_stage = FIREI2C_STAGE_STOP;
		} else {
			/* Send ACK on all but last byte */
			if (sc->sc_i < sc->sc_buflen - 1)
				ctrl |= FIREI2C_CTRL_AAK;
			firei2c_reg_write(sc, FIREI2C_CTRL, ctrl);
			sc->sc_i++;
		}
		break;
	}

	if (sc->sc_stage == FIREI2C_STAGE_STOP) {
		DPRINTF(2)("%s: intr stop (err=%d)\n",
		    device_xname(sc->sc_dev), sc->sc_err);
		cv_signal(&sc->sc_cv);
	}

	mutex_exit(&sc->sc_mutex);

	return 1;

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
