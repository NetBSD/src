/*	$NetBSD: ki2c.c,v 1.43 2025/09/21 18:03:28 thorpej Exp $	*/
/*	Id: ki2c.c,v 1.7 2002/10/05 09:56:05 tsubai Exp	*/

/*-
 * Copyright (c) 2001 Tsubai Masanari.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/mutex.h>

#include <dev/ofw/openfirm.h>
#include <machine/autoconf.h>
#include <powerpc/pic/picvar.h>

#include "opt_ki2c.h"
#include <macppc/dev/ki2cvar.h>

#ifdef KI2C_DEBUG
#define DPRINTF printf
#else
#define DPRINTF while (0) printf
#endif

#define KI2C_EXEC_MAX_CMDLEN	32
#define KI2C_EXEC_MAX_BUFLEN	32

int ki2c_match(device_t, cfdata_t, void *);
void ki2c_attach(device_t, device_t, void *);
inline uint8_t ki2c_readreg(struct ki2c_softc *, int);
inline void ki2c_writereg(struct ki2c_softc *, int, uint8_t);
u_int ki2c_getmode(struct ki2c_softc *);
void ki2c_setmode(struct ki2c_softc *, u_int);
u_int ki2c_getspeed(struct ki2c_softc *);
void ki2c_setspeed(struct ki2c_softc *, u_int);
int ki2c_intr(void *);
int ki2c_poll(struct ki2c_softc *, int);
int ki2c_start(struct ki2c_softc *, int, int, void *, int);
int ki2c_read(struct ki2c_softc *, int, int, void *, int);
int ki2c_write(struct ki2c_softc *, int, int, void *, int);

/* I2C glue */
static int ki2c_i2c_exec(void *, i2c_op_t, i2c_addr_t, const void *, size_t,
		    void *, size_t, int);

static void
ki2c_select_channel(struct ki2c_softc *sc, int channel)
{
	uint8_t val = channel ? 0x10 : 0x00;
	
	/* We handle the subaddress stuff ourselves. */
	ki2c_setmode(sc, val | I2C_STDMODE);	
}

static int
ki2c_acquire_bus(void *v, int flags)
{
	struct ki2c_channel *ch = v;
	struct ki2c_softc *sc = ch->ch_sc;

	int error = iic_acquire_bus_lock(&sc->sc_mux_lock, flags);
	if (error) {
		return error;
	}

	ki2c_select_channel(sc, ch->ch_channel);
	return 0;
}

static void
ki2c_release_bus(void *v, int flags)
{
	struct ki2c_channel *ch = v;
	struct ki2c_softc *sc = ch->ch_sc;

	iic_release_bus_lock(&sc->sc_mux_lock);
}

static void
ki2c_init_channel(struct ki2c_softc *sc, int channel, int node)
{
	struct ki2c_channel *ch = &sc->sc_channels[channel];

	iic_tag_init(&ch->ch_i2c);
	ch->ch_i2c.ic_channel = channel;
	ch->ch_i2c.ic_cookie = ch;
	ch->ch_i2c.ic_exec = ki2c_i2c_exec;
	ch->ch_i2c.ic_acquire_bus = ki2c_acquire_bus;
	ch->ch_i2c.ic_release_bus = ki2c_release_bus;

	ch->ch_sc = sc;
	ch->ch_node = node;
	ch->ch_channel = channel;
}

CFATTACH_DECL_NEW(ki2c, sizeof(struct ki2c_softc), ki2c_match, ki2c_attach,
	NULL, NULL);

int
ki2c_match(device_t parent, cfdata_t match, void *aux)
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, "i2c") == 0)
		return 1;

	return 0;
}

void
ki2c_attach(device_t parent, device_t self, void *aux)
{
	struct ki2c_softc *sc = device_private(self);
	struct confargs *ca = aux;
	int node = ca->ca_node, root;
	uint32_t addr, channel, intr[2];
	int rate, child;
	int intrparent;
	char name[32], intr_xname[32], model[32];
	uint32_t picbase;

	sc->sc_dev = self;
	sc->sc_tag = ca->ca_tag;
	ca->ca_reg[0] += ca->ca_baseaddr;

	root = OF_finddevice("/");
	model[0] = 0;
	OF_getprop(root, "model", model, 32);
	DPRINTF("model %s\n", model);
	if (OF_getprop(node, "AAPL,i2c-rate", &rate, 4) != 4) {
		aprint_error(": cannot get i2c-rate\n");
		return;
	}
	if (OF_getprop(node, "AAPL,address", &addr, 4) != 4) {
		aprint_error(": unable to find i2c address\n");
		return;
	}
	if (bus_space_map(sc->sc_tag, addr, PAGE_SIZE, 0, &sc->sc_bh) != 0) {
		aprint_error_dev(sc->sc_dev, "failed to map registers\n");
		return;
	}

	if (OF_getprop(node, "AAPL,address-step", &sc->sc_regstep, 4) != 4) {
		aprint_error(": unable to find i2c address step\n");
		return;
	}

	if(OF_getprop(node, "interrupts", intr, 8) != 8) {
		aprint_error(": can't find interrupt\n");
		return;
	}

	/*
	 * on some G5 we have two openpics, one in mac-io, one in /u3
	 * in order to get interrupts we need to know which one we're
	 * connected to
	 */
	sc->sc_poll = 0;

	if(OF_getprop(node, "interrupt-parent", &intrparent, 4) == 4) {
		uint32_t preg[8];
		struct pic_ops *pic;
		
		sc->sc_poll = 1;
		if(OF_getprop(intrparent, "reg", preg, 8) > 4) {
			/* now look for a pic with that base... */
			picbase = preg[0];
			if ((picbase & 0x80000000) == 0) {
				/* some OF versions have the openpic's reg as
				 * an offset into mac-io just to be annoying */
				int mio = OF_parent(intrparent);
				if (OF_getprop(mio, "ranges", preg, 20) == 20)
					picbase += preg[3];
			}
			DPRINTF("PIC base %08x\n", picbase);
			pic = find_pic_by_cookie((void *)picbase);
			if (pic != NULL) {
				sc->sc_poll = 0;
				intr[0] += pic->pic_intrbase;
			}
		}
	}

	if (sc->sc_poll) {
		aprint_normal(" polling");
	} else aprint_normal(" irq %d", intr[0]);

	printf("\n");

	ki2c_writereg(sc, STATUS, 0);
	ki2c_writereg(sc, ISR, 0);
	ki2c_writereg(sc, IER, 0);

	ki2c_setmode(sc, I2C_STDSUBMODE);
	ki2c_setspeed(sc, I2C_100kHz);		/* XXX rate */

	ki2c_writereg(sc, IER,I2C_INT_DATA|I2C_INT_ADDR|I2C_INT_STOP);

	/*
	 * Newer OpenFirmware will have "i2c-bus" nodes below the controller
	 * node.  If we don't find any, then the devices are direct children
	 * of the controller node, and the channel number is encoded in
	 * bit 8 of the i2c address cell (see ofw_i2c_machdep.c).
	 */
	bool found_busnode = false;
	for (child = OF_child(node); child != 0; child = OF_peer(child)) {
		OF_getprop(child, "name", name, sizeof(name));
		if (strcmp(name, "i2c-bus") == 0) {
			OF_getprop(child, "reg", &channel, sizeof(channel));
			DPRINTF("found channel %x\n", channel);
			if (channel >= KI2C_MAX_CHANNELS) {
				aprint_error_dev(self,
				    "ignoring invalid I2C channel %u\n",
				    channel);
				continue;
			}
			ki2c_init_channel(sc, channel, child);
			found_busnode = true;
		}
	}
	if (!found_busnode) {
		for (channel = 0; channel < KI2C_MAX_CHANNELS; channel++) {
			ki2c_init_channel(sc, channel, node);
		}
	}

	/*
	 * The Keywest I2C controller has a built-in I2C mux, but it's not
	 * really a distinct device in the device tree, so we handle it here.
	 *
	 * The locking order is:
	 *
	 *	iic bus mutex -> mux_lock
	 *
	 * mux_lock is taken in ki2c_acquire_bus().
	 */
	mutex_init(&sc->sc_mux_lock, MUTEX_DEFAULT, IPL_NONE);

	cv_init(&sc->sc_todev, device_xname(self));
	mutex_init(&sc->sc_todevmtx, MUTEX_DEFAULT, IPL_NONE);

	if(sc->sc_poll == 0) {
		snprintf(intr_xname, sizeof(intr_xname), "%s intr", device_xname(self));
		intr_establish_xname(intr[0], (intr[1] & 1) ? IST_LEVEL : IST_EDGE,
		    IPL_BIO, ki2c_intr, sc, intr_xname);

		ki2c_writereg(sc, IER, I2C_INT_DATA | I2C_INT_ADDR| I2C_INT_STOP);
	}

	for (channel = 0; channel < KI2C_MAX_CHANNELS; channel++) {
		if (sc->sc_channels[channel].ch_node == 0) {
			continue;
		}
		iicbus_attach_with_devhandle(sc->sc_dev,
		    &sc->sc_channels[channel].ch_i2c,
		    devhandle_from_of(device_handle(sc->sc_dev),
				      sc->sc_channels[channel].ch_node));
	}
}

uint8_t
ki2c_readreg(struct ki2c_softc *sc, int reg)
{

	return bus_space_read_1(sc->sc_tag, sc->sc_bh, sc->sc_regstep * reg);
}

void
ki2c_writereg(struct ki2c_softc *sc, int reg, uint8_t val)
{
	
	bus_space_write_1(sc->sc_tag, sc->sc_bh, reg * sc->sc_regstep, val);
	delay(10);
}

u_int
ki2c_getmode(struct ki2c_softc *sc)
{
	return ki2c_readreg(sc, MODE) & I2C_MODE;
}

void
ki2c_setmode(struct ki2c_softc *sc, u_int mode)
{
	ki2c_writereg(sc, MODE, mode);
}

u_int
ki2c_getspeed(struct ki2c_softc *sc)
{
	return ki2c_readreg(sc, MODE) & I2C_SPEED;
}

void
ki2c_setspeed(struct ki2c_softc *sc, u_int speed)
{
	u_int x;

	KASSERT((speed & ~I2C_SPEED) == 0);
	x = ki2c_readreg(sc, MODE);
	x &= ~I2C_SPEED;
	x |= speed;
	ki2c_writereg(sc, MODE, x);
}

int
ki2c_intr(void *cookie)
{
	struct ki2c_softc *sc = cookie;
	u_int isr, x;
	isr = ki2c_readreg(sc, ISR);
	if (isr & I2C_INT_ADDR) {
#if 0
		if ((ki2c_readreg(sc, STATUS) & I2C_ST_LASTAAK) == 0) {
			/* No slave responded. */
			sc->sc_flags |= I2C_ERROR;
			goto out;
		}
#endif

		if (sc->sc_flags & I2C_READING) {
			if (sc->sc_resid > 1) {
				x = ki2c_readreg(sc, CONTROL);
				x |= I2C_CT_AAK;
				ki2c_writereg(sc, CONTROL, x);
			}
		} else {
			ki2c_writereg(sc, DATA, *sc->sc_data++);
			sc->sc_resid--;
		}
	}

	if (isr & I2C_INT_DATA) {
		if (sc->sc_flags & I2C_READING) {
			*sc->sc_data++ = ki2c_readreg(sc, DATA);
			sc->sc_resid--;

			if (sc->sc_resid == 0) {	/* Completed */
				ki2c_writereg(sc, CONTROL, 0);
				goto out;
			}
		} else {
#if 0
			if ((ki2c_readreg(sc, STATUS) & I2C_ST_LASTAAK) == 0) {
				/* No slave responded. */
				sc->sc_flags |= I2C_ERROR;
				goto out;
			}
#endif

			if (sc->sc_resid == 0) {
				x = ki2c_readreg(sc, CONTROL) | I2C_CT_STOP;
				ki2c_writereg(sc, CONTROL, x);
			} else {
				ki2c_writereg(sc, DATA, *sc->sc_data++);
				sc->sc_resid--;
			}
		}
	}

out:
	if (isr & I2C_INT_STOP) {
		ki2c_writereg(sc, CONTROL, 0);
		sc->sc_flags &= ~I2C_BUSY;
		cv_signal(&sc->sc_todev);
	}

	ki2c_writereg(sc, ISR, isr);

	return 1;
}

int
ki2c_poll(struct ki2c_softc *sc, int timo)
{
	int bail = 0;
	while (sc->sc_flags & I2C_BUSY) {
		if ((cold) || (bail > 10) || (sc->sc_poll)) {
			if (ki2c_readreg(sc, ISR))
				ki2c_intr(sc);
			timo -= 10;
			if (timo < 0) {
				DPRINTF("i2c_poll: timeout\n");
				return -1;
			}
			delay(10);
		} else {
			mutex_enter(&sc->sc_todevmtx);
			cv_timedwait_sig(&sc->sc_todev, &sc->sc_todevmtx, hz/10);
			mutex_exit(&sc->sc_todevmtx);
			bail++;
		}
	}
	return 0;
}

int
ki2c_start(struct ki2c_softc *sc, int addr, int subaddr, void *data, int len)
{
	int rw = (sc->sc_flags & I2C_READING) ? 1 : 0;
	int timo, x;

	KASSERT((addr & 1) == 0);

	sc->sc_data = data;
	sc->sc_resid = len;
	sc->sc_flags |= I2C_BUSY;

	timo = 1000 + len * 200;

	/* XXX TAS3001 sometimes takes 50ms to finish writing registers. */
	/* if (addr == 0x68) */
		timo += 100000;

	ki2c_writereg(sc, ADDR, addr | rw);
	ki2c_writereg(sc, SUBADDR, subaddr);

	x = ki2c_readreg(sc, CONTROL) | I2C_CT_ADDR;
	ki2c_writereg(sc, CONTROL, x);

	if (ki2c_poll(sc, timo))
		return -1;
	if (sc->sc_flags & I2C_ERROR) {
		DPRINTF("I2C_ERROR\n");
		return -1;
	}
	return 0;
}

int
ki2c_read(struct ki2c_softc *sc, int addr, int subaddr, void *data, int len)
{
	sc->sc_flags = I2C_READING;
	DPRINTF("ki2c_read: %02x %d\n", addr, len);
	return ki2c_start(sc, addr, subaddr, data, len);
}

int
ki2c_write(struct ki2c_softc *sc, int addr, int subaddr, void *data, int len)
{
	sc->sc_flags = 0;
	DPRINTF("ki2c_write: %02x %d\n",addr,len);
	return ki2c_start(sc, addr, subaddr, data, len);
}

int
ki2c_i2c_exec(void *cookie, i2c_op_t op, i2c_addr_t addr, const void *vcmd,
    size_t cmdlen, void *vbuf, size_t buflen, int flags)
{
	struct ki2c_channel *ch = cookie;
	struct ki2c_softc *sc = ch->ch_sc;
	int i;
	size_t w_len;
	uint8_t *wp;
	uint8_t wrbuf[KI2C_EXEC_MAX_CMDLEN + KI2C_EXEC_MAX_CMDLEN];

	/*
	 * We don't have any idea if the ki2c controller can execute
	 * i2c quick_{read,write} operations, so if someone tries one,
	 * return an error.
	 */
	if (cmdlen == 0 && buflen == 0)
		return -1;

	/*
	 * Transaction could be much larger now. Bail if it exceeds our
	 * small combining buffer, we don't expect such devices.
	 */
	if (cmdlen + buflen > sizeof(wrbuf))
		return -1;

	addr &= 0x7f;	/* KASSERT((addr & ~0x7f) == 0); ??? */

	ki2c_setspeed(sc, I2C_50kHz);

	/* Write-buffer defaults to vcmd */
	wp = (uint8_t *)(__UNCONST(vcmd));
	w_len = cmdlen;

	/*
	 * Concatenate vcmd and vbuf for write operations
	 *
	 * Drivers written specifically for ki2c might already do this,
	 * but "generic" i2c drivers still provide separate arguments
	 * for the cmd and buf parts of iic_smbus_write_{byte,word}.
	 */
	if (I2C_OP_WRITE_P(op) && buflen != 0) {
		if (cmdlen == 0) {
			wp = (uint8_t *)vbuf;
			w_len = buflen;
		} else {
			KASSERT((cmdlen + buflen) <= sizeof(wrbuf));
			wp = (uint8_t *)(__UNCONST(vcmd));
			w_len = 0;
			for (i = 0; i < cmdlen; i++)
				wrbuf[w_len++] = *wp++;
			wp = (uint8_t *)vbuf;
			for (i = 0; i < buflen; i++)
				wrbuf[w_len++] = *wp++;
			wp = wrbuf;
		}
	}

	if (w_len > 0)
		if (ki2c_write(sc, addr << 1, 0, wp, w_len) !=0 )
			return -1;

	if (I2C_OP_READ_P(op)) {
		if (ki2c_read(sc, addr << 1, 0, vbuf, buflen) !=0 )
			return -1;
	}
	return 0;
}
