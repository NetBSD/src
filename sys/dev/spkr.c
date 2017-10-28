/*	$NetBSD: spkr.c,v 1.15 2017/10/28 03:47:24 riastradh Exp $	*/

/*
 * Copyright (c) 1990 Eric S. Raymond (esr@snark.thyrsus.com)
 * Copyright (c) 1990 Andrew A. Chernov (ache@astral.msk.su)
 * Copyright (c) 1990 Lennart Augustsson (lennart@augustsson.net)
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
 *	This product includes software developed by Eric S. Raymond
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * spkr.c -- device driver for console speaker on 80386
 *
 * v1.1 by Eric S. Raymond (esr@snark.thyrsus.com) Feb 1990
 *      modified for 386bsd by Andrew A. Chernov <ache@astral.msk.su>
 *      386bsd only clean version, all SYSV stuff removed
 *      use hz value from param.c
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: spkr.c,v 1.15 2017/10/28 03:47:24 riastradh Exp $");

#if defined(_KERNEL_OPT)
#include "wsmux.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <sys/uio.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/conf.h>

#include <sys/bus.h>

#include <dev/spkrio.h>
#include <dev/spkrvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsbellvar.h>
#include <dev/wscons/wsbellmuxvar.h>

#include "ioconf.h"

dev_type_open(spkropen);
dev_type_close(spkrclose);
dev_type_write(spkrwrite);
dev_type_ioctl(spkrioctl);

const struct cdevsw spkr_cdevsw = {
	.d_open = spkropen,
	.d_close = spkrclose,
	.d_read = noread,
	.d_write = spkrwrite,
	.d_ioctl = spkrioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER
};

static void playinit(struct spkr_softc *);
static void playtone(struct spkr_softc *, int, int, int);
static void playstring(struct spkr_softc *, const char *, size_t);

/**************** PLAY STRING INTERPRETER BEGINS HERE **********************
 *
 * Play string interpretation is modelled on IBM BASIC 2.0's PLAY statement;
 * M[LNS] are missing and the ~ synonym and octave-tracking facility is added.
 * Requires spkr_tone(), spkr_rest(). String play is not interruptible
 * except possibly at physical block boundaries.
 */

#define dtoi(c)		((c) - '0')

/*
 * Magic number avoidance...
 */
#define SECS_PER_MIN	60	/* seconds per minute */
#define WHOLE_NOTE	4	/* quarter notes per whole note */
#define MIN_VALUE	64	/* the most we can divide a note by */
#define DFLT_VALUE	4	/* default value (quarter-note) */
#define FILLTIME	8	/* for articulation, break note in parts */
#define STACCATO	6	/* 6/8 = 3/4 of note is filled */
#define NORMAL		7	/* 7/8ths of note interval is filled */
#define LEGATO		8	/* all of note interval is filled */
#define DFLT_OCTAVE	4	/* default octave */
#define MIN_TEMPO	32	/* minimum tempo */
#define DFLT_TEMPO	120	/* default tempo */
#define MAX_TEMPO	255	/* max tempo */
#define NUM_MULT	3	/* numerator of dot multiplier */
#define DENOM_MULT	2	/* denominator of dot multiplier */

/* letter to half-tone:  A   B  C  D  E  F  G */
static const int notetab[8] = {9, 11, 0, 2, 4, 5, 7};

/*
 * This is the American Standard A440 Equal-Tempered scale with frequencies
 * rounded to nearest integer. Thank Goddess for the good ol' CRC Handbook...
 * our octave 0 is standard octave 2.
 */
#define OCTAVE_NOTES	12	/* semitones per octave */
static const int pitchtab[] =
{
/*        C     C#    D     D#    E     F     F#    G     G#    A     A#    B*/
/* 0 */   65,   69,   73,   78,   82,   87,   93,   98,  103,  110,  117,  123,
/* 1 */  131,  139,  147,  156,  165,  175,  185,  196,  208,  220,  233,  247,
/* 2 */  262,  277,  294,  311,  330,  349,  370,  392,  415,  440,  466,  494,
/* 3 */  523,  554,  587,  622,  659,  698,  740,  784,  831,  880,  932,  988,
/* 4 */ 1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1975,
/* 5 */ 2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951,
/* 6 */ 4186, 4435, 4698, 4978, 5274, 5588, 5920, 6272, 6644, 7040, 7459, 7902,
};
#define NOCTAVES (int)(__arraycount(pitchtab) / OCTAVE_NOTES)

static void
playinit(struct spkr_softc *sc)
{
	sc->sc_octave = DFLT_OCTAVE;
	sc->sc_whole = (hz * SECS_PER_MIN * WHOLE_NOTE) / DFLT_TEMPO;
	sc->sc_fill = NORMAL;
	sc->sc_value = DFLT_VALUE;
	sc->sc_octtrack = false;
	sc->sc_octprefix = true;/* act as though there was an initial O(n) */
}

/* play tone of proper duration for current rhythm signature */
static void
playtone(struct spkr_softc *sc, int pitch, int val, int sustain)
{
	int sound, silence, snum = 1, sdenom = 1;

	/* this weirdness avoids floating-point arithmetic */
	for (; sustain; sustain--) {
		snum *= NUM_MULT;
		sdenom *= DENOM_MULT;
	}

	if (pitch == -1) {
		(*sc->sc_rest)(sc->sc_dev, sc->sc_whole
		    * snum / (val * sdenom));
		return;
	}

	int fac = sc->sc_whole * (FILLTIME - sc->sc_fill);
	int fval = FILLTIME * val;
	sound = (sc->sc_whole * snum) / (val * sdenom) -  fac / fval;
	silence = fac * snum / (fval * sdenom);

#ifdef SPKRDEBUG
	aprint_debug_dev(sc->sc_dev,
	    "%s: pitch %d for %d ticks, rest for %d ticks\n", __func__,
	    pitch, sound, silence);
#endif /* SPKRDEBUG */

	(*sc->sc_tone)(sc->sc_dev, pitchtab[pitch], sound);
	if (sc->sc_fill != LEGATO)
		(*sc->sc_rest)(sc->sc_dev, silence);
}

/* interpret and play an item from a notation string */
static void
playstring(struct spkr_softc *sc, const char *cp, size_t slen)
{
	int		pitch, lastpitch = OCTAVE_NOTES * DFLT_OCTAVE;

#define GETNUM(cp, v)	\
	for (v = 0; slen > 0 && isdigit((unsigned char)cp[1]); ) { \
		v = v * 10 + (*++cp - '0'); \
		slen--; \
	}

	for (; slen--; cp++) {
		int sustain, timeval, tempo;
		char c = toupper((unsigned char)*cp);

#ifdef SPKRDEBUG
		aprint_debug_dev(sc->sc_dev, "%s: %c (%x)\n", __func__, c, c);
#endif /* SPKRDEBUG */

		switch (c) {
		case 'A':  case 'B': case 'C': case 'D':
		case 'E': case 'F': case 'G':
			/* compute pitch */
			pitch = notetab[c - 'A'] + sc->sc_octave * OCTAVE_NOTES;

			/* this may be followed by an accidental sign */
			if (slen > 0 && (cp[1] == '#' || cp[1] == '+')) {
				++pitch;
				++cp;
				slen--;
			} else if (slen > 0 && cp[1] == '-') {
				--pitch;
				++cp;
				slen--;
			}

			/*
			 * If octave-tracking mode is on, and there has been no
			 * octave- setting prefix, find the version of the
			 * current letter note * closest to the last
			 * regardless of octave.
			 */
			if (sc->sc_octtrack && !sc->sc_octprefix) {
				int d = abs(pitch - lastpitch);
				if (d > abs(pitch + OCTAVE_NOTES - lastpitch)) {
					if (sc->sc_octave < NOCTAVES - 1) {
						++sc->sc_octave;
						pitch += OCTAVE_NOTES;
					}
				}

				if (d > abs(pitch - OCTAVE_NOTES - lastpitch)) {
					if (sc->sc_octave > 0) {
						--sc->sc_octave;
						pitch -= OCTAVE_NOTES;
					}
				}
			}
			sc->sc_octprefix = false;
			lastpitch = pitch;

			/*
			 * ...which may in turn be followed by an override
			 * time value
			 */
			GETNUM(cp, timeval);
			if (timeval <= 0 || timeval > MIN_VALUE)
				timeval = sc->sc_value;

			/* ...and/or sustain dots */
			for (sustain = 0; slen > 0 && cp[1] == '.'; cp++) {
				slen--;
				sustain++;
			}

			/* time to emit the actual tone */
			playtone(sc, pitch, timeval, sustain);
			break;

		case 'O':
			if (slen > 0 && (cp[1] == 'N' || cp[1] == 'n')) {
				sc->sc_octprefix = sc->sc_octtrack = false;
				++cp;
				slen--;
			} else if (slen > 0 && (cp[1] == 'L' || cp[1] == 'l')) {
				sc->sc_octtrack = true;
				++cp;
				slen--;
			} else {
				GETNUM(cp, sc->sc_octave);
				if (sc->sc_octave >= NOCTAVES)
					sc->sc_octave = DFLT_OCTAVE;
				sc->sc_octprefix = true;
			}
			break;

		case '>':
			if (sc->sc_octave < NOCTAVES - 1)
				sc->sc_octave++;
			sc->sc_octprefix = true;
			break;

		case '<':
			if (sc->sc_octave > 0)
				sc->sc_octave--;
			sc->sc_octprefix = true;
			break;

		case 'N':
			GETNUM(cp, pitch);
			for (sustain = 0; slen > 0 && cp[1] == '.'; cp++) {
				slen--;
				sustain++;
			}
			playtone(sc, pitch - 1, sc->sc_value, sustain);
			break;

		case 'L':
			GETNUM(cp, sc->sc_value);
			if (sc->sc_value <= 0 || sc->sc_value > MIN_VALUE)
				sc->sc_value = DFLT_VALUE;
			break;

		case 'P':
		case '~':
			/* this may be followed by an override time value */
			GETNUM(cp, timeval);
			if (timeval <= 0 || timeval > MIN_VALUE)
				timeval = sc->sc_value;
			for (sustain = 0; slen > 0 && cp[1] == '.'; cp++) {
				slen--;
				sustain++;
			}
			playtone(sc, -1, timeval, sustain);
			break;

		case 'T':
			GETNUM(cp, tempo);
			if (tempo < MIN_TEMPO || tempo > MAX_TEMPO)
				tempo = DFLT_TEMPO;
			sc->sc_whole = (hz * SECS_PER_MIN * WHOLE_NOTE) / tempo;
			break;

		case 'M':
			if (slen > 0 && (cp[1] == 'N' || cp[1] == 'n')) {
				sc->sc_fill = NORMAL;
				++cp;
				slen--;
			} else if (slen > 0 && (cp[1] == 'L' || cp[1] == 'l')) {
				sc->sc_fill = LEGATO;
				++cp;
				slen--;
			} else if (slen > 0 && (cp[1] == 'S' || cp[1] == 's')) {
				sc->sc_fill = STACCATO;
				++cp;
				slen--;
			}
			break;
		}
	}
}

/******************* UNIX DRIVER HOOKS BEGIN HERE **************************
 *
 * This section implements driver hooks to run playstring() and the spkr_tone()
 * and spkr_rest() functions defined above.
 */
#define spkrenter(d)	device_lookup_private(&spkr_cd, d)

void
spkr_attach(device_t self, void (*tone)(device_t, u_int, u_int),
    void (*rest)(device_t, int))
{
	struct spkr_softc *sc = device_private(self);

#ifdef SPKRDEBUG
	aprint_debug("%s: entering for unit %d\n", __func__, self->dv_unit);
#endif /* SPKRDEBUG */
	sc->sc_dev = self;
	sc->sc_tone = tone;
	sc->sc_rest = rest;
	sc->sc_inbuf = NULL;
	sc->sc_wsbelldev = NULL;

	spkr_rescan(self, "", NULL);
}

int
spkr_detach(device_t self, int flags)
{
	struct spkr_softc *sc = device_private(self);
	int rc;

#ifdef SPKRDEBUG
	aprint_debug("%s: entering for unit %d\n", __func__, self->dv_unit);
#endif /* SPKRDEBUG */
	if (sc == NULL)
		return ENXIO;

	/* XXXNS If speaker never closes, we cannot complete the detach. */
	while ((flags & DETACH_FORCE) != 0 && sc->sc_inbuf != NULL)
		kpause("spkrwait", TRUE, 1, NULL);
	if (sc->sc_inbuf != NULL)
		return EBUSY;

	rc = config_detach_children(self, flags);

	return rc;
}

/* ARGSUSED */
int
spkr_rescan(device_t self, const char *iattr, const int *locators)
{
#if NWSMUX > 0
	struct spkr_softc *sc = device_private(self);
	struct wsbelldev_attach_args a;

	if (sc->sc_wsbelldev == NULL) {
		a.accesscookie = sc;
		sc->sc_wsbelldev = config_found(self, &a, wsbelldevprint);
	}
#endif
	return 0;
}

int
spkr_childdet(device_t self, device_t child)
{
	struct spkr_softc *sc = device_private(self);

	if (sc->sc_wsbelldev == child)
		sc->sc_wsbelldev = NULL;

	return 0;
}

int
spkropen(dev_t dev, int	flags, int mode, struct lwp *l)
{
#ifdef SPKRDEBUG
	aprint_debug("%s: entering with dev = %"PRIx64"\n", __func__, dev);
#endif /* SPKRDEBUG */
	struct spkr_softc *sc = spkrenter(minor(dev));

	if (sc == NULL)
		return ENXIO;
	if (sc->sc_inbuf != NULL)
		return EBUSY;

	sc->sc_inbuf = malloc(DEV_BSIZE, M_DEVBUF, M_WAITOK);
	playinit(sc);
	return 0;
}

int
spkrwrite(dev_t dev, struct uio *uio, int flags)
{
#ifdef SPKRDEBUG
	aprint_debug("%s: entering with dev = %"PRIx64", count = %zu\n",
	    __func__, dev, uio->uio_resid);
#endif /* SPKRDEBUG */
	struct spkr_softc *sc = spkrenter(minor(dev));

	if (sc == NULL)
		return ENXIO;
	if (sc->sc_inbuf == NULL)
		return EINVAL;

	size_t n = min(DEV_BSIZE, uio->uio_resid);
	int error = uiomove(sc->sc_inbuf, n, uio);
	if (error)
		return error;
	playstring(sc, sc->sc_inbuf, n);
	return 0;
}

int
spkrclose(dev_t dev, int flags, int mode, struct lwp *l)
{
#ifdef SPKRDEBUG
	aprint_debug("%s: entering with dev = %"PRIx64"\n", __func__, dev);
#endif /* SPKRDEBUG */
	struct spkr_softc *sc = spkrenter(minor(dev));

	if (sc == NULL)
		return ENXIO;
	if (sc->sc_inbuf == NULL)
		return EINVAL;

	sc->sc_tone(sc->sc_dev, 0, 0);
	free(sc->sc_inbuf, M_DEVBUF);
	sc->sc_inbuf = NULL;

	return 0;
}

static void
playonetone(struct spkr_softc *sc, tone_t *tp)
{
    if (tp->frequency == 0)
	    (*sc->sc_rest)(sc->sc_dev, tp->duration);
    else
	    (*sc->sc_tone)(sc->sc_dev, tp->frequency, tp->duration);
}

int
spkrioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	tone_t *tp;
	tone_t ttp;
	int error;
#ifdef SPKRDEBUG
	aprint_debug("%s: entering with dev = %"PRIx64", cmd = %lx\n",
	    __func__, dev, cmd);
#endif /* SPKRDEBUG */

	struct spkr_softc *sc = spkrenter(minor(dev));

	if (sc == NULL)
		return ENXIO;
	if (sc->sc_inbuf == NULL)
		return EINVAL;

	switch (cmd) {
    	case SPKRTONE:
		playonetone(sc, data);
		return 0;
	case SPKRTUNE:
		for (tp = *(void **)data;; tp++) {
			error = copyin(tp, &ttp, sizeof(tone_t));
			if (error)
				return(error);
			if (ttp.duration == 0)
				break;
			playonetone(sc, &ttp);
		}
		return 0;
	case SPKRGETVOL:
		if (data != NULL)
			*(u_int *)data = sc->sc_vol;
		return 0;
	case SPKRSETVOL:
		if (data != NULL && *(u_int *)data <= 100)
			sc->sc_vol = *(u_int *)data;
		return 0;
	default:
		return ENOTTY;
	}
}

#ifdef _MODULE
#include "ioconf.c"
#endif

MODULE(MODULE_CLASS_DRIVER, spkr, "audio" /* and/or pcppi */ );

int
spkr_modcmd(modcmd_t cmd, void *arg)
{
	int error = 0;
#ifdef _MODULE
	devmajor_t bmajor, cmajor;
#endif

	switch(cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		bmajor = cmajor = -1;
		error = devsw_attach(spkr_cd.cd_name, NULL, &bmajor,
		    &spkr_cdevsw, &cmajor);
		if (error)
			break;

		error = config_init_component(cfdriver_ioconf_spkr,
		    cfattach_ioconf_spkr, cfdata_ioconf_spkr);
		if (error) {
			devsw_detach(NULL, &spkr_cdevsw);
		}
#endif
		break;

	case MODULE_CMD_FINI:
#ifdef _MODULE
		devsw_detach(NULL, &spkr_cdevsw);
		error = config_fini_component(cfdriver_ioconf_spkr,
		    cfattach_ioconf_spkr, cfdata_ioconf_spkr);
		if (error)
			devsw_attach(spkr_cd.cd_name, NULL, &bmajor,
			    &spkr_cdevsw, &cmajor);
#endif
		break;

	default:
		error = ENOTTY;
		break;
	}

	return error;
}
