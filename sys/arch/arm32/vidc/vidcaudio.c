/* $NetBSD: vidcaudio.c,v 1.1 1996/01/31 23:25:06 mark Exp $ */

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
 *
 *	$Id: vidcaudio.c,v 1.1 1996/01/31 23:25:06 mark Exp $
 */

/*
 * vidcaudio driver for RiscBSD by Melvin Tang-Richardson (c) 1995
 *
 * Interfaces with the NetBSD generic audio driver to provide SUN
 * /dev/audio (partial) compatibility.
 *
 * Do not include me in conf.c.  My interface is done by the generic
 * audio driver.  Put me in config file and files.arm32.  Do not put
 * the audio.c generic driver in them.  I do the autoconfig for it.
 *
 * The following files need to be altered
 *
 * [files.arm32]
 * device  vidcaudio at mainbus :audio
 * file    arch/arm32/mainbus/vidcaudio.c	vidcaudio needs-flag
 * major { audio=36 }
 *
 * [conf.c]
 * #include "audio.h"
 * cdev_decl(audio)
 *
 * cdev_audio_init(NAUDIO,audio)	/* 36: generic audio I/O */
 *
 * [GENERIC]
 * vidcaudio0	at mainbus? base 0x00000000
 *
 */

#include <sys/param.h>	/* proc.h */
#include <sys/types.h>  /* dunno  */
#include <sys/conf.h>   /* autoconfig functions */
#include <sys/device.h> /* device calls */
#include <sys/proc.h>	/* device calls */
#include <sys/audioio.h>
#include <sys/errno.h>
#include <sys/systm.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>

#include <dev/audio_if.h>

#include <machine/irqhandler.h>
#include <machine/iomd.h>
#include <machine/vidc.h>
#include <machine/katelib.h> /* ReadByte etc. */

#include <arm32/mainbus/mainbus.h>
#include "waveform.h"
#include "vidcaudio.h"

#undef DEBUG

struct vidcaudio_softc;

/* Note to Nut - ideally these should use the __P() macro to match the kernel style guide for prototypes.
 * Another niggle ... the function declartions need modifying to match the kernel style as well.
 * I have notfinished doing it too all my stuff yet... :-(
 */
 
int vidcaudio_intr ( void *arg );
int vidcaudio_dma_program ( vm_offset_t cur, vm_offset_t end,
			    void (*intr)(), void *arg );
void vidcaudio_dummy_routine ( void *arg );
int vidcaudio_stereo ( int channel, int position );
int vidcaudio_rate ( int rate );
void vidcaudio_shutdown ( void );
int vidcaudio_hw_attach ( struct vidcaudio_softc *sc );

struct audio_general {
    int in_sr;
    int out_sr;
    vm_offset_t silence;
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
} ag;

/* ************************************************************************* * 
 | Compatibility with /dev/beep                                              |
 * ************************************************************************* */

void vidcaudio_beep_generate ( void )
{
     vidcaudio_dma_program ( ag.silence, ag.silence+sizeof(beep_waveform)-16,
		    vidcaudio_dummy_routine, NULL );
}


/* ************************************************************************* * 
 | Kernel interface for the autoconfiguration                                |
 * ************************************************************************* */

struct vidcaudio_softc {
    struct device device;
    int iobase;

    int open;

    u_int encoding;
    int inport;
    int outport;
};

int  vidcaudio_probe ( struct device *parent, struct device *match, void *aux );
void vidcaudio_attach( struct device *parent, struct device *self, void *aux );
int  vidcaudio_open  ( dev_t dev, int flags );
void vidcaudio_close ( void *addr );

struct cfdriver vidcaudiocd = {
    NULL, "vidcaudio", vidcaudio_probe, vidcaudio_attach,
    DV_DULL, sizeof(struct vidcaudio_softc)
};

int vidcaudio_probe ( struct device *parent, struct device *match, void *aux )
{
    int id = ReadByte(IOMD_ID0) | ReadByte(IOMD_ID1)>>8;

    switch ( id )
    {
        case RPC600_IOMD_ID:
	    return (1);
    }

    return (0);
}

void vidcaudio_attach ( struct device *parent, struct device *self, void *aux )
{
    struct mainbus_attach_args *mb = aux;
    struct vidcaudio_softc *sc = (void *)self;

    sc->iobase = mb->mb_iobase;

    sc->open = 0;
    sc->encoding = 0;
    sc->inport = 0;
    sc->outport = 0;
    ag.in_sr = 24*1024;
    ag.out_sr = 24*1024;
    ag.in_progress = 0;

    ag.next_cur = 0;
    ag.next_end = 0;
    ag.next_intr = NULL;
    ag.next_arg = NULL;

    vidcaudio_rate ( 32 );

    /* Program the silence buffer and reset the DMA channel */
    ag.silence = kmem_alloc(kernel_map, NBPG);
    bzero ( (char *)ag.silence, NBPG );
    bcopy ( (char *)beep_waveform, (char *)ag.silence, sizeof(beep_waveform) );

    ag.buffer = 0;

    /* Install the irq handler for the DMA interrupt */
    ag.ih.ih_func = vidcaudio_intr;
    ag.ih.ih_arg = NULL;
    ag.ih.ih_level = IPL_NONE;

    ag.intr = NULL;
    ag.intr = NULL;

    disable_irq ( IRQ_DMASCH0 );

    if ( irq_claim(IRQ_DMASCH0, &(ag.ih)) == -1 )
	panic ( "vidcaudio: couldn't claim IRQ_DMASCH0" );

    disable_irq ( IRQ_DMASCH0 );

    vidcaudio_dma_program ( ag.silence, ag.silence+NBPG-16,
				vidcaudio_dummy_routine, NULL );

    vidcaudio_hw_attach ( sc );

#ifdef DEBUG
    printf ( " UNDER DEVELOPMENT (nuts)\n" );
#endif
}

int vidcaudio_open ( dev_t dev, int flags )
{
    struct vidcaudio_softc *sc;
    int unit = AUDIOUNIT (dev);
    int s;

#ifdef DEBUG
printf ( "DEBUG: vidcaudio_open called\n" );
#endif

    if ( unit >= vidcaudiocd.cd_ndevs )
        return ENODEV;

    sc = vidcaudiocd.cd_devs[unit];

    if (!sc)
	return ENXIO;

    s = splhigh ();

    if ( sc->open!=0 )
	return EBUSY;

    sc->open=1;
    ag.open=1;

    (void)splx(s);
    return 0;
}
 
void vidcaudio_close ( void *addr )
{
    struct vidcaudio_softc *sc = addr;

    vidcaudio_shutdown ();

#ifdef DEBUG
printf ( "DEBUG: vidcaudio_close called\n" );
#endif

    sc->open = 0;
    ag.open = 0;
}

/* ************************************************************************* * 
 | Interface to the generic audio driver                                     |
 * ************************************************************************* */

int    vidcaudio_set_in_sr       ( void *, u_long );
u_long vidcaudio_get_in_sr       ( void * );
int    vidcaudio_set_out_sr      ( void *, u_long );
u_long vidcaudio_get_out_sr      ( void * );
int    vidcaudio_query_encoding  ( void *, struct audio_encoding * );
int    vidcaudio_set_encoding	 ( void *, u_int );
int    vidcaudio_get_encoding	 ( void * );
int    vidcaudio_set_precision	 ( void *, u_int );
int    vidcaudio_getprecision	 ( void * );
int    vidcaudio_set_channels 	 ( void *, int );
int    vidcaudio_get_channels	 ( void * );
int    vidcaudio_round_blocksize ( void *, int );
int    vidcaudio_set_out_port	 ( void *, int );
int    vidcaudio_get_out_port	 ( void * );
int    vidcaudio_set_in_port	 ( void *, int );
int    vidcaudio_get_in_port  	 ( void * );
int    vidcaudio_commit_settings ( void * );
u_int  vidcaudio_get_silence	 ( int );
void   vidcaudio_sw_encode	 ( int, u_char *, int );
void   vidcaudio_sw_decode	 ( int, u_char *, int );
int    vidcaudio_start_output	 ( void *, void *, int, void (*)(), void *);
int    vidcaudio_start_input	 ( void *, void *, int, void (*)(), void *);
int    vidcaudio_halt_output	 ( void * );
int    vidcaudio_halt_input 	 ( void * );
int    vidcaudio_cont_output	 ( void * );
int    vidcaudio_cont_input	 ( void * );
int    vidcaudio_speaker_ctl	 ( void *, int );
int    vidcaudio_getdev		 ( void *, struct audio_device * );
int    vidcaudio_setfd	 	 ( void *, int );
int    vidcaudio_set_port	 ( void *, mixer_ctrl_t * );
int    vidcaudio_get_port  ( void *, mixer_ctrl_t * );
int    vidcaudio_query_devinfo	 ( void *, mixer_devinfo_t * );

struct audio_device vidcaudio_device = {
    "VidcAudio 8-bit",
    "x",
    "vidcaudio"
};

int vidcaudio_set_in_sr ( void *addr, u_long sr )
{
    ag.in_sr = sr;
    return 0;
}

u_long vidcaudio_get_in_sr ( void *addr )
{
    return ag.in_sr;
}

int vidcaudio_set_out_sr ( void *addr, u_long sr )
{
    ag.out_sr = sr;
    return 0;
}

u_long vidcaudio_get_out_sr ( void *addr )
{
    return ( ag.out_sr );
}

int vidcaudio_query_encoding ( void *addr, struct audio_encoding *fp )
{
    switch ( fp->index )
    {
        case 0:
	    strcpy (fp->name, "vidc" );
            fp->format_id = AUDIO_ENCODING_ULAW;
            break;

	default:
	    return (EINVAL);
    }
    return 0;
}

int vidcaudio_set_encoding ( void *addr, u_int enc )
{
    struct vidcaudio_softc *sc = addr;

    switch (enc)
    {
	case AUDIO_ENCODING_ULAW:
#ifdef DEBUG
	    printf ( "DEBUG: Set ulaw encoding\n" );
#endif
            sc->encoding = enc;
	    break;

        default:
	    return (EINVAL);
    }
    return 0;
}

int vidcaudio_get_encoding ( void *addr )
{
    struct vidcaudio_softc *sc = addr;
    return sc->encoding;
}

int vidcaudio_set_precision ( void *addr, u_int prec )
{
    if (prec != 8)
	return EINVAL;
    return 0;
}

int vidcaudio_get_precision ( void *addr )
{
    return (8);
}

int vidcaudio_set_channels ( void *addr, int chans )
{
    if ( chans!=1 )
	return EINVAL;

    return 0;
}

int vidcaudio_get_channels ( void *addr )
{
    return 1;
}

int vidcaudio_round_blocksize ( void *addr, int blk )
{
    if (blk>NBPG) blk=NBPG;
    return blk;
}

int vidcaudio_set_out_port ( void *addr, int port )
{
    struct vidcaudio_softc *sc = addr;
    sc->outport = port;
    return 0;
}

int vidcaudio_get_out_port ( void *addr )
{
    struct vidcaudio_softc *sc = addr;
    return sc->outport;
}

int vidcaudio_set_in_port ( void *addr, int port )
{
    struct vidcaudio_softc *sc = addr;
    sc->inport = port;
    return 0;
}

int vidcaudio_get_in_port ( void *addr )
{
    struct vidcaudio_softc *sc = addr;
    return sc->inport;
}

int vidcaudio_commit_settings ( void *addr )
{
#ifdef DEBUG
printf ( "DEBUG: committ_settings\n" );
#endif
    return 0;
}

u_int vidcaudio_get_silence ( int enc )
{
    switch (enc)
    {
	case AUDIO_ENCODING_ULAW:
	    return 0x7f;

        default:
	    return 0;
    }
    return 0;
}

void vidcaudio_sw_encode ( int e, u_char *p, int cc )
{
#ifdef DEBUG
    printf ( "DEBUG: sw_encode\n" );    
#endif
    return;
}

void vidcaudio_sw_decode ( int e, u_char *p, int cc )
{
#ifdef DEBUG
    printf ( "DEBUG: sw_decode\n" );    
#endif
}

#define ROUND(s)  ( ((int)s) & (~(NBPG-1)) )

int vidcaudio_start_output ( void *addr, void *p, int cc,
			      void (*intr)(), void *arg )
{
    /* I can only DMA inside 1 page */

#ifdef DEBUG
    printf ( "vidcaudio_start_output (%d) %08x %08x\n", cc, intr, arg );
#endif

    if ( ROUND(p) != ROUND(p+cc) )
    {
        /* If it's over a page I can fix it up by copying it into my buffer */

#ifdef DEBUG
        printf ( "vidcaudio: DMA over page boundary requested.  Fixing up\n" );
#endif
        bcopy ( (char *)ag.silence, p, cc>NBPG ? NBPG : cc );
        p=(void *)ag.silence;

 	/* I can't DMA any more than that, but it is possible to fix it up  */
        /* by handling multiple buffers and only interrupting the audio     */
        /* driver after sending out all the stuff it gave me.  That it more */
        /* than I can be bothered to do right now and it probablly wont     */
        /* happen so I'll just truncate the buffer and tell the user        */

	if ( cc>NBPG )
	{
	    printf ( "vidcaudio: DMA buffer truncated. I could fix this up\n" );
	    cc=NBPG;
	}
    }
    vidcaudio_dma_program ( (vm_offset_t)p, (vm_offset_t)(p+cc), intr, arg );
    return 0;
}

int vidcaudio_start_input ( void *addr, void *p, int cc,
			     void (*intr)(), void *arg )
{
    return EIO;
}

int vidcaudio_halt_output ( void *addr )
{
#ifdef DEBUG
    printf ( "DEBUG: vidcaudio_halt_output\n" );
#endif
    return EIO;
}

int vidcaudio_halt_input ( void *addr )
{
#ifdef DEBUG
    printf ( "DEBUG: vidcaudio_halt_output\n" );
#endif
    return EIO;
}

int vidcaudio_cont_output ( void *addr )
{
#ifdef DEBUG
    printf ( "DEBUG: vidcaudio_halt_output\n" );
#endif
    return EIO;
}

int vidcaudio_cont_input ( void *addr )
{
#ifdef DEBUG
    printf ( "DEBUG: vidcaudio_halt_output\n" );
#endif
    return EIO;
}

int vidcaudio_speaker_ctl ( void *addr, int newstate )
{
#ifdef DEBUG
    printf ( "DEBUG: vidcaudio_speaker_ctl\n" );
#endif
    return 0;
}

int vidcaudio_getdev ( void *addr, struct audio_device *retp )
{
    *retp = vidcaudio_device;
    return 0;
}


int vidcaudio_setfd ( void *addr, int flag )
{
    return ENOTTY;
}

int vidcaudio_set_port ( void *addr, mixer_ctrl_t *cp )
{
    return EINVAL;
}

int vidcaudio_get_port ( void *addr, mixer_ctrl_t *cp )
{
    return EINVAL;
}

int vidcaudio_query_devinfo ( void *addr, mixer_devinfo_t *dip )
{
    return 0;
}

struct audio_hw_if vidcaudio_hw_if = {
    vidcaudio_open,
    vidcaudio_close,
    NULL,
    vidcaudio_set_in_sr,
    vidcaudio_get_in_sr,
    vidcaudio_set_out_sr,
    vidcaudio_get_out_sr,
    vidcaudio_query_encoding,
    vidcaudio_set_encoding,
    vidcaudio_get_encoding,
    vidcaudio_set_precision,
    vidcaudio_get_precision,
    vidcaudio_set_channels,
    vidcaudio_get_channels,
    vidcaudio_round_blocksize,
    vidcaudio_set_out_port,
    vidcaudio_get_out_port,
    vidcaudio_set_in_port,
    vidcaudio_get_in_port,
    vidcaudio_commit_settings,
    vidcaudio_get_silence,
    vidcaudio_sw_encode,
    vidcaudio_sw_decode,
    vidcaudio_start_output,
    vidcaudio_start_input,
    vidcaudio_halt_output,
    vidcaudio_halt_input,
    vidcaudio_cont_output,
    vidcaudio_cont_input,
    vidcaudio_speaker_ctl,
    vidcaudio_getdev,
    vidcaudio_setfd,
    vidcaudio_set_port,
    vidcaudio_get_port,
    vidcaudio_query_devinfo,
    0, /* not full duplex */
    0
};

void vidcaudio_dummy_routine ( void *arg )
{
#ifdef DEBUG
    printf ( "vidcaudio_dummy_routine\n" );
#endif
}

int vidcaudio_hw_attach ( struct vidcaudio_softc *sc )
{
    return ( audio_hardware_attach ( &vidcaudio_hw_if, sc ) );
}

int vidcaudio_rate ( int rate )
{
    WriteWord ( VIDC_BASE, VIDC_SFR | rate );
    return 0;
}

int vidcaudio_stereo ( int channel, int position )
{
    if ( channel<0 ) return EINVAL;
    if ( channel>7 ) return EINVAL;
    channel = channel<<24 | VIDC_SIR0;
    WriteWord ( VIDC_BASE, channel|position );
    return 0;
}

#define PHYS(x) (pmap_extract( kernel_pmap, ((x)&PG_FRAME) ))

/*
 * Program the next buffer to be used
 * This function must be re-entrant, maximum re-entrancy of 2
 */

#define FLAGS (0)

int vidcaudio_dma_program ( vm_offset_t cur, vm_offset_t end,
			    void (*intr)(), void *arg )
{
    /* If there isn't a transfer in progress then start a new one */
    if ( ag.in_progress==0 )
    {
	ag.buffer = 0;
        WriteWord ( IOMD_SD0CR, 0x90 );	/* Reset State Machine */
        WriteWord ( IOMD_SD0CR, 0x30 );	/* Reset State Machine */

 	WriteWord ( IOMD_SD0CURB, PHYS(cur) );
        WriteWord ( IOMD_SD0ENDB, PHYS(end-16)|FLAGS );
 	WriteWord ( IOMD_SD0CURA, PHYS(cur) );
        WriteWord ( IOMD_SD0ENDA, PHYS(end-16)|FLAGS );

        ag.in_progress = 1;

	ag.next_cur = ag.next_end = 0;
        ag.next_intr = ag.next_arg = 0; 

	ag.intr = intr;
        ag.arg = arg;

        /* The driver 'clicks' between buffer swaps, leading me to think */
        /* that the fifo is much small than on other sound cards.  So    */
        /* so I'm going to have to do some tricks here                   */

        (*ag.intr)(ag.arg);			/* Schedule the next buffer */
	ag.intr = vidcaudio_dummy_routine;	/* Already done this        */
	ag.arg = NULL;

#ifdef PRINT
printf ( "vidcaudio: start output\n" );
#endif
#ifdef DEBUG
	printf ( "SE" );
#endif
        enable_irq ( IRQ_DMASCH0 );
    }
    else
    {
        /* Otherwise schedule the next one */
	if ( ag.next_cur!=0 )
	{
	    /* If there's one scheduled then complain */
	    printf ( "vidcaudio: Buffer already Q'ed\n" );
	    return EIO;
	}
	else
	{
	    /* We're OK to schedule it now */
            ag.buffer = (++ag.buffer) & 1;
            ag.next_cur = PHYS(cur);
            ag.next_end = PHYS(end-16);
            ag.next_intr = intr;
            ag.next_arg = arg;
#ifdef DEBUG
printf ( "s" );
#endif
	}
    }
    return 0;
}

void vidcaudio_shutdown ( void )
{
    /* Shut down the channel */
    ag.intr = NULL;
    ag.in_progress = 0;
#ifdef PRINT
printf ( "vidcaudio: stop output\n" );
#endif
    WriteWord ( IOMD_SD0CURB, ag.silence );
    WriteWord ( IOMD_SD0ENDB, (ag.silence + NBPG - 16) | (1<<30) );
    disable_irq ( IRQ_DMASCH0 );
}

int vidcaudio_intr ( void *arg )
{
    int status = ReadByte(IOMD_SD0ST);
    void (*nintr)();
    void *narg;
    void (*xintr)();
    void *xarg;
    int xcur, xend;
    WriteWord ( IOMD_DMARQ, 0x10 );

#ifdef PRINT
    printf ( "I" );
#endif

    if ( ag.open==0 )
    {
	vidcaudio_shutdown ();
	return 0;
    }

    /* Have I got the generic audio device attached */

#ifdef DEBUG
    printf ( "[B%01x]", status );
#endif

    nintr=ag.intr;
    narg=ag.arg;
    ag.intr = NULL;

    xintr=ag.next_intr;
    xarg=ag.next_arg;
    xcur=ag.next_cur;
    xend=ag.next_end;
	ag.next_cur = 0;
        ag.intr = xintr;
	ag.arg = xarg;

    if ( nintr )
    {
#ifdef DEBUG
	printf ( "i" );
#endif
	(*nintr)(narg);
    }

    if ( xcur==0 )
    {
	vidcaudio_shutdown ();
    }
    else
    {
#define OVERRUN 	(0x04)
#define INTERRUPT	(0x02)
#define BANK_A		(0x00)
#define BANK_B		(0x01)
	switch ( status&0x7 )
	{
	    case (INTERRUPT|BANK_A):
#ifdef PRINT
printf( "B" );
#endif
        	WriteWord ( IOMD_SD0CURB, xcur );
        	WriteWord ( IOMD_SD0ENDB, xend|FLAGS );
		break;

	    case (INTERRUPT|BANK_B):
#ifdef PRINT
printf( "A" );
#endif
        	WriteWord ( IOMD_SD0CURA, xcur );
        	WriteWord ( IOMD_SD0ENDA, xend|FLAGS );
		break;

	    case (OVERRUN|INTERRUPT|BANK_A):
#ifdef PRINT
printf( "A" );
#endif
        	WriteWord ( IOMD_SD0CURA, xcur );
        	WriteWord ( IOMD_SD0ENDA, xend|FLAGS );
		break;
		 
	    case (OVERRUN|INTERRUPT|BANK_B):
#ifdef PRINT
printf( "B" );
#endif
        	WriteWord ( IOMD_SD0CURB, xcur );
        	WriteWord ( IOMD_SD0ENDB, xend|FLAGS );
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

    if ( ag.next_cur==0 )
    {
        (*ag.intr)(ag.arg);			/* Schedule the next buffer */
	ag.intr = vidcaudio_dummy_routine;	/* Already done this        */
	ag.arg = NULL;
    }
    return 0;
}


