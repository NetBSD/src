/*	$NetBSD: bcm2835_bsc.c,v 1.15.22.1 2024/02/16 12:08:02 skrll Exp $	*/

/*
 * Copyright (c) 2019 Jason R. Thorpe
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
__KERNEL_RCSID(0, "$NetBSD: bcm2835_bsc.c,v 1.15.22.1 2024/02/16 12:08:02 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/kernhist.h>
#include <sys/intr.h>
#include <sys/mutex.h>
#include <sys/systm.h>

#include <dev/i2c/i2cvar.h>

#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm2835_bscreg.h>
#include <arm/broadcom/bcm2835_bscvar.h>

static void	bsciic_exec_func_idle(struct bsciic_softc * const);
static void	bsciic_exec_func_send_addr(struct bsciic_softc * const);
static void	bsciic_exec_func_send_cmd(struct bsciic_softc * const);
static void	bsciic_exec_func_send_data(struct bsciic_softc * const);
static void	bsciic_exec_func_recv_data(struct bsciic_softc * const);
static void	bsciic_exec_func_done(struct bsciic_softc * const);
static void	bsciic_exec_func_error(struct bsciic_softc * const);

const struct {
	void			(*func)(struct bsciic_softc * const);
	uint32_t		c_bits;
	uint32_t		s_bits;
} bsciic_exec_state_data[] = {
	[BSC_EXEC_STATE_IDLE] = {
		.func = bsciic_exec_func_idle,
	},
	[BSC_EXEC_STATE_SEND_ADDR] = {
		.func = bsciic_exec_func_send_addr,
	},
	[BSC_EXEC_STATE_SEND_CMD] = {
		.func = bsciic_exec_func_send_cmd,
		.c_bits = BSC_C_INTT,
		.s_bits = BSC_S_TXW,
	},
	[BSC_EXEC_STATE_SEND_DATA] = {
		.func = bsciic_exec_func_send_data,
		.c_bits = BSC_C_INTT,
		.s_bits = BSC_S_TXW,
	},
	[BSC_EXEC_STATE_RECV_DATA] = {
		.func = bsciic_exec_func_recv_data,
		.c_bits = BSC_C_READ | BSC_C_INTR,
		.s_bits = BSC_S_RXR,
	},
	[BSC_EXEC_STATE_DONE] = {
		.func = bsciic_exec_func_done,
	},
	[BSC_EXEC_STATE_ERROR] = {
		.func = bsciic_exec_func_error,
	},
};

int	bsciic_debug = 0;

void
bsciic_attach(struct bsciic_softc *sc)
{
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_VM);
	cv_init(&sc->sc_intr_wait, device_xname(sc->sc_dev));

	/* clear FIFO, disable controller */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_C, BSC_C_CLEAR_CLEAR);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_S, BSC_S_CLKT |
	    BSC_S_ERR | BSC_S_DONE);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_DLEN, 0);

	u_int divider = howmany(sc->sc_frequency, sc->sc_clkrate);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_DIV,
	   __SHIFTIN(divider, BSC_DIV_CDIV));
}

int
bsciic_acquire_bus(void *v, int flags)
{
	struct bsciic_softc * const sc = v;
	uint32_t s __diagused;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_S, BSC_S_CLKT |
	    BSC_S_ERR | BSC_S_DONE);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_C, BSC_C_I2CEN |
	    BSC_C_CLEAR_CLEAR);
	s = bus_space_read_4(sc->sc_iot, sc->sc_ioh, BSC_S);
	KASSERT((s & BSC_S_TA) == 0);

	return 0;
}

void
bsciic_release_bus(void *v, int flags)
{
	struct bsciic_softc * const sc = v;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_C, BSC_C_CLEAR_CLEAR);
}

static void
bsciic_exec_lock(struct bsciic_softc * const sc)
{
	if ((sc->sc_exec.flags & I2C_F_POLL) == 0) {
		mutex_enter(&sc->sc_intr_lock);
	}
}

static void
bsciic_exec_unlock(struct bsciic_softc * const sc)
{
	if ((sc->sc_exec.flags & I2C_F_POLL) == 0) {
		mutex_exit(&sc->sc_intr_lock);
	}
}

static void
bsciic_txfill(struct bsciic_softc * const sc)
{
	uint32_t s;

	while (sc->sc_bufpos != sc->sc_buflen) {
		s = bus_space_read_4(sc->sc_iot, sc->sc_ioh, BSC_S);
		if ((s & BSC_S_TXD) == 0)
			break;
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_FIFO,
		    sc->sc_buf[sc->sc_bufpos++]);
	}
}

static void
bsciic_rxdrain(struct bsciic_softc * const sc)
{
	uint32_t s;

	while (sc->sc_bufpos != sc->sc_buflen) {
		s = bus_space_read_4(sc->sc_iot, sc->sc_ioh, BSC_S);
		if ((s & BSC_S_RXD) == 0)
			break;
		sc->sc_buf[sc->sc_bufpos++] =
		    (uint8_t)bus_space_read_4(sc->sc_iot, sc->sc_ioh, BSC_FIFO);
	}
}

static bsc_exec_state_t
bsciic_next_state(struct bsciic_softc * const sc)
{
	switch (sc->sc_exec_state) {
	case BSC_EXEC_STATE_IDLE:
		if (sc->sc_exec.addr > 0x7f) {
			return BSC_EXEC_STATE_SEND_ADDR;
		}
		/* FALLTHROUGH */

	case BSC_EXEC_STATE_SEND_ADDR:
		if (sc->sc_exec.cmdlen) {
			return BSC_EXEC_STATE_SEND_CMD;
		}
		/* FALLTHROUGH */

	case BSC_EXEC_STATE_SEND_CMD:
		if (sc->sc_exec.datalen == 0) {
			return BSC_EXEC_STATE_DONE;
		}

		if (I2C_OP_READ_P(sc->sc_exec.op)) {
			return BSC_EXEC_STATE_RECV_DATA;
		}

		return BSC_EXEC_STATE_SEND_DATA;

	case BSC_EXEC_STATE_SEND_DATA:
	case BSC_EXEC_STATE_RECV_DATA:
		return BSC_EXEC_STATE_DONE;

	case BSC_EXEC_STATE_DONE:
	case BSC_EXEC_STATE_ERROR:
		return sc->sc_exec_state;
	}

	panic("bsciic_next_state: invalid state: %d", sc->sc_exec_state);
}

#define	BSC_EXEC_PHASE_COMPLETE(sc)				\
	((sc)->sc_exec_state == BSC_EXEC_STATE_ERROR ||		\
	 (sc)->sc_bufpos == (sc)->sc_buflen)

static void
bsciic_signal(struct bsciic_softc * const sc)
{
	if ((sc->sc_exec.flags & I2C_F_POLL) == 0) {
		cv_signal(&sc->sc_intr_wait);
	}
}

static void
bsciic_phase_done(struct bsciic_softc * const sc)
{
	sc->sc_exec_state = bsciic_next_state(sc);
	(*bsciic_exec_state_data[sc->sc_exec_state].func)(sc);
}

static void
bsciic_abort(struct bsciic_softc * const sc)
{
	sc->sc_exec_state = BSC_EXEC_STATE_ERROR;
	bsciic_phase_done(sc);
}

int
bsciic_intr(void *v)
{
	struct bsciic_softc * const sc = v;
	uint32_t s;

	bsciic_exec_lock(sc);

	if ((sc->sc_exec.flags & I2C_F_POLL) == 0 &&
	    sc->sc_expecting_interrupt == false) {
		bsciic_exec_unlock(sc);
		return 0;
	}

	s = bus_space_read_4(sc->sc_iot, sc->sc_ioh, BSC_S);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_S,
	    BSC_S_CLKT | BSC_S_ERR | BSC_S_DONE);

	if (s & (BSC_S_CLKT | BSC_S_ERR)) {
		/*
		 * ERR might be a normal "probing for device" sort
		 * of thing, so don't complain about that one.
		 * Do complain about CLKT, though.
		 */
		if ((s & BSC_S_CLKT) ||
		    (bsciic_debug && (s & BSC_S_ERR))) {
			device_printf(sc->sc_dev,
			    "error s=0x%08x, aborting transfer\n", s);
		}
		bsciic_abort(sc);
		goto out;
	}

	if (BSC_EXEC_STATE_SENDING(sc)) {
		/*
		 * When transmitting, we need to wait for one final
		 * interrupt after pushing out the last of our data.
		 * Catch that case here and go to the next state.
		 */
		if (BSC_EXEC_PHASE_COMPLETE(sc)) {
			bsciic_phase_done(sc);
		} else {
			bsciic_txfill(sc);
		}
	} else if (BSC_EXEC_STATE_RECEIVING(sc)) {
		bsciic_rxdrain(sc);
		/*
		 * If we've received all of the data, go to the next
		 * state now; we might not get another interrupt.
		 */
		if (BSC_EXEC_PHASE_COMPLETE(sc)) {
			bsciic_phase_done(sc);
		}
	} else {
		device_printf(sc->sc_dev,
		    "unexpected interrupt: state=%d s=0x%08x\n",
		    sc->sc_exec_state, s);
		bsciic_abort(sc);
	}

 out:
	bsciic_exec_unlock(sc);
	return (1);
}

static void
bsciic_wait(struct bsciic_softc * const sc, const uint32_t events)
{
	if ((sc->sc_exec.flags & I2C_F_POLL) == 0) {
		return;
	}

	const uint32_t s_bits =
	    events | BSC_S_CLKT | BSC_S_ERR | BSC_S_DONE;
	uint32_t s;

	/* sc_intr_lock is not held in this case. */

	for (;;) {
		s = bus_space_read_4(sc->sc_iot, sc->sc_ioh, BSC_S);
		if (s & s_bits) {
			(void) bsciic_intr(sc);
		}
		if (BSC_EXEC_PHASE_COMPLETE(sc)) {
			bsciic_phase_done(sc);
		}
		if (sc->sc_exec_state >= BSC_EXEC_STATE_DONE) {
			return;
		}
		delay(1);
	}
}

static void
bsciic_start(struct bsciic_softc * const sc)
{

	sc->sc_c_bits = BSC_C_I2CEN | BSC_C_INTD |
	    bsciic_exec_state_data[sc->sc_exec_state].c_bits;

	/* Clear the interrupt-enable bits if we're polling. */
	if (sc->sc_exec.flags & I2C_F_POLL) {
		sc->sc_c_bits &= ~(BSC_C_INTD | BSC_C_INTT | BSC_C_INTR);
	}

	sc->sc_expecting_interrupt =
	    (sc->sc_c_bits & (BSC_C_INTD | BSC_C_INTT | BSC_C_INTR)) ? true
								     : false;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_C,
	    sc->sc_c_bits | BSC_C_ST);
	bsciic_wait(sc, bsciic_exec_state_data[sc->sc_exec_state].s_bits);
}

static void
bsciic_exec_func_idle(struct bsciic_softc * const sc)
{
	/* We kick off a transfer by setting the slave address register. */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_A, sc->sc_exec.addr);

	/* Immediately transition to the next state. */
	bsciic_phase_done(sc);
}

static void
bsciic_exec_func_send_addr(struct bsciic_softc * const sc)
{
	/* XXX For 10-bit addressing; not implemented yet. */
	panic("bsciic_exec_func_send_addr is not supposed to be called");
}

static void
bsciic_exec_func_send_cmd(struct bsciic_softc * const sc)
{
	sc->sc_buf = __UNCONST(sc->sc_exec.cmdbuf);
	sc->sc_bufpos = 0;
	sc->sc_buflen = sc->sc_exec.cmdlen;

	uint32_t dlen = sc->sc_exec.cmdlen;
	if (! I2C_OP_READ_P(sc->sc_exec.op)) {
		dlen += sc->sc_exec.datalen;
	}
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_DLEN, dlen);

	bsciic_start(sc);
}

static void
bsciic_exec_func_send_data(struct bsciic_softc * const sc)
{
	sc->sc_buf = sc->sc_exec.databuf;
	sc->sc_bufpos = 0;
	sc->sc_buflen = sc->sc_exec.datalen;

	if (sc->sc_exec.cmdlen) {
		/*
		 * Output has already been started in this case; we just
		 * needed to switch buffers.
		 */
		bsciic_wait(sc, BSC_S_TXW);
	} else {
		uint32_t dlen = sc->sc_exec.datalen;
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_DLEN, dlen);
		bsciic_start(sc);
	}
}

static void
bsciic_exec_func_recv_data(struct bsciic_softc * const sc)
{
	sc->sc_buf = sc->sc_exec.databuf;
	sc->sc_bufpos = 0;
	sc->sc_buflen = sc->sc_exec.datalen;

	uint32_t dlen = sc->sc_exec.datalen;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_DLEN, dlen);

	bsciic_start(sc);
}

static void
bsciic_exec_func_done(struct bsciic_softc * const sc)
{
	/* We're done!  Disable interrupts. */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_C, BSC_C_I2CEN);
	sc->sc_expecting_interrupt = false;
	bsciic_signal(sc);
}

static void
bsciic_exec_func_error(struct bsciic_softc * const sc)
{
	/* Clear the FIFO and disable interrupts. */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_C,
	    BSC_C_I2CEN | BSC_C_CLEAR_CLEAR);
	sc->sc_expecting_interrupt = false;
	bsciic_signal(sc);
}

int
bsciic_exec(void *v, i2c_op_t op, i2c_addr_t addr, const void *cmdbuf,
    size_t cmdlen, void *databuf, size_t datalen, int flags)
{
	struct bsciic_softc * const sc = v;

	/* XXX We don't do 10-bit addressing correctly yet. */
	if (addr > 0x7f)
		return (ENOTSUP);

	/*
	 * The I2C middle layer has ensured that the client device has
	 * exclusive access to the controller.  Copy the parameters
	 * and start the state machine that runs through the necessary
	 * phases of the request.
	 */
	KASSERT(sc->sc_exec_state == BSC_EXEC_STATE_IDLE);
	sc->sc_exec.op = op;
	sc->sc_exec.addr = addr;
	sc->sc_exec.cmdbuf = cmdbuf;
	sc->sc_exec.cmdlen = cmdlen;
	sc->sc_exec.databuf = databuf;
	sc->sc_exec.datalen = datalen;
	sc->sc_exec.flags = flags;

	bsciic_exec_lock(sc);
	(*bsciic_exec_state_data[sc->sc_exec_state].func)(sc);
	while (sc->sc_exec_state < BSC_EXEC_STATE_DONE) {
		KASSERT((flags & I2C_F_POLL) == 0);
		cv_wait(&sc->sc_intr_wait, &sc->sc_intr_lock);
	}
	int error = sc->sc_exec_state == BSC_EXEC_STATE_ERROR ? EIO : 0;
	uint32_t s;
	do {
		s = bus_space_read_4(sc->sc_iot, sc->sc_ioh, BSC_S);
	} while ((s & BSC_S_TA) != 0);
	if (s & (BSC_S_CLKT | BSC_S_ERR)) {
		error = EIO;
	}
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_C,
	    BSC_C_I2CEN | BSC_C_CLEAR_CLEAR);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_S,
	    BSC_S_CLKT | BSC_S_ERR | BSC_S_DONE);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BSC_DLEN, 0);
	bsciic_exec_unlock(sc);

	sc->sc_exec.flags = 0;
	sc->sc_exec_state = BSC_EXEC_STATE_IDLE;
	memset(&sc->sc_exec, 0, sizeof(sc->sc_exec));

	return error;
}
