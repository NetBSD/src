/* $NetBSD: lmcaudio.c,v 1.4 1997/03/20 17:04:11 mycroft Exp $ */

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
 *      This product includes software developed by Michael L. Hitch.
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

/*
 * audio driver for lmc1982
 *
 * Interfaces with the NetBSD generic audio driver to provide SUN
 * /dev/audio (partial) compatibility.
 */

#include <sys/param.h>	/* proc.h */
#include <sys/types.h>  /* dunno  */
#include <sys/conf.h>   /* autoconfig functions */
#include <sys/device.h> /* device calls */
#include <sys/proc.h>	/* device calls */
#include <sys/audioio.h>
#include <sys/errno.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>

#include <dev/audio_if.h>

#include <machine/irqhandler.h>
#include <machine/iomd.h>
#include <machine/vidc.h>
#include <machine/katelib.h>

#include <arm32/mainbus/mainbus.h>
#include <arm32/mainbus/waveform.h>
#include "lmcaudio.h"

#include <arm32/mainbus/lmc1982.h>
 
struct audio_general {
	int in_sr;
	int out_sr;
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

struct lmcaudio_softc {
	struct device device;
	int iobase;

	int open;

	u_int encoding;
	u_int precision;
	int channels;

	int inport;
	int outport;
};

int  lmcaudio_probe	__P((struct device *parent, void *match, void *aux));
void lmcaudio_attach	__P((struct device *parent, struct device *self, void *aux));
int  lmcaudio_open	__P((dev_t dev, int flags));
void lmcaudio_close	__P((void *addr));
int lmcaudio_drain	__P((void *addr));
void lmcaudio_timeout	__P((void *arg));

int lmcaudio_intr	__P((void *arg));
int lmcaudio_dma_program	__P((vm_offset_t cur, vm_offset_t end, void (*intr)(), void *arg));
void lmcaudio_dummy_routine	__P((void *arg));
int lmcaudio_stereo	__P((int channel, int position));
int lmcaudio_rate	__P((int rate));
void lmcaudio_shutdown	__P((void));
int lmcaudio_hw_attach	__P((struct lmcaudio_softc *sc));


struct cfattach lmcaudio_ca = {
	sizeof(struct lmcaudio_softc), lmcaudio_probe, lmcaudio_attach
};
struct cfdriver lmcaudio_cd = {
	NULL, "lmcaudio", DV_DULL
};


int curr_rate = 11;

void
lmcaudio_beep_generate()
{
	lmcaudio_dma_program(ag.beep, ag.beep+sizeof(beep_waveform)-16,
	    lmcaudio_dummy_routine, NULL);
}


static int sdma_channel;
int
lmcaudio_probe(parent, match, aux)
	struct device *parent;
	void *match;
	void *aux;
{
	int id;

	id = IOMD_ID;

	switch (id) {
	case RPC600_IOMD_ID:
		return(0);
#ifdef RC7500
	case RC7500_IOC_ID:
		sdma_channel = IRQ_SDMA;
		return(1);
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
	sc->encoding = AUDIO_ENCODING_PCM16;
	sc->precision = 16;
	sc->channels = 2;
	sc->inport = 0;
	sc->outport = 0;
	ag.in_sr = 20000;
	ag.out_sr = 20000;
	ag.in_progress = 0;

	ag.next_cur = 0;
	ag.next_end = 0;
	ag.next_intr = NULL;
	ag.next_arg = NULL;

	/*
	 * Enable serial sound.  The digital serial sound interface
	 * consists of 16 bits sample on each channel.
	 */
	outl(VIDC_BASE, VIDC_SCR | 0x03);
 
	/*
	 * Video LCD and Serial Sound Mux control.  - Japanese format.
	 */
	outb(IOMD_VIDMUX, 0x02);
 
	volume_ctl(VINPUTSEL, VIN1);
	volume_ctl(VLOUD, 0);
	volume_ctl(VBASS, VDBM0);
	volume_ctl(VTREB, VDBM0);
	volume_ctl(VLEFT, 18);
	volume_ctl(VRIGHT, 18);
	volume_ctl(VMODE, VSTEREO);
	volume_ctl(VDIN, 0);

	/* Program the silence buffer and reset the DMA channel */

	ag.silence = kmem_alloc(kernel_map, NBPG);
	if (ag.silence == NULL)
		panic("lmcaudio: Cannot allocate memory\n");

	bzero((char *)ag.silence, NBPG);

	ag.beep = kmem_alloc(kernel_map, NBPG);
	if (ag.beep == NULL)
		panic("lmcaudio: Cannot allocate memory\n");

	bcopy((char *)beep_waveform, (char *)ag.beep, sizeof(beep_waveform));

	conv_jap((u_char *) ag.beep, sizeof(beep_waveform));

	ag.buffer = 0;

	/* Install the irq handler for the DMA interrupt */
	ag.ih.ih_func = lmcaudio_intr;
	ag.ih.ih_arg = NULL;
	ag.ih.ih_level = IPL_NONE;

	ag.intr = NULL;

	disable_irq(sdma_channel);

	if (irq_claim(sdma_channel, &(ag.ih)))
		panic("lmcaudio: couldn't claim VIDC AUDIO DMA channel");

	disable_irq(sdma_channel);

	lmcaudio_rate(ag.out_sr);

	lmcaudio_beep_generate();

	lmcaudio_hw_attach(sc);

}

int nauzero = 0;
int ndmacall = 0;
int
lmcaudio_open(dev, flags)
	dev_t dev;
	int flags;
{
	struct lmcaudio_softc *sc;
	int unit = AUDIOUNIT (dev);
	int s;

#ifdef DEBUG
	printf("DEBUG: lmcaudio_open called\n");
#endif

	if (unit >= lmcaudio_cd.cd_ndevs)
		return ENODEV;

	sc = lmcaudio_cd.cd_devs[unit];

	if (!sc)
		return ENXIO;

	s = splhigh();

	if (sc->open != 0) {
		(void)splx(s);
		return EBUSY;
	}

	sc->open = 1;
	ag.open = 1;
	ag.drain = 0;

	nauzero = 0;
	ndmacall = 0;

	(void)splx(s);

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
	timeout(lmcaudio_timeout, &ag.drain, 30 * hz);
	(void) tsleep(lmcaudio_timeout, PWAIT | PCATCH, "lmcdrain", 0);
	ag.drain = 0;
	return(0);
}

/* ************************************************************************* * 
 | Interface to the generic audio driver                                     |
 * ************************************************************************* */

int    lmcaudio_set_in_sr       __P((void *, u_long));
u_long lmcaudio_get_in_sr       __P((void *));
int    lmcaudio_set_out_sr      __P((void *, u_long));
u_long lmcaudio_get_out_sr      __P((void *));
int    lmcaudio_query_encoding  __P((void *, struct audio_encoding *));
int    lmcaudio_set_format	 __P((void *, u_int, u_int));
int    lmcaudio_get_encoding	 __P((void *));
int    lmcaudio_get_precision	 __P((void *));
int    lmcaudio_set_channels 	 __P((void *, int));
int    lmcaudio_get_channels	 __P((void *));
int    lmcaudio_round_blocksize __P((void *, int));
int    lmcaudio_set_out_port	 __P((void *, int));
int    lmcaudio_get_out_port	 __P((void *));
int    lmcaudio_set_in_port	 __P((void *, int));
int    lmcaudio_get_in_port  	 __P((void *));
int    lmcaudio_commit_settings __P((void *));
void   lmcaudio_sw_encode	 __P((void *, int, u_char *, int));
void   lmcaudio_sw_decode	 __P((void *, int, u_char *, int));
int    lmcaudio_start_output	 __P((void *, void *, int, void (*)(), void *));
int    lmcaudio_start_input	 __P((void *, void *, int, void (*)(), void *));
int    lmcaudio_halt_output	 __P((void *));
int    lmcaudio_halt_input 	 __P((void *));
int    lmcaudio_cont_output	 __P((void *));
int    lmcaudio_cont_input	 __P((void *));
int    lmcaudio_speaker_ctl	 __P((void *, int));
int    lmcaudio_getdev		 __P((void *, struct audio_device *));
int    lmcaudio_setfd	 	 __P((void *, int));
int    lmcaudio_set_port	 __P((void *, mixer_ctrl_t *));
int    lmcaudio_get_port	 __P((void *, mixer_ctrl_t *));
int    lmcaudio_query_devinfo	 __P((void *, mixer_devinfo_t *));

struct audio_device lmcaudio_device = {
	"LMCAudio 16-bit",
	"x",
	"lmcaudio"
};

int
lmcaudio_set_in_sr(addr, sr)
	void *addr;
	u_long sr;
{
	ag.in_sr = sr;
	return 0;
}

u_long
lmcaudio_get_in_sr(addr)
	void *addr;
{
	return ag.in_sr;
}

int
lmcaudio_set_out_sr(addr, sr)
	void *addr;
	u_long sr;
{
	ag.out_sr = sr;
	lmcaudio_rate(sr);
	return 0;
}

u_long
lmcaudio_get_out_sr(addr)
	void *addr;
{
	return(ag.out_sr);
}

int
lmcaudio_query_encoding(addr, fp)
	void *addr;
	struct audio_encoding *fp;
{
	switch (fp->index) {
	case 0:
		strcpy (fp->name, AudioElinear);
		fp->format_id = AUDIO_ENCODING_LINEAR;
		break;

	default:
		return (EINVAL);
	}
	return 0;
}

int
lmcaudio_set_format(addr, encoding, precision)
	void *addr;
	u_int encoding, precision;
{
	struct lmcaudio_softc *sc = addr;

	if (encoding != AUDIO_ENCODING_PCM16)
		return (EINVAL);
	if (precision != 16)
		return (EINVAL);

	sc->encoding = encoding;
	sc->precision = precision;
	return (0);
}

int
lmcaudio_get_encoding(addr)
	void *addr;
{
	struct lmcaudio_softc *sc = addr;

	return (sc->encoding);
}

int
lmcaudio_get_precision(addr)
	void *addr;
{
	register struct lmcaudio_softc *sc = addr;

	return (sc->precision);
}

int
lmcaudio_set_channels(addr, channels)
	void *addr;
	int channels;
{
	register struct lmcaudio_softc *sc = addr;

	if (channels != 2)
		return (EINVAL);

	sc->channels = channels;
	return (0);
}

int
lmcaudio_get_channels(addr)
	void *addr;
{
	register struct lmcaudio_softc *sc = addr;

	return (sc->channels);
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

int
lmcaudio_set_out_port(addr, port)
	void *addr;
	int port;
{
	register struct lmcaudio_softc *sc = addr;
	sc->outport = port;
	return(0);
}

int
lmcaudio_get_out_port(addr)
	void *addr;
{
	register struct lmcaudio_softc *sc = addr;
	return(sc->outport);
}

int
lmcaudio_set_in_port(addr, port)
	void *addr;
	int port;
{
	register struct lmcaudio_softc *sc = addr;
	sc->inport = port;
	return(0);
}

int
lmcaudio_get_in_port(addr)
	void *addr;
{
	register struct lmcaudio_softc *sc = addr;
	return(sc->inport);
}

int
lmcaudio_commit_settings(addr)
	void *addr;
{
#ifdef DEBUG
printf ( "DEBUG: committ_settings\n" );
#endif
	return(0);
}

void
lmcaudio_sw_encode(addr, e, p, cc)
	void *addr;
	int e;
	u_char *p;
	int cc;
{
#ifdef DEBUG
	printf ( "DEBUG: sw_encode\n" );    
#endif
	return;
}

void
lmcaudio_sw_decode(addr, e, p, cc)
	void *addr;
	int e;
	u_char *p;
	int cc;
{
#ifdef DEBUG
	printf ( "DEBUG: sw_decode\n" );    
#endif
}

#define ROUND(s)  ( ((int)s) & (~(NBPG-1)) )

extern char *auzero_block;
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

if (p == auzero_block)
	nauzero++;
ndmacall++;

	if (((u_int) p & 0x0000000f) || (ROUND(p) != ROUND(p+cc-1))) {
		/*
		 * Not on quad word boundary.
		 */
		bcopy(p, (char *)ag.silence, (cc > NBPG ? NBPG : cc));
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
	printf ( "DEBUG: lmcaudio_halt_output\n" );
#endif
	return EIO;
}

int
lmcaudio_cont_output(addr)
	void *addr;
{
#ifdef DEBUG
	printf ( "DEBUG: lmcaudio_halt_output\n" );
#endif
	return EIO;
}

int
lmcaudio_cont_input(addr)
	void *addr;
{
#ifdef DEBUG
	printf ( "DEBUG: lmcaudio_halt_output\n" );
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
lmcaudio_setfd(addr, flag)
	void *addr;
	int flag;
{
	/* Can't do full-duplex */
	return(ENOTTY);
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
lmcaudio_query_devinfo(addr, dip)
	void *addr;
	register mixer_devinfo_t *dip;
{
	return(0);
}

struct audio_hw_if lmcaudio_hw_if = {
	lmcaudio_open,
	lmcaudio_close,
	lmcaudio_drain,
	lmcaudio_set_in_sr,
	lmcaudio_get_in_sr,
	lmcaudio_set_out_sr,
	lmcaudio_get_out_sr,
	lmcaudio_query_encoding,
	lmcaudio_set_format,
	lmcaudio_get_encoding,
	lmcaudio_get_precision,
	lmcaudio_set_channels,
	lmcaudio_get_channels,
	lmcaudio_round_blocksize,
	lmcaudio_set_out_port,
	lmcaudio_get_out_port,
	lmcaudio_set_in_port,
	lmcaudio_get_in_port,
	lmcaudio_commit_settings,
	lmcaudio_sw_encode,
	lmcaudio_sw_decode,
	lmcaudio_start_output,
	lmcaudio_start_input,
	lmcaudio_halt_output,
	lmcaudio_halt_input,
	lmcaudio_cont_output,
	lmcaudio_cont_input,
	lmcaudio_speaker_ctl,
	lmcaudio_getdev,
	lmcaudio_setfd,
	lmcaudio_set_port,
	lmcaudio_get_port,
	lmcaudio_query_devinfo,
	0, /* not full duplex */
	0
};

void
lmcaudio_dummy_routine(arg)
	void *arg;
{
}

int
lmcaudio_hw_attach(sc)
	struct lmcaudio_softc *sc;
{
	return(audio_hardware_attach(&lmcaudio_hw_if, sc));
}

int
lmcaudio_rate(rate)
	int rate;
{
	curr_rate = (int)(250000/rate + 0.5) - 2;
	outl(VIDC_BASE, VIDC_SFR | curr_rate);
	return(0);
}

int
lmcaudio_stereo(channel, position)
	int channel;
	int position;
{
	return(0);
}

#define PHYS(x) (pmap_extract( kernel_pmap, ((x)&PG_FRAME) ))

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

	if (ag.drain) {
		ag.drain++;
		stopflag = 0x80000000;
	}

	/* If there isn't a transfer in progress then start a new one */
	if (ag.in_progress == 0) {
		ag.buffer = 0;
		outl(IOMD_SD0CR, 0x90);	/* Reset State Machine */
		outl(IOMD_SD0CR, 0x30);	/* Reset State Machine */

		outl(IOMD_SD0CURA, PHYS(cur));
		outl(IOMD_SD0ENDA, (PHYS(cur) + size - 16)|stopflag);
		outl(IOMD_SD0CURB, PHYS(cur));
		outl(IOMD_SD0ENDB, (PHYS(cur) + size - 16)|stopflag);

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
			ag.next_cur = PHYS(cur);
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
	/* Shut down the channel */
	ag.intr = NULL;
	ag.in_progress = 0;

#ifdef PRINT
	printf ( "lmcaudio: stop output\n" );
#endif

	bzero((char *)ag.silence, NBPG);
	outl(IOMD_SD0CURA, PHYS(ag.silence));
	outl(IOMD_SD0ENDA, (PHYS(ag.silence) + NBPG - 16) | 0x80000000);
	disable_irq(sdma_channel);
	outl(IOMD_SD0CR, 0x90);	/* Reset State Machine */
}

#define OVERRUN 	(0x04)
#define INTERRUPT	(0x02)
#define BANK_A		(0x00)
#define BANK_B		(0x01)

int
lmcaudio_intr(arg)
	void *arg;
{
	int status = inb(IOMD_SD0ST);
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
		untimeout(lmcaudio_timeout, &ag.drain);
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
			outl(IOMD_SD0CURB, xcur);
			outl(IOMD_SD0ENDB, xend);
		} else {
			outl(IOMD_SD0CURA, xcur);
			outl(IOMD_SD0ENDA, xend);
		}
		status = inb(IOMD_SD0ST);
		if (status & OVERRUN) {
			if (status & BANK_B) {
				outl(IOMD_SD0CURB, xcur);
				outl(IOMD_SD0ENDB, xend);
			} else {
				outl(IOMD_SD0CURA, xcur);
				outl(IOMD_SD0ENDA, xend);
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
