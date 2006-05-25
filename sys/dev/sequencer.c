/*	$NetBSD: sequencer.c,v 1.30.14.19 2006/05/25 21:05:29 chap Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (augustss@NetBSD.org).
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sequencer.c,v 1.30.14.19 2006/05/25 21:05:29 chap Exp $");

#include "sequencer.h"

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/kernel.h>
#include <sys/signalvar.h>
#include <sys/conf.h>
#include <sys/audioio.h>
#include <sys/midiio.h>
#include <sys/device.h>

#include <dev/midi_if.h>
#include <dev/midivar.h>
#include <dev/sequencervar.h>

#define ADDTIMEVAL(a, b) ( \
	(a)->tv_sec += (b)->tv_sec, \
	(a)->tv_usec += (b)->tv_usec, \
	(a)->tv_usec > 1000000 ? ((a)->tv_sec++, (a)->tv_usec -= 1000000) : 0\
	)

#define SUBTIMEVAL(a, b) ( \
	(a)->tv_sec -= (b)->tv_sec, \
	(a)->tv_usec -= (b)->tv_usec, \
	(a)->tv_usec < 0 ? ((a)->tv_sec--, (a)->tv_usec += 1000000) : 0\
	)

#ifdef AUDIO_DEBUG
#define DPRINTF(x)	if (sequencerdebug) printf x
#define DPRINTFN(n,x)	if (sequencerdebug >= (n)) printf x
int	sequencerdebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

#define SEQ_NOTE_MAX 128
#define SEQ_NOTE_XXX 255

#define RECALC_TICK(t) ((t)->tick = 60 * 1000000L / ((t)->tempo * (t)->timebase))

struct sequencer_softc seqdevs[NSEQUENCER];

void sequencerattach(int);
static void seq_reset(struct sequencer_softc *);
static int seq_do_command(struct sequencer_softc *, seq_event_t *);
static int seq_do_chnvoice(struct sequencer_softc *, seq_event_t *);
static int seq_do_chncommon(struct sequencer_softc *, seq_event_t *);
static void seq_timer_waitabs(struct sequencer_softc *, uint32_t);
static int seq_do_timing(struct sequencer_softc *, seq_event_t *);
static int seq_do_local(struct sequencer_softc *, seq_event_t *);
static int seq_do_sysex(struct sequencer_softc *, seq_event_t *);
static int seq_do_fullsize(struct sequencer_softc *, seq_event_t *, struct uio *);
static int seq_input_event(struct sequencer_softc *, seq_event_t *);
static int seq_drain(struct sequencer_softc *);
static void seq_startoutput(struct sequencer_softc *);
static void seq_timeout(void *);
static int seq_to_new(seq_event_t *, struct uio *);
static int seq_sleep_timo(int *, const char *, int);
static int seq_sleep(int *, const char *);
static void seq_wakeup(int *);

struct midi_softc;
static int midiseq_out(struct midi_dev *, u_char *, u_int, int);
static struct midi_dev *midiseq_open(int, int);
static void midiseq_close(struct midi_dev *);
static void midiseq_reset(struct midi_dev *);
static int midiseq_noteon(struct midi_dev *, int, int, seq_event_t *);
static int midiseq_noteoff(struct midi_dev *, int, int, seq_event_t *);
static int midiseq_keypressure(struct midi_dev *, int, int, seq_event_t *);
static int midiseq_pgmchange(struct midi_dev *, int, seq_event_t *);
static int midiseq_chnpressure(struct midi_dev *, int, seq_event_t *);
static int midiseq_ctlchange(struct midi_dev *, int, seq_event_t *);
static int midiseq_pitchbend(struct midi_dev *, int, seq_event_t *);
static int midiseq_loadpatch(struct midi_dev *, struct sysex_info *, struct uio *);
void midiseq_in(struct midi_dev *, u_char *, int);

static dev_type_open(sequenceropen);
static dev_type_close(sequencerclose);
static dev_type_read(sequencerread);
static dev_type_write(sequencerwrite);
static dev_type_ioctl(sequencerioctl);
static dev_type_poll(sequencerpoll);
static dev_type_kqfilter(sequencerkqfilter);

const struct cdevsw sequencer_cdevsw = {
	sequenceropen, sequencerclose, sequencerread, sequencerwrite,
	sequencerioctl, nostop, notty, sequencerpoll, nommap,
	sequencerkqfilter,
};

void
sequencerattach(int n)
{

	for (n = 0; n < NSEQUENCER; n++)
		callout_init(&seqdevs[n].sc_callout);
}

static int
sequenceropen(dev_t dev, int flags, int ifmt, struct lwp *l)
{
	int unit = SEQUENCERUNIT(dev);
	struct sequencer_softc *sc;
	struct midi_dev *md;
	int nmidi;

	DPRINTF(("sequenceropen\n"));

	if (unit >= NSEQUENCER)
		return (ENXIO);
	sc = &seqdevs[unit];
	if (sc->isopen)
		return EBUSY;
	if (SEQ_IS_OLD(unit))
		sc->mode = SEQ_OLD;
	else
		sc->mode = SEQ_NEW;
	sc->isopen++;
	sc->flags = flags & (FREAD|FWRITE);
	sc->rchan = 0;
	sc->wchan = 0;
	sc->pbus = 0;
	sc->async = 0;
	sc->input_stamp = ~0;

	sc->nmidi = 0;
	nmidi = midi_unit_count();

	sc->devs = malloc(nmidi * sizeof(struct midi_dev *),
			  M_DEVBUF, M_WAITOK);
	for (unit = 0; unit < nmidi; unit++) {
		md = midiseq_open(unit, flags);
		if (md) {
			sc->devs[sc->nmidi++] = md;
			md->seq = sc;
		}
	}

	sc->timer.timebase = 100;
	sc->timer.tempo = 60;
	sc->doingsysex = 0;
	RECALC_TICK(&sc->timer);
	sc->timer.last = 0;
	microtime(&sc->timer.start);

	SEQ_QINIT(&sc->inq);
	SEQ_QINIT(&sc->outq);
	sc->lowat = SEQ_MAXQ / 2;

	seq_reset(sc);

	DPRINTF(("sequenceropen: mode=%d, nmidi=%d\n", sc->mode, sc->nmidi));
	return 0;
}

static int
seq_sleep_timo(int *chan, const char *label, int timo)
{
	int st;

	if (!label)
		label = "seq";

	DPRINTFN(5, ("seq_sleep_timo: %p %s %d\n", chan, label, timo));
	*chan = 1;
	st = tsleep(chan, PWAIT | PCATCH, label, timo);
	*chan = 0;
#ifdef MIDI_DEBUG
	if (st != 0)
	    printf("seq_sleep: %d\n", st);
#endif
	return st;
}

static int
seq_sleep(int *chan, const char *label)
{
	return seq_sleep_timo(chan, label, 0);
}

static void
seq_wakeup(int *chan)
{
	if (*chan) {
		DPRINTFN(5, ("seq_wakeup: %p\n", chan));
		wakeup(chan);
		*chan = 0;
	}
}

static int
seq_drain(struct sequencer_softc *sc)
{
	int error;

	DPRINTFN(3, ("seq_drain: %p, len=%d\n", sc, SEQ_QLEN(&sc->outq)));
	seq_startoutput(sc);
	error = 0;
	while(!SEQ_QEMPTY(&sc->outq) && !error)
		error = seq_sleep_timo(&sc->wchan, "seq_dr", 60*hz);
	return (error);
}

static void
seq_timeout(void *addr)
{
	struct sequencer_softc *sc = addr;
	DPRINTFN(4, ("seq_timeout: %p\n", sc));
	sc->timeout = 0;
	seq_startoutput(sc);
	if (SEQ_QLEN(&sc->outq) < sc->lowat) {
		seq_wakeup(&sc->wchan);
		selnotify(&sc->wsel, 0);
		if (sc->async)
			psignal(sc->async, SIGIO);
	}

}

static void
seq_startoutput(struct sequencer_softc *sc)
{
	struct sequencer_queue *q = &sc->outq;
	seq_event_t cmd;

	if (sc->timeout)
		return;
	DPRINTFN(4, ("seq_startoutput: %p, len=%d\n", sc, SEQ_QLEN(q)));
	while(!SEQ_QEMPTY(q) && !sc->timeout) {
		SEQ_QGET(q, cmd);
		seq_do_command(sc, &cmd);
	}
}

static int
sequencerclose(dev_t dev, int flags, int ifmt, struct lwp *l)
{
	struct sequencer_softc *sc = &seqdevs[SEQUENCERUNIT(dev)];
	int n, s;

	DPRINTF(("sequencerclose: %p\n", sc));

	seq_drain(sc);
	s = splaudio();
	if (sc->timeout) {
		callout_stop(&sc->sc_callout);
		sc->timeout = 0;
	}
	splx(s);

	for (n = 0; n < sc->nmidi; n++)
		midiseq_close(sc->devs[n]);
	free(sc->devs, M_DEVBUF);
	sc->isopen = 0;
	return (0);
}

static int
seq_input_event(struct sequencer_softc *sc, seq_event_t *cmd)
{
	struct sequencer_queue *q = &sc->inq;

	DPRINTFN(2, ("seq_input_event: %02x %02x %02x %02x %02x %02x %02x %02x\n",
		     cmd->tag,
		     cmd->unknown.byte[0], cmd->unknown.byte[1],
		     cmd->unknown.byte[2], cmd->unknown.byte[3],
		     cmd->unknown.byte[4], cmd->unknown.byte[5],
		     cmd->unknown.byte[6]));
	if (SEQ_QFULL(q))
		return (ENOMEM);
	SEQ_QPUT(q, *cmd);
	seq_wakeup(&sc->rchan);
	selnotify(&sc->rsel, 0);
	if (sc->async)
		psignal(sc->async, SIGIO);
	return 0;
}

void
seq_event_intr(void *addr, seq_event_t *iev)
{
	struct sequencer_softc *sc = addr;
	u_long t;
	struct timeval now;

	microtime(&now);
	SUBTIMEVAL(&now, &sc->timer.start);
	t = now.tv_sec * 1000000 + now.tv_usec;
	t /= sc->timer.tick;
	if (t != sc->input_stamp) {
		seq_input_event(sc, &SEQ_MK_TIMING(WAIT_ABS, .divisions=t));
		sc->input_stamp = t;
	}
	seq_input_event(sc, iev);
}

static int
sequencerread(dev_t dev, struct uio *uio, int ioflag)
{
	struct sequencer_softc *sc = &seqdevs[SEQUENCERUNIT(dev)];
	struct sequencer_queue *q = &sc->inq;
	seq_event_t ev;
	int error, s;

	DPRINTFN(20, ("sequencerread: %p, count=%d, ioflag=%x\n",
		     sc, (int) uio->uio_resid, ioflag));

	if (sc->mode == SEQ_OLD) {
		DPRINTFN(-1,("sequencerread: old read\n"));
		return (EINVAL); /* XXX unimplemented */
	}

	error = 0;
	while (SEQ_QEMPTY(q)) {
		if (ioflag & IO_NDELAY)
			return EWOULDBLOCK;
		else {
			error = seq_sleep(&sc->rchan, "seq rd");
			if (error)
				return error;
		}
	}
	s = splaudio();
	while (uio->uio_resid >= sizeof ev && !error && !SEQ_QEMPTY(q)) {
		SEQ_QGET(q, ev);
		error = uiomove(&ev, sizeof ev, uio);
	}
	splx(s);
	return error;
}

static int
sequencerwrite(dev_t dev, struct uio *uio, int ioflag)
{
	struct sequencer_softc *sc = &seqdevs[SEQUENCERUNIT(dev)];
	struct sequencer_queue *q = &sc->outq;
	int error;
	seq_event_t cmdbuf;
	int size;
	
	DPRINTFN(2, ("sequencerwrite: %p, count=%d\n", sc, (int) uio->uio_resid));

	error = 0;
	size = sc->mode == SEQ_NEW ? sizeof cmdbuf : SEQOLD_CMDSIZE;
	while (uio->uio_resid >= size) {
		error = uiomove(&cmdbuf, size, uio);
		if (error)
			break;
		if (sc->mode == SEQ_OLD)
			if (seq_to_new(&cmdbuf, uio))
				continue;
		if (cmdbuf.tag == SEQ_FULLSIZE) {
			/* We do it like OSS does, asynchronously */
			error = seq_do_fullsize(sc, &cmdbuf, uio);
			if (error)
				break;
			continue;
		}
		while (SEQ_QFULL(q)) {
			seq_startoutput(sc);
			if (SEQ_QFULL(q)) {
				if (ioflag & IO_NDELAY)
					return EWOULDBLOCK;
				error = seq_sleep(&sc->wchan, "seq_wr");
				if (error)
					return error;
			}
		}
		SEQ_QPUT(q, cmdbuf);
	}
	seq_startoutput(sc);

#ifdef SEQUENCER_DEBUG
	if (error)
		DPRINTFN(2, ("sequencerwrite: error=%d\n", error));
#endif
	return error;
}

static int
sequencerioctl(dev_t dev, u_long cmd, caddr_t addr, int flag, struct lwp *l)
{
	struct sequencer_softc *sc = &seqdevs[SEQUENCERUNIT(dev)];
	struct synth_info *si;
	struct midi_dev *md;
	int devno;
	int error;
	int t;

	DPRINTFN(2, ("sequencerioctl: %p cmd=0x%08lx\n", sc, cmd));

	error = 0;
	switch (cmd) {
	case FIONBIO:
		/* All handled in the upper FS layer. */
		break;

	case FIOASYNC:
		if (*(int *)addr) {
			if (sc->async)
				return EBUSY;
			sc->async = l->l_proc;
			DPRINTF(("sequencer_ioctl: FIOASYNC %p\n", l));
		} else
			sc->async = 0;
		break;

	case SEQUENCER_RESET:
		seq_reset(sc);
		break;

	case SEQUENCER_PANIC:
		seq_reset(sc);
		/* Do more?  OSS doesn't */
		break;

	case SEQUENCER_SYNC:
		if (sc->flags == FREAD)
			return 0;
		seq_drain(sc);
		error = 0;
		break;

	case SEQUENCER_INFO:
		si = (struct synth_info*)addr;
		devno = si->device;
		if (devno < 0 || devno >= sc->nmidi)
			return EINVAL;
		md = sc->devs[devno];
		strncpy(si->name, md->name, sizeof si->name);
		si->synth_type = SYNTH_TYPE_MIDI;
		si->synth_subtype = md->subtype;
		si->nr_voices = md->nr_voices;
		si->instr_bank_size = md->instr_bank_size;
		si->capabilities = md->capabilities;
		break;

	case SEQUENCER_NRSYNTHS:
		*(int *)addr = sc->nmidi;
		break;

	case SEQUENCER_NRMIDIS:
		*(int *)addr = sc->nmidi;
		break;

	case SEQUENCER_OUTOFBAND:
		DPRINTFN(3, ("sequencer_ioctl: OOB=%02x %02x %02x %02x %02x %02x %02x %02x\n",
			     *(u_char *)addr, *(u_char *)(addr+1),
			     *(u_char *)(addr+2), *(u_char *)(addr+3),
			     *(u_char *)(addr+4), *(u_char *)(addr+5),
			     *(u_char *)(addr+6), *(u_char *)(addr+7)));
		if ( !(sc->flags & FWRITE ) )
		        return EBADF;
		error = seq_do_command(sc, (seq_event_t *)addr);
		break;

	case SEQUENCER_TMR_TIMEBASE:
		t = *(int *)addr;
		if (t < 1)
			t = 1;
		if (t > 10000)
			t = 10000;
		sc->timer.timebase = t;
		*(int *)addr = t;
		RECALC_TICK(&sc->timer);
		break;

	case SEQUENCER_TMR_START:
		error = seq_do_timing(sc, &SEQ_MK_TIMING(START));
		break;

	case SEQUENCER_TMR_STOP:
		error = seq_do_timing(sc, &SEQ_MK_TIMING(STOP));
		break;

	case SEQUENCER_TMR_CONTINUE:
		error = seq_do_timing(sc, &SEQ_MK_TIMING(CONTINUE));
		break;

	case SEQUENCER_TMR_TEMPO:
		error = seq_do_timing(sc,
		    &SEQ_MK_TIMING(TEMPO, .bpm=*(int *)addr));
		if (!error)
		    *(int *)addr = sc->timer.tempo;
		break;

	case SEQUENCER_TMR_SOURCE:
		*(int *)addr = SEQUENCER_TMR_INTERNAL;
		break;

	case SEQUENCER_TMR_METRONOME:
		/* noop */
		break;

	case SEQUENCER_THRESHOLD:
		t = SEQ_MAXQ - *(int *)addr / sizeof (seq_event_rec);
		if (t < 1)
			t = 1;
		if (t > SEQ_MAXQ)
			t = SEQ_MAXQ;
		sc->lowat = t;
		break;

	case SEQUENCER_CTRLRATE:
		*(int *)addr = (sc->timer.tempo*sc->timer.timebase + 30) / 60;
		break;

	case SEQUENCER_GETTIME:
	{
		struct timeval now;
		u_long tx;
		microtime(&now);
		SUBTIMEVAL(&now, &sc->timer.start);
		tx = now.tv_sec * 1000000 + now.tv_usec;
		tx /= sc->timer.tick;
		*(int *)addr = tx;
		break;
	}

	default:
		DPRINTFN(-1,("sequencer_ioctl: unimpl %08lx\n", cmd));
		error = EINVAL;
		break;
	}
	return error;
}

static int
sequencerpoll(dev_t dev, int events, struct lwp *l)
{
	struct sequencer_softc *sc = &seqdevs[SEQUENCERUNIT(dev)];
	int revents = 0;

	DPRINTF(("sequencerpoll: %p events=0x%x\n", sc, events));

	if (events & (POLLIN | POLLRDNORM))
		if ((sc->flags&FREAD) && !SEQ_QEMPTY(&sc->inq))
			revents |= events & (POLLIN | POLLRDNORM);

	if (events & (POLLOUT | POLLWRNORM))
		if ((sc->flags&FWRITE) && SEQ_QLEN(&sc->outq) < sc->lowat)
			revents |= events & (POLLOUT | POLLWRNORM);

	if (revents == 0) {
		if ((sc->flags&FREAD) && (events & (POLLIN | POLLRDNORM)))
			selrecord(l, &sc->rsel);

		if ((sc->flags&FWRITE) && (events & (POLLOUT | POLLWRNORM)))
			selrecord(l, &sc->wsel);
	}

	return revents;
}

static void
filt_sequencerrdetach(struct knote *kn)
{
	struct sequencer_softc *sc = kn->kn_hook;
	int s;

	s = splaudio();
	SLIST_REMOVE(&sc->rsel.sel_klist, kn, knote, kn_selnext);
	splx(s);
}

static int
filt_sequencerread(struct knote *kn, long hint)
{
	struct sequencer_softc *sc = kn->kn_hook;

	/* XXXLUKEM (thorpej): make sure this is correct */

	if (SEQ_QEMPTY(&sc->inq))
		return (0);
	kn->kn_data = sizeof(seq_event_rec);
	return (1);
}

static const struct filterops sequencerread_filtops =
	{ 1, NULL, filt_sequencerrdetach, filt_sequencerread };

static void
filt_sequencerwdetach(struct knote *kn)
{
	struct sequencer_softc *sc = kn->kn_hook;
	int s;

	s = splaudio();
	SLIST_REMOVE(&sc->wsel.sel_klist, kn, knote, kn_selnext);
	splx(s);
}

static int
filt_sequencerwrite(struct knote *kn, long hint)
{
	struct sequencer_softc *sc = kn->kn_hook;

	/* XXXLUKEM (thorpej): make sure this is correct */

	if (SEQ_QLEN(&sc->outq) >= sc->lowat)
		return (0);
	kn->kn_data = sizeof(seq_event_rec);
	return (1);
}

static const struct filterops sequencerwrite_filtops =
	{ 1, NULL, filt_sequencerwdetach, filt_sequencerwrite };

static int
sequencerkqfilter(dev_t dev, struct knote *kn)
{
	struct sequencer_softc *sc = &seqdevs[SEQUENCERUNIT(dev)];
	struct klist *klist;
	int s;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &sc->rsel.sel_klist;
		kn->kn_fop = &sequencerread_filtops;
		break;

	case EVFILT_WRITE:
		klist = &sc->wsel.sel_klist;
		kn->kn_fop = &sequencerwrite_filtops;
		break;

	default:
		return (1);
	}

	kn->kn_hook = sc;

	s = splaudio();
	SLIST_INSERT_HEAD(klist, kn, kn_selnext);
	splx(s);

	return (0);
}

static void
seq_reset(struct sequencer_softc *sc)
{
	int i, chn;
	struct midi_dev *md;

	if ( !(sc->flags & FWRITE) )
	        return;
	for (i = 0; i < sc->nmidi; i++) {
		md = sc->devs[i];
		midiseq_reset(md);
		for (chn = 0; chn < MAXCHAN; chn++) {
			midiseq_ctlchange(md, chn, &SEQ_MK_CHN(CTL_CHANGE,
			    .controller=MIDI_CTRL_ALLOFF));
			midiseq_ctlchange(md, chn, &SEQ_MK_CHN(CTL_CHANGE,
			    .controller=MIDI_CTRL_RESET));
			midiseq_pitchbend(md, chn, &SEQ_MK_CHN(PITCH_BEND,
			    .value=MIDI_BEND_NEUTRAL));
		}
	}
}

static int
seq_do_command(struct sequencer_softc *sc, seq_event_t *b)
{
	int dev;

	DPRINTFN(4, ("seq_do_command: %p cmd=0x%02x\n", sc, SEQ_CMD(b)));

	switch(b->tag) {
	case SEQ_LOCAL:
		return seq_do_local(sc, b);
	case SEQ_TIMING:
		return seq_do_timing(sc, b);
	case SEQ_CHN_VOICE:
		return seq_do_chnvoice(sc, b);
	case SEQ_CHN_COMMON:
		return seq_do_chncommon(sc, b);
	case SEQ_SYSEX:
		return seq_do_sysex(sc, b);
	/* COMPAT */
	case SEQOLD_MIDIPUTC:
		dev = b->unknown.byte[1];
		if (dev < 0 || dev >= sc->nmidi)
			return (ENXIO);
		return midiseq_out(sc->devs[dev], b->unknown.byte, 1, 0);
	default:
		DPRINTFN(-1,("seq_do_command: unimpl command %02x\n", b->tag));
		return (EINVAL);
	}
}

static int
seq_do_chnvoice(struct sequencer_softc *sc, seq_event_t *b)
{
	int dev;
	int error;
	struct midi_dev *md;

	dev = b->voice.device;
	if (dev < 0 || dev >= sc->nmidi ||
	    b->voice.channel > 15 ||
	    b->voice.key >= SEQ_NOTE_MAX)
		return ENXIO;
	md = sc->devs[dev];
	switch(b->voice.op) {
	case MIDI_NOTEON: /* no need to special-case hidden noteoff here */
		error = midiseq_noteon(md, b->voice.channel, b->voice.key, b);
		break;
	case MIDI_NOTEOFF:
		error = midiseq_noteoff(md, b->voice.channel, b->voice.key, b);
		break;
	case MIDI_KEY_PRESSURE:
		error = midiseq_keypressure(md,
		    b->voice.channel, b->voice.key, b);
		break;
	default:
		DPRINTFN(-1,("seq_do_chnvoice: unimpl command %02x\n",
			b->voice.op));
		error = EINVAL;
		break;
	}
	return error;
}

static int
seq_do_chncommon(struct sequencer_softc *sc, seq_event_t *b)
{
	int dev;
	int error;
	struct midi_dev *md;

	dev = b->common.device;
	if (dev < 0 || dev >= sc->nmidi ||
	    b->common.channel > 15)
		return ENXIO;
	md = sc->devs[dev];
	DPRINTFN(2,("seq_do_chncommon: %02x\n", b->common.op));

	error = 0;
	switch(b->common.op) {
	case MIDI_PGM_CHANGE:
		error = midiseq_pgmchange(md, b->common.channel, b);
		break;
	case MIDI_CTL_CHANGE:
		error = midiseq_ctlchange(md, b->common.channel, b);
		break;
	case MIDI_PITCH_BEND:
		error = midiseq_pitchbend(md, b->common.channel, b);
		break;
	case MIDI_CHN_PRESSURE:
		error = midiseq_chnpressure(md, b->common.channel, b);
		break;
	default:
		DPRINTFN(-1,("seq_do_chncommon: unimpl command %02x\n",
			b->common.op));
		error = EINVAL;
		break;
	}
	return error;
}

static int
seq_do_local(struct sequencer_softc *sc, seq_event_t *b)
{
	return (EINVAL);
}

static int
seq_do_sysex(struct sequencer_softc *sc, seq_event_t *b)
{
	int dev, i;
	struct midi_dev *md;
	uint8_t *bf = b->sysex.buffer;

	dev = b->sysex.device;
	if (dev < 0 || dev >= sc->nmidi)
		return (ENXIO);
	DPRINTF(("seq_do_sysex: dev=%d\n", dev));
	md = sc->devs[dev];

	if (!sc->doingsysex) {
		midiseq_out(md, (uint8_t[]){MIDI_SYSEX_START}, 1, 0);
		sc->doingsysex = 1;
	}

	for (i = 0; i < 6 && bf[i] != 0xff; i++)
		;
	midiseq_out(md, bf, i, 0);
	if (i < 6 || (i > 0 && bf[i-1] == MIDI_SYSEX_END))
		sc->doingsysex = 0;
	return 0;
}

static void
seq_timer_waitabs(struct sequencer_softc *sc, uint32_t divs)
{
	struct timeval when;
	long long usec;
	struct syn_timer *t;
	int ticks;

	t = &sc->timer;
	t->last = divs;
	usec = (long long)divs * (long long)t->tick; /* convert to usec */
	when.tv_sec = usec / 1000000;
	when.tv_usec = usec % 1000000;
	DPRINTFN(4, ("seq_timer_waitabs: divs=%d, sleep when=%ld.%06ld", divs,
		     when.tv_sec, when.tv_usec));
	ADDTIMEVAL(&when, &t->start); /* abstime for end */
	ticks = hzto(&when);
	DPRINTFN(4, (" when+start=%ld.%06ld, tick=%d\n",
		     when.tv_sec, when.tv_usec, ticks));
	if (ticks > 0) {
#ifdef DIAGNOSTIC
		if (ticks > 20 * hz) {
			/* Waiting more than 20s */
			printf("seq_timer_waitabs: funny ticks=%d, "
			       "usec=%lld, parm=%d, tick=%ld\n",
			       ticks, usec, parm, t->tick);
		}
#endif
		sc->timeout = 1;
		callout_reset(&sc->sc_callout, ticks,
		    seq_timeout, sc);
	}
#ifdef SEQUENCER_DEBUG
	else if (tick < 0)
		DPRINTF(("seq_timer_waitabs: ticks = %d\n", ticks));
#endif
}

static int
seq_do_timing(struct sequencer_softc *sc, seq_event_t *b)
{
	struct syn_timer *t = &sc->timer;
	struct timeval when;
	int error;

	error = 0;
	switch(b->timing.op) {
	case TMR_WAIT_REL:
		seq_timer_waitabs(sc, b->t_WAIT_REL.divisions + t->last);
		break;
	case TMR_WAIT_ABS:
		seq_timer_waitabs(sc, b->t_WAIT_ABS.divisions);
		break;
	case TMR_START:
		microtime(&t->start);
		t->running = 1;
		break;
	case TMR_STOP:
		microtime(&t->stop);
		t->running = 0;
		break;
	case TMR_CONTINUE:
		microtime(&when);
		SUBTIMEVAL(&when, &t->stop);
		ADDTIMEVAL(&t->start, &when);
		t->running = 1;
		break;
	case TMR_TEMPO:
		/* bpm is unambiguously MIDI clocks per minute / 24 */
		/* (24 MIDI clocks are usually but not always a quarter note) */
		if (b->t_TEMPO.bpm < 8) /* where are these limits specified? */
			t->tempo = 8;
		else if (b->t_TEMPO.bpm > 360) /* ? */
			t->tempo = 360;
		else
			t->tempo = b->t_TEMPO.bpm;
		RECALC_TICK(t);
		break;
	case TMR_ECHO:
		error = seq_input_event(sc, b);
		break;
	case TMR_RESET:
		t->last = 0;
		microtime(&t->start);
		break;
	case TMR_SPP:
	case TMR_TIMESIG:
		DPRINTF(("seq_do_timing: unimplemented %02x\n", b->timing.op));
		error = EINVAL; /* not quite accurate... */
		break;
	default:
		DPRINTF(("seq_timer: unknown %02x\n", cmd));
		error = EINVAL;
		break;
	}
	return (error);
}

static int
seq_do_fullsize(struct sequencer_softc *sc, seq_event_t *b, struct uio *uio)
{
	struct sysex_info sysex;
	u_int dev;

#ifdef DIAGNOSTIC
	if (sizeof(seq_event_rec) != SEQ_SYSEX_HDRSIZE) {
		printf("seq_do_fullsize: sysex size ??\n");
		return EINVAL;
	}
#endif
	memcpy(&sysex, b, sizeof sysex);
	dev = sysex.device_no;
	if (dev < 0 || dev >= sc->nmidi)
		return (ENXIO);
	DPRINTFN(2, ("seq_do_fullsize: fmt=%04x, dev=%d, len=%d\n",
		     sysex.key, dev, sysex.len));
	return (midiseq_loadpatch(sc->devs[dev], &sysex, uio));
}

/* Convert an old sequencer event to a new one. */
static int
seq_to_new(seq_event_t *ev, struct uio *uio)
{
	int cmd, chan, note, parm;
	uint32_t tmp_delay;
	int error;
	uint8_t *bfp;

	cmd = ev->tag;
	bfp = ev->unknown.byte;
	chan = *bfp++;
	note = *bfp++;
	parm = *bfp++;
	DPRINTFN(3, ("seq_to_new: 0x%02x %d %d %d\n", cmd, chan, note, parm));

	if (cmd >= 0x80) {
		/* Fill the event record */
		if (uio->uio_resid >= sizeof *ev - SEQOLD_CMDSIZE) {
			error = uiomove(bfp, sizeof *ev - SEQOLD_CMDSIZE, uio);
			if (error)
				return error;
		} else
			return EINVAL;
	}

	switch(cmd) {
	case SEQOLD_NOTEOFF:
		/*
		 * What's with the SEQ_NOTE_XXX?  In OSS this seems to have
		 * been undocumented magic for messing with the overall volume
		 * of a 'voice', equated precariously with 'channel' and
		 * pretty much unimplementable except by directly frobbing a
		 * synth chip. For us, who treat everything as interfaced over
		 * MIDI, this will just be unceremoniously discarded as
		 * invalid in midiseq_noteoff, making the whole event an
		 * elaborate no-op, and that doesn't seem to be any different
		 * from what happens on linux with a MIDI-interfaced device,
		 * by the way. The moral is ... use the new /dev/music API, ok?
		 */
		*ev = SEQ_MK_CHN(NOTEOFF, .device=0, .channel=chan,
		    .key=SEQ_NOTE_XXX, .velocity=parm);
		break;
	case SEQOLD_NOTEON:
		*ev = SEQ_MK_CHN(NOTEON,
		    .device=0, .channel=chan, .key=note, .velocity=parm);
		break;
	case SEQOLD_WAIT:
		/*
		 * This event cannot even /exist/ on non-littleendian machines,
		 * and so help me, that's exactly the way OSS defined it.
		 * Also, the OSS programmer's guide states (p. 74, v1.11)
		 * that seqold time units are system clock ticks, unlike
		 * the new 'divisions' which are determined by timebase. In
		 * that case we would need to do scaling here - but no such
		 * behavior is visible in linux either--which also treats this
		 * value, surprisingly, as an absolute, not relative, time.
		 * My guess is that this event has gone unused so long that
		 * nobody could agree we got it wrong no matter what we do.
		 */
		tmp_delay = *(uint32_t *)ev >> 8;
		*ev = SEQ_MK_TIMING(WAIT_ABS, .divisions=tmp_delay);
		break;
	case SEQOLD_SYNCTIMER:
		/*
		 * The TMR_RESET event is not defined in any OSS materials
		 * I can find; it may have been invented here just to provide
		 * an accurate _to_new translation of this event.
		 */
		*ev = SEQ_MK_TIMING(RESET);
		break;
	case SEQOLD_PGMCHANGE:
		*ev = SEQ_MK_CHN(PGM_CHANGE,
		    .device=0, .channel=chan, .program=note);
		break;
	case SEQOLD_MIDIPUTC:
		break;		/* interpret in normal mode */
	case SEQOLD_ECHO:
	case SEQOLD_PRIVATE:
	case SEQOLD_EXTENDED:
	default:
		DPRINTF(("seq_to_new: not impl 0x%02x\n", cmd));
		return EINVAL;
	/* In case new-style events show up */
	case SEQ_TIMING:
	case SEQ_CHN_VOICE:
	case SEQ_CHN_COMMON:
	case SEQ_FULLSIZE:
		break;
	}
	return 0;
}

/**********************************************/

void
midiseq_in(struct midi_dev *md, u_char *msg, int len)
{
	int unit = md->unit;
	seq_event_t ev;
	int status, chan;

	DPRINTFN(2, ("midiseq_in: %p %02x %02x %02x\n",
		     md, msg[0], msg[1], msg[2]));

	status = MIDI_GET_STATUS(msg[0]);
	chan = MIDI_GET_CHAN(msg[0]);
	switch (status) {
	case MIDI_NOTEON: /* midi(4) always canonicalizes hidden note-off */
		ev = SEQ_MK_CHN(NOTEON, .device=unit, .channel=chan,
		    .key=msg[1], .velocity=msg[2]);
		break;
	case MIDI_NOTEOFF:
		ev = SEQ_MK_CHN(NOTEOFF, .device=unit, .channel=chan,
		    .key=msg[1], .velocity=msg[2]);
		break;
	case MIDI_KEY_PRESSURE:
		ev = SEQ_MK_CHN(KEY_PRESSURE, .device=unit, .channel=chan,
		    .key=msg[1], .pressure=msg[2]);
		break;
	case MIDI_CTL_CHANGE: /* XXX not correct for MSB */
		ev = SEQ_MK_CHN(CTL_CHANGE, .device=unit, .channel=chan,
		    .controller=msg[1], .value=msg[2]);
		break;
	case MIDI_PGM_CHANGE:
		ev = SEQ_MK_CHN(PGM_CHANGE, .device=unit, .channel=chan,
		    .program=msg[1]);
		break;
	case MIDI_CHN_PRESSURE:
		ev = SEQ_MK_CHN(CHN_PRESSURE, .device=unit, .channel=chan,
		    .pressure=msg[1]);
		break;
	case MIDI_PITCH_BEND:
		ev = SEQ_MK_CHN(PITCH_BEND, .device=unit, .channel=chan,
		    .value=(msg[1] & 0x7f) | ((msg[2] & 0x7f) << 7));
		break;
	default: /* this is now the point where MIDI_ACKs disappear */
		return;
	}
	seq_event_intr(md->seq, &ev);
}

static struct midi_dev *
midiseq_open(int unit, int flags)
{
	extern struct cfdriver midi_cd;
	extern const struct cdevsw midi_cdevsw;
	int error;
	struct midi_dev *md;
	struct midi_softc *sc;
	struct midi_info mi;

	midi_getinfo(makedev(0, unit), &mi);
	if ( !(mi.props & MIDI_PROP_CAN_INPUT) )
	        flags &= ~FREAD;
	if ( 0 == ( flags & ( FREAD | FWRITE ) ) )
	        return 0;
	DPRINTFN(2, ("midiseq_open: %d %d\n", unit, flags));
	error = (*midi_cdevsw.d_open)(makedev(0, unit), flags, 0, 0);
	if (error)
		return (0);
	sc = midi_cd.cd_devs[unit];
	sc->seqopen = 1;
	md = malloc(sizeof *md, M_DEVBUF, M_WAITOK|M_ZERO);
	sc->seq_md = md;
	md->msc = sc;
	md->unit = unit;
	md->name = mi.name;
	md->subtype = 0;
	md->nr_voices = 128;	/* XXX */
	md->instr_bank_size = 128; /* XXX */
	if (mi.props & MIDI_PROP_CAN_INPUT)
		md->capabilities |= SYNTH_CAP_INPUT;
	return (md);
}

static void
midiseq_close(struct midi_dev *md)
{
	extern const struct cdevsw midi_cdevsw;

	DPRINTFN(2, ("midiseq_close: %d\n", md->unit));
	(*midi_cdevsw.d_close)(makedev(0, md->unit), 0, 0, 0);
	free(md, M_DEVBUF);
}

static void
midiseq_reset(struct midi_dev *md)
{
	/* XXX send GM reset? */
	DPRINTFN(3, ("midiseq_reset: %d\n", md->unit));
}

static int
midiseq_out(struct midi_dev *md, u_char *bf, u_int cc, int chk)
{
	DPRINTFN(5, ("midiseq_out: m=%p, unit=%d, bf[0]=0x%02x, cc=%d\n",
		     md->msc, md->unit, bf[0], cc));

	/* midi(4) does running status compression where appropriate. */
	return midi_writebytes(md->unit, bf, cc);
}

/*
 * If the writing process hands us a hidden note-off in a note-on event,
 * we will simply write it that way; no need to special case it here,
 * as midi(4) will always canonicalize or compress as appropriate anyway.
 */
static int
midiseq_noteon(struct midi_dev *md, int chan, int key, seq_event_t *ev)
{
	return midiseq_out(md, (uint8_t[]){
	    MIDI_NOTEON | chan, key, ev->c_NOTEON.velocity & 0x7f}, 3, 1);
}

static int
midiseq_noteoff(struct midi_dev *md, int chan, int key, seq_event_t *ev)
{
	return midiseq_out(md, (uint8_t[]){
	    MIDI_NOTEOFF | chan, key, ev->c_NOTEOFF.velocity & 0x7f}, 3, 1);
}

static int
midiseq_keypressure(struct midi_dev *md, int chan, int key, seq_event_t *ev)
{
	return midiseq_out(md, (uint8_t[]){
	    MIDI_KEY_PRESSURE | chan, key,
	    ev->c_KEY_PRESSURE.pressure & 0x7f}, 3, 1);
}

static int
midiseq_pgmchange(struct midi_dev *md, int chan, seq_event_t *ev)
{
	if (ev->c_PGM_CHANGE.program > 127)
		return EINVAL;
	return midiseq_out(md, (uint8_t[]){
	    MIDI_PGM_CHANGE | chan, ev->c_PGM_CHANGE.program}, 2, 1);
}

static int
midiseq_chnpressure(struct midi_dev *md, int chan, seq_event_t *ev)
{
	if (ev->c_CHN_PRESSURE.pressure > 127)
		return EINVAL;
	return midiseq_out(md, (uint8_t[]){
	    MIDI_CHN_PRESSURE | chan, ev->c_CHN_PRESSURE.pressure}, 2, 1);
}

static int
midiseq_ctlchange(struct midi_dev *md, int chan, seq_event_t *ev)
{
	if (ev->c_CTL_CHANGE.controller > 127)
		return EINVAL;
	return midiseq_out( md, (uint8_t[]){
	    MIDI_CTL_CHANGE | chan, ev->c_CTL_CHANGE.controller,
	    ev->c_CTL_CHANGE.value & 0x7f /* XXX this is SO wrong */
	    }, 3, 1);
}

static int
midiseq_pitchbend(struct midi_dev *md, int chan, seq_event_t *ev)
{
	return midiseq_out(md, (uint8_t[]){
	    MIDI_PITCH_BEND | chan,
	    ev->c_PITCH_BEND.value & 0x7f,
	    (ev->c_PITCH_BEND.value >> 7) & 0x7f}, 3, 1);
}

static int
midiseq_loadpatch(struct midi_dev *md,
                  struct sysex_info *sysex, struct uio *uio)
{
	u_char c, bf[128];
	int i, cc, error;

	if (sysex->key != SEQ_SYSEX_PATCH) {
		DPRINTFN(-1,("midiseq_loadpatch: bad patch key 0x%04x\n",
			     sysex->key));
		return (EINVAL);
	}
	if (uio->uio_resid < sysex->len)
		/* adjust length, should be an error */
		sysex->len = uio->uio_resid;

	DPRINTFN(2, ("midiseq_loadpatch: len=%d\n", sysex->len));
	if (sysex->len == 0)
		return EINVAL;
	error = uiomove(&c, 1, uio);
	if (error)
		return error;
	if (c != MIDI_SYSEX_START)		/* must start like this */
		return EINVAL;
	error = midiseq_out(md, &c, 1, 0);
	if (error)
		return error;
	--sysex->len;
	while (sysex->len > 0) {
		cc = sysex->len;
		if (cc > sizeof bf)
			cc = sizeof bf;
		error = uiomove(bf, cc, uio);
		if (error)
			break;
		for(i = 0; i < cc && !MIDI_IS_STATUS(bf[i]); i++)
			;
		/*
		 * XXX midi(4)'s buffer might not accomodate this, and the
		 * function will not block us (though in this case we have
		 * a process and could in principle block).
		 */
		error = midiseq_out(md, bf, i, 0);
		if (error)
			break;
		sysex->len -= i;
		if (i != cc)
			break;
	}
	/*
	 * Any leftover data in uio is rubbish;
	 * the SYSEX should be one write ending in SYSEX_END.
	 */
	uio->uio_resid = 0;
	c = MIDI_SYSEX_END;
	return midiseq_out(md, &c, 1, 0);
}

#include "midi.h"
#if NMIDI == 0
static dev_type_open(midiopen);
static dev_type_close(midiclose);

const struct cdevsw midi_cdevsw = {
	midiopen, midiclose, noread, nowrite, noioctl,
	nostop, notty, nopoll, nommap,
};

/*
 * If someone has a sequencer, but no midi devices there will
 * be unresolved references, so we provide little stubs.
 */

int
midi_unit_count()
{
	return (0);
}

static int
midiopen(dev_t dev, int flags, int ifmt, struct lwp *l)
{
	return (ENXIO);
}

struct cfdriver midi_cd;

void
midi_getinfo(dev_t dev, struct midi_info *mi)
{
}

static int
midiclose(dev_t dev, int flags, int ifmt, struct lwp *l)
{
	return (ENXIO);
}

int
midi_writebytes(int unit, u_char *bf, int cc)
{
	return (ENXIO);
}
#endif /* NMIDI == 0 */
