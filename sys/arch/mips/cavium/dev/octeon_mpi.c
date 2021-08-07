/*	$NetBSD: octeon_mpi.c,v 1.7 2021/08/07 16:18:59 thorpej Exp $	*/

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


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: octeon_mpi.c,v 1.7 2021/08/07 16:18:59 thorpej Exp $");

#include "opt_octeon.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/lock.h>
#include <sys/cdefs.h>

#include <mips/locore.h>
#include <sys/bus.h>

#include <mips/cavium/include/iobusvar.h>
#include <mips/cavium/dev/octeon_mpireg.h>
#include <mips/cavium/dev/octeon_mpivar.h>
#include <mips/cavium/dev/octeon_ciureg.h>

struct octmpi_softc {
	device_t		sc_dev;

	bus_space_tag_t		sc_regt;
	bus_space_handle_t	sc_regh;

	void *sc_ih;				/* XXX Interrupt Handler */

	/* board-specific chip-select hook ops */
	void			(*sc_ops_cs_on)(void);
	void			(*sc_ops_cs_off)(void);
	struct octmpi_controller ctrl;

};
 
static int		octmpi_match(device_t, struct cfdata *, void *);
static void		octmpi_attach(device_t, device_t, void *);
#if 0
static int		octmpi_intr(void *);
#endif
void			octmpi_read(void *, u_int, u_int, size_t, uint8_t *);
void			octmpi_write(void *, u_int, u_int, size_t, uint8_t *);
static void		octmpi_xfer(struct octmpi_softc *, size_t, size_t);
static void		octmpi_wait(struct octmpi_softc *);
static inline uint64_t	octmpi_reg_rd(struct octmpi_softc *, int);
static inline void	octmpi_reg_wr(struct octmpi_softc *, int, uint64_t);

/* SPI service routines */
int octmpi_configure(void *, void *, void *);

#define GETREG(sc, x)	\
	bus_space_read_8(sc->sc_regt, sc->sc_regh, x)
#define PUTREG(sc, x, v)	\
	bus_space_write_8(sc->sc_regt, sc->sc_regh, x, v)

CFATTACH_DECL_NEW(octeon_mpi, sizeof(struct octmpi_softc),
    octmpi_match, octmpi_attach, NULL, NULL);


static int
spi_print(void *aux, const char *pnp)
{
	aprint_normal(" spi");
	return (UNCONF);
}

static int
octmpi_match(device_t parent, struct cfdata *cf, void *aux)
{
	struct iobus_attach_args *aa = aux;

	if (strcmp(cf->cf_name, aa->aa_name) != 0)
		return 0;
	return 1;
}

static void
octmpi_attach(device_t parent, device_t self, void *aux)
{
	struct octmpi_softc *sc = device_private(self);
	struct iobus_attach_args *aa = aux;
	struct octmpi_attach_args pa;
	int status;

	sc->sc_regt = aa->aa_bust;

	/*
	 * Map registers.
	 */
	status = bus_space_map(sc->sc_regt, MPI_BASE, MPI_SIZE, 0,
	    &sc->sc_regh);
	if (status != 0)
		panic(": can't map register");

	aprint_normal(": Octeon MPI/SPI Controller\n");

	/*
	 * Initialize MPI/SPI Controller
	 */
	sc->ctrl.sc_bust = sc->sc_regt;
	sc->ctrl.sc_bush = sc->sc_regh;
	sc->ctrl.sct_cookie = sc;
	sc->ctrl.sct_configure = octmpi_configure;
	sc->ctrl.sct_read = octmpi_read;
	sc->ctrl.sct_write = octmpi_write;
	pa.octmpi_ctrl = &(sc->ctrl);

	/* Enable SPI mode */
#if 0
	octmpi_reg_wr(sc, MPI_CFG_OFFSET,
	    (0x7d << MPI_CFG_CLKDIV_SHIFT) | MPI_CFG_CSENA | MPI_CFG_ENABLE | MPI_CFG_INT_ENA);
	/* Enable device interrupts */
	sc->sc_ih = octeon_intr_establish(CIU_INT_MPI, IPL_SERIAL, octmpi_intr, sc);
	if (sc->sc_ih == NULL)
		panic("l2sw: can't establish interrupt\n");
#else
	octmpi_reg_wr(sc, MPI_CFG_OFFSET,
	    (0x7d << MPI_CFG_CLKDIV_SHIFT) | MPI_CFG_CSENA | MPI_CFG_ENABLE);
#endif
	octmpi_reg_wr(sc, MPI_TX_OFFSET, 0);

	config_found(&sc->sc_dev, &pa, spi_print,
	    CFARGS(.iattr = "octmpi"));
}

#if 0
static int
octmpi_intr(void *arg)
{
	struct octmpi_softc *sc = arg;

	octmpi_recv(sc);

	/* Clear interrupts? */

	return 1;
}
#endif

void
octmpi_read(void *parent, u_int cmd, u_int addr, size_t len, uint8_t *data)
{
	struct octmpi_softc *sc = (void *)parent;
	int i;

	octmpi_reg_wr(sc, MPI_DAT0_OFFSET, cmd);
	octmpi_reg_wr(sc, MPI_DAT1_OFFSET, addr);

	octmpi_xfer(sc, 2, 2 + len);

	for (i = 0; i < (int)len; i++)
		data[i] = octmpi_reg_rd(sc, MPI_DAT2_OFFSET + i * 0x8);
}

void
octmpi_write(void *parent, u_int cmd, u_int addr, size_t len, uint8_t *data)
{
	struct octmpi_softc *sc = (void *)parent;
	int i;

	octmpi_reg_wr(sc, MPI_DAT0_OFFSET, cmd);
	octmpi_reg_wr(sc, MPI_DAT1_OFFSET, addr);

	for (i = 0; i < (int)len; i++)
		octmpi_reg_wr(sc, MPI_DAT2_OFFSET + i * 0x8, data[i]);

	octmpi_xfer(sc, 2 + len, 2 + len);
}

static void
octmpi_xfer(struct octmpi_softc *sc, size_t tx, size_t total)
{
	if (sc->sc_ops_cs_on != NULL)
		(*sc->sc_ops_cs_on)();

	octmpi_reg_wr(sc, MPI_TX_OFFSET,
	    (tx << MPI_TX_TXNUM_SHIFT) | (total << MPI_TX_TOTNUM_SHIFT));
	octmpi_wait(sc);

	if (sc->sc_ops_cs_off != NULL)
		(*sc->sc_ops_cs_off)();
}

static void
octmpi_wait(struct octmpi_softc *sc)
{
	uint64_t tmp;
	
	/* XXX ltsleep & interrupt */
	tmp = octmpi_reg_rd(sc, MPI_STS_OFFSET);
	while (ISSET(tmp, MPI_STS_BUSY)) {
		delay(10);
		tmp = octmpi_reg_rd(sc, MPI_STS_OFFSET);
	}
}

static inline uint64_t
octmpi_reg_rd(struct octmpi_softc *sc, int offset)
{

	return GETREG(sc, offset);
}

static inline void
octmpi_reg_wr(struct octmpi_softc *sc, int offset, uint64_t datum)
{

	PUTREG(sc, offset, datum);
}

int
octmpi_configure(void *arg, void *cs_on, void *cs_off)
{
	struct octmpi_softc *sc = arg;

	sc->sc_ops_cs_on = cs_on;
	sc->sc_ops_cs_off = cs_off;

	return 0;
}
