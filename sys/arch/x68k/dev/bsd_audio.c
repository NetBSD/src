/*	$NetBSD: bsd_audio.c,v 1.4 1998/01/12 21:13:41 thorpej Exp $	*/

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
 */

/*
 * This is a (partially) SunOS-compatible /dev/audio driver for NetBSD.
 *
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
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/select.h>
#include <sys/malloc.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/pte.h>

extern u_int	kvtop __P((register caddr_t addr));
int	uiomove __P((caddr_t cp, int n, struct uio *uio));

#include <x68k/dev/bsd_audiovar.h>
#include <x68k/dev/bsd_audioreg.h>
#include <machine/bsd_audioio.h>
#include <x68k/x68k/iodevice.h>

#define AUDIODEBUG	if (audiodebug) printf
int	audiodebug = 0;

#define dma3	(IODEVbase->io_dma[3])

/*
 * Initial/default block size is patchable.
 */
int audio_blocksize = DEFBLKSIZE;
int audio_backlog = 400;		/* 50ms in samples */

/*
 * Software state, per MSM6258V audio chip.
 */
struct audio_softc {
	struct adpcm_softc sc_adpcm;
	u_char	sc_open;		/* single use device */
	u_char	sc_mode;		/*  */
	u_char	sc_rbus;		/* input dma in progress */
	u_char	sc_pbus;		/* output dma in progress */
	u_char	sc_rencoding;
	u_char	sc_pencoding;
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
	int     sc_ochan;

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
	int	sc_finish;

	/* sc_au is special in that the hardware interrupt handler uses it */
	int	sc_rblks;	/* number of phantom record blocks */
} audio_softc[NAUDIO];

/* forward declarations */
inline static int audio_sleep __P((int *));

/* autoconfiguration driver */
int	audioattach();

static int audio_default_level = 150;

static void ausetrgain __P((struct audio_softc *, int));
static void ausetpgain __P((struct audio_softc *, int));
static int audiosetinfo __P((struct audio_softc *, struct audio_info *));
static int audiogetinfo __P((struct audio_softc *, struct audio_info *));

struct sun_audio_info;

void audio_init_record __P((struct audio_softc *));
void audio_init_play __P((struct audio_softc *));
void audiostartr __P((struct audio_softc *));
void audiostartp __P((struct audio_softc *));
inline void audio_rint __P((struct audio_softc *));
inline void audio_pint __P((struct audio_softc *));

void audio_tomulaw __P((short *, register int));
void audio_frommulaw __P((u_char *, register int));
void audio_tolinear __P((struct audio_softc *, register u_char *, register int));
void audio_fromlinear __P((struct audio_softc *, register u_char *, register int));


void
audio_initbuf(sc)
	struct audio_softc *sc;
{
	register int nblk = NBPG / sc->sc_blksize;

	sc->sc_ep = sc->sc_bp + nblk * sc->sc_blksize;
	sc->sc_hp = sc->sc_tp = sc->sc_bp;
	sc->sc_maxblk = nblk;
	sc->sc_nblk = 0;
	sc->sc_lowat = 3;
	sc->sc_hiwat = nblk - 1;
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
audio_wakeup(chan)
	int *chan;
{
	if (*chan) {
		wakeup((caddr_t)chan);
		*chan = 0;
	}
}

/*XXX*/
int auzero[NBPG/4];
short transbuf[NBPG*2];
u_char transbuf2[NBPG];

int	audiomatch __P((struct device *, void *, void *));
void	audioattach __P((struct device *, struct device *, void *));

struct cfattach audio_ca = {
	sizeof(struct audio_softc), audiomatch, audioattach
};

int
audiomatch(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct cfdata *cf = match;

	if (strcmp(aux, "adpcm") || cf->cf_unit > 0)
		return 0;
	return 1;
}

void
audioattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct dmac *dmac = &IODEVbase->io_dma[3];	
	register struct audio_softc *sc = &audio_softc[0];

	dmac->csr = 0xff;
	dmac->dcr = 0x80;
	dmac->ocr = 0x32;
	dmac->scr = 0x04;
	dmac->mfc = 0x05;
	dmac->dfc = 0x05;
	dmac->bfc = 0x05;
	dmac->dar = (unsigned long)kvtop(&(adpcm.data));
	dmac->niv = 0x6a;
	dmac->eiv = 0x6b;

	sc->sc_rbus = 0;
	sc->sc_pbus = 0;

	/*printf("audio0: MSM6258V ADPCM chip.\n");*/
}

int
audioopen(dev, flags, ifmt, p)
	dev_t dev;
	int flags;
	int ifmt;
	struct proc *p;
{
	register struct audio_softc *sc = &audio_softc[0];
	int s, i;
	int unit = minor(dev);
	int error;

	AUDIODEBUG("audio: open\n");
	if((unit & 0x0000003f) >= NAUDIO)
		return (ENXIO);
	if (sc->sc_open != 0)
		return (EBUSY);

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

	sc->sc_adpcm.sc_amp = 0;
	sc->sc_adpcm.sc_estim = 0;

	/*
	 * Here's a hack: do ulaw conversion if 6-7 bit of 
	 * minor device is set.  That way, we can have /dev/audio
	 * (minor 0x80) do ulaw conversion,  and /dev/sound or
	 * whatever, do adpcm.
	 */
	switch (minor(dev) >> 6) {
	case 0x00:
		/* /dev/adpcm */
		AUDIODEBUG("audioopen: adpcm mode\n");
		sc->sc_pencoding = sc->sc_rencoding = AUDIO_ENCODING_ADPCM;
		sc->sc_orate = 15625;
		sc->sc_irate = 15625;
		for (i = NBPG / 4; --i >= 0; ) {
			((u_long *)sc->sc_zp)[i] = 0x88008800;
			auzero[i] = 0x88008800;
		      }
		break;
	case 0x01:
		/* /dev/sound */
		AUDIODEBUG("audioopen: linear mode\n");
		sc->sc_pencoding = sc->sc_rencoding = AUDIO_ENCODING_LINEAR;
		sc->sc_orate = 15625;
		sc->sc_irate = 15625;
		for (i = NBPG / 4; --i >= 0; ) {
			((u_long *)sc->sc_zp)[i] = 0x88008800;
			auzero[i] = 0x88008800;
		}
		break;
	case 0x02:
		/* /dev/audio */
		AUDIODEBUG("audioopen: ulaw mode\n");
		sc->sc_pencoding = sc->sc_rencoding = AUDIO_ENCODING_ULAW;
		sc->sc_orate = 7813;
		sc->sc_irate = 7813;
		for (i = NBPG / 4; --i >= 0; ) {
			((u_long *)sc->sc_zp)[i] = 0x7f7f7f7f;
			auzero[i] = 0x80808080;
		      }
		break;
	case 0x03:
		/* /dev/audio */
		AUDIODEBUG("audioopen: ulaw mode\n");
		sc->sc_pencoding = sc->sc_rencoding = AUDIO_ENCODING_ULAW;
		sc->sc_orate = 7813;
		sc->sc_irate = 7813;
		for (i = NBPG / 4; --i >= 0; ) {
			((u_long *)sc->sc_zp)[i] = 0x7f7f7f7f;
			auzero[i] = 0x80808080;
		      }
		break;
	}
#if 0
	ausetrgain(sc, audio_default_level);
	ausetpgain(sc, audio_default_level);
#endif
	/* XXX */
	s = splaudio();

	if ((sc->sc_pbus == 1) || (sc->sc_rbus == 1))
		error = audio_sleep(&sc->sc_ochan);
	sc->sc_open = 1;

	if ((flags & FREAD) != 0) {
		audio_init_record(sc);
	} else {
		audio_init_play(sc);
	}
	splx(s);

	return (0);
nobufs:
	sc->sc_open = 0;
	return (ENOBUFS);
}

/*
 * Must be called from task context.
 */
void
audio_init_record(sc)
	struct audio_softc *sc;
{
	register int s;

	s = splaudio();
	dma3.csr = 0xff;
	dma3.ocr = 0xb2;
	sc->sc_mode = AUMODE_RECORD;
	sc->sc_rblks = 0;
	adpcm.stat = ADPCM_CMD_STOP;
	adpcm_set_sr(sc->sc_irate);
	splx(s);
}

/*
 * Must be called from task context.
 */
void
audio_init_play(sc)
	struct audio_softc *sc;
{
	register int s;

	s = splaudio();
	dma3.csr = 0xff;
	dma3.ocr = 0x32;
	sc->sc_mode = AUMODE_PLAY;
	sc->sc_rblks = 0;
	adpcm.stat = ADPCM_CMD_STOP;
	adpcm_set_sr(sc->sc_orate);
	splx(s);
}

static int
audio_drain(sc)
	register struct audio_softc *sc;
{
	register int error;

	while (sc->sc_nblk > 0) {
		error = audio_sleep(&sc->sc_wchan);
		if (error != 0) {
			AUDIODEBUG("audio: Interrupted?\n");
			return (error);
		}
	}
	return (0);
}

/*
 * Close an audio chip.
 */
/* ARGSUSED */
int
audioclose(dev, flags, ifmt, p)
	dev_t dev;
	int flags;
	int ifmt;
	struct proc *p;
{
	register struct audio_softc *sc =&audio_softc[0];
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
	splx(s);
	free(sc->sc_bp, M_DEVBUF);
	free(sc->sc_zp, M_DEVBUF);
	sc->sc_open = 0;
	return (0);
}

int
audioread(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{
	register struct audio_softc *sc = &audio_softc[0];
	register u_char *hp;
	register int blocksize = sc->sc_blksize;
	register int error, error2, s;

	AUDIODEBUG("audio: read\n");
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
	error = error2 = 0;
	do {
		while ((sc->sc_nblk <= 0) && (error2 == 0)){
			if (ioflag & IO_NDELAY) {
				error = EWOULDBLOCK;
				return (error);
			}
			s = splaudio();
			if (!sc->sc_rbus)
				audiostartr(sc);
			error2 = error = audio_sleep(&sc->sc_rchan);
			splx(s);
			if (error != 0) {
				AUDIODEBUG("audio: read Interrupted?\n");
				if (sc->sc_rbus == 1){
					s = splaudio();
					dma3.csr = 0xff;
					dma3.ccr = 0x08;
					sc->sc_finish = 1;
					splx(s);
					audio_sleep(&sc->sc_rchan);
				} else {
					return (error);
				}
			}
		}
		hp = sc->sc_hp;
		switch (sc->sc_rencoding) {
		case AUDIO_ENCODING_ULAW:
			audio_tolinear(sc, hp, blocksize);
			audio_tomulaw(transbuf, blocksize*2);
			error = uiomove((u_char *)transbuf2, blocksize*2, uio);
			break;
		case AUDIO_ENCODING_LINEAR:
			audio_tolinear(sc, hp, blocksize);
			error = uiomove((u_char *)transbuf, blocksize*4, uio);
			break;
		case AUDIO_ENCODING_ADPCM:
			error = uiomove(hp, blocksize, uio);
			break;
		}
		if (error) {
			printf("audio: uiomove failed\n");
			break;
		}
		hp += blocksize;
		if (hp >= sc->sc_ep)
			hp = sc->sc_bp;
		sc->sc_hp = hp;
		--sc->sc_nblk;
	} while (uio->uio_resid >= blocksize);

	return (error ? error : error2);
}

void
audio_clear(sc)
	struct audio_softc *sc;
{
	register int s = splaudio();

	if (sc->sc_rbus || sc->sc_pbus) {
		dma3.ccr = 0x10;
		sc->sc_rbus = 0;
		sc->sc_pbus = 0;
	}
	sc->sc_nblk = 0;
	sc->sc_hp = sc->sc_tp = sc->sc_bp;
	splx(s);
}

int
audiowrite(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{
	register struct audio_softc *sc;
	register u_char *tp;
	register int error, s, cc;
	register int blocksize;

	AUDIODEBUG("audio: write\n");
	sc = &audio_softc[0];
	blocksize = sc->sc_blksize;
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
				printf("audiowrite: ioflag=%x\n", ioflag);
				return (error);
			}
			error = audio_sleep(&sc->sc_wchan);
			if (error != 0) {
				splx(s);
				AUDIODEBUG("audiowrite: Interrupted?\n");
				return (error);
			}
			watermark = sc->sc_lowat;
		}
		splx(s);
		tp = sc->sc_tp;
		cc = uio->uio_resid;
		switch (sc->sc_pencoding) {
		case AUDIO_ENCODING_ULAW:
			if (cc < blocksize*2) {
				error = uiomove((u_char *)transbuf2, cc, uio);
				if (error){
					printf("audio: uiomove failed\n");
					break;
				      }
				AUDIODEBUG("audiowrite: zero suppress(%x,%x,%d)\n",transbuf, cc, blocksize*4 - cc);
				bcopy((char *)auzero, (u_char *)transbuf2 + cc, blocksize*2 - cc);
			} else {
				error = uiomove((u_char *)transbuf2, blocksize*2, uio);
				if (error) {
					printf("audio: uiomove failed\n");
					break;
				      }
			}
			break;
		case AUDIO_ENCODING_ADPCM:
			if (cc < blocksize) {
				error = uiomove(tp, cc, uio);
				if (error){
					printf("audio: uiomove failed\n");
					break;
				      }
				bcopy((char *)auzero, tp + cc, blocksize - cc);
			} else {
				error = uiomove(tp, blocksize, uio);
				if (error) {
					printf("audio: uiomove failed\n");
					break;
				      }
			}
			break;
		case AUDIO_ENCODING_LINEAR:
			if (cc < blocksize*4) {
				error = uiomove((u_char *)transbuf, cc, uio);
				if (error){
					printf("audio: uiomove failed\n");
					break;
				      }
				AUDIODEBUG("audiowrite: zero suppress(%x,%x,%d)\n",transbuf, cc, blocksize*4 - cc);
				bzero((char *)transbuf + cc, blocksize*4 - cc);
			} else {
				error = uiomove((u_char *)transbuf, blocksize*4, uio);
				if (error) {
					printf("audio: uiomove failed\n");
					break;
				      }
			}
			break;
		}
		switch (sc->sc_pencoding) {
		case AUDIO_ENCODING_ULAW:
			audio_frommulaw(transbuf2, blocksize*2);
			audio_fromlinear(sc, tp, blocksize*2);
			break;
		case AUDIO_ENCODING_LINEAR:
			audio_fromlinear(sc, tp, blocksize*2);
			break;
		case AUDIO_ENCODING_ADPCM:
			break;
		}
		tp += blocksize;
		if (tp >= sc->sc_ep)
			tp = sc->sc_bp;
		sc->sc_tp = tp;
		++sc->sc_nblk;
		sc->sc_finish = 0;
		/*
		 * If output isn't active, start it up.
		 */
		s = splaudio();
		if (sc->sc_pbus == 0)
			audiostartp(sc);
		splx(s);
	}
	AUDIODEBUG("audiowrite: exit\n");
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
	u_char	balance;
	u_short	minordev;
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
audioioctl(dev, cmd, addr, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t addr;
	int flag;
	struct proc *p;
{
	register struct audio_softc *sc = &audio_softc[0];
	int error = 0, s;

	AUDIODEBUG("audio: ioctl(0x%x)\n", cmd);
	switch (cmd) {

#if 0
	case AUDIO_GETMAP:
		bcopy((caddr_t)&sc->sc_map, addr, sizeof(sc->sc_map));
		break;

	case AUDIO_SETMAP:
		bcopy(addr, (caddr_t)&sc->sc_map, sizeof(sc->sc_map));
		sc->sc_map.mr_mmr2 &= 0x7f;
		audio_setmap(sc->sc_au.au_msm, &sc->sc_map);
		break;

	case AUDIO_FLUSH:
		sc->sc_wseek = 0;
		sc->sc_rseek = 0;
		break;

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
		error = audiosetinfo(sc, (struct audio_info *)addr);
		break;

	case AUDIO_GETINFO:
		error = audiogetinfo(sc, (struct audio_info *)addr);
		break;

	case AUDIO_DRAIN:
		error = audio_drain(sc);
		break;

	default:
		error = EINVAL;
		break;
	}
	return (error);
}

/* ARGSUSED */
int
audioselect(dev, rw, p)
	dev_t dev;
	int rw;
	struct proc *p;
{
	register struct audio_softc *sc = &audio_softc[0];
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

static inline void
audio_dmastart(read, addr, count)
	int read;
	u_char *addr;
	int count;
{
	dma3.csr = 0xff;
	dma3.mtc = (u_short)count;
	asm("nop");
	asm("nop");
	dma3.mar = (u_long)kvtop(addr);
#if defined(M68040)
		/*
		 * Push back dirty cache lines
		 */
		if (mmutype == MMU_68040)
			DCFP(kvtop(addr));
#endif
	adpcm.stat = read ? ADPCM_CMD_PLAY : ADPCM_CMD_REC;
	dma3.ccr = 0x88;
}

void
audiostartr(sc)
	struct audio_softc *sc;
{
	register u_char *tp = sc->sc_tp;
	register int cc = sc->sc_blksize;

	AUDIODEBUG("audio: startr\n");
	audio_dmastart(0, tp, cc);
	sc->sc_rbus = 1;
}

void
audiostartp(sc)
	struct audio_softc *sc;
{
	audio_dmastart(1, sc->sc_hp, sc->sc_blksize);
	sc->sc_pbus = 1;
}

void
audiointr()
{
	register struct audio_softc *sc = &audio_softc[0];

	dma3.csr = 0xff;
	PCIA(); /* XXX? by oki */
	if (sc->sc_pbus == 1){
		audio_pint(sc);
	} else if (sc->sc_rbus == 1){
		audio_rint(sc);
	} else {
		printf("audiointr: sc_pbus == sc_rbus == 0. Why interrupt?\n");
	}
	if (sc->sc_open == 0) {
		audio_wakeup(&sc->sc_ochan);
	}
}

void
audioerrintr()
{
	register struct audio_softc *sc = &audio_softc[0];

	printf("audioerrintr: software abort?\ncsr=%x, cer=%x\n          pbus = %d, rbus = %d\n", dma3.csr, dma3.cer, sc->sc_pbus, sc->sc_rbus);

	dma3.csr = 0xff;
	if (sc->sc_pbus == 1){
		audio_pint(sc);
	} else if (sc->sc_rbus == 1){
		audio_rint(sc);
	} else {
		printf("audioerrintr: sc_pbus == sc_rbus == 0. Why interrupt?\n");
	}
	if (sc->sc_open == 0) {
		audio_wakeup(&sc->sc_ochan);
	}
}

inline void
audio_pint(sc)
	struct audio_softc *sc;
{
	register u_char *hp = sc->sc_hp;
	register int cc = sc->sc_blksize;
	register int s;

	AUDIODEBUG("audio: pint sc_nblk %d\n", sc->sc_nblk);
	s = splaudio();
	if (sc->sc_finish == 1) {
		adpcm.stat = ADPCM_CMD_STOP;
		sc->sc_pbus = 0;
	} else {
		--sc->sc_nblk;
		hp = sc->sc_hp;
		hp += cc;
		if (hp >= sc->sc_ep)
			hp = sc->sc_bp;
		sc->sc_hp = hp;
		if (sc->sc_nblk > 0) {
			audio_dmastart(1, hp, cc);
	  		sc->sc_finish = 0;
		} else {
			audio_dmastart(1, (char *)&auzero, cc);
			sc->sc_finish = 1;
		}
	}
	splx(s);
	++sc->sc_rblks;

	if (sc->sc_mode == AUMODE_PLAY) {
		if (sc->sc_nblk <= sc->sc_lowat) {
			audio_wakeup(&sc->sc_wchan);
			selwakeup(&sc->sc_wsel);
		}
	}
}

/*
 * Called from sb module on completion of dma input.
 * Copy the input frame into the ring buffer at the
 * current position.  Do a wakeup if necessary.
 */
void
audio_rint(sc)
	struct audio_softc *sc;
{
	register u_char *tp = sc->sc_tp;
	register int cc = sc->sc_blksize;

	tp = sc->sc_tp;
	tp += cc;
	if (tp >= sc->sc_ep)
		tp = sc->sc_bp;
	if (++sc->sc_nblk < sc->sc_maxblk)
		audio_dmastart(0, tp, cc);
	else {
		adpcm.stat = ADPCM_CMD_STOP;
		sc->sc_rbus = 0;
	}
	sc->sc_tp = tp;

	audio_wakeup(&sc->sc_rchan);
	selwakeup(&sc->sc_rsel);
}

static void
ausetrgain(sc, level)
	register struct audio_softc *sc;
	register int level;
{
#ifdef x68k
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
#ifdef x68k
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
		p->sample_rate = adpcm_round_sr(p->sample_rate);
	printf("audiosetinfo: rate=%d\n", p->sample_rate);
		sc->sc_orate = p->sample_rate;
		if (sc->sc_mode == AUMODE_PLAY)
			(void)adpcm_set_sr(sc->sc_orate);
	}
	if (r->sample_rate != ~0) {
		if (!cleared)
			audio_clear(sc);
		cleared = 1;
		r->sample_rate = adpcm_round_sr(r->sample_rate);
		sc->sc_irate = r->sample_rate;
		if (sc->sc_mode != AUMODE_PLAY)
			(void)adpcm_set_sr(sc->sc_irate);
	}
	if (p->encoding != ~0) {
		if (!cleared)
			audio_clear(sc);
		cleared = 1;
		switch (p->encoding) {
		case AUDIO_ENCODING_ULAW:
			sc->sc_pencoding = AUDIO_ENCODING_ULAW;
			break;
		case AUDIO_ENCODING_LINEAR:
			sc->sc_pencoding = AUDIO_ENCODING_LINEAR;
			break;
		default:
			sc->sc_pencoding = AUDIO_ENCODING_ADPCM;
			p->encoding = AUDIO_ENCODING_ADPCM;
		}
	}
	if (r->encoding != ~0) {
		switch (r->encoding) {
		case AUDIO_ENCODING_ULAW:
			sc->sc_rencoding = AUDIO_ENCODING_ULAW;
			break;
		case AUDIO_ENCODING_LINEAR:
			sc->sc_rencoding = AUDIO_ENCODING_LINEAR;
			break;
		default:
			sc->sc_rencoding = AUDIO_ENCODING_ADPCM;
			r->encoding = AUDIO_ENCODING_ADPCM;
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
#if 0
	if (cleared) {
		if (sc->sc_mode != AUMODE_PLAY)
			audiostartr(sc);
		else
			audiostartp(sc);
	}
#endif
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
	p->encoding = sc->sc_pencoding;
	r->encoding = sc->sc_rencoding;

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

#if 0
void
audio_tomulaw(p, cc)
	register u_char *p;
	register int cc;
{
	register u_char *utab = lintomulaw;
	while (--cc >= 0) {
		*p = utab[*p];
		++p;
	}
}
#endif

static inline char
short2char(x)
	short x;
{
	if (x < 0){
		x /= -256;
		return (-1 * (char)x);
	} else {
		return ((char)x/256);
	}
}

void
audio_tomulaw(p, cc)
	short *p;
	register int cc;
{
	register u_char *utab = lintomulaw;
	register int i;
	for (i = 0; i < cc; i++) {
		transbuf2[i] = utab[(u_char)(short2char(p[i]))];
	}
}

void
audio_frommulaw(p, cc)
	u_char *p;
	register int cc;
{
	register u_char *utab = mulawtolin;
	register int i;
	for (i = 0; i < cc; i++) {
		transbuf[i] = (short)utab[p[i]];
	}
}

double adpcm_estimindex[16] = {
    1.0/8,  3.0/8,  5.0/8,  7.0/8,  9.0/8,  11.0/8,  13.0/8,  15.0/8,
   -1.0/8, -3.0/8, -5.0/8, -7.0/8, -9.0/8, -11.0/8, -13.0/8, -15.0/8
};

double adpcm_estim[49] = {
   16,  17,  19,  21,  23,  25,  28,  31,  34,  37,
   41,  45,  50,  55,  60,  66,  73,  80,  88,  97,
  107, 118, 130, 143, 157, 173, 190, 209, 230, 253,
  279, 307, 337, 371, 408, 449, 494, 544, 598, 658,
  724, 796, 875, 963, 1060, 1166, 1282, 1411, 1552 };

u_char adpcm_estimindex_correct[16] = {
  -1, -1, -1, -1, 2, 4, 6, 8,
  -1, -1, -1, -1, 2, 4, 6, 8
};

static inline void
adpcm2pcm_step(b, y, x)
	u_char b;
	short *y;
	signed char *x;
{
	*y += (short)(adpcm_estimindex[b] * adpcm_estim[*x]);
	*x += adpcm_estimindex_correct[b];
	if (*x < 0)
		*x = 0;
	else if (*x > 48)
		*x = 48;
}

void
audio_tolinear(sc, p, cc)
	struct audio_softc *sc;
	register u_char *p;
	register int cc;
{
	signed char *x = &(sc->sc_adpcm.sc_estim);
	short *y = &(sc->sc_adpcm.sc_amp);
	u_char a, b;
	int i;

	AUDIODEBUG("audio_tolinear:\n");
	for (i = 0; i < cc*2;) {
		a = *p;
		p++;
		b = a & 0x0f;
		adpcm2pcm_step(b, y, x);
		transbuf[i++] = *y;
		b = a >> 4;
		adpcm2pcm_step(b, y, x);
		transbuf[i++] = *y;
	}
}
#if 0
void
audio_tolinear(sc, p, cc)
	struct audio_softc *sc;
	register u_char *p;
	register int cc;
{
	signed char x = 0;
	short y = audio_softc[0].sc_amp;
	u_char a, b;
	int i;

	for (i = 0; i < cc; i++) {
		a = *p;
		p++;
		b = a & 0x0f;
		y += (short)floor(adpcm_estimindex[b] * adpcm_estim[x]);
		transbuf[i] = y;
		x += adpcm_estimindex_correct[b];
		if (x < 0)
			x = 0;
		else if (x > 48)
			x = 48;
	}
}
#endif

inline u_char
pcm2adpcm_step(a, y, x)
	short a;
	short *y;
	signed char *x;
{
	register u_char	b;
	double	c, d;

	a -= *y;
	c = (double)a*4.0 / (d = adpcm_estim[*x]);
	if (c < 0.0) {
		b = (u_char)-c;
		if (b >= 8)
			b = 7;
		b |= 0x08;
	} else {
		b = (u_char)c;
		if (b >= 8)
			b = 7;
	}	  

	*y += (short)(adpcm_estimindex[b] * d);
	*x += adpcm_estimindex_correct[b];
	if (*x < 0)
		*x = 0;
	else if (*x > 48)
		*x = 48;
	return b;
}

void
audio_fromlinear(sc, p, cc)
	struct audio_softc *sc;
	register u_char *p;
	register int cc;
{
	signed char *x = &(sc->sc_adpcm.sc_estim);
	short *y = &(sc->sc_adpcm.sc_amp);
	u_char f;
	register int i;

	for (i = 0; i < cc;) {
		f = pcm2adpcm_step(transbuf[i++], y, x);
		*p++ = f + (pcm2adpcm_step(transbuf[i++], y, x) << 4);
	}
}

#endif
