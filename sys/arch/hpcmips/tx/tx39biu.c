/*	$NetBSD: tx39biu.c,v 1.3.2.2 1999/12/27 18:32:11 wrstuden Exp $ */

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
#include "opt_tx39_watchdogtimer.h"
#include "opt_tx39biudebug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/tx39biureg.h>

#include <hpcmips/tx/txcsbusvar.h>

#define ISSET(x, s)	((x) & (1 << (s)))
#define ISSETPRINT(r, s, m) __is_set_print((u_int32_t)(r), \
	TX39_MEMCONFIG##s##_##m, #m)

int	tx39biu_match __P((struct device*, struct cfdata*, void*));
void	tx39biu_attach __P((struct device*, struct device*, void*));
void	tx39biu_callback __P((struct device*));
int	tx39biu_print __P((void*, const char*));
int	tx39biu_intr __P((void*));

static void *__sc; /* XXX */

void	tx39biu_dump __P((tx_chipset_tag_t));

struct tx39biu_softc {
	struct	device sc_dev;
	tx_chipset_tag_t sc_tc;
};

struct cfattach tx39biu_ca = {
	sizeof(struct tx39biu_softc), tx39biu_match, tx39biu_attach
};

int
tx39biu_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return 1;
}

void
tx39biu_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct txsim_attach_args *ta = aux;
	struct tx39biu_softc *sc = (void*)self;
	tx_chipset_tag_t tc;
#ifdef TX39_WATCHDOGTIMER
	txreg_t reg;
#endif

	sc->sc_tc = tc = ta->ta_tc;
	printf("\n");
#ifdef TX39BIUDEBUG
	tx39biu_dump(tc);
#endif

#ifdef TX39_WATCHDOGTIMER
	/*
	 * CLRWRBUSERRINT Bus error connected CPU HwInt0
	 */
	reg = tx_conf_read(tc, TX39_MEMCONFIG4_REG);
	reg |= TX39_MEMCONFIG4_ENWATCH;
	reg = TX39_MEMCONFIG4_WATCHTIMEVAL_SET(reg, 0xf);
	tx_conf_write(tc, TX39_MEMCONFIG4_REG, reg);
	
	reg = tx_conf_read(tc, TX39_MEMCONFIG4_REG);
	if (reg & TX39_MEMCONFIG4_ENWATCH) {
		int i;
		i = TX39_MEMCONFIG4_WATCHTIMEVAL(reg);
		i = (1000 * (i + 1) * 64) / 36864;
		printf("WatchDogTimerRate: %dus\n", i);
	}
#endif
	__sc = sc;

	/*	Clear watch dog timer interrupt */
	tx39biu_intr(sc);

	/* 
	 *	Chip select virtual bridge
	 */
	config_defer(self, tx39biu_callback);
}

void
tx39biu_callback(self)
	struct device *self;
{
	struct tx39biu_softc *sc = (void*)self;
	struct csbus_attach_args cba;

	cba.cba_busname = "txcsbus";
	cba.cba_tc = sc->sc_tc;
	config_found(self, &cba, tx39biu_print);
}

int
tx39biu_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	return pnp ? QUIET : UNCONF;
}

int
tx39biu_intr(arg)
	void *arg;
{
	struct tx39biu_softc *sc = __sc;
	tx_chipset_tag_t tc;
	txreg_t reg;

	if (!sc) {
		return 0;
	}
	tc = sc->sc_tc;
	/* Clear interrupt */
	reg = tx_conf_read(tc, TX39_MEMCONFIG4_REG);
	reg |= TX39_MEMCONFIG4_CLRWRBUSERRINT;
	tx_conf_write(tc, TX39_MEMCONFIG4_REG, reg);
	reg = tx_conf_read(tc, TX39_MEMCONFIG4_REG);
	reg &= ~TX39_MEMCONFIG4_CLRWRBUSERRINT;
	tx_conf_write(tc, TX39_MEMCONFIG4_REG, reg);

	return 0;
}

void
tx39biu_dump(tc)
	tx_chipset_tag_t tc;	
{
	char *rowsel[] = {"18,17:9", "22,18,20,19,17:9", "20,22,21,19,17:9", "22,23,21,19,17:9"};
	char *colsel[] = {"22,20,18,8:1", "19,18,8:2", "21,20,18,8:2", "23,22,20,18,8:2",
			 "24,22,20,18,8:2", "18,p,X,8:0","22,p,X,21,8:0", "18,p,X,21,8:1",
			 "22,p,X,23,21,8:1", "24,23,21,8:2"};
	txreg_t reg;
	int i;
	/* 
	 *	Memory config 0 register
	 */
	reg = tx_conf_read(tc, TX39_MEMCONFIG0_REG);
	printf(" config0:");
	ISSETPRINT(reg, 0, ENDCLKOUTTRI);
	ISSETPRINT(reg, 0, DISDQMINIT);
	ISSETPRINT(reg, 0, ENSDRAMPD);
	ISSETPRINT(reg, 0, SHOWDINO);
	ISSETPRINT(reg, 0, ENRMAP2);
	ISSETPRINT(reg, 0, ENRMAP1);
	ISSETPRINT(reg, 0, ENWRINPAGE);
	ISSETPRINT(reg, 0, ENCS3USER);
	ISSETPRINT(reg, 0, ENCS2USER);
	ISSETPRINT(reg, 0, ENCS1USER);
	ISSETPRINT(reg, 0, ENCS1DRAM);
	ISSETPRINT(reg, 0, CS3SIZE);
	ISSETPRINT(reg, 0, CS2SIZE);
	ISSETPRINT(reg, 0, CS1SIZE);
	ISSETPRINT(reg, 0, CS0SIZE);
	printf("\n");
	for (i = 0; i < 2; i++) {
		int r, c;
		printf(" BANK%d: ", i);
		switch (i ? TX39_MEMCONFIG0_BANK1CONF(reg)
			: TX39_MEMCONFIG0_BANK0CONF(reg)) {
		case TX39_MEMCONFIG0_BANKCONF_16BITSDRAM:
			printf("16bit SDRAM");
			break;
		case TX39_MEMCONFIG0_BANKCONF_8BITSDRAM:
			printf("8bit SDRAM");
			break;
		case TX39_MEMCONFIG0_BANKCONF_32BITSDHDRAM:
			printf("32bit DRAM/HDRAM");
			break;
		case TX39_MEMCONFIG0_BANKCONF_16BITSDHDRAM:
			printf("16bit DRAM/HDRAM");
			break;
		}
		if (i == 1) {
			r = TX39_MEMCONFIG0_ROWSEL1(reg);
			c = TX39_MEMCONFIG0_COLSEL1(reg);
		} else {
			r = TX39_MEMCONFIG0_ROWSEL0(reg);
			c = TX39_MEMCONFIG0_COLSEL0(reg);
		}
		printf(" ROW %s COL %s\n", rowsel[r], colsel[c]);
	}

	/* 
	 *	Memory config 3 register
	 */
	printf(" config3:");
	reg = tx_conf_read(tc, TX39_MEMCONFIG3_REG);
#ifdef TX391X
	ISSETPRINT(reg, 3, ENMCS3PAGE);
	ISSETPRINT(reg, 3, ENMCS2PAGE);    
	ISSETPRINT(reg, 3, ENMCS1PAGE);    
	ISSETPRINT(reg, 3, ENMCS0PAGE);    
#endif /* TX391X */
	ISSETPRINT(reg, 3, ENCS3PAGE);    
	ISSETPRINT(reg, 3, ENCS2PAGE);     
	ISSETPRINT(reg, 3, ENCS1PAGE);     
	ISSETPRINT(reg, 3, ENCS0PAGE);     
	ISSETPRINT(reg, 3, CARD2WAITEN);     
	ISSETPRINT(reg, 3, CARD1WAITEN);   
	ISSETPRINT(reg, 3, CARD2IOEN);   
	ISSETPRINT(reg, 3, CARD1IOEN);     
#ifdef TX391X
	ISSETPRINT(reg, 3, PORT8SEL);     
#endif /* TX391X */
#ifdef TX392X 
	ISSETPRINT(reg, 3, CARD2_8SEL);
	ISSETPRINT(reg, 3, CARD1_8SEL);
#endif /* TX392X */

	printf("\n");

	/* 
	 *	Memory config 4 register
	 */
	reg = tx_conf_read(tc, TX39_MEMCONFIG4_REG);
	printf(" config4:");
	ISSETPRINT(reg, 4, ENBANK1HDRAM);
	ISSETPRINT(reg, 4, ENBANK0HDRAM);  
	ISSETPRINT(reg, 4, ENARB);  
	ISSETPRINT(reg, 4, DISSNOOP);
	ISSETPRINT(reg, 4, CLRWRBUSERRINT);
	ISSETPRINT(reg, 4, ENBANK1OPT);
	ISSETPRINT(reg, 4, ENBANK0OPT);    
	ISSETPRINT(reg, 4, ENWATCH);    
	ISSETPRINT(reg, 4, MEMPOWERDOWN);
	ISSETPRINT(reg, 4, ENRFSH1);  
	ISSETPRINT(reg, 4, ENRFSH0);       
	if (reg & TX39_MEMCONFIG4_ENWATCH) {
		i = TX39_MEMCONFIG4_WATCHTIMEVAL(reg);
		i = (1000 * (i + 1) * 64) / 36864;
		printf("WatchDogTimerRate: %dus", i);
	}
	printf("\n");
}

