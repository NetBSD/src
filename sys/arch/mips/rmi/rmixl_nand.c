/*	$NetBSD: rmixl_nand.c,v 1.8 2023/05/10 00:07:58 riastradh Exp $	*/

/*-
 * Copyright (c) 2010 Department of Software Engineering,
 *		      University of Szeged, Hungary
 * Copyright (c) 2010 Adam Hoka <ahoka@NetBSD.org>
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by the Department of Software Engineering, University of Szeged, Hungary
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Device driver for the RMI XLS NAND controller
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rmixl_nand.c,v 1.8 2023/05/10 00:07:58 riastradh Exp $");

#include "opt_flash.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cdefs.h>
#include <sys/device.h>
#include <sys/endian.h>

#include <sys/bus.h>

#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixlvar.h>
#include <mips/rmi/rmixl_iobusvar.h>
#include <mips/rmi/rmixl_intr.h>

#include <dev/nand/nand.h>
#include <dev/nand/onfi.h>


static int  rmixl_nand_match(device_t, cfdata_t, void *);
static void rmixl_nand_attach(device_t, device_t, void *);
static int  rmixl_nand_detach(device_t, int);
static void rmixl_nand_command(device_t, uint8_t);
static void rmixl_nand_address(device_t, uint8_t);
static void rmixl_nand_busy(device_t);
static void rmixl_nand_read_1(device_t, uint8_t *);
static void rmixl_nand_write_1(device_t, uint8_t);
static void rmixl_nand_read_2(device_t, uint16_t *);
static void rmixl_nand_write_2(device_t, uint16_t);
static void rmixl_nand_read_buf(device_t, void *, size_t);
static void rmixl_nand_write_buf(device_t, const void *, size_t);


struct rmixl_nand_softc {
	device_t sc_dev;
	device_t sc_nanddev;
	struct iobus_softc *sc_iobus_sc;

	int			sc_cs;		/* chip select index */
	int			sc_buswidth;	/* in bytes */

	struct nand_interface	sc_nand_if;

	bus_space_tag_t		sc_obio_bst;
	bus_space_handle_t	sc_obio_bsh;
	u_long			sc_cmd_reg;
	u_long			sc_addr_reg;

	bus_addr_t		sc_iobus_addr;
	bus_size_t		sc_iobus_size;
	bus_space_tag_t		sc_iobus_bst;
	bus_space_handle_t	sc_iobus_bsh;
	u_long			sc_data_reg;

};

CFATTACH_DECL_NEW(rmixl_nand, sizeof(struct rmixl_nand_softc), rmixl_nand_match,
    rmixl_nand_attach, rmixl_nand_detach, NULL);

static int
rmixl_nand_match(device_t parent, cfdata_t match, void *aux)
{
	struct rmixl_iobus_attach_args *ia = aux;
	bus_space_handle_t bsh;
	volatile uint32_t *vaddr;
	int err;
	int rv;

	if ((ia->ia_dev_parm & RMIXL_FLASH_CSDEV_NANDEN) == 0)
		return 0;	/* not NAND */

	if (cpu_rmixlp(mips_options.mips_cpu)) {
		aprint_error("%s: NAND not yet supported on XLP", __func__);
		return 0;
	}

	if (! cpu_rmixls(mips_options.mips_cpu)) {
		aprint_error("%s: NAND not supported on this processor",
			__func__);
		return 0;
	}

	/*
	 * probe for NAND -- this may be redundant
	 * if the device isn't there, the NANDEN test (above)
	 * should have failed
	 */
	err = bus_space_map(ia->ia_iobus_bst, ia->ia_iobus_addr,
		sizeof(uint32_t), 0, &bsh);
	if (err != 0) {
		aprint_debug("%s: bus_space_map err %d, "
			"iobus space addr %#" PRIxBUSADDR "\n",
			__func__, err, ia->ia_iobus_addr);
		return 0;
	}

	vaddr = bus_space_vaddr(ia->ia_iobus_bst, bsh);
	rv = rmixl_probe_4(vaddr);
	if (rv == 0)
		aprint_debug("%s: rmixl_probe_4 failed, vaddr %p\n",
			__func__, vaddr);

	bus_space_unmap(ia->ia_iobus_bst, bsh, sizeof(uint32_t));

	return rv;
}

static void
rmixl_nand_attach(device_t parent, device_t self, void *aux)
{
	struct rmixl_nand_softc *sc = device_private(self);
	sc->sc_iobus_sc = device_private(parent);
	struct rmixl_iobus_attach_args *ia = aux;
	uint32_t val;
	int err;

	aprint_normal("\n");

	sc->sc_dev = self;
	sc->sc_obio_bst = ia->ia_obio_bst;
	sc->sc_obio_bsh = ia->ia_obio_bsh;
	sc->sc_iobus_bst = ia->ia_iobus_bst;
	sc->sc_iobus_addr = ia->ia_iobus_addr;
	sc->sc_iobus_size = sizeof(uint32_t);
	sc->sc_cs = ia->ia_cs;

        sc->sc_cmd_reg = RMIXL_NAND_CLEn(ia->ia_cs);
	sc->sc_addr_reg = RMIXL_NAND_ALEn(ia->ia_cs);

	aprint_debug_dev(self, "CS#%d cstime_parma %#x, cstime_parmb %#x\n",
		ia->ia_cs,
		bus_space_read_4(sc->sc_obio_bst, sc->sc_obio_bsh,
			RMIXL_FLASH_CSTIME_PARMAn(ia->ia_cs)),
		bus_space_read_4(sc->sc_obio_bst, sc->sc_obio_bsh,
			RMIXL_FLASH_CSTIME_PARMBn(ia->ia_cs)));

	err = bus_space_map(sc->sc_iobus_bst, sc->sc_iobus_addr,
		sc->sc_iobus_size, 0, &sc->sc_iobus_bsh);
	if (err != 0) {
		aprint_error_dev(self,
			"bus space map err %d, iobus space\n", err);
		return;
	}

	/*
	 * determine buswidth
	 */
	val = ia->ia_dev_parm;
	val &= RMIXL_FLASH_CSDEV_DWIDTH;
	val >>= RMIXL_FLASH_CSDEV_DWIDTH_SHFT;
	switch(val) {
	case 0:	/* FALLTHROUGH */
	case 3:
		sc->sc_buswidth = 1;	/* 8 bit */
		break;
	case 1:
		sc->sc_buswidth = 2;	/* 16 bit */
		break;
	case 2:
		sc->sc_buswidth = 4;	/* 32 bit */
		break;
	}
	aprint_debug_dev(self, "bus width %d bits\n", 8 * sc->sc_buswidth);

	nand_init_interface(&sc->sc_nand_if);

	sc->sc_nand_if.command = rmixl_nand_command;
	sc->sc_nand_if.address = rmixl_nand_address;
	sc->sc_nand_if.read_buf_1 = rmixl_nand_read_buf;
	sc->sc_nand_if.read_buf_2 = rmixl_nand_read_buf;
	sc->sc_nand_if.read_1 = rmixl_nand_read_1;
	sc->sc_nand_if.read_2 = rmixl_nand_read_2;
	sc->sc_nand_if.write_buf_1 = rmixl_nand_write_buf;
	sc->sc_nand_if.write_buf_2 = rmixl_nand_write_buf;
	sc->sc_nand_if.write_1 = rmixl_nand_write_1;
	sc->sc_nand_if.write_2 = rmixl_nand_write_2;
	sc->sc_nand_if.busy = rmixl_nand_busy;

	sc->sc_nand_if.ecc.necc_code_size = 3;
	sc->sc_nand_if.ecc.necc_block_size = 256;

	/*
	 * reset to get NAND into known state
	 */
	rmixl_nand_command(self, ONFI_RESET);
	rmixl_nand_busy(self);

	if (! pmf_device_register1(self, NULL, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	sc->sc_nanddev = nand_attach_mi(&sc->sc_nand_if, self);
}

static int
rmixl_nand_detach(device_t self, int flags)
{
	struct rmixl_nand_softc *sc = device_private(self);
	int error;

	error = config_detach_children(self, flags);
	if (error)
		return error;

	pmf_device_deregister(self);

	bus_space_unmap(sc->sc_iobus_bst, sc->sc_iobus_bsh, sc->sc_iobus_size);

	return 0;
}

static void
rmixl_nand_command(device_t self, uint8_t command)
{
	struct rmixl_nand_softc *sc = device_private(self);

	bus_space_write_4(sc->sc_obio_bst, sc->sc_obio_bsh,
		sc->sc_cmd_reg, command);
}

static void
rmixl_nand_address(device_t self, uint8_t address)
{
	struct rmixl_nand_softc *sc = device_private(self);

	bus_space_write_4(sc->sc_obio_bst, sc->sc_obio_bsh,
		sc->sc_addr_reg, address);
}

static void
rmixl_nand_busy(device_t self)
{
	struct rmixl_nand_softc *sc = device_private(self);
	uint32_t istatus;

	for(u_int count=100000; count--;) {
		istatus = bus_space_read_4(sc->sc_obio_bst, sc->sc_obio_bsh,
			RMIXL_FLASH_INT_STATUS);
		if ((istatus & __BIT(8)) != 0)
			return;
		DELAY(1);
	}
#ifdef DEBUG
	printf("%s: timed out, istatus=%#x\n", __func__, istatus);
#ifdef DDB
	Debugger();
#endif
#endif
}

static void
rmixl_nand_read_1(device_t self, uint8_t *data)
{
	struct rmixl_nand_softc *sc = device_private(self);

	*data = bus_space_read_1(sc->sc_iobus_bst, sc->sc_iobus_bsh, 0);
}

static void
rmixl_nand_write_1(device_t self, uint8_t data)
{
	struct rmixl_nand_softc *sc = device_private(self);

	bus_space_write_1(sc->sc_iobus_bst, sc->sc_iobus_bsh, 0, data);
}

static void
rmixl_nand_read_2(device_t self, uint16_t *data)
{
	struct rmixl_nand_softc *sc = device_private(self);

	*data = bus_space_read_2(sc->sc_iobus_bst, sc->sc_iobus_bsh, 0);
}

static void
rmixl_nand_write_2(device_t self, uint16_t data)
{
	struct rmixl_nand_softc *sc = device_private(self);

	bus_space_write_2(sc->sc_iobus_bst, sc->sc_iobus_bsh, 0, data);
}

static void
rmixl_nand_read_buf(device_t self, void *buf, size_t len)
{
	struct rmixl_nand_softc *sc = device_private(self);
	uintptr_t addr = (uintptr_t)buf;
	size_t sz;

	/* leading byte alignment */
	if ((len >= 1) && ((addr & 1) != 0)) {
		*((uint8_t *)addr) = bus_space_read_1(sc->sc_iobus_bst,
			sc->sc_iobus_bsh, 0);
		addr += 1;
		len -= 1;
	}

	/* leading short alignment */
	if ((len >= 2) && ((addr & 2) != 0)) {
		*((uint16_t *)addr) = bus_space_read_2(sc->sc_iobus_bst,
			sc->sc_iobus_bsh, 0);
		addr += 2;
		len -= 2;
	}

	/* word alignment */
	sz = len >> 2;
	if (sz != 0) {
		bus_space_read_multi_4(sc->sc_iobus_bst, sc->sc_iobus_bsh,
			0, (uint32_t *)addr, sz);
		sz <<= 2;
		addr += sz;
		len  -= sz;
	}

	/* trailing short alignment */
	if (len >= 2) {
		*((uint16_t *)addr) = bus_space_read_2(sc->sc_iobus_bst,
			sc->sc_iobus_bsh, 0);
		addr += 2;
		len -= 2;
	}

	/* trailing byte alignment */
	if (len != 0)
		*((uint8_t *)addr) = bus_space_read_1(sc->sc_iobus_bst,
			sc->sc_iobus_bsh, 0);
}

static void
rmixl_nand_write_buf(device_t self, const void *buf, size_t len)
{
	struct rmixl_nand_softc *sc = device_private(self);
	uintptr_t addr = (uintptr_t)buf;
	size_t sz;

	/* leading byte alignment */
	if ((len >= 1) && ((addr & 1) != 0)) {
		bus_space_write_1(sc->sc_iobus_bst, sc->sc_iobus_bsh, 0,
			*((uint8_t *)addr));
		addr += 1;
		len -= 1;
	}

	/* leading short alignment */
	if ((len >= 2) && ((addr & 2) != 0)) {
		bus_space_write_2(sc->sc_iobus_bst, sc->sc_iobus_bsh, 0,
			*((uint16_t *)addr));
		addr += 2;
		len -= 2;
	}

	/* word alignment */
	sz = len >> 2;
	if (sz != 0) {
		bus_space_write_multi_4(sc->sc_iobus_bst, sc->sc_iobus_bsh,
			0, (uint32_t *)addr, sz);
		sz <<= 2;
		addr += sz;
		len  -= sz;
	}

	/* trailing short alignment */
	if (len >= 2) {
		bus_space_write_2(sc->sc_iobus_bst, sc->sc_iobus_bsh, 0,
			*((uint16_t *)addr));
		addr += 2;
		len -= 2;
	}

	/* trailing byte alignment */
	if (len != 0)
		bus_space_write_1(sc->sc_iobus_bst, sc->sc_iobus_bsh, 0,
			*((uint8_t *)addr));
}
