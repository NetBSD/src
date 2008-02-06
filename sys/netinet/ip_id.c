/*	$NetBSD: ip_id.c,v 1.12 2008/02/06 03:20:51 matt Exp $	*/
/*	$OpenBSD: ip_id.c,v 1.6 2002/03/15 18:19:52 millert Exp $	*/

/*
 * Copyright 1998 Niels Provos <provos@citi.umich.edu>
 * All rights reserved.
 *
 * Theo de Raadt <deraadt@openbsd.org> came up with the idea of using
 * such a mathematical system to generate more random (yet non-repeating)
 * ids to solve the resolver/named problem.  But Niels designed the
 * actual system based on the constraints.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 */

/*
 * seed = random 15bit
 * n = prime, g0 = generator to n,
 * j = random so that gcd(j,n-1) == 1
 * g = g0^j mod n will be a generator again.
 *
 * X[0] = random seed.
 * X[n] = a*X[n-1]+b mod m is a Linear Congruential Generator
 * with a = 7^(even random) mod m,
 *      b = random with gcd(b,m) == 1
 *      m = 31104 and a maximal period of m-1.
 *
 * The transaction id is determined by:
 * id[n] = seed xor (g^X[n] mod n)
 *
 * Effectively the id is restricted to the lower 15 bits, thus
 * yielding two different cycles by toggling the msb on and off.
 * This avoids reuse issues caused by reseeding.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ip_id.c,v 1.12 2008/02/06 03:20:51 matt Exp $");

#include "opt_inet.h"

#include <sys/param.h>
#include <lib/libkern/libkern.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_var.h>

#define	IPID_MAXID	65535
#define	IPID_NUMIDS	32768

static struct ipid_state {
	uint16_t ids_start_slot;
	uint16_t ids_slots[IPID_MAXID];
} idstate;

static inline uint32_t
ipid_random(void)
{
	return arc4random();
}

/*
 * Initalizes the  
 * the msb flag. The msb flag is used to generate two distinct
 * cycles of random numbers and thus avoiding reuse of ids.
 *
 * This function is called from id_randomid() when needed, an
 * application does not have to worry about it.
 */
void
ip_initid(void)
{
	size_t i;

	idstate.ids_start_slot = ipid_random();
	for (i = 0; i < __arraycount(idstate.ids_slots); i++)
		idstate.ids_slots[i] = i;

	/*
	 * Shuffle the array.
	 */
	for (i = __arraycount(idstate.ids_slots); --i > 0;) {
		size_t k = ipid_random() % (i + 1);
		uint16_t t = idstate.ids_slots[i];
		idstate.ids_slots[i] = idstate.ids_slots[k];
		idstate.ids_slots[k] = t;
	}
}

uint16_t
ip_randomid(uint16_t salt)
{
	uint32_t r, k, id;

	/*
	 * We need a random number 
	 */
	r = ipid_random();

	/*
	 * We do a modified Fisher-Yates shuffle but only one position at a
	 * time. Instead of the last entry, we swap with the first entry and
	 * then advance the start of the window by 1.  The next time that 
	 * swapped-out entry can be used is at least 32768 iterations in the
	 * future.
 	 *
	 * The easiest way to visual this is to imagine a card deck with 52
	 * cards.  First thing we do is split that into two sets, each with
	 * half of the cards; call them deck A and deck B.  Pick a card
	 * randomly from deck A and remember it, then place it at the
	 * bottom of deck B.  Then take the top card from deck B and add it
	 * to deck A.  Pick another card randomly from deck A and ...
	 */
	k = (r & (IPID_NUMIDS-1)) + idstate.ids_start_slot;
	if (k >= IPID_MAXID)
		k -= IPID_MAXID;

	id = idstate.ids_slots[k];
	if (k != idstate.ids_start_slot) {
		idstate.ids_slots[k] = idstate.ids_slots[idstate.ids_start_slot];
		idstate.ids_slots[idstate.ids_start_slot] = id;
	}
	if (++idstate.ids_start_slot == IPID_MAXID)
		idstate.ids_start_slot = 0;
	/*
	 * Add an optional salt to the id to further obscure it.
	 */
	id += salt;
	if (id >= IPID_MAXID)
		id -= IPID_MAXID;

	return (uint16_t) htons(id + 1);
}
