/*	$NetBSD: lmcaudio.c,v 1.30.4.1 2001/08/03 04:11:14 lukem Exp $	*/

/*
 * Copyright (c) 1996, Danny C Tsen.
 * Copyright (c) 1996, VLSI Technology Inc. All Rights Reserved.
 * Copyright (c) 1995 Melvin Tang-Richardson
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
 *	This product includes software developed by the RiscBSD team.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 *
 */

/*
 * audio driver for lmc1982
 *
 * Interfaces with the NetBSD generic audio driver to provide SUN
 * /dev/audio (partial) compatibility.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/audioio.h>
#include <sys/errno.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

#include <dev/audio_if.h>

#include <machine/irqhandler.h>
#include <machine/vidc.h>
#include <machine/katelib.h>

#include <arm32/iomd/iomdreg.h>
#include <arm32/iomd/iomdvar.h>
#include <arm/mainbus/mainbus.h>
#include <arm32/vidc/waveform.h>
#include "lmcaudio.h"

#include <arm32/vidc/lmc1982.h>

struct audio_general {
	vm_offset_t silence;
	vm_offset_t beep;
	irqhandler_t ih;

	void (*intr) ();
	void *arg;

	vm_offset_t next_cur;
	vm_offset_t next_end;
	void (*next_intr) ();
	void *next_arg;

	int buffer;
	int in_progress;

	int open;
	int drain;
} ag;

static struct callout ag_drain_ch = CALLOUT_INITIALIZER;

struct lmcaudio_softc {
	struct device device;
	int iobase;

	int open;
};

int  lmcaudio_probe	__P((struct device *parent, struct cfdata *cf, void *aux));
void lmcaudio_attach	__P((struct device *parent, struct device *self, void *aux));
int  lmcaudio_open	__P((void *addr, int flags));
void lmcaudio_close	__P((void *addr));
int lmcaudio_drain	__P((void *addr));
void lmcaudio_timeout	__P((void *arg));

int lmcaudio_intr	__P((void *arg));
int lmcaudio_dma_program	__P((vm_offset_t cur, vm_offset_t end, void (*intr)(), void *arg));
void lmcaudio_dummy_routine	__P((void *arg));
int lmcaudio_rate	__P((int rate));
void lmcaudio_shutdown	__P((void));


struct cfattach lmcaudio_ca = {
	sizeof(struct lmcaudio_softc), lmcaudio_probe, lmcaudio_attach
};

int curr_rate = 11;

int    lmcaudio_query_encoding   __P((void *, struct audio_encoding *));
int    lmcaudio_set_params       __P((void *, int, int, struct audio_params *, struct audio_params *));
int    lmcaudio_round_blocksize  __P((void *, int));
int    lmcaudio_start_output	 __P((void *, void *, int, void (*)(), void *));
int    lmcaudio_start_input	 __P((void *, void *, int, void (*)(), void *));
int    lmcaudio_halt_output	 __P((void *));
int    lmcaudio_halt_input 	 __P((void *));
int    lmcaudio_speaker_ctl	 __P((void *, int));
int    lmcaudio_getdev		 __P((void *, struct audio_device *));
int    lmcaudio_set_port	 __P((void *, mixer_ctrl_t *));
int    lmcaudio_get_port	 __P((void *, mixer_ctrl_t *));
int    lmcaudio_query_devinfo	 __P((void *, mixer_devinfo_t *));
int    lmcaudio_get_props	 __P((void *));

struct audio_device lmcaudio_device = {
	"LMCAudio 16-bit",
	"x",
	"lmcaudio"
};

struct audio_hw_if lmcaudio_hw_if = {
	lmcaudio_open,
	lmcaudio_close,
	lmcaudio_drain,
	lmcaudio_query_encoding,
	lmcaudio_set_params,
	lmcaudio_round_blocksize,
	0,
	0,
	0,
	lmcaudio_start_output,
	lmcaudio_start_input,
	lmcaudio_halt_output,
	lmcaudio_halt_input,
	lmcaudio_speaker_ctl,
	lmcaudio_getdev,
	0,
	lmcaudio_set_port,
	lmcaudio_get_port,
	lmcaudio_query_devinfo,
	0,
	0,
	0,
	0,
	lmcaudio_get_props,
};

void
lmcaudio_beep_generate()
{
	lmcaudio_dma_program(ag.beep, ag.beep+sizeof(beep_waveform)-16,
	    lmcaudio_dummy_routine, NULL);
}


static int sdma_channel;
int
lmcaudio_probe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	int id;

	id = IOMD_ID;

	switch (id) {
	case RPC600_IOMD_ID:
		return(0);
	case ARM7500_IOC_ID:
	case ARM7500FE_IOC_ID:
#ifdef RC7500
		sdma_channel = IRQ_SDMA;
		return(1);
#else
		return(0);
#endif
	default:
		printf("lmcaudio: Unknown IOMD id=%04x", id);
		break;
	}

	return (0);
}


void
lmcaudio_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct mainbus_attach_args *mb = aux;
	struct lmcaudio_softc *sc = (void *)self;

	sc->iobase = mb->mb_iobase;

	sc->open = 0;
	ag.in_progress = 0;

	ag.next_cur = 0;
	ag.next_end = 0;
	ag.next_intr = NULL;
	ag.next_arg = NULL;

	/*
	 * Enable serial sound.  The digital serial sound interface
	 * consists of 16 bits sample on each channel.
	 */
	outl(vidc_base, VIDC_SCR | 0x03);
 
	/*
	 * Video LCD and Serial Sound Mux control.  - Japanese format.
	 */
	IOMD_WRITE_BYTE(IOMD_VIDMUX, 0x02);
 
	volume_ctl(VINPUTSEL, VIN1);
	volume_ctl(VLOUD, 0);
	volume_ctl(VBASS, VDBM0);
	volume_ctl(VTREB, VDBM0);
	volume_ctl(VLEFT, 18);
	volume_ctl(VRIGHT, 18);
	volume_ctl(VMODE, VSTEREO);
	volume_ctl(VDIN, 0);

	/* Program the silence buffer and reset the DMA channel */

	ag.silence = uvm_km_alloc(kernel_map, NBPG);
	ag.beep = uvm_km_zalloc(kernel_map, NBPG);
	if (ag.silence == NULL || ag.beep == NULL)
		panic("lmcaudio: Cannot allocate memory\n");
	memset((char *)ag.silence, 0, NBPG);
	memcpy((char *)ag.beep, (char *)beep_waveform, sizeof(beep_waveform));

	conv_jap((u_char *)ag.beep, sizeof(beep_waveform));

	ag.buffer = 0;

	/* Install the irq handler for the DMA interrupt */
	ag.ih.ih_func = lmcaudio_intr;
	ag.ih.ih_arg = NULL;
	ag.ih.ih_level = IPL_AUDIO;

	ag.intr = NULL;

	disable_irq(sdma_channel);

	if (irq_claim(sdma_channel, &(ag.ih)))
		panic("lmcaudio: couldn't claim VIDC AUDIO DMA channel");

	disable_irq(sdma_channel);

	lmcaudio_rate(20000);

	lmcaudio_beep_generate();

	audio_attach_mi(&lmcaudio_hw_if, sc, &sc->device);
}

int nauzero = 0;
int ndmacall = 0;
int
lmcaudio_open(addr, flags)
	void *addr;
	int flags;
{
	struct lmcaudio_softc *sc = addr;

#ifdef DEBUG
	printf("DEBUG: lmcaudio_open called\n");
#endif

	if (sc->open)
		return EBUSY;

	sc->open = 1;
	ag.open = 1;
	ag.drain = 0;

	nauzero = 0;
	ndmacall = 0;

	return 0;
}
 
void
lmcaudio_close(addr)
	void *addr;
{
	struct lmcaudio_softc *sc = addr;

#ifdef DEBUG
	printf("DEBUG: lmcaudio_close called\n");
#endif

	lmcaudio_shutdown();

	sc->open = 0;
	ag.open = 0;
	ag.drain = 0;

#if 0
	printf("ndmacall=%d, auzero=%d\n", ndmacall, nauzero);
#endif

	nauzero = 0;
	ndmacall = 0;
}

void
lmcaudio_timeout(arg)
	void *arg;
{
	wakeup(arg);
}

/*
 * Drain the buffer before closing the device.
 */
int
lmcaudio_drain(addr)
	void *addr;
{
	ag.drain = 1;
	callout_reset(&ag_drain_ch, 30 * hz, lmcaudio_timeout, &ag.drain);
	(void) tsleep(lmcaudio_timeout, PWAIT | PCATCH, "lmcdrain", 0);
	ag.drain = 0;
	return(0);
}

/* ************************************************************************* * 
 | Interface to the generic audio driver                                     |
 * ************************************************************************* */

int
lmcaudio_set_params(addr, setmode, usemode, p, r)
	void *addr;
	int setmode, usemode;
	struct audio_params *p, *r;
{
	if (p->encoding != AUDIO_ENCODING_SLINEAR_LE ||
	    p->precision != 16 ||
	    p->channels != 2)
		return EINVAL;

	return lmcaudio_rate(p->sample_rate);
}

int
lmcaudio_query_encoding(addr, fp)
	void *addr;
	struct audio_encoding *fp;
{
	switch (fp->index) {
	case 0:
		strcpy (fp->name, AudioEslinear_le);
		fp->encoding = AUDIO_ENCODING_SLINEAR_LE;
		fp->precision = 16;
		fp->flags = 0;
		break;

	default:
		return (EINVAL);
	}
	return 0;
}

int
lmcaudio_round_blocksize(addr, blk)
	void *addr;
	int blk;
{

	if (blk > NBPG)
		blk = NBPG;
	else if (blk & 0x0f)	/* quad word */
		blk &= ~0x0f;
	return (blk);
}

#define ROUND(s)  ( ((int)s) & (~(NBPG-1)) )

int
lmcaudio_start_output(addr, p, cc, intr, arg)
	void *addr;
	void *p;
	int cc;
	void (*intr)();
	void *arg;
{
#ifdef DEBUG
	printf ( "lmcaudio_start_output (%d) %08x %08x\n", cc, intr, arg );
#endif

	if (((u_int) p & 0x0000000f) || (ROUND(p) != ROUND(p+cc-1))) {
		/*
		 * Not on quad word boundary.
		 */
		memcpy((char *)ag.silence, p, (cc > NBPG ? NBPG : cc));
		p = (void *)ag.silence;
		if (cc > NBPG) {
			cc = NBPG;
		}
	}
	lmcaudio_dma_program((vm_offset_t)p, (vm_offset_t)(p+cc), intr, arg);
	return(0);
}

int
lmcaudio_start_input(addr, p, cc, intr, arg)
	void *addr;
	void *p;
	int cc;
	void (*intr)();
	void *arg;
{
	return EIO;
}

int
lmcaudio_halt_output(addr)
	void *addr;
{
#ifdef DEBUG
	printf ( "DEBUG: lmcaudio_halt_output\n" );
#endif
	return EIO;
}

int
lmcaudio_halt_input(addr) 
	void *addr;
{
#ifdef DEBUG
	printf ( "DEBUG: lmcaudio_halt_input\n" );
#endif
	return EIO;
}

int
lmcaudio_speaker_ctl(addr, newstate)
	void *addr;
	int newstate;
{
#ifdef DEBUG
	printf("DEBUG: lmcaudio_speaker_ctl\n");
#endif

	switch (newstate) {

	case SPKR_ON:
		volume_ctl(VINPUTSEL, VIN1);
		break;
	case SPKR_OFF:
		volume_ctl(VINPUTSEL, VMUTE);
		break;
	}
	return(0);
}

int
lmcaudio_getdev(addr, retp)
	void *addr;
	struct audio_device *retp;
{
	*retp = lmcaudio_device;
	return(0);
}


int
lmcaudio_set_port(addr, cp)
	void *addr;
	mixer_ctrl_t *cp;
{
	return(EINVAL);
}

int
lmcaudio_get_port(addr, cp)
	void *addr;
	mixer_ctrl_t *cp;
{
	return(EINVAL);
}

int
lmcaudio_get_props(addr)
	void *addr;
	mixer_devinfo_t *dip;
{
	return(0);
}

int
lmcaudio_query_devinfo(addr, dip)
	void *addr;
	mixer_devinfo_t *dip;
{
	return(ENXIO);
}

void
lmcaudio_dummy_routine(arg)
	void *arg;
{
}

int
lmcaudio_rate(rate)
	int rate;
{
	curr_rate = (int)(250000/rate + 0.5) - 2;
	outl(vidc_base, VIDC_SFR | curr_rate);
	return(0);
}

#define PHYS(x, y) pmap_extract(pmap_kernel(), ((x)&PG_FRAME), (paddr_t *)(y))

/*
 * Program the next buffer to be used
 * This function must be re-entrant, maximum re-entrancy of 2
 */

int
lmcaudio_dma_program(cur, end, intr, arg)
	vm_offset_t cur;
	vm_offset_t end;
	void (*intr)();
	void *arg;
{
	int size = end - cur;
	u_int stopflag = 0;
	paddr_t pa;

	if (ag.drain) {
		ag.drain++;
		stopflag = 0x80000000;
	}

	/* If there isn't a transfer in progress then start a new one */
	if (ag.in_progress == 0) {
		ag.buffer = 0;
		IOMD_WRITE_WORD(IOMD_SD0CR, 0x90);	/* Reset State Machine */
		IOMD_WRITE_WORD(IOMD_SD0CR, 0x30);	/* Reset State Machine */

		PHYS(cur, &pa);

		IOMD_WRITE_WORD(IOMD_SD0CURA, pa);
		IOMD_WRITE_WORD(IOMD_SD0ENDA, (pa + size - 16)|stopflag);
		IOMD_WRITE_WORD(IOMD_SD0CURB, pa);
		IOMD_WRITE_WORD(IOMD_SD0ENDB, (pa + size - 16)|stopflag);

		ag.in_progress = 1;

		ag.next_cur = ag.next_end = 0;
		ag.next_intr = ag.next_arg = 0; 

		ag.intr = intr;
		ag.arg = arg;

		/* The driver 'clicks' between buffer swaps, leading me to think */
		/* that the fifo is much small than on other sound cards.  So    */
		/* so I'm going to have to do some tricks here                   */

		(*ag.intr)(ag.arg);			/* Schedule the next buffer */
		ag.intr = lmcaudio_dummy_routine;	/* Already done this        */
		ag.arg = NULL;

		enable_irq(sdma_channel);
	} else {
		/* Otherwise schedule the next one */
		if (ag.next_cur != 0) {
			/* If there's one scheduled then complain */
			printf ( "lmcaudio: Buffer already Q'ed\n" );
			return EIO;
		} else {
			/* We're OK to schedule it now */
			ag.buffer = (++ag.buffer) & 1;
			PHYS(cur, &ag.next_cur);
			ag.next_end = (ag.next_cur + size - 16) | stopflag;
			ag.next_intr = intr;
			ag.next_arg = arg;
		}
	}
	return(0);
}

void
lmcaudio_shutdown()
{
	paddr_t pa;

	/* Shut down the channel */
	ag.intr = NULL;
	ag.in_progress = 0;

#ifdef PRINT
	printf ( "lmcaudio: stop output\n" );
#endif

	memset((char *)ag.silence, 0, NBPG);

	PHYS(ag.silence, &pa);

	IOMD_WRITE_WORD(IOMD_SD0CURA, pa);
	IOMD_WRITE_WORD(IOMD_SD0ENDA, (pa + NBPG - 16) | 0x80000000);
	disable_irq(sdma_channel);
	IOMD_WRITE_WORD(IOMD_SD0CR, 0x90);	/* Reset State Machine */
}

#define OVERRUN 	(0x04)
#define INTERRUPT	(0x02)
#define BANK_A		(0x00)
#define BANK_B		(0x01)

int
lmcaudio_intr(arg)
	void *arg;
{
	int status = IOMD_READ_BYTE(IOMD_SD0ST);
	void (*nintr)();
	void *narg;
	void (*xintr)();
	void *xarg;
	int xcur, xend;
	u_char direction;

	if (ag.open == 0) {
		lmcaudio_shutdown ();
		return(0);
	}

	nintr = ag.intr;
	narg = ag.arg;
	ag.intr = NULL;

	xintr = ag.next_intr;
	xarg = ag.next_arg;
	xcur = ag.next_cur;
	xend = ag.next_end;
	ag.next_cur = 0;
	ag.intr = xintr;
	ag.arg = xarg;

	if (nintr) {
		(*nintr)(narg);
	}

	if (xcur == 0) {
		callout_stop(&ag_drain_ch);
		wakeup(lmcaudio_timeout);
		return(0);
#if 0
		lmcaudio_shutdown ();
#endif
	} else {
		/*
		 * OIA means channel A should be programmed.
		 * OIB means channel B should be programmed.
		 * IA means channel A is busy, program B.
		 * IB means channel B is busy, program A.
		 */
		if (status & OVERRUN) {
			direction = (status & BANK_B);
		} else {
			direction = (status ^ BANK_B) & BANK_B;
		}

		if (direction) {
			IOMD_WRITE_WORD(IOMD_SD0CURB, xcur);
			IOMD_WRITE_WORD(IOMD_SD0ENDB, xend);
		} else {
			IOMD_WRITE_WORD(IOMD_SD0CURA, xcur);
			IOMD_WRITE_WORD(IOMD_SD0ENDA, xend);
		}
		status = inb(IOMD_SD0ST);
		if (status & OVERRUN) {
			if (status & BANK_B) {
				IOMD_WRITE_WORD(IOMD_SD0CURB, xcur);
				IOMD_WRITE_WORD(IOMD_SD0ENDB, xend);
			} else {
				IOMD_WRITE_WORD(IOMD_SD0CURA, xcur);
				IOMD_WRITE_WORD(IOMD_SD0ENDA, xend);
			}
		}
	}

	if (!ag.drain && ag.next_cur == 0) {
		(*ag.intr)(ag.arg);			/* Schedule the next buffer */
		ag.intr = lmcaudio_dummy_routine;	/* Already done this        */
		ag.arg = NULL;
	}
	if (ag.drain == 1) {
		lmcaudio_dma_program(ag.silence, ag.silence+NBPG,
			lmcaudio_dummy_routine, NULL);
	}
	return 0;
}
