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
 *	$Id: bsd_audio.c,v 1.2 1994/04/24 01:29:56 mycroft Exp $
 */

/*
 * This is a (partially) SunOS-compatible /dev/audio driver for NetBSD.
 * This code assumes SoundBlaster type hardware, supported by the
 * code in isa/sb.c.  A major problem with this hardware is that it
 * is half-duplex.  E.g., you cannot simultaneously record and play
 * samples.  Thus, it doesn't really make sense to allow O_RDWR access.
 * However, opening and closing the device to "turn around the line"
 * is relatively expensive and costs a card reset (which can take
 * some time).  Instead, we allow O_RDWR access, and provide an
 * ioctl to set the "mode", e.g., playing or recording.  If you
 * write to the device in record mode, the data is tossed.  If you
 * read from the device in play mode, you get zero filled buffers
 * at the rate at which samples are naturally generated.
 */

#include "audio.h"
#if NAUDIO > 0

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>
#include <sys/select.h>
#include <sys/malloc.h>

#include <machine/bsd_audioio.h>

#include <i386/isa/sbreg.h>

#define AUDIODEBUG	if (audiodebug) printf
int	audiodebug = 0;

/*
 * Initial/default block size is patchable.
 */
#ifndef AUDIOBLKSIZE
#ifdef SBPRO
#define AUDIOBLKSIZE 1024	/* ~20ms at 43478 Hz */
#else
#define AUDIOBLKSIZE 160	/* 20ms at 8KHz */
#endif
#endif
int audio_blocksize = AUDIOBLKSIZE;
int audio_backlog = 3;		/* 60ms in samples */

/* XXX */
#define splaudio splclock

/*
 * Software state, per audio device.
 */
struct audio_softc {
	struct sb_softc *sc_sb;
	u_char	sc_open;		/* single use device */
	u_char	sc_mode;		/*  */
	u_char	sc_rbus;		/* input dma in progress */
	u_char	sc_pbus;		/* output dma in progress */
	u_char	sc_rulaw;
	u_char	sc_pulaw;
	u_char	sc_pad[2];
	u_long	sc_wseek;		/* timestamp of last frame written */
	u_long	sc_rseek;		/* timestamp of last frame read */
	u_long	sc_orate;		/* input sampling rate */
	u_long	sc_irate;		/* output sampling rate */
	struct	selinfo sc_wsel;	/* write selector */
	struct	selinfo sc_rsel;	/* read selector */

	int	sc_rlevel;		/* record level */
	int	sc_plevel;		/* play level */

	/*
	 * Sleep channels for reading and writing.
	 */
	int	sc_rchan;
	int	sc_wchan;

	/*
	 * Buffer management.
	 */
	u_char	*sc_hp;		/* head */
	u_char	*sc_tp;		/* tail */
	u_char	*sc_bp;		/* start of buffer */
	u_char	*sc_ep;		/* end of buffer */
	u_char	*sc_zp;		/* block of silence */

	int	sc_nblk;
	int	sc_maxblk;
	int	sc_lowat;		/* xmit low water mark (for wakeup) */
	int	sc_hiwat;		/* xmit high water mark (for wakeup) */
	int	sc_blksize;		/* recv block (chunk) size */
	int	sc_backlog;		/* # blks of xmit backlog to gen. */

	int	sc_rblks;	/* number of phantom record blocks */
};

/* XXX */
struct sb_softc *sbopen();

static int audio_default_level = 150;

static void ausetrgain(struct audio_softc *, int);
static void ausetpgain(struct audio_softc *, int);
static int audiosetinfo(struct audio_softc *, struct audio_info *);
static int audiogetinfo(struct audio_softc *, struct audio_info *);

struct audio_softc audio_softc;

void audio_init_record(struct audio_softc *);
void audio_init_play(struct audio_softc *);
void audiostartr(struct audio_softc *);
void audiostartp(struct audio_softc *);
void audio_rint(struct audio_softc *);
void audio_pint(struct audio_softc *);

void audio_tomulaw(register u_char *, register int);
void audio_frommulaw(register u_char *, register int);

audio_initbuf(struct audio_softc *sc)
{
	register int nblk = NBPG / sc->sc_blksize;

	sc->sc_ep = sc->sc_bp + nblk * sc->sc_blksize;
	sc->sc_hp = sc->sc_tp = sc->sc_bp;
	sc->sc_maxblk = nblk;
	sc->sc_nblk = 0;
	sc->sc_lowat = 1;
	sc->sc_hiwat = nblk - sc->sc_lowat;
}

static inline int
audio_sleep(int *chan)
{
	int st;
	*chan = 1;
	st = (tsleep((caddr_t)chan, PWAIT | PCATCH, "audio", 0));
	*chan = 0;
	return (st);
}

static inline void
audio_wakeup(int *chan)
{
	if (*chan) {
		wakeup((caddr_t)chan);
		*chan = 0;
	}
}

/*XXX*/
int auzero[1024];

void
audioattach(int unused)
{
	AUDIODEBUG("audio: attach\n");
}

int
audioopen(dev_t dev, int flags, int ifmt, struct proc *p)
{
	register struct audio_softc *sc = &audio_softc;
	int s;

	AUDIODEBUG("audio: open\n");
	if (sc->sc_open != 0 || (sc->sc_sb = sbopen()) == 0)
		return (EBUSY);
	sc->sc_open = 1;

	/*
	 * Allocate a single page so it won't cross a page boundary.
	 * This way the dma carried out in the sb module will be efficient
	 * (i.e., at_dma() won't have to make a copy)
	 */
	sc->sc_zp = malloc(NBPG, M_DEVBUF, M_WAITOK);
	if (sc->sc_zp == 0)
		goto nobufs;
	sc->sc_bp = malloc(NBPG, M_DEVBUF, M_WAITOK);
	if (sc->sc_bp == 0) {
		free(sc->sc_zp, M_DEVBUF);
		goto nobufs;
	}
	sc->sc_blksize = audio_blocksize;
	sc->sc_backlog = audio_backlog;

	audio_initbuf(sc);
	
	/* nothing read or written yet */
	sc->sc_rseek = 0;
	sc->sc_wseek = 0;

	sc->sc_rchan = 0;
	sc->sc_wchan = 0;

	/*
	 * Here's a hack: do ulaw conversion if high bit of 
	 * minor device is set.  That way, we can have /dev/audio
	 * (minor 0x80) do ulaw conversion,  and /dev/sound or
	 * whatever, do linear.
	 */
	if (minor(dev) & 0x80) {
		/* /dev/audio */
		int i;
		sc->sc_pulaw = sc->sc_rulaw = 1;
		sc->sc_orate = 8000;
		sc->sc_irate = 8000;
		for (i = NBPG / 4; --i >= 0; ) {
			((u_long *)sc->sc_zp)[i] = 0x7f7f7f7f;
			auzero[i] = 0x80808080;
		}
	} else {
		/* /dev/sound */
		sc->sc_pulaw = sc->sc_rulaw = 0;
#ifdef SBPRO
		sc->sc_orate = 43478;
		sc->sc_irate = 43478;
#else
#ifdef notdef
		sc->sc_orate = 14925;
		sc->sc_irate = 14925;
#endif
		sc->sc_orate = 8000;
		sc->sc_irate = 8000;
#endif
		bzero(sc->sc_zp, NBPG);
	}
	ausetrgain(sc, audio_default_level);
	ausetpgain(sc, audio_default_level);

	/* XXX */
	s = splaudio();

	sc->sc_rbus = 0;
	sc->sc_pbus = 0;
	if ((flags & FREAD) != 0) {
		audio_init_record(sc);
		audiostartr(sc);
	} else {
		audio_init_play(sc);
		audio_pint(sc);
	}
	splx(s);

	return (0);
nobufs:
	sbclose(sc->sc_sb);
	sc->sc_open = 0;
	return (ENOBUFS);
}

audio_to(caddr_t arg)
{
	wakeup(arg);
}

/*
 * Wait a little while because doing certain things to 
 * the soundblaster (like toggling the speaker) make
 * it go away for a while.
 */
void
audio_pause(struct audio_softc *sc)
{
	extern int hz;
	timeout(audio_to, audio_to, hz / 8);
	(void)tsleep((caddr_t)audio_to, PWAIT, "audio", 0);
}

/*
 * Must be called from task context.
 */
void
audio_init_record(struct audio_softc *sc)
{
	register int s = splaudio();

	sc->sc_mode = AUMODE_RECORD;
	(void)sb_set_sr(sc->sc_sb, &sc->sc_irate, SB_INPUT_RATE);
	sb_spkroff(sc->sc_sb);
	audio_pause(sc);
	splx(s);
}

/*
 * Must be called from task context.
 */
void
audio_init_play(struct audio_softc *sc)
{
	register int s = splaudio();

	sc->sc_mode = AUMODE_PLAY;
	sc->sc_rblks = 0;
	(void)sb_set_sr(sc->sc_sb, &sc->sc_orate, SB_OUTPUT_RATE);
	sb_spkron(sc->sc_sb);
	audio_pause(sc);
	splx(s);
}

static int
audio_drain(sc)
	register struct audio_softc *sc;
{
	register int error;

	while (sc->sc_nblk > 0) {
		error = audio_sleep(&sc->sc_wchan);
		if (error != 0)
			return (error);
	}
	return (0);
}

/*
 * Close an audio chip.
 */
/* ARGSUSED */
int
audioclose(dev_t dev, int flags, int ifmt, struct proc *p)
{
	register struct audio_softc *sc = &audio_softc;
	register struct aucb *cb;
	register int s;

	AUDIODEBUG("audio: close\n");
	/*
	 * Block until output drains, but allow ^C interrupt.
	 */
	sc->sc_lowat = 0;	/* avoid excessive wakeups */
	s = splaudio();
	/*
	 * If there is pending output, let it drain (unless
	 * the output is paused).
	 */
	if (sc->sc_pbus && sc->sc_nblk > 0)
		(void)audio_drain(sc);

	sbclose(sc->sc_sb);
	splx(s);
	free(sc->sc_bp, M_DEVBUF);
	free(sc->sc_zp, M_DEVBUF);
	sc->sc_open = 0;
	return (0);
}

int
audioread(dev_t dev, struct uio *uio, int ioflag)
{
	register struct audio_softc *sc = &audio_softc;
	register u_char *hp;
	register int blocksize = sc->sc_blksize;
	register int error, s;

	if (uio->uio_resid == 0)
		return (0);
	if (uio->uio_resid < blocksize)
		return (EINVAL);

	if (sc->sc_mode == AUMODE_PLAY) {
		/*
		 * If we're in play mode, return silence blocks
		 * based on the number of blocks we have output.
		 */
		do {
			s = splaudio();
			while (sc->sc_rblks <= 0) {
				if (ioflag & IO_NDELAY) {
					splx(s);
					return (EWOULDBLOCK);
				}
				error = audio_sleep(&sc->sc_rchan);
				if (error != 0) {
					splx(s);
					return (error);
				}
			}
			splx(s);
			/*XXX handle ulaw 0 */
			error = uiomove(sc->sc_zp, blocksize, uio);
			if (error)
				break;
			--sc->sc_rblks;
		} while (uio->uio_resid >= blocksize);
		return (error);
	}
	error = 0;
	do {
		while (sc->sc_nblk <= 0) {
			if (ioflag & IO_NDELAY) {
				error = EWOULDBLOCK;
				return (error);
			}
			s = splaudio();
			if (!sc->sc_rbus)
				audiostartr(sc);
			error = audio_sleep(&sc->sc_rchan);
			splx(s);
			if (error != 0)
				return (error);
		}
		hp = sc->sc_hp;
		if (sc->sc_rulaw)
			audio_tomulaw(hp, blocksize);
		error = uiomove(hp, blocksize, uio);
		if (error)
			break;
		hp += blocksize;
		if (hp >= sc->sc_ep)
			hp = sc->sc_bp;
		sc->sc_hp = hp;
		--sc->sc_nblk;
	} while (uio->uio_resid >= blocksize);

	return (error);
}

void
audio_clear(struct audio_softc *sc)
{
	register int s = splaudio();

	if (sc->sc_rbus || sc->sc_pbus) {
		sb_haltdma(sc->sc_sb);
		sc->sc_rbus = 0;
		sc->sc_pbus = 0;
	}
	sc->sc_nblk = 0;
	sc->sc_hp = sc->sc_tp = sc->sc_bp;
	splx(s);
}

int
audiowrite(dev_t dev, struct uio *uio, int ioflag)
{
	register struct audio_softc *sc = &audio_softc;
	register u_char *tp;
	register int error, s, cc;
	register int blocksize = sc->sc_blksize;

	/*
	 * If currently recording, throw away data.
	 */
	if (sc->sc_mode != AUMODE_PLAY) {
		uio->uio_offset += uio->uio_resid;
		uio->uio_resid = 0;
		return (0);
	}
	error = 0;

	while (uio->uio_resid > 0) {
		register int watermark = sc->sc_hiwat;
		s = splaudio();
		while (sc->sc_nblk > watermark) {
			if (ioflag & IO_NDELAY) {
				splx(s);
				error = EWOULDBLOCK;
				return (error);
			}
			error = audio_sleep(&sc->sc_wchan);
			if (error != 0) {
				splx(s);
				return (error);
			}
			watermark = sc->sc_lowat;
		}
		splx(s);
		if (sc->sc_nblk == 0 && uio->uio_resid <= blocksize) {
			/*
			 * the write is 'small', the buffer is empty
			 * and we have been silent for at least 50ms
			 * so we might be dealing with an application
			 * that writes frames synchronously with
			 * reading them.  If so, we need an output
			 * backlog to cover scheduling delays or
			 * there will be gaps in the sound output.
			 * Also take this opportunity to reset the
			 * buffer pointers in case we ended up on
			 * a bad boundary (odd byte, blksize bytes
			 * from end, etc.).
			 */
			s = splaudio();/*XXX*/
			sc->sc_hp = sc->sc_bp;
			bzero(sc->sc_hp, 3 * blocksize);
			sc->sc_nblk = 3;
			sc->sc_tp = sc->sc_hp + 3 * blocksize;
			splx(s);
		}
		tp = sc->sc_tp;
		cc = uio->uio_resid;
		if (cc < blocksize) {
			error = uiomove(tp, cc, uio);
			if (error)
				break;
			tp += cc;
			cc = blocksize - cc;
			while (--cc >= 0)
				*tp++ = 0x7f;
		} else {
			error = uiomove(tp, blocksize, uio);
			if (error)
				break;
			tp += blocksize;
		}
		if (sc->sc_pulaw)
			audio_frommulaw(sc->sc_tp, blocksize);
		if (tp >= sc->sc_ep)
			tp = sc->sc_bp;
		sc->sc_tp = tp;
		++sc->sc_nblk;
		/*
		 * If output isn't active, start it up.
		 */
		s = splaudio();
		if (sc->sc_pbus == 0)
			audiostartp(sc);
		splx(s);
	}
	return (error);
}

/* Sun audio compatibility */
struct sun_audio_prinfo {
	u_int	sample_rate;
	u_int	channels;
	u_int	precision;
	u_int	encoding;
	u_int	gain;
	u_int	port;
	u_int	reserved0[4];
	u_int	samples;
	u_int	eof;
	u_char	pause;
	u_char	error;
	u_char	waiting;
	u_char	reserved1[3];
	u_char	open;
	u_char	active;
};
struct sun_audio_info {
	struct sun_audio_prinfo play;
	struct sun_audio_prinfo record;
	u_int monitor_gain;
	u_int reserved[4];
};

int
audioioctl(dev_t dev, int cmd, caddr_t addr, int flag, struct proc *p)
{
	register struct audio_softc *sc = &audio_softc;
	int error = 0, i, s;

	AUDIODEBUG("audio: ioctl(0x%x)\n", cmd);
	switch (cmd) {

	case AUDIO_FLUSH:
		AUDIODEBUG("AUDIO_FLUSH\n");
		audio_clear(sc);
		if (sc->sc_mode != AUMODE_PLAY)
			audiostartr(sc);
		break;

#ifdef notdef
	/*
	 * Number of read samples dropped.  We don't know where or
	 * when they were dropped.
	 */
	case AUDIO_RERROR:
		*(int *)addr = sc->sc_au.au_rb.cb_drops != 0;
		break;

	/*
	 * How many samples will elapse until mike hears the first
	 * sample of what we last wrote?
	 */
	case AUDIO_WSEEK:
		s = splaudio();
		*(u_long *)addr = sc->sc_wseek - sc->sc_au.au_stamp
				  + AUCB_LEN(&sc->sc_au.au_rb);
		splx(s);
		break;
#endif

	case AUDIO_SETINFO:
		AUDIODEBUG("AUDIO_SETINFO\n");
		error = audiosetinfo(sc, (struct audio_info *)addr);
		break;

	case AUDIO_GETINFO:
		AUDIODEBUG("AUDIO_GETINFO\n");
		error = audiogetinfo(sc, (struct audio_info *)addr);
		break;

	case AUDIO_DRAIN:
		AUDIODEBUG("AUDIO_DRAIN\n");
		error = audio_drain(sc);
		break;

	default:
		AUDIODEBUG("audio: unknown ioctl\n");
		error = EINVAL;
		break;
	}
	AUDIODEBUG("audio: ioctl(%d) result %d\n", cmd, error);
	return (error);
}

int
audioselect(dev_t dev, int rw, struct proc *p)
{
	register struct audio_softc *sc = &audio_softc;
	register int s = splaudio();

	switch (rw) {

	case FREAD:
		if (sc->sc_mode == AUMODE_PLAY) {
			if (sc->sc_rblks > 0) {
				splx(s);
				return (1);
			}
		} else if (sc->sc_nblk > 0) {
			splx(s);
			return (1);
		}
		selrecord(p, &sc->sc_rsel);
		break;

	case FWRITE:
		/*
		 * Can write if we're recording because it gets preempted.
		 * Otherwise, can write when below low water.
		 * XXX this won't work right if we're in 
		 * record mode -- we need to note that a write
		 * select has happed and flip the speaker.
		 */
		if (sc->sc_mode != AUMODE_PLAY ||
		    sc->sc_nblk < sc->sc_lowat) {
			splx(s);
			return (1);
		}
		selrecord(p, &sc->sc_wsel);
		break;
	}
	splx(s);
	return (0);
}

void
audiostartr(struct audio_softc *sc)
{
	sb_dma_input(sc->sc_sb, sc->sc_tp, sc->sc_blksize,
		     audio_rint, (void *)sc);
	sc->sc_rbus = 1;
}

void
audiostartp(struct audio_softc *sc)
{
	/*XXX check for nblk == 0 */
	sb_dma_output(sc->sc_sb, sc->sc_hp, sc->sc_blksize,
		      audio_pint, (void *)sc);
	sc->sc_pbus = 1;
}

void
audio_pint(struct audio_softc *sc)
{
	register u_char *hp;
	register int cc = sc->sc_blksize;

	if (sc->sc_nblk > 0) {
		--sc->sc_nblk;
		hp = sc->sc_hp;
		hp += cc;
		if (hp >= sc->sc_ep)
			hp = sc->sc_bp;
		sc->sc_hp = hp;
		sb_dma_output(sc->sc_sb, hp, cc, audio_pint, (void *)sc);
	} else {
		sb_dma_output(sc->sc_sb, auzero, cc,
			      audio_pint, (void *)sc);
	}
	++sc->sc_rblks;

	if (sc->sc_mode == AUMODE_PLAY) {
		if (sc->sc_nblk <= sc->sc_lowat) {
			audio_wakeup(&sc->sc_wchan);
			selwakeup(&sc->sc_wsel);
		}
	}
	/*XXX*/
	audio_wakeup(&sc->sc_rchan);
	selwakeup(&sc->sc_rsel);
}

/*
 * Called from sb module on completion of dma input.
 * Copy the input frame into the ring buffer at the
 * current position.  Do a wakeup if necessary.
 */
void
audio_rint(struct audio_softc *sc)
{
	register u_char *tp;
	register int cc = sc->sc_blksize;

	tp = sc->sc_tp;
	tp += cc;
	if (tp >= sc->sc_ep)
		tp = sc->sc_bp;
	if (++sc->sc_nblk < sc->sc_maxblk)
		sb_dma_input(sc->sc_sb, tp, cc, audio_rint, (void *)sc);
	else
		sc->sc_rbus = 0;
	sc->sc_tp = tp;

	audio_wakeup(&sc->sc_rchan);
	selwakeup(&sc->sc_rsel);
}

static void
ausetrgain(sc, level)
	register struct audio_softc *sc;
	register int level;
{
#ifdef SBPRO
	/* XXX */
#endif
}

/*
 * XXX Looks like we need a pro to do volume control...
 */
static void
ausetpgain(sc, level)
	register struct audio_softc *sc;
	register int level;
{
#ifdef SBPRO
	/* XXX */
#endif
}

static int
audiosetinfo(sc, ai)
	struct audio_softc *sc;
	struct audio_info *ai;
{
	struct audio_prinfo *r = &ai->record, *p = &ai->play;
	register int cleared = 0;
	register int s, bsize;

	if (p->gain != ~0)
		ausetpgain(sc, p->gain);
	if (r->gain != ~0)
		ausetrgain(sc, r->gain);

	if (p->sample_rate != ~0) {
		if (!cleared)
			audio_clear(sc);
		cleared = 1;
		p->sample_rate = sb_round_sr(p->sample_rate, SB_OUTPUT_RATE);
		sc->sc_orate = p->sample_rate;
		if (sc->sc_mode == AUMODE_PLAY)
			(void)sb_set_sr(sc->sc_sb, &sc->sc_orate,
					SB_OUTPUT_RATE);
	}
	if (r->sample_rate != ~0) {
		if (!cleared)
			audio_clear(sc);
		cleared = 1;
		r->sample_rate = sb_round_sr(r->sample_rate, SB_INPUT_RATE);
		sc->sc_irate = r->sample_rate;
		if (sc->sc_mode != AUMODE_PLAY)
			(void)sb_set_sr(sc->sc_sb, &sc->sc_irate,
					SB_INPUT_RATE);
	}
	if (p->encoding != ~0) {
		if (!cleared)
			audio_clear(sc);
		cleared = 1;
		if (p->encoding == AUDIO_ENCODING_ULAW)
			sc->sc_pulaw = 1;
		else {
			sc->sc_pulaw = 0;
			p->encoding = AUDIO_ENCODING_LINEAR;
		}
	}
	if (r->encoding != ~0) {
		if (r->encoding == AUDIO_ENCODING_ULAW)
			sc->sc_rulaw = 1;
		else {
			r->encoding = AUDIO_ENCODING_LINEAR;
			sc->sc_rulaw = 0;
		}
	}
#ifdef notdef
	if (p->pause != (u_char)~0)
		sc->sc_au.au_wb.cb_pause = p->pause;
	if (r->pause != (u_char)~0)
		sc->sc_au.au_rb.cb_pause = r->pause;
#endif
	if (ai->blocksize != ~0) {
		if (!cleared)
			audio_clear(sc);
		cleared = 1;
		if (ai->blocksize == 0)
			bsize = audio_blocksize;
		else if (ai->blocksize > NBPG/2)
			bsize = NBPG/2;
		else
			bsize = ai->blocksize;
		ai->blocksize = sc->sc_blksize = bsize;
		audio_initbuf(sc);
	}

	if (ai->hiwat != ~0) {
		if ((unsigned)ai->hiwat > sc->sc_maxblk)
			ai->hiwat = sc->sc_maxblk;
		sc->sc_hiwat = ai->hiwat;
	}
	if (ai->lowat != ~0) {
		if ((unsigned)ai->lowat > sc->sc_maxblk)
			ai->lowat = sc->sc_maxblk;
		sc->sc_lowat = ai->lowat;
	}
	if (ai->backlog != ~0) {
		if ((unsigned)ai->backlog > (sc->sc_maxblk/2))
			ai->backlog = sc->sc_maxblk/2;
		sc->sc_backlog = ai->backlog;
	}
	if (ai->mode != ~0) {
		if (!cleared)
			audio_clear(sc);
		cleared = 1;
		sc->sc_mode = ai->mode;
		if (sc->sc_mode == AUMODE_PLAY)
			audio_init_play(sc);
		else 
			audio_init_record(sc);
	}
	if (cleared) {
		if (sc->sc_mode != AUMODE_PLAY)
			audiostartr(sc);
		else
			audiostartp(sc);
	}
			
	return (0);
}

static int
audiogetinfo(sc, ai)
	struct audio_softc *sc;
	struct audio_info *ai;
{
	struct audio_prinfo *r = &ai->record, *p = &ai->play;

	p->sample_rate = sc->sc_orate;
	r->sample_rate = sc->sc_irate;
	p->channels = r->channels = 1;
	p->precision = r->precision = 8;
	p->encoding = sc->sc_pulaw ? AUDIO_ENCODING_ULAW :
		AUDIO_ENCODING_LINEAR;
	r->encoding = sc->sc_rulaw ? AUDIO_ENCODING_ULAW :
		AUDIO_ENCODING_LINEAR;

	ai->monitor_gain = 0;
	r->gain = sc->sc_rlevel;
	p->gain = sc->sc_plevel;
	r->port = 1; p->port = AUDIO_SPEAKER;

#ifdef notdef
	p->pause = sc->sc_au.au_wb.cb_pause;
	r->pause = sc->sc_au.au_rb.cb_pause;
	p->error = sc->sc_au.au_wb.cb_drops != 0;
	r->error = sc->sc_au.au_rb.cb_drops != 0;
#endif
	p->open = sc->sc_open;
	r->open = sc->sc_open;

#ifdef notdef
	p->samples = sc->sc_au.au_stamp - sc->sc_au.au_wb.cb_pdrops;
	r->samples = sc->sc_au.au_stamp - sc->sc_au.au_rb.cb_pdrops;
#endif

	p->seek = sc->sc_wseek;
	r->seek = sc->sc_rseek;

	ai->blocksize = sc->sc_blksize;
	ai->hiwat = sc->sc_hiwat;
	ai->lowat = sc->sc_lowat;
	ai->backlog = sc->sc_backlog;
	ai->mode = sc->sc_mode;

	return (0);
}
u_char mulawtolin[256] = {
	128, 4, 8, 12, 16, 20, 24, 28, 
	32, 36, 40, 44, 48, 52, 56, 60, 
	64, 66, 68, 70, 72, 74, 76, 78, 
	80, 82, 84, 86, 88, 90, 92, 94, 
	96, 97, 98, 99, 100, 101, 102, 103, 
	104, 105, 106, 107, 108, 109, 110, 111, 
	112, 112, 113, 113, 114, 114, 115, 115, 
	116, 116, 117, 117, 118, 118, 119, 119, 
	120, 120, 120, 121, 121, 121, 121, 122, 
	122, 122, 122, 123, 123, 123, 123, 124, 
	124, 124, 124, 124, 125, 125, 125, 125, 
	125, 125, 125, 125, 126, 126, 126, 126, 
	126, 126, 126, 126, 126, 126, 126, 126, 
	127, 127, 127, 127, 127, 127, 127, 127, 
	127, 127, 127, 127, 127, 127, 127, 127, 
	127, 127, 127, 127, 127, 127, 127, 127, 
	255, 251, 247, 243, 239, 235, 231, 227, 
	223, 219, 215, 211, 207, 203, 199, 195, 
	191, 189, 187, 185, 183, 181, 179, 177, 
	175, 173, 171, 169, 167, 165, 163, 161, 
	159, 158, 157, 156, 155, 154, 153, 152, 
	151, 150, 149, 148, 147, 146, 145, 144, 
	143, 143, 142, 142, 141, 141, 140, 140, 
	139, 139, 138, 138, 137, 137, 136, 136, 
	135, 135, 135, 134, 134, 134, 134, 133, 
	133, 133, 133, 132, 132, 132, 132, 131, 
	131, 131, 131, 131, 130, 130, 130, 130, 
	130, 130, 130, 130, 129, 129, 129, 129, 
	129, 129, 129, 129, 129, 129, 129, 129, 
	128, 128, 128, 128, 128, 128, 128, 128, 
	128, 128, 128, 128, 128, 128, 128, 128, 
	128, 128, 128, 128, 128, 128, 128, 128, 
};
u_char lintomulaw[256] = {
	0, 0, 0, 0, 0, 1, 1, 1, 
	1, 2, 2, 2, 2, 3, 3, 3, 
	3, 4, 4, 4, 4, 5, 5, 5, 
	5, 6, 6, 6, 6, 7, 7, 7, 
	7, 8, 8, 8, 8, 9, 9, 9, 
	9, 10, 10, 10, 10, 11, 11, 11, 
	11, 12, 12, 12, 12, 13, 13, 13, 
	13, 14, 14, 14, 14, 15, 15, 15, 
	15, 16, 16, 17, 17, 18, 18, 19, 
	19, 20, 20, 21, 21, 22, 22, 23, 
	23, 24, 24, 25, 25, 26, 26, 27, 
	27, 28, 28, 29, 29, 30, 30, 31, 
	31, 32, 33, 34, 35, 36, 37, 38, 
	39, 40, 41, 42, 43, 44, 45, 46, 
	47, 48, 50, 52, 54, 56, 58, 60, 
	62, 65, 69, 73, 77, 83, 91, 103, 
	255, 231, 219, 211, 205, 201, 197, 193, 
	190, 188, 186, 184, 182, 180, 178, 176, 
	175, 174, 173, 172, 171, 170, 169, 168, 
	167, 166, 165, 164, 163, 162, 161, 160, 
	159, 159, 158, 158, 157, 157, 156, 156, 
	155, 155, 154, 154, 153, 153, 152, 152, 
	151, 151, 150, 150, 149, 149, 148, 148, 
	147, 147, 146, 146, 145, 145, 144, 144, 
	143, 143, 143, 143, 142, 142, 142, 142, 
	141, 141, 141, 141, 140, 140, 140, 140, 
	139, 139, 139, 139, 138, 138, 138, 138, 
	137, 137, 137, 137, 136, 136, 136, 136, 
	135, 135, 135, 135, 134, 134, 134, 134, 
	133, 133, 133, 133, 132, 132, 132, 132, 
	131, 131, 131, 131, 130, 130, 130, 130, 
	129, 129, 129, 129, 128, 128, 128, 128, 
};

void
audio_tomulaw(register u_char *p, register int cc)
{
	register u_char *utab = lintomulaw;
	while (--cc >= 0) {
		*p = utab[*p];
		++p;
	}
}

void
audio_frommulaw(register u_char *p, register int cc)
{
	register u_char *utab = mulawtolin;
	while (--cc >= 0) {
		*p = utab[*p];
		++p;
	}
}

#endif
