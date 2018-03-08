/*	$NetBSD: ki2c.c,v 1.25 2018/03/08 21:53:20 macallan Exp $	*/
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

#include <macppc/dev/ki2cvar.h>

#ifdef KI2C_DEBUG
#define DPRINTF printf
#else
#define DPRINTF while (0) printf
#endif

int ki2c_match(device_t, cfdata_t, void *);
void ki2c_attach(device_t, device_t, void *);
inline uint8_t ki2c_readreg(struct ki2c_softc *, int);
inline void ki2c_writereg(struct ki2c_softc *, int, uint8_t);
u_int ki2c_getmode(struct ki2c_softc *);
void ki2c_setmode(struct ki2c_softc *, u_int);
u_int ki2c_getspeed(struct ki2c_softc *);
void ki2c_setspeed(struct ki2c_softc *, u_int);
int ki2c_intr(struct ki2c_softc *);
int ki2c_poll(struct ki2c_softc *, int);
int ki2c_start(struct ki2c_softc *, int, int, void *, int);
int ki2c_read(struct ki2c_softc *, int, int, void *, int);
int ki2c_write(struct ki2c_softc *, int, int, void *, int);

/* I2C glue */
static int ki2c_i2c_acquire_bus(void *, int);
static void ki2c_i2c_release_bus(void *, int);
static int ki2c_i2c_exec(void *, i2c_op_t, i2c_addr_t, const void *, size_t,
		    void *, size_t, int);


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
	int node = ca->ca_node;
	uint32_t addr, channel;
	int rate, child, /*namelen,*/ i2cbus;
	struct i2cbus_attach_args iba;
	prop_dictionary_t dict = device_properties(self);
	prop_array_t cfg;
	int devs;
	char compat[256];
	prop_dictionary_t dev;
	prop_data_t data;
	char name[32];

	sc->sc_dev = self;
	sc->sc_tag = ca->ca_tag;
	ca->ca_reg[0] += ca->ca_baseaddr;

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

	printf("\n");

	ki2c_writereg(sc, STATUS, 0);
	ki2c_writereg(sc, ISR, 0);
	ki2c_writereg(sc, IER, 0);

	ki2c_setmode(sc, I2C_STDSUBMODE);
	ki2c_setspeed(sc, I2C_100kHz);		/* XXX rate */
	
	mutex_init(&sc->sc_buslock, MUTEX_DEFAULT, IPL_NONE);
	ki2c_writereg(sc, IER,I2C_INT_DATA|I2C_INT_ADDR|I2C_INT_STOP);
	
	cfg = prop_array_create();
	prop_dictionary_set(dict, "i2c-child-devices", cfg);
	prop_object_release(cfg);

	/* 
	 * newer OF puts I2C devices under 'i2c-bus' instead of attaching them 
	 * directly to the ki2c node so we just check if we have a child named
	 * 'i2c-bus' and if so we attach its children, not ours
	 *
	 * XXX
	 * should probably check for multiple i2c-bus children
	 */
	i2cbus = 0;
	channel = 0;
	child = OF_child(node);
	while ((child != 0) && (i2cbus == 0)) {
		OF_getprop(child, "name", name, sizeof(name));
		if (strcmp(name, "i2c-bus") == 0) {
			OF_getprop(child, "reg", &channel, sizeof(channel));
			i2cbus = child;
			DPRINTF("found channel %x\n", channel);
		}
		child = OF_peer(child);
	}
	if (i2cbus == 0) 
		i2cbus = node;

	devs = OF_child(i2cbus);
	while (devs != 0) {
		if (OF_getprop(devs, "name", name, 32) <= 0)
			goto skip;
		if (OF_getprop(devs, "compatible", compat, 256) <= 0) {
			/* some i2c device nodes don't have 'compatible' */
			memset(compat, 0, 256);
			strncpy(compat, name, 256);
		} 
		if (OF_getprop(devs, "reg", &addr, 4) <= 0)
			if (OF_getprop(devs, "i2c-address", &addr, 4) <= 0)
				goto skip;
		addr |= channel << 8;
		addr = addr >> 1;
		DPRINTF("-> %s@%x\n", name, addr);
		dev = prop_dictionary_create();
		prop_dictionary_set_cstring(dev, "name", name);
		data = prop_data_create_data(compat, strlen(compat)+1);
		prop_dictionary_set(dev, "compatible", data);
		prop_object_release(data);
		prop_dictionary_set_uint32(dev, "addr", addr);
		prop_dictionary_set_uint64(dev, "cookie", devs);
		prop_array_add(cfg, dev);
		prop_object_release(dev);
	skip:
		devs = OF_peer(devs);
	}

	/* fill in the i2c tag */
	sc->sc_i2c.ic_cookie = sc;
	sc->sc_i2c.ic_acquire_bus = ki2c_i2c_acquire_bus;
	sc->sc_i2c.ic_release_bus = ki2c_i2c_release_bus;
	sc->sc_i2c.ic_send_start = NULL;
	sc->sc_i2c.ic_send_stop = NULL;
	sc->sc_i2c.ic_initiate_xfer = NULL;
	sc->sc_i2c.ic_read_byte = NULL;
	sc->sc_i2c.ic_write_byte = NULL;
	sc->sc_i2c.ic_exec = ki2c_i2c_exec;

	memset(&iba, 0, sizeof(iba));
	iba.iba_tag = &sc->sc_i2c;
	(void) config_found_ia(sc->sc_dev, "i2cbus", &iba, iicbus_print);
		
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
ki2c_intr(struct ki2c_softc *sc)
{
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
	}

	ki2c_writereg(sc, ISR, isr);

	return 1;
}

int
ki2c_poll(struct ki2c_softc *sc, int timo)
{
	while (sc->sc_flags & I2C_BUSY) {
		if (ki2c_readreg(sc, ISR))
			ki2c_intr(sc);
		timo -= 100;
		if (timo < 0) {
			DPRINTF("i2c_poll: timeout\n");
			return -1;
		}
		delay(100);
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

static int
ki2c_i2c_acquire_bus(void *cookie, int flags)
{
	struct ki2c_softc *sc = cookie;

	mutex_enter(&sc->sc_buslock);
	return 0;
}

static void
ki2c_i2c_release_bus(void *cookie, int flags)
{
	struct ki2c_softc *sc = cookie;

	mutex_exit(&sc->sc_buslock);
}

int
ki2c_i2c_exec(void *cookie, i2c_op_t op, i2c_addr_t addr, const void *vcmd,
    size_t cmdlen, void *vbuf, size_t buflen, int flags)
{
	struct ki2c_softc *sc = cookie;
	int i;
	size_t w_len;
	uint8_t *wp;
	uint8_t wrbuf[I2C_EXEC_MAX_CMDLEN + I2C_EXEC_MAX_CMDLEN];
	uint8_t channel;

	/*
	 * We don't have any idea if the ki2c controller can execute
	 * i2c quick_{read,write} operations, so if someone tries one,
	 * return an error.
	 */
	if (cmdlen == 0 && buflen == 0)
		return -1;

	channel = (addr & 0xf80) ? 0x10 : 0x00;
	addr &= 0x7f;
	

	/* we handle the subaddress stuff ourselves */
	ki2c_setmode(sc, channel | I2C_STDMODE);	

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
