/*	$NetBSD: esp.c,v 1.59 2009/11/23 00:11:45 rmind Exp $	*/

/*-
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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
 * Copyright (c) 1994 Peter Galbavy
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Peter Galbavy
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Based on aic6360 by Jarle Greipsland
 *
 * Acknowledgements: Many of the algorithms used in this driver are
 * inspired by the work of Julian Elischer (julian@tfs.com) and
 * Charles Hannum (mycroft@duality.gnu.ai.mit.edu).  Thanks a million!
 */

/*
 * Grabbed from the sparc port at revision 1.73 for the NeXT.
 * Darrin B. Jewell <dbj@NetBSD.org>  Sat Jul  4 15:41:32 1998
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: esp.c,v 1.59 2009/11/23 00:11:45 rmind Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/queue.h>

#include <uvm/uvm_extern.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#include <dev/scsipi/scsi_message.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/cpu.h>

#include <dev/ic/ncr53c9xreg.h>
#include <dev/ic/ncr53c9xvar.h>

#include <next68k/next68k/isr.h>

#include <next68k/dev/intiovar.h>
#include <next68k/dev/nextdmareg.h>
#include <next68k/dev/nextdmavar.h>

#include <next68k/dev/espreg.h>
#include <next68k/dev/espvar.h>

#ifdef DEBUG
#undef ESP_DEBUG
#endif

#ifdef ESP_DEBUG
int esp_debug = 0;
#define DPRINTF(x) if (esp_debug) printf x;
extern char *ndtracep;
extern char ndtrace[];
extern int ndtraceshow;
#define NDTRACEIF(x) if (10 && ndtracep < (ndtrace + 8192)) do {x;} while (0)
#else
#define DPRINTF(x)
#define NDTRACEIF(x)
#endif
#define PRINTF(x) printf x;


int	espmatch_intio(device_t, cfdata_t, void *);
void	espattach_intio(device_t, device_t, void *);

/* DMA callbacks */
bus_dmamap_t esp_dmacb_continue(void *);
void esp_dmacb_completed(bus_dmamap_t, void *);
void esp_dmacb_shutdown(void *);

static void	findchannel_defer(struct device *);

#ifdef ESP_DEBUG
char esp_dma_dump[5*1024] = "";
struct ncr53c9x_softc *esp_debug_sc = 0;
void esp_dma_store(struct ncr53c9x_softc *);
void esp_dma_print(struct ncr53c9x_softc *);
int esp_dma_nest = 0;
#endif


/* Linkup to the rest of the kernel */
CFATTACH_DECL_NEW(esp, sizeof(struct esp_softc),
    espmatch_intio, espattach_intio, NULL, NULL);

static int attached = 0;

/*
 * Functions and the switch for the MI code.
 */
uint8_t	esp_read_reg(struct ncr53c9x_softc *, int);
void	esp_write_reg(struct ncr53c9x_softc *, int, uint8_t);
int	esp_dma_isintr(struct ncr53c9x_softc *);
void	esp_dma_reset(struct ncr53c9x_softc *);
int	esp_dma_intr(struct ncr53c9x_softc *);
int	esp_dma_setup(struct ncr53c9x_softc *, uint8_t **, size_t *, int,
	    size_t *);
void	esp_dma_go(struct ncr53c9x_softc *);
void	esp_dma_stop(struct ncr53c9x_softc *);
int	esp_dma_isactive(struct ncr53c9x_softc *);

struct ncr53c9x_glue esp_glue = {
	esp_read_reg,
	esp_write_reg,
	esp_dma_isintr,
	esp_dma_reset,
	esp_dma_intr,
	esp_dma_setup,
	esp_dma_go,
	esp_dma_stop,
	esp_dma_isactive,
	NULL,			/* gl_clear_latched_intr */
};

#ifdef ESP_DEBUG
#define XCHR(x) hexdigits[(x) & 0xf]
static void
esp_hex_dump(unsigned char *pkt, size_t len)
{
	size_t i, j;

	printf("00000000  ");
	for(i = 0; i < len; i++) {
		printf("%c%c ", XCHR(pkt[i]>>4), XCHR(pkt[i]));
		if ((i + 1) % 16 == 8) {
			printf(" ");
		}
		if ((i + 1) % 16 == 0) {
			printf(" %c", '|');
			for(j = 0; j < 16; j++) {
				printf("%c", pkt[i-15+j]>=32 && pkt[i-15+j]<127?pkt[i-15+j]:'.');
			}
			printf("%c\n%c%c%c%c%c%c%c%c  ", '|', 
					XCHR((i+1)>>28),XCHR((i+1)>>24),XCHR((i+1)>>20),XCHR((i+1)>>16),
					XCHR((i+1)>>12), XCHR((i+1)>>8), XCHR((i+1)>>4), XCHR(i+1));
		}
	}
	printf("\n");
}
#endif

int
espmatch_intio(device_t parent, cfdata_t cf, void *aux)
{
	struct intio_attach_args *ia = aux;

	if (attached)
		return 0;

	ia->ia_addr = (void *)NEXT_P_SCSI;

	return 1;
}

static void
findchannel_defer(struct device *self)
{
	struct esp_softc *esc = device_private(self);
	struct ncr53c9x_softc *sc = &esc->sc_ncr53c9x;
	int error;

	if (!esc->sc_dma) {
		aprint_normal("%s", device_xname(sc->sc_dev));
		esc->sc_dma = nextdma_findchannel("scsi");
		if (!esc->sc_dma)
			panic("%s: can't find DMA channel",
			       device_xname(sc->sc_dev));
	}

	nextdma_setconf(esc->sc_dma, shutdown_cb, &esp_dmacb_shutdown);
	nextdma_setconf(esc->sc_dma, continue_cb, &esp_dmacb_continue);
	nextdma_setconf(esc->sc_dma, completed_cb, &esp_dmacb_completed);
	nextdma_setconf(esc->sc_dma, cb_arg, sc);

	error = bus_dmamap_create(esc->sc_dma->sc_dmat,
				  sc->sc_maxxfer,
				  sc->sc_maxxfer / PAGE_SIZE + 1,
				  sc->sc_maxxfer,
				  0, BUS_DMA_ALLOCNOW, &esc->sc_main_dmamap);
	if (error) {
		panic("%s: can't create main i/o DMA map, error = %d",
		      device_xname(sc->sc_dev), error);
	}

	error = bus_dmamap_create(esc->sc_dma->sc_dmat,
				  ESP_DMA_TAILBUFSIZE, 1, ESP_DMA_TAILBUFSIZE,
				  0, BUS_DMA_ALLOCNOW, &esc->sc_tail_dmamap);
	if (error) {
		panic("%s: can't create tail i/o DMA map, error = %d",
		      device_xname(sc->sc_dev), error);
	}
	
#if 0
	/* Turn on target selection using the `DMA' method */
	sc->sc_features |= NCR_F_DMASELECT;
#endif

	/* Do the common parts of attachment. */
	sc->sc_adapter.adapt_minphys = minphys;
	sc->sc_adapter.adapt_request = ncr53c9x_scsipi_request;
	ncr53c9x_attach(sc);

	/* Establish interrupt channel */
	isrlink_autovec(ncr53c9x_intr, sc, NEXT_I_IPL(NEXT_I_SCSI), 0, NULL);
	INTR_ENABLE(NEXT_I_SCSI);

	/* register interrupt stats */
	evcnt_attach_dynamic(&sc->sc_intrcnt, EVCNT_TYPE_INTR, NULL,
			     device_xname(sc->sc_dev), "intr");

	aprint_normal_dev(sc->sc_dev, "using DMA channel %s\n",
	    device_xname(&esc->sc_dma->sc_dev));
}

void
espattach_intio(device_t parent, device_t self, void *aux)
{
	struct esp_softc *esc = device_private(self);
	struct ncr53c9x_softc *sc = &esc->sc_ncr53c9x;
	struct intio_attach_args *ia = aux;

	sc->sc_dev = self;

#ifdef ESP_DEBUG
	esp_debug_sc = sc;
#endif

	esc->sc_bst = ia->ia_bst;
	if (bus_space_map(esc->sc_bst, NEXT_P_SCSI, 
			ESP_DEVICE_SIZE, 0, &esc->sc_bsh)) {
		aprint_normal("\n");
		panic("%s: can't map ncr53c90 registers",
		      device_xname(self));
	}

	sc->sc_id = 7;
	sc->sc_freq = 20;	/* MHz */

	/*
	 * Set up glue for MI code early; we use some of it here.
	 */
	sc->sc_glue = &esp_glue;

	/*
	 * XXX More of this should be in ncr53c9x_attach(), but
	 * XXX should we really poke around the chip that much in
	 * XXX the MI code?  Think about this more...
	 */

	/*
	 * It is necessary to try to load the 2nd config register here,
	 * to find out what rev the esp chip is, else the ncr53c9x_reset
	 * will not set up the defaults correctly.
	 */
	sc->sc_cfg1 = sc->sc_id | NCRCFG1_PARENB;
	sc->sc_cfg2 = NCRCFG2_SCSI2 | NCRCFG2_RPE;
	sc->sc_cfg3 = NCRCFG3_CDB;
	NCR_WRITE_REG(sc, NCR_CFG2, sc->sc_cfg2);

	if ((NCR_READ_REG(sc, NCR_CFG2) & ~NCRCFG2_RSVD) !=
	    (NCRCFG2_SCSI2 | NCRCFG2_RPE)) {
		sc->sc_rev = NCR_VARIANT_ESP100;
	} else {
		sc->sc_cfg2 = NCRCFG2_SCSI2;
		NCR_WRITE_REG(sc, NCR_CFG2, sc->sc_cfg2);
		sc->sc_cfg3 = 0;
		NCR_WRITE_REG(sc, NCR_CFG3, sc->sc_cfg3);
		sc->sc_cfg3 = (NCRCFG3_CDB | NCRCFG3_FCLK);
		NCR_WRITE_REG(sc, NCR_CFG3, sc->sc_cfg3);
		if (NCR_READ_REG(sc, NCR_CFG3) !=
		    (NCRCFG3_CDB | NCRCFG3_FCLK)) {
			sc->sc_rev = NCR_VARIANT_ESP100A;
		} else {
			/* NCRCFG2_FE enables > 64K transfers */
			sc->sc_cfg2 |= NCRCFG2_FE;
			sc->sc_cfg3 = 0;
			NCR_WRITE_REG(sc, NCR_CFG3, sc->sc_cfg3);
			sc->sc_rev = NCR_VARIANT_ESP200;
		}
	}

	/*
	 * XXX minsync and maxxfer _should_ be set up in MI code,
	 * XXX but it appears to have some dependency on what sort
	 * XXX of DMA we're hooked up to, etc.
	 */

	/*
	 * This is the value used to start sync negotiations
	 * Note that the NCR register "SYNCTP" is programmed
	 * in "clocks per byte", and has a minimum value of 4.
	 * The SCSI period used in negotiation is one-fourth
	 * of the time (in nanoseconds) needed to transfer one byte.
	 * Since the chip's clock is given in MHz, we have the following
	 * formula: 4 * period = (1000 / freq) * 4
	 */
	sc->sc_minsync = /* 1000 / sc->sc_freq */ 0;

	/*
	 * Alas, we must now modify the value a bit, because it's
	 * only valid when can switch on FASTCLK and FASTSCSI bits  
	 * in config register 3... 
	 */
	switch (sc->sc_rev) {
	case NCR_VARIANT_ESP100:
		sc->sc_maxxfer = 64 * 1024;
		sc->sc_minsync = 0;	/* No synch on old chip? */
		break;

	case NCR_VARIANT_ESP100A:
		sc->sc_maxxfer = 64 * 1024;
		/* Min clocks/byte is 5 */
		sc->sc_minsync = /* ncr53c9x_cpb2stp(sc, 5) */ 0;
		break;

	case NCR_VARIANT_ESP200:
		sc->sc_maxxfer = 16 * 1024 * 1024;
		/* XXX - do actually set FAST* bits */
		break;
	}

	/* @@@ Some ESP_DCTL bits probably need setting */
	NCR_WRITE_REG(sc, ESP_DCTL, 
	    ESPDCTL_16MHZ | ESPDCTL_INTENB | ESPDCTL_RESET);
	DELAY(10);
	DPRINTF(("esp dctl is 0x%02x\n",NCR_READ_REG(sc,ESP_DCTL)));
	NCR_WRITE_REG(sc, ESP_DCTL, ESPDCTL_16MHZ | ESPDCTL_INTENB);
	DELAY(10);
	DPRINTF(("esp dctl is 0x%02x\n",NCR_READ_REG(sc,ESP_DCTL)));

	esc->sc_dma = nextdma_findchannel ("scsi");
	if (esc->sc_dma) {
		findchannel_defer(self);
	} else {
		aprint_normal("\n");
		config_defer(self, findchannel_defer);
	}

	attached = 1;
}

/*
 * Glue functions.
 */

uint8_t
esp_read_reg(struct ncr53c9x_softc *sc, int reg)
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	return bus_space_read_1(esc->sc_bst, esc->sc_bsh, reg);
}

void
esp_write_reg(struct ncr53c9x_softc *sc, int reg, uint8_t val)
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	bus_space_write_1(esc->sc_bst, esc->sc_bsh, reg, val);
}

volatile uint32_t save1;

#define xADDR 0x0211a000
int doze(volatile int);
int
doze(volatile int c)
{
/* 	static int tmp1; */
	uint32_t tmp1;
	volatile uint8_t tmp2;
	volatile uint8_t *reg = (volatile uint8_t *)IIOV(xADDR);

	if (c > 244)
		return 0;
	if (c == 0)
		return 0;
/* 		((*(volatile u_long *)IIOV(NEXT_P_INTRMASK))&=(~NEXT_I_BIT(x))) */
	(*reg) = 0;
	(*reg) = 0;
	do {
		save1 = (*reg);
		tmp2 = *(reg + 3);
		tmp1 = tmp2;
	} while (tmp1 <= c);
	return 0;
}

int
esp_dma_isintr(struct ncr53c9x_softc *sc)
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	if (INTR_OCCURRED(NEXT_I_SCSI)) {
		NDTRACEIF (*ndtracep++ = 'i');
		NCR_WRITE_REG(sc, ESP_DCTL,
		    ESPDCTL_16MHZ | ESPDCTL_INTENB |
		    (esc->sc_datain ? ESPDCTL_DMARD : 0));
		return 1;
	} else {
		return 0;
	}
}

#define nd_bsr4(reg) \
	bus_space_read_4(nsc->sc_bst, nsc->sc_bsh, (reg))
#define nd_bsw4(reg,val) \
	bus_space_write_4(nsc->sc_bst, nsc->sc_bsh, (reg), (val))

int
esp_dma_intr(struct ncr53c9x_softc *sc)
{
	struct esp_softc *esc = (struct esp_softc *)sc;
	struct nextdma_softc *nsc = esc->sc_dma;
	struct nextdma_status *stat = &nsc->sc_stat;
	int r = (INTR_OCCURRED(NEXT_I_SCSI));
	int flushcount;

	r = 1;

	NDTRACEIF (*ndtracep++ = 'I');
	if (r) {
		/* printf ("esp_dma_isintr start\n"); */
		{
			int s = spldma();
			void *ndmap = stat->nd_map;
			int ndidx = stat->nd_idx;
			splx(s);

			flushcount = 0;

#ifdef ESP_DEBUG
/* 			esp_dma_nest++; */

			if (esp_debug) {
				char sbuf[256];

				snprintb(sbuf, sizeof(sbuf), NEXT_INTR_BITS, 
				    (*(volatile u_long *)IIOV(NEXT_P_INTRSTAT)));
				
				printf("esp_dma_isintr = 0x%s\n", sbuf);
			}
#endif

			while (!nextdma_finished(nsc)) {
			/* esp_dma_isactive(sc)) { */
				NDTRACEIF (*ndtracep++ = 'w');
				NDTRACEIF (
					sprintf(ndtracep, "f%dm%dl%dw",
					    NCR_READ_REG(sc, NCR_FFLAG) &
					    NCRFIFO_FF,
					    NCR_READ_REG((sc), NCR_TCM),
					    NCR_READ_REG((sc), NCR_TCL));
					ndtracep += strlen(ndtracep);
				);
				if (NCR_READ_REG(sc, NCR_FFLAG) & NCRFIFO_FF)
					flushcount = 5;
				NCR_WRITE_REG(sc, ESP_DCTL,
				    ESPDCTL_16MHZ | ESPDCTL_INTENB |
				    ESPDCTL_DMAMOD |
				    (esc->sc_datain ? ESPDCTL_DMARD : 0));

				s = spldma();
				while (ndmap == stat->nd_map &&
				    ndidx == stat->nd_idx &&
				    (nd_bsr4 (DD_CSR) & 0x08000000) == 0&&
				       ++flushcount < 5) {
					splx(s);
					NDTRACEIF (*ndtracep++ = 'F');
					NCR_WRITE_REG(sc, ESP_DCTL,
					    ESPDCTL_FLUSH | ESPDCTL_16MHZ |
					    ESPDCTL_INTENB | ESPDCTL_DMAMOD |
					    (esc->sc_datain ?
					     ESPDCTL_DMARD : 0));
					doze(0x32);
					NCR_WRITE_REG(sc, ESP_DCTL, 
					    ESPDCTL_16MHZ | ESPDCTL_INTENB |
					    ESPDCTL_DMAMOD |
					    (esc->sc_datain ?
					     ESPDCTL_DMARD : 0));
					doze(0x32);
					s = spldma();
				}
				NDTRACEIF (*ndtracep++ = '0' + flushcount);
				if (flushcount > 4) {
					int next;
					int onext = 0;

					splx(s);
					DPRINTF(("DMA reset\n"));
					while (((next = nd_bsr4 (DD_NEXT)) !=
					    (nd_bsr4(DD_LIMIT) & 0x7FFFFFFF)) &&
					     onext != next) {
						onext = next;
						DELAY(50);
					}
					NDTRACEIF (*ndtracep++ = 'R');
					NCR_WRITE_REG(sc, ESP_DCTL,
					    ESPDCTL_16MHZ | ESPDCTL_INTENB);
					NDTRACEIF (
						sprintf(ndtracep,
						    "ff:%d tcm:%d tcl:%d ",
						    NCR_READ_REG(sc, NCR_FFLAG)
						    & NCRFIFO_FF,
						    NCR_READ_REG((sc), NCR_TCM),
						    NCR_READ_REG((sc),
						    NCR_TCL));
						ndtracep += strlen (ndtracep);
						);
					s = spldma();
					nextdma_reset (nsc);
					splx(s);
					goto out;
				}
				splx(s);

#ifdef DIAGNOSTIC
				if (flushcount > 4) {
					NDTRACEIF (*ndtracep++ = '+');
					printf("%s: unexpected flushcount"
					    " %d on %s\n",
					    device_xname(sc->sc_dev),
					    flushcount,
					    esc->sc_datain ? "read" : "write");
				}
#endif

				if (!nextdma_finished(nsc)) {
				/* esp_dma_isactive(sc)) { */
					NDTRACEIF (*ndtracep++ = '1');
				}
				flushcount = 0;
				s = spldma();
				ndmap = stat->nd_map;
				ndidx = stat->nd_idx;
				splx(s);

			}
		out:
			;

#ifdef ESP_DEBUG
/* 			esp_dma_nest--; */
#endif

		}

		doze(0x32);
		NCR_WRITE_REG(sc, ESP_DCTL,
		    ESPDCTL_16MHZ | ESPDCTL_INTENB |
		    (esc->sc_datain ? ESPDCTL_DMARD : 0));
		NDTRACEIF (*ndtracep++ = 'b');

		while (esc->sc_datain != -1)
			DELAY(50);

		if (esc->sc_dmaaddr) {
			bus_size_t xfer_len = 0;
			int resid;

			NCR_WRITE_REG(sc, ESP_DCTL,
			    ESPDCTL_16MHZ | ESPDCTL_INTENB);
			if (stat->nd_exception == 0) {
				resid = NCR_READ_REG((sc), NCR_TCL) +
				    (NCR_READ_REG((sc), NCR_TCM) << 8);
				if (resid) {
					resid += (NCR_READ_REG(sc, NCR_FFLAG) &
					    NCRFIFO_FF);
#ifdef ESP_DEBUG
					if (NCR_READ_REG(sc, NCR_FFLAG) &
					    NCRFIFO_FF)
						if ((NCR_READ_REG(sc,
						    NCR_FFLAG) & NCRFIFO_FF) !=
						    16 ||
						    NCR_READ_REG((sc),
						    NCR_TCL) != 240)
							ndtraceshow++;
#endif
				}
				xfer_len = esc->sc_dmasize - resid;
			} else {
#define ncr53c9x_sched_msgout(m) \
	do {							\
		NCR_MISC(("ncr53c9x_sched_msgout %x %d", m, __LINE__));	\
		NCRCMD(sc, NCRCMD_SETATN);			\
		sc->sc_flags |= NCR_ATN;			\
		sc->sc_msgpriq |= (m);				\
	} while (0)
				int i;

				xfer_len = 0;
				if (esc->sc_begin)
					xfer_len += esc->sc_begin_size;
				if (esc->sc_main_dmamap)
					xfer_len +=
					    esc->sc_main_dmamap->dm_xfer_len;
				if (esc->sc_tail_dmamap)
					xfer_len +=
					    esc->sc_tail_dmamap->dm_xfer_len;
				resid = 0;
				printf ("X\n");
				for (i = 0; i < 16; i++) {
					NCR_WRITE_REG(sc, ESP_DCTL,
					    ESPDCTL_FLUSH | ESPDCTL_16MHZ |
					    ESPDCTL_INTENB |
					    (esc->sc_datain ?
					     ESPDCTL_DMARD : 0));
					NCR_WRITE_REG(sc, ESP_DCTL, 
					    ESPDCTL_16MHZ | ESPDCTL_INTENB |
					    (esc->sc_datain ?
					     ESPDCTL_DMARD : 0));
				}
#if 0
				printf ("ff:%02x tcm:%d tcl:%d esp_dstat:%02x"
				    " stat:%02x step: %02x intr:%02x"
				    " new stat:%02X\n",
				    NCR_READ_REG(sc, NCR_FFLAG),
				    NCR_READ_REG((sc), NCR_TCM),
				    NCR_READ_REG((sc), NCR_TCL),
				    NCR_READ_REG(sc, ESP_DSTAT),
				    sc->sc_espstat, sc->sc_espstep,
				    sc->sc_espintr,
				    NCR_READ_REG(sc, NCR_STAT));
				printf("sc->sc_state: %x sc->sc_phase: %x"
				    " sc->sc_espstep:%x sc->sc_prevphase:%x"
				    " sc->sc_flags:%x\n",
				    sc->sc_state, sc->sc_phase, sc->sc_espstep,
				    sc->sc_prevphase, sc->sc_flags);
#endif
				/* sc->sc_flags &= ~NCR_ICCS; */
				sc->sc_nexus->flags |= ECB_ABORT;
				if (sc->sc_phase == MESSAGE_IN_PHASE) {
					/* ncr53c9x_sched_msgout(SEND_ABORT); */
					ncr53c9x_abort(sc, sc->sc_nexus);
				} else if (sc->sc_phase != STATUS_PHASE) {
					printf("ATTENTION!!!  "
					    "not message/status phase: %d\n",
					    sc->sc_phase);
				}
			}
			
			NDTRACEIF(
				sprintf(ndtracep, "f%dm%dl%ds%dx%dr%dS",
				    NCR_READ_REG(sc, NCR_FFLAG) & NCRFIFO_FF,
				    NCR_READ_REG((sc), NCR_TCM),
				    NCR_READ_REG((sc), NCR_TCL),
				    esc->sc_dmasize, (int)xfer_len, resid);
				ndtracep += strlen(ndtracep);
			);

			*esc->sc_dmaaddr += xfer_len;
			*esc->sc_dmalen -= xfer_len;
			esc->sc_dmaaddr = 0;
			esc->sc_dmalen  = 0;
			esc->sc_dmasize = 0;
		}

		NDTRACEIF (*ndtracep++ = 'B');
		sc->sc_espstat = NCR_READ_REG(sc, NCR_STAT) |
		    (sc->sc_espstat & NCRSTAT_INT);

		DPRINTF(("esp dctl is 0x%02x\n", NCR_READ_REG(sc, ESP_DCTL)));
		/* printf ("esp_dma_isintr DONE\n"); */

	}

	return r;
}

void
esp_dma_reset(struct ncr53c9x_softc *sc)
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	DPRINTF(("esp DMA reset\n"));

#ifdef ESP_DEBUG
	if (esp_debug) {
		char sbuf[256];

		snprintb(sbuf, sizeof(sbuf), NEXT_INTR_BITS, 
		    (*(volatile u_long *)IIOV(NEXT_P_INTRSTAT)));
		printf("  *intrstat = 0x%s\n", sbuf);

		snprintb(sbuf, sizeof(sbuf), NEXT_INTR_BITS, 
		    (*(volatile u_long *)IIOV(NEXT_P_INTRMASK)));
		printf("  *intrmask = 0x%s\n", sbuf);
	}
#endif

#if 0
	/* Clear the DMAMOD bit in the DCTL register: */
	NCR_WRITE_REG(sc, ESP_DCTL, ESPDCTL_16MHZ | ESPDCTL_INTENB);
	DPRINTF(("esp dctl is 0x%02x\n",NCR_READ_REG(sc,ESP_DCTL)));
#endif

	nextdma_reset(esc->sc_dma);
	nextdma_init(esc->sc_dma);

	esc->sc_datain = -1;
	esc->sc_dmaaddr = 0;
	esc->sc_dmalen  = 0;
	esc->sc_dmasize = 0;

	esc->sc_loaded = 0;

	esc->sc_begin = 0;
	esc->sc_begin_size = 0;

	if (esc->sc_main_dmamap->dm_mapsize) {
		bus_dmamap_unload(esc->sc_dma->sc_dmat, esc->sc_main_dmamap);
	}
	esc->sc_main = 0;
	esc->sc_main_size = 0;

	if (esc->sc_tail_dmamap->dm_mapsize) {
		bus_dmamap_unload(esc->sc_dma->sc_dmat, esc->sc_tail_dmamap);
	}
	esc->sc_tail = 0;
	esc->sc_tail_size = 0;
}

/* it appears that:
 * addr and len arguments to this need to be kept up to date
 * with the status of the transfter.
 * the dmasize of this is the actual length of the transfer
 * request, which is guaranteed to be less than maxxfer.
 * (len may be > maxxfer)
 */

int
esp_dma_setup(struct ncr53c9x_softc *sc, uint8_t **addr, size_t *len,
    int datain, size_t *dmasize)
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	NDTRACEIF (*ndtracep++ = 'h');
#ifdef DIAGNOSTIC
#ifdef ESP_DEBUG
	/* if this is a read DMA, pre-fill the buffer with 0xdeadbeef
	 * to identify bogus reads
	 */
	if (datain) {
		int *v = (int *)(*addr);
		int i;
		for (i = 0; i < ((*len) / 4); i++)
			v[i] = 0xdeadbeef;
		v = (int *)(&(esc->sc_tailbuf[0]));
		for (i = 0; i < ((sizeof(esc->sc_tailbuf) / 4)); i++)
			v[i] = 0xdeafbeef;
	} else {
		int *v;
		int i;
		v = (int *)(&(esc->sc_tailbuf[0]));
		for (i = 0; i < ((sizeof(esc->sc_tailbuf) / 4)); i++)
			v[i] = 0xfeeb1eed;
	}
#endif
#endif

	DPRINTF(("esp_dma_setup(%p,0x%08x,0x%08x)\n", *addr, *len, *dmasize));

#if 0
#ifdef DIAGNOSTIC /* @@@ this is ok sometimes. verify that we handle it ok
		   * and then remove this check
		   */
	if (*len != *dmasize) {
		panic("esp dmalen 0x%lx != size 0x%lx", *len, *dmasize);
	}
#endif
#endif

#ifdef DIAGNOSTIC
	if ((esc->sc_datain != -1) ||
	    (esc->sc_main_dmamap->dm_mapsize != 0) ||
	    (esc->sc_tail_dmamap->dm_mapsize != 0) ||
	    (esc->sc_dmasize != 0)) {			
		panic("%s: map already loaded in esp_dma_setup"
		    "\tdatain = %d\n\tmain_mapsize=%ld\n"
		    "\tail_mapsize=%ld\n\tdmasize = %d",
		    device_xname(sc->sc_dev), esc->sc_datain,
		    esc->sc_main_dmamap->dm_mapsize,
		    esc->sc_tail_dmamap->dm_mapsize,
		    esc->sc_dmasize);
	}
#endif

	/* we are sometimes asked to DMA zero  bytes, that's easy */
	if (*dmasize <= 0) {
		return 0;
	}

	if (*dmasize > ESP_MAX_DMASIZE)
		*dmasize = ESP_MAX_DMASIZE;

	/* Save these in case we have to abort DMA */
	esc->sc_datain   = datain;
	esc->sc_dmaaddr  = addr;
	esc->sc_dmalen   = len;
	esc->sc_dmasize  = *dmasize;

	esc->sc_loaded = 0;

#define DMA_SCSI_ALIGNMENT 16
#define DMA_SCSI_ALIGN(type, addr)	\
	((type)(((unsigned int)(addr) + DMA_SCSI_ALIGNMENT - 1) \
		&~(DMA_SCSI_ALIGNMENT-1)))
#define DMA_SCSI_ALIGNED(addr) \
	(((unsigned int)(addr) & (DMA_SCSI_ALIGNMENT - 1))==0)

	{
		size_t slop_bgn_size; /* # bytes to be fifo'd at beginning */
		size_t slop_end_size; /* # bytes to be transferred in tail buffer */
		
		{
			u_long bgn = (u_long)(*esc->sc_dmaaddr);
			u_long end = bgn + esc->sc_dmasize;

			slop_bgn_size =
			    DMA_SCSI_ALIGNMENT - (bgn % DMA_SCSI_ALIGNMENT);
			if (slop_bgn_size == DMA_SCSI_ALIGNMENT)
				slop_bgn_size = 0;
			slop_end_size = end % DMA_ENDALIGNMENT;
		}

		/* Force a minimum slop end size. This ensures that write
		 * requests will overrun, as required to get completion
		 * interrupts.
		 * In addition, since the tail buffer is guaranteed to be mapped
		 * in a single DMA segment, the overrun won't accidentally
		 * end up in its own segment.
		 */
		if (!esc->sc_datain) {
#if 0
			slop_end_size += ESP_DMA_MAXTAIL;
#else
			slop_end_size += 0x10;
#endif
		}

		/* Check to make sure we haven't counted extra slop
		 * as would happen for a very short DMA buffer, also
		 * for short buffers, just stuff the entire thing in the tail
		 */
		if ((slop_bgn_size+slop_end_size >= esc->sc_dmasize)
#if 0
		    || (esc->sc_dmasize <= ESP_DMA_MAXTAIL)
#endif
		    ) {
 			slop_bgn_size = 0;
			slop_end_size = esc->sc_dmasize;
		}

		/* initialize the fifo buffer */
		if (slop_bgn_size) {
			esc->sc_begin = *esc->sc_dmaaddr;
			esc->sc_begin_size = slop_bgn_size;
		} else {
			esc->sc_begin = 0;
			esc->sc_begin_size = 0;
		}

#if 01
		/* Load the normal DMA map */
		{
			esc->sc_main = *esc->sc_dmaaddr;
			esc->sc_main += slop_bgn_size;
			esc->sc_main_size =
			    (esc->sc_dmasize) - (slop_end_size+slop_bgn_size);

			if (esc->sc_main_size) {
				int error;

				if (!esc->sc_datain ||
				    DMA_ENDALIGNED(esc->sc_main_size +
				    slop_end_size)) {
					KASSERT(DMA_SCSI_ALIGNMENT ==
					    DMA_ENDALIGNMENT);
					KASSERT(DMA_BEGINALIGNMENT ==
					    DMA_ENDALIGNMENT);
					esc->sc_main_size += slop_end_size;
					slop_end_size = 0;
					if (!esc->sc_datain) {
						esc->sc_main_size =
						    DMA_ENDALIGN(uint8_t *,
						    esc->sc_main +
						    esc->sc_main_size) -
						    esc->sc_main;
					}
				}

				error = bus_dmamap_load(esc->sc_dma->sc_dmat,
				    esc->sc_main_dmamap,
				    esc->sc_main, esc->sc_main_size,
				    NULL, BUS_DMA_NOWAIT);
				if (error) {
#ifdef ESP_DEBUG
					printf("%s: esc->sc_main_dmamap->"
					    "_dm_size = %ld\n",
					    device_xname(sc->sc_dev),
					    esc->sc_main_dmamap->_dm_size);
					printf("%s: esc->sc_main_dmamap->"
					    "_dm_segcnt = %d\n",
					    device_xname(sc->sc_dev),
					    esc->sc_main_dmamap->_dm_segcnt);
					printf("%s: esc->sc_main_dmamap->"
					    "_dm_maxsegsz = %ld\n",
					    device_xname(sc->sc_dev),
					    esc->sc_main_dmamap->_dm_maxsegsz);
					printf("%s: esc->sc_main_dmamap->"
					    "_dm_boundary = %ld\n",
					    device_xname(sc->sc_dev),
					    esc->sc_main_dmamap->_dm_boundary);
					esp_dma_print(sc);
#endif
					panic("%s: can't load main DMA map."
					    " error = %d, addr=%p, size=0x%08x",
					    device_xname(sc->sc_dev),
					    error, esc->sc_main,
					    esc->sc_main_size);
				}
				if (!esc->sc_datain) {
					/*
					 * patch the DMA map for write overrun
					*/
					esc->sc_main_dmamap->dm_mapsize +=
					    ESP_DMA_OVERRUN;
					esc->sc_main_dmamap->dm_segs[
					    esc->sc_main_dmamap->dm_nsegs -
					    1].ds_len +=
						ESP_DMA_OVERRUN;
				}
#if 0
				bus_dmamap_sync(esc->sc_dma->sc_dmat,
				    esc->sc_main_dmamap,
				    0, esc->sc_main_dmamap->dm_mapsize, 
				    (esc->sc_datain ?  BUS_DMASYNC_PREREAD :
				     BUS_DMASYNC_PREWRITE));
				esc->sc_main_dmamap->dm_xfer_len = 0;
#endif
			} else {
				esc->sc_main = 0;
			}
		}

		/* Load the tail DMA map */
		if (slop_end_size) {
			esc->sc_tail = DMA_ENDALIGN(uint8_t *,
			    esc->sc_tailbuf + slop_end_size) - slop_end_size;
			/*
			 * If the beginning of the tail is not correctly
			 * aligned, we have no choice but to align the start,
			 * which might then unalign the end.
			 */
			esc->sc_tail = DMA_SCSI_ALIGN(uint8_t *, esc->sc_tail);
			/*
			 * So therefore, we change the tail size to be
			 * end aligned again.
			 */
			esc->sc_tail_size = DMA_ENDALIGN(uint8_t *,
			    esc->sc_tail + slop_end_size) - esc->sc_tail;

			/* @@@ next DMA overrun lossage */
			if (!esc->sc_datain) {
				esc->sc_tail_size += ESP_DMA_OVERRUN;
			}

			{
				int error;
				error = bus_dmamap_load(esc->sc_dma->sc_dmat,
				    esc->sc_tail_dmamap,
			 	    esc->sc_tail, esc->sc_tail_size,
				    NULL, BUS_DMA_NOWAIT);
				if (error) {
					panic("%s: can't load tail DMA map."
					    " error = %d, addr=%p, size=0x%08x",
					    device_xname(sc->sc_dev), error,
					    esc->sc_tail,esc->sc_tail_size);
				}
#if 0
				bus_dmamap_sync(esc->sc_dma->sc_dmat,
				    esc->sc_tail_dmamap, 0,
				    esc->sc_tail_dmamap->dm_mapsize, 
				    (esc->sc_datain ? BUS_DMASYNC_PREREAD :
				     BUS_DMASYNC_PREWRITE));
				esc->sc_tail_dmamap->dm_xfer_len = 0;
#endif
			}
		}
#else

		esc->sc_begin = *esc->sc_dmaaddr;
		slop_bgn_size = DMA_SCSI_ALIGNMENT -
		    ((u_long)esc->sc_begin % DMA_SCSI_ALIGNMENT);
		if (slop_bgn_size == DMA_SCSI_ALIGNMENT)
			slop_bgn_size = 0;
		slop_end_size = esc->sc_dmasize - slop_bgn_size;

		if (slop_bgn_size < esc->sc_dmasize) {
			int error;

			esc->sc_tail = 0;
			esc->sc_tail_size = 0;

			esc->sc_begin_size = slop_bgn_size;
			esc->sc_main = *esc->sc_dmaaddr;
			esc->sc_main += slop_bgn_size;
			esc->sc_main_size = DMA_ENDALIGN(uint8_t *,
			    esc->sc_main + esc->sc_dmasize - slop_bgn_size) -
			    esc->sc_main;

			if (!esc->sc_datain) {
				esc->sc_main_size += ESP_DMA_OVERRUN;
			}
			error = bus_dmamap_load(esc->sc_dma->sc_dmat,
			    esc->sc_main_dmamap,
			    esc->sc_main, esc->sc_main_size,
			    NULL, BUS_DMA_NOWAIT);
			if (error) {
				panic("%s: can't load main DMA map."
				    " error = %d, addr=%p, size=0x%08x",
				    device_xname(sc->sc_dev), error,
				    esc->sc_main,esc->sc_main_size);
			}
		} else {
			esc->sc_begin = 0;
			esc->sc_begin_size = 0;
			esc->sc_main = 0;
			esc->sc_main_size = 0;

#if 0
			esc->sc_tail = DMA_ENDALIGN(uint8_t *,
			    esc->sc_tailbuf + slop_bgn_size) - slop_bgn_size;
			/*
			 * If the beginning of the tail is not correctly
			 * aligned, we have no choice but to align the start,
			 * which might then unalign the end.
			 */
#endif
			esc->sc_tail = DMA_SCSI_ALIGN(void *, esc->sc_tailbuf);
			/*
			 * So therefore, we change the tail size to be
			 * end aligned again.
			 */
			esc->sc_tail_size = DMA_ENDALIGN(uint8_t *,
			    esc->sc_tail + esc->sc_dmasize) - esc->sc_tail;

			/* @@@ next DMA overrun lossage */
			if (!esc->sc_datain) {
				esc->sc_tail_size += ESP_DMA_OVERRUN;
			}

			{
				int error;
				error = bus_dmamap_load(esc->sc_dma->sc_dmat,
				    esc->sc_tail_dmamap,
				    esc->sc_tail, esc->sc_tail_size,
				    NULL, BUS_DMA_NOWAIT);
				if (error) {
					panic("%s: can't load tail DMA map."
					    " error = %d, addr=%p, size=0x%08x",
					    device_xname(sc->sc_dev), error,
					    esc->sc_tail, esc->sc_tail_size);
				}
			}
		}
#endif

		DPRINTF(("%s: setup: %8p %d %8p %d %8p %d %8p %d\n",
		    device_xname(sc->sc_dev),
		    *esc->sc_dmaaddr, esc->sc_dmasize,
		    esc->sc_begin, esc->sc_begin_size,
		    esc->sc_main, esc->sc_main_size,
		    esc->sc_tail, esc->sc_tail_size));
	}

	return 0;
}

#ifdef ESP_DEBUG
/* For debugging */
void
esp_dma_store(struct ncr53c9x_softc *sc)
{
	struct esp_softc *esc = (struct esp_softc *)sc;
	char *p = &esp_dma_dump[0];
	
	p += sprintf(p, "%s: sc_datain=%d\n",
	    device_xname(sc->sc_dev), esc->sc_datain);
	p += sprintf(p, "%s: sc_loaded=0x%08x\n",
	    device_xname(sc->sc_dev), esc->sc_loaded);

	if (esc->sc_dmaaddr) {
		p += sprintf(p, "%s: sc_dmaaddr=%p\n",
		    device_xname(sc->sc_dev), *esc->sc_dmaaddr);
	} else {
		p += sprintf(p, "%s: sc_dmaaddr=NULL\n",
		    device_xname(sc->sc_dev));
	}
	if (esc->sc_dmalen) {
		p += sprintf(p, "%s: sc_dmalen=0x%08x\n", 
		    device_xname(sc->sc_dev), *esc->sc_dmalen);
	} else {
		p += sprintf(p, "%s: sc_dmalen=NULL\n",
		    device_xname(sc->sc_dev));
	}
	p += sprintf(p, "%s: sc_dmasize=0x%08x\n",
	    device_xname(sc->sc_dev), esc->sc_dmasize);

	p += sprintf(p, "%s: sc_begin = %p, sc_begin_size = 0x%08x\n",
	    device_xname(sc->sc_dev), esc->sc_begin, esc->sc_begin_size);
	p += sprintf(p, "%s: sc_main = %p, sc_main_size = 0x%08x\n",
	    device_xname(sc->sc_dev), esc->sc_main, esc->sc_main_size);
	/* if (esc->sc_main) */ {
		int i;
		bus_dmamap_t map = esc->sc_main_dmamap;
		p += sprintf(p, "%s: sc_main_dmamap."
		    " mapsize = 0x%08lx, nsegs = %d\n",
		    device_xname(sc->sc_dev), map->dm_mapsize, map->dm_nsegs);
		for(i = 0; i < map->dm_nsegs; i++) {
			p += sprintf(p, "%s:"
			    " map->dm_segs[%d].ds_addr = 0x%08lx,"
			    " len = 0x%08lx\n",
			    device_xname(sc->sc_dev),
			    i, map->dm_segs[i].ds_addr,
			    map->dm_segs[i].ds_len);
		}
	}
	p += sprintf(p, "%s: sc_tail = %p, sc_tail_size = 0x%08x\n",
	    device_xname(sc->sc_dev), esc->sc_tail, esc->sc_tail_size);
	/* if (esc->sc_tail) */ {
		int i;
		bus_dmamap_t map = esc->sc_tail_dmamap;
		p += sprintf(p, "%s: sc_tail_dmamap."
		    " mapsize = 0x%08lx, nsegs = %d\n",
		    device_xname(sc->sc_dev), map->dm_mapsize, map->dm_nsegs);
		for (i = 0; i < map->dm_nsegs; i++) {
			p += sprintf(p, "%s:"
			    " map->dm_segs[%d].ds_addr = 0x%08lx,"
			    " len = 0x%08lx\n",
			    device_xname(sc->sc_dev),
			    i, map->dm_segs[i].ds_addr,
			     map->dm_segs[i].ds_len);
		}
	}
}

void
esp_dma_print(struct ncr53c9x_softc *sc)
{

	esp_dma_store(sc);
	printf("%s", esp_dma_dump);
}
#endif

void
esp_dma_go(struct ncr53c9x_softc *sc)
{
	struct esp_softc *esc = (struct esp_softc *)sc;
	struct nextdma_softc *nsc = esc->sc_dma;
	struct nextdma_status *stat = &nsc->sc_stat;
/* 	int s = spldma(); */

#ifdef ESP_DEBUG
	if (ndtracep != ndtrace) {
		if (ndtraceshow) {
			*ndtracep = '\0';
			printf("esp ndtrace: %s\n", ndtrace);
			ndtraceshow = 0;
		} else {
			DPRINTF(("X"));
		}
		ndtracep = ndtrace;
	}
#endif

	DPRINTF(("%s: esp_dma_go(datain = %d)\n",
	    device_xname(sc->sc_dev), esc->sc_datain));

#ifdef ESP_DEBUG
	if (esp_debug)
		esp_dma_print(sc);
	else
		esp_dma_store(sc);
#endif

#ifdef ESP_DEBUG
	{
		int n = NCR_READ_REG(sc, NCR_FFLAG);
		DPRINTF(("%s: fifo size = %d, seq = 0x%x\n",
		    device_xname(sc->sc_dev),
		    n & NCRFIFO_FF, (n & NCRFIFO_SS) >> 5));
	}
#endif

	/* zero length DMA transfers are boring */
	if (esc->sc_dmasize == 0) {
/* 		splx(s); */
		return;
	}

#if defined(DIAGNOSTIC)
	if ((esc->sc_begin_size == 0) &&
	    (esc->sc_main_dmamap->dm_mapsize == 0) &&
	    (esc->sc_tail_dmamap->dm_mapsize == 0)) {
#ifdef ESP_DEBUG
		esp_dma_print(sc);
#endif
		panic("%s: No DMA requested!", device_xname(sc->sc_dev));
	}
#endif

	/* Stuff the fifo with the begin buffer */
	if (esc->sc_datain) {
		int i;
		DPRINTF(("%s: FIFO read of %d bytes:",
		    device_xname(sc->sc_dev), esc->sc_begin_size));
		for (i = 0; i < esc->sc_begin_size; i++) {
			esc->sc_begin[i] = NCR_READ_REG(sc, NCR_FIFO);
			DPRINTF((" %02x", esc->sc_begin[i] & 0xff));
		}
		DPRINTF(("\n"));
	} else {
		int i;
		DPRINTF(("%s: FIFO write of %d bytes:",
		    device_xname(sc->sc_dev), esc->sc_begin_size));
		for (i = 0; i < esc->sc_begin_size; i++) {
			NCR_WRITE_REG(sc, NCR_FIFO, esc->sc_begin[i]);
			DPRINTF((" %02x",esc->sc_begin[i] & 0xff));
		}
		DPRINTF(("\n"));
	}

	if (esc->sc_main_dmamap->dm_mapsize) {
		bus_dmamap_sync(esc->sc_dma->sc_dmat, esc->sc_main_dmamap,
		    0, esc->sc_main_dmamap->dm_mapsize, 
		    (esc->sc_datain ?
		     BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE));
		esc->sc_main_dmamap->dm_xfer_len = 0;
	}

	if (esc->sc_tail_dmamap->dm_mapsize) {
		/* if we are a DMA write cycle, copy the end slop */
		if (!esc->sc_datain) {
			memcpy(esc->sc_tail, *esc->sc_dmaaddr +
			    esc->sc_begin_size+esc->sc_main_size,
			    esc->sc_dmasize -
			    (esc->sc_begin_size + esc->sc_main_size));
		}
		bus_dmamap_sync(esc->sc_dma->sc_dmat, esc->sc_tail_dmamap,
		    0, esc->sc_tail_dmamap->dm_mapsize, 
		    (esc->sc_datain ?
		     BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE));
		esc->sc_tail_dmamap->dm_xfer_len = 0;
	}

	stat->nd_exception = 0;
	nextdma_start(nsc, (esc->sc_datain ? DMACSR_SETREAD : DMACSR_SETWRITE));

	if (esc->sc_datain) { 
		NCR_WRITE_REG(sc, ESP_DCTL,
		    ESPDCTL_16MHZ | ESPDCTL_INTENB | ESPDCTL_DMAMOD |
		    ESPDCTL_DMARD);
	} else {
		NCR_WRITE_REG(sc, ESP_DCTL,
		    ESPDCTL_16MHZ | ESPDCTL_INTENB | ESPDCTL_DMAMOD);
	}
	DPRINTF(("esp dctl is 0x%02x\n",NCR_READ_REG(sc,ESP_DCTL)));

	NDTRACEIF(
		if (esc->sc_begin_size) {
			*ndtracep++ = '1';
			*ndtracep++ = 'A' + esc->sc_begin_size;
		}
	);
	NDTRACEIF(
		if (esc->sc_main_size) {
			*ndtracep++ = '2';
			*ndtracep++ = '0' + esc->sc_main_dmamap->dm_nsegs;
		}
	);
	NDTRACEIF(
		if (esc->sc_tail_size) {
			*ndtracep++ = '3';
			*ndtracep++ = 'A' + esc->sc_tail_size;
		}
	);

/* 	splx(s); */
}

void
esp_dma_stop(struct ncr53c9x_softc *sc)
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	nextdma_print(esc->sc_dma);
#ifdef ESP_DEBUG
	esp_dma_print(sc);
#endif
#if 1
	panic("%s: stop not yet implemented", device_xname(sc->sc_dev));
#endif
}

int
esp_dma_isactive(struct ncr53c9x_softc *sc)
{
	struct esp_softc *esc = (struct esp_softc *)sc;
	int r;

	r = (esc->sc_dmaaddr != NULL);   /* !nextdma_finished(esc->sc_dma); */
	DPRINTF(("esp_dma_isactive = %d\n",r));
	return r;
}

/****************************************************************/

int esp_dma_int(void *);
int esp_dma_int(void *arg)
{
	void nextdma_rotate(struct nextdma_softc *);
	void nextdma_setup_curr_regs(struct nextdma_softc *);
	void nextdma_setup_cont_regs(struct nextdma_softc *);

	struct ncr53c9x_softc *sc = (struct ncr53c9x_softc *)arg;
	struct esp_softc *esc = (struct esp_softc *)sc;
	struct nextdma_softc *nsc = esc->sc_dma;
	struct nextdma_status *stat = &nsc->sc_stat;
	unsigned int state;

	NDTRACEIF (*ndtracep++ = 'E');

	state = nd_bsr4 (DD_CSR);

#if 1
	NDTRACEIF (
		if (state & DMACSR_COMPLETE)
			*ndtracep++ = 'c';
		if (state & DMACSR_ENABLE)
			*ndtracep++ = 'e';
		if (state & DMACSR_BUSEXC)
			*ndtracep++ = 'b';
		if (state & DMACSR_READ)
			*ndtracep++ = 'r';
		if (state & DMACSR_SUPDATE)
			*ndtracep++ = 's';
		);

	NDTRACEIF (*ndtracep++ = 'E');

#ifdef ESP_DEBUG
	if (0)
		if ((state & DMACSR_BUSEXC) && (state & DMACSR_ENABLE))
			ndtraceshow++;
	if (0)
		if ((state & DMACSR_SUPDATE))
			ndtraceshow++;
#endif
#endif

	if ((stat->nd_exception == 0) &&
	    (state & DMACSR_COMPLETE) &&
	    (state & DMACSR_ENABLE)) {
		stat->nd_map->dm_xfer_len +=
		    stat->nd_map->dm_segs[stat->nd_idx].ds_len;
	}

	if ((stat->nd_idx + 1) == stat->nd_map->dm_nsegs) {
		if (nsc->sc_conf.nd_completed_cb) 
			(*nsc->sc_conf.nd_completed_cb)(stat->nd_map,
			    nsc->sc_conf.nd_cb_arg);
	}
	nextdma_rotate(nsc);

	if ((state & DMACSR_COMPLETE) && (state & DMACSR_ENABLE)) {
#if 0
		int l = nd_bsr4 (DD_LIMIT) & 0x7FFFFFFF;
		int s = nd_bsr4 (DD_STOP);
#endif
/* 		nextdma_setup_cont_regs(nsc); */
		if (stat->nd_map_cont) {
			nd_bsw4(DD_START, stat->nd_map_cont->dm_segs[
			    stat->nd_idx_cont].ds_addr);
			nd_bsw4(DD_STOP, (stat->nd_map_cont->dm_segs[
			    stat->nd_idx_cont].ds_addr +
			    stat->nd_map_cont->dm_segs[
			    stat->nd_idx_cont].ds_len));
		}

		nd_bsw4 (DD_CSR, DMACSR_CLRCOMPLETE |
		    (state & DMACSR_READ ? DMACSR_SETREAD : DMACSR_SETWRITE) |
		     (stat->nd_map_cont ? DMACSR_SETSUPDATE : 0));

#if 0
#ifdef ESP_DEBUG
		if (state & DMACSR_BUSEXC) {
			sprintf(ndtracep, "CE/BUSEXC: %08lX %08X %08X\n", 
			    (stat->nd_map->dm_segs[stat->nd_idx].ds_addr +
			     stat->nd_map->dm_segs[stat->nd_idx].ds_len),
			    l, s);
			ndtracep += strlen(ndtracep);
		}
#endif
#endif
	} else {
#if 0
		if (state & DMACSR_BUSEXC) {
			while (nd_bsr4(DD_NEXT) !=
			       (nd_bsr4(DD_LIMIT) & 0x7FFFFFFF))
				printf("Y"); /* DELAY(50); */
			state = nd_bsr4(DD_CSR);
		}
#endif

		if (!(state & DMACSR_SUPDATE)) {
			nextdma_rotate(nsc);
		} else {
			nd_bsw4(DD_CSR, DMACSR_CLRCOMPLETE |
			    DMACSR_INITBUF | DMACSR_RESET |
			    (state & DMACSR_READ ?
			     DMACSR_SETREAD : DMACSR_SETWRITE));

			nd_bsw4(DD_NEXT,
			    stat->nd_map->dm_segs[stat->nd_idx].ds_addr);
			nd_bsw4(DD_LIMIT, 
			    (stat->nd_map->dm_segs[stat->nd_idx].ds_addr +
			    stat->nd_map->dm_segs[stat->nd_idx].ds_len) |
			    0/* x80000000 */);
			if (stat->nd_map_cont) {
				nd_bsw4(DD_START,
				    stat->nd_map_cont->dm_segs[
				    stat->nd_idx_cont].ds_addr);
				nd_bsw4(DD_STOP,
				    (stat->nd_map_cont->dm_segs[
				     stat->nd_idx_cont].ds_addr +
				     stat->nd_map_cont->dm_segs[
				     stat->nd_idx_cont].ds_len) |
				     0/* x80000000 */);
			}
			nd_bsw4(DD_CSR, DMACSR_SETENABLE | DMACSR_CLRCOMPLETE |
			    (state & DMACSR_READ ?
			     DMACSR_SETREAD : DMACSR_SETWRITE) |
			    (stat->nd_map_cont ? DMACSR_SETSUPDATE : 0));
#if 1
#ifdef ESP_DEBUG
				sprintf(ndtracep, "supdate ");
				ndtracep += strlen(ndtracep);
				sprintf(ndtracep, "%08X %08X %08X %08X ",
				    nd_bsr4(DD_NEXT),
				    nd_bsr4(DD_LIMIT) & 0x7FFFFFFF,
				    nd_bsr4 (DD_START),
				    nd_bsr4 (DD_STOP) & 0x7FFFFFFF);
				ndtracep += strlen(ndtracep);
#endif
#endif
			stat->nd_exception++; 
			return 1;
			/* NCR_WRITE_REG(sc, ESP_DCTL, ctl); */
			goto restart;
		}

		if (stat->nd_map) {
#if 1
#ifdef ESP_DEBUG
			sprintf(ndtracep, "%08X %08X %08X %08X ",
			    nd_bsr4 (DD_NEXT),
			    nd_bsr4 (DD_LIMIT) & 0x7FFFFFFF,
			    nd_bsr4 (DD_START),
			    nd_bsr4 (DD_STOP) & 0x7FFFFFFF);
			ndtracep += strlen(ndtracep);
#endif
#endif

#if 0			
			nd_bsw4(DD_CSR, DMACSR_CLRCOMPLETE | DMACSR_RESET);
			
			nd_bsw4(DD_CSR, 0);
#endif
#if 1
 /* 6/2 */
			nd_bsw4(DD_CSR, DMACSR_CLRCOMPLETE |
			    DMACSR_INITBUF | DMACSR_RESET |
			    (state & DMACSR_READ ?
			     DMACSR_SETREAD : DMACSR_SETWRITE));
			
			/* nextdma_setup_curr_regs(nsc); */
			nd_bsw4(DD_NEXT,
			    stat->nd_map->dm_segs[stat->nd_idx].ds_addr);
			nd_bsw4(DD_LIMIT, 
			    (stat->nd_map->dm_segs[stat->nd_idx].ds_addr +
			    stat->nd_map->dm_segs[stat->nd_idx].ds_len) |
			    0/* x80000000 */);
			/* nextdma_setup_cont_regs(nsc); */
			if (stat->nd_map_cont) {
				nd_bsw4(DD_START,
				    stat->nd_map_cont->dm_segs[
				    stat->nd_idx_cont].ds_addr);
				nd_bsw4(DD_STOP,
				    (stat->nd_map_cont->dm_segs[
				    stat->nd_idx_cont].ds_addr +
				    stat->nd_map_cont->dm_segs[
				    stat->nd_idx_cont].ds_len) |
				    0/* x80000000 */);
			}
			
			nd_bsw4(DD_CSR, DMACSR_SETENABLE |
			    (stat->nd_map_cont ? DMACSR_SETSUPDATE : 0) |
			    (state & DMACSR_READ ?
			     DMACSR_SETREAD : DMACSR_SETWRITE));
#ifdef ESP_DEBUG
			/* ndtraceshow++; */
#endif
			stat->nd_exception++; 
			return 1;
#endif
			/* NCR_WRITE_REG(sc, ESP_DCTL, ctl); */
			goto restart;
		restart:
#if 1
#ifdef ESP_DEBUG
			sprintf(ndtracep, "restart %08lX %08lX\n",
			    stat->nd_map->dm_segs[stat->nd_idx].ds_addr, 
			    stat->nd_map->dm_segs[stat->nd_idx].ds_addr +
			    stat->nd_map->dm_segs[stat->nd_idx].ds_len);
			if (stat->nd_map_cont) {
				sprintf(ndtracep + strlen(ndtracep) - 1,
				    " %08lX %08lX\n",
				    stat->nd_map_cont->dm_segs[
				    stat->nd_idx_cont].ds_addr,
				    stat->nd_map_cont->dm_segs[
				    stat->nd_idx_cont].ds_addr +
				    stat->nd_map_cont->dm_segs[
				    stat->nd_idx_cont].ds_len);
			}
			ndtracep += strlen(ndtracep);
#endif
#endif
			nextdma_print(nsc);
			NCR_WRITE_REG(sc, ESP_DCTL,
			    ESPDCTL_16MHZ | ESPDCTL_INTENB);
			printf("ff:%02x tcm:%d tcl:%d esp_dstat:%02x"
			    " state:%02x step: %02x intr:%02x state:%08X\n",
			    NCR_READ_REG(sc, NCR_FFLAG),
			    NCR_READ_REG((sc), NCR_TCM),
			    NCR_READ_REG((sc), NCR_TCL),
			    NCR_READ_REG(sc, ESP_DSTAT),
			    NCR_READ_REG(sc, NCR_STAT),
			    NCR_READ_REG(sc, NCR_STEP),
			    NCR_READ_REG(sc, NCR_INTR), state);
#ifdef ESP_DEBUG
			*ndtracep = '\0';
			printf("ndtrace: %s\n", ndtrace);
#endif
			panic("%s: busexc/supdate occurred."
			    "  Please email this output to chris@pin.lu.",
			    device_xname(sc->sc_dev));
#ifdef ESP_DEBUG
			ndtraceshow++;
#endif
		} else {
			nd_bsw4(DD_CSR, DMACSR_CLRCOMPLETE | DMACSR_RESET);
			if (nsc->sc_conf.nd_shutdown_cb)
				(*nsc->sc_conf.nd_shutdown_cb)(nsc->sc_conf.nd_cb_arg);
		}
	}
	return 1;
}

/* Internal DMA callback routines */
bus_dmamap_t
esp_dmacb_continue(void *arg)
{
	struct ncr53c9x_softc *sc = arg;
	struct esp_softc *esc = (struct esp_softc *)sc;

	NDTRACEIF (*ndtracep++ = 'x');
	DPRINTF(("%s: DMA continue\n",sc->sc_dev.dv_xname));

#ifdef DIAGNOSTIC
	if ((esc->sc_datain < 0) || (esc->sc_datain > 1)) {
		panic("%s: map not loaded in DMA continue callback,"
		    " datain = %d",
		    device_xname(sc->sc_dev), esc->sc_datain);
	}
#endif

	if (((esc->sc_loaded & ESP_LOADED_MAIN) == 0) && 
	    (esc->sc_main_dmamap->dm_mapsize)) {
		DPRINTF(("%s: Loading main map\n", device_xname(sc->sc_dev)));
#if 0
		bus_dmamap_sync(esc->sc_dma->sc_dmat, esc->sc_main_dmamap,
		    0, esc->sc_main_dmamap->dm_mapsize, 
		    (esc->sc_datain ?
		     BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE));
		    esc->sc_main_dmamap->dm_xfer_len = 0;
#endif
		esc->sc_loaded |= ESP_LOADED_MAIN;
		return esc->sc_main_dmamap;
	}

	if (((esc->sc_loaded & ESP_LOADED_TAIL) == 0) && 
	    (esc->sc_tail_dmamap->dm_mapsize)) {
		DPRINTF(("%s: Loading tail map\n", device_xname(sc->sc_dev)));
#if 0
		bus_dmamap_sync(esc->sc_dma->sc_dmat, esc->sc_tail_dmamap,
		    0, esc->sc_tail_dmamap->dm_mapsize, 
		    (esc->sc_datain ?
		     BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE));
		esc->sc_tail_dmamap->dm_xfer_len = 0;
#endif
		esc->sc_loaded |= ESP_LOADED_TAIL;
		return esc->sc_tail_dmamap;
	}

	DPRINTF(("%s: not loading map\n", device_xname(sc->sc_dev)));
	return 0;
}


void
esp_dmacb_completed(bus_dmamap_t map, void *arg)
{
	struct ncr53c9x_softc *sc = (struct ncr53c9x_softc *)arg;
	struct esp_softc *esc = (struct esp_softc *)sc;

	NDTRACEIF (*ndtracep++ = 'X');
	DPRINTF(("%s: DMA completed\n", device_xname(sc->sc_dev)));

#ifdef DIAGNOSTIC
	if ((esc->sc_datain < 0) || (esc->sc_datain > 1)) {
		panic("%s: invalid DMA direction in completed callback,"
		    " datain = %d",
		    device_xname(sc->sc_dev), esc->sc_datain);
	}
#endif

#if defined(DIAGNOSTIC) && 0
	{
		int i;
		for(i = 0; i < map->dm_nsegs; i++) {
			if (map->dm_xfer_len != map->dm_mapsize) {
				printf("%s: map->dm_mapsize = %d\n",
				    device_xname(sc->sc_dev), map->dm_mapsize);
				printf("%s: map->dm_nsegs = %d\n",
				    device_xname(sc->sc_dev), map->dm_nsegs);
				printf("%s: map->dm_xfer_len = %d\n",
				    device_xname(sc->sc_dev), map->dm_xfer_len);
				for(i = 0; i < map->dm_nsegs; i++) {
					printf("%s: map->dm_segs[%d].ds_addr ="
					    " 0x%08lx\n",
					    device_xname(sc->sc_dev), i,
					    map->dm_segs[i].ds_addr);
					printf("%s: map->dm_segs[%d].ds_len ="
					    " %d\n",
					    device_xname(sc->sc_dev), i,
					    map->dm_segs[i].ds_len);
				}
				panic("%s: incomplete DMA transfer",
				    device_xname(sc->sc_dev));
			}
		}
	}
#endif

	if (map == esc->sc_main_dmamap) {
#ifdef DIAGNOSTIC
		if ((esc->sc_loaded & ESP_UNLOADED_MAIN) ||
		    (esc->sc_loaded & ESP_LOADED_MAIN) == 0) {
			panic("%s: unexpected completed call for main map",
			    device_xname(sc->sc_dev));
		}
#endif
		esc->sc_loaded |= ESP_UNLOADED_MAIN;
	} else if (map == esc->sc_tail_dmamap) {
#ifdef DIAGNOSTIC
		if ((esc->sc_loaded & ESP_UNLOADED_TAIL) ||
		    (esc->sc_loaded & ESP_LOADED_TAIL) == 0) {
			panic("%s: unexpected completed call for tail map",
			    device_xname(sc->sc_dev));
		}
#endif
		esc->sc_loaded |= ESP_UNLOADED_TAIL;
	}
#ifdef DIAGNOSTIC
	 else {
		panic("%s: unexpected completed map", device_xname(sc->sc_dev));
	}
#endif

#ifdef ESP_DEBUG
	if (esp_debug) {
		if (map == esc->sc_main_dmamap) {
			printf("%s: completed main map\n",
			    device_xname(sc->sc_dev));
		} else if (map == esc->sc_tail_dmamap) {
			printf("%s: completed tail map\n",
			    device_xname(sc->sc_dev));
		}
	}
#endif

#if 0
	if ((map == esc->sc_tail_dmamap) ||
	    ((esc->sc_tail_size == 0) && (map == esc->sc_main_dmamap))) {

		/*
		 * Clear the DMAMOD bit in the DCTL register to give control
		 * back to the scsi chip.
		 */
		if (esc->sc_datain) {
			NCR_WRITE_REG(sc, ESP_DCTL,
			    ESPDCTL_16MHZ | ESPDCTL_INTENB | ESPDCTL_DMARD);
		} else {
			NCR_WRITE_REG(sc, ESP_DCTL,
			    ESPDCTL_16MHZ | ESPDCTL_INTENB);
		}
		DPRINTF(("esp dctl is 0x%02x\n", NCR_READ_REG(sc, ESP_DCTL)));
	}
#endif


#if 0
	bus_dmamap_sync(esc->sc_dma->sc_dmat, map,
	    0, map->dm_mapsize,
	    (esc->sc_datain ? BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE));
#endif

}

void
esp_dmacb_shutdown(void *arg)
{
	struct ncr53c9x_softc *sc = (struct ncr53c9x_softc *)arg;
	struct esp_softc *esc = (struct esp_softc *)sc;

	NDTRACEIF (*ndtracep++ = 'S');
	DPRINTF(("%s: DMA shutdown\n", device_xname(sc->sc_dev)));

	if (esc->sc_loaded == 0)
		return;

#if 0
	{
		/* Clear the DMAMOD bit in the DCTL register to give control
		 * back to the scsi chip.
		 */
		if (esc->sc_datain) {
			NCR_WRITE_REG(sc, ESP_DCTL,
			    ESPDCTL_16MHZ | ESPDCTL_INTENB | ESPDCTL_DMARD);
		} else {
			NCR_WRITE_REG(sc, ESP_DCTL,
			    ESPDCTL_16MHZ | ESPDCTL_INTENB);
		}
		DPRINTF(("esp dctl is 0x%02x\n", NCR_READ_REG(sc, ESP_DCTL)));
	}
#endif

	DPRINTF(("%s: esp_dma_nest == %d\n",
	    device_xname(sc->sc_dev), esp_dma_nest));

	/* Stuff the end slop into fifo */

#ifdef ESP_DEBUG
	if (esp_debug) {
		int n = NCR_READ_REG(sc, NCR_FFLAG);

		DPRINTF(("%s: fifo size = %d, seq = 0x%x\n",
		    device_xname(sc->sc_dev), n & NCRFIFO_FF,
		    (n & NCRFIFO_SS) >> 5));
	}
#endif

	if (esc->sc_main_dmamap->dm_mapsize) {
		if (!esc->sc_datain) {
			/* unpatch the DMA map for write overrun */
			esc->sc_main_dmamap->dm_mapsize -= ESP_DMA_OVERRUN;
			esc->sc_main_dmamap->dm_segs[
			    esc->sc_main_dmamap->dm_nsegs - 1].ds_len -=
			    ESP_DMA_OVERRUN;
		}
		bus_dmamap_sync(esc->sc_dma->sc_dmat, esc->sc_main_dmamap,
		    0, esc->sc_main_dmamap->dm_mapsize,
		    (esc->sc_datain ?
		     BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE));
		bus_dmamap_unload(esc->sc_dma->sc_dmat, esc->sc_main_dmamap);
		NDTRACEIF (
			sprintf(ndtracep, "m%ld",
			    esc->sc_main_dmamap->dm_xfer_len);
			ndtracep += strlen(ndtracep);
		);
	}

	if (esc->sc_tail_dmamap->dm_mapsize) {
		bus_dmamap_sync(esc->sc_dma->sc_dmat, esc->sc_tail_dmamap,
		    0, esc->sc_tail_dmamap->dm_mapsize,
		    (esc->sc_datain ?
		     BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE));
		bus_dmamap_unload(esc->sc_dma->sc_dmat, esc->sc_tail_dmamap);
		/* copy the tail DMA buffer data for read transfers */
		if (esc->sc_datain) {
			memcpy(*esc->sc_dmaaddr + esc->sc_begin_size +
			    esc->sc_main_size, esc->sc_tail,
			    esc->sc_dmasize -
			    (esc->sc_begin_size + esc->sc_main_size));
		}
		NDTRACEIF (
			sprintf(ndtracep, "t%ld",
			    esc->sc_tail_dmamap->dm_xfer_len);
			ndtracep += strlen(ndtracep);
		);
	}

#ifdef ESP_DEBUG
	if (esp_debug) {
		printf("%s: dma_shutdown: addr=%p,len=0x%08x,size=0x%08x\n",
		    device_xname(sc->sc_dev),
		    *esc->sc_dmaaddr, *esc->sc_dmalen, esc->sc_dmasize);
		if (esp_debug > 10) {
			esp_hex_dump(*(esc->sc_dmaaddr), esc->sc_dmasize);
			printf("%s: tail=%p,tailbuf=%p,tail_size=0x%08x\n",
			    device_xname(sc->sc_dev),
			    esc->sc_tail, &(esc->sc_tailbuf[0]),
			    esc->sc_tail_size);
			esp_hex_dump(&(esc->sc_tailbuf[0]),
			    sizeof(esc->sc_tailbuf));
		}
	}
#endif

	esc->sc_main = 0;
	esc->sc_main_size = 0;
	esc->sc_tail = 0;
	esc->sc_tail_size = 0;

	esc->sc_datain = -1;
/* 	esc->sc_dmaaddr = 0; */
/* 	esc->sc_dmalen  = 0; */
/* 	esc->sc_dmasize = 0; */

	esc->sc_loaded = 0;

	esc->sc_begin = 0;
	esc->sc_begin_size = 0;

#ifdef ESP_DEBUG
	if (esp_debug) {
		char sbuf[256];

		snprintb(sbuf, sizeof(sbuf), NEXT_INTR_BITS, 
		    (*(volatile u_long *)IIOV(NEXT_P_INTRSTAT)));
		printf("  *intrstat = 0x%s\n", sbuf);

		snprintb(sbuf, sizeof(sbuf), NEXT_INTR_BITS, 
		    (*(volatile u_long *)IIOV(NEXT_P_INTRMASK)));
		printf("  *intrmask = 0x%s\n", sbuf);
	}
#endif
}
