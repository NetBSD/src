/* $NetBSD: dwiic.c,v 1.1 2017/12/10 17:12:54 bouyer Exp $ */

/* $OpenBSD dwiic.c,v 1.24 2017/08/17 20:41:16 kettenis Exp $ */

/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Manuel Bouyer.
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
 * Synopsys DesignWare I2C controller
 *
 * Copyright (c) 2015, 2016 joshua stein <jcs@openbsd.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dwiic.c,v 1.1 2017/12/10 17:12:54 bouyer Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <dev/ic/dwiic_var.h>

//#define DWIIC_DEBUG

#ifdef DWIIC_DEBUG
#define DPRINTF(x) printf x
#else
#define DPRINTF(x)
#endif

/* register offsets */
#define DW_IC_CON		0x0
#define DW_IC_TAR		0x4
#define DW_IC_DATA_CMD		0x10
#define DW_IC_SS_SCL_HCNT	0x14
#define DW_IC_SS_SCL_LCNT	0x18
#define DW_IC_FS_SCL_HCNT	0x1c
#define DW_IC_FS_SCL_LCNT	0x20
#define DW_IC_INTR_STAT		0x2c
#define DW_IC_INTR_MASK		0x30
#define DW_IC_RAW_INTR_STAT	0x34
#define DW_IC_RX_TL		0x38
#define DW_IC_TX_TL		0x3c
#define DW_IC_CLR_INTR		0x40
#define DW_IC_CLR_RX_UNDER	0x44
#define DW_IC_CLR_RX_OVER	0x48
#define DW_IC_CLR_TX_OVER	0x4c
#define DW_IC_CLR_RD_REQ	0x50
#define DW_IC_CLR_TX_ABRT	0x54
#define DW_IC_CLR_RX_DONE	0x58
#define DW_IC_CLR_ACTIVITY	0x5c
#define DW_IC_CLR_STOP_DET	0x60
#define DW_IC_CLR_START_DET	0x64
#define DW_IC_CLR_GEN_CALL	0x68
#define DW_IC_ENABLE		0x6c
#define DW_IC_STATUS		0x70
#define DW_IC_TXFLR		0x74
#define DW_IC_RXFLR		0x78
#define DW_IC_SDA_HOLD		0x7c
#define DW_IC_TX_ABRT_SOURCE	0x80
#define DW_IC_ENABLE_STATUS	0x9c
#define DW_IC_COMP_PARAM_1	0xf4
#define DW_IC_COMP_VERSION	0xf8
#define DW_IC_SDA_HOLD_MIN_VERS	0x3131312A
#define DW_IC_COMP_TYPE		0xfc
#define DW_IC_COMP_TYPE_VALUE	0x44570140

#define DW_IC_CON_MASTER	0x1
#define DW_IC_CON_SPEED_STD	0x2
#define DW_IC_CON_SPEED_FAST	0x4
#define DW_IC_CON_10BITADDR_MASTER 0x10
#define DW_IC_CON_RESTART_EN	0x20
#define DW_IC_CON_SLAVE_DISABLE	0x40

#define DW_IC_DATA_CMD_READ	0x100
#define DW_IC_DATA_CMD_STOP	0x200
#define DW_IC_DATA_CMD_RESTART	0x400

#define DW_IC_INTR_RX_UNDER	0x001
#define DW_IC_INTR_RX_OVER	0x002
#define DW_IC_INTR_RX_FULL	0x004
#define DW_IC_INTR_TX_OVER	0x008
#define DW_IC_INTR_TX_EMPTY	0x010
#define DW_IC_INTR_RD_REQ	0x020
#define DW_IC_INTR_TX_ABRT	0x040
#define DW_IC_INTR_RX_DONE	0x080
#define DW_IC_INTR_ACTIVITY	0x100
#define DW_IC_INTR_STOP_DET	0x200
#define DW_IC_INTR_START_DET	0x400
#define DW_IC_INTR_GEN_CALL	0x800

#define DW_IC_STATUS_ACTIVITY	0x1

/* hardware abort codes from the DW_IC_TX_ABRT_SOURCE register */
#define ABRT_7B_ADDR_NOACK	0
#define ABRT_10ADDR1_NOACK	1
#define ABRT_10ADDR2_NOACK	2
#define ABRT_TXDATA_NOACK	3
#define ABRT_GCALL_NOACK	4
#define ABRT_GCALL_READ		5
#define ABRT_SBYTE_ACKDET	7
#define ABRT_SBYTE_NORSTRT	9
#define ABRT_10B_RD_NORSTRT	10
#define ABRT_MASTER_DIS		11
#define ARB_LOST		12

static int	dwiic_init(struct dwiic_softc *);
static void	dwiic_enable(struct dwiic_softc *, bool);
static int	dwiic_i2c_acquire_bus(void *, int);
static void	dwiic_i2c_release_bus(void *, int);
static uint32_t	dwiic_read(struct dwiic_softc *, int);
static void	dwiic_write(struct dwiic_softc *, int, uint32_t);
static int	dwiic_i2c_exec(void *, i2c_op_t, i2c_addr_t, const void *,
		    size_t, void *, size_t, int);

bool
dwiic_attach(struct dwiic_softc *sc)
{
	if (sc->sc_power != NULL) {
		if (!sc->sc_power(sc, 1)) {
			aprint_error_dev(sc->sc_dev, "failed to power up\n");
			return 0;
		}
	}

	/* fetch timing parameters */
	if (sc->ss_hcnt == 0) 
		sc->ss_hcnt = dwiic_read(sc, DW_IC_SS_SCL_HCNT);
	if (sc->ss_lcnt == 0)
		sc->ss_lcnt = dwiic_read(sc, DW_IC_SS_SCL_LCNT);
	if (sc->fs_hcnt == 0)
		sc->fs_hcnt = dwiic_read(sc, DW_IC_FS_SCL_HCNT);
	if (sc->fs_lcnt == 0)
		sc->fs_lcnt = dwiic_read(sc, DW_IC_FS_SCL_LCNT);
	if (sc->sda_hold_time == 0)
		sc->sda_hold_time = dwiic_read(sc, DW_IC_SDA_HOLD);

	if (dwiic_init(sc)) {
		aprint_error_dev(sc->sc_dev, "failed initializing\n");
		return 0;
	}

	/* leave the controller disabled */
	dwiic_write(sc, DW_IC_INTR_MASK, 0);
	dwiic_enable(sc, 0);
	dwiic_read(sc, DW_IC_CLR_INTR);

	mutex_init(&sc->sc_i2c_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_int_lock, MUTEX_DEFAULT, IPL_VM);
	cv_init(&sc->sc_int_readwait, "dwiicr");
	cv_init(&sc->sc_int_writewait, "dwiicw");

	/* setup and attach iic bus */
	sc->sc_i2c_tag.ic_cookie = sc;
	sc->sc_i2c_tag.ic_acquire_bus = dwiic_i2c_acquire_bus;
	sc->sc_i2c_tag.ic_release_bus = dwiic_i2c_release_bus;
	sc->sc_i2c_tag.ic_exec = dwiic_i2c_exec;

	sc->sc_iba.iba_tag = &sc->sc_i2c_tag;

	config_found_ia(sc->sc_dev, "i2cbus", &sc->sc_iba, iicbus_print);
	return 1;
}

int
dwiic_detach(device_t self, int flags)
{
	struct dwiic_softc *sc = device_private(self);

	dwiic_enable(sc, 0);
	if (sc->sc_ih != NULL) {
		intr_disestablish(sc->sc_ih);
		sc->sc_ih = NULL;
	}

	return 0;
}

bool
dwiic_suspend(device_t self, const pmf_qual_t *qual)
{
	struct dwiic_softc *sc = device_private(self);

	/* disable controller */
	dwiic_enable(sc, 0);

	/* disable interrupts */
	dwiic_write(sc, DW_IC_INTR_MASK, 0);
	dwiic_read(sc, DW_IC_CLR_INTR);
	if (sc->sc_power != NULL) {
		if (!sc->sc_power(sc, 0)) {
			aprint_error_dev(sc->sc_dev, "failed to power off\n");
		}
	}
	return true;
}

bool
dwiic_resume(device_t self, const pmf_qual_t *qual)
{
	struct dwiic_softc *sc = device_private(self);
	if (sc->sc_power != NULL) {
		if (!sc->sc_power(sc, 1)) {
			aprint_error_dev(sc->sc_dev, "failed to power up\n");
			return false;
		}
	}
	dwiic_init(sc);
	return true;
}

static uint32_t
dwiic_read(struct dwiic_softc *sc, int offset)
{
	u_int32_t b = bus_space_read_4(sc->sc_iot, sc->sc_ioh, offset);

	DPRINTF(("%s: read at 0x%x = 0x%x\n", device_xname(sc->sc_dev), offset, b));

	return b;
}

static void
dwiic_write(struct dwiic_softc *sc, int offset, uint32_t val)
{
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, offset, val);

	DPRINTF(("%s: write at 0x%x: 0x%x\n", device_xname(sc->sc_dev), offset,
	    val));
}

static int
dwiic_i2c_acquire_bus(void *cookie, int flags)
{
	struct dwiic_softc *sc = cookie;

	if (cold || sc->sc_poll || (flags & I2C_F_POLL))
		return (0);

	mutex_enter(&sc->sc_i2c_lock);
	return 1;
}

void
dwiic_i2c_release_bus(void *cookie, int flags)
{
	struct dwiic_softc *sc = cookie;

	if (cold || sc->sc_poll || (flags & I2C_F_POLL))
		return;

	mutex_exit(&sc->sc_i2c_lock);
}

static int
dwiic_init(struct dwiic_softc *sc)
{
	uint32_t reg;

	/* make sure we're talking to a device we know */
	reg = dwiic_read(sc, DW_IC_COMP_TYPE);
	if (reg != DW_IC_COMP_TYPE_VALUE) {
		DPRINTF(("%s: invalid component type 0x%x\n",
		    device_xname(sc->sc_dev), reg));
		return 1;
	}

	/* disable the adapter */
	dwiic_enable(sc, 0);

	/* write standard-mode SCL timing parameters */
	dwiic_write(sc, DW_IC_SS_SCL_HCNT, sc->ss_hcnt);
	dwiic_write(sc, DW_IC_SS_SCL_LCNT, sc->ss_lcnt);

	/* and fast-mode SCL timing parameters */
	dwiic_write(sc, DW_IC_FS_SCL_HCNT, sc->fs_hcnt);
	dwiic_write(sc, DW_IC_FS_SCL_LCNT, sc->fs_lcnt);

	/* SDA hold time */
	reg = dwiic_read(sc, DW_IC_COMP_VERSION);
	if (reg >= DW_IC_SDA_HOLD_MIN_VERS)
		dwiic_write(sc, DW_IC_SDA_HOLD, sc->sda_hold_time);

	/* FIFO threshold levels */
	sc->tx_fifo_depth = 32;
	sc->rx_fifo_depth = 32;
	dwiic_write(sc, DW_IC_TX_TL, sc->tx_fifo_depth / 2);
	dwiic_write(sc, DW_IC_RX_TL, 0);

	/* configure as i2c master with fast speed */
	sc->master_cfg = DW_IC_CON_MASTER | DW_IC_CON_SLAVE_DISABLE |
	    DW_IC_CON_RESTART_EN | DW_IC_CON_SPEED_FAST;
	dwiic_write(sc, DW_IC_CON, sc->master_cfg);

	return 0;
}

static void
dwiic_enable(struct dwiic_softc *sc, bool enable)
{
	int retries;

	for (retries = 100; retries > 0; retries--) {
		dwiic_write(sc, DW_IC_ENABLE, enable);
		if ((dwiic_read(sc, DW_IC_ENABLE_STATUS) & 1) == enable)
			return;

		DELAY(25);
	}

	aprint_error_dev(sc->sc_dev, "failed to %sable\n",
	    (enable ? "en" : "dis"));
}

static int
dwiic_i2c_exec(void *cookie, i2c_op_t op, i2c_addr_t addr, const void *cmdbuf,
    size_t cmdlen, void *buf, size_t len, int flags)
{
	struct dwiic_softc *sc = cookie;
	u_int32_t ic_con, st, cmd, resp;
	int retries, tx_limit, rx_avail, x, readpos;
	const uint8_t *bcmd;
	uint8_t *bdata;

	if (cold || sc->sc_poll)
		flags |= I2C_F_POLL;

	DPRINTF(("%s: %s: op %d, addr 0x%02x, cmdlen %zu, len %zu, "
	    "flags 0x%02x\n", device_xname(sc->sc_dev), __func__, op, addr, cmdlen,
	    len, flags));

	/* setup transfer */
	sc->sc_i2c_xfer.op = op;
	sc->sc_i2c_xfer.buf = buf;
	sc->sc_i2c_xfer.len = len;
	sc->sc_i2c_xfer.flags = flags;
	sc->sc_i2c_xfer.error = 0;

	/* wait for bus to be idle */
	for (retries = 100; retries > 0; retries--) {
		st = dwiic_read(sc, DW_IC_STATUS);
		if (!(st & DW_IC_STATUS_ACTIVITY))
			break;
		DELAY(1000);
	}
	DPRINTF(("%s: %s: status 0x%x\n", device_xname(sc->sc_dev), __func__, st));
	if (st & DW_IC_STATUS_ACTIVITY) {
		return (1);
	}

	/* disable controller */
	dwiic_enable(sc, 0);

	/* set slave address */
	ic_con = dwiic_read(sc, DW_IC_CON);
	ic_con &= ~DW_IC_CON_10BITADDR_MASTER;
	dwiic_write(sc, DW_IC_CON, ic_con);
	dwiic_write(sc, DW_IC_TAR, addr);

	/* disable interrupts */
	dwiic_write(sc, DW_IC_INTR_MASK, 0);
	dwiic_read(sc, DW_IC_CLR_INTR);

	/* enable controller */
	dwiic_enable(sc, 1);

	/* wait until the controller is ready for commands */
	if (flags & I2C_F_POLL)
		DELAY(200);
	else {
		mutex_enter(&sc->sc_int_lock);
		dwiic_read(sc, DW_IC_CLR_INTR);
		dwiic_write(sc, DW_IC_INTR_MASK, DW_IC_INTR_TX_EMPTY);

		if (cv_timedwait(&sc->sc_int_writewait,
		    &sc->sc_int_lock, hz / 2) != 0)
			aprint_error_dev(sc->sc_dev,
			    "timed out waiting for tx_empty intr\n");
		dwiic_write(sc, DW_IC_INTR_MASK, 0);
		dwiic_read(sc, DW_IC_CLR_INTR);
		mutex_exit(&sc->sc_int_lock);
	}

	/* send our command, one byte at a time */
	if (cmdlen > 0) {
		bcmd = (const void *)cmdbuf;

		DPRINTF(("%s: %s: sending cmd (len %zu):", device_xname(sc->sc_dev),
		    __func__, cmdlen));
		for (x = 0; x < cmdlen; x++)
			DPRINTF((" %02x", bcmd[x]));
		DPRINTF(("\n"));

		tx_limit = sc->tx_fifo_depth - dwiic_read(sc, DW_IC_TXFLR);
		if (cmdlen > tx_limit) {
			/* TODO */
			aprint_error_dev(sc->sc_dev, "can't write %zu (> %d)\n",
			    cmdlen, tx_limit);
			sc->sc_i2c_xfer.error = 1;
			return (1);
		}

		for (x = 0; x < cmdlen; x++) {
			cmd = bcmd[x];
			/*
			 * Generate STOP condition if this is the last
			 * byte of the transfer.
			 */
			if (x == (cmdlen - 1) && len == 0 && I2C_OP_STOP_P(op))
				cmd |= DW_IC_DATA_CMD_STOP;
			dwiic_write(sc, DW_IC_DATA_CMD, cmd);
		}
	}

	bdata = (void *)buf;
	x = readpos = 0;
	tx_limit = sc->tx_fifo_depth - dwiic_read(sc, DW_IC_TXFLR);

	DPRINTF(("%s: %s: need to read %zu bytes, can send %d read reqs\n",
		device_xname(sc->sc_dev), __func__, len, tx_limit));

	while (x < len) {
		if (I2C_OP_WRITE_P(op))
			cmd = bdata[x];
		else
			cmd = DW_IC_DATA_CMD_READ;

		/*
		 * Generate RESTART condition if we're reversing
		 * direction.
		 */
		if (x == 0 && cmdlen > 0 && I2C_OP_READ_P(op))
			cmd |= DW_IC_DATA_CMD_RESTART;
		/*
		 * Generate STOP conditon on the last byte of the
		 * transfer.
		 */
		if (x == (len - 1) && I2C_OP_STOP_P(op))
			cmd |= DW_IC_DATA_CMD_STOP;

		dwiic_write(sc, DW_IC_DATA_CMD, cmd);

		tx_limit--;
		x++;

		/*
		 * As TXFLR fills up, we need to clear it out by reading all
		 * available data.
		 */
		while (tx_limit == 0 || x == len) {
			DPRINTF(("%s: %s: tx_limit %d, sent %d read reqs\n",
			    device_xname(sc->sc_dev), __func__, tx_limit, x));

			if (flags & I2C_F_POLL) {
				for (retries = 100; retries > 0; retries--) {
					rx_avail = dwiic_read(sc, DW_IC_RXFLR);
					if (rx_avail > 0)
						break;
					DELAY(50);
				}
			} else {
				mutex_enter(&sc->sc_int_lock);
				dwiic_read(sc, DW_IC_CLR_INTR);
				dwiic_write(sc, DW_IC_INTR_MASK,
				    DW_IC_INTR_RX_FULL);

				if (cv_timedwait(&sc->sc_int_readwait,
				    &sc->sc_int_lock, hz / 2) != 0)
					aprint_error_dev(sc->sc_dev,
					    "timed out waiting for "
					    "rx_full intr\n");

				dwiic_write(sc, DW_IC_INTR_MASK, 0);
				dwiic_read(sc, DW_IC_CLR_INTR);
				mutex_exit(&sc->sc_int_lock);
				rx_avail = dwiic_read(sc, DW_IC_RXFLR);
			}

			if (rx_avail == 0) {
				aprint_error_dev(sc->sc_dev,
				    "timed out reading remaining %d\n",
				    (int)(len - 1 - readpos));
				sc->sc_i2c_xfer.error = 1;
				return (1);
			}

			DPRINTF(("%s: %s: %d avail to read (%zu remaining)\n",
			    device_xname(sc->sc_dev), __func__, rx_avail,
			    len - readpos));

			while (rx_avail > 0) {
				resp = dwiic_read(sc, DW_IC_DATA_CMD);
				if (readpos < len) {
					bdata[readpos] = resp;
					readpos++;
				}
				rx_avail--;
			}

			if (readpos >= len)
				break;

			DPRINTF(("%s: still need to read %d bytes\n",
			    device_xname(sc->sc_dev), (int)(len - readpos)));
			tx_limit = sc->tx_fifo_depth -
			    dwiic_read(sc, DW_IC_TXFLR);
		}
	}

	return 0;
}

static uint32_t
dwiic_read_clear_intrbits(struct dwiic_softc *sc)
{
       uint32_t stat;

       stat = dwiic_read(sc, DW_IC_INTR_STAT);

       if (stat & DW_IC_INTR_RX_UNDER)
	       dwiic_read(sc, DW_IC_CLR_RX_UNDER);
       if (stat & DW_IC_INTR_RX_OVER)
	       dwiic_read(sc, DW_IC_CLR_RX_OVER);
       if (stat & DW_IC_INTR_TX_OVER)
	       dwiic_read(sc, DW_IC_CLR_TX_OVER);
       if (stat & DW_IC_INTR_RD_REQ)
	       dwiic_read(sc, DW_IC_CLR_RD_REQ);
       if (stat & DW_IC_INTR_TX_ABRT)
	       dwiic_read(sc, DW_IC_CLR_TX_ABRT);
       if (stat & DW_IC_INTR_RX_DONE)
	       dwiic_read(sc, DW_IC_CLR_RX_DONE);
       if (stat & DW_IC_INTR_ACTIVITY)
	       dwiic_read(sc, DW_IC_CLR_ACTIVITY);
       if (stat & DW_IC_INTR_STOP_DET)
	       dwiic_read(sc, DW_IC_CLR_STOP_DET);
       if (stat & DW_IC_INTR_START_DET)
	       dwiic_read(sc, DW_IC_CLR_START_DET);
       if (stat & DW_IC_INTR_GEN_CALL)
	       dwiic_read(sc, DW_IC_CLR_GEN_CALL);

       return stat;
}

int
dwiic_intr(void *arg)
{
	struct dwiic_softc *sc = arg;
	uint32_t en, stat;

	en = dwiic_read(sc, DW_IC_ENABLE);
	/* probably for the other controller */
	if (!en)
		return 0;

	stat = dwiic_read_clear_intrbits(sc);
	DPRINTF(("%s: %s: enabled=0x%x stat=0x%x\n", device_xname(sc->sc_dev),
	    __func__, en, stat));
	if (!(stat & ~DW_IC_INTR_ACTIVITY))
		return 1;

	if (stat & DW_IC_INTR_TX_ABRT)
		sc->sc_i2c_xfer.error = 1;

	if (sc->sc_i2c_xfer.flags & I2C_F_POLL)
		DPRINTF(("%s: %s: intr in poll mode?\n", device_xname(sc->sc_dev),
		    __func__));
	else {
		mutex_enter(&sc->sc_int_lock);
		if (stat & DW_IC_INTR_RX_FULL) {
			dwiic_write(sc, DW_IC_INTR_MASK, 0);
			DPRINTF(("%s: %s: waking up reader\n",
			    device_xname(sc->sc_dev), __func__));
			cv_signal(&sc->sc_int_readwait);
		}
		if (stat & DW_IC_INTR_TX_EMPTY) {
			dwiic_write(sc, DW_IC_INTR_MASK, 0);
			DPRINTF(("%s: %s: waking up writer\n",
			    device_xname(sc->sc_dev), __func__));
			cv_signal(&sc->sc_int_writewait);
		}
		mutex_exit(&sc->sc_int_lock);
	}

	return 1;
}
