/*	$NetBSD: keymgr.c,v 1.9 2023/01/25 21:43:30 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include <isc/buffer.h>
#include <isc/dir.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/dnssec.h>
#include <dns/kasp.h>
#include <dns/keymgr.h>
#include <dns/keyvalues.h>
#include <dns/log.h>
#include <dns/result.h>

#include <dst/dst.h>
#include <dst/result.h>

#define RETERR(x)                            \
	do {                                 \
		result = (x);                \
		if (result != ISC_R_SUCCESS) \
			goto failure;        \
	} while (0)

/*
 * Set key state to `target` state and change last changed
 * to `time`, only if key state has not been set before.
 */
#define INITIALIZE_STATE(key, state, timing, target, time)                    \
	do {                                                                  \
		dst_key_state_t s;                                            \
		if (dst_key_getstate((key), (state), &s) == ISC_R_NOTFOUND) { \
			dst_key_setstate((key), (state), (target));           \
			dst_key_settime((key), (timing), time);               \
		}                                                             \
	} while (0)

/* Shorter keywords for better readability. */
#define HIDDEN	    DST_KEY_STATE_HIDDEN
#define RUMOURED    DST_KEY_STATE_RUMOURED
#define OMNIPRESENT DST_KEY_STATE_OMNIPRESENT
#define UNRETENTIVE DST_KEY_STATE_UNRETENTIVE
#define NA	    DST_KEY_STATE_NA

/* Quickly get key state timing metadata. */
#define NUM_KEYSTATES (DST_MAX_KEYSTATES)
static int keystatetimes[NUM_KEYSTATES] = { DST_TIME_DNSKEY, DST_TIME_ZRRSIG,
					    DST_TIME_KRRSIG, DST_TIME_DS };
/* Readable key state types and values. */
static const char *keystatetags[NUM_KEYSTATES] = { "DNSKEY", "ZRRSIG", "KRRSIG",
						   "DS" };
static const char *keystatestrings[4] = { "HIDDEN", "RUMOURED", "OMNIPRESENT",
					  "UNRETENTIVE" };

/*
 * Print key role.
 *
 */
static const char *
keymgr_keyrole(dst_key_t *key) {
	bool ksk = false, zsk = false;
	isc_result_t ret;
	ret = dst_key_getbool(key, DST_BOOL_KSK, &ksk);
	if (ret != ISC_R_SUCCESS) {
		return ("UNKNOWN");
	}
	ret = dst_key_getbool(key, DST_BOOL_ZSK, &zsk);
	if (ret != ISC_R_SUCCESS) {
		return ("UNKNOWN");
	}
	if (ksk && zsk) {
		return ("CSK");
	} else if (ksk) {
		return ("KSK");
	} else if (zsk) {
		return ("ZSK");
	}
	return ("NOSIGN");
}

/*
 * Set the remove time on key given its retire time.
 *
 */
static void
keymgr_settime_remove(dns_dnsseckey_t *key, dns_kasp_t *kasp) {
	isc_stdtime_t retire = 0, remove = 0, ksk_remove = 0, zsk_remove = 0;
	bool zsk = false, ksk = false;
	isc_result_t ret;

	REQUIRE(key != NULL);
	REQUIRE(key->key != NULL);

	ret = dst_key_gettime(key->key, DST_TIME_INACTIVE, &retire);
	if (ret != ISC_R_SUCCESS) {
		return;
	}

	ret = dst_key_getbool(key->key, DST_BOOL_ZSK, &zsk);
	if (ret == ISC_R_SUCCESS && zsk) {
		/* ZSK: Iret = Dsgn + Dprp + TTLsig */
		zsk_remove = retire + dns_kasp_zonemaxttl(kasp) +
			     dns_kasp_zonepropagationdelay(kasp) +
			     dns_kasp_retiresafety(kasp) +
			     dns_kasp_signdelay(kasp);
	}
	ret = dst_key_getbool(key->key, DST_BOOL_KSK, &ksk);
	if (ret == ISC_R_SUCCESS && ksk) {
		/* KSK: Iret = DprpP + TTLds */
		ksk_remove = retire + dns_kasp_dsttl(kasp) +
			     dns_kasp_parentpropagationdelay(kasp) +
			     dns_kasp_retiresafety(kasp);
	}

	remove = ksk_remove > zsk_remove ? ksk_remove : zsk_remove;
	dst_key_settime(key->key, DST_TIME_DELETE, remove);
}

/*
 * Set the SyncPublish time (when the DS may be submitted to the parent)
 *
 */
static void
keymgr_settime_syncpublish(dns_dnsseckey_t *key, dns_kasp_t *kasp, bool first) {
	isc_stdtime_t published, syncpublish;
	bool ksk = false;
	isc_result_t ret;

	REQUIRE(key != NULL);
	REQUIRE(key->key != NULL);

	ret = dst_key_gettime(key->key, DST_TIME_PUBLISH, &published);
	if (ret != ISC_R_SUCCESS) {
		return;
	}

	ret = dst_key_getbool(key->key, DST_BOOL_KSK, &ksk);
	if (ret != ISC_R_SUCCESS || !ksk) {
		return;
	}

	syncpublish = published + dst_key_getttl(key->key) +
		      dns_kasp_zonepropagationdelay(kasp) +
		      dns_kasp_publishsafety(kasp);
	if (first) {
		/* Also need to wait until the signatures are omnipresent. */
		isc_stdtime_t zrrsig_present;
		zrrsig_present = published + dns_kasp_zonemaxttl(kasp) +
				 dns_kasp_zonepropagationdelay(kasp) +
				 dns_kasp_publishsafety(kasp);
		if (zrrsig_present > syncpublish) {
			syncpublish = zrrsig_present;
		}
	}
	dst_key_settime(key->key, DST_TIME_SYNCPUBLISH, syncpublish);
}

/*
 * Calculate prepublication time of a successor key of 'key'.
 * This function can have side effects:
 * 1. If there is no active time set, which would be super weird, set it now.
 * 2. If there is no published time set, also super weird, set it now.
 * 3. If there is no syncpublished time set, set it now.
 * 4. If the lifetime is not set, it will be set now.
 * 5. If there should be a retire time and it is not set, it will be set now.
 * 6. The removed time is adjusted accordingly.
 *
 * This returns when the successor key needs to be published in the zone.
 * A special value of 0 means there is no need for a successor.
 *
 */
static isc_stdtime_t
keymgr_prepublication_time(dns_dnsseckey_t *key, dns_kasp_t *kasp,
			   uint32_t lifetime, isc_stdtime_t now) {
	isc_result_t ret;
	isc_stdtime_t active, retire, pub, prepub;
	bool zsk = false, ksk = false;

	REQUIRE(key != NULL);
	REQUIRE(key->key != NULL);

	active = 0;
	pub = 0;
	retire = 0;

	/*
	 * An active key must have publish and activate timing
	 * metadata.
	 */
	ret = dst_key_gettime(key->key, DST_TIME_ACTIVATE, &active);
	if (ret != ISC_R_SUCCESS) {
		/* Super weird, but if it happens, set it to now. */
		dst_key_settime(key->key, DST_TIME_ACTIVATE, now);
		active = now;
	}
	ret = dst_key_gettime(key->key, DST_TIME_PUBLISH, &pub);
	if (ret != ISC_R_SUCCESS) {
		/* Super weird, but if it happens, set it to now. */
		dst_key_settime(key->key, DST_TIME_PUBLISH, now);
		pub = now;
	}

	/*
	 * Calculate prepublication time.
	 */
	prepub = dst_key_getttl(key->key) + dns_kasp_publishsafety(kasp) +
		 dns_kasp_zonepropagationdelay(kasp);
	ret = dst_key_getbool(key->key, DST_BOOL_KSK, &ksk);
	if (ret == ISC_R_SUCCESS && ksk) {
		isc_stdtime_t syncpub;

		/*
		 * Set PublishCDS if not set.
		 */
		ret = dst_key_gettime(key->key, DST_TIME_SYNCPUBLISH, &syncpub);
		if (ret != ISC_R_SUCCESS) {
			uint32_t tag;
			isc_stdtime_t syncpub1, syncpub2;

			syncpub1 = pub + prepub;
			syncpub2 = 0;
			ret = dst_key_getnum(key->key, DST_NUM_PREDECESSOR,
					     &tag);
			if (ret != ISC_R_SUCCESS) {
				/*
				 * No predecessor, wait for zone to be
				 * completely signed.
				 */
				syncpub2 = pub + dns_kasp_zonemaxttl(kasp) +
					   dns_kasp_publishsafety(kasp) +
					   dns_kasp_zonepropagationdelay(kasp);
			}

			syncpub = syncpub1 > syncpub2 ? syncpub1 : syncpub2;
			dst_key_settime(key->key, DST_TIME_SYNCPUBLISH,
					syncpub);
		}
	}

	/*
	 * Not sure what to do when dst_key_getbool() fails here.  Extending
	 * the prepublication time anyway is arguably the safest thing to do,
	 * so ignore the result code.
	 */
	(void)dst_key_getbool(key->key, DST_BOOL_ZSK, &zsk);

	ret = dst_key_gettime(key->key, DST_TIME_INACTIVE, &retire);
	if (ret != ISC_R_SUCCESS) {
		uint32_t klifetime = 0;

		ret = dst_key_getnum(key->key, DST_NUM_LIFETIME, &klifetime);
		if (ret != ISC_R_SUCCESS) {
			dst_key_setnum(key->key, DST_NUM_LIFETIME, lifetime);
			klifetime = lifetime;
		}
		if (klifetime == 0) {
			/*
			 * No inactive time and no lifetime,
			 * so no need to start a rollover.
			 */
			return (0);
		}

		retire = active + klifetime;
		dst_key_settime(key->key, DST_TIME_INACTIVE, retire);
	}

	/*
	 * Update remove time.
	 */
	keymgr_settime_remove(key, kasp);

	/*
	 * Publish successor 'prepub' time before the 'retire' time of 'key'.
	 */
	if (prepub > retire) {
		/* We should have already prepublished the new key. */
		return (now);
	}
	return (retire - prepub);
}

static void
keymgr_key_retire(dns_dnsseckey_t *key, dns_kasp_t *kasp, isc_stdtime_t now) {
	char keystr[DST_KEY_FORMATSIZE];
	isc_result_t ret;
	isc_stdtime_t retire;
	dst_key_state_t s;
	bool ksk = false, zsk = false;

	REQUIRE(key != NULL);
	REQUIRE(key->key != NULL);

	/* This key wants to retire and hide in a corner. */
	ret = dst_key_gettime(key->key, DST_TIME_INACTIVE, &retire);
	if (ret != ISC_R_SUCCESS || (retire > now)) {
		dst_key_settime(key->key, DST_TIME_INACTIVE, now);
	}
	dst_key_setstate(key->key, DST_KEY_GOAL, HIDDEN);
	keymgr_settime_remove(key, kasp);

	/* This key may not have key states set yet. Pretend as if they are
	 * in the OMNIPRESENT state.
	 */
	if (dst_key_getstate(key->key, DST_KEY_DNSKEY, &s) != ISC_R_SUCCESS) {
		dst_key_setstate(key->key, DST_KEY_DNSKEY, OMNIPRESENT);
		dst_key_settime(key->key, DST_TIME_DNSKEY, now);
	}

	ret = dst_key_getbool(key->key, DST_BOOL_KSK, &ksk);
	if (ret == ISC_R_SUCCESS && ksk) {
		if (dst_key_getstate(key->key, DST_KEY_KRRSIG, &s) !=
		    ISC_R_SUCCESS)
		{
			dst_key_setstate(key->key, DST_KEY_KRRSIG, OMNIPRESENT);
			dst_key_settime(key->key, DST_TIME_KRRSIG, now);
		}
		if (dst_key_getstate(key->key, DST_KEY_DS, &s) != ISC_R_SUCCESS)
		{
			dst_key_setstate(key->key, DST_KEY_DS, OMNIPRESENT);
			dst_key_settime(key->key, DST_TIME_DS, now);
		}
	}
	ret = dst_key_getbool(key->key, DST_BOOL_ZSK, &zsk);
	if (ret == ISC_R_SUCCESS && zsk) {
		if (dst_key_getstate(key->key, DST_KEY_ZRRSIG, &s) !=
		    ISC_R_SUCCESS)
		{
			dst_key_setstate(key->key, DST_KEY_ZRRSIG, OMNIPRESENT);
			dst_key_settime(key->key, DST_TIME_ZRRSIG, now);
		}
	}

	dst_key_format(key->key, keystr, sizeof(keystr));
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_DNSSEC, DNS_LOGMODULE_DNSSEC,
		      ISC_LOG_INFO, "keymgr: retire DNSKEY %s (%s)", keystr,
		      keymgr_keyrole(key->key));
}

/*
 * Check if a dnsseckey matches kasp key configuration.  A dnsseckey matches
 * if it has the same algorithm and size, and if it has the same role as the
 * kasp key configuration.
 *
 */
static bool
keymgr_dnsseckey_kaspkey_match(dns_dnsseckey_t *dkey, dns_kasp_key_t *kkey) {
	dst_key_t *key;
	isc_result_t ret;
	bool role = false;

	REQUIRE(dkey != NULL);
	REQUIRE(kkey != NULL);

	key = dkey->key;

	/* Matching algorithms? */
	if (dst_key_alg(key) != dns_kasp_key_algorithm(kkey)) {
		return (false);
	}
	/* Matching length? */
	if (dst_key_size(key) != dns_kasp_key_size(kkey)) {
		return (false);
	}
	/* Matching role? */
	ret = dst_key_getbool(key, DST_BOOL_KSK, &role);
	if (ret != ISC_R_SUCCESS || role != dns_kasp_key_ksk(kkey)) {
		return (false);
	}
	ret = dst_key_getbool(key, DST_BOOL_ZSK, &role);
	if (ret != ISC_R_SUCCESS || role != dns_kasp_key_zsk(kkey)) {
		return (false);
	}

	/* Found a match. */
	return (true);
}

static bool
keymgr_keyid_conflict(dst_key_t *newkey, dns_dnsseckeylist_t *keys) {
	uint16_t id = dst_key_id(newkey);
	uint32_t rid = dst_key_rid(newkey);
	uint32_t alg = dst_key_alg(newkey);

	for (dns_dnsseckey_t *dkey = ISC_LIST_HEAD(*keys); dkey != NULL;
	     dkey = ISC_LIST_NEXT(dkey, link))
	{
		if (dst_key_alg(dkey->key) != alg) {
			continue;
		}
		if (dst_key_id(dkey->key) == id ||
		    dst_key_rid(dkey->key) == id ||
		    dst_key_id(dkey->key) == rid ||
		    dst_key_rid(dkey->key) == rid)
		{
			return (true);
		}
	}
	return (false);
}

/*
 * Create a new key for 'origin' given the kasp key configuration 'kkey'.
 * This will check for key id collisions with keys in 'keylist'.
 * The created key will be stored in 'dst_key'.
 *
 */
static isc_result_t
keymgr_createkey(dns_kasp_key_t *kkey, const dns_name_t *origin,
		 dns_rdataclass_t rdclass, isc_mem_t *mctx,
		 dns_dnsseckeylist_t *keylist, dns_dnsseckeylist_t *newkeys,
		 dst_key_t **dst_key) {
	bool conflict = false;
	int keyflags = DNS_KEYOWNER_ZONE;
	isc_result_t result = ISC_R_SUCCESS;
	dst_key_t *newkey = NULL;

	do {
		uint32_t algo = dns_kasp_key_algorithm(kkey);
		int size = dns_kasp_key_size(kkey);

		if (dns_kasp_key_ksk(kkey)) {
			keyflags |= DNS_KEYFLAG_KSK;
		}
		RETERR(dst_key_generate(origin, algo, size, 0, keyflags,
					DNS_KEYPROTO_DNSSEC, rdclass, mctx,
					&newkey, NULL));

		/* Key collision? */
		conflict = keymgr_keyid_conflict(newkey, keylist);
		if (!conflict) {
			conflict = keymgr_keyid_conflict(newkey, newkeys);
		}
		if (conflict) {
			/* Try again. */
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_DNSSEC,
				      DNS_LOGMODULE_DNSSEC, ISC_LOG_WARNING,
				      "keymgr: key collision id %d",
				      dst_key_id(newkey));
			dst_key_free(&newkey);
		}
	} while (conflict);

	INSIST(!conflict);
	dst_key_setnum(newkey, DST_NUM_LIFETIME, dns_kasp_key_lifetime(kkey));
	dst_key_setbool(newkey, DST_BOOL_KSK, dns_kasp_key_ksk(kkey));
	dst_key_setbool(newkey, DST_BOOL_ZSK, dns_kasp_key_zsk(kkey));
	*dst_key = newkey;
	return (ISC_R_SUCCESS);

failure:
	return (result);
}

/*
 * Return the desired state for this record 'type'.  The desired state depends
 * on whether the key wants to be active, or wants to retire.  This implements
 * the edges of our state machine:
 *
 *            ---->  OMNIPRESENT  ----
 *            |                      |
 *            |                     \|/
 *
 *        RUMOURED     <---->   UNRETENTIVE
 *
 *           /|\                     |
 *            |                      |
 *            ----     HIDDEN    <----
 *
 * A key that wants to be active eventually wants to have its record types
 * in the OMNIPRESENT state (that is, all resolvers that know about these
 * type of records know about these records specifically).
 *
 * A key that wants to be retired eventually wants to have its record types
 * in the HIDDEN state (that is, all resolvers that know about these type
 * of records specifically don't know about these records).
 *
 */
static dst_key_state_t
keymgr_desiredstate(dns_dnsseckey_t *key, dst_key_state_t state) {
	dst_key_state_t goal;

	if (dst_key_getstate(key->key, DST_KEY_GOAL, &goal) != ISC_R_SUCCESS) {
		/* No goal? No movement. */
		return (state);
	}

	if (goal == HIDDEN) {
		switch (state) {
		case RUMOURED:
		case OMNIPRESENT:
			return (UNRETENTIVE);
		case HIDDEN:
		case UNRETENTIVE:
			return (HIDDEN);
		default:
			return (state);
		}
	} else if (goal == OMNIPRESENT) {
		switch (state) {
		case RUMOURED:
		case OMNIPRESENT:
			return (OMNIPRESENT);
		case HIDDEN:
		case UNRETENTIVE:
			return (RUMOURED);
		default:
			return (state);
		}
	}

	/* Unknown goal. */
	return (state);
}

/*
 * Check if 'key' matches specific 'states'.
 * A state in 'states' that is NA matches any state.
 * A state in 'states' that is HIDDEN also matches if the state is not set.
 * If 'next_state' is set (not NA), we are pretending as if record 'type' of
 * 'subject' key already transitioned to the 'next state'.
 *
 */
static bool
keymgr_key_match_state(dst_key_t *key, dst_key_t *subject, int type,
		       dst_key_state_t next_state,
		       dst_key_state_t states[NUM_KEYSTATES]) {
	REQUIRE(key != NULL);

	for (int i = 0; i < NUM_KEYSTATES; i++) {
		dst_key_state_t state;
		if (states[i] == NA) {
			continue;
		}
		if (next_state != NA && i == type &&
		    dst_key_id(key) == dst_key_id(subject))
		{
			/* Check next state rather than current state. */
			state = next_state;
		} else if (dst_key_getstate(key, i, &state) != ISC_R_SUCCESS) {
			/* This is fine only if expected state is HIDDEN. */
			if (states[i] != HIDDEN) {
				return (false);
			}
			continue;
		}
		if (state != states[i]) {
			return (false);
		}
	}
	/* Match. */
	return (true);
}

/*
 * Key d directly depends on k if d is the direct predecessor of k.
 */
static bool
keymgr_direct_dep(dst_key_t *d, dst_key_t *k) {
	uint32_t s, p;

	if (dst_key_getnum(d, DST_NUM_SUCCESSOR, &s) != ISC_R_SUCCESS) {
		return (false);
	}
	if (dst_key_getnum(k, DST_NUM_PREDECESSOR, &p) != ISC_R_SUCCESS) {
		return (false);
	}
	return (dst_key_id(d) == p && dst_key_id(k) == s);
}

/*
 * Determine which key (if any) has a dependency on k.
 */
static bool
keymgr_dep(dst_key_t *k, dns_dnsseckeylist_t *keyring, uint32_t *dep) {
	for (dns_dnsseckey_t *d = ISC_LIST_HEAD(*keyring); d != NULL;
	     d = ISC_LIST_NEXT(d, link))
	{
		/*
		 * Check if k is a direct successor of d, e.g. d depends on k.
		 */
		if (keymgr_direct_dep(d->key, k)) {
			if (dep != NULL) {
				*dep = dst_key_id(d->key);
			}
			return (true);
		}
	}
	return (false);
}

/*
 * Check if a 'z' is a successor of 'x'.
 * This implements Equation(2) of "Flexible and Robust Key Rollover".
 */
static bool
keymgr_key_is_successor(dst_key_t *x, dst_key_t *z, dst_key_t *key, int type,
			dst_key_state_t next_state,
			dns_dnsseckeylist_t *keyring) {
	uint32_t dep_x;
	uint32_t dep_z;

	/*
	 * The successor relation requires that the predecessor key must not
	 * have any other keys relying on it. In other words, there must be
	 * nothing depending on x.
	 */
	if (keymgr_dep(x, keyring, &dep_x)) {
		return (false);
	}

	/*
	 * If there is no keys relying on key z, then z is not a successor.
	 */
	if (!keymgr_dep(z, keyring, &dep_z)) {
		return (false);
	}

	/*
	 * x depends on z, thus key z is a direct successor of key x.
	 */
	if (dst_key_id(x) == dep_z) {
		return (true);
	}

	/*
	 * It is possible to roll keys faster than the time required to finish
	 * the rollover procedure. For example, consider the keys x, y, z.
	 * Key x is currently published and is going to be replaced by y. The
	 * DNSKEY for x is removed from the zone and at the same moment the
	 * DNSKEY for y is introduced. Key y is a direct dependency for key x
	 * and is therefore the successor of x. However, before the new DNSKEY
	 * has been propagated, key z will replace key y. The DNSKEY for y is
	 * removed and moves into the same state as key x. Key y now directly
	 * depends on key z, and key z will be a new successor key for x.
	 */
	dst_key_state_t zst[NUM_KEYSTATES] = { NA, NA, NA, NA };
	for (int i = 0; i < NUM_KEYSTATES; i++) {
		dst_key_state_t state;
		if (dst_key_getstate(z, i, &state) != ISC_R_SUCCESS) {
			continue;
		}
		zst[i] = state;
	}

	for (dns_dnsseckey_t *y = ISC_LIST_HEAD(*keyring); y != NULL;
	     y = ISC_LIST_NEXT(y, link))
	{
		if (dst_key_id(y->key) == dst_key_id(z)) {
			continue;
		}

		if (dst_key_id(y->key) != dep_z) {
			continue;
		}
		/*
		 * This is another key y, that depends on key z. It may be
		 * part of the successor relation if the key states match
		 * those of key z.
		 */

		if (keymgr_key_match_state(y->key, key, type, next_state, zst))
		{
			/*
			 * If y is a successor of x, then z is also a
			 * successor of x.
			 */
			return (keymgr_key_is_successor(x, y->key, key, type,
							next_state, keyring));
		}
	}

	return (false);
}

/*
 * Check if a key exists in 'keyring' that matches 'states'.
 *
 * If 'match_algorithms', the key must also match the algorithm of 'key'.
 * If 'next_state' is not NA, we are actually looking for a key as if
 *   'key' already transitioned to the next state.
 * If 'check_successor', we also want to make sure there is a successor
 *   relationship with the found key that matches 'states2'.
 */
static bool
keymgr_key_exists_with_state(dns_dnsseckeylist_t *keyring, dns_dnsseckey_t *key,
			     int type, dst_key_state_t next_state,
			     dst_key_state_t states[NUM_KEYSTATES],
			     dst_key_state_t states2[NUM_KEYSTATES],
			     bool check_successor, bool match_algorithms) {
	for (dns_dnsseckey_t *dkey = ISC_LIST_HEAD(*keyring); dkey != NULL;
	     dkey = ISC_LIST_NEXT(dkey, link))
	{
		if (match_algorithms &&
		    (dst_key_alg(dkey->key) != dst_key_alg(key->key)))
		{
			continue;
		}

		if (!keymgr_key_match_state(dkey->key, key->key, type,
					    next_state, states))
		{
			continue;
		}

		/* Found a match. */
		if (!check_successor) {
			return (true);
		}

		/*
		 * We have to make sure that the key we are checking, also
		 * has a successor relationship with another key.
		 */
		for (dns_dnsseckey_t *skey = ISC_LIST_HEAD(*keyring);
		     skey != NULL; skey = ISC_LIST_NEXT(skey, link))
		{
			if (skey == dkey) {
				continue;
			}

			if (!keymgr_key_match_state(skey->key, key->key, type,
						    next_state, states2))
			{
				continue;
			}

			/*
			 * Found a possible successor, check.
			 */
			if (keymgr_key_is_successor(dkey->key, skey->key,
						    key->key, type, next_state,
						    keyring))
			{
				return (true);
			}
		}
	}
	/* No match. */
	return (false);
}

/*
 * Check if a key has a successor.
 */
static bool
keymgr_key_has_successor(dns_dnsseckey_t *predecessor,
			 dns_dnsseckeylist_t *keyring) {
	for (dns_dnsseckey_t *successor = ISC_LIST_HEAD(*keyring);
	     successor != NULL; successor = ISC_LIST_NEXT(successor, link))
	{
		if (keymgr_direct_dep(predecessor->key, successor->key)) {
			return (true);
		}
	}
	return (false);
}

/*
 * Check if all keys have their DS hidden.  If not, then there must be at
 * least one key with an OMNIPRESENT DNSKEY.
 *
 * If 'next_state' is not NA, we are actually looking for a key as if
 *   'key' already transitioned to the next state.
 * If 'match_algorithms', only consider keys with same algorithm of 'key'.
 *
 */
static bool
keymgr_ds_hidden_or_chained(dns_dnsseckeylist_t *keyring, dns_dnsseckey_t *key,
			    int type, dst_key_state_t next_state,
			    bool match_algorithms, bool must_be_hidden) {
	/* (3e) */
	dst_key_state_t dnskey_chained[NUM_KEYSTATES] = { OMNIPRESENT, NA,
							  OMNIPRESENT, NA };
	dst_key_state_t ds_hidden[NUM_KEYSTATES] = { NA, NA, NA, HIDDEN };
	/* successor n/a */
	dst_key_state_t na[NUM_KEYSTATES] = { NA, NA, NA, NA };

	for (dns_dnsseckey_t *dkey = ISC_LIST_HEAD(*keyring); dkey != NULL;
	     dkey = ISC_LIST_NEXT(dkey, link))
	{
		if (match_algorithms &&
		    (dst_key_alg(dkey->key) != dst_key_alg(key->key)))
		{
			continue;
		}

		if (keymgr_key_match_state(dkey->key, key->key, type,
					   next_state, ds_hidden))
		{
			/* This key has its DS hidden. */
			continue;
		}

		if (must_be_hidden) {
			return (false);
		}

		/*
		 * This key does not have its DS hidden. There must be at
		 * least one key with the same algorithm that provides a
		 * chain of trust (can be this key).
		 */
		if (keymgr_key_match_state(dkey->key, key->key, type,
					   next_state, dnskey_chained))
		{
			/* This DNSKEY and KRRSIG are OMNIPRESENT. */
			continue;
		}

		/*
		 * Perhaps another key provides a chain of trust.
		 */
		dnskey_chained[DST_KEY_DS] = OMNIPRESENT;
		if (!keymgr_key_exists_with_state(keyring, key, type,
						  next_state, dnskey_chained,
						  na, false, match_algorithms))
		{
			/* There is no chain of trust. */
			return (false);
		}
	}
	/* All good. */
	return (true);
}

/*
 * Check if all keys have their DNSKEY hidden.  If not, then there must be at
 * least one key with an OMNIPRESENT ZRRSIG.
 *
 * If 'next_state' is not NA, we are actually looking for a key as if
 *   'key' already transitioned to the next state.
 * If 'match_algorithms', only consider keys with same algorithm of 'key'.
 *
 */
static bool
keymgr_dnskey_hidden_or_chained(dns_dnsseckeylist_t *keyring,
				dns_dnsseckey_t *key, int type,
				dst_key_state_t next_state,
				bool match_algorithms) {
	/* (3i) */
	dst_key_state_t rrsig_chained[NUM_KEYSTATES] = { OMNIPRESENT,
							 OMNIPRESENT, NA, NA };
	dst_key_state_t dnskey_hidden[NUM_KEYSTATES] = { HIDDEN, NA, NA, NA };
	/* successor n/a */
	dst_key_state_t na[NUM_KEYSTATES] = { NA, NA, NA, NA };

	for (dns_dnsseckey_t *dkey = ISC_LIST_HEAD(*keyring); dkey != NULL;
	     dkey = ISC_LIST_NEXT(dkey, link))
	{
		if (match_algorithms &&
		    (dst_key_alg(dkey->key) != dst_key_alg(key->key)))
		{
			continue;
		}

		if (keymgr_key_match_state(dkey->key, key->key, type,
					   next_state, dnskey_hidden))
		{
			/* This key has its DNSKEY hidden. */
			continue;
		}

		/*
		 * This key does not have its DNSKEY hidden. There must be at
		 * least one key with the same algorithm that has its RRSIG
		 * records OMNIPRESENT.
		 */
		(void)dst_key_getstate(dkey->key, DST_KEY_DNSKEY,
				       &rrsig_chained[DST_KEY_DNSKEY]);
		if (!keymgr_key_exists_with_state(keyring, key, type,
						  next_state, rrsig_chained, na,
						  false, match_algorithms))
		{
			/* There is no chain of trust. */
			return (false);
		}
	}
	/* All good. */
	return (true);
}

/*
 * Check for existence of DS.
 *
 */
static bool
keymgr_have_ds(dns_dnsseckeylist_t *keyring, dns_dnsseckey_t *key, int type,
	       dst_key_state_t next_state, bool secure_to_insecure) {
	/* (3a) */
	dst_key_state_t states[2][NUM_KEYSTATES] = {
		/* DNSKEY, ZRRSIG, KRRSIG, DS */
		{ NA, NA, NA, OMNIPRESENT }, /* DS present */
		{ NA, NA, NA, RUMOURED }     /* DS introducing */
	};
	/* successor n/a */
	dst_key_state_t na[NUM_KEYSTATES] = { NA, NA, NA, NA };

	/*
	 * Equation (3a):
	 * There is a key with the DS in either RUMOURD or OMNIPRESENT state.
	 */
	return (keymgr_key_exists_with_state(keyring, key, type, next_state,
					     states[0], na, false, false) ||
		keymgr_key_exists_with_state(keyring, key, type, next_state,
					     states[1], na, false, false) ||
		(secure_to_insecure &&
		 keymgr_key_exists_with_state(keyring, key, type, next_state,
					      na, na, false, false)));
}

/*
 * Check for existence of DNSKEY, or at least a good DNSKEY state.
 * See equations what are good DNSKEY states.
 *
 */
static bool
keymgr_have_dnskey(dns_dnsseckeylist_t *keyring, dns_dnsseckey_t *key, int type,
		   dst_key_state_t next_state) {
	dst_key_state_t states[9][NUM_KEYSTATES] = {
		/* DNSKEY,     ZRRSIG, KRRSIG,      DS */
		{ OMNIPRESENT, NA, OMNIPRESENT, OMNIPRESENT }, /* (3b) */

		{ OMNIPRESENT, NA, OMNIPRESENT, UNRETENTIVE }, /* (3c)p */
		{ OMNIPRESENT, NA, OMNIPRESENT, RUMOURED },    /* (3c)s */

		{ UNRETENTIVE, NA, UNRETENTIVE, OMNIPRESENT }, /* (3d)p */
		{ OMNIPRESENT, NA, UNRETENTIVE, OMNIPRESENT }, /* (3d)p */
		{ UNRETENTIVE, NA, OMNIPRESENT, OMNIPRESENT }, /* (3d)p */
		{ RUMOURED, NA, RUMOURED, OMNIPRESENT },       /* (3d)s */
		{ OMNIPRESENT, NA, RUMOURED, OMNIPRESENT },    /* (3d)s */
		{ RUMOURED, NA, OMNIPRESENT, OMNIPRESENT },    /* (3d)s */
	};
	/* successor n/a */
	dst_key_state_t na[NUM_KEYSTATES] = { NA, NA, NA, NA };

	return (
		/*
		 * Equation (3b):
		 * There is a key with the same algorithm with its DNSKEY,
		 * KRRSIG and DS records in OMNIPRESENT state.
		 */
		keymgr_key_exists_with_state(keyring, key, type, next_state,
					     states[0], na, false, true) ||
		/*
		 * Equation (3c):
		 * There are two or more keys with an OMNIPRESENT DNSKEY and
		 * the DS records get swapped.  These keys must be in a
		 * successor relation.
		 */
		keymgr_key_exists_with_state(keyring, key, type, next_state,
					     states[1], states[2], true,
					     true) ||
		/*
		 * Equation (3d):
		 * There are two or more keys with an OMNIPRESENT DS and
		 * the DNSKEY records and its KRRSIG records get swapped.
		 * These keys must be in a successor relation.  Since the
		 * state for DNSKEY and KRRSIG move independently, we have
		 * to check all combinations for DNSKEY and KRRSIG in
		 * OMNIPRESENT/UNRETENTIVE state for the predecessor, and
		 * OMNIPRESENT/RUMOURED state for the successor.
		 */
		keymgr_key_exists_with_state(keyring, key, type, next_state,
					     states[3], states[6], true,
					     true) ||
		keymgr_key_exists_with_state(keyring, key, type, next_state,
					     states[3], states[7], true,
					     true) ||
		keymgr_key_exists_with_state(keyring, key, type, next_state,
					     states[3], states[8], true,
					     true) ||
		keymgr_key_exists_with_state(keyring, key, type, next_state,
					     states[4], states[6], true,
					     true) ||
		keymgr_key_exists_with_state(keyring, key, type, next_state,
					     states[4], states[7], true,
					     true) ||
		keymgr_key_exists_with_state(keyring, key, type, next_state,
					     states[4], states[8], true,
					     true) ||
		keymgr_key_exists_with_state(keyring, key, type, next_state,
					     states[5], states[6], true,
					     true) ||
		keymgr_key_exists_with_state(keyring, key, type, next_state,
					     states[5], states[7], true,
					     true) ||
		keymgr_key_exists_with_state(keyring, key, type, next_state,
					     states[5], states[8], true,
					     true) ||
		/*
		 * Equation (3e):
		 * The key may be in any state as long as all keys have their
		 * DS HIDDEN, or when their DS is not HIDDEN, there must be a
		 * key with its DS in the same state and its DNSKEY omnipresent.
		 * In other words, if a DS record for the same algorithm is
		 * is still available to some validators, there must be a
		 * chain of trust for those validators.
		 */
		keymgr_ds_hidden_or_chained(keyring, key, type, next_state,
					    true, false));
}

/*
 * Check for existence of RRSIG (zsk), or a good RRSIG state.
 * See equations what are good RRSIG states.
 *
 */
static bool
keymgr_have_rrsig(dns_dnsseckeylist_t *keyring, dns_dnsseckey_t *key, int type,
		  dst_key_state_t next_state) {
	dst_key_state_t states[11][NUM_KEYSTATES] = {
		/* DNSKEY,     ZRRSIG,      KRRSIG, DS */
		{ OMNIPRESENT, OMNIPRESENT, NA, NA }, /* (3f) */
		{ UNRETENTIVE, OMNIPRESENT, NA, NA }, /* (3g)p */
		{ RUMOURED, OMNIPRESENT, NA, NA },    /* (3g)s */
		{ OMNIPRESENT, UNRETENTIVE, NA, NA }, /* (3h)p */
		{ OMNIPRESENT, RUMOURED, NA, NA },    /* (3h)s */
	};
	/* successor n/a */
	dst_key_state_t na[NUM_KEYSTATES] = { NA, NA, NA, NA };

	return (
		/*
		 * If all DS records are hidden than this rule can be ignored.
		 */
		keymgr_ds_hidden_or_chained(keyring, key, type, next_state,
					    true, true) ||
		/*
		 * Equation (3f):
		 * There is a key with the same algorithm with its DNSKEY and
		 * ZRRSIG records in OMNIPRESENT state.
		 */
		keymgr_key_exists_with_state(keyring, key, type, next_state,
					     states[0], na, false, true) ||
		/*
		 * Equation (3g):
		 * There are two or more keys with OMNIPRESENT ZRRSIG
		 * records and the DNSKEY records get swapped.  These keys
		 * must be in a successor relation.
		 */
		keymgr_key_exists_with_state(keyring, key, type, next_state,
					     states[1], states[2], true,
					     true) ||
		/*
		 * Equation (3h):
		 * There are two or more keys with an OMNIPRESENT DNSKEY
		 * and the ZRRSIG records get swapped.  These keys must be in
		 * a successor relation.
		 */
		keymgr_key_exists_with_state(keyring, key, type, next_state,
					     states[3], states[4], true,
					     true) ||
		/*
		 * Equation (3i):
		 * If no DNSKEYs are published, the state of the signatures is
		 * irrelevant.  In case a DNSKEY is published however, there
		 * must be a path that can be validated from there.
		 */
		keymgr_dnskey_hidden_or_chained(keyring, key, type, next_state,
						true));
}

/*
 * Check if a transition in the state machine is allowed by the policy.
 * This means when we do rollovers, we want to follow the rules of the
 * 1. Pre-publish rollover method (in case of a ZSK)
 *    - First introduce the DNSKEY record.
 *    - Only if the DNSKEY record is OMNIPRESENT, introduce ZRRSIG records.
 *
 * 2. Double-KSK rollover method (in case of a KSK)
 *    - First introduce the DNSKEY record, as well as the KRRSIG records.
 *    - Only if the DNSKEY record is OMNIPRESENT, suggest to introduce the DS.
 */
static bool
keymgr_policy_approval(dns_dnsseckeylist_t *keyring, dns_dnsseckey_t *key,
		       int type, dst_key_state_t next) {
	dst_key_state_t dnskeystate = HIDDEN;
	dst_key_state_t ksk_present[NUM_KEYSTATES] = { OMNIPRESENT, NA,
						       OMNIPRESENT,
						       OMNIPRESENT };
	dst_key_state_t ds_rumoured[NUM_KEYSTATES] = { OMNIPRESENT, NA,
						       OMNIPRESENT, RUMOURED };
	dst_key_state_t ds_retired[NUM_KEYSTATES] = { OMNIPRESENT, NA,
						      OMNIPRESENT,
						      UNRETENTIVE };
	dst_key_state_t ksk_rumoured[NUM_KEYSTATES] = { RUMOURED, NA, NA,
							OMNIPRESENT };
	dst_key_state_t ksk_retired[NUM_KEYSTATES] = { UNRETENTIVE, NA, NA,
						       OMNIPRESENT };
	/* successor n/a */
	dst_key_state_t na[NUM_KEYSTATES] = { NA, NA, NA, NA };

	if (next != RUMOURED) {
		/*
		 * Local policy only adds an extra barrier on transitions to
		 * the RUMOURED state.
		 */
		return (true);
	}

	switch (type) {
	case DST_KEY_DNSKEY:
		/* No restrictions. */
		return (true);
	case DST_KEY_ZRRSIG:
		/* Make sure the DNSKEY record is OMNIPRESENT. */
		(void)dst_key_getstate(key->key, DST_KEY_DNSKEY, &dnskeystate);
		if (dnskeystate == OMNIPRESENT) {
			return (true);
		}
		/*
		 * Or are we introducing a new key for this algorithm? Because
		 * in that case allow publishing the RRSIG records before the
		 * DNSKEY.
		 */
		return (!(keymgr_key_exists_with_state(keyring, key, type, next,
						       ksk_present, na, false,
						       true) ||
			  keymgr_key_exists_with_state(keyring, key, type, next,
						       ds_retired, ds_rumoured,
						       true, true) ||
			  keymgr_key_exists_with_state(
				  keyring, key, type, next, ksk_retired,
				  ksk_rumoured, true, true)));
	case DST_KEY_KRRSIG:
		/* Only introduce if the DNSKEY is also introduced. */
		(void)dst_key_getstate(key->key, DST_KEY_DNSKEY, &dnskeystate);
		return (dnskeystate != HIDDEN);
	case DST_KEY_DS:
		/* Make sure the DNSKEY record is OMNIPRESENT. */
		(void)dst_key_getstate(key->key, DST_KEY_DNSKEY, &dnskeystate);
		return (dnskeystate == OMNIPRESENT);
	default:
		return (false);
	}
}

/*
 * Check if a transition in the state machine is DNSSEC safe.
 * This implements Equation(1) of "Flexible and Robust Key Rollover".
 *
 */
static bool
keymgr_transition_allowed(dns_dnsseckeylist_t *keyring, dns_dnsseckey_t *key,
			  int type, dst_key_state_t next_state,
			  bool secure_to_insecure) {
	/* Debug logging. */
	if (isc_log_wouldlog(dns_lctx, ISC_LOG_DEBUG(1))) {
		bool rule1a, rule1b, rule2a, rule2b, rule3a, rule3b;
		char keystr[DST_KEY_FORMATSIZE];
		dst_key_format(key->key, keystr, sizeof(keystr));
		rule1a = keymgr_have_ds(keyring, key, type, NA,
					secure_to_insecure);
		rule1b = keymgr_have_ds(keyring, key, type, next_state,
					secure_to_insecure);
		rule2a = keymgr_have_dnskey(keyring, key, type, NA);
		rule2b = keymgr_have_dnskey(keyring, key, type, next_state);
		rule3a = keymgr_have_rrsig(keyring, key, type, NA);
		rule3b = keymgr_have_rrsig(keyring, key, type, next_state);
		isc_log_write(
			dns_lctx, DNS_LOGCATEGORY_DNSSEC, DNS_LOGMODULE_DNSSEC,
			ISC_LOG_DEBUG(1),
			"keymgr: dnssec evaluation of %s %s record %s: "
			"rule1=(~%s or %s) rule2=(~%s or %s) "
			"rule3=(~%s or %s)",
			keymgr_keyrole(key->key), keystr, keystatetags[type],
			rule1a ? "true" : "false", rule1b ? "true" : "false",
			rule2a ? "true" : "false", rule2b ? "true" : "false",
			rule3a ? "true" : "false", rule3b ? "true" : "false");
	}

	return (
		/*
		 * Rule 1: There must be a DS at all times.
		 * First check the current situation: if the rule check fails,
		 * we allow the transition to attempt to move us out of the
		 * invalid state.  If the rule check passes, also check if
		 * the next state is also still a valid situation.
		 */
		(!keymgr_have_ds(keyring, key, type, NA, secure_to_insecure) ||
		 keymgr_have_ds(keyring, key, type, next_state,
				secure_to_insecure)) &&
		/*
		 * Rule 2: There must be a DNSKEY at all times.  Again, first
		 * check the current situation, then assess the next state.
		 */
		(!keymgr_have_dnskey(keyring, key, type, NA) ||
		 keymgr_have_dnskey(keyring, key, type, next_state)) &&
		/*
		 * Rule 3: There must be RRSIG records at all times. Again,
		 * first check the current situation, then assess the next
		 * state.
		 */
		(!keymgr_have_rrsig(keyring, key, type, NA) ||
		 keymgr_have_rrsig(keyring, key, type, next_state)));
}

/*
 * Calculate the time when it is safe to do the next transition.
 *
 */
static void
keymgr_transition_time(dns_dnsseckey_t *key, int type,
		       dst_key_state_t next_state, dns_kasp_t *kasp,
		       isc_stdtime_t now, isc_stdtime_t *when) {
	isc_result_t ret;
	isc_stdtime_t lastchange, dstime, nexttime = now;

	/*
	 * No need to wait if we move things into an uncertain state.
	 */
	if (next_state == RUMOURED || next_state == UNRETENTIVE) {
		*when = now;
		return;
	}

	ret = dst_key_gettime(key->key, keystatetimes[type], &lastchange);
	if (ret != ISC_R_SUCCESS) {
		/* No last change, for safety purposes let's set it to now. */
		dst_key_settime(key->key, keystatetimes[type], now);
		lastchange = now;
	}

	switch (type) {
	case DST_KEY_DNSKEY:
	case DST_KEY_KRRSIG:
		switch (next_state) {
		case OMNIPRESENT:
			/*
			 * RFC 7583: The publication interval (Ipub) is the
			 * amount of time that must elapse after the
			 * publication of a DNSKEY (plus RRSIG (KSK)) before
			 * it can be assumed that any resolvers that have the
			 * relevant RRset cached have a copy of the new
			 * information.  This is the sum of the propagation
			 * delay (Dprp) and the DNSKEY TTL (TTLkey).  This
			 * translates to zone-propagation-delay + dnskey-ttl.
			 * We will also add the publish-safety interval.
			 */
			nexttime = lastchange + dst_key_getttl(key->key) +
				   dns_kasp_zonepropagationdelay(kasp) +
				   dns_kasp_publishsafety(kasp);
			break;
		case HIDDEN:
			/*
			 * Same as OMNIPRESENT but without the publish-safety
			 * interval.
			 */
			nexttime = lastchange + dst_key_getttl(key->key) +
				   dns_kasp_zonepropagationdelay(kasp);
			break;
		default:
			nexttime = now;
			break;
		}
		break;
	case DST_KEY_ZRRSIG:
		switch (next_state) {
		case OMNIPRESENT:
		case HIDDEN:
			/*
			 * RFC 7583: The retire interval (Iret) is the amount
			 * of time that must elapse after a DNSKEY or
			 * associated data enters the retire state for any
			 * dependent information (RRSIG ZSK) to be purged from
			 * validating resolver caches.  This is defined as:
			 *
			 *     Iret = Dsgn + Dprp + TTLsig
			 *
			 * Where Dsgn is the Dsgn is the delay needed to
			 * ensure that all existing RRsets have been re-signed
			 * with the new key, Dprp is the propagation delay and
			 * TTLsig is the maximum TTL of all zone RRSIG
			 * records.  This translates to:
			 *
			 *     Dsgn + zone-propagation-delay + max-zone-ttl.
			 *
			 * We will also add the retire-safety interval.
			 */
			nexttime = lastchange + dns_kasp_zonemaxttl(kasp) +
				   dns_kasp_zonepropagationdelay(kasp) +
				   dns_kasp_retiresafety(kasp);
			/*
			 * Only add the sign delay Dsgn if there is an actual
			 * predecessor or successor key.
			 */
			uint32_t tag;
			ret = dst_key_getnum(key->key, DST_NUM_PREDECESSOR,
					     &tag);
			if (ret != ISC_R_SUCCESS) {
				ret = dst_key_getnum(key->key,
						     DST_NUM_SUCCESSOR, &tag);
			}
			if (ret == ISC_R_SUCCESS) {
				nexttime += dns_kasp_signdelay(kasp);
			}
			break;
		default:
			nexttime = now;
			break;
		}
		break;
	case DST_KEY_DS:
		switch (next_state) {
		/*
		 * RFC 7583: The successor DS record is published in
		 * the parent zone and after the registration delay
		 * (Dreg), the time taken after the DS record has been
		 * submitted to the parent zone manager for it to be
		 * placed in the zone.  Key N (the predecessor) must
		 * remain in the zone until any caches that contain a
		 * copy of the DS RRset have a copy containing the new
		 * DS record. This interval is the retire interval
		 * (Iret), given by:
		 *
		 *      Iret = DprpP + TTLds
		 *
		 * This translates to:
		 *
		 *      parent-propagation-delay + parent-ds-ttl.
		 *
		 * We will also add the retire-safety interval.
		 */
		case OMNIPRESENT:
			/* Make sure DS has been seen in the parent. */
			ret = dst_key_gettime(key->key, DST_TIME_DSPUBLISH,
					      &dstime);
			if (ret != ISC_R_SUCCESS || dstime > now) {
				/* Not yet, try again in an hour. */
				nexttime = now + 3600;
			} else {
				nexttime =
					dstime + dns_kasp_dsttl(kasp) +
					dns_kasp_parentpropagationdelay(kasp) +
					dns_kasp_retiresafety(kasp);
			}
			break;
		case HIDDEN:
			/* Make sure DS has been withdrawn from the parent. */
			ret = dst_key_gettime(key->key, DST_TIME_DSDELETE,
					      &dstime);
			if (ret != ISC_R_SUCCESS || dstime > now) {
				/* Not yet, try again in an hour. */
				nexttime = now + 3600;
			} else {
				nexttime =
					dstime + dns_kasp_dsttl(kasp) +
					dns_kasp_parentpropagationdelay(kasp) +
					dns_kasp_retiresafety(kasp);
			}
			break;
		default:
			nexttime = now;
			break;
		}
		break;
	default:
		UNREACHABLE();
		break;
	}

	*when = nexttime;
}

/*
 * Update keys.
 * This implements Algorithm (1) of "Flexible and Robust Key Rollover".
 *
 */
static isc_result_t
keymgr_update(dns_dnsseckeylist_t *keyring, dns_kasp_t *kasp, isc_stdtime_t now,
	      isc_stdtime_t *nexttime, bool secure_to_insecure) {
	bool changed;

	/* Repeat until nothing changed. */
transition:
	changed = false;

	/* For all keys in the zone. */
	for (dns_dnsseckey_t *dkey = ISC_LIST_HEAD(*keyring); dkey != NULL;
	     dkey = ISC_LIST_NEXT(dkey, link))
	{
		char keystr[DST_KEY_FORMATSIZE];
		dst_key_format(dkey->key, keystr, sizeof(keystr));

		/* For all records related to this key. */
		for (int i = 0; i < NUM_KEYSTATES; i++) {
			isc_result_t ret;
			isc_stdtime_t when;
			dst_key_state_t state, next_state;

			ret = dst_key_getstate(dkey->key, i, &state);
			if (ret == ISC_R_NOTFOUND) {
				/*
				 * This record type is not applicable for this
				 * key, continue to the next record type.
				 */
				continue;
			}

			isc_log_write(dns_lctx, DNS_LOGCATEGORY_DNSSEC,
				      DNS_LOGMODULE_DNSSEC, ISC_LOG_DEBUG(1),
				      "keymgr: examine %s %s type %s "
				      "in state %s",
				      keymgr_keyrole(dkey->key), keystr,
				      keystatetags[i], keystatestrings[state]);

			/* Get the desired next state. */
			next_state = keymgr_desiredstate(dkey, state);
			if (state == next_state) {
				/*
				 * This record is in a stable state.
				 * No change needed, continue with the next
				 * record type.
				 */
				isc_log_write(dns_lctx, DNS_LOGCATEGORY_DNSSEC,
					      DNS_LOGMODULE_DNSSEC,
					      ISC_LOG_DEBUG(1),
					      "keymgr: %s %s type %s in "
					      "stable state %s",
					      keymgr_keyrole(dkey->key), keystr,
					      keystatetags[i],
					      keystatestrings[state]);
				continue;
			}

			isc_log_write(dns_lctx, DNS_LOGCATEGORY_DNSSEC,
				      DNS_LOGMODULE_DNSSEC, ISC_LOG_DEBUG(1),
				      "keymgr: can we transition %s %s type %s "
				      "state %s to state %s?",
				      keymgr_keyrole(dkey->key), keystr,
				      keystatetags[i], keystatestrings[state],
				      keystatestrings[next_state]);

			/* Is the transition allowed according to policy? */
			if (!keymgr_policy_approval(keyring, dkey, i,
						    next_state))
			{
				/* No, please respect rollover methods. */
				isc_log_write(
					dns_lctx, DNS_LOGCATEGORY_DNSSEC,
					DNS_LOGMODULE_DNSSEC, ISC_LOG_DEBUG(1),
					"keymgr: policy says no to %s %s type "
					"%s "
					"state %s to state %s",
					keymgr_keyrole(dkey->key), keystr,
					keystatetags[i], keystatestrings[state],
					keystatestrings[next_state]);

				continue;
			}

			/* Is the transition DNSSEC safe? */
			if (!keymgr_transition_allowed(keyring, dkey, i,
						       next_state,
						       secure_to_insecure))
			{
				/* No, this would make the zone bogus. */
				isc_log_write(
					dns_lctx, DNS_LOGCATEGORY_DNSSEC,
					DNS_LOGMODULE_DNSSEC, ISC_LOG_DEBUG(1),
					"keymgr: dnssec says no to %s %s type "
					"%s "
					"state %s to state %s",
					keymgr_keyrole(dkey->key), keystr,
					keystatetags[i], keystatestrings[state],
					keystatestrings[next_state]);
				continue;
			}

			/* Is it time to make the transition? */
			when = now;
			keymgr_transition_time(dkey, i, next_state, kasp, now,
					       &when);
			if (when > now) {
				/* Not yet. */
				isc_log_write(
					dns_lctx, DNS_LOGCATEGORY_DNSSEC,
					DNS_LOGMODULE_DNSSEC, ISC_LOG_DEBUG(1),
					"keymgr: time says no to %s %s type %s "
					"state %s to state %s (wait %u "
					"seconds)",
					keymgr_keyrole(dkey->key), keystr,
					keystatetags[i], keystatestrings[state],
					keystatestrings[next_state],
					when - now);
				if (*nexttime == 0 || *nexttime > when) {
					*nexttime = when;
				}
				continue;
			}

			isc_log_write(dns_lctx, DNS_LOGCATEGORY_DNSSEC,
				      DNS_LOGMODULE_DNSSEC, ISC_LOG_DEBUG(1),
				      "keymgr: transition %s %s type %s "
				      "state %s to state %s!",
				      keymgr_keyrole(dkey->key), keystr,
				      keystatetags[i], keystatestrings[state],
				      keystatestrings[next_state]);

			/* It is safe to make the transition. */
			dst_key_setstate(dkey->key, i, next_state);
			dst_key_settime(dkey->key, keystatetimes[i], now);
			INSIST(dst_key_ismodified(dkey->key));
			changed = true;
		}
	}

	/* We changed something, continue processing. */
	if (changed) {
		goto transition;
	}

	return (ISC_R_SUCCESS);
}

/*
 * See if this key needs to be initialized with properties.  A key created
 * and derived from a dnssec-policy will have the required metadata available,
 * otherwise these may be missing and need to be initialized.  The key states
 * will be initialized according to existing timing metadata.
 *
 */
static void
keymgr_key_init(dns_dnsseckey_t *key, dns_kasp_t *kasp, isc_stdtime_t now,
		bool csk) {
	bool ksk, zsk;
	isc_result_t ret;
	isc_stdtime_t active = 0, pub = 0, syncpub = 0, retire = 0, remove = 0;
	dst_key_state_t dnskey_state = HIDDEN;
	dst_key_state_t ds_state = HIDDEN;
	dst_key_state_t zrrsig_state = HIDDEN;
	dst_key_state_t goal_state = HIDDEN;

	REQUIRE(key != NULL);
	REQUIRE(key->key != NULL);

	/* Initialize role. */
	ret = dst_key_getbool(key->key, DST_BOOL_KSK, &ksk);
	if (ret != ISC_R_SUCCESS) {
		ksk = ((dst_key_flags(key->key) & DNS_KEYFLAG_KSK) != 0);
		dst_key_setbool(key->key, DST_BOOL_KSK, (ksk || csk));
	}
	ret = dst_key_getbool(key->key, DST_BOOL_ZSK, &zsk);
	if (ret != ISC_R_SUCCESS) {
		zsk = ((dst_key_flags(key->key) & DNS_KEYFLAG_KSK) == 0);
		dst_key_setbool(key->key, DST_BOOL_ZSK, (zsk || csk));
	}

	/* Get time metadata. */
	ret = dst_key_gettime(key->key, DST_TIME_ACTIVATE, &active);
	if (active <= now && ret == ISC_R_SUCCESS) {
		dns_ttl_t zone_ttl = dns_kasp_zonemaxttl(kasp);
		zone_ttl += dns_kasp_zonepropagationdelay(kasp);
		if ((active + zone_ttl) <= now) {
			zrrsig_state = OMNIPRESENT;
		} else {
			zrrsig_state = RUMOURED;
		}
		goal_state = OMNIPRESENT;
	}
	ret = dst_key_gettime(key->key, DST_TIME_PUBLISH, &pub);
	if (pub <= now && ret == ISC_R_SUCCESS) {
		dns_ttl_t key_ttl = dst_key_getttl(key->key);
		key_ttl += dns_kasp_zonepropagationdelay(kasp);
		if ((pub + key_ttl) <= now) {
			dnskey_state = OMNIPRESENT;
		} else {
			dnskey_state = RUMOURED;
		}
		goal_state = OMNIPRESENT;
	}
	ret = dst_key_gettime(key->key, DST_TIME_SYNCPUBLISH, &syncpub);
	if (syncpub <= now && ret == ISC_R_SUCCESS) {
		dns_ttl_t ds_ttl = dns_kasp_dsttl(kasp);
		ds_ttl += dns_kasp_parentpropagationdelay(kasp);
		if ((syncpub + ds_ttl) <= now) {
			ds_state = OMNIPRESENT;
		} else {
			ds_state = RUMOURED;
		}
		goal_state = OMNIPRESENT;
	}
	ret = dst_key_gettime(key->key, DST_TIME_INACTIVE, &retire);
	if (retire <= now && ret == ISC_R_SUCCESS) {
		dns_ttl_t zone_ttl = dns_kasp_zonemaxttl(kasp);
		zone_ttl += dns_kasp_zonepropagationdelay(kasp);
		if ((retire + zone_ttl) <= now) {
			zrrsig_state = HIDDEN;
		} else {
			zrrsig_state = UNRETENTIVE;
		}
		ds_state = UNRETENTIVE;
		goal_state = HIDDEN;
	}
	ret = dst_key_gettime(key->key, DST_TIME_DELETE, &remove);
	if (remove <= now && ret == ISC_R_SUCCESS) {
		dns_ttl_t key_ttl = dst_key_getttl(key->key);
		key_ttl += dns_kasp_zonepropagationdelay(kasp);
		if ((remove + key_ttl) <= now) {
			dnskey_state = HIDDEN;
		} else {
			dnskey_state = UNRETENTIVE;
		}
		zrrsig_state = HIDDEN;
		ds_state = HIDDEN;
		goal_state = HIDDEN;
	}

	/* Set goal if not already set. */
	if (dst_key_getstate(key->key, DST_KEY_GOAL, &goal_state) !=
	    ISC_R_SUCCESS)
	{
		dst_key_setstate(key->key, DST_KEY_GOAL, goal_state);
	}

	/* Set key states for all keys that do not have them. */
	INITIALIZE_STATE(key->key, DST_KEY_DNSKEY, DST_TIME_DNSKEY,
			 dnskey_state, now);
	if (ksk || csk) {
		INITIALIZE_STATE(key->key, DST_KEY_KRRSIG, DST_TIME_KRRSIG,
				 dnskey_state, now);
		INITIALIZE_STATE(key->key, DST_KEY_DS, DST_TIME_DS, ds_state,
				 now);
	}
	if (zsk || csk) {
		INITIALIZE_STATE(key->key, DST_KEY_ZRRSIG, DST_TIME_ZRRSIG,
				 zrrsig_state, now);
	}
}

static isc_result_t
keymgr_key_rollover(dns_kasp_key_t *kaspkey, dns_dnsseckey_t *active_key,
		    dns_dnsseckeylist_t *keyring, dns_dnsseckeylist_t *newkeys,
		    const dns_name_t *origin, dns_rdataclass_t rdclass,
		    dns_kasp_t *kasp, uint32_t lifetime, bool rollover,
		    isc_stdtime_t now, isc_stdtime_t *nexttime,
		    isc_mem_t *mctx) {
	char keystr[DST_KEY_FORMATSIZE];
	isc_stdtime_t retire = 0, active = 0, prepub = 0;
	dns_dnsseckey_t *new_key = NULL;
	dns_dnsseckey_t *candidate = NULL;
	dst_key_t *dst_key = NULL;

	/* Do we need to create a successor for the active key? */
	if (active_key != NULL) {
		if (isc_log_wouldlog(dns_lctx, ISC_LOG_DEBUG(1))) {
			dst_key_format(active_key->key, keystr, sizeof(keystr));
			isc_log_write(
				dns_lctx, DNS_LOGCATEGORY_DNSSEC,
				DNS_LOGMODULE_DNSSEC, ISC_LOG_DEBUG(1),
				"keymgr: DNSKEY %s (%s) is active in policy %s",
				keystr, keymgr_keyrole(active_key->key),
				dns_kasp_getname(kasp));
		}

		/*
		 * Calculate when the successor needs to be published
		 * in the zone.
		 */
		prepub = keymgr_prepublication_time(active_key, kasp, lifetime,
						    now);
		if (prepub == 0 || prepub > now) {
			if (isc_log_wouldlog(dns_lctx, ISC_LOG_DEBUG(1))) {
				dst_key_format(active_key->key, keystr,
					       sizeof(keystr));
				isc_log_write(
					dns_lctx, DNS_LOGCATEGORY_DNSSEC,
					DNS_LOGMODULE_DNSSEC, ISC_LOG_DEBUG(1),
					"keymgr: new successor needed for "
					"DNSKEY %s (%s) (policy %s) in %u "
					"seconds",
					keystr, keymgr_keyrole(active_key->key),
					dns_kasp_getname(kasp), (prepub - now));
			}

			/* No need to start rollover now. */
			if (*nexttime == 0 || prepub < *nexttime) {
				*nexttime = prepub;
			}
			return (ISC_R_SUCCESS);
		}

		if (keymgr_key_has_successor(active_key, keyring)) {
			/* Key already has successor. */
			if (isc_log_wouldlog(dns_lctx, ISC_LOG_DEBUG(1))) {
				dst_key_format(active_key->key, keystr,
					       sizeof(keystr));
				isc_log_write(
					dns_lctx, DNS_LOGCATEGORY_DNSSEC,
					DNS_LOGMODULE_DNSSEC, ISC_LOG_DEBUG(1),
					"keymgr: key DNSKEY %s (%s) (policy "
					"%s) already has successor",
					keystr, keymgr_keyrole(active_key->key),
					dns_kasp_getname(kasp));
			}
			return (ISC_R_SUCCESS);
		}

		if (isc_log_wouldlog(dns_lctx, ISC_LOG_DEBUG(1))) {
			dst_key_format(active_key->key, keystr, sizeof(keystr));
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_DNSSEC,
				      DNS_LOGMODULE_DNSSEC, ISC_LOG_DEBUG(1),
				      "keymgr: need successor for DNSKEY %s "
				      "(%s) (policy %s)",
				      keystr, keymgr_keyrole(active_key->key),
				      dns_kasp_getname(kasp));
		}

		/*
		 * If rollover is not allowed, warn.
		 */
		if (!rollover) {
			dst_key_format(active_key->key, keystr, sizeof(keystr));
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_DNSSEC,
				      DNS_LOGMODULE_DNSSEC, ISC_LOG_WARNING,
				      "keymgr: DNSKEY %s (%s) is offline in "
				      "policy %s, cannot start rollover",
				      keystr, keymgr_keyrole(active_key->key),
				      dns_kasp_getname(kasp));
			return (ISC_R_SUCCESS);
		}
	} else if (isc_log_wouldlog(dns_lctx, ISC_LOG_DEBUG(1))) {
		char namestr[DNS_NAME_FORMATSIZE];
		dns_name_format(origin, namestr, sizeof(namestr));
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DNSSEC,
			      DNS_LOGMODULE_DNSSEC, ISC_LOG_DEBUG(1),
			      "keymgr: no active key found for %s (policy %s)",
			      namestr, dns_kasp_getname(kasp));
	}

	/* It is time to do key rollover, we need a new key. */

	/*
	 * Check if there is a key available in pool because keys
	 * may have been pregenerated with dnssec-keygen.
	 */
	for (candidate = ISC_LIST_HEAD(*keyring); candidate != NULL;
	     candidate = ISC_LIST_NEXT(candidate, link))
	{
		if (keymgr_dnsseckey_kaspkey_match(candidate, kaspkey) &&
		    dst_key_is_unused(candidate->key))
		{
			/* Found a candidate in keyring. */
			break;
		}
	}

	if (candidate == NULL) {
		/* No key available in keyring, create a new one. */
		bool csk = (dns_kasp_key_ksk(kaspkey) &&
			    dns_kasp_key_zsk(kaspkey));

		isc_result_t result = keymgr_createkey(kaspkey, origin, rdclass,
						       mctx, keyring, newkeys,
						       &dst_key);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
		dst_key_setttl(dst_key, dns_kasp_dnskeyttl(kasp));
		dst_key_settime(dst_key, DST_TIME_CREATED, now);
		result = dns_dnsseckey_create(mctx, &dst_key, &new_key);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
		keymgr_key_init(new_key, kasp, now, csk);
	} else {
		new_key = candidate;
	}
	dst_key_setnum(new_key->key, DST_NUM_LIFETIME, lifetime);

	/* Got a key. */
	if (active_key == NULL) {
		/*
		 * If there is no active key found yet for this kasp
		 * key configuration, immediately make this key active.
		 */
		dst_key_settime(new_key->key, DST_TIME_PUBLISH, now);
		dst_key_settime(new_key->key, DST_TIME_ACTIVATE, now);
		keymgr_settime_syncpublish(new_key, kasp, true);
		active = now;
	} else {
		/*
		 * This is a successor.  Mark the relationship.
		 */
		isc_stdtime_t created;
		(void)dst_key_gettime(new_key->key, DST_TIME_CREATED, &created);

		dst_key_setnum(new_key->key, DST_NUM_PREDECESSOR,
			       dst_key_id(active_key->key));
		dst_key_setnum(active_key->key, DST_NUM_SUCCESSOR,
			       dst_key_id(new_key->key));
		(void)dst_key_gettime(active_key->key, DST_TIME_INACTIVE,
				      &retire);
		active = retire;

		/*
		 * If prepublication time and/or retire time are
		 * in the past (before the new key was created), use
		 * creation time as published and active time,
		 * effectively immediately making the key active.
		 */
		if (prepub < created) {
			active += (created - prepub);
			prepub = created;
		}
		if (active < created) {
			active = created;
		}
		dst_key_settime(new_key->key, DST_TIME_PUBLISH, prepub);
		dst_key_settime(new_key->key, DST_TIME_ACTIVATE, active);
		keymgr_settime_syncpublish(new_key, kasp, false);

		/*
		 * Retire predecessor.
		 */
		dst_key_setstate(active_key->key, DST_KEY_GOAL, HIDDEN);
	}

	/* This key wants to be present. */
	dst_key_setstate(new_key->key, DST_KEY_GOAL, OMNIPRESENT);

	/* Do we need to set retire time? */
	if (lifetime > 0) {
		dst_key_settime(new_key->key, DST_TIME_INACTIVE,
				(active + lifetime));
		keymgr_settime_remove(new_key, kasp);
	}

	/* Append dnsseckey to list of new keys. */
	dns_dnssec_get_hints(new_key, now);
	new_key->source = dns_keysource_repository;
	INSIST(!new_key->legacy);
	if (candidate == NULL) {
		ISC_LIST_APPEND(*newkeys, new_key, link);
	}

	/* Logging. */
	dst_key_format(new_key->key, keystr, sizeof(keystr));
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_DNSSEC, DNS_LOGMODULE_DNSSEC,
		      ISC_LOG_INFO, "keymgr: DNSKEY %s (%s) %s for policy %s",
		      keystr, keymgr_keyrole(new_key->key),
		      (candidate != NULL) ? "selected" : "created",
		      dns_kasp_getname(kasp));
	return (ISC_R_SUCCESS);
}

static bool
keymgr_key_may_be_purged(dst_key_t *key, uint32_t after, isc_stdtime_t now) {
	bool ksk = false;
	bool zsk = false;
	dst_key_state_t hidden[NUM_KEYSTATES] = { HIDDEN, NA, NA, NA };
	isc_stdtime_t lastchange = 0;

	char keystr[DST_KEY_FORMATSIZE];
	dst_key_format(key, keystr, sizeof(keystr));

	/* If 'purge-keys' is disabled, always retain keys. */
	if (after == 0) {
		return (false);
	}

	/* Don't purge keys with goal OMNIPRESENT */
	if (dst_key_goal(key) == OMNIPRESENT) {
		return (false);
	}

	/* Don't purge unused keys. */
	if (dst_key_is_unused(key)) {
		return (false);
	}

	/* If this key is completely HIDDEN it may be purged. */
	(void)dst_key_getbool(key, DST_BOOL_KSK, &ksk);
	(void)dst_key_getbool(key, DST_BOOL_ZSK, &zsk);
	if (ksk) {
		hidden[DST_KEY_KRRSIG] = HIDDEN;
		hidden[DST_KEY_DS] = HIDDEN;
	}
	if (zsk) {
		hidden[DST_KEY_ZRRSIG] = HIDDEN;
	}
	if (!keymgr_key_match_state(key, key, 0, NA, hidden)) {
		return (false);
	}

	/*
	 * Check 'purge-keys' interval. If the interval has passed since
	 * the last key change, it may be purged.
	 */
	for (int i = 0; i < NUM_KEYSTATES; i++) {
		isc_stdtime_t change = 0;
		(void)dst_key_gettime(key, keystatetimes[i], &change);
		if (change > lastchange) {
			lastchange = change;
		}
	}

	return ((lastchange + after) < now);
}

static void
keymgr_purge_keyfile(dst_key_t *key, const char *dir, int type) {
	isc_result_t ret;
	isc_buffer_t fileb;
	char filename[NAME_MAX];

	/*
	 * Make the filename.
	 */
	isc_buffer_init(&fileb, filename, sizeof(filename));
	ret = dst_key_buildfilename(key, type, dir, &fileb);
	if (ret != ISC_R_SUCCESS) {
		char keystr[DST_KEY_FORMATSIZE];
		dst_key_format(key, keystr, sizeof(keystr));
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DNSSEC,
			      DNS_LOGMODULE_DNSSEC, ISC_LOG_WARNING,
			      "keymgr: failed to purge DNSKEY %s (%s): cannot "
			      "build filename (%s)",
			      keystr, keymgr_keyrole(key),
			      isc_result_totext(ret));
		return;
	}

	if (unlink(filename) < 0) {
		char keystr[DST_KEY_FORMATSIZE];
		dst_key_format(key, keystr, sizeof(keystr));
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DNSSEC,
			      DNS_LOGMODULE_DNSSEC, ISC_LOG_WARNING,
			      "keymgr: failed to purge DNSKEY %s (%s): unlink "
			      "'%s' failed",
			      keystr, keymgr_keyrole(key), filename);
	}
}

/*
 * Examine 'keys' and match 'kasp' policy.
 *
 */
isc_result_t
dns_keymgr_run(const dns_name_t *origin, dns_rdataclass_t rdclass,
	       const char *directory, isc_mem_t *mctx,
	       dns_dnsseckeylist_t *keyring, dns_dnsseckeylist_t *dnskeys,
	       dns_kasp_t *kasp, isc_stdtime_t now, isc_stdtime_t *nexttime) {
	isc_result_t result = ISC_R_SUCCESS;
	dns_dnsseckeylist_t newkeys;
	dns_kasp_key_t *kkey;
	dns_dnsseckey_t *newkey = NULL;
	isc_dir_t dir;
	bool dir_open = false;
	bool secure_to_insecure = false;
	int numkeys = 0;
	int options = (DST_TYPE_PRIVATE | DST_TYPE_PUBLIC | DST_TYPE_STATE);
	char keystr[DST_KEY_FORMATSIZE];

	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(keyring != NULL);

	ISC_LIST_INIT(newkeys);

	isc_dir_init(&dir);
	if (directory == NULL) {
		directory = ".";
	}

	RETERR(isc_dir_open(&dir, directory));
	dir_open = true;

	*nexttime = 0;

	/* Debug logging: what keys are available in the keyring? */
	if (isc_log_wouldlog(dns_lctx, ISC_LOG_DEBUG(1))) {
		if (ISC_LIST_EMPTY(*keyring)) {
			char namebuf[DNS_NAME_FORMATSIZE];
			dns_name_format(origin, namebuf, sizeof(namebuf));
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_DNSSEC,
				      DNS_LOGMODULE_DNSSEC, ISC_LOG_DEBUG(1),
				      "keymgr: keyring empty (zone %s policy "
				      "%s)",
				      namebuf, dns_kasp_getname(kasp));
		}

		for (dns_dnsseckey_t *dkey = ISC_LIST_HEAD(*keyring);
		     dkey != NULL; dkey = ISC_LIST_NEXT(dkey, link))
		{
			dst_key_format(dkey->key, keystr, sizeof(keystr));
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_DNSSEC,
				      DNS_LOGMODULE_DNSSEC, ISC_LOG_DEBUG(1),
				      "keymgr: keyring: %s (policy %s)", keystr,
				      dns_kasp_getname(kasp));
		}
		for (dns_dnsseckey_t *dkey = ISC_LIST_HEAD(*dnskeys);
		     dkey != NULL; dkey = ISC_LIST_NEXT(dkey, link))
		{
			dst_key_format(dkey->key, keystr, sizeof(keystr));
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_DNSSEC,
				      DNS_LOGMODULE_DNSSEC, ISC_LOG_DEBUG(1),
				      "keymgr: dnskeys: %s (policy %s)", keystr,
				      dns_kasp_getname(kasp));
		}
	}

	for (dns_dnsseckey_t *dkey = ISC_LIST_HEAD(*dnskeys); dkey != NULL;
	     dkey = ISC_LIST_NEXT(dkey, link))
	{
		numkeys++;
	}

	/* Do we need to remove keys? */
	for (dns_dnsseckey_t *dkey = ISC_LIST_HEAD(*keyring); dkey != NULL;
	     dkey = ISC_LIST_NEXT(dkey, link))
	{
		bool found_match = false;

		keymgr_key_init(dkey, kasp, now, (numkeys == 1));

		for (kkey = ISC_LIST_HEAD(dns_kasp_keys(kasp)); kkey != NULL;
		     kkey = ISC_LIST_NEXT(kkey, link))
		{
			if (keymgr_dnsseckey_kaspkey_match(dkey, kkey)) {
				found_match = true;
				break;
			}
		}

		/* No match, so retire unwanted retire key. */
		if (!found_match) {
			keymgr_key_retire(dkey, kasp, now);
		}

		/* Check purge-keys interval. */
		if (keymgr_key_may_be_purged(dkey->key,
					     dns_kasp_purgekeys(kasp), now))
		{
			dst_key_format(dkey->key, keystr, sizeof(keystr));
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_DNSSEC,
				      DNS_LOGMODULE_DNSSEC, ISC_LOG_INFO,
				      "keymgr: purge DNSKEY %s (%s) according "
				      "to policy %s",
				      keystr, keymgr_keyrole(dkey->key),
				      dns_kasp_getname(kasp));

			keymgr_purge_keyfile(dkey->key, directory,
					     DST_TYPE_PUBLIC);
			keymgr_purge_keyfile(dkey->key, directory,
					     DST_TYPE_PRIVATE);
			keymgr_purge_keyfile(dkey->key, directory,
					     DST_TYPE_STATE);

			dkey->purge = true;
		}
	}

	/* Create keys according to the policy, if come in short. */
	for (kkey = ISC_LIST_HEAD(dns_kasp_keys(kasp)); kkey != NULL;
	     kkey = ISC_LIST_NEXT(kkey, link))
	{
		uint32_t lifetime = dns_kasp_key_lifetime(kkey);
		dns_dnsseckey_t *active_key = NULL;
		bool rollover_allowed = true;

		/* Do we have keys available for this kasp key? */
		for (dns_dnsseckey_t *dkey = ISC_LIST_HEAD(*keyring);
		     dkey != NULL; dkey = ISC_LIST_NEXT(dkey, link))
		{
			if (keymgr_dnsseckey_kaspkey_match(dkey, kkey)) {
				/* Found a match. */
				dst_key_format(dkey->key, keystr,
					       sizeof(keystr));
				isc_log_write(dns_lctx, DNS_LOGCATEGORY_DNSSEC,
					      DNS_LOGMODULE_DNSSEC,
					      ISC_LOG_DEBUG(1),
					      "keymgr: DNSKEY %s (%s) matches "
					      "policy %s",
					      keystr, keymgr_keyrole(dkey->key),
					      dns_kasp_getname(kasp));

				/* Initialize lifetime if not set. */
				uint32_t l;
				if (dst_key_getnum(dkey->key, DST_NUM_LIFETIME,
						   &l) != ISC_R_SUCCESS)
				{
					dst_key_setnum(dkey->key,
						       DST_NUM_LIFETIME,
						       lifetime);
				}

				if (active_key) {
					/* We already have an active key that
					 * matches the kasp policy.
					 */
					if (!dst_key_is_unused(dkey->key) &&
					    (dst_key_goal(dkey->key) ==
					     OMNIPRESENT) &&
					    !keymgr_dep(dkey->key, keyring,
							NULL) &&
					    !keymgr_dep(active_key->key,
							keyring, NULL))
					{
						/*
						 * Multiple signing keys match
						 * the kasp key configuration.
						 * Retire excess keys in use.
						 */
						keymgr_key_retire(dkey, kasp,
								  now);
					}
					continue;
				}

				/*
				 * Save the matched key only if it is active
				 * or desires to be active.
				 */
				if (dst_key_goal(dkey->key) == OMNIPRESENT ||
				    dst_key_is_active(dkey->key, now))
				{
					active_key = dkey;
				}
			}
		}

		if (active_key == NULL) {
			/*
			 * We didn't found an active key, perhaps the .private
			 * key file is offline. If so, we don't want to create
			 * a successor key. Check if we have an appropriate
			 * state file.
			 */
			for (dns_dnsseckey_t *dnskey = ISC_LIST_HEAD(*dnskeys);
			     dnskey != NULL;
			     dnskey = ISC_LIST_NEXT(dnskey, link))
			{
				if (keymgr_dnsseckey_kaspkey_match(dnskey,
								   kkey))
				{
					/* Found a match. */
					dst_key_format(dnskey->key, keystr,
						       sizeof(keystr));
					isc_log_write(
						dns_lctx,
						DNS_LOGCATEGORY_DNSSEC,
						DNS_LOGMODULE_DNSSEC,
						ISC_LOG_DEBUG(1),
						"keymgr: DNSKEY %s (%s) "
						"offline, policy %s",
						keystr,
						keymgr_keyrole(dnskey->key),
						dns_kasp_getname(kasp));
					rollover_allowed = false;
					active_key = dnskey;
					break;
				}
			}
		}

		/* See if this key requires a rollover. */
		RETERR(keymgr_key_rollover(
			kkey, active_key, keyring, &newkeys, origin, rdclass,
			kasp, lifetime, rollover_allowed, now, nexttime, mctx));
	}

	/* Walked all kasp key configurations.  Append new keys. */
	if (!ISC_LIST_EMPTY(newkeys)) {
		ISC_LIST_APPENDLIST(*keyring, newkeys, link);
	}

	/*
	 * If the policy has an empty key list, this means the zone is going
	 * back to unsigned.
	 */
	secure_to_insecure = dns_kasp_keylist_empty(kasp);

	/* Read to update key states. */
	keymgr_update(keyring, kasp, now, nexttime, secure_to_insecure);

	/* Store key states and update hints. */
	for (dns_dnsseckey_t *dkey = ISC_LIST_HEAD(*keyring); dkey != NULL;
	     dkey = ISC_LIST_NEXT(dkey, link))
	{
		if (dst_key_ismodified(dkey->key) && !dkey->purge) {
			dns_dnssec_get_hints(dkey, now);
			RETERR(dst_key_tofile(dkey->key, options, directory));
			dst_key_setmodified(dkey->key, false);
		}
	}

	result = ISC_R_SUCCESS;

failure:
	if (dir_open) {
		isc_dir_close(&dir);
	}

	if (result != ISC_R_SUCCESS) {
		while ((newkey = ISC_LIST_HEAD(newkeys)) != NULL) {
			ISC_LIST_UNLINK(newkeys, newkey, link);
			INSIST(newkey->key != NULL);
			dst_key_free(&newkey->key);
			dns_dnsseckey_destroy(mctx, &newkey);
		}
	}

	if (isc_log_wouldlog(dns_lctx, ISC_LOG_DEBUG(3))) {
		char namebuf[DNS_NAME_FORMATSIZE];
		dns_name_format(origin, namebuf, sizeof(namebuf));
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DNSSEC,
			      DNS_LOGMODULE_DNSSEC, ISC_LOG_DEBUG(3),
			      "keymgr: %s done", namebuf);
	}
	return (result);
}

static isc_result_t
keymgr_checkds(dns_kasp_t *kasp, dns_dnsseckeylist_t *keyring,
	       const char *directory, isc_stdtime_t now, isc_stdtime_t when,
	       bool dspublish, dns_keytag_t id, unsigned int alg,
	       bool check_id) {
	int options = (DST_TYPE_PRIVATE | DST_TYPE_PUBLIC | DST_TYPE_STATE);
	isc_dir_t dir;
	isc_result_t result;
	dns_dnsseckey_t *ksk_key = NULL;

	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(keyring != NULL);

	for (dns_dnsseckey_t *dkey = ISC_LIST_HEAD(*keyring); dkey != NULL;
	     dkey = ISC_LIST_NEXT(dkey, link))
	{
		isc_result_t ret;
		bool ksk = false;

		ret = dst_key_getbool(dkey->key, DST_BOOL_KSK, &ksk);
		if (ret == ISC_R_SUCCESS && ksk) {
			if (check_id && dst_key_id(dkey->key) != id) {
				continue;
			}
			if (alg > 0 && dst_key_alg(dkey->key) != alg) {
				continue;
			}

			if (ksk_key != NULL) {
				/*
				 * Only checkds for one key at a time.
				 */
				return (DNS_R_TOOMANYKEYS);
			}

			ksk_key = dkey;
		}
	}

	if (ksk_key == NULL) {
		return (DNS_R_NOKEYMATCH);
	}

	if (dspublish) {
		dst_key_settime(ksk_key->key, DST_TIME_DSPUBLISH, when);
	} else {
		dst_key_settime(ksk_key->key, DST_TIME_DSDELETE, when);
	}

	if (isc_log_wouldlog(dns_lctx, ISC_LOG_NOTICE)) {
		char keystr[DST_KEY_FORMATSIZE];
		char timestr[26]; /* Minimal buf as per ctime_r() spec. */

		dst_key_format(ksk_key->key, keystr, sizeof(keystr));
		isc_stdtime_tostring(when, timestr, sizeof(timestr));
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_DNSSEC,
			      DNS_LOGMODULE_DNSSEC, ISC_LOG_NOTICE,
			      "keymgr: checkds DS for key %s seen %s at %s",
			      keystr, dspublish ? "published" : "withdrawn",
			      timestr);
	}

	/* Store key state and update hints. */
	isc_dir_init(&dir);
	if (directory == NULL) {
		directory = ".";
	}
	result = isc_dir_open(&dir, directory);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	dns_dnssec_get_hints(ksk_key, now);
	result = dst_key_tofile(ksk_key->key, options, directory);
	if (result == ISC_R_SUCCESS) {
		dst_key_setmodified(ksk_key->key, false);
	}
	isc_dir_close(&dir);

	return (result);
}

isc_result_t
dns_keymgr_checkds(dns_kasp_t *kasp, dns_dnsseckeylist_t *keyring,
		   const char *directory, isc_stdtime_t now, isc_stdtime_t when,
		   bool dspublish) {
	return (keymgr_checkds(kasp, keyring, directory, now, when, dspublish,
			       0, 0, false));
}

isc_result_t
dns_keymgr_checkds_id(dns_kasp_t *kasp, dns_dnsseckeylist_t *keyring,
		      const char *directory, isc_stdtime_t now,
		      isc_stdtime_t when, bool dspublish, dns_keytag_t id,
		      unsigned int alg) {
	return (keymgr_checkds(kasp, keyring, directory, now, when, dspublish,
			       id, alg, true));
}

static void
keytime_status(dst_key_t *key, isc_stdtime_t now, isc_buffer_t *buf,
	       const char *pre, int ks, int kt) {
	char timestr[26]; /* Minimal buf as per ctime_r() spec. */
	isc_result_t ret;
	isc_stdtime_t when = 0;
	dst_key_state_t state = NA;

	isc_buffer_printf(buf, "%s", pre);
	(void)dst_key_getstate(key, ks, &state);
	ret = dst_key_gettime(key, kt, &when);
	if (state == RUMOURED || state == OMNIPRESENT) {
		isc_buffer_printf(buf, "yes - since ");
	} else if (now < when) {
		isc_buffer_printf(buf, "no  - scheduled ");
	} else {
		isc_buffer_printf(buf, "no\n");
		return;
	}
	if (ret == ISC_R_SUCCESS) {
		isc_stdtime_tostring(when, timestr, sizeof(timestr));
		isc_buffer_printf(buf, "%s\n", timestr);
	}
}

static void
rollover_status(dns_dnsseckey_t *dkey, dns_kasp_t *kasp, isc_stdtime_t now,
		isc_buffer_t *buf, bool zsk) {
	char timestr[26]; /* Minimal buf as per ctime_r() spec. */
	isc_result_t ret = ISC_R_SUCCESS;
	isc_stdtime_t active_time = 0;
	dst_key_state_t state = NA, goal = NA;
	int rrsig, active, retire;
	dst_key_t *key = dkey->key;

	if (zsk) {
		rrsig = DST_KEY_ZRRSIG;
		active = DST_TIME_ACTIVATE;
		retire = DST_TIME_INACTIVE;
	} else {
		rrsig = DST_KEY_KRRSIG;
		active = DST_TIME_PUBLISH;
		retire = DST_TIME_DELETE;
	}

	isc_buffer_printf(buf, "\n");

	(void)dst_key_getstate(key, DST_KEY_GOAL, &goal);
	(void)dst_key_getstate(key, rrsig, &state);
	(void)dst_key_gettime(key, active, &active_time);
	if (active_time == 0) {
		// only interested in keys that were once active.
		return;
	}

	if (goal == HIDDEN && (state == UNRETENTIVE || state == HIDDEN)) {
		isc_stdtime_t remove_time = 0;
		// is the key removed yet?
		state = NA;
		(void)dst_key_getstate(key, DST_KEY_DNSKEY, &state);
		if (state == RUMOURED || state == OMNIPRESENT) {
			ret = dst_key_gettime(key, DST_TIME_DELETE,
					      &remove_time);
			if (ret == ISC_R_SUCCESS) {
				isc_buffer_printf(buf, "  Key is retired, will "
						       "be removed on ");
				isc_stdtime_tostring(remove_time, timestr,
						     sizeof(timestr));
				isc_buffer_printf(buf, "%s", timestr);
			}
		} else {
			isc_buffer_printf(
				buf, "  Key has been removed from the zone");
		}
	} else {
		isc_stdtime_t retire_time = 0;
		uint32_t lifetime = 0;
		(void)dst_key_getnum(key, DST_NUM_LIFETIME, &lifetime);
		ret = dst_key_gettime(key, retire, &retire_time);
		if (ret == ISC_R_SUCCESS) {
			if (now < retire_time) {
				if (goal == OMNIPRESENT) {
					isc_buffer_printf(buf,
							  "  Next rollover "
							  "scheduled on ");
					retire_time = keymgr_prepublication_time(
						dkey, kasp, lifetime, now);
				} else {
					isc_buffer_printf(
						buf, "  Key will retire on ");
				}
			} else {
				isc_buffer_printf(buf,
						  "  Rollover is due since ");
			}
			isc_stdtime_tostring(retire_time, timestr,
					     sizeof(timestr));
			isc_buffer_printf(buf, "%s", timestr);
		} else {
			isc_buffer_printf(buf, "  No rollover scheduled");
		}
	}
	isc_buffer_printf(buf, "\n");
}

static void
keystate_status(dst_key_t *key, isc_buffer_t *buf, const char *pre, int ks) {
	dst_key_state_t state = NA;

	(void)dst_key_getstate(key, ks, &state);
	switch (state) {
	case HIDDEN:
		isc_buffer_printf(buf, "  - %shidden\n", pre);
		break;
	case RUMOURED:
		isc_buffer_printf(buf, "  - %srumoured\n", pre);
		break;
	case OMNIPRESENT:
		isc_buffer_printf(buf, "  - %somnipresent\n", pre);
		break;
	case UNRETENTIVE:
		isc_buffer_printf(buf, "  - %sunretentive\n", pre);
		break;
	case NA:
	default:
		/* print nothing */
		break;
	}
}

void
dns_keymgr_status(dns_kasp_t *kasp, dns_dnsseckeylist_t *keyring,
		  isc_stdtime_t now, char *out, size_t out_len) {
	isc_buffer_t buf;
	char timestr[26]; /* Minimal buf as per ctime_r() spec. */

	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(keyring != NULL);
	REQUIRE(out != NULL);

	isc_buffer_init(&buf, out, out_len);

	// policy name
	isc_buffer_printf(&buf, "dnssec-policy: %s\n", dns_kasp_getname(kasp));
	isc_buffer_printf(&buf, "current time:  ");
	isc_stdtime_tostring(now, timestr, sizeof(timestr));
	isc_buffer_printf(&buf, "%s\n", timestr);

	for (dns_dnsseckey_t *dkey = ISC_LIST_HEAD(*keyring); dkey != NULL;
	     dkey = ISC_LIST_NEXT(dkey, link))
	{
		char algstr[DNS_NAME_FORMATSIZE];
		bool ksk = false, zsk = false;
		isc_result_t ret;

		if (dst_key_is_unused(dkey->key)) {
			continue;
		}

		// key data
		dns_secalg_format((dns_secalg_t)dst_key_alg(dkey->key), algstr,
				  sizeof(algstr));
		isc_buffer_printf(&buf, "\nkey: %d (%s), %s\n",
				  dst_key_id(dkey->key), algstr,
				  keymgr_keyrole(dkey->key));

		// publish status
		keytime_status(dkey->key, now, &buf,
			       "  published:      ", DST_KEY_DNSKEY,
			       DST_TIME_PUBLISH);

		// signing status
		ret = dst_key_getbool(dkey->key, DST_BOOL_KSK, &ksk);
		if (ret == ISC_R_SUCCESS && ksk) {
			keytime_status(dkey->key, now, &buf,
				       "  key signing:    ", DST_KEY_KRRSIG,
				       DST_TIME_PUBLISH);
		}
		ret = dst_key_getbool(dkey->key, DST_BOOL_ZSK, &zsk);
		if (ret == ISC_R_SUCCESS && zsk) {
			keytime_status(dkey->key, now, &buf,
				       "  zone signing:   ", DST_KEY_ZRRSIG,
				       DST_TIME_ACTIVATE);
		}

		// rollover status
		rollover_status(dkey, kasp, now, &buf, zsk);

		// key states
		keystate_status(dkey->key, &buf,
				"goal:           ", DST_KEY_GOAL);
		keystate_status(dkey->key, &buf,
				"dnskey:         ", DST_KEY_DNSKEY);
		keystate_status(dkey->key, &buf,
				"ds:             ", DST_KEY_DS);
		keystate_status(dkey->key, &buf,
				"zone rrsig:     ", DST_KEY_ZRRSIG);
		keystate_status(dkey->key, &buf,
				"key rrsig:      ", DST_KEY_KRRSIG);
	}
}

isc_result_t
dns_keymgr_rollover(dns_kasp_t *kasp, dns_dnsseckeylist_t *keyring,
		    const char *directory, isc_stdtime_t now,
		    isc_stdtime_t when, dns_keytag_t id,
		    unsigned int algorithm) {
	int options = (DST_TYPE_PRIVATE | DST_TYPE_PUBLIC | DST_TYPE_STATE);
	isc_dir_t dir;
	isc_result_t result;
	dns_dnsseckey_t *key = NULL;
	isc_stdtime_t active, retire, prepub;

	REQUIRE(DNS_KASP_VALID(kasp));
	REQUIRE(keyring != NULL);

	for (dns_dnsseckey_t *dkey = ISC_LIST_HEAD(*keyring); dkey != NULL;
	     dkey = ISC_LIST_NEXT(dkey, link))
	{
		if (dst_key_id(dkey->key) != id) {
			continue;
		}
		if (algorithm > 0 && dst_key_alg(dkey->key) != algorithm) {
			continue;
		}
		if (key != NULL) {
			/*
			 * Only rollover for one key at a time.
			 */
			return (DNS_R_TOOMANYKEYS);
		}
		key = dkey;
	}

	if (key == NULL) {
		return (DNS_R_NOKEYMATCH);
	}

	result = dst_key_gettime(key->key, DST_TIME_ACTIVATE, &active);
	if (result != ISC_R_SUCCESS || active > now) {
		return (DNS_R_KEYNOTACTIVE);
	}

	result = dst_key_gettime(key->key, DST_TIME_INACTIVE, &retire);
	if (result != ISC_R_SUCCESS) {
		/**
		 * Default to as if this key was not scheduled to
		 * become retired, as if it had unlimited lifetime.
		 */
		retire = 0;
	}

	/**
	 * Usually when is set to now, which is before the scheduled
	 * prepublication time, meaning we reduce the lifetime of the
	 * key. But in some cases, the lifetime can also be extended.
	 * We accept it, but we can return an error here if that
	 * turns out to be unintuitive behavior.
	 */
	prepub = dst_key_getttl(key->key) + dns_kasp_publishsafety(kasp) +
		 dns_kasp_zonepropagationdelay(kasp);
	retire = when + prepub;

	dst_key_settime(key->key, DST_TIME_INACTIVE, retire);
	dst_key_setnum(key->key, DST_NUM_LIFETIME, (retire - active));

	/* Store key state and update hints. */
	isc_dir_init(&dir);
	if (directory == NULL) {
		directory = ".";
	}
	result = isc_dir_open(&dir, directory);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	dns_dnssec_get_hints(key, now);
	result = dst_key_tofile(key->key, options, directory);
	if (result == ISC_R_SUCCESS) {
		dst_key_setmodified(key->key, false);
	}
	isc_dir_close(&dir);

	return (result);
}
