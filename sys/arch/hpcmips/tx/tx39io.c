/*	$NetBSD: tx39io.c,v 1.9.8.2 2002/02/28 04:10:02 nathanw Exp $ */

/*-
 * Copyright (c) 1999-2001 The NetBSD Foundation, Inc.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/tx39icureg.h>
#define __TX39IO_PRIVATE
#include <hpcmips/tx/tx39iovar.h>
#include <hpcmips/tx/tx39ioreg.h>

#ifdef	TX39IO_DEBUG
#define DPRINTF_ENABLE
#define DPRINTF_DEBUG	tx39io_debug
#endif
#include <machine/debug.h>

#define ISSET(x, s)	((x) & (1 << (s)))

int	tx39io_match(struct device *, struct cfdata *, void *);
void	tx39io_attach(struct device *, struct device *, void *);

struct cfattach tx39io_ca = {
	sizeof(struct tx39io_softc), tx39io_match, tx39io_attach
};

/* IO/MFIO common */
static void port_intr_disestablish(hpcio_chip_t, hpcio_intr_handle_t);
static void port_intr_clear(hpcio_chip_t, hpcio_intr_handle_t);
/* MFIO */
static void *mfio_intr_establish(hpcio_chip_t, int, int, int (*)(void *),
    void *);
static int mfio_in(hpcio_chip_t, int);
static void mfio_out(hpcio_chip_t, int, int);
static int mfio_intr_map(int *, int, int);
static void mfio_dump(hpcio_chip_t);
static void mfio_update(hpcio_chip_t);
/* IO */
static void *io_intr_establish(hpcio_chip_t, int, int, int (*)(void *),
    void *);
#ifdef TX391X
static int tx391x_io_in(hpcio_chip_t, int);
static void tx391x_io_out(hpcio_chip_t, int, int);
static void tx391x_io_update(hpcio_chip_t);
static int tx391x_io_intr_map(int *, int, int);
#endif
#ifdef TX392X
static int tx392x_io_in(hpcio_chip_t, int);
static void tx392x_io_out(hpcio_chip_t, int, int);
static void tx392x_io_update(hpcio_chip_t);
static int tx392x_io_intr_map(int *, int, int);
#endif
#if defined TX391X && defined TX392X
#define tx39_io_intr_map(t, s, p, m)					\
	(IS_TX391X(t)
	    ? tx391x_io_intr_map(s, p, m) : tx392x_io_intr_map(s, p, m))
#elif defined TX391X
#define tx39io_intr_map(t, s, p, m)	tx391x_io_intr_map(s, p, m)
#elif defined TX392X
#define tx39io_intr_map(t, s, p, m)	tx392x_io_intr_map(s, p, m)
#endif
static void io_dump(hpcio_chip_t);

static void __print_port_status(struct tx39io_port_status *, int);

int
tx39io_match(struct device *parent, struct cfdata *cf, void *aux)
{
	return (ATTACH_FIRST); /* 1st attach group of txsim */
}

void
tx39io_attach(struct device *parent, struct device *self, void *aux)
{
	struct txsim_attach_args *ta = aux;
	struct tx39io_softc *sc = (void *)self;
	tx_chipset_tag_t tc;
	struct hpcio_chip *io_hc = &sc->sc_io_ops;
	struct hpcio_chip *mfio_hc = &sc->sc_mfio_ops;
	
	tc = sc->sc_tc = ta->ta_tc;

	printf("\n");
	sc->sc_stat_io_mask = ~(1 << 5); /* exclude Plum2 INT */
	sc->sc_stat_mfio_mask = ~(0x3|(0x3 << 23));

	/* IO */
	io_hc->hc_chipid		= IO;
	io_hc->hc_name			= "IO";
	io_hc->hc_sc			= sc;
	io_hc->hc_intr_establish	= io_intr_establish;
	io_hc->hc_intr_disestablish	= port_intr_disestablish;
	io_hc->hc_intr_clear		= port_intr_clear;
	io_hc->hc_dump			= io_dump;
	if (IS_TX391X(tc)) {
#ifdef TX391X
		io_hc->hc_portread	= tx391x_io_in;
		io_hc->hc_portwrite	= tx391x_io_out;
		io_hc->hc_update	= tx391x_io_update;
#endif
	} else if (IS_TX392X(tc)) {
#ifdef TX392X
		io_hc->hc_portread	= tx392x_io_in;
		io_hc->hc_portwrite	= tx392x_io_out;
		io_hc->hc_update	= tx392x_io_update;
#endif
	}
	tx_conf_register_ioman(tc, io_hc);

	/* MFIO */
	mfio_hc->hc_chipid		= MFIO;
	mfio_hc->hc_name		= "MFIO";
	mfio_hc->hc_sc			= sc;
	mfio_hc->hc_portread		= mfio_in;
	mfio_hc->hc_portwrite		= mfio_out;
	mfio_hc->hc_intr_establish	= mfio_intr_establish;
	mfio_hc->hc_intr_disestablish	= port_intr_disestablish;
	mfio_hc->hc_update		= mfio_update;
	mfio_hc->hc_dump		= mfio_dump;

	tx_conf_register_ioman(tc, mfio_hc);

	hpcio_update(io_hc);
	hpcio_update(mfio_hc);

#ifdef TX39IO_DEBUG
	hpcio_dump(io_hc);
	hpcio_dump(mfio_hc);
	printf("IO i0x%08x o0x%08x MFIO i0x%08x o0x%08x\n",
	       sc->sc_stat_io.in, sc->sc_stat_io.out,
	       sc->sc_stat_mfio.in, sc->sc_stat_mfio.out);
#endif /* TX39IO_DEBUG */
}

/* 
 * TX391X, TX392X common 
 */
static void *
io_intr_establish(hpcio_chip_t arg, int port, int mode, int (*func)(void *),
    void *func_arg)
{
	struct tx39io_softc *sc = arg->hc_sc;
	int src;

	if (tx39io_intr_map(sc->sc_tc, &src, port, mode) != 0)
		return (0);

	return (tx_intr_establish(sc->sc_tc, src, IST_EDGE, IPL_CLOCK, func,
	    func_arg));
}

static void *
mfio_intr_establish(hpcio_chip_t arg, int port, int mode, int (*func)(void *),
    void *func_arg)
{
	struct tx39io_softc *sc = arg->hc_sc;
	int src;

	if (mfio_intr_map(&src, port, mode) != 0)
		return (0);
		
	return (tx_intr_establish(sc->sc_tc, src, IST_EDGE, IPL_CLOCK, func,
	    func_arg));
}

static void
port_intr_disestablish(hpcio_chip_t arg, void *ih)
{
	struct tx39io_softc *sc = arg->hc_sc;
	tx_intr_disestablish(sc->sc_tc, ih);
}

static void
port_intr_clear(hpcio_chip_t arg, void *ih)
{
}

static void
mfio_out(hpcio_chip_t arg, int port, int onoff)
{
	struct tx39io_softc *sc = arg->hc_sc;
	tx_chipset_tag_t tc;
	txreg_t reg, pos;

	DPRINTF("port #%d\n", port);
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
mfio_in(hpcio_chip_t arg, int port)
{
	struct tx39io_softc *sc __attribute__((__unused__)) = arg->hc_sc ;

	DPRINTF("port #%d\n", port);
	return (tx_conf_read(sc->sc_tc, TX39_IOMFIODATAIN_REG) & (1 << port));
}

static int
mfio_intr_map(int *src, int port, int mode)
{

	if (mode & HPCIO_INTR_POSEDGE) {
		*src = MAKEINTR(3, (1 << port));
		return (0);
	} else if (mode & HPCIO_INTR_NEGEDGE) {
		*src = MAKEINTR(4, (1 << port));
		return (0);
	}
		
	DPRINTF("invalid interrupt mode.\n");

	return (1);
}

static void
mfio_update(hpcio_chip_t arg)
{
	struct tx39io_softc *sc = arg->hc_sc;
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
tx391x_io_in(hpcio_chip_t arg, int port)
{
	struct tx39io_softc *sc __attribute__((__unused__)) = arg->hc_sc;
	txreg_t reg = tx_conf_read(sc->sc_tc, TX39_IOCTRL_REG);

	DPRINTF("port #%d\n", port);
	return  (TX391X_IOCTRL_IODIN(reg) & (1 << port));
}

void
tx391x_io_out(hpcio_chip_t arg, int port, int onoff)
{
	struct tx39io_softc *sc = arg->hc_sc;
	tx_chipset_tag_t tc;
	txreg_t reg, pos, iostat;

	KASSERT(sc);
	DPRINTF("port #%d\n", port);

	tc = sc->sc_tc;

	/* IO [0:6] */ 
	pos = 1 << port;
#ifdef DIAGNOSTIC
	if (!(sc->sc_stat_io.dir & pos))
		panic("%s: IO%d is not output port.\n", sc->sc_dev.dv_xname,
		      port);
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
tx391x_io_update(hpcio_chip_t arg)
{
	struct tx39io_softc *sc = arg->hc_sc;
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

int
tx391x_io_intr_map(int *src, int port, int mode)
{

	if (mode & HPCIO_INTR_POSEDGE) {
		*src = MAKEINTR(5, (1 << (port + 7)));
		return (0);
	} else if (mode & HPCIO_INTR_NEGEDGE) {
		*src = MAKEINTR(5, (1 << port));
		return (0);
	}
		
	DPRINTF("invalid interrupt mode.\n");

	return (1);
}
#endif /* TX391X */

#ifdef TX392X
/* 
 * TMPR3922 specific
 */
int
tx392x_io_in(hpcio_chip_t arg, int port)
{
	struct tx39io_softc *sc __attribute__((__unused__)) = arg->hc_sc;
	txreg_t reg = tx_conf_read(sc->sc_tc, TX392X_IODATAINOUT_REG);

	DPRINTF("port #%d\n", port);
	
	return (TX392X_IODATAINOUT_DIN(reg) & (1 << port));
}

void
tx392x_io_out(hpcio_chip_t arg, int port, int onoff)
{
	struct tx39io_softc *sc = arg->hc_sc;
#ifdef DIAGNOSTIC
	const char *devname =  sc->sc_dev.dv_xname;
#endif
	tx_chipset_tag_t tc = sc->sc_tc;
	txreg_t reg, pos, iostat;

	DPRINTF("port #%d\n", port);
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

int
tx392x_io_intr_map(int *src, int port, int mode)
{

	if (mode & HPCIO_INTR_POSEDGE) {
		*src = MAKEINTR(8, (1 << (port + 16)));
		return (0);
	} else if (mode & HPCIO_INTR_NEGEDGE) {
		*src = MAKEINTR(8, (1 << port));
		return (0);
	}
		
	DPRINTF("invalid interrupt mode.\n");

	return (1);
}

void
tx392x_io_update(hpcio_chip_t arg)
{
	struct tx39io_softc *sc = arg->hc_sc;
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
mfio_dump(hpcio_chip_t arg)
{
	struct tx39io_softc *sc = arg->hc_sc;
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
io_dump(hpcio_chip_t arg)
{
	struct tx39io_softc *sc = arg->hc_sc;
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
