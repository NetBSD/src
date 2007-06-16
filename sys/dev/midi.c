/*	$NetBSD: midi.c,v 1.54 2007/06/16 10:25:03 pavel Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (augustss@NetBSD.org) and (MIDI FST and Active
 * Sense handling) Chapman Flack (chap@NetBSD.org).
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
__KERNEL_RCSID(0, "$NetBSD: midi.c,v 1.54 2007/06/16 10:25:03 pavel Exp $");

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

static	struct simplelock hwif_register_lock = SIMPLELOCK_INITIALIZER;
static	struct midi_softc *hwif_softc = NULL;

void	midi_in(void *, int);
void	midi_out(void *);
int     midi_poll_out(struct midi_softc *);
int     midi_intr_out(struct midi_softc *);
int 	midi_msg_out(struct midi_softc *,
                 u_char **, u_char **, u_char **, u_char **);
int	midi_start_output(struct midi_softc *);
int	midi_sleep_timo(int *, const char *, int, struct simplelock *);
int	midi_sleep(int *, const char *, struct simplelock *);
void	midi_wakeup(int *);
void	midi_initbuf(struct midi_buffer *);
void	midi_xmt_asense(void *);
void	midi_rcv_asense(void *);
void	midi_softintr_rd(void *);
void	midi_softintr_wr(void *);

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
	nostop, notty, midipoll, nommap, midikqfilter, D_OTHER,
};

CFATTACH_DECL(midi, sizeof(struct midi_softc),
    midiprobe, midiattach, mididetach, midiactivate);

#define MIDI_XMT_ASENSE_PERIOD mstohz(275)
#define MIDI_RCV_ASENSE_PERIOD mstohz(300)

extern struct cfdriver midi_cd;

int
midiprobe(struct device *parent, struct cfdata *match,
    void *aux)
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
	const struct midi_hw_if *hwp = sa->hwif;
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
	mn = device_unit(self);
	vdevgone(maj, mn, mn, VCHR);
	
	if ( !(sc->props & MIDI_PROP_NO_OUTPUT) ) {
		evcnt_detach(&sc->xmt.bytesDiscarded);
		evcnt_detach(&sc->xmt.incompleteMessages);
	}
	if ( sc->props & MIDI_PROP_CAN_INPUT ) {
		evcnt_detach(&sc->rcv.bytesDiscarded);
		evcnt_detach(&sc->rcv.incompleteMessages);
	}

	if (sc->sih_rd != NULL) {
		softintr_disestablish(sc->sih_rd);
		sc->sih_rd = NULL;
	}
	if (sc->sih_wr != NULL) {
		softintr_disestablish(sc->sih_wr);
		sc->sih_wr = NULL;
	}

	return (0);
}

void
midi_attach(struct midi_softc *sc, struct device *parent)
{
	struct midi_info mi;
	int s;

	callout_init(&sc->xmt_asense_co);
	callout_init(&sc->rcv_asense_co);
	callout_setfunc(&sc->xmt_asense_co, midi_xmt_asense, sc);
	callout_setfunc(&sc->rcv_asense_co, midi_rcv_asense, sc);
	simple_lock_init(&sc->out_lock);
	simple_lock_init(&sc->in_lock);
	sc->dying = 0;
	sc->isopen = 0;

	sc->sc_dev = parent;

	sc->sih_rd = softintr_establish(IPL_SOFTSERIAL, midi_softintr_rd, sc);
	sc->sih_wr = softintr_establish(IPL_SOFTSERIAL, midi_softintr_wr, sc);

	s = splaudio();
	simple_lock(&hwif_register_lock);
	hwif_softc = sc;
	sc->hw_if->getinfo(sc->hw_hdl, &mi);
	hwif_softc = NULL;
	simple_unlock(&hwif_register_lock);
	splx(s);
	
	sc->props = mi.props;
	
	if ( !(sc->props & MIDI_PROP_NO_OUTPUT) ) {
		evcnt_attach_dynamic(&sc->xmt.bytesDiscarded,
			EVCNT_TYPE_MISC, NULL,
			sc->dev.dv_xname, "xmt bytes discarded");
		evcnt_attach_dynamic(&sc->xmt.incompleteMessages,
			EVCNT_TYPE_MISC, NULL,
			sc->dev.dv_xname, "xmt incomplete msgs");
	}
	if ( sc->props & MIDI_PROP_CAN_INPUT ) {
		evcnt_attach_dynamic(&sc->rcv.bytesDiscarded,
			EVCNT_TYPE_MISC, NULL,
			sc->dev.dv_xname, "rcv bytes discarded");
		evcnt_attach_dynamic(&sc->rcv.incompleteMessages,
			EVCNT_TYPE_MISC, NULL,
			sc->dev.dv_xname, "rcv incomplete msgs");
	}
	
	printf(": %s%s\n", mi.name,
	    (sc->props & (MIDI_PROP_OUT_INTR|MIDI_PROP_NO_OUTPUT)) ?
	    "" : " (CPU-intensive output)");
}

void midi_register_hw_if_ext(struct midi_hw_if_ext *exthw) {
	if ( hwif_softc != NULL ) /* ignore calls resulting from non-init */
		hwif_softc->hw_if_ext = exthw; /* uses of getinfo */
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
	mb->idx_producerp = mb->idx_consumerp = mb->idx;
	mb->buf_producerp = mb->buf_consumerp = mb->buf;
}
#define PACK_MB_IDX(cat,len) (((cat)<<4)|(len))
#define MB_IDX_CAT(idx) ((idx)>>4)
#define MB_IDX_LEN(idx) ((idx)&0xf)

int
midi_sleep_timo(int *chan, const char *label, int timo, struct simplelock *lk)
{
	int st;

	if (!label)
		label = "midi";

	DPRINTFN(8, ("midi_sleep_timo: %p %s %d\n", chan, label, timo));
	*chan = 1;
	st = ltsleep(chan, PWAIT | PCATCH, label, timo, lk);
	*chan = 0;
#ifdef MIDI_DEBUG
	if (st != 0)
		printf("midi_sleep: %d\n", st);
#endif
	return st;
}

int
midi_sleep(int *chan, const char *label, struct simplelock *lk)
{
	return midi_sleep_timo(chan, label, 0, lk);
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
#define FST_RETURN(offp,endp,ret) \
	return (s->pos=s->msg+(offp)), (s->end=s->msg+(endp)), (ret)

enum fst_ret { FST_CHN, FST_CHV, FST_COM, FST_SYX, FST_RT, FST_MORE, FST_ERR,
               FST_HUH, FST_SXP };
enum fst_form { FST_CANON, FST_COMPR, FST_VCOMP };
static struct {
	int off;
	enum fst_ret tag;
} const midi_forms[] = {
	[FST_CANON] = { .off=0, .tag=FST_CHN },
	[FST_COMPR] = { .off=1, .tag=FST_CHN },
	[FST_VCOMP] = { .off=0, .tag=FST_CHV }
};
#define FST_CRETURN(endp) \
	FST_RETURN(midi_forms[form].off,endp,midi_forms[form].tag)

/*
 * A MIDI finite state transducer suitable for receiving or transmitting. It
 * will accept correct MIDI input that uses, doesn't use, or sometimes uses the
 * 'running status' compression technique, and transduce it to fully expanded
 * (form=FST_CANON) or fully compressed (form=FST_COMPR or FST_VCOMP) form.
 *
 * Returns FST_MORE if a complete message has not been parsed yet (SysEx
 * messages are the exception), FST_ERR or FST_HUH if the input does not
 * conform to the protocol, or FST_CHN (channel messages), FST_COM (System
 * Common messages), FST_RT (System Real-Time messages), or FST_SYX (System
 * Exclusive) to broadly categorize the message parsed. s->pos and s->end
 * locate the parsed message; while (s->pos<s->end) putchar(*(s->pos++));
 * would output it.
 *
 * FST_HUH means the character c wasn't valid in the original state, but the
 * state has now been reset to START and the caller should try again passing
 * the same c. FST_ERR means c isn't valid in the start state; the caller
 * should kiss it goodbye and continue to try successive characters from the
 * input until something other than FST_ERR or FST_HUH is returned, at which
 * point things are resynchronized.
 *
 * A FST_SYX return means that between pos and end are from 1 to 3
 * bytes of a system exclusive message. A SysEx message will be delivered in
 * one or more chunks of that form, where the first begins with 0xf0 and the
 * last (which is the only one that might have length < 3) ends with 0xf7.
 *
 * Messages corrupted by a protocol error are discarded and won't be seen at
 * all; again SysEx is the exception, as one or more chunks of it may already
 * have been parsed.
 *
 * For FST_CHN messages, s->msg[0] always contains the status byte even if
 * FST_COMPR form was requested (pos then points to msg[1]). That way, the
 * caller can always identify the exact message if there is a need to do so.
 * For all other message types except FST_SYX, the status byte is at *pos
 * (which may not necessarily be msg[0]!). There is only one SysEx status
 * byte, so the return value FST_SYX is sufficient to identify it.
 *
 * To simplify some use cases, compression can also be requested with
 * form=FST_VCOMP. In this form a compressible channel message is indicated
 * by returning a classification of FST_CHV instead of FST_CHN, and pos points
 * to the status byte rather than being advanced past it. If the caller in this
 * case saves the bytes from pos to end, it will have saved the entire message,
 * and can act on the FST_CHV tag to drop the first byte later. In this form,
 * unlike FST_CANON, hidden note-off (i.e. note-on with velocity 0) may occur.
 *
 * Two obscure points in the MIDI protocol complicate things further, both to
 * do with the EndSysEx code, 0xf7. First, this code is permitted (and
 * meaningless) outside of a System Exclusive message, anywhere a status byte
 * could appear. Second, it is allowed to be absent at the end of a System
 * Exclusive message (!) - any status byte at all (non-realtime) is allowed to
 * terminate the message. Both require accomodation in the interface to
 * midi_fst's caller. A stray 0xf7 should be ignored BUT should count as a
 * message received for purposes of Active Sense timeout; the case is
 * represented by a return of FST_COM with a length of zero (pos == end). A
 * status byte other than 0xf7 during a system exclusive message will cause an
 * FST_SXP (sysex plus) return; the bytes from pos to end are the end of the
 * system exclusive message, and after handling those the caller should call
 * midi_fst again with the same input byte.
 *
 * midi(4) will never produce either such form of rubbish.
 */
static enum fst_ret
midi_fst(struct midi_state *s, u_char c, enum fst_form form)
{
	int syxpos = 0;

	if ( c >= 0xf8 ) { /* All realtime messages bypass state machine */
	        if ( c == 0xf9  ||  c == 0xfd ) {
			DPRINTF( ("midi_fst: s=%p c=0x%02x undefined\n", 
				  s, c));
			s->bytesDiscarded.ev_count++;
			return FST_ERR;
		}
		DPRINTFN(9, ("midi_fst: s=%p System Real-Time data=0x%02x\n", 
			     s, c));
		s->msg[2] = c;
		FST_RETURN(2,3,FST_RT);
	}

	DPRINTFN(4, ("midi_fst: s=%p data=0x%02x state=%d\n", 
		     s, c, s->state));

        switch ( s->state   | MIDI_CAT(c) ) { /* break ==> return FST_MORE */

	case MIDI_IN_START  | MIDI_CAT_COMMON:
	case MIDI_IN_RUN1_1 | MIDI_CAT_COMMON:
	case MIDI_IN_RUN2_2 | MIDI_CAT_COMMON:
	case MIDI_IN_RXX2_2 | MIDI_CAT_COMMON:
	        s->msg[0] = c;
	        switch ( c ) {
		case 0xf0: s->state = MIDI_IN_SYX1_3; break;
		case 0xf1: s->state = MIDI_IN_COM0_1; break;
		case 0xf2: s->state = MIDI_IN_COM0_2; break;
		case 0xf3: s->state = MIDI_IN_COM0_1; break;
		case 0xf6: s->state = MIDI_IN_START;  FST_RETURN(0,1,FST_COM);
		case 0xf7: s->state = MIDI_IN_START;  FST_RETURN(0,0,FST_COM);
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
	case MIDI_IN_RXX2_2 | MIDI_CAT_STATUS1:
	case MIDI_IN_START  | MIDI_CAT_STATUS1:
	        s->state = MIDI_IN_RUN0_1;
	        s->msg[0] = c;
		break;
	
	case MIDI_IN_RUN2_2 | MIDI_CAT_STATUS2:
	case MIDI_IN_RXX2_2 | MIDI_CAT_STATUS2:
		if ( c == s->msg[0] ) {
			s->state = MIDI_IN_RNX0_2;
			break;
		}
		if ( (c ^ s->msg[0]) == 0x10 && (c & 0xe0) == 0x80 ) {
			s->state = MIDI_IN_RXX0_2;
			s->msg[0] = c;
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
		FST_RETURN(0,2,FST_COM);

        case MIDI_IN_COM0_2 | MIDI_CAT_DATA:
	        s->state = MIDI_IN_COM1_2;
	        s->msg[1] = c;
		break;

        case MIDI_IN_COM1_2 | MIDI_CAT_DATA:
		s->state = MIDI_IN_START;
	        s->msg[2] = c;
		FST_RETURN(0,3,FST_COM);

        case MIDI_IN_RUN0_1 | MIDI_CAT_DATA:
		s->state = MIDI_IN_RUN1_1;
	        s->msg[1] = c;
		FST_RETURN(0,2,FST_CHN);

        case MIDI_IN_RUN1_1 | MIDI_CAT_DATA:
        case MIDI_IN_RNX0_1 | MIDI_CAT_DATA:
		s->state = MIDI_IN_RUN1_1;
	        s->msg[1] = c;
		FST_CRETURN(2);

        case MIDI_IN_RUN0_2 | MIDI_CAT_DATA:
	        s->state = MIDI_IN_RUN1_2;
	        s->msg[1] = c;
		break;

        case MIDI_IN_RUN1_2 | MIDI_CAT_DATA:
		if ( FST_CANON == form && 0 == c && (s->msg[0]&0xf0) == 0x90 ) {
			s->state = MIDI_IN_RXX2_2;
			s->msg[0] ^= 0x10;
			s->msg[2] = 64;
		} else {
			s->state = MIDI_IN_RUN2_2;
	        	s->msg[2] = c;
		}
		FST_RETURN(0,3,FST_CHN);

        case MIDI_IN_RUN2_2 | MIDI_CAT_DATA:
	        s->state = MIDI_IN_RNX1_2;
	        s->msg[1] = c;
		break;

        case MIDI_IN_RXX2_2 | MIDI_CAT_DATA:
	        s->state = MIDI_IN_RXX1_2;
		s->msg[0] ^= 0x10;
	        s->msg[1] = c;
		break;

        case MIDI_IN_RNX0_2 | MIDI_CAT_DATA:
	        s->state = MIDI_IN_RNY1_2;
	        s->msg[1] = c;
		break;

        case MIDI_IN_RXX0_2 | MIDI_CAT_DATA:
	        s->state = MIDI_IN_RXY1_2;
	        s->msg[1] = c;
		break;

        case MIDI_IN_RNX1_2 | MIDI_CAT_DATA:
        case MIDI_IN_RNY1_2 | MIDI_CAT_DATA:
		if ( FST_CANON == form && 0 == c && (s->msg[0]&0xf0) == 0x90 ) {
			s->state = MIDI_IN_RXX2_2;
			s->msg[0] ^= 0x10;
			s->msg[2] = 64;
			FST_RETURN(0,3,FST_CHN);
		}
		s->state = MIDI_IN_RUN2_2;
	        s->msg[2] = c;
		FST_CRETURN(3);

        case MIDI_IN_RXX1_2 | MIDI_CAT_DATA:
        case MIDI_IN_RXY1_2 | MIDI_CAT_DATA:
		if ( ( 0 == c && (s->msg[0]&0xf0) == 0x90)
		  || (64 == c && (s->msg[0]&0xf0) == 0x80
		      && FST_CANON != form) ) {
			s->state = MIDI_IN_RXX2_2;
			s->msg[0] ^= 0x10;
			s->msg[2] = 64 - c;
			FST_CRETURN(3);
		}
		s->state = MIDI_IN_RUN2_2;
	        s->msg[2] = c;
		FST_RETURN(0,3,FST_CHN);

        case MIDI_IN_SYX1_3 | MIDI_CAT_DATA:
		s->state = MIDI_IN_SYX2_3;
	        s->msg[1] = c;
		break;

        case MIDI_IN_SYX2_3 | MIDI_CAT_DATA:
		s->state = MIDI_IN_SYX0_3;
	        s->msg[2] = c;
		FST_RETURN(0,3,FST_SYX);

        case MIDI_IN_SYX0_3 | MIDI_CAT_DATA:
		s->state = MIDI_IN_SYX1_3;
	        s->msg[0] = c;
		break;

        case MIDI_IN_SYX2_3 | MIDI_CAT_COMMON:
        case MIDI_IN_SYX2_3 | MIDI_CAT_STATUS1:
        case MIDI_IN_SYX2_3 | MIDI_CAT_STATUS2:
		++ syxpos;
		/* FALLTHROUGH */
        case MIDI_IN_SYX1_3 | MIDI_CAT_COMMON:
        case MIDI_IN_SYX1_3 | MIDI_CAT_STATUS1:
        case MIDI_IN_SYX1_3 | MIDI_CAT_STATUS2:
		++ syxpos;
		/* FALLTHROUGH */
        case MIDI_IN_SYX0_3 | MIDI_CAT_COMMON:
        case MIDI_IN_SYX0_3 | MIDI_CAT_STATUS1:
        case MIDI_IN_SYX0_3 | MIDI_CAT_STATUS2:
		s->state = MIDI_IN_START;
	        if ( c == 0xf7 ) {
			s->msg[syxpos] = c;
		        FST_RETURN(0,1+syxpos,FST_SYX);
		}
		s->msg[syxpos] = 0xf7;
		FST_RETURN(0,1+syxpos,FST_SXP);

        default:
protocol_violation:
                DPRINTF(("midi_fst: unexpected %#02x in state %u\n",
		        c, s->state));
		switch ( s->state ) {
		case MIDI_IN_RUN1_1: /* can only get here by seeing an */
		case MIDI_IN_RUN2_2: /* INVALID System Common message */
		case MIDI_IN_RXX2_2:
		        s->state = MIDI_IN_START;
			/* FALLTHROUGH */
		case MIDI_IN_START:
			s->bytesDiscarded.ev_count++;
			return FST_ERR;
		case MIDI_IN_COM1_2:
		case MIDI_IN_RUN1_2:
		case MIDI_IN_RNY1_2:
		case MIDI_IN_RXY1_2:
			s->bytesDiscarded.ev_count++;
			/* FALLTHROUGH */
		case MIDI_IN_COM0_1:
		case MIDI_IN_RUN0_1:
		case MIDI_IN_RNX0_1:
		case MIDI_IN_COM0_2:
		case MIDI_IN_RUN0_2:
		case MIDI_IN_RNX0_2:
		case MIDI_IN_RXX0_2:
		case MIDI_IN_RNX1_2:
		case MIDI_IN_RXX1_2:
			s->bytesDiscarded.ev_count++;
		        s->incompleteMessages.ev_count++;
			break;
#if defined(AUDIO_DEBUG) || defined(DIAGNOSTIC)
		default:
		        printf("midi_fst: mishandled %#02x(%u) in state %u?!\n",
			      c, MIDI_CAT(c), s->state);
#endif
		}
		s->state = MIDI_IN_START;
		return FST_HUH;
	}
	return FST_MORE;
}

void
midi_softintr_rd(void *cookie)
{
	struct midi_softc *sc = cookie;
	struct proc *p;

	if (sc->async != NULL) {
		mutex_enter(&proclist_mutex);
		if ((p = sc->async) != NULL)
			psignal(p, SIGIO);
		mutex_exit(&proclist_mutex);
	}
	midi_wakeup(&sc->rchan);
	selnotify(&sc->rsel, 0); /* filter will spin if locked */
}

void
midi_softintr_wr(void *cookie)
{
	struct midi_softc *sc = cookie;
	struct proc *p;

	if (sc->async != NULL) {
		mutex_enter(&proclist_mutex);
		if ((p = sc->async) != NULL)
			psignal(p, SIGIO);
		mutex_exit(&proclist_mutex);
	}
	midi_wakeup(&sc->wchan);
	selnotify(&sc->wsel, 0); /* filter will spin if locked */
}

void
midi_in(void *addr, int data)
{
	struct midi_softc *sc = addr;
	struct midi_buffer *mb = &sc->inbuf;
	int i;
	int count;
	enum fst_ret got;
	int s; /* hw may have various spls so impose our own */
	MIDI_BUF_DECLARE(idx);
	MIDI_BUF_DECLARE(buf);

	if (!sc->isopen)
		return;

	if (!(sc->flags & FREAD))
		return;		/* discard data if not reading */
	
sxp_again:
	do
		got = midi_fst(&sc->rcv, data, FST_CANON);
	while ( got == FST_HUH );
	
	switch ( got ) {
	case FST_MORE:
	case FST_ERR:
		return;
	case FST_CHN:
	case FST_COM:
	case FST_RT:
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
		if ( got == FST_RT && MIDI_ACK == sc->rcv.pos[0] ) {
			if ( !sc->rcv_expect_asense ) {
				sc->rcv_expect_asense = 1;
				callout_schedule(&sc->rcv_asense_co,
				                 MIDI_RCV_ASENSE_PERIOD);
			}
			sc->rcv_quiescent = 0;
			sc->rcv_eof = 0;
			return;
		}
		/* FALLTHROUGH */
	/*
	 * Ultimately SysEx msgs should be offered to the sequencer also; the
	 * sequencer API addresses them - but maybe our sequencer can't handle
	 * them yet, so offer only to raw reader. (Which means, ultimately,
	 * discard them if the sequencer's open, as it's not doing reads!)
	 * -> When SysEx support is added to the sequencer, be sure to handle
	 *    FST_SXP there too.
	 */
	case FST_SYX:
	case FST_SXP:
		count = sc->rcv.end - sc->rcv.pos;
		MIDI_IN_LOCK(sc,s);
		sc->rcv_quiescent = 0;
		sc->rcv_eof = 0;
		if ( 0 == count ) {
			MIDI_IN_UNLOCK(sc,s);
			break;
		}
		MIDI_BUF_PRODUCER_INIT(mb,idx);
		MIDI_BUF_PRODUCER_INIT(mb,buf);
		if (count > buf_lim - buf_cur
		     || 1 > idx_lim - idx_cur) {
			sc->rcv.bytesDiscarded.ev_count += count;
			MIDI_IN_UNLOCK(sc,s);
			DPRINTF(("midi_in: buffer full, discard data=0x%02x\n", 
				 sc->rcv.pos[0]));
			return;
		}
		for (i = 0; i < count; i++) {
			*buf_cur++ = sc->rcv.pos[i];
			MIDI_BUF_WRAP(buf);
		}
		*idx_cur++ = PACK_MB_IDX(got,count);
		MIDI_BUF_WRAP(idx);
		MIDI_BUF_PRODUCER_WBACK(mb,buf);
		MIDI_BUF_PRODUCER_WBACK(mb,idx);
		MIDI_IN_UNLOCK(sc,s);
		softintr_schedule(sc->sih_rd);
		break;
	default: /* don't #ifdef this away, gcc will say FST_HUH not handled */
		printf("midi_in: midi_fst returned %d?!\n", got);
	}
	if ( FST_SXP == got )
		goto sxp_again;
}

void
midi_out(void *addr)
{
	struct midi_softc *sc = addr;

	if (!sc->isopen)
		return;
	DPRINTFN(8, ("midi_out: %p\n", sc));
	midi_intr_out(sc);
}

int
midiopen(dev_t dev, int flags, int ifmt, struct lwp *l)
{
	struct midi_softc *sc;
	const struct midi_hw_if *hw;
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

	/* put both state machines into known states */
	sc->rcv.state = MIDI_IN_START;
	sc->rcv.pos = sc->rcv.msg;
	sc->rcv.end = sc->rcv.msg;
	sc->xmt.state = MIDI_IN_START;
	sc->xmt.pos = sc->xmt.msg;
	sc->xmt.end = sc->xmt.msg;
	
	/* copy error counters so an ioctl (TBA) can give since-open stats */
	sc->rcv.atOpen.bytesDiscarded  = sc->rcv.bytesDiscarded.ev_count;
	sc->rcv.atQuery.bytesDiscarded = sc->rcv.bytesDiscarded.ev_count;
	
	sc->xmt.atOpen.bytesDiscarded  = sc->xmt.bytesDiscarded.ev_count;
	sc->xmt.atQuery.bytesDiscarded = sc->xmt.bytesDiscarded.ev_count;
	
	/* and the buffers */
	midi_initbuf(&sc->outbuf);
	midi_initbuf(&sc->inbuf);
	
	/* and the receive flags */
	sc->rcv_expect_asense = 0;
	sc->rcv_quiescent = 0;
	sc->rcv_eof = 0;

	error = hw->open(sc->hw_hdl, flags, midi_in, midi_out, sc);
	if (error)
		return error;
	sc->isopen++;
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
midiclose(dev_t dev, int flags, int ifmt,
    struct lwp *l)
{
	int unit = MIDIUNIT(dev);
	struct midi_softc *sc = midi_cd.cd_devs[unit];
	const struct midi_hw_if *hw = sc->hw_if;
	int s, error;

	DPRINTFN(3,("midiclose %p\n", sc));

	/* midi_start_output(sc); anything buffered => pbus already set! */
	error = 0;
	MIDI_OUT_LOCK(sc,s);
	while (sc->pbus) {
		DPRINTFN(8,("midiclose sleep ...\n"));
		error =
		midi_sleep_timo(&sc->wchan, "mid_dr", 30*hz, &sc->out_lock);
	}
	sc->isopen = 0;
	MIDI_OUT_UNLOCK(sc,s);
	callout_stop(&sc->xmt_asense_co); /* xxx fix this - sleep? */
	callout_stop(&sc->rcv_asense_co);
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
	int s;
	MIDI_BUF_DECLARE(idx);
	MIDI_BUF_DECLARE(buf);
	int appetite;
	int first = 1;

	DPRINTFN(6,("midiread: %p, count=%lu\n", sc,
		 (unsigned long)uio->uio_resid));

	if (sc->dying)
		return EIO;
        if ( !(sc->props & MIDI_PROP_CAN_INPUT) )
	        return ENXIO;

	MIDI_IN_LOCK(sc,s);
	MIDI_BUF_CONSUMER_INIT(mb,idx);
	MIDI_BUF_CONSUMER_INIT(mb,buf);
	MIDI_IN_UNLOCK(sc,s);
	
	error = 0;
	for ( ;; ) {
		/*
		 * If the used portion of idx wraps around the end, just take
		 * the first part on this iteration, and we'll get the rest on
		 * the next.
		 */
		if ( idx_lim > idx_end )
			idx_lim = idx_end;
		/*
		 * Count bytes through the last complete message that will
		 * fit in the requested read.
		 */
		for (appetite = uio->uio_resid; idx_cur < idx_lim; ++idx_cur) {
			if ( appetite < MB_IDX_LEN(*idx_cur) )
				break;
			appetite -= MB_IDX_LEN(*idx_cur);
		}
		appetite = uio->uio_resid - appetite;
		/*
		 * Only if the read is too small to hold even the first
		 * complete message will we return a partial one (updating idx
		 * to reflect the remaining length of the message).
		 */
		if ( appetite == 0 && idx_cur < idx_lim ) {
			if ( !first )
				goto unlocked_exit; /* idx_cur not advanced */
			appetite = uio->uio_resid;
			*idx_cur = PACK_MB_IDX(MB_IDX_CAT(*idx_cur),
					       MB_IDX_LEN(*idx_cur) - appetite);
		}
		KASSERT(buf_cur + appetite <= buf_lim);
		
		/* move the bytes */
		if ( appetite > 0 ) {		
			first = 0;  /* we know we won't return empty-handed */
			/* do two uiomoves if data wrap around end of buf */
			if ( buf_cur + appetite > buf_end ) {
				DPRINTFN(8,
					("midiread: uiomove cc=%td (prewrap)\n",
					buf_end - buf_cur));
				error = uiomove(buf_cur, buf_end-buf_cur, uio);
				if ( error )
					goto unlocked_exit;
				appetite -= buf_end - buf_cur;
				buf_cur = mb->buf;
			}
			DPRINTFN(8, ("midiread: uiomove cc=%d\n", appetite));
			error = uiomove(buf_cur, appetite, uio);
			if ( error )
				goto unlocked_exit;
			buf_cur += appetite;
		}
		
		MIDI_BUF_WRAP(idx);
		MIDI_BUF_WRAP(buf);

		MIDI_IN_LOCK(sc,s);
		MIDI_BUF_CONSUMER_WBACK(mb,idx);
		MIDI_BUF_CONSUMER_WBACK(mb,buf);
		if ( 0 == uio->uio_resid ) /* if read satisfied, we're done */
			break;
		MIDI_BUF_CONSUMER_REFRESH(mb,idx);
		if ( idx_cur == idx_lim ) { /* need to wait for data? */
			if ( !first || sc->rcv_eof ) /* never block reader if */
				break;            /* any data already in hand */
			if (ioflag & IO_NDELAY) {
				error = EWOULDBLOCK;
				break;
			}
			error = midi_sleep(&sc->rchan, "mid rd", &sc->in_lock);
			if ( error )
				break;
			MIDI_BUF_CONSUMER_REFRESH(mb,idx); /* what'd we get? */
		}
		MIDI_BUF_CONSUMER_REFRESH(mb,buf);
		MIDI_IN_UNLOCK(sc,s);
		if ( sc->dying )
			return EIO;
	}
	MIDI_IN_UNLOCK(sc,s);

unlocked_exit:
	return error;
}

void
midi_rcv_asense(void *arg)
{
	struct midi_softc *sc = arg;
	int s;
	
	if ( sc->dying || !sc->isopen )
		return;

	if ( sc->rcv_quiescent ) {
		MIDI_IN_LOCK(sc,s);
		sc->rcv_eof = 1;
		sc->rcv_quiescent = 0;
		sc->rcv_expect_asense = 0;
		MIDI_IN_UNLOCK(sc,s);
		softintr_schedule(sc->sih_rd);
		return;
	}
	
	sc->rcv_quiescent = 1;
	callout_schedule(&sc->rcv_asense_co, MIDI_RCV_ASENSE_PERIOD);
}

void
midi_xmt_asense(void *arg)
{
	struct midi_softc *sc = arg;
	int s;
	int error;
	int armed;
	
	if ( sc->dying || !sc->isopen )
		return;

	MIDI_OUT_LOCK(sc,s);
	if ( sc->pbus || sc->dying || !sc->isopen ) {
		MIDI_OUT_UNLOCK(sc,s);
		return;
	}
	sc->pbus = 1;
	DPRINTFN(8,("midi_xmt_asense: %p\n", sc));

	if ( sc->props & MIDI_PROP_OUT_INTR ) {
		error = sc->hw_if->output(sc->hw_hdl, MIDI_ACK);
		armed = (error == 0);
	} else { /* polled output, do with interrupts unmasked */
		MIDI_OUT_UNLOCK(sc,s);
		/* running from softclock, so top half won't sneak in here */
		error = sc->hw_if->output(sc->hw_hdl, MIDI_ACK);
		MIDI_OUT_LOCK(sc,s);
		armed = 0;
	}

	if ( !armed ) {
		sc->pbus = 0;
		callout_schedule(&sc->xmt_asense_co, MIDI_XMT_ASENSE_PERIOD);
	}

	MIDI_OUT_UNLOCK(sc,s);
}

/*
 * The way this function was hacked up to plug into poll_out and intr_out
 * after they were written won't win it any beauty contests, but it'll work
 * (code in haste, refactor at leisure). This may be called with the lock
 * (by intr_out) or without the lock (by poll_out) so it only does what could
 * be safe either way.
 */
int midi_msg_out(struct midi_softc *sc,
                 u_char **idx, u_char **idxl, u_char **buf, u_char **bufl) {
	MIDI_BUF_DECLARE(idx);
	MIDI_BUF_DECLARE(buf);
	MIDI_BUF_EXTENT_INIT(&sc->outbuf,idx);
	MIDI_BUF_EXTENT_INIT(&sc->outbuf,buf);
	int length;
	int error;
	u_char contig[3];
	u_char *cp;
	u_char *ep;
	
	idx_cur = *idx;
	idx_lim = *idxl;
	buf_cur = *buf;
	buf_lim = *bufl;
	
	length = MB_IDX_LEN(*idx_cur);
	
	for ( cp = contig, ep = cp + length; cp < ep; ) {
		*cp++ = *buf_cur++;
		MIDI_BUF_WRAP(buf);
	}
	cp = contig;

	switch ( MB_IDX_CAT(*idx_cur) ) {
	case FST_CHV: /* chnmsg to be compressed (for device that wants it) */
		++ cp;
		-- length;
		/* FALLTHROUGH */
	case FST_CHN:
		error = sc->hw_if_ext->channel(sc->hw_hdl,
		                               MIDI_GET_STATUS(contig[0]),
					       MIDI_GET_CHAN(contig[0]),
					       cp, length);
		break;
	case FST_COM:
		error = sc->hw_if_ext->common(sc->hw_hdl,
		                              MIDI_GET_STATUS(contig[0]),
					      cp, length);
		break;
	case FST_SYX:
	case FST_SXP:
		error = sc->hw_if_ext->sysex(sc->hw_hdl,
					     cp, length);
		break;
	case FST_RT:
		error = sc->hw_if->output(sc->hw_hdl, *cp);
		break;
	default:
		error = EIO;
	}
	
	if ( !error ) {
		++ idx_cur;
		MIDI_BUF_WRAP(idx);
		*idx  = idx_cur;
		*idxl = idx_lim;
		*buf  = buf_cur;
		*bufl = buf_lim;
	}
	
	return error;
}

/*
 * midi_poll_out is intended for the midi hw (the vast majority of MIDI UARTs
 * on sound cards, apparently) that _do not have transmit-ready interrupts_.
 * Every call to hw_if->output for one of these may busy-wait to output the
 * byte; at the standard midi data rate that'll be 320us per byte. The
 * technique of writing only MIDI_MAX_WRITE bytes in a row and then waiting
 * for MIDI_WAIT does not reduce the total time spent busy-waiting, and it
 * adds arbitrary delays in transmission (and, since MIDI_WAIT is roughly the
 * same as the time to send MIDI_MAX_WRITE bytes, it effectively halves the
 * data rate). Here, a somewhat bolder approach is taken. Since midi traffic
 * is bursty but time-sensitive--most of the time there will be none at all,
 * but when there is it should go out ASAP--the strategy is to just get it
 * over with, and empty the buffer in one go. The effect this can have on
 * the rest of the system will be limited by the size of the buffer and the
 * sparseness of the traffic. But some precautions are in order. Interrupts
 * should all be unmasked when this is called, and midiwrite should not fill
 * the buffer more than once (when MIDI_PROP_CAN_INTR is false) without a
 * yield() so some other process can get scheduled. If the write is nonblocking,
 * midiwrite should return a short count rather than yield.
 *
 * Someday when there is fine-grained MP support, this should be reworked to
 * run in a callout so the writing process really could proceed concurrently.
 * But obviously where performance is a concern, interrupt-driven hardware
 * such as USB midi or (apparently) clcs will always be preferable. And it
 * seems (kern/32651) that many of the devices currently working in poll mode
 * may really have tx interrupt capability and want only implementation; that
 * ought to happen.
 */
int
midi_poll_out(struct midi_softc *sc)
{
	struct midi_buffer *mb = &sc->outbuf;
	int error;
	int msglen;
	int s;
	MIDI_BUF_DECLARE(idx);
	MIDI_BUF_DECLARE(buf);

	error = 0;

	MIDI_OUT_LOCK(sc,s);
	MIDI_BUF_CONSUMER_INIT(mb,idx);
	MIDI_BUF_CONSUMER_INIT(mb,buf);
	MIDI_OUT_UNLOCK(sc,s);

	for ( ;; ) {
		while ( idx_cur != idx_lim ) {
			if ( sc->hw_if_ext ) {
				error = midi_msg_out(sc, &idx_cur, &idx_lim,
				                         &buf_cur, &buf_lim);
				if ( error )
					goto ioerror;
				continue;
			}
			/* or, lacking hw_if_ext ... */
			msglen = MB_IDX_LEN(*idx_cur);
			DPRINTFN(7,("midi_poll_out: %p <- %#02x\n",
				   sc->hw_hdl, *buf_cur));
			error = sc->hw_if->output(sc->hw_hdl, *buf_cur);
			if ( error )
				goto ioerror;
			++ buf_cur;
			MIDI_BUF_WRAP(buf);
			-- msglen;
			if ( msglen )
				*idx_cur = PACK_MB_IDX(MB_IDX_CAT(*idx_cur),
				                       msglen);
			else {
				++ idx_cur;
				MIDI_BUF_WRAP(idx);
			}
		}
		KASSERT(buf_cur == buf_lim);
		MIDI_OUT_LOCK(sc,s);
		MIDI_BUF_CONSUMER_WBACK(mb,idx);
		MIDI_BUF_CONSUMER_WBACK(mb,buf);
		MIDI_BUF_CONSUMER_REFRESH(mb,idx); /* any more to transmit? */
		MIDI_BUF_CONSUMER_REFRESH(mb,buf);
		if ( idx_lim == idx_cur )
			break; /* still holding lock */
		MIDI_OUT_UNLOCK(sc,s);
	}
	goto disarm; /* lock held */

ioerror:
#if defined(AUDIO_DEBUG) || defined(DIAGNOSTIC)
	printf("%s: midi_poll_output error %d\n",
	      sc->dev.dv_xname, error);
#endif
	MIDI_OUT_LOCK(sc,s);
	MIDI_BUF_CONSUMER_WBACK(mb,idx);
	MIDI_BUF_CONSUMER_WBACK(mb,buf);

disarm:	
	sc->pbus = 0;
	callout_schedule(&sc->xmt_asense_co, MIDI_XMT_ASENSE_PERIOD);
	MIDI_OUT_UNLOCK(sc,s);
	return error;
}

/*
 * The interrupt flavor acquires spl and lock once and releases at the end,
 * as it expects to write only one byte or message. The interface convention
 * is that if hw_if->output returns 0, it has initiated transmission and the
 * completion interrupt WILL be forthcoming; if it has not returned 0, NO
 * interrupt will be forthcoming, and if it returns EINPROGRESS it wants
 * another byte right away.
 */
int
midi_intr_out(struct midi_softc *sc)
{
	struct midi_buffer *mb = &sc->outbuf;
	int error;
	int msglen;
	int s;
	MIDI_BUF_DECLARE(idx);
	MIDI_BUF_DECLARE(buf);
	int armed = 0;

	error = 0;

	MIDI_OUT_LOCK(sc,s);
	MIDI_BUF_CONSUMER_INIT(mb,idx);
	MIDI_BUF_CONSUMER_INIT(mb,buf);
	
	while ( idx_cur != idx_lim ) {
		if ( sc->hw_if_ext ) {
			error = midi_msg_out(sc, &idx_cur, &idx_lim,
				                 &buf_cur, &buf_lim);
			if ( !error ) /* no EINPROGRESS from extended hw_if */
				armed = 1;
			break;
		}
		/* or, lacking hw_if_ext ... */
		msglen = MB_IDX_LEN(*idx_cur);
		error = sc->hw_if->output(sc->hw_hdl, *buf_cur);
		if ( error  &&  error != EINPROGRESS )
			break;
		++ buf_cur;
		MIDI_BUF_WRAP(buf);
		-- msglen;
		if ( msglen )
			*idx_cur = PACK_MB_IDX(MB_IDX_CAT(*idx_cur),msglen);
		else {
			++ idx_cur;
			MIDI_BUF_WRAP(idx);
		}
		if ( !error ) {
			armed = 1;
			break;
		}
	}
	MIDI_BUF_CONSUMER_WBACK(mb,idx);
	MIDI_BUF_CONSUMER_WBACK(mb,buf);
	if ( !armed ) {
		sc->pbus = 0;
		callout_schedule(&sc->xmt_asense_co, MIDI_XMT_ASENSE_PERIOD);
	}
	MIDI_OUT_UNLOCK(sc,s);
	softintr_schedule(sc->sih_wr);

#if defined(AUDIO_DEBUG) || defined(DIAGNOSTIC)
	if ( error )
		printf("%s: midi_intr_output error %d\n",
	               sc->dev.dv_xname, error);
#endif
	return error;
}

int
midi_start_output(struct midi_softc *sc)
{
	if (sc->dying)
		return EIO;

	if ( sc->props & MIDI_PROP_OUT_INTR )
		return midi_intr_out(sc);
	return midi_poll_out(sc);
}

static int
real_writebytes(struct midi_softc *sc, u_char *ibuf, int cc)
{
	u_char *iend = ibuf + cc;
	struct midi_buffer *mb = &sc->outbuf;
	int arming = 0;
	int count;
	int s;
	int got;
	enum fst_form form;
	MIDI_BUF_DECLARE(idx);
	MIDI_BUF_DECLARE(buf);
	
	/*
	 * If the hardware uses the extended hw_if, pass it canonicalized
	 * messages (or compressed ones if it specifically requests, using
	 * VCOMP form so the bottom half can still pass the op and chan along);
	 * if it does not, send it compressed messages (using COMPR form as
	 * there is no need to preserve the status for the bottom half).
	 */
	if ( NULL == sc->hw_if_ext )
		form = FST_COMPR;
	else if ( sc->hw_if_ext->compress )
		form = FST_VCOMP;
	else
		form = FST_CANON;

	MIDI_OUT_LOCK(sc,s);
	MIDI_BUF_PRODUCER_INIT(mb,idx);
	MIDI_BUF_PRODUCER_INIT(mb,buf);
	MIDI_OUT_UNLOCK(sc,s);

	if (sc->dying)
		return EIO;
	
	while ( ibuf < iend ) {
		got = midi_fst(&sc->xmt, *ibuf, form);
		++ ibuf;
		switch ( got ) {
		case FST_MORE:
			continue;
		case FST_ERR:
		case FST_HUH:
			return EPROTO;
		case FST_CHN:
		case FST_CHV: /* only occurs in VCOMP form */
		case FST_COM:
		case FST_RT:
		case FST_SYX:
		case FST_SXP:
			break; /* go add to buffer */
#if defined(AUDIO_DEBUG) || defined(DIAGNOSTIC)
		default:
			printf("midi_wr: midi_fst returned %d?!\n", got);
#endif
		}
		count = sc->xmt.end - sc->xmt.pos;
		if ( 0 == count ) /* can happen with stray 0xf7; see midi_fst */
			continue;
		/*
		 * return EWOULDBLOCK if the data passed will not fit in
		 * the buffer; the caller should have taken steps to avoid that.
		 * If got==FST_SXP we lose the new status byte, but we're losing
		 * anyway, so c'est la vie.
		 */
		if ( idx_cur == idx_lim || count > buf_lim - buf_cur ) {
			MIDI_OUT_LOCK(sc,s);
			MIDI_BUF_PRODUCER_REFRESH(mb,idx); /* get the most */
			MIDI_BUF_PRODUCER_REFRESH(mb,buf); /*  current facts */
			MIDI_OUT_UNLOCK(sc,s);
			if ( idx_cur == idx_lim || count > buf_lim - buf_cur )
				return EWOULDBLOCK; /* caller's problem */
		}
		*idx_cur++ = PACK_MB_IDX(got,count);
		MIDI_BUF_WRAP(idx);
		while ( count ) {
			*buf_cur++ = *(sc->xmt.pos)++;
			MIDI_BUF_WRAP(buf);
			-- count;
		}
		if ( FST_SXP == got )
			-- ibuf; /* again with same status byte */
	}
	MIDI_OUT_LOCK(sc,s);
	MIDI_BUF_PRODUCER_WBACK(mb,buf);
	MIDI_BUF_PRODUCER_WBACK(mb,idx);
	/*
	 * If the output transfer is not already busy, and there is a message
	 * buffered, mark it busy, stop the Active Sense callout (what if we're
	 * too late and it's expired already? No big deal, an extra Active Sense
	 * never hurt anybody) and start the output transfer once we're out of
	 * the critical section (pbus==1 will stop anyone else doing the same).
	 */
	MIDI_BUF_CONSUMER_INIT(mb,idx); /* check what consumer's got to read */
	if ( !sc->pbus && idx_cur < idx_lim ) {
		sc->pbus = 1;
		callout_stop(&sc->xmt_asense_co);
		arming = 1;
	}
	MIDI_OUT_UNLOCK(sc,s);
	return arming ? midi_start_output(sc) : 0;
}

int
midiwrite(dev_t dev, struct uio *uio, int ioflag)
{
	int unit = MIDIUNIT(dev);
	struct midi_softc *sc = midi_cd.cd_devs[unit];
	struct midi_buffer *mb = &sc->outbuf;
	int error;
	u_char inp[256];
	int s;
	MIDI_BUF_DECLARE(idx);
	MIDI_BUF_DECLARE(buf);
	size_t idxspace;
	size_t bufspace;
	size_t xfrcount;
	int pollout = 0;

	DPRINTFN(6, ("midiwrite: %p, unit=%d, count=%lu\n", sc, unit,
		     (unsigned long)uio->uio_resid));

	if (sc->dying)
		return EIO;

	error = 0;
	while (uio->uio_resid > 0 && !error) {

		/*
		 * block if necessary for the minimum buffer space to guarantee
		 * we can write something.
		 */
		MIDI_OUT_LOCK(sc,s);
		MIDI_BUF_PRODUCER_INIT(mb,idx); /* init can't go above loop; */
		MIDI_BUF_PRODUCER_INIT(mb,buf); /* real_writebytes moves cur */
		for ( ;; ) {
			idxspace = MIDI_BUF_PRODUCER_REFRESH(mb,idx) - idx_cur;
			bufspace = MIDI_BUF_PRODUCER_REFRESH(mb,buf) - buf_cur;
			if ( idxspace >= 1  &&  bufspace >= 3  && !pollout )
				break;
			DPRINTFN(8,("midi_write: sleep idx=%zd buf=%zd\n", 
				 idxspace, bufspace));
			if (ioflag & IO_NDELAY) {
				error = EWOULDBLOCK;
				/*
				 * If some amount has already been transferred,
				 * the common syscall code will automagically
				 * convert this to success with a short count.
				 */
				goto locked_exit;
			}
			if ( pollout ) {
				preempt(); /* see midi_poll_output */
				pollout = 0;
			} else
				error = midi_sleep(&sc->wchan, "mid wr",
				                   &sc->out_lock);
			if (error)
				/*
				 * Similarly, the common code will handle
				 * EINTR and ERESTART properly here, changing to
				 * a short count if something transferred.
				 */
				goto locked_exit;
		}
		MIDI_OUT_UNLOCK(sc,s);			

		/*
		 * The number of bytes we can safely extract from the uio
		 * depends on the available idx and buf space. Worst case,
		 * every byte is a message so 1 idx is required per byte.
		 * Worst case, the first byte completes a 3-byte msg in prior
		 * state, and every subsequent byte is a Program Change or
		 * Channel Pressure msg with running status and expands to 2
		 * bytes, so the buf space reqd is 3+2(n-1) or 2n+1. So limit
		 * the transfer to the min of idxspace and (bufspace-1)>>1.
		 */
		xfrcount = (bufspace - 1) >> 1;
		if ( xfrcount > idxspace )
			xfrcount = idxspace;
		if ( xfrcount > sizeof inp )
			xfrcount = sizeof inp;
		if ( xfrcount > uio->uio_resid )
			xfrcount = uio->uio_resid;

		error = uiomove(inp, xfrcount, uio);
#ifdef MIDI_DEBUG
		if (error)
		        printf("midi_write:(1) uiomove failed %d; "
			       "xfrcount=%d inp=%p\n",
			       error, xfrcount, inp);
#endif
		if ( error )
			break;
		
		/*
		 * The number of bytes we extracted being calculated to
		 * definitely fit in the buffer even with canonicalization,
		 * there is no excuse for real_writebytes to return EWOULDBLOCK.
		 */
		error = real_writebytes(sc, inp, xfrcount);
		KASSERT(error != EWOULDBLOCK);
		
		if ( error )
			break;
		/*
		 * If this is a polling device and we just sent a buffer, let's
		 * not send another without giving some other process a chance.
		 */
		if ( ! (sc->props & MIDI_PROP_OUT_INTR) )
			pollout = 1;
		DPRINTFN(8,("midiwrite: uio_resid now %zu, props=%d\n",
                        uio->uio_resid, sc->props));
	}
	return error;

locked_exit:
	MIDI_OUT_UNLOCK(sc,s);
	return error;
}

/*
 * This write routine is only called from sequencer code and expects
 * a write that is smaller than the MIDI buffer.
 */
int
midi_writebytes(int unit, u_char *bf, int cc)
{
	struct midi_softc *sc = midi_cd.cd_devs[unit];

	DPRINTFN(7, ("midi_writebytes: %p, unit=%d, cc=%d %#02x %#02x %#02x\n",
                    sc, unit, cc, bf[0], bf[1], bf[2]));
	return real_writebytes(sc, bf, cc);
}

int
midiioctl(dev_t dev, u_long cmd, void *addr, int flag, struct lwp *l)
{
	int unit = MIDIUNIT(dev);
	struct midi_softc *sc = midi_cd.cd_devs[unit];
	const struct midi_hw_if *hw = sc->hw_if;
	int error;
	int s;
	MIDI_BUF_DECLARE(buf);

	DPRINTFN(5,("midiioctl: %p cmd=0x%08lx\n", sc, cmd));

	if (sc->dying)
		return EIO;

	error = 0;
	switch (cmd) {
	case FIONBIO:
		/* All handled in the upper FS layer. */
		break;
	
	case FIONREAD:
		/*
		 * This code relies on the current implementation of midi_in
		 * always updating buf and idx together in a critical section,
		 * so buf always ends at a message boundary. Document this
		 * ioctl as always returning a value such that the last message
		 * included is complete (SysEx the only exception), and then
		 * make sure the implementation doesn't regress.  NB that
		 * means if this ioctl returns n and the proc then issues a
		 * read of n, n bytes will be read, but if the proc issues a
		 * read of m < n, fewer than m bytes may be read to ensure the
		 * read ends at a message boundary.
		 */
		MIDI_IN_LOCK(sc,s);
		MIDI_BUF_CONSUMER_INIT(&sc->inbuf,buf);
		MIDI_IN_UNLOCK(sc,s);
		*(int *)addr = buf_lim - buf_cur;
		break;

	case FIOASYNC:
		if (*(int *)addr) {
			if (sc->async)
				return EBUSY;
			sc->async = l->l_proc;
			DPRINTFN(5,("midi_ioctl: FIOASYNC %p\n", l->l_proc));
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
			error = hw->ioctl(sc->hw_hdl, cmd, addr, flag, l);
		else
			error = EINVAL;
		break;
	}
	return error;
}

int
midipoll(dev_t dev, int events, struct lwp *l)
{
	int unit = MIDIUNIT(dev);
	struct midi_softc *sc = midi_cd.cd_devs[unit];
	int revents = 0;
	int s;
	MIDI_BUF_DECLARE(idx);
	MIDI_BUF_DECLARE(buf);

	DPRINTFN(6,("midipoll: %p events=0x%x\n", sc, events));

	if (sc->dying)
		return POLLHUP;

	s = splaudio();

	if ((sc->flags&FREAD) && (events & (POLLIN | POLLRDNORM))) {
		simple_lock(&sc->in_lock);
		MIDI_BUF_CONSUMER_INIT(&sc->inbuf,idx);
		if (idx_cur < idx_lim)
			revents |= events & (POLLIN | POLLRDNORM);
		else
			selrecord(l, &sc->rsel);
		simple_unlock(&sc->in_lock);
	}

	if ((sc->flags&FWRITE) && (events & (POLLOUT | POLLWRNORM))) {
		simple_lock(&sc->out_lock);
		MIDI_BUF_PRODUCER_INIT(&sc->outbuf,idx);
		MIDI_BUF_PRODUCER_INIT(&sc->outbuf,buf);
		if ( idx_lim - idx_cur >= 1  &&  buf_lim - buf_cur >= 3 )
			revents |= events & (POLLOUT | POLLWRNORM);
		else
			selrecord(l, &sc->wsel);
		simple_unlock(&sc->out_lock);
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
	int s;
	MIDI_BUF_DECLARE(buf);

	/* XXXLUKEM (thorpej): please make sure this is correct. */

	MIDI_IN_LOCK(sc,s);
	MIDI_BUF_CONSUMER_INIT(&sc->inbuf,buf);
	kn->kn_data = buf_lim - buf_cur;
	MIDI_IN_UNLOCK(sc,s);
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
	int s;
	MIDI_BUF_DECLARE(idx);
	MIDI_BUF_DECLARE(buf);

	/* XXXLUKEM (thorpej): please make sure this is correct. */

	MIDI_OUT_LOCK(sc,s);
	MIDI_BUF_PRODUCER_INIT(&sc->outbuf,idx);
	MIDI_BUF_PRODUCER_INIT(&sc->outbuf,buf);
	kn->kn_data = ((buf_lim - buf_cur)-1)>>1;
	if ( kn->kn_data > idx_lim - idx_cur )
		kn->kn_data = idx_lim - idx_cur;
	MIDI_OUT_UNLOCK(sc,s);
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

#elif NMIDIBUS > 0 /* but NMIDI == 0 */

void midi_register_hw_if_ext(struct midi_hw_if_ext *exthw) { /* stub */
}

#endif /* NMIDI > 0 */

#if NMIDI > 0 || NMIDIBUS > 0

int	audioprint(void *, const char *);

struct device *
midi_attach_mi(const struct midi_hw_if *mhwp, void *hdlp, struct device *dev)
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
