/*	$NetBSD: pcf8584.c,v 1.26 2026/09/16 06:51:01 jdc Exp $	*/
/*	$OpenBSD: pcf8584.c,v 1.9 2007/10/20 18:46:21 kettenis Exp $ */

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
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/bus.h>

#include <dev/i2c/i2cvar.h>

#include <dev/ic/pcf8584var.h>
#include <dev/ic/pcf8584reg.h>

void		pcfiic_init(struct pcfiic_softc *);
int		pcfiic_i2c_acquire_bus(void *, int);
void		pcfiic_i2c_release_bus(void *, int);
int		pcfiic_i2c_exec(void *, i2c_op_t, i2c_addr_t, const void *,
		    size_t, void *, size_t, int);

int		pcfiic_xmit(struct pcfiic_softc *, const u_int8_t *, size_t,
		    const u_int8_t *, size_t);
int		pcfiic_recv(struct pcfiic_softc *, u_int8_t *, size_t);

u_int8_t	pcfiic_read(struct pcfiic_softc *, bus_size_t);
void		pcfiic_write(struct pcfiic_softc *, bus_size_t, u_int8_t);
void		pcfiic_choose_bus(struct pcfiic_softc *, u_int8_t);
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
pcfiic_attach(struct pcfiic_softc *sc, i2c_addr_t addr, u_int8_t clock)
{
	sc->sc_clock = clock;
	sc->sc_addr = addr;

	pcfiic_init(sc);

	if (sc->sc_has_mux)
		pcfiic_choose_bus(sc, 0);

	iic_tag_init(&sc->sc_i2c);
	sc->sc_i2c.ic_cookie = sc;
	sc->sc_i2c.ic_exec = pcfiic_i2c_exec;

	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, IPL_VM);
	cv_init(&sc->sc_cv, "pcfiic");

	iicbus_attach(sc->sc_dev, &sc->sc_i2c);
}

int
pcfiic_intr(void *arg)
{
	struct pcfiic_softc	*sc = arg;
	u_int8_t 		s1, r;

	mutex_enter(&sc->sc_mutex);

	if (sc->sc_stage == PCFIIC_STAGE_IDLE) {
		/* printf("%s: intr and idle\n", device_xname(sc->sc_dev)); */
		mutex_exit(&sc->sc_mutex);
		return (0);
	}

	s1 = pcfiic_read(sc, PCF8584_S1);
	if ((s1 & PCF8584_STATUS_PIN) != 0) {
		/* printf("%s: intr +PIN (0x%02x)\n",
		    device_xname(sc->sc_dev), s1); */
		mutex_exit(&sc->sc_mutex);
		return (0);
	}

	switch (sc->sc_stage) {
	case PCFIIC_STAGE_WRITE:
		if (s1 & PCF8584_STATUS_LRB) {
			pcfiic_write(sc, PCF8584_S1, PCF8584_CMD_STOP);
			sc->sc_err = 1;
			sc->sc_stage = PCFIIC_STAGE_STOP;
			break;
		}

		/* Start the write from cmd */
		if (sc->sc_i < sc->sc_cmdlen) {
			pcfiic_write(sc, PCF8584_S0, sc->sc_cmdbuf[sc->sc_i]);
			sc->sc_i++;
			break;
		}

		/* If a read OP, switch with repeat start after writing cmd */
		if (sc->sc_op == PCFIIC_OP_WR_RD) {
			sc->sc_i = 0;
			sc->sc_stage = PCFIIC_STAGE_READ;
			pcfiic_write(sc, PCF8584_S1,
			    PCF8584_CMD_REPSTART | PCF8584_CTRL_ENI);
			pcfiic_write(sc, PCF8584_S0,
			    (sc->sc_target << 1) | 0x01);
			break;
		}

		/* If a write OP, continue the write, now from buf */
		if (sc->sc_i < sc->sc_cmdlen + sc->sc_len) {
			pcfiic_write(sc, PCF8584_S0,
			    sc->sc_buf[sc->sc_i - sc->sc_cmdlen]);
			sc->sc_i++;
		} else {
			/* All data written, send stop */
			pcfiic_write(sc, PCF8584_S1, PCF8584_CMD_STOP);
			sc->sc_stage = PCFIIC_STAGE_STOP;
		}
		break;

	case PCFIIC_STAGE_READ:
		if ((sc->sc_i != sc->sc_len) && (s1 & PCF8584_STATUS_LRB)) {
			pcfiic_write(sc, PCF8584_S1, PCF8584_CMD_STOP);
			pcfiic_read(sc, PCF8584_S0);
			sc->sc_err = 1;
			sc->sc_stage = PCFIIC_STAGE_STOP;
			break;
		}

		/* Read to buf, send nak on last - 1 */
		if (sc->sc_i == sc->sc_len - 1) {
			pcfiic_write(sc, PCF8584_S1,
			    PCF8584_CMD_NAK | PCF8584_CTRL_ENI);
		} else if (sc->sc_i == sc->sc_len) {
			pcfiic_write(sc, PCF8584_S1, PCF8584_CMD_STOP);
			sc->sc_stage = PCFIIC_STAGE_STOP;
		}

		/* We need a dummy read to start data shifting into S0 */
		r = pcfiic_read(sc, PCF8584_S0);
		if (sc->sc_i > 0)
			sc->sc_buf[sc->sc_i - 1] = r;

		sc->sc_i++;
		break;
	}

	if (sc->sc_stage == PCFIIC_STAGE_STOP) {
		cv_signal(&sc->sc_cv);
	}

	mutex_exit(&sc->sc_mutex);

	return (1);
}

int
pcfiic_i2c_exec(void *arg, i2c_op_t op, i2c_addr_t addr,
    const void *cmdbuf, size_t cmdlen, void *buf, size_t len, int flags)
{
	struct pcfiic_softc	*sc = arg;
	int			ret = 0;
	u_int8_t		intr;
	unsigned		deadline, rem;

#if 0
        printf("%s: exec op: %d addr: 0x%x cmdlen: %d len: %d flags 0x%x\n",
            device_xname(sc->sc_dev), op, addr, (int)cmdlen, (int)len, flags);
#endif

	if (sc->sc_poll)
		flags |= I2C_F_POLL;

	if (sc->sc_has_mux)
		pcfiic_choose_bus(sc, addr >> 7);
	addr &= 0x7f;

	if (I2C_OP_WRITE_P(op))
		sc->sc_op = PCFIIC_OP_WRITE;
	else
		if(cmdlen > 0)
			sc->sc_op = PCFIIC_OP_WR_RD;
		else
			sc->sc_op = PCFIIC_OP_READ;

	if (flags & I2C_F_POLL) {
		intr = 0;
	} else {
		/* Interrupt-driven setup */
		intr = PCF8584_CTRL_ENI;
		sc->sc_target = addr;
		sc->sc_cmdbuf = cmdbuf;
		sc->sc_cmdlen = cmdlen;
		sc->sc_buf = buf;
		sc->sc_len = len;
		sc->sc_i = 0;
		sc->sc_err = 0;
		mutex_enter(&sc->sc_mutex);
		sc->sc_stage = PCFIIC_STAGE_IDLE;
		mutex_exit(&sc->sc_mutex);
	}

	if (pcfiic_wait_BBN(sc) != 0) {
		printf("%s: transmit failed (BBN)\n", device_xname(sc->sc_dev));
		return (1);
	}

	/* Start the transaction */
	if (sc->sc_op == PCFIIC_OP_READ) {
		mutex_enter(&sc->sc_mutex);
		sc->sc_stage = PCFIIC_STAGE_READ;
		mutex_exit(&sc->sc_mutex);
		pcfiic_write(sc, PCF8584_S0, (addr << 1) | 0x01);
	} else {
		mutex_enter(&sc->sc_mutex);
		sc->sc_stage = PCFIIC_STAGE_WRITE;
		mutex_exit(&sc->sc_mutex);
		pcfiic_write(sc, PCF8584_S0, (addr << 1));
	}
	pcfiic_write(sc, PCF8584_S1, PCF8584_CMD_START | intr);

	if (flags & I2C_F_POLL) {
		/*
		 * If we are writing, write address, cmdbuf, buf.
		 * If we are reading, either:
		 *   write addr, cmdbuf, repeat-start, then read addr to buf,
		 * or:
		 *   read addr to buf.
		 */
		if (sc->sc_op == PCFIIC_OP_WRITE) {
			ret = pcfiic_xmit(sc, cmdbuf, cmdlen, buf, len);
			pcfiic_write(sc, PCF8584_S1, PCF8584_CMD_STOP);
		} else {
			if (sc->sc_op == PCFIIC_OP_WR_RD) {
				if (pcfiic_xmit(sc, cmdbuf, cmdlen,
				    NULL, 0) != 0) {
					pcfiic_write(sc, PCF8584_S1,
					    PCF8584_CMD_STOP);
					return (1);
				}
				pcfiic_write(sc, PCF8584_S1,
				    PCF8584_CMD_REPSTART);
				pcfiic_write(sc, PCF8584_S0,
				    (addr << 1) | 0x01);
			}
			/* PCFIIC_OP_READ */
			ret = pcfiic_recv(sc, buf, len);
		}
		return (ret);
	} else {	/* interrupt-driven */
		/* Wait for the transfer to complete (stop). */
		deadline = getticks() +
		    /*timeout*/ hz / 10 + 10 * (cmdlen + len);
		mutex_enter(&sc->sc_mutex);
		while (sc->sc_stage != PCFIIC_STAGE_STOP) {
			rem = deadline - getticks();
			if (!rem || rem >= INT_MAX) {
				printf("%s: intr timeout\n",
				    device_xname(sc->sc_dev));
				sc->sc_stage = PCFIIC_STAGE_IDLE;
				mutex_exit(&sc->sc_mutex);
				pcfiic_write(sc, PCF8584_S1, PCF8584_CMD_STOP);
				return (1);
			}
			cv_timedwait(&sc->sc_cv, &sc->sc_mutex, rem);
		}
		mutex_exit(&sc->sc_mutex);
		return (sc->sc_err);
	}
}

/* Polling write */
int
pcfiic_xmit(struct pcfiic_softc *sc, const u_int8_t *cmdbuf, size_t cmdlen,
    const u_int8_t *buf, size_t len)
{
	int			i;
	volatile u_int8_t	r;

	for (i = 0; i <= cmdlen + len; i++) {
		if (pcfiic_wait_pin(sc, &r) != 0) {
			printf("%s: transmit failed at %d (PIN)\n",
			    device_xname(sc->sc_dev), i);
			return (1);
		}

		if (r & PCF8584_STATUS_LRB) {
			return (1);
		}

		/* Write from cmd then buf */
		if (i < cmdlen)
			pcfiic_write(sc, PCF8584_S0, cmdbuf[i]);
		else if (i < cmdlen + len)
			pcfiic_write(sc, PCF8584_S0, buf[i - cmdlen]);
	}
	return (0);
}

/* Polling read */
int
pcfiic_recv(struct pcfiic_softc *sc, u_int8_t *buf, size_t len)
{
	int			i = 0, err = 0;
	volatile u_int8_t	r;

	for (i = 0; i <= len; i++) {
		if (pcfiic_wait_pin(sc, &r) != 0) {
			pcfiic_write(sc, PCF8584_S1, PCF8584_CMD_STOP);
			printf("%s: receive failed at %d (PIN)\n",
			    device_xname(sc->sc_dev), i);
			return (1);
		}

		if ((i != len) && (r & PCF8584_STATUS_LRB)) {
			pcfiic_write(sc, PCF8584_S1, PCF8584_CMD_STOP);
			pcfiic_read(sc, PCF8584_S0);
			return (1);
		}

		/* Read to buf, send nak on last - 1 */
		if (i == len - 1) {
			pcfiic_write(sc, PCF8584_S1, PCF8584_CMD_NAK);
		} else if (i == len) {
			pcfiic_write(sc, PCF8584_S1, PCF8584_CMD_STOP);
		}

		/* We need a dummy read to start data shifting into S0 */
		r = pcfiic_read(sc, PCF8584_S0);
		if (i > 0)
			buf[i - 1] = r;
	}
	return (err);
}

static inline void
pcfiic_delay(struct pcfiic_softc *sc)
{
	if (sc->sc_delay) {
		delay(sc->sc_delay);
	}
}

u_int8_t
pcfiic_read(struct pcfiic_softc *sc, bus_size_t r)
{
	u_int8_t val;

	bus_space_barrier(sc->sc_iot, sc->sc_ioh, sc->sc_regmap[r], 1,
	    BUS_SPACE_BARRIER_READ);
	val = bus_space_read_1(sc->sc_iot, sc->sc_ioh, sc->sc_regmap[r]);
	pcfiic_delay(sc);
	return val;
}

void
pcfiic_write(struct pcfiic_softc *sc, bus_size_t r, u_int8_t v)
{
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, sc->sc_regmap[r], v);
	pcfiic_delay(sc);
	bus_space_barrier(sc->sc_iot, sc->sc_ioh, sc->sc_regmap[PCF8584_S1], 1,
	    BUS_SPACE_BARRIER_WRITE | BUS_SPACE_BARRIER_READ);
	(void)bus_space_read_1(sc->sc_iot, sc->sc_ioh,
	    sc->sc_regmap[PCF8584_S1]);
	pcfiic_delay(sc);
}

void
pcfiic_choose_bus(struct pcfiic_softc *sc, u_int8_t bus)
{
	bus_space_write_1(sc->sc_iot, sc->sc_mux_ioh, 0, bus);
	bus_space_barrier(sc->sc_iot, sc->sc_mux_ioh, 0, 1,
	    BUS_SPACE_BARRIER_WRITE);
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
