/*	$NetBSD: vidcaudio.c,v 1.23 2003/12/29 16:49:31 bjh21 Exp $	*/

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

__KERNEL_RCSID(0, "$NetBSD: vidcaudio.c,v 1.23 2003/12/29 16:49:31 bjh21 Exp $");

#include <sys/audioio.h>
#include <sys/conf.h>   /* autoconfig functions */
#include <sys/device.h> /* device calls */
#include <sys/errno.h>
#include <sys/proc.h>	/* device calls */
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

struct audio_general {
	vaddr_t	silence;
	irqhandler_t	ih;

	void	(*intr)(void *);
	void	*arg;

	paddr_t	next_cur;
	paddr_t	next_end;
	void	(*next_intr)(void *);
	void	*next_arg;

	int	buffer;
	int	in_progress;

	int	open;
} ag;

struct vidcaudio_softc {
	struct	device device;

	int	open;
};

int  vidcaudio_probe(struct device *, struct cfdata *, void *);
void vidcaudio_attach(struct device *, struct device *, void *);
int  vidcaudio_open(void *, int);
void vidcaudio_close(void *);

int vidcaudio_intr(void *);
int vidcaudio_dma_program(vaddr_t, vaddr_t, void (*)(void *), void *);
void vidcaudio_dummy_routine(void *);
int vidcaudio_stereo(int, int);
int vidcaudio_rate(int);
void vidcaudio_shutdown(void);

static int sound_dma_intr;

CFATTACH_DECL(vidcaudio, sizeof(struct vidcaudio_softc),
    vidcaudio_probe, vidcaudio_attach, NULL, NULL);

int    vidcaudio_query_encoding(void *, struct audio_encoding *);
int    vidcaudio_set_params(void *, int, int, struct audio_params *,
    struct audio_params *);
int    vidcaudio_round_blocksize(void *, int);
int    vidcaudio_start_output(void *, void *, int, void (*)(void *), void *);
int    vidcaudio_start_input(void *, void *, int, void (*)(void *), void *);
int    vidcaudio_halt_output(void *);
int    vidcaudio_halt_input(void *);
int    vidcaudio_getdev(void *, struct audio_device *);
int    vidcaudio_set_port(void *, mixer_ctrl_t *);
int    vidcaudio_get_port(void *, mixer_ctrl_t *);
int    vidcaudio_query_devinfo(void *, mixer_devinfo_t *);
int    vidcaudio_get_props(void *);

struct audio_device vidcaudio_device = {
	"VidcAudio 8-bit",
	"x",
	"vidcaudio"
};

struct audio_hw_if vidcaudio_hw_if = {
	vidcaudio_open,
	vidcaudio_close,
	NULL,
	vidcaudio_query_encoding,
	vidcaudio_set_params,
	vidcaudio_round_blocksize,
	NULL,
	NULL,
	NULL,
	vidcaudio_start_output,
	vidcaudio_start_input,
	vidcaudio_halt_output,
	vidcaudio_halt_input,
	NULL,
	vidcaudio_getdev,
	NULL,
	vidcaudio_set_port,
	vidcaudio_get_port,
	vidcaudio_query_devinfo,
	NULL,
	NULL,
	NULL,
	NULL,
	vidcaudio_get_props,
	NULL,
	NULL,
	NULL,
};


void
vidcaudio_beep_generate(void)
{

	vidcaudio_dma_program(ag.silence, ag.silence+sizeof(beep_waveform)-16,
	    vidcaudio_dummy_routine, NULL);
}


int
vidcaudio_probe(struct device *parent, struct cfdata *cf, void *aux)
{
	int id;

	id = IOMD_ID;

	/* So far I only know about this IOMD */
	switch (id) {
	case RPC600_IOMD_ID:
		return 1;
		break;
	case ARM7500_IOC_ID:
	case ARM7500FE_IOC_ID:
		return 1;
		break;
	default:
		printf("vidcaudio: Unknown IOMD id=%04x", id);
		break;
	}

	return 0;
}


void
vidcaudio_attach(struct device *parent, struct device *self, void *aux)
{
	struct vidcaudio_softc *sc = (void *)self;
	int id;

	sc->open = 0;
	ag.in_progress = 0;

	ag.next_cur = 0;
	ag.next_end = 0;
	ag.next_intr = NULL;
	ag.next_arg = NULL;

	vidcaudio_rate(32); /* 24*1024 */

	/* Program the silence buffer and reset the DMA channel */
	ag.silence = uvm_km_alloc(kernel_map, PAGE_SIZE);
	if (ag.silence == 0)
		panic("vidcaudio: Cannot allocate memory");

	memset((char *)ag.silence, 0, PAGE_SIZE);
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
		panic("vidcaudio: couldn't claim IRQ %d", sound_dma_intr);

	disable_irq(sound_dma_intr);

	printf("\n");

	vidcaudio_dma_program(ag.silence, ag.silence + PAGE_SIZE - 16,
	    vidcaudio_dummy_routine, NULL);

	audio_attach_mi(&vidcaudio_hw_if, sc, &sc->device);

#ifdef VIDCAUDIO_DEBUG
	printf(" UNDER DEVELOPMENT (nuts)\n");
#endif
}

int
vidcaudio_open(void *addr, int flags)
{
	struct vidcaudio_softc *sc = addr;

#ifdef VIDCAUDIO_DEBUG
	printf("DEBUG: vidcaudio_open called\n");
#endif

	if (sc->open)
		return EBUSY;

	sc->open = 1;
	ag.open = 1;

	return 0;
}
 
void
vidcaudio_close(void *addr)
{
	struct vidcaudio_softc *sc = addr;

	vidcaudio_shutdown();

#ifdef VIDCAUDIO_DEBUG
	printf("DEBUG: vidcaudio_close called\n");
#endif

	sc->open = 0;
	ag.open = 0;
}

/*
 * Interface to the generic audio driver
 */

int
vidcaudio_query_encoding(void *addr, struct audio_encoding *fp)
{

	switch (fp->index) {
	case 0:
		strcpy(fp->name, "vidc");
		fp->encoding = AUDIO_ENCODING_ULAW;
		fp->precision = 8;
		fp->flags = 0;
		break;

	default:
		return EINVAL;
	}
	return 0;
}

int
vidcaudio_set_params(void *addr, int setmode, int usemode,
    struct audio_params *p, struct audio_params *r)
{

	if (p->encoding != AUDIO_ENCODING_ULAW)
		return EINVAL;
	/* FIXME Handle number of channels properly. */
	vidcaudio_rate(4 * p->sample_rate / (3 * 1024)); /* XXX probably wrong */

	return 0;
}

int
vidcaudio_round_blocksize(void *addr, int blk)
{

	if (blk > PAGE_SIZE)
		blk = PAGE_SIZE;
	return (blk);
}

#define ROUND(s)  ( ((int)s) & (~(PAGE_SIZE-1)) )

int
vidcaudio_start_output(void *addr, void *p, int cc, void (*intr)(void *),
    void *arg)
{

	/* I can only DMA inside 1 page */

#ifdef VIDCAUDIO_DEBUG
	printf("vidcaudio_start_output (%d) %p %p\n", cc, intr, arg);
#endif

	if (ROUND(p) != ROUND(p+cc)) {
		/*
		 * If it's over a page I can fix it up by copying it into
		 * my buffer
		 */

#ifdef VIDCAUDIO_DEBUG
		printf("vidcaudio: DMA over page boundary requested."
		    "  Fixing up\n");
#endif
		memcpy(p, (char *)ag.silence, cc > PAGE_SIZE ? PAGE_SIZE : cc);
		p = (void *)ag.silence;

		/*
		 * I can't DMA any more than that, but it is possible to
		 * fix it up by handling multiple buffers and only
		 * interrupting the audio driver after sending out all the
		 * stuff it gave me.  That it more than I can be bothered
		 * to do right now and it probablly wont happen so I'll just
		 * truncate the buffer and tell the user.
		 */

		if (cc > PAGE_SIZE) {
			printf("vidcaudio: DMA buffer truncated. I could fix this up\n");
			cc = PAGE_SIZE;
		}
	}
	vidcaudio_dma_program((vaddr_t)p, (vaddr_t)((char *)p+cc),
	    intr, arg);
	return 0;
}

int
vidcaudio_start_input(void *addr, void *p, int cc, void (*intr)(void *),
    void *arg)
{
	return EIO;
}

int
vidcaudio_halt_output(void *addr)
{

#ifdef VIDCAUDIO_DEBUG
	printf("DEBUG: vidcaudio_halt_output\n");
#endif
	return EIO;
}

int
vidcaudio_halt_input(void *addr)
{

#ifdef VIDCAUDIO_DEBUG
	printf("DEBUG: vidcaudio_halt_input\n");
#endif
	return EIO;
}

int
vidcaudio_getdev(void *addr, struct audio_device *retp)
{

	*retp = vidcaudio_device;
	return 0;
}


int
vidcaudio_set_port(void *addr, mixer_ctrl_t *cp)
{

	return EINVAL;
}

int
vidcaudio_get_port(void *addr, mixer_ctrl_t *cp)
{

	return EINVAL;
}

int
vidcaudio_query_devinfo(void *addr, mixer_devinfo_t *dip)
{

	return ENXIO;
}

int
vidcaudio_get_props(void *addr)
{

	return 0;
}
void
vidcaudio_dummy_routine(void *arg)
{

#ifdef VIDCAUDIO_DEBUG
	printf("vidcaudio_dummy_routine\n");
#endif
}

int
vidcaudio_rate(int rate)
{

	WriteWord(vidc_base, VIDC_SFR | rate);
	return 0;
}

int
vidcaudio_stereo(int channel, int position)
{

	if (channel < 0) return EINVAL;
	if (channel > 7) return EINVAL;
	channel = channel << 24 | VIDC_SIR0;
	WriteWord(vidc_base, channel | position);
	return 0;
}

#define PHYS(x, y) pmap_extract(pmap_kernel(), ((x)&L2_S_FRAME), (paddr_t *)(y))

/*
 * Program the next buffer to be used
 * This function must be re-entrant, maximum re-entrancy of 2
 */

#define FLAGS (0)

int
vidcaudio_dma_program(vaddr_t cur, vaddr_t end, void (*intr)(void *),
    void *arg)
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

		(*ag.intr)(ag.arg);		/* Schedule the next buffer */
		ag.intr = vidcaudio_dummy_routine; /* Already done this     */
		ag.arg = NULL;

#ifdef PRINT
		printf("vidcaudio: start output\n");
#endif
#ifdef VIDCAUDIO_DEBUG
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
#ifdef VIDCAUDIO_DEBUG
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
	IOMD_WRITE_WORD(IOMD_SD0ENDB,
	    (ag.silence + PAGE_SIZE - 16) | (1 << 30));
	disable_irq(sound_dma_intr);
}

int
vidcaudio_intr(void *arg)
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

#ifdef VIDCAUDIO_DEBUG
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
#ifdef VIDCAUDIO_DEBUG
		printf("i");
#endif
		(*nintr)(narg);
	}

	if (xcur == 0) {
		vidcaudio_shutdown();
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
		(*ag.intr)(ag.arg);		/* Schedule the next buffer */
		ag.intr = vidcaudio_dummy_routine; /* Already done this     */
		ag.arg = NULL;
	}
	return 0 ;	/* Pass interrupt on down the chain */
}
