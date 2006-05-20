/*	$NetBSD: midi.c,v 1.43.2.6 2006/05/20 03:16:48 chap Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (augustss@NetBSD.org) and (MIDI DFA and Active
 * Sense handling) Chapman Flack (nblists@anastigmatix.net).
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
__KERNEL_RCSID(0, "$NetBSD: midi.c,v 1.43.2.6 2006/05/20 03:16:48 chap Exp $");

#include "midi.h"
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
#include <sys/callout.h>
#include <sys/syslog.h>
#include <sys/kernel.h>
#include <sys/signalvar.h>
#include <sys/conf.h>
#include <sys/audioio.h>
#include <sys/midiio.h>

#include <dev/audio_if.h>
#include <dev/midi_if.h>
#include <dev/midivar.h>

#if NMIDI > 0


#ifdef AUDIO_DEBUG
#define DPRINTF(x)	if (mididebug) printf x
#define DPRINTFN(n,x)	if (mididebug >= (n)) printf x
int	mididebug = 0;
/*
 *      1: detected protocol errors and buffer overflows
 *      2: probe, attach, detach
 *      3: open, close
 *      4: data received except realtime
 *      5: ioctl
 *      6: read, write, poll
 *      7: data transmitted
 *      8: uiomoves, synchronization
 *      9: realtime data received
 */
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

int midi_wait;

void	midi_in(void *, int);
void	midi_out(void *);
int	midi_start_output(struct midi_softc *, int, int);
int	midi_sleep_timo(int *, char *, int);
int	midi_sleep(int *, char *);
void	midi_wakeup(int *);
void	midi_initbuf(struct midi_buffer *);
void	midi_timeout(void *);

int	midiprobe(struct device *, struct cfdata *, void *);
void	midiattach(struct device *, struct device *, void *);
int	mididetach(struct device *, int);
int	midiactivate(struct device *, enum devact);

dev_type_open(midiopen);
dev_type_close(midiclose);
dev_type_read(midiread);
dev_type_write(midiwrite);
dev_type_ioctl(midiioctl);
dev_type_poll(midipoll);
dev_type_kqfilter(midikqfilter);

const struct cdevsw midi_cdevsw = {
	midiopen, midiclose, midiread, midiwrite, midiioctl,
	nostop, notty, midipoll, nommap, midikqfilter,
};

CFATTACH_DECL(midi, sizeof(struct midi_softc),
    midiprobe, midiattach, mididetach, midiactivate);

#ifdef MIDI_SAVE
#define MIDI_SAVE_SIZE 100000
int midicnt;
struct {
	int cnt;
	u_char buf[MIDI_SAVE_SIZE];
} midisave;
#define MIDI_GETSAVE		_IOWR('m', 100, int)

#endif

extern struct cfdriver midi_cd;

int
midiprobe(struct device *parent, struct cfdata *match, void *aux)
{
	struct audio_attach_args *sa = aux;

	DPRINTFN(2,("midiprobe: type=%d sa=%p hw=%p\n", 
		 sa->type, sa, sa->hwif));
	return (sa->type == AUDIODEV_TYPE_MIDI);
}

void
midiattach(struct device *parent, struct device *self, void *aux)
{
	struct midi_softc *sc = (void *)self;
	struct audio_attach_args *sa = aux;
	struct midi_hw_if *hwp = sa->hwif;
	void *hdlp = sa->hdl;

	DPRINTFN(2, ("MIDI attach\n"));

#ifdef DIAGNOSTIC
	if (hwp == 0 ||
	    hwp->open == 0 ||
	    hwp->close == 0 ||
	    hwp->output == 0 ||
	    hwp->getinfo == 0) {
		printf("midi: missing method\n");
		return;
	}
#endif

	sc->hw_if = hwp;
	sc->hw_hdl = hdlp;
	midi_attach(sc, parent);
}

int
midiactivate(struct device *self, enum devact act)
{
	struct midi_softc *sc = (struct midi_softc *)self;

	switch (act) {
	case DVACT_ACTIVATE:
		return (EOPNOTSUPP);

	case DVACT_DEACTIVATE:
		sc->dying = 1;
		break;
	}
	return (0);
}

int
mididetach(struct device *self, int flags)
{
	struct midi_softc *sc = (struct midi_softc *)self;
	int maj, mn;

	DPRINTFN(2,("midi_detach: sc=%p flags=%d\n", sc, flags));

	sc->dying = 1;

	wakeup(&sc->wchan);
	wakeup(&sc->rchan);

	/* locate the major number */
	maj = cdevsw_lookup_major(&midi_cdevsw);

	/* Nuke the vnodes for any open instances (calls close). */
	mn = self->dv_unit;
	vdevgone(maj, mn, mn, VCHR);
	
	evcnt_detach(&sc->xmt.bytesDiscarded);
	evcnt_detach(&sc->xmt.incompleteMessages);
	if ( sc->props & MIDI_PROP_CAN_INPUT ) {
		evcnt_detach(&sc->rcv.bytesDiscarded);
		evcnt_detach(&sc->rcv.incompleteMessages);
	}

	return (0);
}

void
midi_attach(struct midi_softc *sc, struct device *parent)
{
	struct midi_info mi;


	callout_init(&sc->sc_callout);
	callout_setfunc(&sc->sc_callout, midi_timeout, sc);
	simple_lock_init(&sc->out_lock);
	sc->dying = 0;
	sc->isopen = 0;

	midi_wait = MIDI_WAIT * hz / 1000000;
	if (midi_wait == 0)
		midi_wait = 1;

	sc->sc_dev = parent;
	sc->hw_if->getinfo(sc->hw_hdl, &mi);
	sc->props = mi.props;
	
	evcnt_attach_dynamic(&sc->xmt.bytesDiscarded,
		EVCNT_TYPE_MISC, NULL,
		sc->dev.dv_xname, "xmt bytes discarded");
	evcnt_attach_dynamic(&sc->xmt.incompleteMessages,
		EVCNT_TYPE_MISC, NULL,
		sc->dev.dv_xname, "xmt incomplete msgs");
	if ( sc->props & MIDI_PROP_CAN_INPUT ) {
		evcnt_attach_dynamic(&sc->rcv.bytesDiscarded,
			EVCNT_TYPE_MISC, NULL,
			sc->dev.dv_xname, "rcv bytes discarded");
		evcnt_attach_dynamic(&sc->rcv.incompleteMessages,
			EVCNT_TYPE_MISC, NULL,
			sc->dev.dv_xname, "rcv incomplete msgs");
	}
	
	printf(": %s\n", mi.name);
}

int
midi_unit_count(void)
{
	int i;
	for ( i = 0; i < midi_cd.cd_ndevs; ++i )
	        if ( NULL == midi_cd.cd_devs[i] )
		        break;
        return i;
}

void
midi_initbuf(struct midi_buffer *mb)
{
	mb->used = 0;
	mb->usedhigh = MIDI_BUFSIZE;
	mb->end = mb->start + mb->usedhigh;
	mb->inp = mb->outp = mb->start;
}

int
midi_sleep_timo(int *chan, char *label, int timo)
{
	int st;

	if (!label)
		label = "midi";

	DPRINTFN(8, ("midi_sleep_timo: %p %s %d\n", chan, label, timo));
	*chan = 1;
	st = tsleep(chan, PWAIT | PCATCH, label, timo);
	*chan = 0;
#ifdef MIDI_DEBUG
	if (st != 0)
		printf("midi_sleep: %d\n", st);
#endif
	return st;
}

int
midi_sleep(int *chan, char *label)
{
	return midi_sleep_timo(chan, label, 0);
}

void
midi_wakeup(int *chan)
{
	if (*chan) {
		DPRINTFN(8, ("midi_wakeup: %p\n", chan));
		wakeup(chan);
		*chan = 0;
	}
}

/* in midivar.h:
#define MIDI_CAT_DATA 0
#define MIDI_CAT_STATUS1 1
#define MIDI_CAT_STATUS2 2
#define MIDI_CAT_COMMON 3
*/
static char const midi_cats[] = "\0\0\0\0\0\0\0\0\2\2\2\2\1\1\2\3";
#define MIDI_CAT(d) (midi_cats[((d)>>4)&15])
#define DFA_RETURN(offp,endp,ret) \
	return (s->pos=s->msg+(offp)), (s->end=s->msg+(endp)), (ret)

enum dfa_ret { DFA_MORE, DFA_CHN, DFA_COM, DFA_SYX, DFA_RT, DFA_ERR, DFA_HUH };

/*
 * A MIDI state machine suitable for receiving or transmitting. It will
 * accept correct MIDI input that uses, doesn't use, or sometimes uses the
 * 'running status' compression technique, and transduce it to fully expanded
 * (compress==0) or fully compressed (compress==1) form.
 *
 * Returns DFA_MORE if a complete message has not been parsed yet (SysEx
 * messages are the exception), DFA_ERR or DFA_HUH if the input does not
 * conform to the protocol, or DFA_CHN (channel messages), DFA_COM (System
 * Common messages), DFA_RT (System Real-Time messages), or DFA_SYX (System
 * Exclusive) to broadly categorize the message parsed. s->pos and s->end
 * locate the parsed message; while (s->pos<s->end) putchar(*(s->pos++));
 * would output it.
 *
 * DFA_HUH means the character c wasn't valid in the original state, but the
 * state has now been reset to START and the caller should try again passing
 * the same c. DFA_ERR means c isn't valid in the start state; the caller
 * should kiss it goodbye and continue to try successive characters from the
 * input until something other than DFA_ERR or DFA_HUH is returned, at which
 * point things are resynchronized.
 *
 * A DFA_SYX return means that between pos and end are from 1 to 3
 * bytes of a system exclusive message. A SysEx message will be delivered in
 * one or more chunks of that form, where the first begins with 0xf0 and the
 * last (which is the only one that might have length < 3) ends with 0xf7.
 *
 * Messages corrupted by a protocol error are discarded and won't be seen at
 * all; again SysEx is the exception, as one or more chunks of it may already
 * have been parsed.
 *
 * For DFA_CHN messages, s->msg[0] always contains the status byte even if
 * compression was requested (pos then points to msg[1]). That way, the
 * caller can always identify the exact message if there is a need to do so.
 * For all other message types except DFA_SYX, the status byte is at *pos
 * (which may not necessarily be msg[0]!). There is only one SysEx status
 * byte, so the return value DFA_SYX is sufficient to identify it.
 */
static enum dfa_ret
midi_dfa(struct midi_state *s, u_char c, int compress)
{
	int syxpos = 0;
	compress = compress ? 1 : 0;

	if ( c >= 0xf8 ) { /* All realtime messages bypass state machine */
	        if ( c == 0xf9  ||  c == 0xfd ) {
			DPRINTF( ("midi_dfa: s=%p c=0x%02x undefined\n", 
				  s, c));
			s->bytesDiscarded.ev_count++;
			return DFA_ERR;
		}
		DPRINTFN(9, ("midi_dfa: s=%p System Real-Time data=0x%02x\n", 
			     s, c));
		s->msg[2] = c;
		DFA_RETURN(2,3,DFA_RT);
	}

	DPRINTFN(4, ("midi_dfa: s=%p data=0x%02x state=%d\n", 
		     s, c, s->state));

        switch ( s->state   | MIDI_CAT(c) ) { /* break ==> return DFA_MORE */

	case MIDI_IN_START  | MIDI_CAT_COMMON:
	case MIDI_IN_RUN1_1 | MIDI_CAT_COMMON:
	case MIDI_IN_RUN2_2 | MIDI_CAT_COMMON:
	        s->msg[0] = c;
	        switch ( c ) {
		case 0xf0: s->state = MIDI_IN_SYX1_3; break;
		case 0xf1: s->state = MIDI_IN_COM0_1; break;
		case 0xf2: s->state = MIDI_IN_COM0_2; break;
		case 0xf3: s->state = MIDI_IN_COM0_1; break;
		case 0xf6: s->state = MIDI_IN_START;  DFA_RETURN(0,1,DFA_COM);
		default: goto protocol_violation;
		}
		break;
	
	case MIDI_IN_RUN1_1 | MIDI_CAT_STATUS1:
		if ( c == s->msg[0] ) {
			s->state = MIDI_IN_RNX0_1;
			break;
		}
		/* FALLTHROUGH */
	case MIDI_IN_RUN2_2 | MIDI_CAT_STATUS1:
	case MIDI_IN_START  | MIDI_CAT_STATUS1:
	        s->state = MIDI_IN_RUN0_1;
	        s->msg[0] = c;
		break;
	
	case MIDI_IN_RUN2_2 | MIDI_CAT_STATUS2:
		if ( c == s->msg[0] ) {
			s->state = MIDI_IN_RNX0_2;
			break;
		}
		/* FALLTHROUGH */
	case MIDI_IN_RUN1_1 | MIDI_CAT_STATUS2:
	case MIDI_IN_START  | MIDI_CAT_STATUS2:
	        s->state = MIDI_IN_RUN0_2;
	        s->msg[0] = c;
		break;

        case MIDI_IN_COM0_1 | MIDI_CAT_DATA:
		s->state = MIDI_IN_START;
	        s->msg[1] = c;
		DFA_RETURN(0,2,DFA_COM);

        case MIDI_IN_COM0_2 | MIDI_CAT_DATA:
	        s->state = MIDI_IN_COM1_2;
	        s->msg[1] = c;
		break;

        case MIDI_IN_COM1_2 | MIDI_CAT_DATA:
		s->state = MIDI_IN_START;
	        s->msg[2] = c;
		DFA_RETURN(0,3,DFA_COM);

        case MIDI_IN_RUN0_1 | MIDI_CAT_DATA:
		s->state = MIDI_IN_RUN1_1;
	        s->msg[1] = c;
		DFA_RETURN(0,2,DFA_CHN);

        case MIDI_IN_RUN1_1 | MIDI_CAT_DATA:
        case MIDI_IN_RNX0_1 | MIDI_CAT_DATA:
		s->state = MIDI_IN_RUN1_1;
	        s->msg[1] = c;
		DFA_RETURN(compress,2,DFA_CHN);

        case MIDI_IN_RUN0_2 | MIDI_CAT_DATA:
	        s->state = MIDI_IN_RUN1_2;
	        s->msg[1] = c;
		break;

        case MIDI_IN_RUN1_2 | MIDI_CAT_DATA:
		s->state = MIDI_IN_RUN2_2;
	        s->msg[2] = c;
		DFA_RETURN(0,3,DFA_CHN);

        case MIDI_IN_RUN2_2 | MIDI_CAT_DATA:
	        s->state = MIDI_IN_RNX1_2;
	        s->msg[1] = c;
		break;

        case MIDI_IN_RNX0_2 | MIDI_CAT_DATA:
	        s->state = MIDI_IN_RNY1_2;
	        s->msg[1] = c;
		break;

        case MIDI_IN_RNX1_2 | MIDI_CAT_DATA:
        case MIDI_IN_RNY1_2 | MIDI_CAT_DATA:
		s->state = MIDI_IN_RUN2_2;
	        s->msg[2] = c;
		DFA_RETURN(compress,3,DFA_CHN);

        case MIDI_IN_SYX1_3 | MIDI_CAT_DATA:
		s->state = MIDI_IN_SYX2_3;
	        s->msg[1] = c;
		break;

        case MIDI_IN_SYX2_3 | MIDI_CAT_DATA:
		s->state = MIDI_IN_SYX0_3;
	        s->msg[2] = c;
		DFA_RETURN(0,3,DFA_SYX);

        case MIDI_IN_SYX0_3 | MIDI_CAT_DATA:
		s->state = MIDI_IN_SYX1_3;
	        s->msg[0] = c;
		break;

        case MIDI_IN_SYX2_3 | MIDI_CAT_COMMON:
		++ syxpos;
		/* FALLTHROUGH */
        case MIDI_IN_SYX1_3 | MIDI_CAT_COMMON:
		++ syxpos;
		/* FALLTHROUGH */
        case MIDI_IN_SYX0_3 | MIDI_CAT_COMMON:
	        if ( c == 0xf7 ) {
		        s->state = MIDI_IN_START;
			s->msg[syxpos] = c;
		        DFA_RETURN(0,1+syxpos,DFA_SYX);
		}
		/* FALLTHROUGH */

        default:
protocol_violation:
                DPRINTF(("midi_dfa: unexpected %#02x in state %u\n",
		        c, s->state));
		switch ( s->state ) {
		case MIDI_IN_RUN1_1: /* can only get here by seeing an */
		case MIDI_IN_RUN2_2: /* INVALID System Common message */
		        s->state = MIDI_IN_START;
			/* FALLTHROUGH */
		case MIDI_IN_START:
			s->bytesDiscarded.ev_count++;
			return DFA_ERR;
		case MIDI_IN_COM1_2:
		case MIDI_IN_RUN1_2:
		case MIDI_IN_SYX2_3:
		case MIDI_IN_RNY1_2:
			s->bytesDiscarded.ev_count++;
			/* FALLTHROUGH */
		case MIDI_IN_COM0_1:
		case MIDI_IN_RUN0_1:
		case MIDI_IN_RNX0_1:
		case MIDI_IN_COM0_2:
		case MIDI_IN_RUN0_2:
		case MIDI_IN_RNX0_2:
		case MIDI_IN_RNX1_2:
		case MIDI_IN_SYX1_3:
			s->bytesDiscarded.ev_count++;
			/* FALLTHROUGH */
		case MIDI_IN_SYX0_3:
		        s->incompleteMessages.ev_count++;
			break;
#if defined(AUDIO_DEBUG) || defined(DIAGNOSTIC)
		default:
		        printf("midi_dfa: mishandled %#02x(%u) in state %u?!\n",
			      c, MIDI_CAT(c), s->state);
#endif
		}
		s->state = MIDI_IN_START;
		return DFA_HUH;
	}
	return DFA_MORE;
}

void
midi_in(void *addr, int data)
{
	struct midi_softc *sc = addr;
	struct midi_buffer *mb = &sc->inbuf;
	int i;
	int count;
	enum dfa_ret got;

	if (!sc->isopen)
		return;

	if (!(sc->flags & FREAD))
		return;		/* discard data if not reading */
	
	do
		got = midi_dfa(&sc->rcv, data, 0);
	while ( got == DFA_HUH );
	
	switch ( got ) {
	case DFA_MORE:
	case DFA_ERR:
		return;
	case DFA_CHN:
	case DFA_COM:
	case DFA_RT:
#if NSEQUENCER > 0
		if (sc->seqopen) {
			extern void midiseq_in(struct midi_dev *,u_char *,int);
			count = sc->rcv.end - sc->rcv.pos;
			midiseq_in(sc->seq_md, sc->rcv.pos, count);
			return;
		}
#endif
        	/*
		 * Pass Active Sense to the sequencer if it's open, but not to
		 * a raw reader. (Really should do something intelligent with
		 * it then, though....)
		 */
		if ( got == DFA_RT && MIDI_ACK == sc->rcv.pos[0] )
			return;
		/* FALLTHROUGH */
	/*
	 * Ultimately SysEx msgs should be offered to the sequencer also; the
	 * sequencer API addresses them - but maybe our sequencer can't handle
	 * them yet, so offer only to raw reader. (Which means, ultimately,
	 * discard them if the sequencer's open, as it's not doing reads!)
	 */
	case DFA_SYX:
		count = sc->rcv.end - sc->rcv.pos;
		if (mb->used + count > mb->usedhigh) {
			DPRINTF(("midi_in: buffer full, discard data=0x%02x\n", 
				 sc->rcv.pos[0]));
			sc->rcv.bytesDiscarded.ev_count += count;
			return;
		}
		for (i = 0; i < count; i++) {
			*mb->inp++ = sc->rcv.pos[i];
			if (mb->inp >= mb->end)
				mb->inp = mb->start;
			mb->used++;
		}
		midi_wakeup(&sc->rchan);
		selnotify(&sc->rsel, 0);
		if (sc->async)
			psignal(sc->async, SIGIO);
		break;
#if defined(AUDIO_DEBUG) || defined(DIAGNOSTIC)
	default:
		printf("midi_in: midi_dfa returned %d?!\n", got);
#endif
	}
}

void
midi_out(void *addr)
{
	struct midi_softc *sc = addr;

	if (!sc->isopen)
		return;
	DPRINTFN(8, ("midi_out: %p\n", sc));
	midi_start_output(sc, 0, 1);
}

int
midiopen(dev_t dev, int flags, int ifmt, struct proc *p)
{
	struct midi_softc *sc;
	struct midi_hw_if *hw;
	int error;

	sc = device_lookup(&midi_cd, MIDIUNIT(dev));
	if (sc == NULL)
		return (ENXIO);
	if (sc->dying)
		return (EIO);

	DPRINTFN(3,("midiopen %p\n", sc));

	hw = sc->hw_if;
	if (!hw)
		return ENXIO;
	if (sc->isopen)
		return EBUSY;
	sc->rcv.state = MIDI_IN_START;
	sc->xmt.state = MIDI_IN_START;
	sc->xmt.pos = sc->xmt.msg;
	sc->xmt.end = sc->xmt.msg;
	error = hw->open(sc->hw_hdl, flags, midi_in, midi_out, sc);
	if (error)
		return error;
	sc->isopen++;
	midi_initbuf(&sc->outbuf);
	midi_initbuf(&sc->inbuf);
	sc->flags = flags;
	sc->rchan = 0;
	sc->wchan = 0;
	sc->pbus = 0;
	sc->async = 0;

#ifdef MIDI_SAVE
	if (midicnt != 0) {
		midisave.cnt = midicnt;
		midicnt = 0;
	}
#endif

	return 0;
}

int
midiclose(dev_t dev, int flags, int ifmt, struct proc *p)
{
	int unit = MIDIUNIT(dev);
	struct midi_softc *sc = midi_cd.cd_devs[unit];
	struct midi_hw_if *hw = sc->hw_if;
	int s, error;

	DPRINTFN(3,("midiclose %p\n", sc));

	midi_start_output(sc, 1, 0);
	error = 0;
	s = splaudio();
	while (sc->outbuf.used > 0 && !error) {
		DPRINTFN(8,("midiclose sleep used=%d\n", sc->outbuf.used));
		error = midi_sleep_timo(&sc->wchan, "mid_dr", 30*hz);
	}
	splx(s);
	callout_stop(&sc->sc_callout);
	sc->isopen = 0;
	hw->close(sc->hw_hdl);
#if NSEQUENCER > 0
	sc->seqopen = 0;
	sc->seq_md = 0;
#endif
	return 0;
}

int
midiread(dev_t dev, struct uio *uio, int ioflag)
{
	int unit = MIDIUNIT(dev);
	struct midi_softc *sc = midi_cd.cd_devs[unit];
	struct midi_buffer *mb = &sc->inbuf;
	int error;
	u_char *outp;
	int used, cc, n, resid;
	int s;

	DPRINTFN(6,("midiread: %p, count=%lu\n", sc, 
		 (unsigned long)uio->uio_resid));

	if (sc->dying)
		return EIO;
        if ( !(sc->flags & FREAD) )
	        return EBADF;
        if ( !(sc->props & MIDI_PROP_CAN_INPUT) )
	        return ENXIO;

	error = 0;
	resid = uio->uio_resid;
	while (uio->uio_resid == resid && !error) {
		s = splaudio();
		while (mb->used <= 0) {
			if (ioflag & IO_NDELAY) {
				splx(s);
				return EWOULDBLOCK;
			}
			error = midi_sleep(&sc->rchan, "mid rd");
			if (error) {
				splx(s);
				return error;
			}
		}
		used = mb->used;
		outp = mb->outp;
		splx(s);
		if (sc->dying)
			return EIO;
		cc = used;	/* maximum to read */
		n = mb->end - outp;
		if (n < cc)
			cc = n;	/* don't read beyond end of buffer */
		if (uio->uio_resid < cc)
			cc = uio->uio_resid; /* and no more than we want */
		DPRINTFN(8, ("midiread: uiomove cc=%d\n", cc));
		error = uiomove(outp, cc, uio);
		if (error)
			break;
		used -= cc;
		outp += cc;
		if (outp >= mb->end)
			outp = mb->start;
		s = splaudio();
		mb->outp = outp;
		mb->used = used;
		splx(s);
	}
	return error;
}

void
midi_timeout(void *arg)
{
	struct midi_softc *sc = arg;

	DPRINTFN(8,("midi_timeout: %p\n", sc));
	midi_start_output(sc, 0, 0);
}

/*
 * Control can reach midi_start_output three ways:
 * 1. as a result of user writing (user == 1)
 * 2. by a ready interrupt from output hw (hw == 1)
 * 3. by a callout scheduled at the end of midi_start_output.
 *
 * The callout is not scheduled unless we are sure the hw will not
 * interrupt again (either because it just did and we supplied it
 * no data, or because it lacks interrupt capability). So we do not
 * expect a hw interrupt to occur with a callout pending.
 *
 * A user write may occur while the callout is pending, however, or before
 * an interrupt-capable device has interrupted. There are two cases:
 *
 * 1. A device interrupt is expected, or the pending callout is a MIDI_WAIT
 *    (the brief delay we put in if MIDI_MAX_WRITE characters have been
 *    written without yielding; interrupt-capable hardware can avoid this
 *    penalty by making sure not to use EINPROGRESS to consume MIDI_MAX_WRITE
 *    or more characters at once).  In these cases, the user invocation of
 *    start_output returns without action, leaving the output to be resumed
 *    by the device interrupt or by the callout after the proper delay. This
 *    case is identified by sc->pbus == 1.
 *
 * 2. The pending callout was scheduled to ensure an Active Sense gets
 *    written if there is no user write in time. In this case the user
 *    invocation (if there is something to write) cancels the pending
 *    callout. However, it may be possible the callout timer expired before
 *    we could cancel it, but the callout hasn't run yet. In this case
 *    (INVOKING status on the callout) the user invocation returns without
 *    further action so the callout can do the work.
 *
 * Is it possible that fast interrupt-driven hardware could interrupt while
 * the start_output that triggered it still holds the lock? In this case we'll
 * see on unlock that sc->hw_interrupted == 1 and loop around again if there
 * is more to write.
 */
int
midi_start_output(struct midi_softc *sc, int user, int hw)
{
	struct midi_buffer *mb = &sc->outbuf;
	u_char out;
	int error;
	int s;
	int written;
	int to_write;
	u_char *from;
	u_char *to;

again:
if ( mididebug > 9 ) return EIO;
	error = 0;

	if (sc->dying)
		return EIO;

	s = splaudio();

	if ( ! simple_lock_try(&sc->out_lock) ) {
	        if ( hw )
		        sc->hw_interrupted = 1;
		splx(s);
		DPRINTFN(8,("midi_start_output: locked out user=%d hw=%d\n",
                           user, hw));
		return 0;
	}

	to_write = mb->used;

	if ( user ) {
	        if ( sc->pbus ) {
			simple_unlock(&sc->out_lock);
			splx(s);
			DPRINTFN(8, ("midi_start_output: delaying\n"));
			return 0;
		}
		if ( to_write > 0 ) {
		        callout_stop(&sc->sc_callout);
			if (callout_invoking(&sc->sc_callout)) {
				simple_unlock(&sc->out_lock);
				splx(s);
				DPRINTFN(8,("midi_start_output: "
                                           "callout got away\n"));
				return 0;
			}
		}
	} else if ( hw )
		sc->hw_interrupted = 0;
	else
	        callout_ack(&sc->sc_callout);
	
	splx(s);
	/*
	 * As long as we hold the lock, we can rely on mb->outp not changing
	 * and mb->used not decreasing. We only need splaudio to update them.
	 */
	
	from = mb->outp;
	
	DPRINTFN(8,("midi_start_output: user=%d hw=%d to_write %u from %p\n",
		   user, hw, to_write, from));
	sc->pbus = 0;
	
	written = 0;
	if ( to_write == 0 && ! user && ! hw ) {
		error = sc->hw_if->output(sc->hw_hdl, MIDI_ACK);
		written = 1;
	}
	
	do {
	        if ( to_write > MIDI_MAX_WRITE - written )
		        to_write = MIDI_MAX_WRITE - written;
		if ( to_write == 0 )
			break;
		to = from + to_write;
		if ( to > mb->end )
		        to = mb->end;
		while ( from < to ) {
			out = *from++;
#ifdef MIDI_SAVE
			midisave.buf[midicnt] = out;
			midicnt = (midicnt + 1) % MIDI_SAVE_SIZE;
#endif
			DPRINTFN(8, ("midi_start_output: %p data=0x%02x\n", 
				     sc, out));
			error = sc->hw_if->output(sc->hw_hdl, out);
			if ( error == 0 ) {
				if ( sc->props & MIDI_PROP_OUT_INTR )
					break;
			} else if ( ( sc->props & MIDI_PROP_OUT_INTR )
			            && error == EINPROGRESS )
				continue;
			else {  /* really an error */
				-- from;
				break;
			}
		}
		written += from - mb->outp;
		s = splaudio();
		mb->used -= from - mb->outp;
		if ( from == mb->end )
			from = mb->start;
		mb->outp = from;
		splx(s);
		to_write = mb->used;
	} while ( error == ( sc->props&MIDI_PROP_OUT_INTR ) ? EINPROGRESS : 0 );
	
	if ( written > 0 ) {
		midi_wakeup(&sc->wchan);
		selnotify(&sc->wsel, 0);
		if (sc->async)
			psignal(sc->async, SIGIO);
        }

	if ( sc->props & MIDI_PROP_OUT_INTR ) {
		if ( written > 0 && error == 0 ) {
			sc->pbus = 1;
			goto unlock_exit;
		}
		if ( error == EINPROGRESS )
			error = 0;
	}
	
	if ( error != 0 )
		goto handle_error;
	
	if ( to_write == 0 )
		callout_schedule(&sc->sc_callout, mstohz(285));
	else {
		sc->pbus = 1;
		callout_schedule(&sc->sc_callout, midi_wait);
	}
	goto unlock_exit;

handle_error:
#if defined(AUDIO_DEBUG) || defined(DIAGNOSTIC)
		printf("%s: midi_start_output error %d\n",
		      sc->dev.dv_xname, error);
#endif

unlock_exit:
	simple_unlock(&sc->out_lock);
	if ( sc->hw_interrupted ) {
		user = 0;
		hw = 1;
		goto again;
	}

	return error;
}

int
midiwrite(dev_t dev, struct uio *uio, int ioflag)
{
	int unit = MIDIUNIT(dev);
	struct midi_softc *sc = midi_cd.cd_devs[unit];
	struct midi_buffer *mb = &sc->outbuf;
	int error;
	u_char *inp;
	int used, cc, n;
	int s;

	DPRINTFN(6, ("midiwrite: %p, unit=%d, count=%lu\n", sc, unit, 
		     (unsigned long)uio->uio_resid));

	if (sc->dying)
		return EIO;
        if ( !(sc->flags & FWRITE) )
	        return EBADF;

	error = 0;
	while (uio->uio_resid > 0 && !error) {
		s = splaudio();
		if (mb->used >= mb->usedhigh) {
			DPRINTFN(8,("midi_write: sleep used=%d hiwat=%d\n", 
				 mb->used, mb->usedhigh));
			if (ioflag & IO_NDELAY) {
				splx(s);
				return EWOULDBLOCK;
			}
			error = midi_sleep(&sc->wchan, "mid wr");
			if (error) {
				splx(s);
				return error;
			}
		}			
		used = mb->used;
		inp = mb->inp;
		splx(s);
		if (sc->dying)
			return EIO;
		cc = mb->usedhigh - used; 	/* maximum to write */
		n = mb->end - inp;
		if (n < cc)
			cc = n;		/* don't write beyond end of buffer */
		if (uio->uio_resid < cc)
			cc = uio->uio_resid; 	/* and no more than we have */
		error = uiomove(inp, cc, uio);
#ifdef MIDI_DEBUG
		if (error)
		        printf("midi_write:(1) uiomove failed %d; "
			       "cc=%d inp=%p\n",
			       error, cc, inp);
#endif
		if (error)
			break;
		inp = mb->inp + cc;
		if (inp >= mb->end)
			inp = mb->start;
		s = splaudio();
		mb->inp = inp;
		mb->used += cc;
		splx(s);
		error = midi_start_output(sc, 1, 0);
	}
	return error;
}

/*
 * This write routine is only called from sequencer code and expects
 * a write that is smaller than the MIDI buffer.
 */
int
midi_writebytes(int unit, u_char *buf, int cc)
{
	struct midi_softc *sc = midi_cd.cd_devs[unit];
	struct midi_buffer *mb = &sc->outbuf;
	int n, s;

	DPRINTFN(7, ("midi_writebytes: %p, unit=%d, cc=%d %#02x %#02x %#02x\n",
                    sc, unit, cc, buf[0], buf[1], buf[2]));

	if (sc->dying)
		return EIO;

	s = splaudio();
	if (mb->used + cc >= mb->usedhigh) {
		splx(s);
		return (EWOULDBLOCK);
	}
	n = mb->end - mb->inp;
	if (cc < n)
		n = cc;
	mb->used += cc;
	memcpy(mb->inp, buf, n);
	mb->inp += n;
	if (mb->inp >= mb->end) {
		mb->inp = mb->start;
		cc -= n;
		if (cc > 0) {
			memcpy(mb->inp, buf + n, cc);
			mb->inp += cc;
		}
	}
	splx(s);
	return (midi_start_output(sc, 1, 0));
}

int
midiioctl(dev_t dev, u_long cmd, caddr_t addr, int flag, struct proc *p)
{
	int unit = MIDIUNIT(dev);
	struct midi_softc *sc = midi_cd.cd_devs[unit];
	struct midi_hw_if *hw = sc->hw_if;
	int error;

	DPRINTFN(5,("midiioctl: %p cmd=0x%08lx\n", sc, cmd));

	if (sc->dying)
		return EIO;

	error = 0;
	switch (cmd) {
	case FIONBIO:
		/* All handled in the upper FS layer. */
		break;

	case FIOASYNC:
		if (*(int *)addr) {
			if (sc->async)
				return EBUSY;
			sc->async = p;
			DPRINTFN(5,("midi_ioctl: FIOASYNC %p\n", p));
		} else
			sc->async = 0;
		break;

#if 0
	case MIDI_PRETIME:
		/* XXX OSS
		 * This should set up a read timeout, but that's
		 * why we have poll(), so there's nothing yet. */
		error = EINVAL;
		break;
#endif

#ifdef MIDI_SAVE
	case MIDI_GETSAVE:
		error = copyout(&midisave, *(void **)addr, sizeof midisave);
  		break;
#endif

	default:
		if (hw->ioctl)
			error = hw->ioctl(sc->hw_hdl, cmd, addr, flag, p);
		else
			error = EINVAL;
		break;
	}
	return error;
}

int
midipoll(dev_t dev, int events, struct proc *p)
{
	int unit = MIDIUNIT(dev);
	struct midi_softc *sc = midi_cd.cd_devs[unit];
	int revents = 0;
	int s;

	DPRINTFN(6,("midipoll: %p events=0x%x\n", sc, events));

	if (sc->dying)
		return EIO;

	s = splaudio();

	if ((sc->flags&FREAD) && (events & (POLLIN | POLLRDNORM)))
		if (sc->inbuf.used > 0)
			revents |= events & (POLLIN | POLLRDNORM);

	if ((sc->flags&FWRITE) && (events & (POLLOUT | POLLWRNORM)))
		if (sc->outbuf.used < sc->outbuf.usedhigh)
			revents |= events & (POLLOUT | POLLWRNORM);

	if (revents == 0) {
		if ((sc->flags&FREAD) && (events & (POLLIN | POLLRDNORM)))
			selrecord(p, &sc->rsel);

		if ((sc->flags&FWRITE) && (events & (POLLOUT | POLLWRNORM)))
			selrecord(p, &sc->wsel);
	}

	splx(s);
	return revents;
}

static void
filt_midirdetach(struct knote *kn)
{
	struct midi_softc *sc = kn->kn_hook;
	int s;

	s = splaudio();
	SLIST_REMOVE(&sc->rsel.sel_klist, kn, knote, kn_selnext);
	splx(s);
}

static int
filt_midiread(struct knote *kn, long hint)
{
	struct midi_softc *sc = kn->kn_hook;

	/* XXXLUKEM (thorpej): please make sure this is correct. */

	kn->kn_data = sc->inbuf.used;
	return (kn->kn_data > 0);
}

static const struct filterops midiread_filtops =
	{ 1, NULL, filt_midirdetach, filt_midiread };

static void
filt_midiwdetach(struct knote *kn)
{
	struct midi_softc *sc = kn->kn_hook;
	int s;

	s = splaudio();
	SLIST_REMOVE(&sc->wsel.sel_klist, kn, knote, kn_selnext);
	splx(s);
}

static int
filt_midiwrite(struct knote *kn, long hint)
{
	struct midi_softc *sc = kn->kn_hook;

	/* XXXLUKEM (thorpej): please make sure this is correct. */

	kn->kn_data = sc->outbuf.usedhigh - sc->outbuf.used;
	return (kn->kn_data > 0);
}

static const struct filterops midiwrite_filtops =
	{ 1, NULL, filt_midiwdetach, filt_midiwrite };

int
midikqfilter(dev_t dev, struct knote *kn)
{
	int unit = MIDIUNIT(dev);
	struct midi_softc *sc = midi_cd.cd_devs[unit];
	struct klist *klist;
	int s;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &sc->rsel.sel_klist;
		kn->kn_fop = &midiread_filtops;
		break;

	case EVFILT_WRITE:
		klist = &sc->wsel.sel_klist;
		kn->kn_fop = &midiwrite_filtops;
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

void
midi_getinfo(dev_t dev, struct midi_info *mi)
{
	struct midi_softc *sc;

	sc = device_lookup(&midi_cd, MIDIUNIT(dev));
	if (sc == NULL)
		return;
	if (sc->dying)
		return;

	sc->hw_if->getinfo(sc->hw_hdl, mi);
}

#endif /* NMIDI > 0 */

#if NMIDI > 0 || NMIDIBUS > 0

int	audioprint(void *, const char *);

struct device *
midi_attach_mi(struct midi_hw_if *mhwp, void *hdlp, struct device *dev)
{
	struct audio_attach_args arg;

#ifdef DIAGNOSTIC
	if (mhwp == NULL) {
		aprint_error("midi_attach_mi: NULL\n");
		return (0);
	}
#endif
	arg.type = AUDIODEV_TYPE_MIDI;
	arg.hwif = mhwp;
	arg.hdl = hdlp;
	return (config_found(dev, &arg, audioprint));
}

#endif /* NMIDI > 0 || NMIDIBUS > 0 */
