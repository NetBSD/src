/*	$NetBSD: tx39io.c,v 1.3 1999/12/23 16:58:07 uch Exp $ */

/*
 * Copyright (c) 1999, by UCHIYAMA Yasushi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#include "opt_tx39_debug.h"
#include "opt_tx39iodebug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/tx39ioreg.h>
#include <hpcmips/tx/tx39icureg.h>

#define TX39IO_ATTACH_DUMMYHANDLER 0
#undef TX39IO_MFIOOUTPORT_ON
#undef TX39IO_MFIOOUTPORT_OFF

int	tx39io_match __P((struct device*, struct cfdata*, void*));
void	tx39io_attach __P((struct device*, struct device*, void*));

struct tx39io_softc {
	struct	device sc_dev;
	tx_chipset_tag_t sc_tc;
};

struct cfattach tx39io_ca = {
	sizeof(struct tx39io_softc), tx39io_match, tx39io_attach
};

int	tx39io_intr __P((void*));
int	tx39mfio_intr __P((void*));

void	tx39io_dump_and_attach_handler __P((struct tx39io_softc*, int));
void	__dump_and_attach_handler __P((tx_chipset_tag_t, u_int32_t, u_int32_t, u_int32_t, u_int32_t, int, int, int (*) __P((void*)), void*));

#define ISSET(x, s)	((x) & (1 << (s)))
#define STD_IN		1
#define STD_OUT		2
#define STD_INOUT	3

const struct {
	char *std_pin_name;
	int  std_type;
} mfio_map[TX39_IO_MFIO_MAX] = {
  [31] = {"CHIFS",	STD_INOUT},
  [30] = {"CHICLK",	STD_INOUT},
  [29] = {"CHIDOUT",	STD_OUT},
  [28] = {"CHIDIN",	STD_IN},
  [27] = {"DREQ",	STD_IN},
  [26] = {"DGRINT",	STD_OUT},
  [25] = {"BC32K",	STD_OUT},
  [24] = {"TXD",	STD_OUT},
  [23] = {"RXD",	STD_IN},
  [22] = {"CS1",	STD_OUT},
  [21] = {"CS2",	STD_OUT},
  [20] = {"CS3",	STD_OUT},
  [19] = {"MCS0",	STD_OUT},
  [18] = {"MCS1",	STD_OUT},
#ifdef TX391X
  [17] = {"MCS2",	STD_OUT},
  [16] = {"MCS3",	STD_OUT},
#endif /* TX391X */
#ifdef TX392X
  [17] = {"RXPWR",	STD_OUT},
  [16] = {"IROUT",	STD_OUT},
#endif /* TX392X */
  [15] = {"SPICLK",	STD_OUT},
  [14] = {"SPIOUT",	STD_OUT},
  [13] = {"SPIN",	STD_IN},
  [12] = {"SIBMCLK",	STD_INOUT},
  [11] = {"CARDREG",	STD_OUT},
  [10] = {"CARDIOWR",	STD_OUT},
   [9] = {"CARDIORD",	STD_OUT},
   [8] = {"CARD1CSL",	STD_OUT},
   [7] = {"CARD1CSH",	STD_OUT},
   [6] = {"CARD2CSL",	STD_OUT},
   [5] = {"CARD2CSH",	STD_OUT},
   [4] = {"CARD1WAIT",	STD_IN},
   [3] = {"CARD2WAIT",	STD_IN},
   [2] = {"CARDDIR",	STD_OUT},
#ifdef TX391X
   [1] = {"MFIO[1]",	0},
   [0] = {"MFIO[0]",	0}
#endif /* TX391X */
#ifdef TX392X
   [1] = {"MCS1WAIT",	0},
   [0] = {"MCS0WAIT",	0}
#endif /* TX392X */
};

int
tx39io_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{

	return 1;
}

int
tx39io_intr(arg)
	void *arg;
{
#ifdef TX39_DEBUG
#if 0
	static int i;
	if (i ^= 1) {
		tx39debugflag = 1;
	} else {
		tx39debugflag = 0;
	}
#endif
	printf("io (%d:%d)\n", (tx39intrvec >> 16) & 0xffff, 
	       tx39intrvec & 0xfff);
#endif
	return 0;
}

int
tx39mfio_intr(arg)
	void *arg;
{
#ifdef TX39_DEBUG
#if 0
	struct tx39io_softc *sc = arg;
	tx39io_dump_and_attach_handler(sc, 0);
#endif
	printf("mfio (%d:%d)\n", (tx39intrvec >> 16) & 0xffff, 
	       tx39intrvec & 0xfff);
#endif
	return 0;
}

void
tx39io_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct txsim_attach_args *ta = aux;
	struct tx39io_softc *sc = (void*)self;
	tx_chipset_tag_t tc;
	tc = sc->sc_tc = ta->ta_tc;

	printf("\n");
#ifdef COMPAQ_LOCAL_INTR
	/* 2010c Rec button */
	tx_intr_establish(tc, MAKEINTR(5, (1<<6)), IST_EDGE, IPL_CLOCK, tx39io_intr, sc);
	/* Play button */
	tx_intr_establish(tc, MAKEINTR(5, (1<<5)), IST_EDGE, IPL_CLOCK, tx39io_intr, sc);
	/* It seems that these interrupt arise when serial session start */
	tx_intr_establish(tc, MAKEINTR(3, (1<<30)), IST_EDGE, IPL_CLOCK, tx39mfio_intr, sc);
	tx_intr_establish(tc, MAKEINTR(3, (1<<5)), IST_EDGE, IPL_CLOCK, tx39mfio_intr, sc);
	tx_intr_establish(tc, MAKEINTR(4, (1<<30)), IST_EDGE, IPL_CLOCK, tx39mfio_intr, sc);
	tx_intr_establish(tc, MAKEINTR(4, (1<<5)), IST_EDGE, IPL_CLOCK, tx39mfio_intr, sc);
#endif

#ifdef VICTOR_INTERLINK_INTR
	/* open panel */
	tx_intr_establish(tc, MAKEINTR(8, (1<<20)), IST_EDGE, IPL_CLOCK, tx39io_intr, sc);	
	/* close panel */
	tx_intr_establish(tc, MAKEINTR(8, (1<<4)), IST_EDGE, IPL_CLOCK, tx39io_intr, sc);	
	/* serial session */
	tx_intr_establish(tc, MAKEINTR(4, (1<<29)), IST_EDGE, IPL_CLOCK, tx39mfio_intr, sc);	
	tx_intr_establish(tc, MAKEINTR(4, (1<<30)), IST_EDGE, IPL_CLOCK, tx39mfio_intr, sc);	
	/* REC button */
	tx_intr_establish(tc, MAKEINTR(8, (1<<7)), IST_EDGE, IPL_CLOCK, tx39io_intr, sc);
	/* kbd */
	tx_intr_establish(tc, MAKEINTR(3, (1<<7)), IST_EDGE, IPL_CLOCK, tx39mfio_intr, sc);
#endif

#ifdef TX39IODEBUG
	tx39io_dump_and_attach_handler(sc, TX39IO_ATTACH_DUMMYHANDLER);
#endif /* TX39IODEBUG */
}

#ifdef TX39IODEBUG
void
tx39io_dump_and_attach_handler(sc, dummy)
	struct tx39io_softc *sc;
	int dummy;
{
	tx_chipset_tag_t tc;
	u_int32_t reg, reg_out, reg_dir, reg_in, reg_sel, reg_pwr, reg_deb;
	int i;
	int (*iointr) __P((void*));
	int (*mfiointr) __P((void*));
	
	tc = sc->sc_tc;
	if (dummy) {
		iointr =  tx39io_intr;
		mfiointr = tx39mfio_intr;
	} else {
		iointr = mfiointr = 0;
	}
	
	printf("--------------------------------------------------------------\n");
	printf("	 Debounce Direction DataOut DataIn PowerDown Select\n");
	printf("--------------------------------------------------------------\n");
	/* IO */
	reg = tx_conf_read(tc, TX39_IOCTRL_REG);
	reg_deb = TX39_IOCTRL_IODEBSEL(reg);
	reg_dir = TX39_IOCTRL_IODIREC(reg);
#ifdef TX391X
	reg_out = TX39_IOCTRL_IODOUT(reg);
	reg_in  = TX39_IOCTRL_IODIN(reg);
#endif /* TX391X */
#ifdef TX392X
	reg = tx_conf_read(tc, TX39_IODATAINOUT_REG);
	reg_out = TX39_IODATAINOUT_DOUT(reg);
	reg_in  = TX39_IODATAINOUT_DIN(reg);
#endif /* TX392X */
	reg = tx_conf_read(tc, TX39_IOIOPOWERDWN_REG);
	reg_pwr = TX39_IOIOPOWERDWN_IOPD(reg);
	for (i = TX39_IO_IO_MAX - 1; i >= 0 ; i--) {
		printf("IO   %2d:    ", i);
		printf("%s", ISSET(reg_dir, i) ? "On " : "Off");
		printf("      ");
		__dump_and_attach_handler(tc, reg_dir, reg_out, reg_in, 
					  reg_pwr, i, 1, iointr, sc);
		
		printf("    -");
		printf("\n");
	}
	/* MFIO */
	printf("--------------------------------------------------------------\n");
	reg_out = tx_conf_read(tc, TX39_IOMFIODATAOUT_REG);
	reg_dir = tx_conf_read(tc, TX39_IOMFIODATADIR_REG);
	reg_in  = tx_conf_read(tc, TX39_IOMFIODATAIN_REG);
	reg_sel = tx_conf_read(tc, TX39_IOMFIODATASEL_REG);
	reg_pwr = tx_conf_read(tc, TX39_IOMFIOPOWERDWN_REG);
	for (i = TX39_IO_MFIO_MAX - 1; i >= 0 ; i--) {
		printf("MFIO %2d:     -       ", i);
		__dump_and_attach_handler(tc, reg_dir, reg_out, reg_in, 
					  reg_pwr, i, 0, mfiointr, sc);
		printf("  ");
		printf(ISSET(reg_sel, i) ? "MFIO(%s)" : "%s", 
		       mfio_map[i].std_pin_name);
		printf("\n");
	}
	printf("--------------------------------------------------------------\n");
}

void	
__dump_and_attach_handler(tc, reg_dir, reg_out, reg_in, reg_pwr, 
			  i, io, func, arg)
	tx_chipset_tag_t tc;
	u_int32_t reg_dir, reg_out, reg_in, reg_pwr;
	int i, io;
	int (*func) __P((void*));
	void *arg;
{
	int pset, nset, pofs, nofs;

	if (io) {
#ifdef TX391X
		pset = nset = 5;
		pofs = i + 7;
		nofs = i;
#endif
#ifdef TX392X
		pset = nset = 8;
		pofs = i + 16;
		nofs = i;
#endif
	} else {
		pset = 3;
		nset = 4;
		pofs = nofs = i;
	}
	
	if (ISSET(reg_dir, i)) {
#if defined TX39IO_MFIOOUTPORT_ON || defined TX39IO_MFIOOUTPORT_OFF
		txreg_t reg;
		if (io) {
#ifdef TX392X			
			reg = tx_conf_read(tc, TX39_IODATAINOUT_REG);
#ifdef TX39IO_MFIOOUTPORT_ON
			reg |= (1 << (i + 16));
#else
			reg &= ~(1 << (i + 16));
#endif
			tx_conf_write(tc, TX39_IODATAINOUT_REG, reg);
#endif /* TX392X */
		} else {
			reg = tx_conf_read(tc, TX39_IOMFIODATAOUT_REG);
#ifdef TX39IO_MFIOOUTPORT_ON
			reg |= (1 << i);
#else
			reg &= ~(1 << i);
#endif
			tx_conf_write(tc, TX39_IOMFIODATAOUT_REG, reg);
		}
#endif /* TX39IO_MFIOOUTPORT_ON */
		printf("Out");
	} else {
		printf("In ");
		if (func) {
			/* Positive Edge */
			tx_intr_establish(
				tc, MAKEINTR(pset, (1 << pofs)), 
				IST_EDGE, IPL_TTY, func, arg);
			/* Negative Edge */	
			tx_intr_establish(
				tc, MAKEINTR(nset, (1 << nofs)), 
				IST_EDGE, IPL_TTY, func, arg);
		}
	}
	printf("       ");
	printf("%d", ISSET(reg_out, i) ? 1 : 0);
	printf("       ");
	printf("%d", ISSET(reg_in, i) ? 1 : 0);
	printf("     ");
	printf("%s", ISSET(reg_pwr, i) ? "Down  ": "Active");
};

#endif /* TX39IODEBUG */
