/*	$NetBSD: octeon_twsi.c,v 1.1.18.2 2017/12/03 11:36:27 jdolecek Exp $	*/

/*
 * Copyright (c) 2007 Internet Initiative Japan, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#undef	TWSIDEBUG
#undef	TWSITEST

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: octeon_twsi.c,v 1.1.18.2 2017/12/03 11:36:27 jdolecek Exp $");

#include "opt_octeon.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/lock.h>

#include <sys/bus.h>

#include <dev/i2c/i2cvar.h>

#include <mips/cavium/include/iobusvar.h>
#include <mips/cavium/dev/octeon_twsireg.h>

#ifdef TWSIDEBUG
#define	DPRINTF(x)	printf x
#else
#define	DPRINTF(x)
#endif

struct octeon_twsi_reg;

struct octeon_twsi_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_regt;
	bus_space_handle_t	sc_regh;

	void *sc_ih;

	struct i2c_controller	sc_i2c;
	struct lock		sc_lock;

	/* ... */
};

/* Auto-configuration */

static int		octeon_twsi_match(device_t, struct cfdata *,
			    void *);
static void		octeon_twsi_attach(device_t, device_t,
			    void *);

/* High-Level Controller Master */

#ifdef notyet
static uint8_t		octeon_twsi_hlcm_read_1(struct octeon_twsi_softc *,
			    ...)
static uint64_t		octeon_twsi_hlcm_read_4(struct octeon_twsi_softc *,
			    ...)
static void		octeon_twsi_hlcm_read(struct octeon_twsi_softc *,
			    ...)
static void		octeon_twsi_hlcm_write_1(struct octeon_twsi_softc *,
			    ...)
static void		octeon_twsi_hlcm_write_4(struct octeon_twsi_softc *,
			    ...)
static void		octeon_twsi_hlcm_write(struct octeon_twsi_softc *,
			    ...)
#endif

/* High-Level Controller Slave */

/* XXX */

/* Control Register */

#ifdef notyet
#define	_CONTROL_READ(sc, reg) \
	octeon_twsi_control_read((sc), MIO_TWS_SW_TWSI_EOP_IA_##reg)
#define	_CONTROL_WRITE(sc, reg, value) \
	octeon_twsi_control_write((sc), MIO_TWS_SW_TWSI_EOP_IA_##reg, value)
static uint8_t		octeon_twsi_control_read(struct octeon_twsi_softc *sc,
			    uint64_t);
static void		octeon_twsi_control_write(struct octeon_twsi_softc *sc,
			    uint64_t, uint8_t);
#endif

/* Register accessors */

static inline uint64_t	octeon_twsi_reg_rd(struct octeon_twsi_softc *, int);
static inline void	octeon_twsi_reg_wr(struct octeon_twsi_softc *, int,
			    uint64_t);
#ifdef TWSIDEBUG
static inline void	octeon_twsi_reg_dump(struct octeon_twsi_softc *, int);
#endif

/* Test functions */

#ifdef TWSIDEBUG
static void		octeon_twsi_test(struct octeon_twsi_softc *);
#endif

/* Debug functions */

#ifdef TWSIDEBUG
static inline void	octeon_twsi_debug_reg_dump(struct octeon_twsi_softc *,
			    int);
static void		octeon_twsi_debug_dumpregs(struct octeon_twsi_softc *);
static void		octeon_twsi_debug_dumpreg(struct octeon_twsi_softc *,
			    const struct octeon_twsi_reg *);
#endif

/* -------------------------------------------------------------------------- */

/*
 * Auto-configuration
 */

CFATTACH_DECL_NEW(octeon_twsi, sizeof(struct octeon_twsi_softc),
    octeon_twsi_match, octeon_twsi_attach, NULL, NULL);

static int
octeon_twsi_match(device_t parent, struct cfdata *cf, void *aux)
{
	struct iobus_attach_args *aa = aux;

	if (strcmp(cf->cf_name, aa->aa_name) != 0)
		return 0;
	return 1;
}

static void
octeon_twsi_attach(device_t parent, device_t self, void *aux)
{
	struct octeon_twsi_softc *sc = device_private(self);
	struct iobus_attach_args *aa = aux;
	int status;

	sc->sc_dev = self;
	sc->sc_regt = aa->aa_bust;

	status = bus_space_map(sc->sc_regt, MIO_TWS_BASE_0, MIO_TWS_SIZE, 0,
	    &sc->sc_regh);
	if (status != 0)
		panic(": can't map register");

	aprint_normal("\n");

#ifdef TWSITEST
	octeon_twsi_test(sc);
#endif
}

/* -------------------------------------------------------------------------- */

/*
 * Initialization, basic operations
 */

#ifdef notyet

static void
octeon_twsi_wait(struct octeon_twsi_softc *sc)
{
}

static void
octeon_twsi_intr(struct octeon_twsi_softc *sc)
{
}

static void
octeon_twsi_lock(struct octeon_twsi_softc *sc)
{
}

static void
octeon_twsi_unlock(struct octeon_twsi_softc *sc)
{
}

#endif

/* -------------------------------------------------------------------------- */

/*
 * High-Level Controller as a Master
 */

#ifdef notyet

#define	_BUFTOLE32(buf)						\
	((buf[0] << 24) | (buf[1] << 16) | (buf[2] <<  8) | (buf[3] <<  0))
#define	_LE32TOBUF(buf, x)					\
	do {							\
		buf[0] = (char)((x) >> 24);			\
		buf[1] = (char)((x) >> 16);			\
		buf[2] = (char)((x) >> 8);			\
		buf[3] = (char)((x) >> 0);			\
	} while (0)

static void
octeon_twsi_hlcm_read(struct octeon_twsi_softc *sc, int addr, char *buf,
    size_t len)
{
	uint64_t cmd;
	size_t resid;

	octeon_twsi_lock(sc);

#ifdef notyet

	octeon_twsi_hlcm_setup(sc);

	resid = len;

	while (resid > 4) {
		cmd = MIO_TWS_SW_TWSI_OP_FOUR | MIO_TWS_SW_TWSI_R
		    | (addr << MIO_TWS_SW_TWSI_A_SHIFT);
		octeon_twsi_reg_wr(sc, MIO_TWS_SW_TWSI_OFFSET, cmd);
		octeon_twsi_wait(sc);
		cmd = octeon_twsi_reg_rd(sc);
		_LE32TOBUF(&buf[len - 1 - resid], (uint32_t)cmd);
		resid -= 4;
	}

	while (resid > 0) {
		cmd = MIO_TWS_SW_TWSI_OP_ONE | MIO_TWS_SW_TWSI_R
		    | (addr << MIO_TWS_SW_TWSI_A_SHIFT);
		octeon_twsi_reg_wr(sc, MIO_TWS_SW_TWSI_OFFSET, cmd);
		octeon_twsi_wait(sc);
		cmd = octeon_twsi_reg_rd(sc);
		buf[len - 1 - resid] = (uint8_t)cmd;
		resid--;
	}

#endif

	octeon_twsi_unlock(sc);
}

static void
octeon_twsi_hlcm_write(struct octeon_twsi_softc *sc, int addr, char *buf,
    size_t len)
{
	uint64_t cmd;
	size_t resid;

	octeon_twsi_lock(sc);

#ifdef notyet

	octeon_twsi_hlcm_setup(sc);

	resid = len;

	while (resid > 4) {
		cmd = MIO_TWS_SW_TWSI_OP_FOUR
		    | (addr << MIO_TWS_SW_TWSI_A_SHIFT)
		    | _BUFTOLE32(&buf[len - 1 - resid]);
		octeon_twsi_reg_wr(sc, MIO_TWS_SW_TWSI_OFFSET, cmd);
		octeon_twsi_wait(sc);
		resid -= 4;
	}

	while (resid > 0) {
		cmd = MIO_TWS_SW_TWSI_OP_ONE
		    | (addr << MIO_TWS_SW_TWSI_A_SHIFT)
		    | buf[len - 1 - resid];
		octeon_twsi_reg_wr(sc, MIO_TWS_SW_TWSI_OFFSET, cmd);
		octeon_twsi_wait(sc);
		resid--;
	}

	/* MIO_TWS_SW_TWSI:V must be zero */

	/* check error */
	if (MIO_TWS_SW_TWSI:R == 0) {
		code = MIO_TWS_SW_TWSI:D;
	}
#endif

	octeon_twsi_unlock(sc);
}

static void
octeon_twsi_hlcm_setup(struct octeon_twsi_softc *sc, ...)
{
	/* XXX */

	_CONTROL_WR(sc, TWSI_CTL, TWSI_CTL_CE | TWSI_CTL_ENAB | TWSI_AAK);
}

static uint8_t
octeon_twsi_hlcm_read_1(struct octeon_twsi_softc *sc, ...)
{
	/* XXX */
	return 0;
}

static uint64_t
octeon_twsi_hlcm_read_4(struct octeon_twsi_softc *sc, ...)
{
	/* XXX */
	return 0;
}

static void
octeon_twsi_hlcm_write_1(struct octeon_twsi_softc *sc, ...)
{
	/* XXX */
}

static void
octeon_twsi_hlcm_write_4(struct octeon_twsi_softc *sc, ...)
{
	/* XXX */
}

#endif

/* -------------------------------------------------------------------------- */

/*
 * High-Level Controller as a Slave
 */

#ifdef notyet

static void
octeon_twsi_hlcs_setup(struct octeon_twsi_softc *sc, ...)
{
	/* XXX */
}

#endif

/* -------------------------------------------------------------------------- */

/*
 * TWSI Control Register operations
 */

#ifdef notyet

static uint8_t
octeon_twsi_control_read(struct octeon_twsi_softc *sc, uint64_t eop_ia)
{
	uint64_t cmd;

	cmd = MIO_TWS_SW_TWSI_OP_EXTEND
	    | (addr << MIO_TWS_SW_TWSI_A_SHIFT)
	    | eop_ia;
	octeon_twsi_reg_wr(sc, MIO_TWS_SW_TWSI_OFFSET, cmd);
	octeon_twsi_wait(sc);
	return (uint8_t)octeon_twsi_reg_rd(sc, MIO_TWS_SW_TWSI_OFFSET);
}

static void
octeon_twsi_control_write(struct octeon_twsi_softc *sc, uint64_t eop_ia,
    char *buf, size_t len)
{
	uint64_t cmd;

	cmd = MIO_TWS_SW_TWSI_OP_EXTEND
	    | (addr << MIO_TWS_SW_TWSI_A_SHIFT)
	    | eop_ia
	    | _BUFTOLE32(&buf[len - 1 - resid]);
	octeon_twsi_reg_wr(sc, MIO_TWS_SW_TWSI_OFFSET, cmd);
	octeon_twsi_wait(sc);
}

#endif

/* -------------------------------------------------------------------------- */

/*
 * Send / receive operations
 */

/* Send (== software to TWSI) */

#ifdef notyet

static void
octeon_twsi_send(struct octeon_twsi_softc *sc, ...)
{
	octeon_twsi_reg_wr(sc, MIO_TWS_SW_TWSI_OFFSET, ...);
}

/* Receive (== TWSI to software) */

static void
octeon_twsi_recv(struct octeon_twsi_softc *sc, ...)
{
	/* XXX */
	octeon_twsi_reg_wr(sc, MIO_TWS_SW_TWSI_OFFSET, ...);
	octeon_twsi_wait(sc, MIO_TWS_SW_TWSI_OFFSET, ...);
	octeon_twsi_reg_rd(sc, MIO_TWS_SW_TWSI_OFFSET, ...);
}

#endif

/* -------------------------------------------------------------------------- */

/*
 * Register accessors
 */

static inline uint64_t
octeon_twsi_reg_rd(struct octeon_twsi_softc *sc, int offset)
{
	return bus_space_read_8(sc->sc_regt, sc->sc_regh, offset);
}

static inline void
octeon_twsi_reg_wr(struct octeon_twsi_softc *sc, int offset, uint64_t value)
{
	bus_space_write_8(sc->sc_regt, sc->sc_regh, offset, value);
}

#ifdef TWSIDEBUG

void
octeon_twsi_reg_dump(struct octeon_twsi_softc *sc, int offset)
{
	octeon_twsi_debug_reg_dump(sc, offset);
}

#endif

/* -------------------------------------------------------------------------- */

/*
 * Test functions
 */

#ifdef TWSITEST

void
octeon_twsi_test(struct octeon_twsi_softc *sc)
{
	octeon_twsi_debug_dumpregs(sc);
}

#endif

/* -------------------------------------------------------------------------- */

#ifdef TWSIDEBUG

/*
 * Debug functions
 *
 *	octeon_twsi_debug_reg_dump
 *	octeon_twsi_debug_dumpregs
 *	octeon_twsi_debug_dumpreg
 */

struct octeon_twsi_reg {
	const char		*name;
	int			offset;
	const char		*format;
};

static const struct octeon_twsi_reg octeon_twsi_regs[] = {
#define	_ENTRY(x)	{ #x, x##_OFFSET, x##_BITS }
	_ENTRY(MIO_TWS_SW_TWSI),
	_ENTRY(MIO_TWS_TWSI_SW),
	_ENTRY(MIO_TWS_INT),
	_ENTRY(MIO_TWS_SW_TWSI_EXT)
#undef	_ENTRY
};

void
octeon_twsi_debug_reg_dump(struct octeon_twsi_softc *sc, int offset)
{
	int i;
	const struct octeon_twsi_reg *reg;

	reg = NULL;
	for (i = 0; i < (int)__arraycount(octeon_twsi_regs); i++)
		if (octeon_twsi_regs[i].offset == offset) {
			reg = &octeon_twsi_regs[i];
			break;
		}
	KASSERT(reg != NULL);

	octeon_twsi_debug_dumpreg(sc, reg);
}

void
octeon_twsi_debug_dumpregs(struct octeon_twsi_softc *sc)
{
	int i;

	for (i = 0; i < (int)__arraycount(octeon_twsi_regs); i++)
		octeon_twsi_debug_dumpreg(sc, &octeon_twsi_regs[i]);
}

void
octeon_twsi_debug_dumpreg(struct octeon_twsi_softc *sc,
    const struct octeon_twsi_reg *reg)
{
	uint64_t value;
	char buf[256];

	value = octeon_twsi_reg_rd(sc, reg->offset);
	snprintb(buf, sizeof(buf), reg->format, value);
	printf("\t%-24s: %s\n", reg->name, buf);
}

#endif
