/*	$NetBSD: jbus-i2c.c,v 1.1 2018/10/20 06:25:46 macallan Exp $	*/

/*
 * Copyright (c) 2018 Michael Lorenz
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: jbus-i2c.c,v 1.1 2018/10/20 06:25:46 macallan Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <sys/bus.h>
#include <machine/autoconf.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/i2c_bitbang.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <sparc64/dev/iommureg.h>
#include <sparc64/dev/iommuvar.h>
#include <sparc64/dev/schizovar.h>
#include <sparc64/dev/schizoreg.h>
#include <machine/openfirm.h>

#define DPRINTF printf

/* I2C glue */
static int jbusi2c_i2c_acquire_bus(void *, int);
static void jbusi2c_i2c_release_bus(void *, int);
static int jbusi2c_i2c_send_start(void *, int);
static int jbusi2c_i2c_send_stop(void *, int);
static int jbusi2c_i2c_initiate_xfer(void *, i2c_addr_t, int);
static int jbusi2c_i2c_read_byte(void *, uint8_t *, int);
static int jbusi2c_i2c_write_byte(void *, uint8_t, int);

/* I2C bitbang glue */
static void jbusi2c_i2cbb_set_bits(void *, uint32_t);
static void jbusi2c_i2cbb_set_dir(void *, uint32_t);
static uint32_t jbusi2c_i2cbb_read(void *);

static const struct i2c_bitbang_ops jbusi2c_i2cbb_ops = {
	jbusi2c_i2cbb_set_bits,
	jbusi2c_i2cbb_set_dir,
	jbusi2c_i2cbb_read,
	{
		2,	/* bit 1 is data */
		1,	/* bit 0 is clock */
		3,	/* direction register for both out */
		1	/* data in, clock out */
	}
};

static	int	jbusi2c_match(device_t, cfdata_t, void *);
static	void	jbusi2c_attach(device_t, device_t, void *);

struct jbusi2c_softc {
	device_t sc_dev;
	struct i2c_controller sc_i2c;
	kmutex_t sc_i2c_lock;
	int sc_node;
};

static void jbusi2c_setup_i2c(struct jbusi2c_softc *);

CFATTACH_DECL_NEW(jbusi2c, sizeof(struct jbusi2c_softc),
    jbusi2c_match, jbusi2c_attach, NULL, NULL);

extern struct schizo_softc *schizo0;

int
jbusi2c_match(device_t parent, cfdata_t match, void *aux)
{
	struct mainbus_attach_args *ma = aux;
	char *str;

	if (strcmp(ma->ma_name, "i2c") != 0)
		return (0);

	str = prom_getpropstring(ma->ma_node, "compatible");
	if (strcmp(str, "jbus-i2c") == 0)
		return (1);

	return (0);
}

void
jbusi2c_attach(device_t parent, device_t self, void *aux)
{
	struct jbusi2c_softc *sc = device_private(self);
	struct mainbus_attach_args *ma = aux;

	aprint_normal("\n");

	sc->sc_dev = self;
	sc->sc_node = ma->ma_node;

	if (schizo0 != NULL)
		jbusi2c_setup_i2c(sc);
}



static void
jbusi2c_setup_i2c(struct jbusi2c_softc *sc)
{
	struct i2cbus_attach_args iba;
	prop_array_t cfg;
	prop_dictionary_t dev;
	prop_data_t data;
	prop_dictionary_t dict = device_properties(sc->sc_dev);
	int ret, devs, regs[2], addr;
	uint8_t reg = 0, val[128];
	char name[64], compat[256];

	sc->sc_i2c.ic_cookie = schizo0;
	sc->sc_i2c.ic_acquire_bus = jbusi2c_i2c_acquire_bus;
	sc->sc_i2c.ic_release_bus = jbusi2c_i2c_release_bus;
	sc->sc_i2c.ic_send_start = jbusi2c_i2c_send_start;
	sc->sc_i2c.ic_send_stop = jbusi2c_i2c_send_stop;
	sc->sc_i2c.ic_initiate_xfer = jbusi2c_i2c_initiate_xfer;
	sc->sc_i2c.ic_read_byte = jbusi2c_i2c_read_byte;
	sc->sc_i2c.ic_write_byte = jbusi2c_i2c_write_byte;
	sc->sc_i2c.ic_exec = NULL;

	val[0] = 0;
	ret = iic_exec(&sc->sc_i2c, I2C_OP_READ_WITH_STOP, 0x50,
		    &reg, 1, val, 1, I2C_F_POLL);
	printf("ret %d val %02x\n", ret, val[0]);

	/* round up i2c devices */
	devs = OF_child(sc->sc_node);
	cfg = prop_array_create();
	prop_dictionary_set(dict, "i2c-child-devices", cfg);
	prop_object_release(cfg);
	while (devs != 0) {
		if (OF_getprop(devs, "name", name, 256) <= 0)
			goto skip;
		memset(compat, 0, sizeof(compat));
		if (OF_getprop(devs, "compatible",
		    compat, 255) <= 0)
			goto skip;
		if (OF_getprop(devs, "reg", regs, 8) <= 0)
			goto skip;
		if (regs[0] != 0) goto skip;
		addr = (regs[1] & 0xff) >> 1;
		DPRINTF("-> %s@%d,%x\n", name, regs[0], addr);
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
	memset(&iba, 0, sizeof(iba));
	iba.iba_tag = &sc->sc_i2c;
	mutex_init(&sc->sc_i2c_lock, MUTEX_DEFAULT, IPL_NONE);
	config_found_ia(sc->sc_dev, "i2cbus", &iba,
	    iicbus_print);
}

/* I2C bitbanging */
static void
jbusi2c_i2cbb_set_bits(void *cookie, uint32_t bits)
{
	struct schizo_softc *sc = cookie;

	schizo_write(sc, SCZ_JBUS_GPIO_DATA, bits);
}

static void
jbusi2c_i2cbb_set_dir(void *cookie, uint32_t dir)
{
	struct schizo_softc *sc = cookie;

	schizo_write(sc, SCZ_JBUS_GPIO_DIR, dir);
}

static uint32_t
jbusi2c_i2cbb_read(void *cookie)
{
	struct schizo_softc *sc = cookie;

	return schizo_read(sc, SCZ_JBUS_GPIO_DATA);
}

/* higher level I2C stuff */
static int
jbusi2c_i2c_acquire_bus(void *cookie, int flags)
{

	/* private bus */
	return 0;
}

static void
jbusi2c_i2c_release_bus(void *cookie, int flags)
{

	/* private bus */
}

static int
jbusi2c_i2c_send_start(void *cookie, int flags)
{

	return i2c_bitbang_send_start(cookie, flags, &jbusi2c_i2cbb_ops);
}

static int
jbusi2c_i2c_send_stop(void *cookie, int flags)
{

	return i2c_bitbang_send_stop(cookie, flags, &jbusi2c_i2cbb_ops);
}

static int
jbusi2c_i2c_initiate_xfer(void *cookie, i2c_addr_t addr, int flags)
{

	return i2c_bitbang_initiate_xfer(cookie, addr, flags, 
	    &jbusi2c_i2cbb_ops);
}

static int
jbusi2c_i2c_read_byte(void *cookie, uint8_t *valp, int flags)
{

	return i2c_bitbang_read_byte(cookie, valp, flags, &jbusi2c_i2cbb_ops);
}

static int
jbusi2c_i2c_write_byte(void *cookie, uint8_t val, int flags)
{

	return i2c_bitbang_write_byte(cookie, val, flags, &jbusi2c_i2cbb_ops);
}
