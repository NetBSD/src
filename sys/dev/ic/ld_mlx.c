/*	$NetBSD: ld_mlx.c,v 1.10.6.1 2006/04/22 11:38:55 simonb Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Mylex DAC960 front-end for ld(4) driver.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ld_mlx.c,v 1.10.6.1 2006/04/22 11:38:55 simonb Exp $");

#include "rnd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/endian.h>
#include <sys/dkio.h>
#include <sys/disk.h>
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <machine/vmparam.h>
#include <machine/bus.h>

#include <dev/ldvar.h>

#include <dev/ic/mlxreg.h>
#include <dev/ic/mlxio.h>
#include <dev/ic/mlxvar.h>

struct ld_mlx_softc {
	struct	ld_softc sc_ld;
	int	sc_hwunit;
};

static void	ld_mlx_attach(struct device *, struct device *, void *);
static int	ld_mlx_detach(struct device *, int);
static int	ld_mlx_dobio(struct ld_mlx_softc *, void *, int, int, int,
			     struct buf *);
static int	ld_mlx_dump(struct ld_softc *, void *, int, int);
static void	ld_mlx_handler(struct mlx_ccb *);
static int	ld_mlx_match(struct device *, struct cfdata *, void *);
static int	ld_mlx_start(struct ld_softc *, struct buf *);

CFATTACH_DECL(ld_mlx, sizeof(struct ld_mlx_softc),
    ld_mlx_match, ld_mlx_attach, ld_mlx_detach, NULL);

static int
ld_mlx_match(struct device *parent, struct cfdata *match, void *aux)
{

	return (1);
}

static void
ld_mlx_attach(struct device *parent, struct device *self, void *aux)
{
	struct mlx_attach_args *mlxa;
	struct ld_mlx_softc *sc;
	struct ld_softc *ld;
	struct mlx_softc *mlx;
	struct mlx_sysdrive *ms;
	const char *statestr;

	sc = (struct ld_mlx_softc *)self;
	ld = &sc->sc_ld;
	mlx = (struct mlx_softc *)parent;
	mlxa = aux;
	ms = &mlx->mlx_sysdrive[mlxa->mlxa_unit];

	sc->sc_hwunit = mlxa->mlxa_unit;
	ld->sc_maxxfer = MLX_MAX_XFER;
	ld->sc_secsize = MLX_SECTOR_SIZE;
	ld->sc_maxqueuecnt = 1;
	ld->sc_start = ld_mlx_start;
	ld->sc_dump = ld_mlx_dump;
	ld->sc_secperunit = ms->ms_size;

	/*
	 * Report on current status, and attach to the ld driver proper.
	 */
	switch (ms->ms_state) {
	case MLX_SYSD_ONLINE:
		statestr = "online";
		ld->sc_flags = LDF_ENABLED;
		break;

	case MLX_SYSD_CRITICAL:
		statestr = "critical";
		ld->sc_flags = LDF_ENABLED;
		break;

	case MLX_SYSD_OFFLINE:
		statestr = "offline";
		break;

	default:
		statestr = "state unknown";
		break;
	}

	if (ms->ms_raidlevel == MLX_SYS_DRV_JBOD)
		aprint_normal(": JBOD, %s\n", statestr);
	else
		aprint_normal(": RAID%d, %s\n", ms->ms_raidlevel, statestr);

	ldattach(ld);
}

static int
ld_mlx_detach(struct device *dv, int flags)
{
	int rv;

	if ((rv = ldbegindetach((struct ld_softc *)dv, flags)) != 0)
		return (rv);
	ldenddetach((struct ld_softc *)dv);
	mlx_flush((struct mlx_softc *)device_parent(dv), 1);

	return (0);
}

static int
ld_mlx_dobio(struct ld_mlx_softc *sc, void *data, int datasize, int blkno,
	     int dowrite, struct buf *bp)
{
	struct mlx_ccb *mc;
	struct mlx_softc *mlx;
	int s, rv;
	bus_addr_t sgphys;

	mlx = (struct mlx_softc *)device_parent(&sc->sc_ld.sc_dv);

	if ((rv = mlx_ccb_alloc(mlx, &mc, bp == NULL)) != 0)
		return (rv);

	/* Map the data transfer. */
	rv = mlx_ccb_map(mlx, mc, data, datasize,
	    dowrite ? MC_XFER_OUT : MC_XFER_IN);
	if (rv != 0) {
		mlx_ccb_free(mlx, mc);
		return (rv);
	}

	/* Build the command. */
	sgphys = mlx->mlx_sgls_paddr + (MLX_SGL_SIZE * mc->mc_ident);
	datasize /= MLX_SECTOR_SIZE;

	if (mlx->mlx_ci.ci_iftype <= 2)
		mlx_make_type1(mc,
		    dowrite ? MLX_CMD_WRITESG_OLD : MLX_CMD_READSG_OLD,
		    datasize & 0xff, blkno, sc->sc_hwunit, sgphys,
		    mc->mc_nsgent);
	else
		mlx_make_type5(mc,
		    dowrite ? MLX_CMD_WRITESG : MLX_CMD_READSG,
		    datasize & 0xff,
		    (sc->sc_hwunit << 3) | ((datasize >> 8) & 0x07),
		    blkno, sgphys, mc->mc_nsgent);

	if (bp == NULL) {
		s = splbio();
		rv = mlx_ccb_poll(mlx, mc, 10000);
		mlx_ccb_unmap(mlx, mc);
		mlx_ccb_free(mlx, mc);
		splx(s);
	} else {
 		mc->mc_mx.mx_handler = ld_mlx_handler;
		mc->mc_mx.mx_context = bp;
		mc->mc_mx.mx_dv = &sc->sc_ld.sc_dv;
		mlx_ccb_enqueue(mlx, mc);
		rv = 0;
	}

	return (rv);
}

static int
ld_mlx_start(struct ld_softc *ld, struct buf *bp)
{

	return (ld_mlx_dobio((struct ld_mlx_softc *)ld, bp->b_data,
	    bp->b_bcount, bp->b_rawblkno, (bp->b_flags & B_READ) == 0, bp));
}

static void
ld_mlx_handler(struct mlx_ccb *mc)
{
	struct buf *bp;
	struct mlx_context *mx;
	struct ld_mlx_softc *sc;
	struct mlx_softc *mlx;

	mx = &mc->mc_mx;
	bp = mx->mx_context;
	sc = (struct ld_mlx_softc *)mx->mx_dv;
	mlx = (struct mlx_softc *)device_parent(&sc->sc_ld.sc_dv);

	if (mc->mc_status != MLX_STATUS_OK) {
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		bp->b_resid = bp->b_bcount;

		if (mc->mc_status == MLX_STATUS_RDWROFFLINE)
			printf("%s: drive offline\n",
			    sc->sc_ld.sc_dv.dv_xname);
		else
			printf("%s: I/O error - %s\n",
			    sc->sc_ld.sc_dv.dv_xname,
			    mlx_ccb_diagnose(mc));
	} else
		bp->b_resid = 0;

	mlx_ccb_unmap(mlx, mc);
	mlx_ccb_free(mlx, mc);
	lddone(&sc->sc_ld, bp);
}

static int
ld_mlx_dump(struct ld_softc *ld, void *data, int blkno, int blkcnt)
{

	return (ld_mlx_dobio((struct ld_mlx_softc *)ld, data,
	    blkcnt * ld->sc_secsize, blkno, 1, NULL));
}
