/*	$NetBSD: ki2c.c,v 1.32.2.2 2021/09/10 15:45:28 thorpej Exp $	*/
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
#include <sys/kmem.h>
#include <sys/mutex.h>

#include <dev/ofw/openfirm.h>
#include <machine/autoconf.h>

#include "opt_ki2c.h"
#include <macppc/dev/ki2cvar.h>

#include "locators.h"

#ifdef KI2C_DEBUG
#define DPRINTF printf
#else
#define DPRINTF while (0) printf
#endif

static int	ki2c_match(device_t, cfdata_t, void *);
static void	ki2c_attach(device_t, device_t, void *);
static int	ki2c_intr(struct ki2c_softc *);

/* I2C glue */
static int	ki2c_i2c_exec(void *, i2c_op_t, i2c_addr_t, const void *,
		    size_t, void *, size_t, int);
static int	ki2c_i2c_acquire_bus(void *, int);
static void	ki2c_i2c_release_bus(void *, int);

CFATTACH_DECL_NEW(ki2c, sizeof(struct ki2c_softc), ki2c_match, ki2c_attach,
	NULL, NULL);

static prop_dictionary_t
ki2c_i2c_device_props(struct ki2c_softc *sc, int node)
{
	prop_dictionary_t props = prop_dictionary_create();
	uint32_t reg;
	char descr[32], num[8];

	/* We're fetching descriptions for sensors. */
	/* XXX This is a terrible hack and should not be done this way XXX */

	for (node = OF_child(node); node != 0; node = OF_peer(node)) {
		if (of_getprop_uint32(node, "reg", &reg) == -1) {
			continue;
		}
		if (OF_getprop(node, "location", descr, sizeof(descr)) <= 0) {
			continue;
		}
		snprintf(num, sizeof(num), "s%02x", reg);

		aprint_debug_dev(sc->sc_dev,
		    "%s: sensor %s -> %s\n", __func__, num, descr);

		prop_dictionary_set_string(props, num, descr);
	}

	return props;
}

static bool
ki2c_i2c_enumerate_device(struct ki2c_softc *sc, device_t dev, int node,
    const char *name, uint32_t addr,
    struct i2c_enumerate_devices_args * const args)
{
	int compat_size;
	prop_dictionary_t props;
	char compat_buf[32];
	char *compat;
	bool cbrv;

	compat_size = OF_getproplen(node, "compatible");
	if (compat_size <= 0) {
		/* some i2c device nodes don't have 'compatible' */
		aprint_debug_dev(sc->sc_dev,
		    "no compatible property for phandle %d; using '%s'\n",
		    node, name);
		compat = compat_buf;
		strlcpy(compat, name, sizeof(compat));
		compat_size = strlen(compat) + 1;
	} else {
		compat = kmem_tmpbuf_alloc(compat_size, compat_buf,
		    sizeof(compat_buf), KM_SLEEP);
		if (OF_getprop(node, "compatible", compat,
			      sizeof(compat)) <= 0) {
			aprint_error_dev(sc->sc_dev,
			    "unable to get compatible property for "
			    "phandle %d ('%s')\n", node, name);
			goto bad;
		}
	}

	props = ki2c_i2c_device_props(sc, node);

	args->ia->ia_addr = (i2c_addr_t)addr;
	args->ia->ia_name = name;
	args->ia->ia_clist = compat;
	args->ia->ia_clist_size = compat_size;
	args->ia->ia_prop = props;
	args->ia->ia_devhandle = devhandle_from_of(node);

	cbrv = args->callback(dev, args);

	prop_object_release(props);

	return cbrv;	/* callback decides if we keep enumerating */

 bad:
	if (compat != compat_buf) {
		kmem_tmpbuf_free(compat, compat_size, compat_buf);
	}
	return true;			/* keep enumerating */
}

static int
ki2c_i2c_enumerate_devices(device_t dev, devhandle_t call_handle, void *v)
{
	struct i2c_enumerate_devices_args *args = v;
	int bus_phandle, node;
	uint32_t addr;
	char name[32];

	/* dev is the "iic" bus instance.  ki2c channel is in args. */
	struct ki2c_channel *ch = args->ia->ia_tag->ic_cookie;
	struct ki2c_softc *sc = ch->ch_ki2c;

	/*
	 * If we're not using the separate nodes scheme, we need
	 * to filter out devices from the other channel.  We detect
	 * this by comparing the bus phandle to the controller phandle,
	 * and if they match, we are NOT using the separate nodes
	 * scheme.
	 */
	bus_phandle = devhandle_to_of(device_handle(dev));
	bool filter_by_channel =
	    bus_phandle == devhandle_to_of(device_handle(sc->sc_dev));

	for (node = OF_child(bus_phandle); node != 0; node = OF_peer(node)) {
		if (OF_getprop(node, "name", name, sizeof(name)) <= 0) {
			aprint_error_dev(sc->sc_dev,
			    "unable to get name property for phandle %d\n",
			    node);
			continue;
		} 
		if (of_getprop_uint32(node, "reg", &addr) == -1 &&
		    of_getprop_uint32(node, "i2c-address", &addr) == -1) {
			aprint_error_dev(sc->sc_dev,
			    "unable to get i2c address for phandle %d ('%s')\n",
			    node, name);
			continue;
		}
		if (filter_by_channel && ((addr >> 8) & 1) != ch->ch_channel) {
			continue;
		}
		addr = (addr & 0xff) >> 1;
		if (!ki2c_i2c_enumerate_device(sc, dev, node, name, addr,
					       args)) {
			break;
		}
	}

	return 0;
}

static device_call_t
ki2c_devhandle_lookup_device_call(devhandle_t handle, const char *name,
    devhandle_t *call_handlep)
{
	if (strcmp(name, "i2c-enumerate-devices") == 0) {
		return ki2c_i2c_enumerate_devices;
	}

	/* Defer everything else to the "super". */
	return NULL;
}

static inline uint8_t
ki2c_readreg(struct ki2c_softc *sc, int reg)
{

	return bus_space_read_1(sc->sc_tag, sc->sc_bh, sc->sc_regstep * reg);
}

static inline void
ki2c_writereg(struct ki2c_softc *sc, int reg, uint8_t val)
{
	
	bus_space_write_1(sc->sc_tag, sc->sc_bh, reg * sc->sc_regstep, val);
	delay(10);
}

#if 0
static u_int
ki2c_getmode(struct ki2c_softc *sc)
{
	return ki2c_readreg(sc, MODE) & I2C_MODE;
}
#endif

static void
ki2c_setmode(struct ki2c_softc *sc, u_int mode)
{
	ki2c_writereg(sc, MODE, mode);
}

#if 0
static u_int
ki2c_getspeed(struct ki2c_softc *sc)
{
	return ki2c_readreg(sc, MODE) & I2C_SPEED;
}
#endif

static void
ki2c_setspeed(struct ki2c_softc *sc, u_int speed)
{
	u_int x;

	KASSERT((speed & ~I2C_SPEED) == 0);
	x = ki2c_readreg(sc, MODE);
	x &= ~I2C_SPEED;
	x |= speed;
	ki2c_writereg(sc, MODE, x);
}

static int
ki2c_match(device_t parent, cfdata_t match, void *aux)
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, "i2c") == 0)
		return 1;

	return 0;
}

static void
ki2c_attach(device_t parent, device_t self, void *aux)
{
	struct ki2c_softc *sc = device_private(self);
	struct confargs *ca = aux;
	struct ki2c_channel *ch;
	int node = ca->ca_node;
	uint32_t channel, addr;
	int i, rate, child;
	struct i2cbus_attach_args iba;
	devhandle_t devhandle;
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
	
	ki2c_writereg(sc, IER,I2C_INT_DATA|I2C_INT_ADDR|I2C_INT_STOP);

	/*
	 * Two physical I2C busses share a single controller.  It's not
	 * quite a mux, which is why we don't attach it that way.
	 *
	 * The locking order is:
	 *
	 *	iic bus mutex -> ctrl_lock
	 *
	 * ctrl_lock is taken in ki2c_i2c_acquire_bus.
	 */
	mutex_init(&sc->sc_ctrl_lock, MUTEX_DEFAULT, IPL_NONE);

	/* Set up the channel structures. */
	for (i = 0; i < KI2C_MAX_I2C_CHANNELS; i++) {
		ch = &sc->sc_channels[i];

		iic_tag_init(&ch->ch_i2c);
		ch->ch_i2c.ic_channel = ch->ch_channel = i;
		ch->ch_i2c.ic_cookie = ch;
		ch->ch_i2c.ic_acquire_bus = ki2c_i2c_acquire_bus;
		ch->ch_i2c.ic_release_bus = ki2c_i2c_release_bus;
		ch->ch_i2c.ic_exec = ki2c_i2c_exec;

		ch->ch_ki2c = sc;
	}

	/*
	 * Different systems have different I2C device tree topologies.
	 *
	 * Some systems use a scheme like this:
	 *
	 *   /u3@0,f8000000/i2c@f8001000/temp-monitor@98
	 *   /u3@0,f8000000/i2c@f8001000/fan@15e
	 *
	 * Here, we see the channel encoded in bit #8 of the address.
	 *
	 * Other systems use a scheme like this:
	 *
	 *   /ht@0,f2000000/pci@4000,0,0/mac-io@7/i2c@18000/i2c-bus@0
	 *   /ht@0,f2000000/pci@4000,0,0/mac-io@7/i2c@18000/i2c-bus@0/codec@8c
	 *
	 *   /u4@0,f8000000/i2c@f8001000/i2c-bus@1
	 *   /u4@0,f8000000/i2c@f8001000/i2c-bus@1/temp-monitor@94
	 *
	 * Here, a separate device tree node represents the channel.
	 * Note that in BOTH cases, the I2C address of the devices are
	 * shifted left by 1 (as it would be on the wire to leave room
	 * for the read/write bit).
	 *
	 * So, what we're going to do here is look for i2c-bus nodes.  If
	 * we find them, we remember those phandles, and will use them for
	 * device enumeration.  If we don't, then we will use the controller
	 * phandle for device enumeration and filter based on the channel
	 * bit in the "reg" property.
	 */
	int i2c_bus_phandles[KI2C_MAX_I2C_CHANNELS] = { 0 };
	bool separate_nodes_scheme = false;
	for (child = OF_child(node); child != 0; child = OF_peer(child)) {
		OF_getprop(child, "name", name, sizeof(name));
		if (strcmp(name, "i2c-bus") == 0) {
			separate_nodes_scheme = true;
			if (of_getprop_uint32(child, "reg", &channel) == -1) {
				continue;
			}
			if (channel >= KI2C_MAX_I2C_CHANNELS) {
				continue;
			}
			i2c_bus_phandles[channel] = child;
		}
	}

	/*
	 * Set up our handle implementation (we provide our own
	 * i2c enumeration call).
	 */
	devhandle = device_handle(self);
	devhandle_impl_inherit(&sc->sc_devhandle_impl, devhandle.impl);
	sc->sc_devhandle_impl.lookup_device_call =
	    ki2c_devhandle_lookup_device_call;

	for (i = 0; i < KI2C_MAX_I2C_CHANNELS; i++) {
		int locs[I2CBUSCF_NLOCS];

		ch = &sc->sc_channels[i];

		if (separate_nodes_scheme) {
			if (i2c_bus_phandles[i] == 0) {
				/*
				 * This wasn't represented (either at all
				 * or not correctly) in the device tree,
				 * so skip attaching the "iic" instance.
				 */
				continue;
			}
			devhandle = devhandle_from_of(i2c_bus_phandles[i]);
		} else {
			devhandle = device_handle(self);
		}
		devhandle.impl = &sc->sc_devhandle_impl;

		locs[I2CBUSCF_BUS] = ch->ch_i2c.ic_channel;

		memset(&iba, 0, sizeof(iba));
		iba.iba_tag = &ch->ch_i2c;
		config_found(sc->sc_dev, &iba, iicbus_print_multi,
		    CFARGS(.submatch = config_stdsubmatch,
			   .locators = locs,
			   .devhandle = devhandle));
	}

}

static int
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

static int
ki2c_poll(struct ki2c_softc *sc, int timo)
{
	while (sc->sc_flags & I2C_BUSY) {
		if (ki2c_readreg(sc, ISR))
			ki2c_intr(sc);
		timo -= 100;
		if (timo < 0) {
			DPRINTF("i2c_poll: timeout\n");
			return ETIMEDOUT;
		}
		delay(100);
	}
	return 0;
}

static int
ki2c_start(struct ki2c_softc *sc, int addr, int subaddr, void *data, int len)
{
	int rw = (sc->sc_flags & I2C_READING) ? 1 : 0;
	int error, timo, x;

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

	if ((error = ki2c_poll(sc, timo)) != 0)
		return error;

	if (sc->sc_flags & I2C_ERROR) {
		DPRINTF("I2C_ERROR\n");
		return EIO;
	}
	return 0;
}

static int
ki2c_read(struct ki2c_softc *sc, int addr, int subaddr, void *data, int len)
{
	sc->sc_flags = I2C_READING;
	DPRINTF("ki2c_read: %02x %d\n", addr, len);
	return ki2c_start(sc, addr, subaddr, data, len);
}

static int
ki2c_write(struct ki2c_softc *sc, int addr, int subaddr, void *data, int len)
{
	sc->sc_flags = 0;
	DPRINTF("ki2c_write: %02x %d\n",addr,len);
	return ki2c_start(sc, addr, subaddr, data, len);
}

static int
ki2c_i2c_acquire_bus(void * const v, int const flags)
{
	struct ki2c_channel *ch = v;
	struct ki2c_softc *sc = ch->ch_ki2c;

	if (flags & I2C_F_POLL) {
		if (! mutex_tryenter(&sc->sc_ctrl_lock)) {
			return EBUSY;
		}
	} else {
		mutex_enter(&sc->sc_ctrl_lock);
	}
	return 0;
}

static void
ki2c_i2c_release_bus(void * const v, int const flags)
{
	struct ki2c_channel *ch = v;
	struct ki2c_softc *sc = ch->ch_ki2c;

	mutex_exit(&sc->sc_ctrl_lock);
}

int
ki2c_i2c_exec(void *cookie, i2c_op_t op, i2c_addr_t addr, const void *vcmd,
    size_t cmdlen, void *vbuf, size_t buflen, int flags)
{
	struct ki2c_channel *ch = cookie;
	struct ki2c_softc *sc = ch->ch_ki2c;
	int i, error;
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
		return ENOTSUP;

	/* Don't support 10-bit addressing. */
	if (addr > 0x7f)
		return ENOTSUP;

	channel = ch->ch_channel == 1 ? 0x10 : 0x00;

	/* we handle the subaddress stuff ourselves */
	ki2c_setmode(sc, channel | I2C_STDMODE);	
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

	if (w_len > 0) {
		error = ki2c_write(sc, addr << 1, 0, wp, w_len);
		if (error) {
			return error;
		}
	}

	if (I2C_OP_READ_P(op)) {
		error = ki2c_read(sc, addr << 1, 0, vbuf, buflen);
		if (error) {
			return error;
		}
	}
	return 0;
}
