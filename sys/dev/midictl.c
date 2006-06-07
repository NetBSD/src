/* $NetBSD: midictl.c,v 1.1.2.2 2006/06/07 00:09:39 chap Exp $ */

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Chapman Flack.
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
__KERNEL_RCSID(0, "$NetBSD: midictl.c,v 1.1.2.2 2006/06/07 00:09:39 chap Exp $");

/*
 * See midictl.h for an overview of the purpose and use of this module.
 */

#if defined(_KERNEL)
#define _MIDICTL_ASSERT(x) KASSERT(x)
#define _MIDICTL_MALLOC(s,t) malloc((s), (t), M_WAITOK)
#define _MIDICTL_FREE(s,t) free((s), (t))
#include <sys/systm.h>
#include <sys/types.h>
#else
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#define _MIDICTL_ASSERT(x) assert(x)
#define _MIDICTL_MALLOC(s,t) malloc((s))
#define _MIDICTL_FREE(s,t) free((s))
#endif

#include "midictl.h"

/*
 * The upper part of this file is MIDI-aware, and deals with things like
 * decoding MIDI Control Change messages, dealing with the ones that require
 * special handling as mode messages or parameter updates, and so on.
 *
 * It relies on a "store" layer (implemented in the lower part of this file)
 * that only must be able to stash away 2-, 8-, or 16-bit quantities (which
 * it may pack into larger units as it sees fit) and find them again given
 * a class, channel, and key (controller/parameter number).
 *
 * The MIDI controllers can have 1-, 7-, or 14-bit values; the parameters are
 * also 14-bit. The 14-bit values have to be set in two MIDI messages, 7 bits
 * at a time. The MIDI layer uses store-managed 2- or 8-bit slots for the
 * smaller types, and uses the free high bit to indicate that it has explicitly
 * set the value. (Because the store is allowed to pack things, it may 'find'
 * a zero entry for a value we never set, because it shares a word with a
 * different value that has been set. We know it is not a real value because
 * the high bit is clear.)
 *
 * The 14-bit values are handled similarly: 16-bit store slots are used to hold
 * them, with the two free high bits indicating independently whether the MSB
 * and the LSB have been explicitly set--as two separate MIDI messages are
 * required. If such a control is queried when only one half has been explicitly
 * set, the result is as if it had been set to the specified default value
 * before the explicit set.
 */
typedef struct bucket bucket; /* the store layer completes this type */

typedef enum { CTL1, CTL7, CTL14, RPN, NRPN } class;

/*
 * assert(does_not_apply(KNFNamespaceArgumentAgainstNamesInPrototypes,
 *    PrototypesOfStaticFunctionsWithinNonIncludedFile));
 */
static void reset_all_controllers(midictl *mc, uint_fast8_t chan);
static void enter14(midictl *mc, uint_fast8_t chan, class c,
                    uint_fast16_t key, _Bool islsb, uint8_t val);
static uint_fast16_t read14(midictl *mc, uint_fast8_t chan, class c,
                            uint_fast16_t key, uint_fast16_t dflt);
static class classify(uint_fast16_t *key, _Bool *islsb);
static midictl_notify notify_no_one;

static midictl_store *store_init(void);
static void store_done(midictl_store *s);
static bucket *store_locate(midictl_store *s, class c,
                            uint_fast8_t chan, uint_fast16_t key);
static uint16_t store_extract(bucket *b, class c,
                              uint_fast8_t chan, uint_fast16_t key);
static void store_update(midictl_store *s, bucket *b, class c,
                         uint_fast8_t chan, uint_fast16_t key, uint16_t value);

#define PN_SET 0x8000  /* a parameter number has been explicitly set */
#define C14MSET 0x8000 /* MSB of a 14-bit val has been set */
#define C14LSET 0x4000 /* LSB of a 14-bit val has been set */
#define C7_SET 0x80    /* a 7-bit ctl has been set */
#define C1_SET 2       /* a 1-bit ctl has been set */

#if defined(_MIDICTL_MAIN)
#define XS(s) [MIDICTL_##s]=#s
char const * const evt_strings[] = {
	XS(CTLR), XS(RPN), XS(NRPN), XS(RESET), XS(NOTES_OFF),
	XS(SOUND_OFF), XS(LOCAL), XS(MODE)
};
#undef XS

void
dbgnotify(void *cookie, midictl_evt e, uint_fast8_t chan, uint_fast16_t key)
{
	printf("NFY %p %s chan %u #%u\n", cookie, evt_strings[e], chan, key);
}

midictl mc = {
	.accept_any_ctl_rpn = 0,
	.accept_any_nrpn = 0,
	.base_channel = 16,
	.cookie = NULL,
	.notify = dbgnotify
};

int
main(int argc, char **argv)
{
	int cnt, a, b, c;
	
	midictl_open(&mc);
	do {
		cnt = scanf("%i %i %i", &a, &b, &c);
		if ( 3 == cnt ) {
			midictl_change(&mc, a, (uint8_t[]){b,c});
		}
	} while ( EOF != cnt );
	midictl_close(&mc);
	return 0;
}
#endif /* defined(_MIDICTL_MAIN) */

void
midictl_open(midictl *mc)
{
	if ( NULL == mc->notify )
		mc->notify = notify_no_one;
	mc->store = store_init();
}

void
midictl_close(midictl *mc)
{
	store_done(mc->store);
}

void
midictl_change(midictl *mc, uint_fast8_t chan, uint8_t *ctlval)
{
	class c;
	uint_fast16_t key, val;
	_Bool islsb;
	bucket *bkt;
		
	switch ( ctlval[0] ) {
	/*
	 * Channel mode messages:
	 */
	case MIDI_CTRL_OMNI_OFF:
	case MIDI_CTRL_OMNI_ON:
	case MIDI_CTRL_POLY_OFF:
	case MIDI_CTRL_POLY_ON:
		if ( chan != mc->base_channel )
			return; /* ignored - not on base channel */
		else
			return; /* XXX ignored anyway - not implemented yet */
	case MIDI_CTRL_NOTES_OFF:
		mc->notify(mc->cookie, MIDICTL_NOTES_OFF, chan, 0);
		return;
	case MIDI_CTRL_LOCAL:
		mc->notify(mc->cookie, MIDICTL_LOCAL, chan, ctlval[1]);
		return;
	case MIDI_CTRL_SOUND_OFF:
		mc->notify(mc->cookie, MIDICTL_SOUND_OFF, chan, 0);
		return;
	case MIDI_CTRL_RESET:
		reset_all_controllers(mc, chan);
		return;
	/*
	 * Control changes to be handled specially:
	 */
	case MIDI_CTRL_RPN_LSB:
		mc-> rpn |=  PN_SET | (0x7f & ctlval[1]);
		mc->nrpn &= ~PN_SET;
		return;
	case MIDI_CTRL_RPN_MSB:
		mc-> rpn |=  PN_SET | (0x7f & ctlval[1])<<7;
		mc->nrpn &= ~PN_SET;
		return;
	case MIDI_CTRL_NRPN_LSB:
		mc->nrpn |=  PN_SET | (0x7f & ctlval[1]);
		mc-> rpn &= ~PN_SET;
		return;
	case MIDI_CTRL_NRPN_MSB:
		mc->nrpn |=  PN_SET | (0x7f & ctlval[1])<<7;
		mc-> rpn &= ~PN_SET;
		return;
	case MIDI_CTRL_DATA_ENTRY_LSB:
		islsb = 1;
		goto whichparm;
	case MIDI_CTRL_DATA_ENTRY_MSB:
		islsb = 0;
	whichparm:
		if ( 0 == ( (mc->rpn ^ mc->nrpn) & PN_SET ) )
			return; /* exactly one must be current */
		if ( mc->rpn & PN_SET ) {
			key = mc->rpn;
			c = RPN;
		} else {
			key = mc->nrpn;
			c = NRPN;
		}
		key &= 0x3fff;
		if ( 0x3fff == key ) /* 'null' parm# to lock out changes */
			return;
		enter14(mc, chan, c, key, islsb, ctlval[1]);
		return;
	case MIDI_CTRL_RPN_INCREMENT: /* XXX for later - these are a PITA to */
	case MIDI_CTRL_RPN_DECREMENT: /* get right - 'right' varies by param */
			/* see http://www.midi.org/about-midi/rp18.shtml */
		return;
	}
	
	/*
	 * Channel mode, RPN, and NRPN operations have been ruled out.
	 * This is an ordinary control change.
	 */
	
	key = ctlval[0];
	c = classify(&key, &islsb);
	
	switch ( c ) {
	case CTL14:
		enter14(mc, chan, c, key, islsb, ctlval[1]);
		break;
	case CTL7:
		bkt = store_locate(mc->store, c, chan, key);
		if ( !mc->accept_any_ctl_rpn ) {
			if ( NULL == bkt )
				break;
			val = store_extract(bkt, c, chan, key);
			if ( !(val&C7_SET) )
				break;
		}
		store_update(mc->store, bkt, c, chan, key,
		    C7_SET | (0x7f & ctlval[1]));
		mc->notify(mc->cookie, MIDICTL_CTLR, chan, key);
		break;
	case CTL1:
		bkt = store_locate(mc->store, c, chan, key);
		if ( !mc->accept_any_ctl_rpn ) {
			if ( NULL == bkt )
				break;
			val = store_extract(bkt, c, chan, key);
			if ( !(val&C1_SET) )
				break;
		}
		store_update(mc->store, bkt, c, chan, key,
		    C1_SET | (ctlval[1]>63));
		mc->notify(mc->cookie, MIDICTL_CTLR, chan, key);
		break;
	case RPN:
	case NRPN:
		break; /* won't see these - sop for gcc */
	}
}

uint_fast16_t
midictl_read(midictl *mc, uint_fast8_t chan, uint_fast8_t ctlr,
             uint_fast16_t dflt)
{
	bucket *bkt;
	uint_fast16_t key, val;
	class c;
	_Bool islsb;
	
	key = ctlr;
	c = classify(&key, &islsb);
	switch ( c ) {
	case CTL1:
		bkt = store_locate(mc->store, c, chan, key);
		if ( NULL == bkt ||
		    !(C1_SET&(val = store_extract(bkt, c, chan, key))) ) {
			val = C1_SET | (dflt > 63);
			store_update(mc->store, bkt, c, chan, key, val);
		}
		return (val & 1) ? 127 : 0;
	case CTL7:
		bkt = store_locate(mc->store, c, chan, key);
		if ( NULL == bkt ||
		    !(C7_SET&(val = store_extract(bkt, c, chan, key))) ) {
			val = C7_SET | (dflt & 0x7f);
			store_update(mc->store, bkt, c, chan, key, val);
		}
		return val & 0x7f;
	case CTL14:
		_MIDICTL_ASSERT(!islsb);
		return read14(mc, chan, c, key, dflt);
	case RPN:
	case NRPN:
		break; /* sop for gcc */
	}
	return 0; /* sop for gcc */
}

uint_fast16_t
midictl_rpn_read(midictl *mc, uint_fast8_t chan, uint_fast16_t ctlr,
                 uint_fast16_t dflt)
{
	return read14(mc, chan, RPN, ctlr, dflt);
}

uint_fast16_t
midictl_nrpn_read(midictl *mc, uint_fast8_t chan, uint_fast16_t ctlr,
                  uint_fast16_t dflt)
{
	return read14(mc, chan, NRPN, ctlr, dflt);
}

static void
reset_all_controllers(midictl *mc, uint_fast8_t chan)
{
	uint_fast16_t ctlr, key;
	class c;
	_Bool islsb;
	bucket *bkt;
	
	for ( ctlr = 0 ; ; ++ ctlr ) {
		switch ( ctlr ) {
		/*
		 * exempt by http://www.midi.org/about-midi/rp15.shtml:
		 */
		case MIDI_CTRL_BANK_SELECT_MSB:		/* 0 */
		case MIDI_CTRL_CHANNEL_VOLUME_MSB:	/* 7 */
		case MIDI_CTRL_PAN_MSB:			/* 10 */
			continue;
		case MIDI_CTRL_BANK_SELECT_LSB:		/* 32 */
			ctlr += 31; /* skip all these LSBs anyway */
			continue;
		case MIDI_CTRL_SOUND_VARIATION:		/* 70 */
			ctlr += 9; /* skip all Sound Controllers */
			continue;
		case MIDI_CTRL_EFFECT_DEPTH_1:		/* 91 */
			goto loop_exit; /* nothing more gets reset */
		/*
		 * exempt for our own personal reasons:
		 */
		case MIDI_CTRL_DATA_ENTRY_MSB:		/* 6 */
			continue; /* doesn't go to the store */
		}
		
		key = ctlr;
		c = classify(&key, &islsb);
		
		bkt = store_locate(mc->store, c, chan, key);
		if ( NULL == bkt )
			continue;
		store_update(mc->store, bkt, c, chan, key, 0); /* no C*SET */
	}
loop_exit:
	mc->notify(mc->cookie, MIDICTL_RESET, chan, 0);
}

static void
enter14(midictl *mc, uint_fast8_t chan, class c, uint_fast16_t key,
        _Bool islsb, uint8_t val)
{
	bucket *bkt;
	uint16_t stval;
	
	bkt = store_locate(mc->store, c, chan, key);
	stval = (NULL == bkt) ? 0 : store_extract(bkt, c, chan, key);
	if ( !(stval&(C14MSET|C14LSET)) ) {
		if ( !((NRPN==c)? mc->accept_any_nrpn: mc->accept_any_ctl_rpn) )
			return;
	}
	if ( islsb )
		stval = C14LSET | val | ( stval & ~0x7f );
	else
		stval = C14MSET | ( val << 7 ) | ( stval & ~0x3f80 );
	store_update(mc->store, bkt, c, chan, key, stval);
	mc->notify(mc->cookie, CTL14 == c ? MIDICTL_CTLR
		             : RPN   == c ? MIDICTL_RPN
			     : MIDICTL_NRPN, chan, key);
}

static uint_fast16_t
read14(midictl *mc, uint_fast8_t chan, class c, uint_fast16_t key,
       uint_fast16_t dflt)
{
	bucket *bkt;
	uint16_t val;
	
	bkt = store_locate(mc->store, c, chan, key);
	if ( NULL == bkt )
		goto neitherset;

	val = store_extract(bkt, c, chan, key);
	switch ( val & (C14MSET|C14LSET) ) {
	case C14MSET|C14LSET:
		return val & 0x3fff;
	case C14MSET:
		val = C14LSET | (val & ~0x7f) | (dflt & 0x7f);
		break;
	case C14LSET:
		val = C14MSET | (val & ~0x3f8) | (dflt & 0x3f8);
		break;
neitherset:
	case 0:
		val = C14MSET|C14LSET | (dflt & 0x3fff);
	}
	store_update(mc->store, bkt, c, chan, key, val);
	return val & 0x3fff;
}

/*
 * Determine the controller class; ranges based on
 * http://www.midi.org/about-midi/table3.shtml dated 1995/1999/2002
 * and viewed 2 June 2006.
 */
static class
classify(uint_fast16_t *key, _Bool *islsb) {
	if ( *key < 32 ) {
		*islsb = 0;
		return CTL14;
	} else if ( *key < 64 ) {
		*islsb = 1;
		*key -= 32;
		return CTL14;
	} else if ( *key < 70 ) {
		*key -= 64;
		return CTL1;
	}	  	/* 70-84 defined, 85-90 undef'd, 91-95 def'd */
	return CTL7;	/* 96-101,120- handled above, 102-119 all undef'd */
		  	/* treat them all as CTL7 */
}

static void
notify_no_one(void *cookie, midictl_evt evt, uint_fast8_t chan, uint_fast16_t k)
{
}

#undef PN_SET
#undef C14MSET
#undef C14LSET
#undef C7_SET
#undef C1_SET

/*
 *   I M P L E M E N T A T I O N     O F     T H E     S T O R E :
 *
 * MIDI defines a metric plethora of possible controllers, registered
 * parameters, and nonregistered parameters: a bit more than 32k possible words
 * to store. The saving grace is that only a handful are likely to appear in
 * typical MIDI data, and only a handful are likely implemented by or
 * interesting to a typical client. So the store implementation needs to be
 * suited to a largish but quite sparse data set.
 *
 * For greatest efficiency, this could be implemented over the hash API.
 * For now, it is implemented over libprop, which is not a perfect fit,
 * but because that API does so much more than the hash API, this code
 * has to do less, and simplicity is worth something.
 *
 * prop_numbers are uintmax_t's, which are wider than anything we store, and
 * to reduce waste we want to fill them. The choice is to fill an entry
 * with values for the same controller across some consecutive channels
 * (rather than for consecutive controllers on a channel) because very few
 * controllers are likely to be used, but those that are will probably be used
 * on more than one channel.
 */

#include <prop/proplib.h>
#include <sys/malloc.h>

#define KEYSTRSIZE 8
static void tokeystr(char s[static KEYSTRSIZE],
                     class c, uint_fast8_t chan, uint_fast16_t key);

static uint_fast8_t const packing[] = {
	[CTL1 ] = 4*sizeof(uintmax_t)/sizeof(uint8_t),
	[CTL7 ] =   sizeof(uintmax_t)/sizeof(uint8_t),
	[CTL14] =   sizeof(uintmax_t)/sizeof(uint16_t),
	[RPN  ] =   sizeof(uintmax_t)/sizeof(uint16_t),
	[NRPN ] =   sizeof(uintmax_t)/sizeof(uint16_t)
};

struct bucket {
	union {
		uintmax_t val;
		uint8_t   c7[sizeof(uintmax_t)/sizeof(uint8_t)];
		uint16_t c14[sizeof(uintmax_t)/sizeof(uint16_t)];
	} __packed un;
	midictl_store *ms;
};

struct midictl_store {
	prop_dictionary_t pd;
	bucket bkt; /* assume any one client nonreentrant (for now?) */
};

static midictl_store *
store_init(void)
{
	midictl_store *s;
	
	s = _MIDICTL_MALLOC(sizeof *s, M_DEVBUF);
	s->pd = prop_dictionary_create();
	s->bkt.ms = s;
	return s;
}

static void
store_done(midictl_store *s)
{
	prop_object_release(s->pd);
	_MIDICTL_FREE(s, M_DEVBUF);
}

static bucket *
store_locate(midictl_store *s, class c, uint_fast8_t chan, uint_fast16_t key)
{
	char buf[8];
	prop_number_t pn;
	
	tokeystr(buf, c, chan, key);
	pn = (prop_number_t)prop_dictionary_get(s->pd, buf);
	if ( NULL == pn ) {
		s->bkt.un.val = 0;
		return NULL;
	}
	s->bkt.un.val = prop_number_integer_value(pn);
	return &s->bkt;
}

static uint16_t
store_extract(bucket *b, class c, uint_fast8_t chan, uint_fast16_t key)
{
	chan %= packing[c];
	switch ( c ) {
	case CTL1:
		return 3 & (b->un.c7[chan/4]>>(chan%4)*2);
	case CTL7:
		return b->un.c7[chan];
	case CTL14:
	case RPN:
	case NRPN:
		break;
	}
	return b->un.c14[chan];
}

static void
store_update(midictl_store *s, bucket *b, class c, uint_fast8_t chan,
	     uint_fast16_t key, uint16_t value)
{
	uintmax_t orig;
	char buf[KEYSTRSIZE];
	prop_number_t pn;
	boolean_t success;
	uint_fast8_t ent;
	
	if ( NULL == b ) {
		b = &s->bkt;
		orig = 0;
	} else
		orig = b->un.val;
		
	ent = chan % packing[c];
	
	switch ( c ) {
	case CTL1:
		b->un.c7[ent/4] &= ~(3<<(ent%4)*2);
		b->un.c7[ent/4] |= (3&value)<<(ent%4)*2;
		break;
	case CTL7:
		b->un.c7[ent] = value;
		break;
	case CTL14:
	case RPN:
	case NRPN:
		b->un.c14[ent] = value;
		break;
	}
	
	if ( orig == b->un.val )
		return;

	tokeystr(buf, c, chan, key);
	
	if ( 0 == b->un.val )
		prop_dictionary_remove(s->pd, buf);
	else {
		pn = prop_number_create_integer(b->un.val);
		_MIDICTL_ASSERT(NULL != pn);
		success = prop_dictionary_set(s->pd, buf, pn);
		_MIDICTL_ASSERT(success);
		prop_object_release(pn);
	}
}

static void
tokeystr(char s[static KEYSTRSIZE],
         class c, uint_fast8_t chan, uint_fast16_t key)
{
	snprintf(s, KEYSTRSIZE, "%x%x%x", c, chan/packing[c], key);
}

#if defined(_MIDICTL_MAIN)
void
dumpstore(void)
{
	char *s = prop_dictionary_externalize(mc.store->pd);
	printf("%s", s);
	free(s);
}
#endif
