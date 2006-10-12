/* $NetBSD: midictl.c,v 1.3 2006/10/12 01:30:51 christos Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: midictl.c,v 1.3 2006/10/12 01:30:51 christos Exp $");

/*
 * See midictl.h for an overview of the purpose and use of this module.
 */

#if defined(_KERNEL)
#define _MIDICTL_ASSERT(x) KASSERT(x)
#define _MIDICTL_MALLOC(s,t) malloc((s), (t), M_WAITOK)
#define _MIDICTL_ZMALLOC(s,t) malloc((s), (t), M_WAITOK|M_ZERO)
#define _MIDICTL_IMZMALLOC(s,t) malloc((s), (t), M_NOWAIT|M_ZERO)
#define _MIDICTL_PANIC(...) panic(__VA_ARGS__)
#define _MIDICTL_FREE(s,t) free((s), (t))
#include <sys/systm.h>
#include <sys/types.h>
#else
#include <assert.h>
#include <err.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#define _MIDICTL_ASSERT(x) assert(x)
#define _MIDICTL_MALLOC(s,t) malloc((s))
#define _MIDICTL_ZMALLOC(s,t) calloc(1,(s))
#define _MIDICTL_IMZMALLOC(s,t) calloc(1,(s))
#define _MIDICTL_PANIC(...) errx(1,__VA_ARGS__)
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
static _Bool store_locate(midictl_store *s, class c,
                            uint_fast8_t chan, uint_fast16_t key);
/*
 * store_extract and store_update operate on the bucket most recently found
 * by store_locate on this store. That works because reentrancy of midictl
 * functions is limited: they /can/ be reentered during midictl_notify
 * callbacks, but not at other arbitrary times. We never call notify /during/
 * a locate/extract/update transaction.
 */
static uint16_t store_extract(midictl_store *s, class c,
                              uint_fast8_t chan, uint_fast16_t key);
static void store_update(midictl_store *s, class c,
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
	_Bool islsb, present;
		
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
		mc-> rpn &= ~0x7f;
		mc-> rpn |=  PN_SET | (0x7f & ctlval[1]);
		mc->nrpn &= ~PN_SET;
		return;
	case MIDI_CTRL_RPN_MSB:
		mc-> rpn &= ~0x7f<<7;
		mc-> rpn |=  PN_SET | (0x7f & ctlval[1])<<7;
		mc->nrpn &= ~PN_SET;
		return;
	case MIDI_CTRL_NRPN_LSB:
		mc->nrpn &= ~0x7f;
		mc->nrpn |=  PN_SET | (0x7f & ctlval[1]);
		mc-> rpn &= ~PN_SET;
		return;
	case MIDI_CTRL_NRPN_MSB:
		mc->nrpn &= ~0x7f<<7;
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
		return;
	case CTL7:
		present = store_locate(mc->store, c, chan, key);
		if ( !mc->accept_any_ctl_rpn ) {
			if ( !present )
				break;
			val = store_extract(mc->store, c, chan, key);
			if ( !(val&C7_SET) )
				break;
		}
		store_update(mc->store, c, chan, key,
		    C7_SET | (0x7f & ctlval[1]));
		mc->notify(mc->cookie, MIDICTL_CTLR, chan, key);
		return;
	case CTL1:
		present = store_locate(mc->store, c, chan, key);
		if ( !mc->accept_any_ctl_rpn ) {
			if ( !present )
				break;
			val = store_extract(mc->store, c, chan, key);
			if ( !(val&C1_SET) )
				break;
		}
		store_update(mc->store, c, chan, key,
		    C1_SET | (ctlval[1]>63));
		mc->notify(mc->cookie, MIDICTL_CTLR, chan, key);
		return;
	case RPN:
	case NRPN:
		return; /* won't see these - sop for gcc */
	}
}

uint_fast16_t
midictl_read(midictl *mc, uint_fast8_t chan, uint_fast8_t ctlr,
             uint_fast16_t dflt)
{
	uint_fast16_t key, val;
	class c;
	_Bool islsb, present;
	
	key = ctlr;
	c = classify(&key, &islsb);
	switch ( c ) {
	case CTL1:
		present = store_locate(mc->store, c, chan, key);
		if ( !present ||
		    !(C1_SET&(val = store_extract(mc->store, c, chan, key))) ) {
			val = C1_SET | (dflt > 63); /* convert to boolean */
			store_update(mc->store, c, chan, key, val);
		}
		return (val & 1) ? 127 : 0;
	case CTL7:
		present = store_locate(mc->store, c, chan, key);
		if ( !present ||
		    !(C7_SET&(val = store_extract(mc->store, c, chan, key))) ) {
			val = C7_SET | (dflt & 0x7f);
			store_update(mc->store, c, chan, key, val);
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
	_Bool islsb, present;
	
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
		
		present = store_locate(mc->store, c, chan, key);
		if ( !present )
			continue;
		store_update(mc->store, c, chan, key, 0); /* no C*SET */
	}
loop_exit:
	mc->notify(mc->cookie, MIDICTL_RESET, chan, 0);
}

static void
enter14(midictl *mc, uint_fast8_t chan, class c, uint_fast16_t key,
        _Bool islsb, uint8_t val)
{
	uint16_t stval;
	_Bool present;
	
	present = store_locate(mc->store, c, chan, key);
	stval = (present) ? store_extract(mc->store, c, chan, key) : 0;
	if ( !( stval & (C14MSET|C14LSET) ) ) {
		if ( !((NRPN==c)? mc->accept_any_nrpn: mc->accept_any_ctl_rpn) )
			return;
	}
	if ( islsb )
		stval = C14LSET | val | ( stval & ~0x7f );
	else
		stval = C14MSET | ( val << 7 ) | ( stval & ~0x3f80 );
	store_update(mc->store, c, chan, key, stval);
	mc->notify(mc->cookie, CTL14 == c ? MIDICTL_CTLR
		             : RPN   == c ? MIDICTL_RPN
			     : MIDICTL_NRPN, chan, key);
}

static uint_fast16_t
read14(midictl *mc, uint_fast8_t chan, class c, uint_fast16_t key,
       uint_fast16_t dflt)
{
	uint16_t val;
	_Bool present;
	
	present = store_locate(mc->store, c, chan, key);
	if ( !present )
		goto neitherset;

	val = store_extract(mc->store, c, chan, key);
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
	store_update(mc->store, c, chan, key, val);
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
		return CTL1;
	}	  	/* 70-84 defined, 85-90 undef'd, 91-95 def'd */
	return CTL7;	/* 96-101,120- handled above, 102-119 all undef'd */
		  	/* treat them all as CTL7 */
}

static void
notify_no_one(void *cookie __unused, midictl_evt evt __unused,
    uint_fast8_t chan __unused, uint_fast16_t k __unused)
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
 * A double-hashed, open address table is used here. Each slot is a uint64
 * that contains the match key (control class|channel|ctl-or-PN-number) as
 * well as the values for two or more channels. CTL14s, RPNs, and NRPNs can
 * be packed two channels to the slot; CTL7s, six channels; and CTL1s get all
 * 16 channels into one slot. The channel value used in the key is the lowest
 * channel stored in the slot. Open addressing is appropriate here because the
 * link fields in a chained approach would be at least 100% overhead, and also,
 * we don't delete (MIDICTL_RESET is the only event that logically deletes
 * things, and at the moment it does not remove anything from the table, but
 * zeroes the stored value). If wanted, the deletion algorithm for open
 * addressing could be used, with shrinking/rehashing when the load factor
 * drops below 3/8 (3/4 is the current threshold for expansion), and the
 * rehashing would relieve the fills-with-DELETED problem in most cases. But
 * for now the table never shrinks while the device is open.
 */

#include <sys/malloc.h>

#define INITIALLGCAPACITY 6 /* initial capacity 1<<6 */
#define IS_USED 1<<15
#define IS_CTL7 1<<14

#define CTL1SHIFT(chan) (23+((chan)<<1))
#define CTL7SHIFT(chan) (16+((chan)<<3))
#define CTLESHIFT(chan) (23+((chan)<<4))

static uint_fast8_t const packing[] = {
	[CTL1 ] = 16, /* 16 * 2 bits ==> 32 bits, all chns in one bucket */
	[CTL7 ] =  6, /*  6 * 8 bits ==> 48 bits, 6 chns in one bucket */
	[CTL14] =  2, /*  2 *16 bits ==> 32 bits, 2 chns in one bucket */
	[RPN  ] =  2,
	[NRPN ] =  2
};

struct midictl_store {
	uint64_t *table;
	uint64_t key;
	uint32_t idx;
	uint32_t lgcapacity;
	uint32_t used;
};

static uint32_t store_idx(uint32_t lgcapacity,
			  uint64_t table[static 1<<lgcapacity],
                          uint64_t key, uint64_t mask);
static void store_rehash(midictl_store *s);

static midictl_store *
store_init(void)
{
	midictl_store *s;
	
	s = _MIDICTL_MALLOC(sizeof *s, M_DEVBUF);
	s->used = 0;
	s->lgcapacity = INITIALLGCAPACITY;
	s->table = _MIDICTL_ZMALLOC(sizeof *s->table<<s->lgcapacity, M_DEVBUF);
	return s;
}

static void
store_done(midictl_store *s)
{
	_MIDICTL_FREE(s->table, M_DEVBUF);
	_MIDICTL_FREE(s, M_DEVBUF);
}

static _Bool
store_locate(midictl_store *s, class c, uint_fast8_t chan, uint_fast16_t key)
{
	uint64_t mask;
	
	if ( s->used >= 1 << s->lgcapacity )
		_MIDICTL_PANIC("%s: repeated attempts to expand table failed, "
		    "plumb ran out of space", __func__);

	chan = packing[c] * (chan/packing[c]);

	if ( CTL7 == c ) {	/* only 16 bits here (key's only 7) */
		s->key = IS_USED | IS_CTL7 | (chan << 7) | key;
		mask = 0xffff;
	} else {		/* use 23 bits (key could be 14) */
		s->key = (c << 20) | (chan << 16) | IS_USED | key;
		mask = 0x7fffff;
	}
	
	s->idx = store_idx(s->lgcapacity, s->table, s->key, mask);
	
	if ( !(s->table[s->idx] & IS_USED) )
		return 0;

	return 1;
}

static uint16_t
store_extract(midictl_store *s, class c, uint_fast8_t chan,
    uint_fast16_t key __unused)
{
	chan %= packing[c];
	switch ( c ) {
	case CTL1:
		return 3 & (s->table[s->idx]>>CTL1SHIFT(chan));
	case CTL7:
		return 0xff & (s->table[s->idx]>>CTL7SHIFT(chan));
	case CTL14:
	case RPN:
	case NRPN:
		break;
	}
	return 0xffff & (s->table[s->idx]>>CTLESHIFT(chan));
}

static void
store_update(midictl_store *s, class c, uint_fast8_t chan,
    uint_fast16_t key __unused, uint16_t value)
{
	uint64_t orig;
	
	orig = s->table[s->idx];
	if ( !(orig & IS_USED) ) {
		orig = s->key;
		++ s->used;
	}
		
	chan %= packing[c];
	
	switch ( c ) {
	case CTL1:
		orig &= ~(((uint64_t)3)<<CTL1SHIFT(chan));
		orig |= ((uint64_t)(3 & value)) << CTL1SHIFT(chan);
		break;
	case CTL7:
		orig &= ~(((uint64_t)0xff)<<CTL7SHIFT(chan));
		orig |= ((uint64_t)(0xff & value)) << CTL7SHIFT(chan);
		break;
	case CTL14:
	case RPN:
	case NRPN:
		orig &= ~(((uint64_t)0xffff)<<CTLESHIFT(chan));
		orig |= ((uint64_t)value) << CTLESHIFT(chan);
		break;
	}
	
	s->table[s->idx] = orig;
	if ( s->used * 4 >= 3 << s->lgcapacity )
		store_rehash(s);
}

static uint32_t
store_idx(uint32_t lgcapacity, uint64_t table[static 1<<lgcapacity],
          uint64_t key, uint64_t mask)
{
	uint32_t val;
	uint32_t k, h1, h2;
	int32_t idx;
	
	k = key;
	
	h1 = ((k * 0x61c88646) >> (32-lgcapacity)) & ((1<<lgcapacity) - 1);
	h2 = ((k * 0x9e3779b9) >> (32-lgcapacity)) & ((1<<lgcapacity) - 1);	
	h2 |= 1;

	for ( idx = h1 ;; idx -= h2 ) {
		if ( idx < 0 )
			idx += 1<<lgcapacity;
		val = (uint32_t)(table[idx] & mask);
		if ( val == k )
			break;
		if ( !(val & IS_USED) )
			break; 
	}
	
	return idx;
}

static void
store_rehash(midictl_store *s)
{
	uint64_t *newtbl, mask;
	uint32_t newlgcap, oidx, nidx;
	
	newlgcap = 1 + s->lgcapacity;
	newtbl = _MIDICTL_IMZMALLOC(sizeof *newtbl << newlgcap, M_DEVBUF);
	
	/*
	 * Because IMZMALLOC can't sleep, it might have returned NULL.
	 * We rehash when there is some capacity left in the table, so
	 * just leave it alone; we'll get another chance on the next insertion.
	 * Nothing to panic about unless the load factor really hits 1.
	 */
	if ( NULL == newtbl )
		return;

	for ( oidx = 1<<s->lgcapacity ; oidx --> 0 ; ) {
		if ( !(s->table[oidx] & IS_USED) )
			continue;
		if ( s->table[oidx] & IS_CTL7 )
			mask = 0xffff;
		else
			mask = 0x3fffff;
		nidx = store_idx(newlgcap, newtbl, s->table[oidx]&mask, mask);
		newtbl[nidx] = s->table[oidx];
	}
	
	_MIDICTL_FREE(s->table, M_DEVBUF);
	s->table = newtbl;
	s->lgcapacity = newlgcap;
}

#if defined(_MIDICTL_MAIN)
void
dumpstore(void)
{
	uint64_t val;
	uint32_t i, remain;	
	midictl_store *s;
	char const *lbl;
	uint_fast16_t key;
	uint_fast8_t chan;
	class c;
	
	s = mc.store;
	
	printf("store capacity %u, used %u\n", 1<<s->lgcapacity, s->used);
	remain = s->used;
	
	for ( i = 1<<s->lgcapacity; i --> 0; ) {
		if ( !(s->table[i] & IS_USED) )
			continue;
		-- remain;
		if ( s->table[i] & IS_CTL7 ) {
			c = CTL7;
			chan = 0xf & (s->table[i]>>7);
			key = 0x7f & s->table[i];
		} else {
			c = 0x7 & (s->table[i]>>20);
			chan = 0xf & (s->table[i]>>16);
			key = 0x3fff & s->table[i];
		}
		switch ( c ) {
		case CTL1:
			lbl = "CTL1";
			val = s->table[i] >> CTL1SHIFT(0);
			break;
		case CTL7:
			lbl = "CTL7";
			val = s->table[i] >> CTL7SHIFT(0);
			break;
		case CTL14:
			lbl = "CTL14";
			val = s->table[i] >> CTLESHIFT(0);
			break;
		case RPN:
			lbl = "RPN";
			val = s->table[i] >> CTLESHIFT(0);
			break;
		case NRPN:
			lbl = "NRPN";
			val = s->table[i] >> CTLESHIFT(0);
			break;
		default:
			lbl = "???";
			chan = 0;
			key = 0;
			val = s->table[i];
		}
		printf("[%7u] %5s chans %x-%x key %5u: %"PRIx64"\n",
		    i, lbl, chan, chan+packing[c]-1, key, val);
	}
	
	if ( 0 != remain )
		printf("remain == %u ??\n", remain);
}
#endif
