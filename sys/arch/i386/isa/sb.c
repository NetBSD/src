/*	$NetBSD: sb.c,v 1.18 1995/02/28 21:47:42 brezak Exp $	*/

/*
 * Copyright (c) 1991-1993 Regents of the University of California.
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
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
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
 *
 *	$Id: sb.c,v 1.18 1995/02/28 21:47:42 brezak Exp $
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <machine/cpu.h>
#include <machine/pio.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>

#include <i386/isa/isavar.h>
#include <i386/isa/dmavar.h>
#include <i386/isa/icu.h>

#include <i386/isa/sbdspvar.h>
#include <i386/isa/sbreg.h>

#define DEBUG	/*XXX*/
#ifdef DEBUG
#define DPRINTF(x)	if (sbdebug) printf x
int	sbdebug = 0;
#else
#define DPRINTF(x)
#endif

struct sb_softc {
	struct	device sc_dev;		/* base device */
	struct	isadev sc_id;		/* ISA device */
	struct	intrhand sc_ih;		/* interrupt vectoring */

	struct sbdsp_softc sc_sbdsp;
};

int	sbprobe();
void	sbattach __P((struct device *, struct device *, void *));

struct cfdriver sbcd = {
	NULL, "sb", sbprobe, sbattach, DV_DULL, sizeof(struct sb_softc)
};

struct audio_device sb_device = {
	"SoundBlaster",
	"x",
	"sb"
};

int	sbopen __P((dev_t, int));

int	sbprobe();
void	sbattach();

int	sb_getdev __P((caddr_t, struct audio_device *));

/*
 * Define our interface to the higher level audio driver.
 */

struct audio_hw_if sb_hw_if = {
	sbopen,
	sbdsp_close,
	NULL,
	sbdsp_set_in_sr,
	sbdsp_get_in_sr,
	sbdsp_set_out_sr,
	sbdsp_get_out_sr,
	sbdsp_query_encoding,
	sbdsp_set_encoding,
	sbdsp_get_encoding,
	sbdsp_set_precision,
	sbdsp_get_precision,
	sbdsp_set_channels,
	sbdsp_get_channels,
	sbdsp_round_blocksize,
	sbdsp_set_out_port,
	sbdsp_get_out_port,
	sbdsp_set_in_port,
	sbdsp_get_in_port,
	sbdsp_commit_settings,
	sbdsp_get_silence,
	sbdsp_expand,
	sbdsp_compress,
	sbdsp_dma_output,
	sbdsp_dma_input,
	sbdsp_haltdma,
	sbdsp_haltdma,
	sbdsp_contdma,
	sbdsp_contdma,
	sbdsp_speaker_ctl,
	sb_getdev,
	sbdsp_setfd,
	sbdsp_mixer_set_port,
	sbdsp_mixer_get_port,
	sbdsp_mixer_query_devinfo,
	0,	/* not full-duplex */
	0
};

/*
 * Probe / attach routines.
 */

/*
 * Probe for the soundblaster hardware.
 */
int
sbprobe(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	register struct sb_softc *sc = (void *)self;
	register struct isa_attach_args *ia = aux;
	register u_short iobase = ia->ia_iobase;

	if (!SB_BASE_VALID(ia->ia_iobase)) {
		printf("sb: configured iobase %d invalid\n", ia->ia_iobase);
		return 0;
	}
	sc->sc_sbdsp.sc_iobase = iobase;
	if (sbdsp_probe(&sc->sc_sbdsp) == 0) {
		DPRINTF(("sb: sbdsp probe failed\n"));
		return 0;
	}
		
	/*
	 * Cannot auto-discover DMA channel.
	 */
	if (ISSBPROCLASS(&sc->sc_sbdsp)) {
		if (!SBP_DRQ_VALID(ia->ia_drq)) {
			printf("sb: configured dma chan %d invalid\n", ia->ia_drq);
			return 0;
		}
	}
	else {
		if (!SB_DRQ_VALID(ia->ia_drq)) {
			printf("sb: configured dma chan %d invalid\n", ia->ia_drq);
			return 0;
		}
	}
	
#ifdef NEWCONFIG
	/*
	 * If the IRQ wasn't compiled in, auto-detect it.
	 */
	if (ia->ia_irq == IRQUNK) {
		ia->ia_irq = isa_discoverintr(sbforceintr, aux);
		sbdsp_reset(&sc->sc_sbdsp);
		if (ISSBPROCLASS(&sc->sc_sbdsp)) {
			if (!SBP_IRQ_VALID(ia->ia_irq)) {
				printf("sb: couldn't auto-detect interrupt");
				return 0;
			}
		}
		else {
			if (!SB_IRQ_VALID(ia->ia_irq)) {
				printf("sb: couldn't auto-detect interrupt");
				return 0;
			}
		}
	} else
#endif
	if (ISSBPROCLASS(&sc->sc_sbdsp)) {
		if (!SBP_IRQ_VALID(ia->ia_irq)) {
			printf("sb: configured irq %d invalid\n", ia->ia_irq);
			return 0;
		}
	}
	else {
		if (!SB_IRQ_VALID(ia->ia_irq)) {
			printf("sb: configured irq %d invalid\n", ia->ia_irq);
			return 0;
		}
	}

	sc->sc_sbdsp.sc_irq = ia->ia_irq;
	sc->sc_sbdsp.sc_drq = ia->ia_drq;
	
	if (ISSBPROCLASS(&sc->sc_sbdsp))
		ia->ia_iosize = SBP_NPORT;
	else
		ia->ia_iosize = SB_NPORT;
	return 1;
}

#ifdef NEWCONFIG
void
sbforceintr(aux)
	void *aux;
{
	static char dmabuf;
	struct isa_attach_args *ia = aux;
	u_short iobase = ia->ia_iobase;

	/*
	 * Set up a DMA read of one byte.
	 * XXX Note that at this point we haven't called 
	 * at_setup_dmachan().  This is okay because it just
	 * allocates a buffer in case it needs to make a copy,
	 * and it won't need to make a copy for a 1 byte buffer.
	 * (I think that calling at_setup_dmachan() should be optional;
	 * if you don't call it, it will be called the first time
	 * it is needed (and you pay the latency).  Also, you might
	 * never need the buffer anyway.)
	 */
	at_dma(B_READ, &dmabuf, 1, ia->ia_drq);
	if (sbdsp_wdsp(iobase, SB_DSP_RDMA) == 0) {
		(void)sbdsp_wdsp(iobase, 0);
		(void)sbdsp_wdsp(iobase, 0);
	}
}
#endif

/*
 * Attach hardware to driver, attach hardware driver to audio
 * pseudo-device driver .
 */
void
sbattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	register struct sb_softc *sc = (struct sb_softc *)self;
	struct isa_attach_args *ia = (struct isa_attach_args *)aux;
	register u_short iobase = ia->ia_iobase;

#ifdef NEWCONFIG
	isa_establish(&sc->sc_id, &sc->sc_dev);
#endif
	sc->sc_ih.ih_fun = sbdsp_intr;
	sc->sc_ih.ih_arg = &sc->sc_sbdsp;
	sc->sc_ih.ih_level = IPL_BIO;
	intr_establish(ia->ia_irq, IST_EDGE, &sc->sc_ih);

	sbdsp_attach(&sc->sc_sbdsp);

	if (audio_hardware_attach(&sb_hw_if, (caddr_t)&sc->sc_sbdsp) != 0)
		printf("sb: could not attach to audio pseudo-device driver\n");
}

/*
 * Various routines to interface to higher level audio driver
 */

int
sbopen(dev, flags)
    dev_t dev;
    int flags;
{
    struct sb_softc *sc;
    int unit = AUDIOUNIT(dev);
    
    if (unit >= sbcd.cd_ndevs)
	return ENODEV;
    
    sc = sbcd.cd_devs[unit];
    if (!sc)
	return ENXIO;
    
    return sbdsp_open(&sc->sc_sbdsp, dev, flags);
}

int
sb_getdev(addr, retp)
	caddr_t addr;
	struct audio_device *retp;
{
	*retp = sb_device;
	return 0;
}
