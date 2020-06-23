/*	$NetBSD: octeon_smi.c,v 1.5 2020/06/23 05:18:02 simonb Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: octeon_smi.c,v 1.5 2020/06/23 05:18:02 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>

#include <mips/locore.h>
#include <mips/cavium/octeonvar.h>
#include <mips/cavium/dev/octeon_fpareg.h>
#include <mips/cavium/dev/octeon_fpavar.h>
#include <mips/cavium/dev/octeon_pipreg.h>
#include <mips/cavium/dev/octeon_smireg.h>
#include <mips/cavium/dev/octeon_smivar.h>
#include <mips/cavium/include/iobusvar.h>

/*
 * System Management Interface
 *
 *
 * CN30XX  - 1 SMI interface
 * CN31XX  - 1 SMI interface
 * CN38XX  - 1 SMI interface
 * CN50XX  - 1 SMI interface
 * CN52XX  - 2 SMI interfaces
 * CN56XX  - 2 SMI interfaces
 * CN58XX  - 1 SMI interface
 * CN61XX  - 2 SMI interfaces
 * CN63XX  - 2 SMI interfaces
 * CN66XX  - 2 SMI interfaces
 * CN68XX  - 4 SMI interfaces
 * CN70XX  - 2 SMI interfaces
 * CN73XX  - 2 SMI interfaces
 * CN78XX  - 4 SMI interfaces
 * CNF71XX - 2 SMI interfaces
 * CNF75XX - 2 SMI interfaces
 */

static int	octsmi_match(device_t, struct cfdata *, void *);
static void	octsmi_attach(device_t, device_t, void *);

struct octsmi_softc *smi_list;	/* XXX up to 4 SMIs on CN68XX,CN78XX */

#define	_SMI_RD8(sc, off) \
	bus_space_read_8((sc)->sc_regt, (sc)->sc_regh, (off))
#define	_SMI_WR8(sc, off, v) \
	bus_space_write_8((sc)->sc_regt, (sc)->sc_regh, (off), (v))

CFATTACH_DECL_NEW(octsmi, sizeof(struct octsmi_softc),
    octsmi_match, octsmi_attach, NULL, NULL);

static int
octsmi_match(device_t parent, struct cfdata *cf, void *aux)
{
	struct iobus_attach_args *aa = aux;

	if (strcmp(cf->cf_name, aa->aa_name) != 0)
		return 0;
	if (aa->aa_unitno < SMI_NUNITS)
		return 1;
	else
		return 0;
}

static void
octsmi_attach(device_t parent, device_t self, void *aux)
{
	struct octsmi_softc *sc = device_private(self);
	struct iobus_attach_args *aa = aux;
	int status;

	sc->sc_dev = self;
	sc->sc_regt = aa->aa_bust;

	aprint_normal("\n");

	status = bus_space_map(sc->sc_regt, aa->aa_unit->addr, SMI_SIZE, 0,
	    &sc->sc_regh);
	if (status != 0) {
		aprint_error_dev(self, "could not map registers\n");
		return;
	}

	smi_list = sc;

	const uint64_t magic_value =
	    SMI_CLK_PREAMBLE |
	    __SHIFTIN(0x4, SMI_CLK_SAMPLE) |		/* XXX magic 0x4 */
	    __SHIFTIN(0x64, SMI_CLK_PHASE);		/* XXX magic 0x64 */
	_SMI_WR8(sc, SMI_CLK_OFFSET, magic_value);
	_SMI_WR8(sc, SMI_EN_OFFSET, SMI_EN_EN);
}

int
octsmi_read(struct octsmi_softc *sc, int phy_addr, int reg, uint16_t *val)
{
	uint64_t smi_rd;
	int timo;

	_SMI_WR8(sc, SMI_CMD_OFFSET, 
	    __SHIFTIN(SMI_CMD_PHY_OP_READ, SMI_CMD_PHY_OP) |
	    __SHIFTIN(phy_addr, SMI_CMD_PHY_ADR) |
	    __SHIFTIN(reg, SMI_CMD_REG_ADR));

	timo = 10000;
	smi_rd = _SMI_RD8(sc, SMI_RD_DAT_OFFSET);
	while (ISSET(smi_rd, SMI_RD_DAT_PENDING)) {
		if (timo-- == 0)
			return ETIMEDOUT;
		delay(10);
		smi_rd = _SMI_RD8(sc, SMI_RD_DAT_OFFSET);
	}

	if (ISSET(smi_rd, SMI_RD_DAT_VAL)) {
		*val = (smi_rd & SMI_RD_DAT_DAT);
		return 0;
	}

	return -1;
}

int
octsmi_write(struct octsmi_softc *sc, int phy_addr, int reg, uint16_t value)
{
	uint64_t smi_wr;
	int timo;

	smi_wr = 0;
	SET(smi_wr, value);
	_SMI_WR8(sc, SMI_WR_DAT_OFFSET, smi_wr);

	_SMI_WR8(sc, SMI_CMD_OFFSET,
	    __SHIFTIN(SMI_CMD_PHY_OP_WRITE, SMI_CMD_PHY_OP) |
	    __SHIFTIN(phy_addr, SMI_CMD_PHY_ADR) |
	    __SHIFTIN(reg, SMI_CMD_REG_ADR));

	timo = 10000;
	smi_wr = _SMI_RD8(sc, SMI_WR_DAT_OFFSET);
	while (ISSET(smi_wr, SMI_WR_DAT_PENDING)) {
		if (timo-- == 0) {
			return ETIMEDOUT;
		}
		delay(10);
		smi_wr = _SMI_RD8(sc, SMI_WR_DAT_OFFSET);
	}
	if (ISSET(smi_wr, SMI_WR_DAT_PENDING)) {
		/* XXX log */
		printf("ERROR: octsmi_write(0x%x, 0x%x, 0x%hx) timed out.\n",
		    phy_addr, reg, value);
	}

	return 0;
}

struct octsmi_softc *
octsmi_lookup(int phandle, int port)
{
	struct octsmi_softc *smi;

	/* XXX deal with more than one SMI ... */
	smi = smi_list;

	return smi;
}
