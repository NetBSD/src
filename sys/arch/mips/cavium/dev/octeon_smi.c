/*	$NetBSD: octeon_smi.c,v 1.1.2.2 2015/06/06 14:40:01 skrll Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: octeon_smi.c,v 1.1.2.2 2015/06/06 14:40:01 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <mips/locore.h>
#include <mips/cavium/octeonvar.h>
#include <mips/cavium/dev/octeon_fpavar.h>
#include <mips/cavium/dev/octeon_pipreg.h>
#include <mips/cavium/dev/octeon_smireg.h>
#include <mips/cavium/dev/octeon_smivar.h>

static void		octeon_smi_enable(struct octeon_smi_softc *);

/* XXX */
void
octeon_smi_init(struct octeon_smi_attach_args *aa,
    struct octeon_smi_softc **rsc)
{
	struct octeon_smi_softc *sc;
	int status;

	sc = malloc(sizeof(*sc), M_DEVBUF, M_WAITOK | M_ZERO);
	if (sc == NULL)
		panic("can't allocate memory: %s", __func__);

	sc->sc_port = aa->aa_port;
	sc->sc_regt = aa->aa_regt;

	status = bus_space_map(sc->sc_regt, SMI_BASE, SMI_SIZE, 0,
	    &sc->sc_regh);
	if (status != 0)
		panic("can't map %s space", "smi register");

	octeon_smi_enable(sc);

	*rsc = sc;
}

#define	_SMI_RD8(sc, off) \
	bus_space_read_8((sc)->sc_regt, (sc)->sc_regh, (off))
#define	_SMI_WR8(sc, off, v) \
	bus_space_write_8((sc)->sc_regt, (sc)->sc_regh, (off), (v))

int
octeon_smi_read(struct octeon_smi_softc *sc, int phy_addr, int reg)
{
	uint64_t smi_rd;
	int timo;

	_SMI_WR8(sc, SMI_CMD_OFFSET, SMI_CMD_PHY_OP | 
	    (phy_addr << SMI_CMD_PHY_ADR_SHIFT) |
	    (reg << SMI_CMD_REG_ADR_SHIFT));

	timo = 10000;
	smi_rd = _SMI_RD8(sc, SMI_RD_DAT_OFFSET);
	while (ISSET(smi_rd, SMI_RD_DAT_PENDING)) {
		if (timo-- == 0)
			break;
		delay(10);
		smi_rd = _SMI_RD8(sc, SMI_RD_DAT_OFFSET);
	}
	if (ISSET(smi_rd, SMI_RD_DAT_PENDING)) {
		return -1;
	}

	return ISSET(smi_rd, SMI_RD_DAT_VAL) ? (smi_rd & SMI_RD_DAT_DAT) : 0;
}

void
octeon_smi_write(struct octeon_smi_softc *sc, int phy_addr, int reg, int value)
{
	uint64_t smi_wr;
	int timo;

	smi_wr = 0;
	SET(smi_wr, value);
	_SMI_WR8(sc, SMI_WR_DAT_OFFSET, smi_wr);

	_SMI_WR8(sc, SMI_CMD_OFFSET, (phy_addr << SMI_CMD_PHY_ADR_SHIFT) |
	    (reg << SMI_CMD_REG_ADR_SHIFT));

	timo = 10000;
	smi_wr = _SMI_RD8(sc, SMI_WR_DAT_OFFSET);
	while (ISSET(smi_wr, SMI_WR_DAT_PENDING)) {
		if (timo-- == 0)
			break;
		delay(10);
		smi_wr = _SMI_RD8(sc, SMI_WR_DAT_OFFSET);
	}
	if (ISSET(smi_wr, SMI_WR_DAT_PENDING)) {
		/* XXX log */
		printf("ERROR: cnmac_mii_writereg(0x%x, 0x%x, 0x%x) timed out.\n",
		    phy_addr, reg, value);
	}
}

static void
octeon_smi_enable(struct octeon_smi_softc *sc)
{
	_SMI_WR8(sc, SMI_EN_OFFSET, SMI_EN_EN);
}

void
octeon_smi_set_clock(struct octeon_smi_softc *sc, uint64_t clock)
{
	_SMI_WR8(sc, SMI_CLK_OFFSET, clock);
}

