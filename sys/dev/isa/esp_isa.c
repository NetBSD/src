/*	$NetBSD: esp_isa.c,v 1.2 1997/05/18 06:08:02 thorpej Exp $	*/

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
 * Copyright (c) 1995 Paul Kranenburg
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
/*
 * Copyright (c) 1994, 1996, 1997 Charles M. Hannum.  All rights reserved.
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
 *	This product includes software developed by Charles M. Hannum.
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
 * Copyright (c) 1997 Eric S. Hvozda (hvozda@netcom.com)
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
 *      This product includes software developed by Eric S. Hvozda.
 * 4. The name of Eric S. Hvozda may not be used to endorse or promote products
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/buf.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <scsi/scsi_all.h>
#include <scsi/scsiconf.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <dev/ic/ncr53c9xreg.h>
#include <dev/ic/ncr53c9xvar.h>

#include <dev/isa/espvar.h>

int	esp_isa_match __P((struct device *, void *, void *)); 
void	esp_isa_attach __P((struct device *, struct device *, void *));  

struct cfattach esp_isa_ca = {
	sizeof(struct esp_softc), esp_isa_match, esp_isa_attach
};

struct cfdriver esp_cd = {
	NULL, "esp", DV_DULL
};

struct scsi_adapter esp_switch = {
	ncr53c9x_scsi_cmd,
	minphys,		/* no max at this level; handled by DMA code */
	NULL,
	NULL,
};

struct scsi_device esp_dev = {
	NULL,			/* Use default error handler */
	NULL,			/* have a queue, served by this */
	NULL,			/* have no async handler */
	NULL,			/* Use default 'done' routine */
};

int esp_debug = 0;	/* ESP_SHOWTRAC | ESP_SHOWREGS | ESP_SHOWMISC */

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

/*
 * Look for the board
 */
int
esp_find(iot, ioh, epd)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	struct esp_probe_data *epd;
{
	u_int vers;
	u_int p1;
	u_int p2;
	u_int jmp;

	ESP_TRACE(("[esp_find] "));

	/* reset card before we probe? */

	/*
	 * Switch to the PIO regs and look for the bit pattern
	 * we expect...
	 */
	bus_space_write_1(iot, ioh, NCR_CFG4,
		NCRCFG4_CRS1 | bus_space_read_1(iot, ioh, NCR_CFG4));

#define SIG_MASK 0x87
#define REV_MASK 0x70
#define	M1	 0x02
#define	M2	 0x05
#define ISNCR	 0x80
#define ISESP406 0x40

	vers = bus_space_read_1(iot, ioh, NCR_SIGNTR);
	p1 = bus_space_read_1(iot, ioh, NCR_SIGNTR) & SIG_MASK;
	p2 = bus_space_read_1(iot, ioh, NCR_SIGNTR) & SIG_MASK;

	ESP_MISC(("%s: 0x%0x 0x%0x 0x%0x\n", epd->sc_dev.dv_xname,
	    vers, p1, p2));

	if (!((p1 == M1 && p2 == M2) || (p1 == M2 && p2 == M1)))
		return 0;

	/* Ok, what is it? */
	epd->sc_isncr = (vers & ISNCR);
	epd->sc_rev = ((vers & REV_MASK) == ISESP406) ?
	    NCR_VARIANT_ESP406 : NCR_VARIANT_FAS408;

	/* What do the jumpers tell us? */
	jmp = bus_space_read_1(iot, ioh, NCR_JMP);

	epd->sc_msize = (jmp & NCRJMP_ROMSZ) ? 0x4000 : 0x8000;
	epd->sc_parity = jmp & NCRJMP_J2;
	epd->sc_sync = jmp & NCRJMP_J4;
	epd->sc_id = (jmp & NCRJMP_J3) ? 7 : 6;
	switch (jmp & (NCRJMP_J0 | NCRJMP_J1)) {
		case NCRJMP_J0 | NCRJMP_J1:
			epd->sc_irq = 11;
			break;
		case NCRJMP_J0:
			epd->sc_irq = 10;
			break;
		case NCRJMP_J1:
			epd->sc_irq = 15;
			break;
		default:
			epd->sc_irq = 12;
			break;
	}

	bus_space_write_1(iot, ioh, NCR_CFG4,
		~NCRCFG4_CRS1 & bus_space_read_1(iot, ioh, NCR_CFG4));

	/* Try to set NCRESPCFG3_FCLK, some FAS408's don't support
	 * NCRESPCFG3_FCLK even though it is documented.  A bad
	 * batch of chips perhaps?
	 */
	bus_space_write_1(iot, ioh, NCR_ESPCFG3,
	    bus_space_read_1(iot, ioh, NCR_ESPCFG3) | NCRESPCFG3_FCLK);
	epd->sc_isfast = bus_space_read_1(iot, ioh, NCR_ESPCFG3)
	    & NCRESPCFG3_FCLK;

	return 1;
}

void
esp_init(esc, epd)
	struct esp_softc *esc;
	struct esp_probe_data *epd;
{
	struct ncr53c9x_softc *sc = &esc->sc_ncr53c9x;

	ESP_TRACE(("[esp_init] "));

	/*
	 * Set up the glue for MI code early; we use some of it here.
	 */
	sc->sc_glue = &esp_glue;

	sc->sc_rev = epd->sc_rev;
	sc->sc_id = epd->sc_id;

	/* If we could set NCRESPCFG3_FCLK earlier, we can really move */
	sc->sc_cfg3 = NCR_READ_REG(sc, NCR_ESPCFG3);
	if ((epd->sc_rev == NCR_VARIANT_FAS408) && epd->sc_isfast) {
		sc->sc_freq = 40;
		sc->sc_cfg3 |= NCRESPCFG3_FCLK;
	}
	else
		sc->sc_freq = 24;

	/* Setup the register defaults */
	sc->sc_cfg1 = sc->sc_id;
	if (epd->sc_parity)
		sc->sc_cfg1 |= NCRCFG1_PARENB;
	sc->sc_cfg2 = NCRCFG2_SCSI2;
	sc->sc_cfg3 |= NCRESPCFG3_IDM | NCRESPCFG3_FSCSI;

	/*
	 * This is the value used to start sync negotiations
	 * Note that the NCR register "SYNCTP" is programmed
	 * in "clocks per byte", and has a minimum value of 4.
	 * The SCSI period used in negotiation is one-fourth
	 * of the time (in nanoseconds) needed to transfer one byte.
	 * Since the chip's clock is given in MHz, we have the following
	 * formula: 4 * period = (1000 / freq) * 4
	 */
	if (epd->sc_sync)
	{
#ifdef DIAGNOSTIC
		printf("%s: sync requested, but not supported; will do async\n",
		    sc->sc_dev.dv_xname);
#endif
		epd->sc_sync = 0;
	}

	sc->sc_minsync = 0;

	/* Really no limit, but since we want to fit into the TCR... */
	sc->sc_maxxfer = 64 * 1024;
}

/*
 * Check the slots looking for a board we recognise
 * If we find one, note it's address (slot) and call
 * the actual probe routine to check it out.
 */
int
esp_isa_match(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct ncr53c9x_softc *sc = match;
	struct isa_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_handle_t ioh;
	struct esp_probe_data epd;
	int rv;

	ESP_TRACE(("[esp_isa_match] "));

	if (ia->ia_iobase != 0x230 && ia->ia_iobase != 0x330) {
#ifdef DIAGNOSTIC
		printf("%s: invalid iobase 0x%0x, device not configured\n",
		    sc->sc_dev.dv_xname, ia->ia_iobase);
#endif
		return 0;
	}

	if (bus_space_map(iot, ia->ia_iobase, ESP_ISA_IOSIZE, 0, &ioh)) {
#ifdef DIAGNOSTIC
		printf("%s: bus_space_map() failed!\n", sc->sc_dev.dv_xname);
#endif
		return 0;
	}

	epd.sc_dev = sc->sc_dev;
	rv = esp_find(iot, ioh, &epd);

	bus_space_unmap(iot, ioh, ESP_ISA_IOSIZE);

	if (rv) {
		if (ia->ia_irq != IRQUNK && ia->ia_irq != epd.sc_irq) {
#ifdef DIAGNOSTIC
		printf("%s: configured IRQ (%0d) does not match board IRQ (%0d), device not configured\n",
		    sc->sc_dev.dv_xname, ia->ia_irq, epd.sc_irq);
#endif
			return 0;
		}
		ia->ia_irq = epd.sc_irq;
		ia->ia_msize = 0;
		ia->ia_iosize = ESP_ISA_IOSIZE;
	}
	return (rv);
}

/*
 * Attach this instance, and then all the sub-devices
 */
void
esp_isa_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct isa_attach_args *ia = aux;
	struct esp_softc *esc = (void *)self;
	struct ncr53c9x_softc *sc = &esc->sc_ncr53c9x;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_handle_t ioh;
	struct esp_probe_data epd;
	isa_chipset_tag_t ic = ia->ia_ic;

	printf("\n");
	ESP_TRACE(("[esp_isa_attach] "));

	if (bus_space_map(iot, ia->ia_iobase, ESP_ISA_IOSIZE, 0, &ioh))
		panic("espattach: bus_space_map failed");

	epd.sc_dev = sc->sc_dev;
	if (!esp_find(iot, ioh, &epd))
		panic("espattach: esp_find failed");

	if (ia->ia_drq != DRQUNK)
		isa_dmacascade(ia->ia_drq);

	esc->sc_ih = isa_intr_establish(ic, ia->ia_irq, IST_EDGE, IPL_BIO,
	    (int (*)(void *))ncr53c9x_intr, esc);
	if (esc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	esp_init(esc, &epd);

	esc->sc_ioh = ioh;
	esc->sc_iot = iot;

	printf("%s:%ssync,%sparity\n", sc->sc_dev.dv_xname,
	    epd.sc_sync ? " " : " no ", epd.sc_parity ? " " : " no ");
	printf("%s", sc->sc_dev.dv_xname);

	/*
	 * Now try to attach all the sub-devices
	 */
	ncr53c9x_attach(sc, &esp_switch, &esp_dev);
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
	u_char v;

	v =  bus_space_read_1(esc->sc_iot, esc->sc_ioh, reg);

	ESP_REGS(("[esp_read_reg CRS%c 0x%02x=0x%02x] ",
	    (bus_space_read_1(esc->sc_iot, esc->sc_ioh, NCR_CFG4) &
	    NCRCFG4_CRS1) ? '1' : '0', reg, v));

	return v;
}

void
esp_write_reg(sc, reg, val)
	struct ncr53c9x_softc *sc;
	int reg;
	u_char val;
{
	struct esp_softc *esc = (struct esp_softc *)sc;
	u_char v = val;

	if (reg == NCR_CMD && v == (NCRCMD_TRANS|NCRCMD_DMA)) {
		v = NCRCMD_TRANS;
	}

	ESP_REGS(("[esp_write_reg CRS%c 0x%02x=0x%02x] ",
	    (bus_space_read_1(esc->sc_iot, esc->sc_ioh, NCR_CFG4) &
	    NCRCFG4_CRS1) ? '1' : '0', reg, v));

	bus_space_write_1(esc->sc_iot, esc->sc_ioh, reg, v);
}

int
esp_dma_isintr(sc)
	struct ncr53c9x_softc *sc;
{
	ESP_TRACE(("[esp_dma_isintr] "));

	return NCR_READ_REG(sc, NCR_STAT) & NCRSTAT_INT;
}

void
esp_dma_reset(sc)
	struct ncr53c9x_softc *sc;
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	ESP_TRACE(("[esp_dma_reset] "));

	esc->sc_active = 0;
	esc->sc_tc = 0;
}

int
esp_dma_intr(sc)
	struct ncr53c9x_softc *sc;
{
	struct esp_softc *esc = (struct esp_softc *)sc;
	u_char	*p;
	u_int	espphase, espstat, espintr;
	int	cnt;

	ESP_TRACE(("[esp_dma_intr] "));

	if (esc->sc_active == 0) {
		printf("%s: dma_intr--inactive DMA\n", sc->sc_dev.dv_xname);
		return -1;
	}

	if ((sc->sc_espintr & NCRINTR_BS) == 0) {
		esc->sc_active = 0;
		return 0;
	}

	cnt = *esc->sc_pdmalen;
	if (*esc->sc_pdmalen == 0) {
		printf("%s: data interrupt, but no count left\n",
		    sc->sc_dev.dv_xname);
	}

	p = *esc->sc_dmaaddr;
	espphase = sc->sc_phase;
	espstat = (u_int) sc->sc_espstat;
	espintr = (u_int) sc->sc_espintr;
	do {
		if (esc->sc_datain) {
			*p++ = NCR_READ_REG(sc, NCR_FIFO);
			cnt--;
			if (espphase == DATA_IN_PHASE) {
				NCR_WRITE_REG(sc, NCR_CMD, NCRCMD_TRANS);
			} else {
				esc->sc_active = 0;
			}
	 	} else {
			if (   (espphase == DATA_OUT_PHASE)
			    || (espphase == MESSAGE_OUT_PHASE)) {
				NCR_WRITE_REG(sc, NCR_FIFO, *p++);
				cnt--;
				NCR_WRITE_REG(sc, NCR_CMD, NCRCMD_TRANS);
			} else {
				esc->sc_active = 0;
			}
		}

		if (esc->sc_active) {
			while (!(NCR_READ_REG(sc, NCR_STAT) & 0x80));
			espstat = NCR_READ_REG(sc, NCR_STAT);
			espintr = NCR_READ_REG(sc, NCR_INTR);
			espphase = (espintr & NCRINTR_DIS)
				    ? /* Disconnected */ BUSFREE_PHASE
				    : espstat & PHASE_MASK;
		}
	} while (esc->sc_active && espintr);
	sc->sc_phase = espphase;
	sc->sc_espstat = (u_char) espstat;
	sc->sc_espintr = (u_char) espintr;
	*esc->sc_dmaaddr = p;
	*esc->sc_pdmalen = cnt;

	if (*esc->sc_pdmalen == 0) {
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

	ESP_TRACE(("[esp_dma_setup] "));

	esc->sc_dmaaddr = addr;
	esc->sc_pdmalen = len;
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

	ESP_TRACE(("[esp_dma_go] "));

	esc->sc_active = 1;
}

void
esp_dma_stop(sc)
	struct ncr53c9x_softc *sc;
{
	ESP_TRACE(("[esp_dma_stop] "));
}

int
esp_dma_isactive(sc)
	struct ncr53c9x_softc *sc;
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	ESP_TRACE(("[esp_dma_isactive] "));

	return esc->sc_active;
}
