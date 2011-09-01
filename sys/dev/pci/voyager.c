/*	$NetBSD: voyager.c,v 1.2 2011/09/01 00:06:42 macallan Exp $	*/

/*
 * Copyright (c) 2009 Michael Lorenz
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: voyager.c,v 1.2 2011/09/01 00:06:42 macallan Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/lwp.h>
#include <sys/kauth.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciio.h>
#include <dev/ic/sm502reg.h>
#include <dev/i2c/i2cvar.h>
#include <dev/i2c/i2c_bitbang.h>

#include <dev/pci/voyagervar.h>

struct voyager_softc {
	device_t sc_dev;

	pci_chipset_tag_t sc_pc;
	pcitag_t sc_pcitag;

	bus_space_tag_t sc_memt;
	bus_space_tag_t sc_iot;

	bus_space_handle_t sc_fbh, sc_regh;
	bus_addr_t sc_fb, sc_reg;
	bus_size_t sc_fbsize, sc_regsize;

	struct i2c_controller sc_i2c;
	kmutex_t sc_i2c_lock;
};

static int	voyager_match(device_t, cfdata_t, void *);
static void	voyager_attach(device_t, device_t, void *);
static int  voyager_print(void *, const char *);

CFATTACH_DECL_NEW(voyager, sizeof(struct voyager_softc),
    voyager_match, voyager_attach, NULL, NULL);
    
/* I2C glue */
static int voyager_i2c_acquire_bus(void *, int);
static void voyager_i2c_release_bus(void *, int);
static int voyager_i2c_send_start(void *, int);
static int voyager_i2c_send_stop(void *, int);
static int voyager_i2c_initiate_xfer(void *, i2c_addr_t, int);
static int voyager_i2c_read_byte(void *, uint8_t *, int);
static int voyager_i2c_write_byte(void *, uint8_t, int);

/* I2C bitbang glue */
static void voyager_i2cbb_set_bits(void *, uint32_t);
static void voyager_i2cbb_set_dir(void *, uint32_t);
static uint32_t voyager_i2cbb_read(void *);

static const struct i2c_bitbang_ops voyager_i2cbb_ops = {
	voyager_i2cbb_set_bits,
	voyager_i2cbb_set_dir,
	voyager_i2cbb_read,
	{
		1 << 13,
		1 << 6,
		1 << 13,
		0
	}
};
#define GPIO_I2C_BITS ((1 << 6) | (1 << 13))

static int
voyager_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;

	if (PCI_CLASS(pa->pa_class) != PCI_CLASS_DISPLAY)
		return 0;
	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_SILMOTION)
		return 0;

	/* only chip tested on so far - may need a list */
	if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_SILMOTION_SM502)
		return 100;
	return (0);
}

static void
voyager_attach(device_t parent, device_t self, void *aux)
{
	struct voyager_softc	*sc = device_private(self);
	struct pci_attach_args	*pa = aux;
	char devinfo[256];
	struct voyager_attach_args	vaa;
	struct i2cbus_attach_args iba;
	uint32_t reg;

	sc->sc_pc = pa->pa_pc;
	sc->sc_pcitag = pa->pa_tag;
	sc->sc_memt = pa->pa_memt;
	sc->sc_iot = pa->pa_iot;
	sc->sc_dev = self;

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	aprint_normal(": %s\n", devinfo);

	if (pci_mapreg_map(pa, 0x14, PCI_MAPREG_TYPE_MEM, 0,
	    &sc->sc_memt, &sc->sc_regh, &sc->sc_reg, &sc->sc_regsize)) {
		aprint_error("%s: failed to map registers.\n",
		    device_xname(sc->sc_dev));
	}

	if (pci_mapreg_map(pa, 0x10, PCI_MAPREG_TYPE_MEM,
	    BUS_SPACE_MAP_LINEAR,
	    &sc->sc_memt, &sc->sc_fbh, &sc->sc_fb, &sc->sc_fbsize)) {
		aprint_error("%s: failed to map the frame buffer.\n",
		    device_xname(sc->sc_dev));
	}

	/* attach the framebuffer driver */
	vaa.vaa_memh = sc->sc_fbh;
	vaa.vaa_mem_pa = sc->sc_fb;
	vaa.vaa_regh = sc->sc_regh;
	vaa.vaa_reg_pa = sc->sc_reg;
	vaa.vaa_tag = sc->sc_memt;
	vaa.vaa_pc = sc->sc_pc;
	vaa.vaa_pcitag = sc->sc_pcitag;
	strcpy(vaa.vaa_name, "voyagerfb");
	config_found_ia(sc->sc_dev, "voyagerbus", &vaa, voyager_print);

	reg = bus_space_read_4(sc->sc_memt, sc->sc_regh, SM502_GPIO0_CONTROL);
	if ((reg & GPIO_I2C_BITS) == 0) {
		/* Fill in the i2c tag */
		sc->sc_i2c.ic_cookie = sc;
		sc->sc_i2c.ic_acquire_bus = voyager_i2c_acquire_bus;
		sc->sc_i2c.ic_release_bus = voyager_i2c_release_bus;
		sc->sc_i2c.ic_send_start = voyager_i2c_send_start;
		sc->sc_i2c.ic_send_stop = voyager_i2c_send_stop;
		sc->sc_i2c.ic_initiate_xfer = voyager_i2c_initiate_xfer;
		sc->sc_i2c.ic_read_byte = voyager_i2c_read_byte;
		sc->sc_i2c.ic_write_byte = voyager_i2c_write_byte;
		sc->sc_i2c.ic_exec = NULL;
		mutex_init(&sc->sc_i2c_lock, MUTEX_DEFAULT, IPL_NONE);
		iba.iba_tag = &sc->sc_i2c;
		config_found_ia(self, "i2cbus", &iba, iicbus_print);
	}
}

static int
voyager_print(void *aux, const char *what)
{
	/*struct voyager_attach_args *vaa = aux;*/

	if (what == NULL)
		return 0;

	printf("%s:", what);

	return 0;
}

static void
voyager_i2cbb_set_bits(void *cookie, uint32_t bits)
{
	struct voyager_softc *sc = cookie;
	uint32_t reg;

	reg = bus_space_read_4(sc->sc_memt, sc->sc_regh, SM502_GPIO_DATA0);
	reg &= ~GPIO_I2C_BITS;
	reg |= bits;
	bus_space_write_4(sc->sc_memt, sc->sc_regh, SM502_GPIO_DATA0, reg);
}

static void
voyager_i2cbb_set_dir(void *cookie, uint32_t bits)
{
	struct voyager_softc *sc = cookie;
	uint32_t reg;

	reg = bus_space_read_4(sc->sc_memt, sc->sc_regh, SM502_GPIO_DIR0);
	reg &= ~(1 << 13);
	reg |= bits;
	bus_space_write_4(sc->sc_memt, sc->sc_regh, SM502_GPIO_DIR0, reg);
}

static uint32_t
voyager_i2cbb_read(void *cookie)
{
	struct voyager_softc *sc = cookie;
	uint32_t reg;

	reg = bus_space_read_4(sc->sc_memt, sc->sc_regh, SM502_GPIO_DATA0);
	return reg;
}

/* higher level I2C stuff */
static int
voyager_i2c_acquire_bus(void *cookie, int flags)
{
	struct voyager_softc *sc = cookie;

	mutex_enter(&sc->sc_i2c_lock);
	return 0;
}

static void
voyager_i2c_release_bus(void *cookie, int flags)
{
	struct voyager_softc *sc = cookie;

	mutex_exit(&sc->sc_i2c_lock);
}

static int
voyager_i2c_send_start(void *cookie, int flags)
{
	return (i2c_bitbang_send_start(cookie, flags, &voyager_i2cbb_ops));
}

static int
voyager_i2c_send_stop(void *cookie, int flags)
{

	return (i2c_bitbang_send_stop(cookie, flags, &voyager_i2cbb_ops));
}

static int
voyager_i2c_initiate_xfer(void *cookie, i2c_addr_t addr, int flags)
{
	/*
	 * for some reason i2c_bitbang_initiate_xfer left-shifts
	 * the I2C-address and then sets the direction bit
	 */
	return (i2c_bitbang_initiate_xfer(cookie, addr, flags, 
	    &voyager_i2cbb_ops));
}

static int
voyager_i2c_read_byte(void *cookie, uint8_t *valp, int flags)
{
	return (i2c_bitbang_read_byte(cookie, valp, flags, &voyager_i2cbb_ops));
}

static int
voyager_i2c_write_byte(void *cookie, uint8_t val, int flags)
{
	return (i2c_bitbang_write_byte(cookie, val, flags, &voyager_i2cbb_ops));
}
