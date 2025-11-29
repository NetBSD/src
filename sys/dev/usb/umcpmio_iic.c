/*	$NetBSD: umcpmio_iic.c,v 1.1 2025/11/29 18:39:14 brad Exp $	*/

/*
 * Copyright (c) 2024, 2025 Brad Spencer <brad@anduin.eldar.org>
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: umcpmio_iic.c,v 1.1 2025/11/29 18:39:14 brad Exp $");

#ifdef _KERNEL_OPT
#include "opt_usb.h"
#endif

#include <sys/param.h>
#include <sys/types.h>

#include <dev/i2c/i2cvar.h>

#include <dev/usb/umcpmio.h>
#include <dev/usb/umcpmio_hid_reports.h>
#include <dev/usb/umcpmio_transport.h>
#include <dev/usb/umcpmio_iic.h>
#include <dev/usb/umcpmio_subr.h>

#define UMCPMIO_DEBUG 1
#ifdef UMCPMIO_DEBUG
#define DPRINTF(x)	do { if (umcpmiodebug) printf x; } while (0)
#define DPRINTFN(n, x)	do { if (umcpmiodebug > (n)) printf x; } while (0)
extern int umcpmiodebug;
#else
#define DPRINTF(x)	__nothing
#define DPRINTFN(n, x)	__nothing
#endif

/* Stuff required for the I2C part of the MCP2221 / MCP2221A */

/* Clear status of the I2C engine */

static void
mcp2221_set_i2c_speed(struct mcp2221_status_req *req, int flags)
{
	int i2cbaud = MCP2221_DEFAULT_I2C_SPEED;

	if (flags & I2C_F_SPEED)
		i2cbaud = 400000;

	req->set_i2c_speed = MCP2221_I2C_SET_SPEED;
	if (i2cbaud <= 0)
		i2cbaud = MCP2221_DEFAULT_I2C_SPEED;

	/* Everyone and their brother seems to store the I2C divider like this,
	 * so do likewise */
	req->i2c_clock_divider = (MCP2221_INTERNAL_CLOCK / i2cbaud) - 3;
}

static int
mcp2221_set_i2c_speed_one(struct umcpmio_softc *sc, int flags)
{
	int err = 0;
	struct mcp2221_status_req req;
	struct mcp2221_status_res res;

	KASSERT(mutex_owned(&sc->sc_action_mutex));

	memset(&req, 0, MCP2221_REQ_BUFFER_SIZE);
	mcp2221_set_i2c_speed(&req, flags);
	err = mcp2221_put_status(sc, &req, &res);
	if (err)
		goto out;
	if (res.set_i2c_speed == MCP2221_I2C_SPEED_BUSY)
		err = EBUSY;
out:
	return err;
}

static int
mcp2221_i2c_clear(struct umcpmio_softc *sc)
{
	int err = 0;
	struct mcp2221_status_req status_req;
	struct mcp2221_status_res status_res;

	KASSERT(mutex_owned(&sc->sc_action_mutex));

	memset(&status_req, 0, MCP2221_REQ_BUFFER_SIZE);
	status_req.cmd = MCP2221_CMD_STATUS;
	status_req.cancel_transfer = MCP2221_I2C_DO_CANCEL;

	err = umcpmio_send_report(sc,
	    (uint8_t *) & status_req, MCP2221_REQ_BUFFER_SIZE,
	    (uint8_t *) & status_res, sc->sc_cv_wait);

	if (err) {
		err = EIO;
		goto out;
	}
	umcpmio_dump_buffer(sc->sc_dumpbuffer,
	    (uint8_t *) & status_res, MCP2221_RES_BUFFER_SIZE,
	    "mcp2221_i2c_clear buffer copy");

	if (status_res.cmd != MCP2221_CMD_STATUS &&
	    status_res.completion != MCP2221_CMD_COMPLETE_OK) {
		err = EIO;
		goto out;
	}
	umcpmio_dump_buffer(true,
	    (uint8_t *) & status_res, MCP2221_RES_BUFFER_SIZE,
	    "mcp2221_i2c_clear res buffer");

out:
	return err;
}

/*
 * There isn't much required to acquire or release the I2C bus, but the man
 * pages says these are needed
 */

static int
mcp2221_acquire_bus(void *v, int flags)
{
	return 0;
}

static void
mcp2221_release_bus(void *v, int flags)
{
	return;
}

/*
 * The I2C write and I2C read functions mostly use an algorithm that Adafruit
 * came up with in their Python based driver.  A lot of other people have used
 * this same algorithm to good effect.  If changes are made to the I2C read and
 * write functions, it is HIGHLY advisable that a MCP2221 or MCP2221A be on
 * hand to test them.
 */

/* This is what is considered a fatal return from the engine. */

static bool
mcp2221_i2c_fatal(uint8_t state)
{
	int r = false;

	if (state == MCP2221_ENGINE_ADDRNACK ||
	    state == MCP2221_ENGINE_STARTTIMEOUT ||
	    state == MCP2221_ENGINE_REPSTARTTIMEOUT ||
	    state == MCP2221_ENGINE_STOPTIMEOUT ||
	    state == MCP2221_ENGINE_READTIMEOUT ||
	    state == MCP2221_ENGINE_WRITETIMEOUT ||
	    state == MCP2221_ENGINE_ADDRTIMEOUT)
		r = true;
	return r;
}

static int
mcp2221_i2c_write(struct umcpmio_softc *sc, i2c_op_t op, i2c_addr_t addr,
    const void *cmdbuf, size_t cmdlen, void *databuf, size_t datalen,
    int flags)
{
	struct mcp2221_i2c_req i2c_req;
	struct mcp2221_i2c_res i2c_res;
	struct mcp2221_status_res status_res;
	int remaining;
	int err = 0;
	uint8_t cmd;
	size_t totallen = 0;
	int wretry = sc->sc_retry_busy_write;
	int wsretry = sc->sc_retry_busy_write;

	mutex_enter(&sc->sc_action_mutex);
	err = mcp2221_get_status(sc, &status_res);
	mutex_exit(&sc->sc_action_mutex);
	if (err)
		goto out;
	if (status_res.internal_i2c_state != 0) {
		DPRINTF(("mcp2221_i2c_write: internal state not zero,"
		    " clearing. internal_i2c_state=%02x\n",
		    status_res.internal_i2c_state));
		mutex_enter(&sc->sc_action_mutex);
		err = mcp2221_i2c_clear(sc);
		mutex_exit(&sc->sc_action_mutex);
	}
	if (err)
		goto out;

	if (cmdbuf != NULL)
		totallen += cmdlen;
	if (databuf != NULL)
		totallen += datalen;

again:
	memset(&i2c_req, 0, MCP2221_REQ_BUFFER_SIZE);
	cmd = MCP2221_I2C_WRITE_DATA_NS;
	if (I2C_OP_STOP_P(op))
		cmd = MCP2221_I2C_WRITE_DATA;
	i2c_req.cmd = cmd;
	i2c_req.lsblen = totallen;
	i2c_req.msblen = 0;
	i2c_req.slaveaddr = addr << 1;

	remaining = 0;
	if (cmdbuf != NULL) {
		memcpy(&i2c_req.data[0], cmdbuf, cmdlen);
		remaining = cmdlen;
	}
	if (databuf != NULL)
		memcpy(&i2c_req.data[remaining], databuf, datalen);

	DPRINTF(("mcp2221_i2c_write: I2C WRITE: cmd: %02x\n", cmd));
	umcpmio_dump_buffer(sc->sc_dumpbuffer,
	    (uint8_t *) & i2c_req, MCP2221_REQ_BUFFER_SIZE,
	    "mcp2221_i2c_write: write req buffer copy");

	mutex_enter(&sc->sc_action_mutex);
	err = umcpmio_send_report(sc,
	    (uint8_t *) & i2c_req, MCP2221_REQ_BUFFER_SIZE,
	    (uint8_t *) & i2c_res, sc->sc_cv_wait);
	mutex_exit(&sc->sc_action_mutex);
	if (err) {
		err = EIO;
		goto out;
	}
	umcpmio_dump_buffer(sc->sc_dumpbuffer,
	    (uint8_t *) & i2c_res, MCP2221_RES_BUFFER_SIZE,
	    "mcp2221_i2c_write: write res buffer copy");
	if (i2c_res.cmd == cmd &&
	    i2c_res.completion == MCP2221_CMD_COMPLETE_OK) {
		/* Adafruit does a read back of the status at this point.  We
		 * choose not to do that.  That is done later anyway, and it
		 * seemed to be redundent. */
	} else if (i2c_res.cmd == cmd &&
	    i2c_res.completion == MCP2221_I2C_ENGINE_BUSY) {
		DPRINTF(("mcp2221_i2c_write:"
		    " I2C engine busy\n"));

		if (mcp2221_i2c_fatal(i2c_res.internal_i2c_state)) {
			err = EIO;
			goto out;
		}
		wretry--;
		if (wretry > 0) {
			WAITMS(sc->sc_busy_delay);
			goto again;
		} else {
			err = EBUSY;
			goto out;
		}
	} else {
		err = EIO;
		goto out;
	}

	while (wsretry > 0) {
		wsretry--;

		DPRINTF(("mcp2221_i2c_write: checking status loop:"
		    " wcretry=%d\n", wsretry));

		mutex_enter(&sc->sc_action_mutex);
		err = mcp2221_get_status(sc, &status_res);
		mutex_exit(&sc->sc_action_mutex);
		if (err) {
			err = EIO;
			break;
		}
		umcpmio_dump_buffer(sc->sc_dumpbuffer,
		    (uint8_t *) & status_res,
		    MCP2221_RES_BUFFER_SIZE,
		    "mcp2221_i2c_write post check status");
		/* Since there isn't any documentation on what some of the
		 * internal state means, it isn't clear that this is any
		 * different than than MCP2221_ENGINE_ADDRNACK in the other
		 * state register. */

		if (status_res.internal_i2c_state20 &
		    MCP2221_ENGINE_T1_MASK_NACK) {
			DPRINTF(("mcp2221_i2c_write post check:"
			    " engine internal state T1 says NACK\n"));
			err = EIO;
			break;
		}
		if (status_res.internal_i2c_state == 0) {
			DPRINTF(("mcp2221_i2c_write post check:"
			    " engine internal state is ZERO\n"));
			err = 0;
			break;
		}
		if (status_res.internal_i2c_state ==
		    MCP2221_ENGINE_WRITINGNOSTOP &&
		    cmd == MCP2221_I2C_WRITE_DATA_NS) {
			DPRINTF(("mcp2221_i2c_write post check:"
			    " engine internal state is WRITINGNOSTOP\n"));
			err = 0;
			break;
		}
		if (mcp2221_i2c_fatal(status_res.internal_i2c_state)) {
			DPRINTF(("mcp2221_i2c_write post check:"
			    " engine internal state is fatal: %02x\n",
			    status_res.internal_i2c_state));
			err = EIO;
			break;
		}
		WAITMS(sc->sc_busy_delay);
	}

out:
	return err;
}

/*
 * This one deviates a bit from Adafruit in that is supports a straight
 * read and a write + read.  That is, write a register to read from and
 * then do the read.
 */

static int
mcp2221_i2c_read(struct umcpmio_softc *sc, i2c_op_t op, i2c_addr_t addr,
    const void *cmdbuf, size_t cmdlen, void *databuf, size_t datalen, int
    flags)
{
	struct mcp2221_i2c_req i2c_req;
	struct mcp2221_i2c_res i2c_res;
	struct mcp2221_i2c_fetch_req i2c_fetch_req;
	struct mcp2221_i2c_fetch_res i2c_fetch_res;
	struct mcp2221_status_res status_res;
	int err = 0;
	uint8_t cmd;
	int rretry = sc->sc_retry_busy_read;

	if (cmdbuf != NULL) {
		DPRINTF(("mcp2221_i2c_read: has a cmdbuf, doing write first:"
		    " addr=%02x\n", addr));
		err = mcp2221_i2c_write(sc, I2C_OP_WRITE, addr, cmdbuf, cmdlen,
		    NULL, 0, flags);
	}
	if (err)
		goto out;

	mutex_enter(&sc->sc_action_mutex);
	err = mcp2221_get_status(sc, &status_res);
	mutex_exit(&sc->sc_action_mutex);
	if (err)
		goto out;

	if (status_res.internal_i2c_state != 0 &&
	    status_res.internal_i2c_state != MCP2221_ENGINE_WRITINGNOSTOP) {
		DPRINTF(("mcp2221_i2c_read:"
		    " internal state not zero and not WRITINGNOSTOP,"
		    " clearing. internal_i2c_state=%02x\n",
		    status_res.internal_i2c_state));
		mutex_enter(&sc->sc_action_mutex);
		err = mcp2221_i2c_clear(sc);
		mutex_exit(&sc->sc_action_mutex);
	}
	if (err)
		goto out;

	memset(&i2c_req, 0, MCP2221_REQ_BUFFER_SIZE);
	if (cmdbuf == NULL &&
	    status_res.internal_i2c_state != MCP2221_ENGINE_WRITINGNOSTOP) {
		cmd = MCP2221_I2C_READ_DATA;
	} else {
		cmd = MCP2221_I2C_READ_DATA_RS;
	}

	/* The chip apparently can't do a READ without a STOP operation. Report
	 * that, and try treating it like a READ with a STOP.  This won't work
	 * for a lot of devices. */

	if (!I2C_OP_STOP_P(op) && sc->sc_reportreadnostop) {
		device_printf(sc->sc_dev,
		    "mcp2221_i2c_read: ************ called with READ"
		    " without STOP ***************\n");
	}
	i2c_req.cmd = cmd;
	i2c_req.lsblen = datalen;
	i2c_req.msblen = 0;
	i2c_req.slaveaddr = (addr << 1) | 0x01;

	DPRINTF(("mcp2221_i2c_read: I2C READ normal read:"
	    " cmd=%02x, addr=%02x\n", cmd, addr));

	umcpmio_dump_buffer(sc->sc_dumpbuffer,
	    (uint8_t *) & i2c_req, MCP2221_RES_BUFFER_SIZE,
	    "mcp2221_i2c_read normal read req buffer copy");

	mutex_enter(&sc->sc_action_mutex);
	err = umcpmio_send_report(sc,
	    (uint8_t *) & i2c_req, MCP2221_REQ_BUFFER_SIZE,
	    (uint8_t *) & i2c_res, sc->sc_cv_wait);
	mutex_exit(&sc->sc_action_mutex);

	if (err) {
		err = EIO;
		goto out;
	}
	umcpmio_dump_buffer(sc->sc_dumpbuffer,
	    (uint8_t *) & i2c_res, MCP2221_RES_BUFFER_SIZE,
	    "mcp2221_i2c_read read-request response buffer copy");

	while (rretry > 0) {
		rretry--;
		DPRINTF(("mcp2221_i2c_read: fetch loop: rretry=%d\n", rretry));
		err = 0;
		memset(&i2c_fetch_req, 0, MCP2221_REQ_BUFFER_SIZE);
		i2c_fetch_req.cmd = MCP2221_CMD_I2C_FETCH_READ_DATA;
		mutex_enter(&sc->sc_action_mutex);
		err = umcpmio_send_report(sc,
		    (uint8_t *) & i2c_fetch_req, MCP2221_REQ_BUFFER_SIZE,
		    (uint8_t *) & i2c_fetch_res, sc->sc_cv_wait);
		mutex_exit(&sc->sc_action_mutex);
		umcpmio_dump_buffer(sc->sc_dumpbuffer,
		    (uint8_t *) & i2c_fetch_req, MCP2221_RES_BUFFER_SIZE,
		    "mcp2221_i2c_read fetch res buffer copy");

		if (i2c_fetch_res.cmd != MCP2221_CMD_I2C_FETCH_READ_DATA) {
			err = EIO;
			break;
		}
		if (i2c_fetch_res.completion ==
		    MCP2221_FETCH_READ_PARTIALDATA ||
		    i2c_fetch_res.fetchlen == MCP2221_FETCH_READERROR) {
			DPRINTF(("mcp2221_i2c_read: fetch loop:"
			    " partial data or read error:"
			    " completion=%02x, fetchlen=%02x\n",
			    i2c_fetch_res.completion,
			    i2c_fetch_res.fetchlen));
			WAITMS(sc->sc_busy_delay);
			err = EAGAIN;
			continue;
		}
		if (i2c_fetch_res.internal_i2c_state ==
		    MCP2221_ENGINE_ADDRNACK) {
			DPRINTF(("mcp2221_i2c_read:"
			    " fetch loop: engine NACK\n"));
			err = EIO;
			break;
		}
		if (i2c_fetch_res.internal_i2c_state == 0 &&
		    i2c_fetch_res.fetchlen == 0) {
			DPRINTF(("mcp2221_i2c_read: fetch loop:"
			    " internal state and fetch len are ZERO\n"));
			err = 0;
			break;
		}
		if (i2c_fetch_res.internal_i2c_state ==
		    MCP2221_ENGINE_READPARTIAL ||
		    i2c_fetch_res.internal_i2c_state ==
		    MCP2221_ENGINE_READCOMPLETE) {
			DPRINTF(("mcp2221_i2c_read:"
			    " fetch loop: read partial or"
			    " read complete: internal_i2c_state=%02x\n",
			    i2c_fetch_res.internal_i2c_state));
			err = 0;
			break;
		}
	}
	if (err == EAGAIN)
		err = ETIMEDOUT;
	if (err)
		goto out;

	if (databuf == NULL ||
	    i2c_fetch_res.fetchlen == MCP2221_FETCH_READERROR) {
		DPRINTF(("mcp2221_i2c_read: copy data:"
		    " databuf is NULL\n"));
		goto out;
	}
	const int size = uimin(i2c_fetch_res.fetchlen, datalen);
	DPRINTF(("mcp2221_i2c_read: copy data: size=%d, fetchlen=%d\n",
	    size, i2c_fetch_res.fetchlen));
	if (size > 0) {
		memcpy(databuf, &i2c_fetch_res.data[0], size);
	}
out:
	return err;
}

static int
mcp2221_i2c_exec(void *v, i2c_op_t op, i2c_addr_t addr, const void *cmdbuf,
    size_t cmdlen, void *databuf, size_t datalen, int flags)
{
	struct umcpmio_softc *sc = v;
	size_t totallen = 0;
	int err = 0;

	if (sc->sc_dying)
		return EIO;

	if (addr > 0x7f)
		return ENOTSUP;

	if (cmdbuf != NULL)
		totallen += cmdlen;
	if (databuf != NULL)
		totallen += datalen;

	/* There is a way to do a transfer that is larger than 60 bytes, but it
	 * requires that your break the transfer up into pieces and send them
	 * in 60 byte chunks.  We just won't support that right now. It would
	 * be somewhat unusual for there to be a transfer that big, unless you
	 * are trying to do block transfers and that isn't natively supported
	 * by the chip anyway...  so those have to be broken up and sent as
	 * bytes. */

	if (totallen > 60)
		return ENOTSUP;

	if (I2C_OP_WRITE_P(op)) {
		err = mcp2221_i2c_write(sc, op, addr, cmdbuf, cmdlen,
		    databuf, datalen, flags);

		DPRINTF(("umcpmio_exec: I2C WRITE: err=%d\n", err));
	} else {
		err = mcp2221_i2c_read(sc, op, addr, cmdbuf, cmdlen,
		    databuf, datalen, flags);

		DPRINTF(("umcpmio_exec: I2C READ: err=%d\n", err));
	}

	return err;
}

int
umcpmio_i2c_attach(struct umcpmio_softc *sc)
{
	sc->sc_reportreadnostop = true;
	sc->sc_busy_delay = 1;
	sc->sc_retry_busy_read = 50;
	sc->sc_retry_busy_write = 50;

	int err = 0;

	/* The datasheet suggests that it is possble for this to fail if the
	 * I2C port is currently being used. However...  since you just plugged
	 * in the chip, the I2C port should not really be in use at that
	 * moment. In any case, try hard to set this and don't make it fatal if
	 * it did not get set. */
	int i2cspeed;
	for (i2cspeed = 0; i2cspeed < 3; i2cspeed++) {
		mutex_enter(&sc->sc_action_mutex);
		err = mcp2221_set_i2c_speed_one(sc, I2C_SPEED_SM);
		mutex_exit(&sc->sc_action_mutex);
		if (err) {
			aprint_error_dev(sc->sc_dev, "umcpmio_i2c_attach:"
			    " set I2C speed: err=%d\n",
			    err);
			delay(300);
		}
		break;
	}

	iic_tag_init(&sc->sc_i2c_tag);
	sc->sc_i2c_tag.ic_cookie = sc;
	sc->sc_i2c_tag.ic_acquire_bus = mcp2221_acquire_bus;
	sc->sc_i2c_tag.ic_release_bus = mcp2221_release_bus;
	sc->sc_i2c_tag.ic_exec = mcp2221_i2c_exec;

	sc->sc_i2c_dev = iicbus_attach(sc->sc_dev, &sc->sc_i2c_tag);

	return err;
}
