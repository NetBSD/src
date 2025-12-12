/*	$NetBSD: umcpmio_spi.c,v 1.2 2025/12/12 17:49:35 andvar Exp $	*/

/*
 * Copyright (c) 2025 Brad Spencer <brad@anduin.eldar.org>
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
__KERNEL_RCSID(0, "$NetBSD: umcpmio_spi.c,v 1.2 2025/12/12 17:49:35 andvar Exp $");

#ifdef _KERNEL_OPT
#include "opt_usb.h"
#endif

#include <sys/param.h>
#include <sys/types.h>

#include <dev/spi/spivar.h>

#include <dev/usb/umcpmio.h>
#include <dev/usb/umcpmio_hid_reports.h>
#include <dev/usb/umcpmio_transport.h>
#include <dev/usb/umcpmio_spi.h>
#include <dev/usb/umcpmio_gpio.h>
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

/* Stuff required for the SPI part of the MCP2210 */

static int
mcp2210_get_spi_sram(struct umcpmio_softc *sc,
    struct mcp2210_get_spi_sram_res *res)
{
	struct mcp2210_get_spi_sram_req req;
	int err = 0;

	memset(&req, 0, MCP2210_REQ_BUFFER_SIZE);
	req.cmd = MCP2210_CMD_GET_SPI_SRAM;

	umcpmio_dump_buffer(sc->sc_dumpbuffer,
	    (uint8_t *) & req, MCP2210_REQ_BUFFER_SIZE,
	    "mcp2210_get_spi_sram req");

	mutex_enter(&sc->sc_action_mutex);
	err = umcpmio_send_report(sc,
	    (uint8_t *) & req, MCP2210_REQ_BUFFER_SIZE,
	    (uint8_t *) res, sc->sc_cv_wait);
	mutex_exit(&sc->sc_action_mutex);

	umcpmio_dump_buffer(sc->sc_dumpbuffer,
	    (uint8_t *) res, MCP2210_RES_BUFFER_SIZE,
	    "mcp2210_get_spi_sram res");

	err = mcp2210_decode_errors(req.cmd, err, res->completion);

	return err;
}

static int
mcp2210_set_spi_sram(struct umcpmio_softc *sc,
    struct mcp2210_set_spi_sram_req *req, struct mcp2210_set_spi_sram_res *res)
{
	int err = 0;

	req->cmd = MCP2210_CMD_SET_SPI_SRAM;

	umcpmio_dump_buffer(sc->sc_dumpbuffer,
	    (uint8_t *) req, MCP2210_REQ_BUFFER_SIZE,
	    "mcp2210_set_spi_sram req");

	mutex_enter(&sc->sc_action_mutex);
	err = umcpmio_send_report(sc,
	    (uint8_t *) req, MCP2210_REQ_BUFFER_SIZE,
	    (uint8_t *) res, sc->sc_cv_wait);
	mutex_exit(&sc->sc_action_mutex);

	umcpmio_dump_buffer(sc->sc_dumpbuffer,
	    (uint8_t *) res, MCP2210_RES_BUFFER_SIZE,
	    "mcp2210_set_spi_sram res");

	err = mcp2210_decode_errors(req->cmd, err, res->completion);

	return err;
}

static int
mcp2210_do_spi_transfer(struct umcpmio_softc *sc,
    struct mcp2210_spi_transfer_req *req, struct mcp2210_spi_transfer_res *res)
{
	int err = 0;

	req->cmd = MCP2210_CMD_SPI_TRANSFER;

	umcpmio_dump_buffer(sc->sc_dumpbuffer,
	    (uint8_t *) req, MCP2210_REQ_BUFFER_SIZE,
	    "mcp2210_do_spi_transfer req");

	mutex_enter(&sc->sc_action_mutex);
	err = umcpmio_send_report(sc,
	    (uint8_t *) req, MCP2210_REQ_BUFFER_SIZE,
	    (uint8_t *) res, sc->sc_cv_wait);
	mutex_exit(&sc->sc_action_mutex);

	umcpmio_dump_buffer(sc->sc_dumpbuffer,
	    (uint8_t *) res, MCP2210_RES_BUFFER_SIZE,
	    "mcp2210_do_spi_transfer res");

	err = mcp2210_decode_errors(req->cmd, err, res->completion);

	return err;
}

/* Most configuration is deferred until a transfer actually happens.  The
 * exception is setting the pin that is to be used as a CS to ALT0.
 *
 * There is no attempt to reset the mode of the pin used for the CS,
 * as there is no concept of a session with the SPI framework.  That is,
 * you would never know when to do it.  Further, it will cause pin
 * flapping of all of the other pins if the CS is set to INPUT
 * or OUTPUT.  Of course, that will happen when the CS mode (ALT0)
 * is set, but only the first time the pin is used as a CS.
 *
 * It is advisable to make use of the NVRAM to set up the pin mode
 * upon boot up of the chip.
 *
 */

static int
mcp2210_spi_configure(void *cookie, int slave, int mode, int speed)
{
	struct umcpmio_softc *sc = cookie;
	uint8_t desired_mode = 0;
	struct umcpmio_spi_received *r;
	int err = 0;

	if (slave >= sc->sc_spi.sct_nslaves)
		return EINVAL;

	/* The speed is in bps and the MCP2210 uses bps, but is not suppose to
	 * be able to go below 1500 bps.  There is also an upper bound of 12Mbps,
	 * but the chip will not do anything wrong if you try to go faster as it
	 * will cap the speed.
	 */

	if (speed < 1500) {
		aprint_error_dev(sc->sc_dev,
		    "Speed %d is too low and not supported.", speed);
		return EINVAL;
	}
	mutex_enter(&sc->sc_spi_mutex);

	umcpmio_gpio_pin_ctl(sc, slave, GPIO_PIN_ALT0);

	switch (mode) {
	case SPI_MODE_0:
		desired_mode = MCP2210_SPI_MODE_0;
		break;
	case SPI_MODE_1:
		desired_mode = MCP2210_SPI_MODE_1;
		break;
	case SPI_MODE_2:
		desired_mode = MCP2210_SPI_MODE_2;
		break;
	case SPI_MODE_3:
		desired_mode = MCP2210_SPI_MODE_3;
		break;
	default:
		err = EINVAL;
	}


	/* Remove any queued up data.  This is a choice, but it seems the
	 * right now.  If you call configure() you are probably resetting
	 * the SPI port.
	 */

	if (!err) {
		sc->sc_slave_configs[slave].bit_rate = (uint32_t) speed;
		sc->sc_slave_configs[slave].mode = desired_mode;

		if (!SIMPLEQ_EMPTY(&sc->sc_received[slave])) {
			while ((r = SIMPLEQ_FIRST(&sc->sc_received[slave])) != NULL) {
				SIMPLEQ_REMOVE_HEAD(&sc->sc_received[slave], umcpmio_spi_received_q);
				kmem_free(r, sizeof(struct umcpmio_spi_received));
			}
		}
	}

	mutex_exit(&sc->sc_spi_mutex);

	return err;
}

/* Set all of the stuff about the slave device that is needed
 * to do a transfer.
 */

static int
mcp2210_spi_set_slave_config(struct umcpmio_softc *sc,
    uint16_t max_transfer_size, int slave)
{
	struct mcp2210_get_spi_sram_res get_res;
	struct mcp2210_set_spi_sram_req set_req;
	struct mcp2210_set_spi_sram_res set_res;
	uint16_t idle;
	uint16_t active;
	int err = 0;

	err = mcp2210_get_spi_sram(sc, &get_res);
	if (err)
		goto out;

	memset(&set_res, 0, MCP2210_RES_BUFFER_SIZE);
	memcpy(&set_req, &get_res, MCP2210_REQ_BUFFER_SIZE);

	/* The MCP2210 supports the notion of a high active for
	 * the CS, but spi(4) doesn't so, just use the normal
	 * low active behavior all of the time.
	 *
	 * The chip also appears to support more than one CS
	 * being active at the same time, but that is really
	 * an unusual desire to have.
	 */

	idle = 0xffff;
	active = ~(1 << slave);

	set_req.idle_cs_value_lsb = (uint8_t)(idle & 0xff);
	set_req.idle_cs_value_msb = (uint8_t)(idle >> 8);
	set_req.active_cs_value_lsb = (uint8_t)(active & 0xff);
	set_req.active_cs_value_msb = (uint8_t)(active >> 8);

	set_req.spi_mode = sc->sc_slave_configs[slave].mode;

	uint32_t s = sc->sc_slave_configs[slave].bit_rate;
	set_req.bit_rate_byte_3 = (0x000000ff & s);
	set_req.bit_rate_byte_2 |= (0x000000ff & (s >> 8));
	set_req.bit_rate_byte_1 |= (0x000000ff & (s >> 16));
	set_req.bit_rate_byte_0 |= (0x000000ff & (s >> 24));

	/* The MCP2210 does best when the transfer size is set to the
	 * same max amount that a transfer incoming might have.
	 */

	set_req.bytes_per_spi_transaction_lsb = (uint8_t)(max_transfer_size & 0xff);
	set_req.bytes_per_spi_transaction_msb = (uint8_t)(max_transfer_size >> 8);

	err = mcp2210_set_spi_sram(sc, &set_req, &set_res);

out:

	DPRINTF(("mcp2210_spi_set_slave exit: err=%d\n", err));

	return err;
}

/* Fill up the outgoing transfer buffers.  It is generally assumed that the state
 * of the req buffer is set coming into this function.  It should either be
 * zero filled or a repeat buffer in a retry situation.
 */

static void
mcp2210_spi_tx_fill(struct umcpmio_softc *sc,
    struct mcp2210_spi_transfer_req *req)
{
	struct spi_chunk *chunk = sc->sc_wchunk;
	size_t len;

	if (chunk == NULL)
		return;

	if (sc->sc_dying)
		return;

	len = MIN(MCP2210_MAX_SPI_BYTES, chunk->chunk_wresid);
	DPRINTF(("mcp2210_spi_tx_fill: len=%zd; chunk_wresid=%d\n",
	    len, chunk->chunk_wresid));
	chunk->chunk_wresid -= len;
	req->num_send_bytes += len;
	if (chunk->chunk_wptr)
		memcpy(&req->send_bytes[0], chunk->chunk_wptr, len);
	DPRINTF(("mcp2210_spi_tx_fill: checking sending values..  req->num_send_bytes=%d, chunk_wresid=%d\n",
	    req->num_send_bytes, chunk->chunk_wresid));
	if (chunk->chunk_wresid == 0) {
		DPRINTF(("mcp2210_spi_tx_fill: moving onto the next chunk\n"));
		sc->sc_wchunk = sc->sc_wchunk->chunk_next;
		if (sc->sc_wchunk)
			DPRINTF(("mcp2210_spi_tx_fill: new chunk EXISTS\n"));
		else
			DPRINTF(("mcp2210_spi_tx_fill: new chunk does not EXIST, NULL\n"));
	}
}

/* Drain the incoming buffer into the upstream structure.  If there is no
 * upstream to drain into, then queue up the result for later.
 */

static int
mcp2210_spi_rx_drain(struct umcpmio_softc *sc, int slave,
    struct mcp2210_spi_transfer_res *res)
{
	struct spi_chunk *chunk = sc->sc_rchunk;
	size_t len;
	struct umcpmio_spi_received *r;
	int err = 0;

	if (chunk == NULL)
		return err;

	if (sc->sc_dying)
		return err;

	len = MIN(res->num_receive_bytes, chunk->chunk_rresid);
	chunk->chunk_rresid -= len;
	DPRINTF(("mcp2210_spi_rx_drain: ENTER: len=%zd; res->num_receive_bytes=%d; chunk_rresid=%d\n",
	    len, res->num_receive_bytes, chunk->chunk_rresid));
	if (chunk->chunk_rptr)
		memcpy(chunk->chunk_rptr, &res->receive_bytes[0], len);
	if (chunk->chunk_rresid == 0) {
		DPRINTF(("mcp2210_spi_rx_drain: moving onto the next chunk\n"));
		sc->sc_rchunk = sc->sc_rchunk->chunk_next;
		if (sc->sc_rchunk)
			DPRINTF(("mcp2210_spi_rx_drain: new chunk EXISTS\n"));
		else
			DPRINTF(("mcp2210_spi_rx_drain: new chunk does not EXIST, NULL\n"));
	}
	DPRINTF(("mcp2210_spi_rx_drain: checking received values..  res->num_received_bytes=%d\n",
	    res->num_receive_bytes));
	if (len < res->num_receive_bytes) {
		DPRINTF(("mcp2210_spi_rx_drain: did not consume all that was received, Q it up....  res->num_received_bytes=%d\n",
		    res->num_receive_bytes));
		r = kmem_alloc(sizeof(struct umcpmio_spi_received), KM_NOSLEEP);
		if (r != NULL) {
			memcpy(&r->receive_bytes[0], &res->receive_bytes[len], res->num_receive_bytes - len);
			r->num_receive_bytes = res->num_receive_bytes - len;
			SIMPLEQ_INSERT_HEAD(&sc->sc_received[slave], r, umcpmio_spi_received_q);
		} else {
			err = ENOMEM;
		}
	}
	return err;
}

/* Very similar to receiving from the chip buffer, except from this
 * is from the local queue.  Requeue if needed.
 */

static int
mcp2210_spi_rx_q(struct umcpmio_softc *sc, int slave,
    struct umcpmio_spi_received *r)
{
	struct spi_chunk *chunk = sc->sc_rchunk;
	size_t len;
	struct umcpmio_spi_received *mr;
	int err = 0;

	if (chunk == NULL)
		return err;

	if (sc->sc_dying)
		return err;

	len = MIN(r->num_receive_bytes, chunk->chunk_rresid);
	chunk->chunk_rresid -= len;
	DPRINTF(("mcp2210_spi_rx_q: ENTER: len=%zd; r->num_receive_bytes=%d; chunk_rresid=%d\n",
	    len, r->num_receive_bytes, chunk->chunk_rresid));
	if (chunk->chunk_rptr)
		memcpy(chunk->chunk_rptr, &r->receive_bytes[0], len);
	if (chunk->chunk_rresid == 0) {
		DPRINTF(("mcp2210_spi_rx_q: moving onto the next chunk\n"));
		sc->sc_rchunk = sc->sc_rchunk->chunk_next;
		if (sc->sc_rchunk)
			DPRINTF(("mcp2210_spi_rx_q: new chunk EXISTS\n"));
		else
			DPRINTF(("mcp2210_spi_rx_q: new chunk does not EXIST, NULL\n"));
	}
	DPRINTF(("mcp2210_spi_rx_q: checking received values..  r->num_received_bytes=%d\n",
	    r->num_receive_bytes));
	if (len < r->num_receive_bytes) {
		DPRINTF(("mcp2210_spi_rx_q: did not consume all that was received, Q it up... r->num_received_bytes=%d\n",
		    r->num_receive_bytes));
		mr = kmem_alloc(sizeof(struct umcpmio_spi_received), KM_NOSLEEP);
		if (mr != NULL) {
			memcpy(&mr->receive_bytes[0], &r->receive_bytes[len], r->num_receive_bytes - len);
			mr->num_receive_bytes = r->num_receive_bytes - len;
			SIMPLEQ_INSERT_HEAD(&sc->sc_received[slave], mr, umcpmio_spi_received_q);
		} else {
			err = ENOMEM;
		}
	}
	return err;
}

/* Drain the chip buffers into the local queue.  No upstream involved. */

static int
mcp2210_spi_rx_drainchip(struct umcpmio_softc *sc, int slave,
    struct mcp2210_spi_transfer_res *res)
{
	int err = 0;
	struct umcpmio_spi_received *r;

	r = kmem_alloc(sizeof(struct umcpmio_spi_received), KM_NOSLEEP);
	if (r == NULL)
		return ENOMEM;
	memcpy(&r->receive_bytes[0], &res->receive_bytes[0], res->num_receive_bytes);
	r->num_receive_bytes = res->num_receive_bytes;
	SIMPLEQ_INSERT_TAIL(&sc->sc_received[slave], r, umcpmio_spi_received_q);

	return err;
}

/* Service a transfer.  We do the local queue first if there is something, then
 * send a transfer, with an optional amount of data, and then process any
 * incoming chip data.
 */

static int
mcp2210_spi_txrx(struct umcpmio_softc *sc, int slave)
{
	struct mcp2210_spi_transfer_req req;
	struct mcp2210_spi_transfer_res res;
	struct umcpmio_spi_received *r;
	int err = 0;
	int wretry = sc->sc_retry_busy_write;

	DPRINTF(("mcp2210_spi_txrx: ENTER\n"));

	if (sc->sc_dying)
		return err;

	/* There was local queue data and an upstream to receive it */

	if (!SIMPLEQ_EMPTY(&sc->sc_received[slave]) &&
	    sc->sc_rchunk != NULL) {
		DPRINTF(("mcp2210_spi_txrx: previous received bytes to send to upstream\n"));
		while ((r = SIMPLEQ_FIRST(&sc->sc_received[slave])) != NULL) {
			SIMPLEQ_REMOVE_HEAD(&sc->sc_received[slave], umcpmio_spi_received_q);
			err = mcp2210_spi_rx_q(sc, slave, r);
			kmem_free(r, sizeof(struct umcpmio_spi_received));
			if (err)
				break;
		}

		goto out;
	}
	if (sc->sc_dying)
		return err;

	/* Set up a transfer and fill it once */

	memset(&req, 0, UMCPMIO_REQ_BUFFER_SIZE);

	if (sc->sc_wchunk != NULL)
		DPRINTF(("mcp2210_spi_txrx: filling up req for write transfer; chunk_wresid=%d\n", sc->sc_wchunk->chunk_wresid));
	else
		DPRINTF(("mcp2210_spi_txrx: filling up req for write transfer; wchunk is NULL\n"));
	mcp2210_spi_tx_fill(sc, &req);

retry:

	memset(&res, 0, UMCPMIO_RES_BUFFER_SIZE);

	if (sc->sc_dying)
		return err;

	err = mcp2210_do_spi_transfer(sc, &req, &res);
	if (err)
		if (err != EBUSY)
			goto out;

	DPRINTF(("mcp2210_spi_txrx: did transfer res.completion=%02x ;"
	    "res.spi_engine_status=%02x, res.num_receive_bytes=%d\n",
	    res.completion, res.spi_engine_status, res.num_receive_bytes));

	/* This error situation of MCP2210_SPI_STATUS_DATA (0x30) and
	 * zero received data means either the chip is stuck because
	 * upstream didn't send enough data, or the chip is working
	 * on receiving some data.  We only let this condition go on
	 * a certain number of times before passing the error up.
	 */

	if (!err) {
		if (res.spi_engine_status == MCP2210_SPI_STATUS_DATA &&
		    res.num_receive_bytes == 0) {
			err = ECANCELED;
		}
	}

	/* A normal busy caused by the SPI bus being busy or the USB
	 * bus being busy.  Try the whole transfer again.
	 */
	
	if (err == EBUSY) {
		DPRINTF(("mcp2210_spi_txrx: transfer is busy\n"));
		wretry--;
		if (wretry > 0) {
			WAITMS(sc->sc_busy_delay);
			goto retry;
		} else {
			err = EIO;
			goto out;
		}
	}

	/* If the chip knows that there is data coming, but has not seen
	 * any yet you get this error.  Retry the transfer with a zero
	 * length outgoing buffer.
	 */
	
	if (res.spi_engine_status == MCP2210_SPI_STATUS_NO_DATA_YET) {
		DPRINTF(("mcp2210_spi_txrx: transfer is NO DATA YET\n"));
		wretry--;
		if (wretry > 0) {
			WAITMS(sc->sc_busy_delay);
			memset(&req, 0, UMCPMIO_REQ_BUFFER_SIZE);
			goto retry;
		} else {
			err = EIO;
			goto out;
		}
	}
	DPRINTF(("mcp2210_spi_txrx: DOING RECEIVE: res.num_receive_bytes=%d\n",
	    res.num_receive_bytes));

	/* If we actually received some data, drain it somewhere */

	if (res.num_receive_bytes > 0) {
		if (sc->sc_rchunk != NULL &&
		    sc->sc_rchunk->chunk_rptr) {
			DPRINTF(("mcp2210_spi_txrx: send data to upstream\n"));
			err = mcp2210_spi_rx_drain(sc, slave, &res);
		} else {
			DPRINTF(("mcp2210_spi_txrx: drain chip, send data to Q\n"));
			err = mcp2210_spi_rx_drainchip(sc, slave, &res);
		}
	}
out:

	DPRINTF(("mcp2210_spi_txrx exit.  err=%d\n", err));

	return err;
}

/* Cancel a SPI transfer that might be in progress, or at
 * least the chip thinks is in progress.
 */

int
mcp2210_cancel_spi_transfer(struct umcpmio_softc *sc,
    struct mcp2210_cancel_spi_res *res)
{
	struct mcp2210_cancel_spi_req req;
	int err = 0;

	KASSERT(mutex_owned(&sc->sc_action_mutex));

	memset(&req, 0, MCP2210_REQ_BUFFER_SIZE);
	req.cmd = MCP2210_CMD_SPI_CANCEL;

	umcpmio_dump_buffer(sc->sc_dumpbuffer,
	    (uint8_t *) & req, MCP2210_REQ_BUFFER_SIZE,
	    "mcp2210_cancel_spi_transfer req");

	err = umcpmio_send_report(sc,
	    (uint8_t *) & req, MCP2210_REQ_BUFFER_SIZE,
	    (uint8_t *) res, sc->sc_cv_wait);

	umcpmio_dump_buffer(sc->sc_dumpbuffer,
	    (uint8_t *) res, MCP2210_RES_BUFFER_SIZE,
	    "mcp2210_get_spi_sram res");

	err = mcp2210_decode_errors(req.cmd, err, res->completion);

	return err;
}

/* As long as there are outgoing chunks to send,
 * or incoming chunks to receive, poll the SPI port.
 */

static void
mcp2210_spi_poll(struct umcpmio_softc *sc,
    int slave, int *stuckcount)
{
	struct spi_transfer *st;
	struct mcp2210_cancel_spi_res cancel_res;
	int err = 0;

	err = mcp2210_spi_txrx(sc, slave);

	DPRINTF(("mcp2210_spi_poll: err from transfer: err=%d\n", err));

	/* Give up if we exceed the stuck count.  It is ok for this to happen
	 * a couple of times during a transfer, but if it persists, something is
	 * going wrong.
	 */
	if (err == ECANCELED) {
		int s;

		s = *stuckcount;
		s--;
		DPRINTF(("mcp2210_spi_poll: STUCK s=%d\n", s));
		if (s > 0)
			err = 0;
		*stuckcount = s;
	}

	if (err) {
		if (err == ECANCELED) {
			if (sc->sc_spi_verbose)
				device_printf(sc->sc_dev, "The SPI transfer may be stuck.  Canceling.\n");
			mutex_enter(&sc->sc_action_mutex);
			mcp2210_cancel_spi_transfer(sc, &cancel_res);
			mutex_exit(&sc->sc_action_mutex);
		}
		sc->sc_rchunk = sc->sc_wchunk = NULL;
	}

	/* Finialize the transfer */
	
	if (sc->sc_rchunk == NULL &&
	    sc->sc_wchunk == NULL) {
		st = sc->sc_transfer;
		sc->sc_transfer = NULL;
		KASSERT(st != NULL);
		spi_done(st, 0);
		sc->sc_running = false;
	}
	return;
}

/* Start a SPI transfer up and running */

static void
mcp2210_spi_start(struct umcpmio_softc *sc)
{
	struct spi_transfer *st;
	struct spi_chunk *chunk;
	uint16_t max_transfer_size = 0;
	int transfer_size = 0;
	int err = 0;
	int stuckcount = 4; /* XXX - maybe this should be a sysctl */
	struct mcp2210_cancel_spi_res cancel_res;

	while ((st = spi_transq_first(&sc->sc_q)) != NULL) {
		spi_transq_dequeue(&sc->sc_q);
		KASSERT(sc->sc_transfer == NULL);
		sc->sc_transfer = st;
		sc->sc_rchunk = sc->sc_wchunk = st->st_chunks;
		sc->sc_running = true;

		KASSERT(st->st_slave < sc->sc_spi.sct_nslaves);

		/* Figure out the maximum size for this transfer.
		 * It doesn't matter if it is an incoming or outgoing
		 * chunk.
		 */
		chunk = st->st_chunks;
		while (chunk != NULL) {
			if (chunk->chunk_wresid > transfer_size)
				transfer_size = chunk->chunk_wresid;
			if (chunk->chunk_rresid > transfer_size)
				transfer_size = chunk->chunk_rresid;
			chunk = chunk->chunk_next;
		}

		KASSERT(transfer_size > 0);

		/* The chip can do up to 65535 bytes in a transfer.  We subtract
		 * 1 so the slave device can send back an end byte.  There is,
		 * apparently some way to do larger transfers, but I could
		 * not find documentation on how to do that.
		 */
		if (transfer_size > 65534) {
			if (sc->sc_spi_verbose)
				device_printf(sc->sc_dev, "Canceling transfer.  "
				    "The size of the requested transfer is too big.  "
				    "transfer_size=%d\n",
				    transfer_size);

			st = sc->sc_transfer;
			sc->sc_transfer = NULL;
			spi_done(st, 0);
			sc->sc_running = false;

			return;
		}
		max_transfer_size = (uint16_t) transfer_size + 1;

		DPRINTF(("mcp2210_spi_start: transfer_size=%d; max_transfer_size=%d\n", transfer_size, max_transfer_size));

		/* Set all of the deferred setting now that we are committed to trying
		 * to do the transfer.
		 */
		err = mcp2210_spi_set_slave_config(sc, max_transfer_size, st->st_slave);
		/* If we get EBUSY here, then it probably means that there was a incomplete
		 * transfer.  Cancel it, and try one more time.
		 */
		if (err == EBUSY) {
			mutex_enter(&sc->sc_action_mutex);
			mcp2210_cancel_spi_transfer(sc, &cancel_res);
			mutex_exit(&sc->sc_action_mutex);
			err = mcp2210_spi_set_slave_config(sc, max_transfer_size, st->st_slave);
		}
		if (err) {
			if (sc->sc_spi_verbose)
				device_printf(sc->sc_dev, "Canceling transfer.  Error setting up slave config.  error=%d\n", err);

			st = sc->sc_transfer;
			sc->sc_transfer = NULL;
			spi_done(st, 0);
			sc->sc_running = false;

			return;
		}

		for (;;) {
			mcp2210_spi_poll(sc, st->st_slave, &stuckcount);
			if (ISSET(st->st_flags, SPI_F_DONE))
				break;
			if (sc->sc_dying)
				break;
		}
	}
	sc->sc_running = false;

	return;
}

static int
mcp2210_spi_transfer(void *cookie, struct spi_transfer *st)
{
	struct umcpmio_softc *sc = cookie;

	mutex_enter(&sc->sc_spi_mutex);
	spi_transq_enqueue(&sc->sc_q, st);
	if (sc->sc_running == false) {
		mcp2210_spi_start(sc);
	}
	mutex_exit(&sc->sc_spi_mutex);

	return 0;
}

int
umcpmio_spi_attach(struct umcpmio_softc *sc)
{
	struct mcp2210_get_spi_sram_res get_res;
	struct mcp2210_set_spi_sram_req set_req;
	struct mcp2210_set_spi_sram_res set_res;
	struct mcp2210_cancel_spi_res cancel_res;
	int err = 0;

	sc->sc_spi_verbose = true;
	sc->sc_busy_delay = 0;
	sc->sc_retry_busy_read = 50;
	sc->sc_retry_busy_write = 50;
	sc->sc_running = false;

	DPRINTF(("umcpmio_spi_attach: sc->sc_chipinfo->num_spi_slaves=%d\n", sc->sc_chipinfo->num_spi_slaves));

	SIMPLEQ_INIT(&sc->sc_q);

	for (int i = 0; i < sc->sc_chipinfo->num_spi_slaves; i++){
		SIMPLEQ_INIT(&sc->sc_received[i]);
		sc->sc_slave_configs[i].bit_rate = (uint32_t) SPI_FREQ_MHz(1);
		sc->sc_slave_configs[i].mode = MCP2210_SPI_MODE_0;
	}

	/* Set all of the active and idle CS directions here. */
	err = mcp2210_get_spi_sram(sc, &get_res);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "%s: unable to read SPI sram: %d\n",
		    __func__, err);
		goto out;
	}
	memset(&set_res, 0, MCP2210_RES_BUFFER_SIZE);
	memcpy(&set_req, &get_res, MCP2210_REQ_BUFFER_SIZE);

	set_req.idle_cs_value_lsb = set_req.idle_cs_value_msb = 0xff;
	set_req.active_cs_value_lsb = set_req.active_cs_value_msb = 0x00;

	err = mcp2210_set_spi_sram(sc, &set_req, &set_res);
	/* If there is a EBUSY here, then a transfer was incomplete
	 * in the past.  That can happen on attach if the chip stays
	 * powered up, but is removed from the USB port and
	 * reinserted.  Cancel the transfer and try one more time.
	 */
	if (err == EBUSY) {
		mutex_enter(&sc->sc_action_mutex);
		mcp2210_cancel_spi_transfer(sc, &cancel_res);
		mutex_exit(&sc->sc_action_mutex);
		err = mcp2210_set_spi_sram(sc, &set_req, &set_res);
	}
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "%s: unable to set SPI sram: %d\n",
		    __func__, err);
		goto out;
	}
	sc->sc_spi.sct_cookie = sc;
	sc->sc_spi.sct_configure = mcp2210_spi_configure;
	sc->sc_spi.sct_transfer = mcp2210_spi_transfer;
	sc->sc_spi.sct_nslaves = sc->sc_chipinfo->num_spi_slaves;

	spibus_attach(sc->sc_dev, &sc->sc_spi);

out:

	return err;
}
