/*	$NetBSD: ki2c.c,v 1.8.34.2 2008/01/09 01:47:09 matt Exp $	*/
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
#include <uvm/uvm_extern.h>
#include <machine/autoconf.h>

#include <macppc/dev/ki2cvar.h>

int ki2c_match(struct device *, struct cfdata *, void *);
void ki2c_attach(struct device *, struct device *, void *);
inline u_int ki2c_readreg(struct ki2c_softc *, int);
inline void ki2c_writereg(struct ki2c_softc *, int, u_int);
u_int ki2c_getmode(struct ki2c_softc *);
void ki2c_setmode(struct ki2c_softc *, u_int);
u_int ki2c_getspeed(struct ki2c_softc *);
void ki2c_setspeed(struct ki2c_softc *, u_int);
int ki2c_intr(struct ki2c_softc *);
int ki2c_poll(struct ki2c_softc *, int);
int ki2c_start(struct ki2c_softc *, int, int, void *, int);
int ki2c_read(struct ki2c_softc *, int, int, void *, int);
int ki2c_write(struct ki2c_softc *, int, int, void *, int);
int ki2c_print __P((void *, const char *));

/* I2C glue */
static int ki2c_i2c_acquire_bus(void *, int);
static void ki2c_i2c_release_bus(void *, int);
static int ki2c_i2c_exec(void *, i2c_op_t, i2c_addr_t, const void *, size_t,
		    void *, size_t, int);


CFATTACH_DECL(ki2c, sizeof(struct ki2c_softc), ki2c_match, ki2c_attach,
	NULL, NULL);

int
ki2c_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, "i2c") == 0)
		return 1;

	return 0;
}

void
ki2c_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct ki2c_softc *sc = (struct ki2c_softc *)self;
	struct confargs *ca = aux;
	int node = ca->ca_node;
	int rate, child, namelen, i2cbus;
	struct ki2c_confargs ka;
	struct i2cbus_attach_args iba;

	char name[32];
	u_int reg[20];
	
	ca->ca_reg[0] += ca->ca_baseaddr;

	if (OF_getprop(node, "AAPL,i2c-rate", &rate, 4) != 4) {
		printf(": cannot get i2c-rate\n");
		return;
	}
	if (OF_getprop(node, "AAPL,address", &sc->sc_reg, 4) != 4) {
		printf(": unable to find i2c address\n");
		return;
	}
	if (OF_getprop(node, "AAPL,address-step", &sc->sc_regstep, 4) != 4) {
		printf(": unable to find i2c address step\n");
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

	iba.iba_tag = &sc->sc_i2c;
	(void) config_found_ia(&sc->sc_dev, "i2cbus", &iba, iicbus_print);

	/* 
	 * newer OF puts I2C devices under 'i2c-bus' instead of attaching them 
	 * directly to the ki2c node so we just check if we have a child named
	 * 'i2c-bus' and if so we attach its children, not ours 
	 */
	i2cbus = 0;
	child = OF_child(node);
	while ((child != 0) && (i2cbus == 0)) {
		OF_getprop(child, "name", name, sizeof(name));
		if (strcmp(name, "i2c-bus") == 0)
			i2cbus = child;
		child = OF_peer(child);
	}
	if (i2cbus == 0) 
		i2cbus = node;
		
	for (child = OF_child(i2cbus); child; child = OF_peer(child)) {
		int ok = 0;
		namelen = OF_getprop(child, "name", name, sizeof(name));
		if (namelen < 0)
			continue;
		if (namelen >= sizeof(name))
			continue;

		name[namelen] = 0;
		ka.ka_name = name;
		ka.ka_node = child;
		ok = OF_getprop(child, "reg", reg, sizeof(reg));
		if (ok <= 0) {
			ok = OF_getprop(child, "i2c-address", reg,
			    sizeof(reg));
		}
		if (ok > 0) {
			ka.ka_addr = reg[0];
			ka.ka_tag = &sc->sc_i2c;	
			config_found_ia(self, "ki2c", &ka, ki2c_print);
		} 
#ifdef DIAGNOSTIC
		else {
			printf("%s: device (%s) has no reg or i2c-address property.\n",
			    sc->sc_dev.dv_xname, name);
		}
#endif
	}
}

int
ki2c_print(aux, ki2c)
	void *aux;
	const char *ki2c;
{
	struct ki2c_confargs *ka = aux;

	if (ki2c) {
		aprint_normal("%s at %s", ka->ka_name, ki2c);
		aprint_normal(" address 0x%x", ka->ka_addr);
	}
	return UNCONF;
}

u_int
ki2c_readreg(sc, reg)
	struct ki2c_softc *sc;
	int reg;
{
	u_char *addr = sc->sc_reg + sc->sc_regstep * reg;

	return *addr;
}

void
ki2c_writereg(sc, reg, val)
	struct ki2c_softc *sc;
	int reg;
	u_int val;
{
	u_char *addr = sc->sc_reg + sc->sc_regstep * reg;

	*addr = val;
	__asm volatile ("eieio");
	delay(10);
}

u_int
ki2c_getmode(sc)
	struct ki2c_softc *sc;
{
	return ki2c_readreg(sc, MODE) & I2C_MODE;
}

void
ki2c_setmode(sc, mode)
	struct ki2c_softc *sc;
	u_int mode;
{
	u_int x;

	KASSERT((mode & ~I2C_MODE) == 0);
	x = ki2c_readreg(sc, MODE);
	x &= ~I2C_MODE;
	x |= mode;
	ki2c_writereg(sc, MODE, x);
}

u_int
ki2c_getspeed(sc)
	struct ki2c_softc *sc;
{
	return ki2c_readreg(sc, MODE) & I2C_SPEED;
}

void
ki2c_setspeed(sc, speed)
	struct ki2c_softc *sc;
	u_int speed;
{
	u_int x;

	KASSERT((speed & ~I2C_SPEED) == 0);
	x = ki2c_readreg(sc, MODE);
	x &= ~I2C_SPEED;
	x |= speed;
	ki2c_writereg(sc, MODE, x);
}

int
ki2c_intr(sc)
	struct ki2c_softc *sc;
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
ki2c_poll(sc, timo)
	struct ki2c_softc *sc;
	int timo;
{
	while (sc->sc_flags & I2C_BUSY) {
		if (ki2c_readreg(sc, ISR))
			ki2c_intr(sc);
		timo -= 100;
		if (timo < 0) {
			printf("i2c_poll: timeout\n");
			return -1;
		}
		delay(100);
	}
	return 0;
}

int
ki2c_start(sc, addr, subaddr, data, len)
	struct ki2c_softc *sc;
	int addr, subaddr, len;
	void *data;
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
		printf("I2C_ERROR\n");
		return -1;
	}
	return 0;
}

int
ki2c_read(sc, addr, subaddr, data, len)
	struct ki2c_softc *sc;
	int addr, subaddr, len;
	void *data;
{
	sc->sc_flags = I2C_READING;
	#ifdef KI2C_DEBUG
		printf("ki2c_read: %02x %d\n", addr, len);
	#endif
	return ki2c_start(sc, addr, subaddr, data, len);
}

int
ki2c_write(sc, addr, subaddr, data, len)
	struct ki2c_softc *sc;
	int addr, subaddr, len;
	void *data;
{
	sc->sc_flags = 0;
	#ifdef KI2C_DEBUG
		printf("ki2c_write: %02x %d\n",addr,len);
	#endif
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
	
	/* we handle the subaddress stuff ourselves */
	ki2c_setmode(sc, I2C_STDMODE);	

	if (ki2c_write(sc, addr, 0, __UNCONST(vcmd), cmdlen) !=0 )
		return -1;

	if (I2C_OP_READ_P(op)) {
		if (ki2c_read(sc, addr, 0, vbuf, buflen) !=0 )
			return -1;
	}
	return 0;
}
