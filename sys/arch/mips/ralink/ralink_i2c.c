/*	$NetBSD: ralink_i2c.c,v 1.2 2011/07/28 15:38:49 matt Exp $	*/
/*-
 * Copyright (c) 2011 CradlePoint Technology, Inc.
 * All rights reserved.
 *
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
 * THIS SOFTWARE IS PROVIDED BY CRADLEPOINT TECHNOLOGY, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* ra_i2c.c - Ralink i2c 3052 driver */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ralink_i2c.c,v 1.2 2011/07/28 15:38:49 matt Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/systm.h>

#include <dev/i2c/i2cvar.h>

#include <mips/ralink/ralink_var.h>
#include <mips/ralink/ralink_reg.h>

#if 0
/*
 * Defined for the Ralink 3050, w/320MHz CPU:  milage may vary.
 * Set the I2C clock to 100K bps (low speed) transfer rate.
 * Value is based upon the forms defined in the Ralink reference document
 * for RT3050, page 53.  JCL.
 */
#define CLKDIV_VALUE 533
#endif

/*
 * Slow the I2C bus clock to 12.5 KHz to work around the misbehavior 
 * of the TI part.
 */
#define CLKDIV_VALUE 4264

#define i2c_busy_loop		(clkdiv*30)
#define max_ee_busy_loop	(clkdiv*25)


typedef struct ra_i2c_softc {
	device_t		sc_dev;
	struct i2c_controller	sc_i2c;
	bus_space_tag_t		sc_memt;
	bus_space_handle_t	sc_i2c_memh;
	bus_space_handle_t	sc_sy_memh;
} ra_i2c_softc_t;


static int  ra_i2c_match(struct device *, cfdata_t, void *);
static void ra_i2c_attach(struct device *, struct device *, void *);

/* RT3052 I2C functions */
static int  i2c_write(ra_i2c_softc_t *, u_long, const u_char *, u_long);
static int  i2c_read(ra_i2c_softc_t *, u_long, u_char *, u_long);
#ifdef NOTYET
static void i2c_write_stop(ra_i2c_softc_t *, u_long, u_char *);
static void i2c_read_stop(ra_i2c_softc_t *, u_long, u_char *);
#endif

/* i2c driver functions */
int  ra_i2c_acquire_bus(void *, int);
void ra_i2c_release_bus(void *, int);
int  ra_i2c_exec(void *, i2c_op_t, i2c_addr_t, const void *, size_t,
	void *, size_t, int);
void ra_i2c_reset(ra_i2c_softc_t *);

CFATTACH_DECL_NEW(ri2c, sizeof(struct ra_i2c_softc),
	ra_i2c_match, ra_i2c_attach, NULL, NULL);

unsigned int clkdiv = CLKDIV_VALUE;

static inline void
ra_i2c_busy_wait(ra_i2c_softc_t *sc)
{
	for (int i=0; i < i2c_busy_loop; i++) {
		uint32_t r;
		r = bus_space_read_4(sc->sc_memt, sc->sc_i2c_memh,
			RA_I2C_STATUS);
		if ((r & I2C_STATUS_BUSY) == 0)
			break;
	}
}

int
ra_i2c_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}


void
ra_i2c_attach(device_t parent, device_t self, void *aux)
{
	ra_i2c_softc_t * const sc = device_private(self);
	const struct mainbus_attach_args *ma = aux;
	struct i2cbus_attach_args iba;
	uint32_t r;
	int error;

	aprint_naive(": Ralink I2C controller\n");
	aprint_normal(": Ralink I2C controller\n");

	/* save out bus space tag */
	sc->sc_memt = ma->ma_memt;

	/* Map Sysctl registers */
	if ((error = bus_space_map(ma->ma_memt, RA_SYSCTL_BASE, 0x10000,
	    0, &sc->sc_sy_memh)) != 0) {
		aprint_error_dev(self, "unable to map Sysctl registers, "
			"error=%d\n", error);
		return;
	}

	/* map the I2C registers */
	if ((error = bus_space_map(sc->sc_memt, RA_I2C_BASE, 0x100,
	    0, &sc->sc_i2c_memh)) != 0) {
		aprint_error_dev(self, "unable to map registers, "
			"error=%d\n", error);
		bus_space_unmap(ma->ma_memt, sc->sc_sy_memh, 0x10000);
		return;
	}

	/*  Enable I2C block */
	r = bus_space_read_4(sc->sc_memt, sc->sc_sy_memh,
		RA_SYSCTL_GPIOMODE);
	r &= ~GPIOMODE_I2C;
	bus_space_write_4(sc->sc_memt, sc->sc_sy_memh,
		RA_SYSCTL_GPIOMODE, r);

	sc->sc_i2c.ic_cookie = sc;
	sc->sc_i2c.ic_acquire_bus = ra_i2c_acquire_bus;
	sc->sc_i2c.ic_release_bus = ra_i2c_release_bus;
	sc->sc_i2c.ic_exec = ra_i2c_exec;

	memset(&iba, 0, sizeof(iba));
	iba.iba_type = I2C_TYPE_SMBUS;
	iba.iba_tag = &sc->sc_i2c;
	config_found(self, &iba, iicbus_print);
}



/*
 * I2C API
 */

/* Might not be needed.  JCL. */
int
ra_i2c_acquire_bus(void *cookie, int flags)
{
	/* nothing */
	return 0;
}



/* Might not be needed.  JCL. */
void
ra_i2c_release_bus(void *cookie, int flags)
{
	/* nothing */
}

int
ra_i2c_exec(void *cookie, i2c_op_t op, i2c_addr_t addr, const void *cmdbuf,
	size_t cmdlen, void *buf, size_t len, int flags)
{
	ra_i2c_softc_t * const sc = cookie;

	/*
	 * Make sure we only pass a seven-bit device address,
	 * as per I2C standard.
	 */
	KASSERT(addr <= 127);
	if (addr > 127)
		return -1;

	bus_space_write_4(sc->sc_memt, sc->sc_i2c_memh, RA_I2C_DEVADDR,
		addr);

	ra_i2c_reset(sc);

	/*
	 * Process the requested operation.
	 * There are four I2C operations of interest:
	 *   - Write
	 *   - Write with stop
	 *   - Read
	 *   - Read with stop
	 * Because the I2C block on the Ralink part generates stop markers
	 * at approprite places, based upon the byte count, the read and write
	 * with stop operations will rarely be needed.  They are included here
	 * as placeholders, but haven't been implemented or tested.
	 */
	switch(op) {
	case  I2C_OP_WRITE:
		return i2c_write(sc, addr, cmdbuf, cmdlen);
		break;
	case I2C_OP_READ:
		return i2c_read(sc, addr, buf, len);
		break;
#ifdef NOTYET
	case I2C_OP_WRITE_WITH_STOP:
		i2c_write_stop(sc, addr, buf);
		break;
	case I2C_OP_READ_WITH_STOP:
		i2c_read_stop(sc, addr, buf);
		break;
#endif
	default:
		return -1;  /* Illegal operation, error return. */
	}

	return 0;
}

static int
i2c_write(ra_i2c_softc_t *sc, u_long addr, const u_char *data,
	u_long nbytes)
{
	uint32_t r;
	int i, j;

	bus_space_write_4(sc->sc_memt, sc->sc_i2c_memh, RA_I2C_DEVADDR,
		addr);
	bus_space_write_4(sc->sc_memt, sc->sc_i2c_memh, RA_I2C_BYTECNT,
		nbytes - 1);
	bus_space_write_4(sc->sc_memt, sc->sc_i2c_memh, RA_I2C_STARTXFR,
		I2C_OP_WRITE);

	for (i=0; i < nbytes; i++) {
		for (j=0; j < max_ee_busy_loop; j++) {
			r = bus_space_read_4(sc->sc_memt, sc->sc_i2c_memh, 
				RA_I2C_STATUS);
			if ((r & I2C_STATUS_SDOEMPTY) != 0) {
				bus_space_write_4(sc->sc_memt, sc->sc_i2c_memh,
					RA_I2C_DATAOUT, data[i]);
				break;
			}
		}
#if 0
		if ((r & I2C_STATUS_ACKERR) != 0) {
			aprint_error_dev(sc->sc_dev, "ACK error in %s\n",
				__func__);
			return EAGAIN;
		}
#endif
		if (j == max_ee_busy_loop) {
			aprint_error_dev(sc->sc_dev, "timeout error in %s\n",
				__func__);
			return EAGAIN;
		}	
	}

	ra_i2c_busy_wait(sc);

	return 0;
}


static int
i2c_read(ra_i2c_softc_t *sc, u_long addr, u_char *data, u_long nbytes)
{
	bus_space_write_4(sc->sc_memt, sc->sc_i2c_memh, RA_I2C_DEVADDR,
		addr);
	bus_space_write_4(sc->sc_memt, sc->sc_i2c_memh, RA_I2C_BYTECNT,
		nbytes - 1);
	bus_space_write_4(sc->sc_memt, sc->sc_i2c_memh, RA_I2C_STARTXFR,
		I2C_OP_READ);

	for (u_int i = 0; i < nbytes; i++) {
		u_long j;
		uint32_t r;

		for (j=0; j < max_ee_busy_loop; j++) {
			r = bus_space_read_4(sc->sc_memt, sc->sc_i2c_memh, 
				RA_I2C_STATUS);
			if ((r & I2C_STATUS_DATARDY) != 0) {
				data[i] = bus_space_read_4(
						sc->sc_memt, sc->sc_i2c_memh,
						RA_I2C_DATAIN);
				break;
			}
		}
#if 0
		if ((r & I2C_STATUS_ACKERR) != 0) {
			aprint_error_dev(sc->sc_dev, "ACK error in %s\n",
				__func__);
			return EAGAIN;
		}
#endif
		if (j == max_ee_busy_loop) {
			aprint_error_dev(sc->sc_dev, "timeout error in %s\n",
				__func__);
			return EAGAIN;
		}
	}

	ra_i2c_busy_wait(sc);

	return 0;

}


#ifdef NOTYET
static void
i2c_write_stop(ra_i2c_softc_t *sc, u_long address, u_char *data)
{
	/* unimplemented */
}

static void
i2c_read_stop(ra_i2c_softc_t *sc, u_long address, u_char *data)
{
	/* unimplemented */
}
#endif

void
ra_i2c_reset(ra_i2c_softc_t *sc)
{
	uint32_t r;

	/* reset i2c block */
	r = bus_space_read_4(sc->sc_memt, sc->sc_sy_memh, RA_SYSCTL_RST);
	bus_space_write_4(sc->sc_memt, sc->sc_sy_memh, RA_SYSCTL_RST, 
		r | RST_I2C);
	bus_space_write_4(sc->sc_memt, sc->sc_sy_memh, RA_SYSCTL_RST, r);

	r = I2C_CONFIG_ADDRLEN(I2C_CONFIG_ADDRLEN_8) |
	    I2C_CONFIG_DEVADLEN(I2C_CONFIG_DEVADLEN_7) |
	    I2C_CONFIG_ADDRDIS;
	bus_space_write_4(sc->sc_memt, sc->sc_i2c_memh, RA_I2C_CONFIG, r);

	/*
	 * Set the I2C clock divider.  Appears to be set to 200,000,
	 * which is strange, as I2C is 100K/400K/3.?M bps.
	 */
	bus_space_write_4(sc->sc_memt, sc->sc_i2c_memh, RA_I2C_CLKDIV,
		clkdiv);
}
