/*
 * Copyright (c) 1993 Christian E. Hopps
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Christian E. Hopps.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
#include "cc.h"
#include "cc_audio.h"

/* Audio */
struct audio_channel {
    u_word  play_count;
};

/* - channel[4] */
/* the data for each audio channel and what to do with it. */
static struct audio_channel channel[4];

/* audio vbl node for vbl function  */
struct vbl_node audio_vbl_node;    

/*--------------------------------------------------
 * Audio Interrupt Handler
 *--------------------------------------------------*/
void
audio_handler (void)
{
    u_word audio_dma = custom.dmaconr;
    u_word disable_dma = 0;
    int i;

    /* only check channels who have DMA enabled. */
    audio_dma &= (DMAF_AUD0|DMAF_AUD1|DMAF_AUD2|DMAF_AUD3);

    /* disable all audio interupts with DMA set */
    custom.intena = (audio_dma << 7);

    /* if no audio dma enabled then exit quick. */
    if (!audio_dma) {
	custom.intreq = INTF_AUD0|INTF_AUD1|INTF_AUD2|INTF_AUD3; /* clear all interrupts. */
	goto end_int;				  /* cleanup. */
    }

    for (i = 0; i < 4; i++) {
	u_word flag = (1 << i);
	u_word ir = custom.intreqr;
	/* check if this channel's interrupt is set */
	if (ir & (flag << 7)) {
	    if (channel[i].play_count) {
		channel[i].play_count--;
	    } else {
		/* disable DMA to this channel. */
		custom.dmacon = flag;
		
		/* disable interrupts to this channel. */
		custom.intena = (flag << 7);
	    }
	    custom.intreq = (flag << 7);	  /* clear this channels interrupt. */
	}
    }

  end_int:
    /* enable audio interupts with dma still set. */
    audio_dma = custom.dmaconr;
    audio_dma &= (DMAF_AUD0|DMAF_AUD1|DMAF_AUD2|DMAF_AUD3);
    custom.intena = INTF_SETCLR| (audio_dma << 7);
}

/*--------------------------------------------------
 * Audio Init
 *--------------------------------------------------*/
void
cc_init_audio (void)
{
    int i;

    /* disable all audio interupts */
    custom.intena = INTF_AUD0|INTF_AUD1|INTF_AUD2|INTF_AUD3;

    /* initialize audio channels to off. */
    for (i=0; i < 4; i++) {
	channel[i].play_count = 0;
    };
}

/*--------------------------------------------------
 * play_sample (len, data, period, volume,
 *              channels, count);
 * play_sample (u_word, u_word *, u_word, u_byte,
 *              u_byte, u_long);
 *
 * play the ``len'' of samples pointed to by ``data''
 * with ``period'' at ``volume''  on ``channels''
 * for ``count'' number of times.
 *
 * len  - number of uwords that make up the data.
 * data - the actual words of audio data.
 * period - the period between sample fetch.
 * volume - the decbel level to play the sound at.
 * channels  - bits 0-3 set for which channels to use.
 * count -  the number of times the sound will repeat.
 *--------------------------------------------------*/
void
play_sample (u_word len, u_word *data, u_word period, u_word volume, u_word channels, u_long count)
{
    u_word dmabits = channels & 0xf;
    u_word ch;

    custom.dmacon = dmabits;			  /* turn off the correct channels */

    /* load the channels */
    for (ch = 0; ch < 4; ch++) {
	if (dmabits & (ch << ch)) {
	    custom.aud[ch].len = len;
	    custom.aud[ch].lc = data;
	    custom.aud[ch].per = period;
	    custom.aud[ch].vol = volume;
	    channel[ch].play_count = count;
	}
    }
    custom.intena = INTF_SETCLR | (dmabits << 7); /* turn on interrupts for channels */
    custom.dmacon = DMAF_SETCLR | dmabits;	  /* turn on the correct channels */
}


