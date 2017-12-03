/*	$NetBSD: octeon_mpi.c,v 1.2.18.2 2017/12/03 11:36:27 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: octeon_mpi.c,v 1.2.18.2 2017/12/03 11:36:27 jdolecek Exp $");

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

struct octeon_mpi_softc {
	device_t		sc_dev;

	bus_space_tag_t		sc_regt;
	bus_space_handle_t	sc_regh;

	void *sc_ih;				/* XXX Interrupt Handler */

	/* board-specific chip-select hook ops */
	void			(*sc_ops_cs_on)(void);
	void			(*sc_ops_cs_off)(void);
	struct octeon_mpi_controller ctrl;

};
 
static int		octeon_mpi_match(device_t, struct cfdata *,
			    void *);
static void		octeon_mpi_attach(device_t, device_t,
			    void *);
#if 0
static int		octeon_mpi_intr(void *);
#endif
void			octeon_mpi_read(void *, u_int,
			    u_int, size_t, uint8_t *);
void			octeon_mpi_write(void *, u_int,
			    u_int, size_t, uint8_t *);
static void		octeon_mpi_xfer(struct octeon_mpi_softc *, size_t,
			    size_t);
static void		octeon_mpi_wait(struct octeon_mpi_softc *);
static inline uint64_t	octeon_mpi_reg_rd(struct octeon_mpi_softc *, int);
static inline void	octeon_mpi_reg_wr(struct octeon_mpi_softc *, int,
			    uint64_t);

/* SPI service routines */
int octeon_mpi_configure(void *, void *, void *);

#define GETREG(sc, x)	\
	bus_space_read_8(sc->sc_regt, sc->sc_regh, x)
#define PUTREG(sc, x, v)	\
	bus_space_write_8(sc->sc_regt, sc->sc_regh, x, v)

CFATTACH_DECL_NEW(octeon_mpi, sizeof(struct octeon_mpi_softc),
    octeon_mpi_match, octeon_mpi_attach, NULL, NULL);


static int
spi_print(void *aux, const char *pnp)
{
	aprint_normal(" spi");
	return (UNCONF);
}

static int
octeon_mpi_match(device_t parent, struct cfdata *cf, void *aux)
{
	struct iobus_attach_args *aa = aux;

	if (strcmp(cf->cf_name, aa->aa_name) != 0)
		return 0;
	return 1;
}

static void
octeon_mpi_attach(device_t parent, device_t self, void *aux)
{
	struct octeon_mpi_softc *sc = device_private(self);
	struct iobus_attach_args *aa = aux;
	struct octeon_mpi_attach_args pa;
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
	sc->ctrl.sct_configure = octeon_mpi_configure;
	sc->ctrl.sct_read = octeon_mpi_read;
	sc->ctrl.sct_write = octeon_mpi_write;
	pa.octeon_mpi_ctrl = &(sc->ctrl);

	/* Enable SPI mode */
#if 0
	octeon_mpi_reg_wr(sc, MPI_CFG_OFFSET,
	    (0x7d << MPI_CFG_CLKDIV_SHIFT) | MPI_CFG_CSENA | MPI_CFG_ENABLE | MPI_CFG_INT_ENA);
	/* Enable device interrupts */
	sc->sc_ih = octeon_intr_establish(ffs64(CIU_INTX_SUM0_MPI) - 1,
		IPL_SERIAL, octeon_mpi_intr, sc);
	if (sc->sc_ih == NULL)
		panic("l2sw: can't establish interrupt\n");
#else
	octeon_mpi_reg_wr(sc, MPI_CFG_OFFSET,
	    (0x7d << MPI_CFG_CLKDIV_SHIFT) | MPI_CFG_CSENA | MPI_CFG_ENABLE);
#endif
	octeon_mpi_reg_wr(sc, MPI_TX_OFFSET, 0);

	config_found_ia(&sc->sc_dev, "octeon_mpi", &pa, spi_print);
}

#if 0
static int
octeon_mpi_intr(void *arg)
{
	struct octeon_mpi_softc *sc = arg;

	octeon_mpi_recv(sc);

	/* Clear interrupts? */

	return 1;
}
#endif

void
octeon_mpi_read(void *parent, u_int cmd, u_int addr,
    size_t len, uint8_t *data)
{
	struct octeon_mpi_softc *sc = (void *)parent;
	int i;

	octeon_mpi_reg_wr(sc, MPI_DAT0_OFFSET, cmd);
	octeon_mpi_reg_wr(sc, MPI_DAT1_OFFSET, addr);

	octeon_mpi_xfer(sc, 2, 2 + len);

	for (i = 0; i < (int)len; i++)
		data[i] = octeon_mpi_reg_rd(sc, MPI_DAT2_OFFSET + i * 0x8);
}

void
octeon_mpi_write(void *parent, u_int cmd, u_int addr,
    size_t len, uint8_t *data)
{
	struct octeon_mpi_softc *sc = (void *)parent;
	int i;

	octeon_mpi_reg_wr(sc, MPI_DAT0_OFFSET, cmd);
	octeon_mpi_reg_wr(sc, MPI_DAT1_OFFSET, addr);

	for (i = 0; i < (int)len; i++)
		octeon_mpi_reg_wr(sc, MPI_DAT2_OFFSET + i * 0x8, data[i]);

	octeon_mpi_xfer(sc, 2 + len, 2 + len);
}

static void
octeon_mpi_xfer(struct octeon_mpi_softc *sc, size_t tx, size_t total)
{
	if (sc->sc_ops_cs_on != NULL)
		(*sc->sc_ops_cs_on)();

	octeon_mpi_reg_wr(sc, MPI_TX_OFFSET,
	    (tx << MPI_TX_TXNUM_SHIFT) | (total << MPI_TX_TOTNUM_SHIFT));
	octeon_mpi_wait(sc);

	if (sc->sc_ops_cs_off != NULL)
		(*sc->sc_ops_cs_off)();
}

static void
octeon_mpi_wait(struct octeon_mpi_softc *sc)
{
	uint64_t tmp;
	
	/* XXX ltsleep & interrupt */
	tmp = octeon_mpi_reg_rd(sc, MPI_STS_OFFSET);
	while (ISSET(tmp, MPI_STS_BUSY)) {
		delay(10);
		tmp = octeon_mpi_reg_rd(sc, MPI_STS_OFFSET);
	}
}

static inline uint64_t
octeon_mpi_reg_rd(struct octeon_mpi_softc *sc, int offset)
{
	return GETREG(sc, offset);
}

static inline void
octeon_mpi_reg_wr(struct octeon_mpi_softc *sc, int offset, uint64_t datum)
{
	PUTREG(sc, offset, datum);
}

int
octeon_mpi_configure(void *arg, void *cs_on, void *cs_off)
{
	struct octeon_mpi_softc *sc = arg;

	sc->sc_ops_cs_on = cs_on;
	sc->sc_ops_cs_off = cs_off;

	return 0;
}
