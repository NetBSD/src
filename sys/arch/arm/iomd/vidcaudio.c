/*	$NetBSD: vidcaudio.c,v 1.6 2002/03/10 15:47:44 bjh21 Exp $	*/

/*
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
 * audio driver for the RiscPC 16 bit sound
 *
 * Interfaces with the NetBSD generic audio driver to provide SUN
 * /dev/audio (partial) compatibility.
 */

#include <sys/param.h>	/* proc.h */

__KERNEL_RCSID(0, "$NetBSD: vidcaudio.c,v 1.6 2002/03/10 15:47:44 bjh21 Exp $");

#include <sys/conf.h>   /* autoconfig functions */
#include <sys/device.h> /* device calls */
#include <sys/proc.h>	/* device calls */
#include <sys/audioio.h>
#include <sys/errno.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <dev/audio_if.h>

#include <machine/intr.h>
#include <arm/arm32/katelib.h>

#include <arm/iomd/vidcaudiovar.h>
#include <arm/iomd/iomdreg.h>
#include <arm/iomd/iomdvar.h>
#include <arm/iomd/vidc.h>
#include <arm/mainbus/mainbus.h>
#include <arm/iomd/waveform.h>
#include "vidcaudio.h"

extern int *vidc_base;

#undef DEBUG

struct audio_general {
	vm_offset_t silence;
	irqhandler_t ih;

	void (*intr) (void *);
	void *arg;

	vm_offset_t next_cur;
	vm_offset_t next_end;
	void (*next_intr) (void *);
	void *next_arg;

	int buffer;
	int in_progress;

	int open;
} ag;

struct vidcaudio_softc {
	struct device device;
	int iobase;

	int open;
};

int  vidcaudio_probe	__P((struct device *parent, struct cfdata *cf, void *aux));
void vidcaudio_attach	__P((struct device *parent, struct device *self, void *aux));
int  vidcaudio_open	__P((void *addr, int flags));
void vidcaudio_close	__P((void *addr));

int vidcaudio_intr	__P((void *arg));
int vidcaudio_dma_program	__P((vm_offset_t cur, vm_offset_t end, void (*intr)(void *), void *arg));
void vidcaudio_dummy_routine	__P((void *arg));
int vidcaudio_stereo	__P((int channel, int position));
int vidcaudio_rate	__P((int rate));
void vidcaudio_shutdown	__P((void));

static int sound_dma_intr;

struct cfattach vidcaudio_ca = {
	sizeof(struct vidcaudio_softc), vidcaudio_probe, vidcaudio_attach
};

int    vidcaudio_query_encoding  __P((void *, struct audio_encoding *));
int    vidcaudio_set_params	 __P((void *, int, int, struct audio_params *, struct audio_params *));
int    vidcaudio_round_blocksize __P((void *, int));
int    vidcaudio_start_output	 __P((void *, void *, int, void (*)(void *),
					 void *));
int    vidcaudio_start_input	 __P((void *, void *, int, void (*)(void *),
					 void *));
int    vidcaudio_halt_output	 __P((void *));
int    vidcaudio_halt_input 	 __P((void *));
int    vidcaudio_speaker_ctl	 __P((void *, int));
int    vidcaudio_getdev		 __P((void *, struct audio_device *));
int    vidcaudio_set_port	 __P((void *, mixer_ctrl_t *));
int    vidcaudio_get_port	 __P((void *, mixer_ctrl_t *));
int    vidcaudio_query_devinfo	 __P((void *, mixer_devinfo_t *));
int    vidcaudio_get_props	 __P((void *));

struct audio_device vidcaudio_device = {
	"VidcAudio 8-bit",
	"x",
	"vidcaudio"
};

struct audio_hw_if vidcaudio_hw_if = {
	vidcaudio_open,
	vidcaudio_close,
	0,
	vidcaudio_query_encoding,
	vidcaudio_set_params,
	vidcaudio_round_blocksize,
	0,
	0,
	0,
	vidcaudio_start_output,
	vidcaudio_start_input,
	vidcaudio_halt_output,
	vidcaudio_halt_input,
	vidcaudio_speaker_ctl,
	vidcaudio_getdev,
	0,
	vidcaudio_set_port,
	vidcaudio_get_port,
	vidcaudio_query_devinfo,
	0,
	0,
	0,
	0,
	vidcaudio_get_props,
	0,
	0,
	0,
};


void
vidcaudio_beep_generate()
{
	vidcaudio_dma_program(ag.silence, ag.silence+sizeof(beep_waveform)-16,
	    vidcaudio_dummy_routine, NULL);
}


int
vidcaudio_probe(parent, cf, aux)
	struct device *parent;
	struct cfdata* cf;
	void *aux;
{
	int id;

	id = IOMD_ID;

	/* So far I only know about this IOMD */
	switch (id) {
	case RPC600_IOMD_ID:
		return(1);
		break;
	case ARM7500_IOC_ID:
	case ARM7500FE_IOC_ID:
		return(1);
		break;
	default:
		printf("vidcaudio: Unknown IOMD id=%04x", id);
		break;
	}

	return (0);
}


void
vidcaudio_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct mainbus_attach_args *mb = aux;
	struct vidcaudio_softc *sc = (void *)self;
	int id;

	sc->iobase = mb->mb_iobase;

	sc->open = 0;
	ag.in_progress = 0;

	ag.next_cur = 0;
	ag.next_end = 0;
	ag.next_intr = NULL;
	ag.next_arg = NULL;

	vidcaudio_rate(32); /* 24*1024*/

	/* Program the silence buffer and reset the DMA channel */
	ag.silence = uvm_km_alloc(kernel_map, NBPG);
	if (ag.silence == NULL)
		panic("vidcaudio: Cannot allocate memory\n");

	memset((char *)ag.silence, 0, NBPG);
	memcpy((char *)ag.silence, (char *)beep_waveform, sizeof(beep_waveform));

	ag.buffer = 0;

	/* Install the irq handler for the DMA interrupt */
	ag.ih.ih_func = vidcaudio_intr;
	ag.ih.ih_arg = NULL;
	ag.ih.ih_level = IPL_AUDIO;
	ag.ih.ih_name = "vidcaudio";

	ag.intr = NULL;
/*	ag.nextintr = NULL;*/

	id = IOMD_ID;

	switch (id) {
	case RPC600_IOMD_ID:
		sound_dma_intr = IRQ_DMASCH0;
		break;
	case ARM7500_IOC_ID:
	case ARM7500FE_IOC_ID:
		sound_dma_intr = IRQ_SDMA;
		break;
	}

	disable_irq(sound_dma_intr);

	if (irq_claim(sound_dma_intr, &(ag.ih)))
		panic("vidcaudio: couldn't claim IRQ %d\n", sound_dma_intr);

	disable_irq(sound_dma_intr);

	printf("\n");

	vidcaudio_dma_program(ag.silence, ag.silence+NBPG-16,
	    vidcaudio_dummy_routine, NULL);

	audio_attach_mi(&vidcaudio_hw_if, sc, &sc->device);

#ifdef DEBUG
	printf(" UNDER DEVELOPMENT (nuts)\n");
#endif
}

int
vidcaudio_open(addr, flags)
	void *addr;
	int flags;
{
	struct vidcaudio_softc *sc = addr;

#ifdef DEBUG
	printf("DEBUG: vidcaudio_open called\n");
#endif

	if (sc->open)
		return EBUSY;

	sc->open = 1;
	ag.open = 1;

	return 0;
}
 
void
vidcaudio_close(addr)
	void *addr;
{
	struct vidcaudio_softc *sc = addr;

	vidcaudio_shutdown();

#ifdef DEBUG
	printf("DEBUG: vidcaudio_close called\n");
#endif

	sc->open = 0;
	ag.open = 0;
}

/* ************************************************************************* * 
 | Interface to the generic audio driver                                     |
 * ************************************************************************* */

int
vidcaudio_query_encoding(addr, fp)
	void *addr;
	struct audio_encoding *fp;
{
	switch (fp->index) {
	case 0:
		strcpy(fp->name, "vidc");
		fp->encoding = AUDIO_ENCODING_ULAW;
		fp->precision = 8;
		fp->flags = 0;
		break;

	default:
		return(EINVAL);
	}
	return 0;
}

int
vidcaudio_set_params(addr, setmode, usemode, p, r)
	void *addr;
	int setmode, usemode;
	struct audio_params *p, *r;
{
	if (p->encoding != AUDIO_ENCODING_ULAW ||
	    p->channels != 8)
		return EINVAL;
	vidcaudio_rate(4 * p->sample_rate / (3 * 1024)); /* XXX probably wrong */

	return 0;
}

int
vidcaudio_round_blocksize(addr, blk)
	void *addr;
	int blk;
{
	if (blk > NBPG)
		blk = NBPG;
	return (blk);
}

#define ROUND(s)  ( ((int)s) & (~(NBPG-1)) )

int
vidcaudio_start_output(addr, p, cc, intr, arg)
	void *addr;
	void *p;
	int cc;
	void (*intr)(void *);
	void *arg;
{
	/* I can only DMA inside 1 page */

#ifdef DEBUG
	printf("vidcaudio_start_output (%d) %08x %08x\n", cc, intr, arg);
#endif

	if (ROUND(p) != ROUND(p+cc)) {
		/*
		 * If it's over a page I can fix it up by copying it into
		 * my buffer
		 */

#ifdef DEBUG
		printf("vidcaudio: DMA over page boundary requested."
		    "  Fixing up\n");
#endif
		memcpy(p, (char *)ag.silence, cc > NBPG ? NBPG : cc);
		p = (void *)ag.silence;

		/*
		 * I can't DMA any more than that, but it is possible to
		 * fix it up by handling multiple buffers and only
		 * interrupting the audio driver after sending out all the
		 * stuff it gave me.  That it more than I can be bothered
		 * to do right now and it probablly wont happen so I'll just
		 * truncate the buffer and tell the user.
		 */

		if (cc > NBPG) {
			printf("vidcaudio: DMA buffer truncated. I could fix this up\n");
			cc = NBPG;
		}
	}
	vidcaudio_dma_program((vm_offset_t)p, (vm_offset_t)((char *)p+cc),
	    intr, arg);
	return 0;
}

int
vidcaudio_start_input(addr, p, cc, intr, arg)
	void *addr;
	void *p;
	int cc;
	void (*intr)(void *);
	void *arg;
{
	return EIO;
}

int
vidcaudio_halt_output(addr)
	void *addr;
{
#ifdef DEBUG
	printf("DEBUG: vidcaudio_halt_output\n");
#endif
	return EIO;
}

int
vidcaudio_halt_input(addr)
	void *addr;
{
#ifdef DEBUG
	printf("DEBUG: vidcaudio_halt_input\n");
#endif
	return EIO;
}

int
vidcaudio_speaker_ctl(addr, newstate)
	void *addr;
	int newstate;
{
#ifdef DEBUG
	printf("DEBUG: vidcaudio_speaker_ctl\n");
#endif
	return 0;
}

int
vidcaudio_getdev(addr, retp)
	void *addr;
	struct audio_device *retp;
{
	*retp = vidcaudio_device;
	return 0;
}


int
vidcaudio_set_port(addr, cp)
	void *addr;
	mixer_ctrl_t *cp;
{
	return EINVAL;
}

int
vidcaudio_get_port(addr, cp)
	void *addr;
	mixer_ctrl_t *cp;
{
	return EINVAL;
}

int
vidcaudio_query_devinfo(addr, dip)
	void *addr;
	mixer_devinfo_t *dip;
{
	return ENXIO;
}

int
vidcaudio_get_props(addr)
	void *addr;
{
	return 0;
}
void
vidcaudio_dummy_routine(arg)
	void *arg;
{
#ifdef DEBUG
	printf("vidcaudio_dummy_routine\n");
#endif
}

int
vidcaudio_rate(rate)
	int rate;
{
	WriteWord(vidc_base, VIDC_SFR | rate);
	return 0;
}

int
vidcaudio_stereo(channel, position)
	int channel;
	int position;
{
	if (channel < 0) return EINVAL;
	if (channel > 7) return EINVAL;
	channel = channel<<24 | VIDC_SIR0;
	WriteWord(vidc_base, channel | position);
	return 0;
}

#define PHYS(x, y) pmap_extract(pmap_kernel(), ((x)&PG_FRAME), (paddr_t *)(y))

/*
 * Program the next buffer to be used
 * This function must be re-entrant, maximum re-entrancy of 2
 */

#define FLAGS (0)

int
vidcaudio_dma_program(cur, end, intr, arg)
	vm_offset_t cur;
	vm_offset_t end;
	void (*intr)(void *);
	void *arg;
{
	paddr_t pa1, pa2;

	/* If there isn't a transfer in progress then start a new one */
	if (ag.in_progress == 0) {
		ag.buffer = 0;
		IOMD_WRITE_WORD(IOMD_SD0CR, 0x90);	/* Reset State Machine */
		IOMD_WRITE_WORD(IOMD_SD0CR, 0x30);	/* Reset State Machine */

		PHYS(cur, &pa1);
		PHYS(end - 16, &pa2);

		IOMD_WRITE_WORD(IOMD_SD0CURB, pa1);
		IOMD_WRITE_WORD(IOMD_SD0ENDB, pa2|FLAGS);
		IOMD_WRITE_WORD(IOMD_SD0CURA, pa1);
		IOMD_WRITE_WORD(IOMD_SD0ENDA, pa2|FLAGS);

		ag.in_progress = 1;

		ag.next_cur = ag.next_end = 0;
		ag.next_intr = ag.next_arg = 0; 

		ag.intr = intr;
		ag.arg = arg;

		/*
		 * The driver 'clicks' between buffer swaps, leading me
		 * to think  that the fifo is much small than on other
		 * sound cards so I'm going to have to do some tricks here
		 */

		(*ag.intr)(ag.arg);			/* Schedule the next buffer */
		ag.intr = vidcaudio_dummy_routine;	/* Already done this        */
		ag.arg = NULL;

#ifdef PRINT
		printf("vidcaudio: start output\n");
#endif
#ifdef DEBUG
		printf("SE");
#endif
		enable_irq(sound_dma_intr);
	} else {
		/* Otherwise schedule the next one */
		if (ag.next_cur != 0) {
			/* If there's one scheduled then complain */
			printf("vidcaudio: Buffer already Q'ed\n");
			return EIO;
		} else {
			/* We're OK to schedule it now */
			ag.buffer = (++ag.buffer) & 1;
			PHYS(cur, &ag.next_cur);
			PHYS(end - 16, &ag.next_end);
			ag.next_intr = intr;
			ag.next_arg = arg;
#ifdef DEBUG
			printf("s");
#endif
		}
	}
	return 0;
}

void
vidcaudio_shutdown(void)
{
	/* Shut down the channel */
	ag.intr = NULL;
	ag.in_progress = 0;
#ifdef PRINT
	printf("vidcaudio: stop output\n");
#endif
	IOMD_WRITE_WORD(IOMD_SD0CURB, ag.silence);
	IOMD_WRITE_WORD(IOMD_SD0ENDB, (ag.silence + NBPG - 16) | (1<<30));
	disable_irq(sound_dma_intr);
}

int
vidcaudio_intr(arg)
	void *arg;
{
	int status = IOMD_READ_BYTE(IOMD_SD0ST);
	void (*nintr)(void *);
	void *narg;
	void (*xintr)(void *);
	void *xarg;
	int xcur, xend;
	IOMD_WRITE_WORD(IOMD_DMARQ, 0x10);

#ifdef PRINT
	printf ( "I" );
#endif

	if (ag.open == 0) {
		vidcaudio_shutdown();
		return 0;
	}

	/* Have I got the generic audio device attached */

#ifdef DEBUG
	printf ( "[B%01x]", status );
#endif

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
#ifdef DEBUG
		printf("i");
#endif
		(*nintr)(narg);
	}

	if (xcur == 0) {
		vidcaudio_shutdown ();
	} else {
#define OVERRUN 	(0x04)
#define INTERRUPT	(0x02)
#define BANK_A		(0x00)
#define BANK_B		(0x01)
		switch (status & 0x7) {
		case (INTERRUPT|BANK_A):
#ifdef PRINT
			printf("B");
#endif
			IOMD_WRITE_WORD(IOMD_SD0CURB, xcur);
			IOMD_WRITE_WORD(IOMD_SD0ENDB, xend|FLAGS);
			break;

		case (INTERRUPT|BANK_B):
#ifdef PRINT
			printf("A");
#endif
			IOMD_WRITE_WORD(IOMD_SD0CURA, xcur);
			IOMD_WRITE_WORD(IOMD_SD0ENDA, xend|FLAGS);
			break;

		case (OVERRUN|INTERRUPT|BANK_A):
#ifdef PRINT
			printf("A");
#endif
			IOMD_WRITE_WORD(IOMD_SD0CURA, xcur);
			IOMD_WRITE_WORD(IOMD_SD0ENDA, xend|FLAGS);
			break;
		 
		case (OVERRUN|INTERRUPT|BANK_B):
#ifdef PRINT
			printf("B");
#endif
			IOMD_WRITE_WORD(IOMD_SD0CURB, xcur);
			IOMD_WRITE_WORD(IOMD_SD0ENDB, xend|FLAGS);
			break;
		}
/*
	ag.next_cur = 0;
	ag.intr = xintr;
	ag.arg = xarg;
*/
	}
#ifdef PRINT
	printf ( "i" );
#endif

	if (ag.next_cur == 0) {
		(*ag.intr)(ag.arg);			/* Schedule the next buffer */
		ag.intr = vidcaudio_dummy_routine;	/* Already done this        */
		ag.arg = NULL;
	}
	return(0);	/* Pass interrupt on down the chain */
}
