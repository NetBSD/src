/*	$NetBSD: esp.c,v 1.29 2001/04/25 17:53:14 bouyer Exp $	*/

/*
 * Copyright (c) 1997 Jason R. Thorpe.
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
 *	This product includes software developed for the NetBSD Project
 *	by Jason R. Thorpe.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
 * Initial m68k mac support from Allen Briggs <briggs@macbsd.com>
 * (basically consisting of the match, a bit of the attach, and the
 *  "DMA" glue functions).
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/queue.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#include <dev/scsipi/scsi_message.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/param.h>

#include <dev/ic/ncr53c9xreg.h>
#include <dev/ic/ncr53c9xvar.h>

#include <machine/viareg.h>

#include <mac68k/obio/espvar.h>
#include <mac68k/obio/obiovar.h>

void	espattach	__P((struct device *, struct device *, void *));
int	espmatch	__P((struct device *, struct cfdata *, void *));

/* Linkup to the rest of the kernel */
struct cfattach esp_ca = {
	sizeof(struct esp_softc), espmatch, espattach
};

/*
 * Functions and the switch for the MI code.
 */
u_char	esp_read_reg __P((struct ncr53c9x_softc *, int));
void	esp_write_reg __P((struct ncr53c9x_softc *, int, u_char));
int	esp_dma_isintr __P((struct ncr53c9x_softc *));
void	esp_dma_reset __P((struct ncr53c9x_softc *));
int	esp_dma_intr __P((struct ncr53c9x_softc *));
int	esp_dma_setup __P((struct ncr53c9x_softc *, caddr_t *,
	    size_t *, int, size_t *));
void	esp_dma_go __P((struct ncr53c9x_softc *));
void	esp_dma_stop __P((struct ncr53c9x_softc *));
int	esp_dma_isactive __P((struct ncr53c9x_softc *));
void	esp_quick_write_reg __P((struct ncr53c9x_softc *, int, u_char));
int	esp_quick_dma_intr __P((struct ncr53c9x_softc *));
int	esp_quick_dma_setup __P((struct ncr53c9x_softc *, caddr_t *,
	    size_t *, int, size_t *));
void	esp_quick_dma_go __P((struct ncr53c9x_softc *));

void	esp_intr __P((void *sc));
void	esp_dualbus_intr __P((void *sc));
static struct esp_softc		*esp0 = NULL, *esp1 = NULL;

static __inline__ int esp_dafb_have_dreq __P((struct esp_softc *esc));
static __inline__ int esp_iosb_have_dreq __P((struct esp_softc *esc));
int (*esp_have_dreq) __P((struct esp_softc *esc));

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

int
espmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	int	found = 0;

	if ((cf->cf_unit == 0) && mac68k_machine.scsi96) {
		found = 1;
	}
	if ((cf->cf_unit == 1) && mac68k_machine.scsi96_2) {
		found = 1;
	}

	return found;
}

/*
 * Attach this instance, and then all the sub-devices
 */
void
espattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct obio_attach_args *oa = (struct obio_attach_args *)aux;
	extern vaddr_t		SCSIBase;
	struct esp_softc	*esc = (void *)self;
	struct ncr53c9x_softc	*sc = &esc->sc_ncr53c9x;
	int			quick = 0;
	unsigned long		reg_offset;

	reg_offset = SCSIBase - IOBase;
	esc->sc_tag = oa->oa_tag;
	/*
	 * For Wombat, Primus and Optimus motherboards, DREQ is
	 * visible on bit 0 of the IOSB's emulated VIA2 vIFR (and
	 * the scsi registers are offset 0x1000 bytes from IOBase).
	 *
	 * For the Q700/900/950 it's at f9800024 for bus 0 and
	 * f9800028 for bus 1 (900/950).  For these machines, that is also
	 * a (12-bit) configuration register for DAFB's control of the
	 * pseudo-DMA timing.  The default value is 0x1d1.
	 */
	esp_have_dreq = esp_dafb_have_dreq;
	if (sc->sc_dev.dv_unit == 0) {
		if (reg_offset == 0x10000) {
			quick = 1;
			esp_have_dreq = esp_iosb_have_dreq;
		} else if (reg_offset == 0x18000) {
			quick = 0;
		} else {
			if (bus_space_map(esc->sc_tag, 0xf9800024,
					  4, 0, &esc->sc_bsh)) {
				printf("failed to map 4 at 0xf9800024.\n");
			} else {
				quick = 1;
				bus_space_write_4(esc->sc_tag,
						  esc->sc_bsh, 0, 0x1d1);
			}
		}
	} else {
		if (bus_space_map(esc->sc_tag, 0xf9800028,
				  4, 0, &esc->sc_bsh)) {
			printf("failed to map 4 at 0xf9800028.\n");
		} else {
			quick = 1;
			bus_space_write_4(esc->sc_tag, esc->sc_bsh, 0, 0x1d1);
		}
	}
	if (quick) {
		esp_glue.gl_write_reg = esp_quick_write_reg;
		esp_glue.gl_dma_intr = esp_quick_dma_intr;
		esp_glue.gl_dma_setup = esp_quick_dma_setup;
		esp_glue.gl_dma_go = esp_quick_dma_go;
	}

	/*
	 * Set up the glue for MI code early; we use some of it here.
	 */
	sc->sc_glue = &esp_glue;

	/*
	 * Save the regs
	 */
	if (sc->sc_dev.dv_unit == 0) {
		esp0 = esc;

		esc->sc_reg = (volatile u_char *) SCSIBase;
		via2_register_irq(VIA2_SCSIIRQ, esp_intr, esc);
		esc->irq_mask = V2IF_SCSIIRQ;
		if (reg_offset == 0x10000) {
			/* From the Q650 developer's note */
			sc->sc_freq = 16500000;
		} else {
			sc->sc_freq = 25000000;
		}

		if (esp_glue.gl_dma_go == esp_quick_dma_go) {
			printf(" (quick)");
		}
	} else {
		esp1 = esc;

		esc->sc_reg = (volatile u_char *) SCSIBase + 0x402;
		via2_register_irq(VIA2_SCSIIRQ, esp_dualbus_intr, NULL);
		esc->irq_mask = 0;
		sc->sc_freq = 25000000;

		if (esp_glue.gl_dma_go == esp_quick_dma_go) {
			printf(" (quick)");
		}
	}

	printf(": address %p", esc->sc_reg);

	sc->sc_id = 7;

	/* gimme Mhz */
	sc->sc_freq /= 1000000;

	/*
	 * It is necessary to try to load the 2nd config register here,
	 * to find out what rev the esp chip is, else the esp_reset
	 * will not set up the defaults correctly.
	 */
	sc->sc_cfg1 = sc->sc_id; /* | NCRCFG1_PARENB; */
	sc->sc_cfg2 = NCRCFG2_SCSI2;
	sc->sc_cfg3 = 0;
	sc->sc_rev = NCR_VARIANT_NCR53C96;

	/*
	 * This is the value used to start sync negotiations
	 * Note that the NCR register "SYNCTP" is programmed
	 * in "clocks per byte", and has a minimum value of 4.
	 * The SCSI period used in negotiation is one-fourth
	 * of the time (in nanoseconds) needed to transfer one byte.
	 * Since the chip's clock is given in MHz, we have the following
	 * formula: 4 * period = (1000 / freq) * 4
	 */
	sc->sc_minsync = 1000 / sc->sc_freq;

	/* We need this to fit into the TCR... */
	sc->sc_maxxfer = 64 * 1024;

	if (!quick) {
		sc->sc_minsync = 0;	/* No synchronous xfers w/o DMA */
		sc->sc_maxxfer = 8 * 1024;
	}

	/*
	 * Configure interrupts.
	 */
	if (esc->irq_mask) {
		via2_reg(vPCR) = 0x22;
		via2_reg(vIFR) = esc->irq_mask;
		via2_reg(vIER) = 0x80 | esc->irq_mask;
	}

	/*
	 * Now try to attach all the sub-devices
	 */
	sc->sc_adapter.adapt_minphys = minphys;
	sc->sc_adapter.adapt_request = ncr53c9x_scsipi_request;
	ncr53c9x_attach(sc);
}

/*
 * Glue functions.
 */

u_char
esp_read_reg(sc, reg)
	struct ncr53c9x_softc *sc;
	int reg;
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	return esc->sc_reg[reg * 16];
}

void
esp_write_reg(sc, reg, val)
	struct ncr53c9x_softc *sc;
	int reg;
	u_char val;
{
	struct esp_softc *esc = (struct esp_softc *)sc;
	u_char	v = val;

	if (reg == NCR_CMD && v == (NCRCMD_TRANS|NCRCMD_DMA)) {
		v = NCRCMD_TRANS;
	}
	esc->sc_reg[reg * 16] = v;
}

void
esp_dma_stop(sc)
	struct ncr53c9x_softc *sc;
{
}

int
esp_dma_isactive(sc)
	struct ncr53c9x_softc *sc;
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	return esc->sc_active;
}

int
esp_dma_isintr(sc)
	struct ncr53c9x_softc *sc;
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	return esc->sc_reg[NCR_STAT * 16] & 0x80;
}

void
esp_dma_reset(sc)
	struct ncr53c9x_softc *sc;
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	esc->sc_active = 0;
	esc->sc_tc = 0;
}

int
esp_dma_intr(sc)
	struct ncr53c9x_softc *sc;
{
	struct esp_softc *esc = (struct esp_softc *)sc;
	volatile u_char *cmdreg, *intrreg, *statreg, *fiforeg;
	u_char	*p;
	u_int	espphase, espstat, espintr;
	int	cnt, s;

	if (esc->sc_active == 0) {
		printf("dma_intr--inactive DMA\n");
		return -1;
	}

	if ((sc->sc_espintr & NCRINTR_BS) == 0) {
		esc->sc_active = 0;
		return 0;
	}

	cnt = esc->sc_dmasize;
	if (esc->sc_dmasize == 0) {
		printf("data interrupt, but no count left.");
	}

	p = *esc->sc_dmaaddr;
	espphase = sc->sc_phase;
	espstat = (u_int) sc->sc_espstat;
	espintr = (u_int) sc->sc_espintr;
	cmdreg = esc->sc_reg + NCR_CMD * 16;
	fiforeg = esc->sc_reg + NCR_FIFO * 16;
	statreg = esc->sc_reg + NCR_STAT * 16;
	intrreg = esc->sc_reg + NCR_INTR * 16;
	do {
		if (esc->sc_datain) {
			*p++ = *fiforeg;
			cnt--;
			if (espphase == DATA_IN_PHASE) {
				*cmdreg = NCRCMD_TRANS;
			} else {
				esc->sc_active = 0;
			}
	 	} else {
			if (   (espphase == DATA_OUT_PHASE)
			    || (espphase == MESSAGE_OUT_PHASE)) {
				*fiforeg = *p++;
				cnt--;
				*cmdreg = NCRCMD_TRANS;
			} else {
				esc->sc_active = 0;
			}
		}

		if (esc->sc_active) {
			while (!(*statreg & 0x80));
			s = splhigh();
			espstat = *statreg;
			espintr = *intrreg;
			espphase = (espintr & NCRINTR_DIS)
				    ? /* Disconnected */ BUSFREE_PHASE
				    : espstat & PHASE_MASK;
			splx(s);
		}
	} while (esc->sc_active && (espintr & NCRINTR_BS));
	sc->sc_phase = espphase;
	sc->sc_espstat = (u_char) espstat;
	sc->sc_espintr = (u_char) espintr;
	*esc->sc_dmaaddr = p;
	esc->sc_dmasize = cnt;

	if (esc->sc_dmasize == 0) {
		esc->sc_tc = NCRSTAT_TC;
	}
	sc->sc_espstat |= esc->sc_tc;
	return 0;
}

int
esp_dma_setup(sc, addr, len, datain, dmasize)
	struct ncr53c9x_softc *sc;
	caddr_t *addr;
	size_t *len;
	int datain;
	size_t *dmasize;
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	esc->sc_dmaaddr = addr;
	esc->sc_dmalen = len;
	esc->sc_datain = datain;
	esc->sc_dmasize = *dmasize;
	esc->sc_tc = 0;

	return 0;
}

void
esp_dma_go(sc)
	struct ncr53c9x_softc *sc;
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	if (esc->sc_datain == 0) {
		esc->sc_reg[NCR_FIFO * 16] = **esc->sc_dmaaddr;
		(*esc->sc_dmalen)--;
		(*esc->sc_dmaaddr)++;
	}
	esc->sc_active = 1;
}

void
esp_quick_write_reg(sc, reg, val)
	struct ncr53c9x_softc *sc;
	int reg;
	u_char val;
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	esc->sc_reg[reg * 16] = val;
}

#if DEBUG
int mac68k_esp_debug=0;
#endif

int
esp_quick_dma_intr(sc)
	struct ncr53c9x_softc *sc;
{
	struct esp_softc *esc = (struct esp_softc *)sc;
	int trans=0, resid=0;

	if (esc->sc_active == 0)
		panic("dma_intr--inactive DMA\n");

	esc->sc_active = 0;

	if (esc->sc_dmasize == 0) {
		int	res;

		res = NCR_READ_REG(sc, NCR_TCL);
		res += NCR_READ_REG(sc, NCR_TCM) << 8;
		/* This can happen in the case of a TRPAD operation */
		/* Pretend that it was complete */
		sc->sc_espstat |= NCRSTAT_TC;
#if DEBUG
		if (mac68k_esp_debug) {
			printf("dmaintr: DMA xfer of zero xferred %d\n",
			    65536 - res);
		}
#endif
		return 0;
	}

	if ((sc->sc_espstat & NCRSTAT_TC) == 0) {
		if (esc->sc_datain == 0) {
			resid = NCR_READ_REG(sc, NCR_FFLAG) & 0x1f;
#if DEBUG
			if (mac68k_esp_debug) {
				printf("Write FIFO residual %d bytes\n", resid);
			}
#endif
		}
		resid += NCR_READ_REG(sc, NCR_TCL);
		resid += NCR_READ_REG(sc, NCR_TCM) << 8;
		if (resid == 0)
			resid = 65536;
	}

	trans = esc->sc_dmasize - resid;
	if (trans < 0) {
		printf("dmaintr: trans < 0????");
		trans = *esc->sc_dmalen;
	}

	NCR_DMA(("dmaintr: trans %d, resid %d.\n", trans, resid));
#if DEBUG
	if (mac68k_esp_debug) {
		printf("eqd_intr: trans %d, resid %d.\n", trans, resid);
	}
#endif
	*esc->sc_dmaaddr += trans;
	*esc->sc_dmalen -= trans;

	return 0;
}

int
esp_quick_dma_setup(sc, addr, len, datain, dmasize)
	struct ncr53c9x_softc *sc;
	caddr_t *addr;
	size_t *len;
	int datain;
	size_t *dmasize;
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	esc->sc_dmaaddr = addr;
	esc->sc_dmalen = len;

	if (*len & 1) {
		esc->sc_pad = 1;
	} else {
		esc->sc_pad = 0;
	}

	esc->sc_datain = datain;
	esc->sc_dmasize = *dmasize;

#if DIAGNOSTIC
	if (esc->sc_dmasize == 0) {
		/* This can happen in the case of a TRPAD operation */
	}
#endif
#if DEBUG
	if (mac68k_esp_debug) {
	printf("eqd_setup: addr %lx, len %lx, in? %d, dmasize %lx\n",
	    (long) *addr, (long) *len, datain, (long) esc->sc_dmasize);
	}
#endif

	return 0;
}

static __inline__ int
esp_dafb_have_dreq(esc)
	struct esp_softc *esc;
{
	return (*(volatile u_int32_t *)(esc->sc_bsh.base) & 0x200);
}

static __inline__ int
esp_iosb_have_dreq(esc)
	struct esp_softc *esc;
{
	return (via2_reg(vIFR) & V2IF_SCSIDRQ);
}

static volatile int espspl=-1;

/*
 * Apple "DMA" is weird.
 *
 * Basically, the CPU acts like the DMA controller.  The DREQ/ off the
 * chip goes to a register that we've mapped at attach time (on the
 * IOSB or DAFB, depending on the machine).  Apple also provides some
 * space for which the memory controller handshakes data to/from the
 * NCR chip with the DACK/ line.  This space appears to be mapped over
 * and over, every 4 bytes, but only the lower 16 bits are valid (but
 * reading the upper 16 bits will handshake DACK/ just fine, so if you
 * read *u_int16_t++ = *u_int16_t++ in a loop, you'll get
 * <databyte><databyte>0xff0xff<databyte><databyte>0xff0xff...
 *
 * When you're attempting to read or write memory to this DACK/ed space,
 * and the NCR is not ready for some timeout period, the system will
 * generate a bus error.  This might be for one of several reasons:
 *
 *	1) (on write) The FIFO is full and is not draining.
 *	2) (on read) The FIFO is empty and is not filling.
 *	3) An interrupt condition has occurred.
 *	4) Anything else?
 *
 * So if a bus error occurs, we first turn off the nofault bus error handler,
 * then we check for an interrupt (which would render the first two
 * possibilities moot).  If there's no interrupt, check for a DREQ/.  If we
 * have that, then attempt to resume stuffing (or unstuffing) the FIFO.  If
 * neither condition holds, pause briefly and check again.
 *
 * NOTE!!!  In order to make allowances for the hardware structure of
 *          the mac, spl values in here are hardcoded!!!!!!!!!
 *          This is done to allow serial interrupts to get in during
 *          scsi transfers.  This is ugly.
 */
void
esp_quick_dma_go(sc)
	struct ncr53c9x_softc *sc;
{
	struct esp_softc *esc = (struct esp_softc *)sc;
	extern long mac68k_a2_fromfault;
	extern int *nofault;
	label_t faultbuf;
	u_int16_t volatile *pdma;
	u_int16_t *addr;
	int		len, res;
	u_short		cnt32, cnt2;
	u_char volatile *statreg;

	esc->sc_active = 1;

	espspl = splhigh();

	addr = (u_int16_t *) *esc->sc_dmaaddr;
	len  = esc->sc_dmasize;

restart_dmago:
#if DEBUG
	if (mac68k_esp_debug) {
		printf("eqdg: a %lx, l %lx, in? %d ... ",
		    (long) addr, (long) len, esc->sc_datain);
	}
#endif
	nofault = (int *) &faultbuf;
	if (setjmp((label_t *) nofault)) {
		int	i=0;

		nofault = (int *) 0;
#if DEBUG
		if (mac68k_esp_debug) {
			printf("be\n");
		}
#endif
		/*
		 * Bus error...
		 * So, we first check for an interrupt.  If we have
		 * one, go handle it.  Next we check for DREQ/.  If
		 * we have it, then we restart the transfer.  If
		 * neither, then loop until we get one or the other.
		 */
		statreg = esc->sc_reg + NCR_STAT * 16;
		for (;;) {
			spl2();		/* Give serial a chance... */
			splhigh();	/* That's enough... */

			if (*statreg & 0x80) {
				goto gotintr;
			}

			if (esp_have_dreq(esc)) {
				/*
				 * Get the remaining length from the address
				 * differential.
				 */
				addr = (u_int16_t *) mac68k_a2_fromfault;
				len = esc->sc_dmasize -
				    ((long) addr - (long) *esc->sc_dmaaddr);

				if (esc->sc_datain == 0) {
					/*
					 * Let the FIFO drain before we read
					 * the transfer count.
					 * Do we need to do this?
					 * Can we do this?
					 */
					while (NCR_READ_REG(sc, NCR_FFLAG)
					    & 0x1f);
					/*
					 * Get the length from the transfer
					 * counters.
					 */
					res = NCR_READ_REG(sc, NCR_TCL);
					res += NCR_READ_REG(sc, NCR_TCM) << 8;
					/*
					 * If they don't agree,
					 * adjust accordingly.
					 */
					while (res > len) {
						len+=2; addr--;
					}
					if (res != len) {
						panic("esp_quick_dma_go: res %d != len %d\n",
							res, len);
					}
				}
				break;
			}

			DELAY(1);
			if (i++ > 1000000)
				panic("esp_dma_go: Bus error, but no condition!  Argh!");
		}
		goto restart_dmago;
	}

	len &= ~1;

	statreg = esc->sc_reg + NCR_STAT * 16;
	pdma = (u_int16_t *) (esc->sc_reg + 0x100);

	/*
	 * These loops are unrolled into assembly for two reasons:
	 * 1) We can make sure that they are as efficient as possible, and
	 * 2) (more importantly) we need the address that we are reading
	 *    from or writing to to be in a2.
	 */
	cnt32 = len / 32;
	cnt2 = (len % 32) / 2;
	if (esc->sc_datain == 0) {
		/* while (cnt32--) { 16 instances of *pdma = *addr++; } */
		/* while (cnt2--) { *pdma = *addr++; } */
		__asm __volatile ("
				movl %1, %%a2
				movl %2, %%a3
				movw %3, %%d2
				cmpw #0, %%d2
				beq  2f
				subql #1, %%d2
			1:	movw %%a2@+,%%a3@; movw %%a2@+,%%a3@
				movw %%a2@+,%%a3@; movw %%a2@+,%%a3@
				movw %%a2@+,%%a3@; movw %%a2@+,%%a3@
				movw %%a2@+,%%a3@; movw %%a2@+,%%a3@
				movw %%a2@+,%%a3@; movw %%a2@+,%%a3@
				movw %%a2@+,%%a3@; movw %%a2@+,%%a3@
				movw %%a2@+,%%a3@; movw %%a2@+,%%a3@
				movw %%a2@+,%%a3@; movw %%a2@+,%%a3@
				movw #8704,%%sr
				movw #9728,%%sr
				dbra %%d2, 1b
			2:	movw %4, %%d2
				cmpw #0, %%d2
				beq  4f
				subql #1, %%d2
			3:	movw %%a2@+,%%a3@
				dbra %%d2, 3b
			4:	movl %%a2, %0"
			: "=g" (addr)
			: "0" (addr), "g" (pdma), "g" (cnt32), "g" (cnt2)
			: "a2", "a3", "d2");
		if (esc->sc_pad) {
			unsigned char	*c;
			c = (unsigned char *) addr;
			/* Wait for DREQ */
			while (!esp_have_dreq(esc)) {
				if (*statreg & 0x80) {
					nofault = (int *) 0;
					goto gotintr;
				}
			}
			*(unsigned char *)pdma = *c;
		}
	} else {
		/* while (cnt32--) { 16 instances of *addr++ = *pdma; } */
		/* while (cnt2--) { *addr++ = *pdma; } */
		__asm __volatile ("
				movl %1, %%a2
				movl %2, %%a3
				movw %3, %%d2
				cmpw #0, %%d2
				beq  6f
				subql #1, %%d2
			5:	movw %%a3@,%%a2@+; movw %%a3@,%%a2@+
				movw %%a3@,%%a2@+; movw %%a3@,%%a2@+
				movw %%a3@,%%a2@+; movw %%a3@,%%a2@+
				movw %%a3@,%%a2@+; movw %%a3@,%%a2@+
				movw %%a3@,%%a2@+; movw %%a3@,%%a2@+
				movw %%a3@,%%a2@+; movw %%a3@,%%a2@+
				movw %%a3@,%%a2@+; movw %%a3@,%%a2@+
				movw %%a3@,%%a2@+; movw %%a3@,%%a2@+
				movw #8704,%%sr
				movw #9728,%%sr
				dbra %%d2, 5b
			6:	movw %4, %%d2
				cmpw #0, %%d2
				beq  8f
				subql #1, %%d2
			7:	movw %%a3@,%%a2@+
				dbra %%d2, 7b
			8:	movl %%a2, %0"
			: "=g" (addr)
			: "0" (addr), "g" (pdma), "g" (cnt32), "g" (cnt2)
			: "a2", "a3", "d2");
		if (esc->sc_pad) {
			unsigned char	*c;
			c = (unsigned char *) addr;
			/* Wait for DREQ */
			while (!esp_have_dreq(esc)) {
				if (*statreg & 0x80) {
					nofault = (int *) 0;
					goto gotintr;
				}
			}
			*c = *(unsigned char *)pdma;
		}
	}

	nofault = (int *) 0;

	/*
	 * If we have not received an interrupt yet, we should shortly,
	 * and we can't prevent it, so return and wait for it.
	 */
	if ((*statreg & 0x80) == 0) {
#if DEBUG
		if (mac68k_esp_debug) {
			printf("g.\n");
		}
#endif
		if (espspl != -1) splx(espspl); espspl = -1;
		return;
	}

gotintr:
#if DEBUG
	if (mac68k_esp_debug) {
		printf("g!\n");
	}
#endif
	ncr53c9x_intr(sc);
	if (espspl != -1) splx(espspl); espspl = -1;
}

void
esp_intr(sc)
	void *sc;
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	if (esc->sc_reg[NCR_STAT * 16] & 0x80) {
		ncr53c9x_intr((struct ncr53c9x_softc *) esp0);
	}
}

void
esp_dualbus_intr(sc)
	void *sc;
{
	if (esp0 && (esp0->sc_reg[NCR_STAT * 16] & 0x80)) {
		ncr53c9x_intr((struct ncr53c9x_softc *) esp0);
	}

	if (esp1 && (esp1->sc_reg[NCR_STAT * 16] & 0x80)) {
		ncr53c9x_intr((struct ncr53c9x_softc *) esp1);
	}
}
