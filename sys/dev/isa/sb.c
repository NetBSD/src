/*	$NetBSD: sb.c,v 1.71 2001/10/03 00:04:51 augustss Exp $	*/

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
 */

#include "midi.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/bus.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <dev/midi_if.h>

#include <dev/midi_if.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <dev/isa/sbreg.h>
#include <dev/isa/sbvar.h>
#include <dev/isa/sbdspvar.h>

#if NMPU > 0
struct midi_hw_if sb_midi_hw_if = {
	sbdsp_midi_open,
	sbdsp_midi_close,
	sbdsp_midi_output,
	sbdsp_midi_getinfo,
	0,			/* ioctl */
};
#endif

struct audio_device sb_device = {
	"SoundBlaster",
	"x",
	"sb"
};

int	sb_getdev __P((void *, struct audio_device *));

/*
 * Define our interface to the higher level audio driver.
 */

struct audio_hw_if sb_hw_if = {
	sbdsp_open,
	sbdsp_close,
	0,
	sbdsp_query_encoding,
	sbdsp_set_params,
	sbdsp_round_blocksize,
	0,
	0,
	0,
	0,
	0,
	sbdsp_halt_output,
	sbdsp_halt_input,
	sbdsp_speaker_ctl,
	sb_getdev,
	0,
	sbdsp_mixer_set_port,
	sbdsp_mixer_get_port,
	sbdsp_mixer_query_devinfo,
	sb_malloc,
	sb_free,
	sb_round_buffersize,
        sb_mappage,
	sbdsp_get_props,
	sbdsp_trigger_output,
	sbdsp_trigger_input,
	0,
};

/*
 * Probe / attach routines.
 */


int
sbmatch(sc)
	struct sbdsp_softc *sc;
{
	static u_char drq_conf[8] = {
		0x01, 0x02, -1, 0x08, -1, 0x20, 0x40, 0x80
	};

	static u_char irq_conf[11] = {
		-1, -1, 0x01, -1, -1, 0x02, -1, 0x04, -1, 0x01, 0x08
	};

	if (sbdsp_probe(sc) == 0)
		return 0;

	/*
	 * Cannot auto-discover DMA channel.
	 */
	if (ISSBPROCLASS(sc)) {
		if (!SBP_DRQ_VALID(sc->sc_drq8)) {
			printf("%s: configured dma chan %d invalid\n",
			    sc->sc_dev.dv_xname, sc->sc_drq8);
			return 0;
		}
	} else {
		if (!SB_DRQ_VALID(sc->sc_drq8)) {
			printf("%s: configured dma chan %d invalid\n",
			    sc->sc_dev.dv_xname, sc->sc_drq8);
			return 0;
		}
	}

        if (0 <= sc->sc_drq16 && sc->sc_drq16 <= 3)
        	/* 
                 * XXX Some ViBRA16 cards seem to have two 8 bit DMA 
                 * channels.  I've no clue how to use them, so ignore
                 * one of them for now.  -- augustss@netbsd.org
                 */
        	sc->sc_drq16 = -1;

	if (ISSB16CLASS(sc)) {
		if (sc->sc_drq16 == -1)
			sc->sc_drq16 = sc->sc_drq8;
		if (!SB16_DRQ_VALID(sc->sc_drq16)) {
			printf("%s: configured dma chan %d invalid\n",
			    sc->sc_dev.dv_xname, sc->sc_drq16);
			return 0;
		}
	} else
		sc->sc_drq16 = sc->sc_drq8;
	
	if (ISSBPROCLASS(sc)) {
		if (!SBP_IRQ_VALID(sc->sc_irq)) {
			printf("%s: configured irq %d invalid\n",
			    sc->sc_dev.dv_xname, sc->sc_irq);
			return 0;
		}
	} else {
		if (!SB_IRQ_VALID(sc->sc_irq)) {
			printf("%s: configured irq %d invalid\n",
			    sc->sc_dev.dv_xname, sc->sc_irq);
			return 0;
		}
	}

	if (ISSB16CLASS(sc) && !(sc->sc_quirks & SB_QUIRK_NO_INIT_DRQ)) {
		int w, r;
#if 0
		printf("%s: old drq conf %02x\n", sc->sc_dev.dv_xname,
		    sbdsp_mix_read(sc, SBP_SET_DRQ));
		printf("%s: try drq conf %02x\n", sc->sc_dev.dv_xname,
		    drq_conf[sc->sc_drq16] | drq_conf[sc->sc_drq8]);
#endif
		w = drq_conf[sc->sc_drq16] | drq_conf[sc->sc_drq8];
		sbdsp_mix_write(sc, SBP_SET_DRQ, w);
		r = sbdsp_mix_read(sc, SBP_SET_DRQ) & 0xeb;
		if (r != w) {
			printf("%s: setting drq mask %02x failed, got %02x\n",
			    sc->sc_dev.dv_xname, w, r);
			return 0;
		}
#if 0
		printf("%s: new drq conf %02x\n", sc->sc_dev.dv_xname,
		    sbdsp_mix_read(sc, SBP_SET_DRQ));
#endif

#if 0
		printf("%s: old irq conf %02x\n", sc->sc_dev.dv_xname,
		    sbdsp_mix_read(sc, SBP_SET_IRQ));
		printf("%s: try irq conf %02x\n", sc->sc_dev.dv_xname,
		    irq_conf[sc->sc_irq]);
#endif
		w = irq_conf[sc->sc_irq];
		sbdsp_mix_write(sc, SBP_SET_IRQ, w);
		r = sbdsp_mix_read(sc, SBP_SET_IRQ) & 0x0f;
		if (r != w) {
			printf("%s: setting irq mask %02x failed, got %02x\n",
			    sc->sc_dev.dv_xname, w, r);
			return 0;
		}
#if 0
		printf("%s: new irq conf %02x\n", sc->sc_dev.dv_xname,
		    sbdsp_mix_read(sc, SBP_SET_IRQ));
#endif
	}

	return 1;
}


void
sbattach(sc)
	struct sbdsp_softc *sc;
{
	struct audio_attach_args arg;

	sbdsp_attach(sc);

	audio_attach_mi(&sb_hw_if, sc, &sc->sc_dev);

#if NMPU > 0
	if (sc->sc_hasmpu) {
		arg.type = AUDIODEV_TYPE_MPU;
		arg.hwif = 0;
		arg.hdl = 0;
		sc->sc_mpudev = config_found(&sc->sc_dev, &arg, audioprint);
	} else {
		midi_attach_mi(&sb_midi_hw_if, sc, &sc->sc_dev);
	}
#endif

	if (sc->sc_model >= SB_20) {
		arg.type = AUDIODEV_TYPE_OPL;
		arg.hwif = 0;
		arg.hdl = 0;
		(void)config_found(&sc->sc_dev, &arg, audioprint);
	}
}

/*
 * Various routines to interface to higher level audio driver
 */

int
sb_getdev(addr, retp)
	void *addr;
	struct audio_device *retp;
{
	struct sbdsp_softc *sc = addr;
	static char *names[] = SB_NAMES;
	char *config;

	if (sc->sc_model == SB_JAZZ)
		strncpy(retp->name, "MV Jazz16", sizeof(retp->name));
	else
		strncpy(retp->name, "SoundBlaster", sizeof(retp->name));
	sprintf(retp->version, "%d.%02d", 
		SBVER_MAJOR(sc->sc_version),
		SBVER_MINOR(sc->sc_version));
	if (0 <= sc->sc_model && sc->sc_model < sizeof names / sizeof names[0])
		config = names[sc->sc_model];
	else
		config = "??";
	strncpy(retp->config, config, sizeof(retp->config));
		
	return 0;
}
