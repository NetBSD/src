/*	$NetBSD: tx39io.c,v 1.7 2000/10/22 10:42:32 uch Exp $ */

/*-
 * Copyright (c) 1999, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
#undef TX39IODEBUG
#include "opt_tx39_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/tx39icureg.h>
#define __TX39IO_PRIVATE
#include <hpcmips/tx/tx39iovar.h>
#include <hpcmips/tx/tx39ioreg.h>

#ifdef TX39IODEBUG
#define	DPRINTF(arg) printf arg
#else
#define	DPRINTF(arg)
#endif

#define ISSET(x, s)	((x) & (1 << (s)))

int	tx39io_match(struct device *, struct cfdata *, void *);
void	tx39io_attach(struct device *, struct device *, void *);

struct cfattach tx39io_ca = {
	sizeof(struct tx39io_softc), tx39io_match, tx39io_attach
};

/* IO/MFIO common */
static void *port_intr_establish(void *, int, int (*)(void *), void *);
static void port_intr_disestablish(void *, void *);
/* MFIO */
static int mfio_in(void *, int);
static void mfio_out(void *, int, int);
static void mfio_intr_map(void *, int, int *, int *);
static void mfio_dump(void *);
static void mfio_update(void *);
/* IO */
#ifdef TX391X
static int tx391x_io_in(void *, int);
static void tx391x_io_out(void *, int, int);
static void tx391x_io_update(void *);
static void tx391x_io_intr_map(void *, int, int *, int *);
#endif
#ifdef TX392X
static int tx392x_io_in(void *, int);
static void tx392x_io_out(void *, int, int);
static void tx392x_io_update(void *);
static void tx392x_io_intr_map(void *, int, int *, int *);
#endif
static void io_dump(void *);

static void __print_port_status(struct tx39io_port_status *, int);

int
tx39io_match(struct device *parent, struct cfdata *cf, void *aux)
{
	return ATTACH_FIRST; /* 1st attach group of txsim */
}

void
tx39io_attach(struct device *parent, struct device *self, void *aux)
{
	struct txsim_attach_args *ta = aux;
	struct tx39io_softc *sc = (void *)self;
	tx_chipset_tag_t tc;
	struct txio_ops *ops_io = &sc->sc_io_ops;
	struct txio_ops *ops_mfio = &sc->sc_mfio_ops;
	
	tc = sc->sc_tc = ta->ta_tc;

	printf("\n");
	sc->sc_stat_io_mask = ~(1 << 5); /* exclude Plum2 INT */
	sc->sc_stat_mfio_mask = ~(0x3|(0x3 << 23));

	/* IO */
	ops_io->_v			= sc;
	ops_io->_group			= IO;
	ops_io->_intr_establish		= port_intr_establish;
	ops_io->_intr_disestablish	= port_intr_disestablish;
	ops_io->_dump			= io_dump;
	if (IS_TX391X(tc)) {
#ifdef TX391X
		ops_io->_in		= tx391x_io_in;
		ops_io->_out		= tx391x_io_out;
		ops_io->_intr_map	= tx391x_io_intr_map;
		ops_io->_update		= tx391x_io_update;
#endif
	} else if (IS_TX392X(tc)) {
#ifdef TX392X
		ops_io->_in		= tx392x_io_in;
		ops_io->_out		= tx392x_io_out;
		ops_io->_intr_map	= tx392x_io_intr_map;
		ops_io->_update		= tx392x_io_update;
#endif
	}
	tx_conf_register_ioman(tc, ops_io);

	/* MFIO */
	ops_mfio->_v			= sc;
	ops_mfio->_group		= MFIO;
	ops_mfio->_in			= mfio_in;
	ops_mfio->_out			= mfio_out;
	ops_mfio->_intr_map		= mfio_intr_map;
	ops_mfio->_intr_establish	= port_intr_establish;
	ops_mfio->_intr_disestablish	= port_intr_disestablish;
	ops_mfio->_update		= mfio_update;
	ops_mfio->_dump			= mfio_dump;

	tx_conf_register_ioman(tc, ops_mfio);

	tx_ioman_update(IO);
	tx_ioman_update(MFIO);

#ifdef TX39IODEBUG
	tx_ioman_dump(IO);
	tx_ioman_dump(MFIO);
#else
	printf("IO i0x%08x o0x%08x MFIO i0x%08x o0x%08x\n",
	       sc->sc_stat_io.in, sc->sc_stat_io.out,
	       sc->sc_stat_mfio.in, sc->sc_stat_mfio.out);
#endif
}

/* 
 * TX391X, TX392X common 
 */
static void *
port_intr_establish(void *arg, int edge, int (*func)(void *), void *func_arg)
{
	struct tx39io_softc *sc = arg;
	return tx_intr_establish(sc->sc_tc, edge, IST_EDGE, IPL_CLOCK, func,
				 func_arg);
}

static void
port_intr_disestablish(void *arg, void *ih)
{
	struct tx39io_softc *sc = arg;
	tx_intr_disestablish(sc->sc_tc, ih);
}

static void
mfio_out(void *arg, int port, int onoff)
{
	struct tx39io_softc *sc = arg;
	tx_chipset_tag_t tc;
	txreg_t reg, pos;

	DPRINTF(("%s: port #%d\n", __FUNCTION__, port));
	tc = sc->sc_tc;
	/* MFIO */
	pos = 1 << port;
#ifdef DIAGNOSTIC
	if (!(sc->sc_stat_mfio.dir & pos)) {
		panic("%s: MFIO%d is not output port.\n",
		      sc->sc_dev.dv_xname, port);
	}
#endif
	reg = tx_conf_read(tc, TX39_IOMFIODATAOUT_REG);
	if (onoff)
		reg |= pos;
	else
		reg &= ~pos;
	tx_conf_write(tc, TX39_IOMFIODATAOUT_REG, reg);
}

static int
mfio_in(void *arg, int port)
{
	struct tx39io_softc *sc __attribute__((__unused__)) = arg ;
	DPRINTF(("%s: port #%d\n", __FUNCTION__, port));
	return tx_conf_read(sc->sc_tc, TX39_IOMFIODATAIN_REG) & (1 << port);
}

static void
mfio_intr_map(void *arg, int port, int *pedge, int *nedge)
{
	if (pedge)
		*pedge = MAKEINTR(3, (1 << port));
	if (nedge)
		*nedge = MAKEINTR(4, (1 << port));
}

static void
mfio_update(void *arg)
{
	struct tx39io_softc *sc = arg;
	tx_chipset_tag_t tc = sc->sc_tc;
	struct tx39io_port_status *stat_mfio = &sc->sc_stat_mfio;

	sc->sc_ostat_mfio = *stat_mfio; /* save old status */
	stat_mfio->dir		= tx_conf_read(tc, TX39_IOMFIODATADIR_REG);
	stat_mfio->in		= tx_conf_read(tc, TX39_IOMFIODATAIN_REG);
	stat_mfio->out		= tx_conf_read(tc, TX39_IOMFIODATAOUT_REG);
	stat_mfio->power	= tx_conf_read(tc, TX39_IOMFIOPOWERDWN_REG);
	stat_mfio->u.select	= tx_conf_read(tc, TX39_IOMFIODATASEL_REG);
}

#ifdef TX391X
/* 
 * TMPR3912 specific 
 */
int
tx391x_io_in(void *arg, int port)
{
	struct tx39io_softc *sc __attribute__((__unused__)) = arg;
	txreg_t reg = tx_conf_read(sc->sc_tc, TX39_IOCTRL_REG);

	DPRINTF(("%s: port #%d\n", __FUNCTION__, port));
	return  TX391X_IOCTRL_IODIN(reg) & (1 << port);
}

void
tx391x_io_out(void *arg, int port, int onoff)
{
#ifdef DIAGNOSTIC
	const char *devname;
#endif
	struct tx39io_softc *sc = arg;
	tx_chipset_tag_t tc;
	txreg_t reg, pos, iostat;

	KASSERT(sc);
	DPRINTF(("%s: port #%d\n", __FUNCTION__, port));

	devname  =  sc->sc_dev.dv_xname;
	tc = sc->sc_tc;

	/* IO [0:6] */ 
	pos = 1 << port;
#ifdef DIAGNOSTIC
	if (!(sc->sc_stat_io.dir & pos))
		panic("%s: IO%d is not output port.\n", devname, port);
#endif
	reg = tx_conf_read(tc, TX39_IOCTRL_REG);
	iostat = TX391X_IOCTRL_IODOUT(reg);
	if (onoff)
		iostat |= pos;
	else
		iostat &= ~pos;
	TX391X_IOCTRL_IODOUT_CLR(reg);
	reg = TX391X_IOCTRL_IODOUT_SET(reg, iostat);
	tx_conf_write(tc, TX39_IOCTRL_REG, reg);
}

void
tx391x_io_intr_map(void *arg, int port, int *pedge, int *nedge)
{
	if (pedge)
		*pedge = MAKEINTR(5, (1 << (port + 7)));
	if (nedge)
		*nedge = MAKEINTR(5, (1 << port));
}

void
tx391x_io_update(void *arg)
{
	struct tx39io_softc *sc = arg;
	struct tx39io_port_status *stat_io = &sc->sc_stat_io;
	tx_chipset_tag_t tc = sc->sc_tc;
	txreg_t reg;

	/* IO [0:6] */
	sc->sc_ostat_io = *stat_io; /* save old status */
	reg = tx_conf_read(tc, TX39_IOCTRL_REG);
	stat_io->dir		= TX391X_IOCTRL_IODIREC(reg);
	stat_io->in		= TX391X_IOCTRL_IODIN(reg);
	stat_io->out		= TX391X_IOCTRL_IODOUT(reg); 
	stat_io->u.debounce	= TX391X_IOCTRL_IODEBSEL(reg);
	reg = tx_conf_read(tc, TX39_IOIOPOWERDWN_REG);
	stat_io->power = TX391X_IOIOPOWERDWN_IOPD(reg);
}
#endif /* TX391X */

#ifdef TX392X
/* 
 * TMPR3922 specific
 */
int
tx392x_io_in(void *arg, int port)
{
	struct tx39io_softc *sc __attribute__((__unused__)) = arg;
	txreg_t reg = tx_conf_read(sc->sc_tc, TX392X_IODATAINOUT_REG);
	DPRINTF(("%s: port #%d\n", __FUNCTION__, port));
	
	return TX392X_IODATAINOUT_DIN(reg) & (1 << port);
}

void
tx392x_io_out(void *arg, int port, int onoff)
{
	struct tx39io_softc *sc = arg;
#ifdef DIAGNOSTIC
	const char *devname =  sc->sc_dev.dv_xname;
#endif
	tx_chipset_tag_t tc = sc->sc_tc;
	txreg_t reg, pos, iostat;

	DPRINTF(("%s: port #%d\n", __FUNCTION__, port));
	/* IO [0:15] */
	pos = 1 << port;
#ifdef DIAGNOSTIC
	if (!(sc->sc_status.dir_io & pos))
		panic("%s: IO%d is not output port.\n", devname, port);
#endif
	reg = tx_conf_read(tc, TX392X_IODATAINOUT_REG);
	iostat = TX392X_IODATAINOUT_DOUT(reg);
	if (onoff)
		iostat |= pos;
	else
		iostat &= ~pos;
	TX392X_IODATAINOUT_DOUT_CLR(reg);
	reg = TX392X_IODATAINOUT_DOUT_SET(reg, iostat);
	tx_conf_write(tc, TX392X_IODATAINOUT_REG, reg);
}

void
tx392x_io_intr_map(void *arg, int port, int *pedge, int *nedge)
{
	if (pedge)
		*pedge = MAKEINTR(8, (1 << (port + 16)));
	if (nedge)
		*nedge = MAKEINTR(8, (1 << port));
}

void
tx392x_io_update(void *arg)
{
	struct tx39io_softc *sc = arg;
	struct tx39io_port_status *stat_io = &sc->sc_stat_io;
	tx_chipset_tag_t tc = sc->sc_tc;
	txreg_t reg;

	sc->sc_ostat_io = *stat_io; /* save old status */
	/* IO [0:15] */
	reg = tx_conf_read(tc, TX39_IOCTRL_REG);
	stat_io->dir		= TX392X_IOCTRL_IODIREC(reg);
	stat_io->u.debounce	= TX392X_IOCTRL_IODEBSEL(reg);
	reg = tx_conf_read(tc, TX392X_IODATAINOUT_REG);
	stat_io->in		= TX392X_IODATAINOUT_DIN(reg);
	stat_io->out		= TX392X_IODATAINOUT_DOUT(reg); 
	reg = tx_conf_read(tc, TX39_IOIOPOWERDWN_REG);
	stat_io->power = TX392X_IOIOPOWERDWN_IOPD(reg);
}

#endif /* TX392X */

static const char *line = "--------------------------------------------------"
"------------\n";
static void
mfio_dump(void *arg)
{
	struct tx39io_softc *sc = arg;
	const struct tx39io_mfio_map *map = tx39io_get_mfio_map(tc);
	struct tx39io_port_status *stat;
	int i;
	
	printf("%s", line);
	stat = &sc->sc_stat_mfio;
	for (i = TX39_IO_MFIO_MAX - 1; i >= 0 ; i--) {
		/* MFIO port has power down state */
		printf("MFIO %2d:     -       ", i);
		__print_port_status(stat, i);
		printf(ISSET(stat->u.select, i) ? "  MFIO(%s)\n" : "  %s\n", 
		       map[i].std_pin_name);
	}
	printf("%s", line);
}

static void
io_dump(void *arg)
{
	struct tx39io_softc *sc = arg;
	struct tx39io_port_status *stat;
	int i;	

	printf("%s	 Debounce Direction DataOut DataIn PowerDown Select"
	       "\n%s", line, line);
	stat = &sc->sc_stat_io;
	for (i = tx39io_get_io_max(tc) - 1; i >= 0 ; i--) {
		/* IO port has debouncer */
		printf("IO   %2d:    %s      ", i,
		       ISSET(stat->u.debounce, i) ? "On " : "Off");
		__print_port_status(stat, i);
		printf("    -\n");
	}
}

static void
__print_port_status(struct tx39io_port_status *stat, int i)
{
	printf("%s       %d       %d     %s",
	       ISSET(stat->dir, i) ? "Out" : "In ",
	       ISSET(stat->out, i) ? 1 : 0,
	       ISSET(stat->in, i) ? 1 : 0,
	       ISSET(stat->power, i) ? "Down  ": "Active");
}

