/*	$NetBSD: keymgr.c,v 1.3 2020/08/03 17:23:41 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

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
	} while (/*CONSTCOND*/0)

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
	} while (/*CONSTCOND*/0)

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
	bool ksk, zsk;
	dst_key_getbool(key, DST_BOOL_KSK, &ksk);
	dst_key_getbool(key, DST_BOOL_ZSK, &zsk);
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
	if (zsk && ksk) {
		ksk_remove += dns_kasp_parentregistrationdelay(kasp);
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

	(void)dst_key_getbool(key->key, DST_BOOL_ZSK, &zsk);
	if (!zsk && ksk) {
		/*
		 * Include registration delay in prepublication time.
		 */
		prepub += dns_kasp_parentregistrationdelay(kasp);
	}

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
	bool ksk, zsk;

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

	(void)dst_key_getbool(key->key, DST_BOOL_KSK, &ksk);
	if (ksk) {
		if (dst_key_getstate(key->key, DST_KEY_KRRSIG, &s) !=
		    ISC_R_SUCCESS) {
			dst_key_setstate(key->key, DST_KEY_KRRSIG, OMNIPRESENT);
			dst_key_settime(key->key, DST_TIME_KRRSIG, now);
		}
		if (dst_key_getstate(key->key, DST_KEY_DS, &s) != ISC_R_SUCCESS)
		{
			dst_key_setstate(key->key, DST_KEY_DS, OMNIPRESENT);
			dst_key_settime(key->key, DST_TIME_DS, now);
		}
	}
	(void)dst_key_getbool(key->key, DST_BOOL_ZSK, &zsk);
	if (zsk) {
		if (dst_key_getstate(key->key, DST_KEY_ZRRSIG, &s) !=
		    ISC_R_SUCCESS) {
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

/*
 * Create a new key for 'origin' given the kasp key configuration 'kkey'.
 * This will check for key id collisions with keys in 'keylist'.
 * The created key will be stored in 'dst_key'.
 *
 */
static isc_result_t
keymgr_createkey(dns_kasp_key_t *kkey, const dns_name_t *origin,
		 dns_rdataclass_t rdclass, isc_mem_t *mctx,
		 dns_dnsseckeylist_t *keylist, dst_key_t **dst_key) {
	bool conflict;
	int keyflags = DNS_KEYOWNER_ZONE;
	isc_result_t result = ISC_R_SUCCESS;
	dst_key_t *newkey = NULL;

	do {
		uint16_t id;
		uint32_t rid;
		uint32_t algo = dns_kasp_key_algorithm(kkey);
		int size = dns_kasp_key_size(kkey);

		conflict = false;

		if (dns_kasp_key_ksk(kkey)) {
			keyflags |= DNS_KEYFLAG_KSK;
		}
		RETERR(dst_key_generate(origin, algo, size, 0, keyflags,
					DNS_KEYPROTO_DNSSEC, rdclass, mctx,
					&newkey, NULL));

		/* Key collision? */
		id = dst_key_id(newkey);
		rid = dst_key_rid(newkey);
		for (dns_dnsseckey_t *dkey = ISC_LIST_HEAD(*keylist);
		     dkey != NULL; dkey = ISC_LIST_NEXT(dkey, link))
		{
			if (dst_key_alg(dkey->key) != algo) {
				continue;
			}
			if (dst_key_id(dkey->key) == id ||
			    dst_key_rid(dkey->key) == id ||
			    dst_key_id(dkey->key) == rid ||
			    dst_key_rid(dkey->key) == rid)
			{
				/* Try again. */
				conflict = true;
				isc_log_write(dns_lctx, DNS_LOGCATEGORY_DNSSEC,
					      DNS_LOGMODULE_DNSSEC,
					      ISC_LOG_WARNING,
					      "keymgr: key collision id %d",
					      dst_key_id(newkey));
				dst_key_free(&newkey);
			}
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
		       dst_key_state_t next_state, dst_key_state_t states[4]) {
	REQUIRE(key != NULL);

	for (int i = 0; i < 4; i++) {
		dst_key_state_t state;
		if (states[i] == NA) {
			continue;
		}
		if (next_state != NA && i == type &&
		    dst_key_id(key) == dst_key_id(subject)) {
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
 * Check if a 'k2' is a successor of 'k1'. This is a simplified version of
 * Equation(2) of "Flexible and Robust Key Rollover" which defines a
 * recursive relation.
 *
 */
static bool
keymgr_key_is_successor(dst_key_t *k1, dst_key_t *k2) {
	uint32_t suc = 0, pre = 0;
	if (dst_key_getnum(k1, DST_NUM_SUCCESSOR, &suc) != ISC_R_SUCCESS) {
		return (false);
	}
	if (dst_key_getnum(k2, DST_NUM_PREDECESSOR, &pre) != ISC_R_SUCCESS) {
		return (false);
	}
	return (dst_key_id(k1) == pre && dst_key_id(k2) == suc);
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
			     dst_key_state_t states[4],
			     dst_key_state_t states2[4], bool check_successor,
			     bool match_algorithms) {
	for (dns_dnsseckey_t *dkey = ISC_LIST_HEAD(*keyring); dkey != NULL;
	     dkey = ISC_LIST_NEXT(dkey, link))
	{
		if (match_algorithms &&
		    (dst_key_alg(dkey->key) != dst_key_alg(key->key))) {
			continue;
		}

		if (check_successor &&
		    keymgr_key_match_state(dkey->key, key->key, type,
					   next_state, states2))
		{
			/* Found a possible successor, look for predecessor. */
			for (dns_dnsseckey_t *pkey = ISC_LIST_HEAD(*keyring);
			     pkey != NULL; pkey = ISC_LIST_NEXT(pkey, link))
			{
				if (pkey == dkey) {
					continue;
				}
				if (!keymgr_key_match_state(pkey->key, key->key,
							    type, next_state,
							    states)) {
					continue;
				}

				/*
				 * Found a possible predecessor, check
				 * relationship.
				 */
				if (keymgr_key_is_successor(pkey->key,
							    dkey->key)) {
					return (true);
				}
			}
		}

		if (!check_successor &&
		    keymgr_key_match_state(dkey->key, key->key, type,
					   next_state, states))
		{
			return (true);
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
		if (keymgr_key_is_successor(predecessor->key, successor->key)) {
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
	dst_key_state_t dnskey_omnipresent[4] = { OMNIPRESENT, NA, OMNIPRESENT,
						  NA };	       /* (3e) */
	dst_key_state_t ds_hidden[4] = { NA, NA, NA, HIDDEN }; /* (3e) */
	dst_key_state_t na[4] = { NA, NA, NA, NA }; /* successor n/a */

	for (dns_dnsseckey_t *dkey = ISC_LIST_HEAD(*keyring); dkey != NULL;
	     dkey = ISC_LIST_NEXT(dkey, link))
	{
		char keystr[DST_KEY_FORMATSIZE];
		dst_key_format(dkey->key, keystr, sizeof(keystr));

		if (match_algorithms &&
		    (dst_key_alg(dkey->key) != dst_key_alg(key->key))) {
			continue;
		}

		if (keymgr_key_match_state(dkey->key, key->key, type,
					   next_state, ds_hidden)) {
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
		dnskey_omnipresent[DST_KEY_DS] = NA;
		if (next_state != NA &&
		    dst_key_id(dkey->key) == dst_key_id(key->key)) {
			/* Check next state rather than current state. */
			dnskey_omnipresent[DST_KEY_DS] = next_state;
		} else {
			(void)dst_key_getstate(dkey->key, DST_KEY_DS,
					       &dnskey_omnipresent[DST_KEY_DS]);
		}
		if (!keymgr_key_exists_with_state(
			    keyring, key, type, next_state, dnskey_omnipresent,
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
	dst_key_state_t rrsig_omnipresent[4] = { NA, OMNIPRESENT, NA,
						 NA };		   /* (3i) */
	dst_key_state_t dnskey_hidden[4] = { HIDDEN, NA, NA, NA }; /* (3i) */
	dst_key_state_t na[4] = { NA, NA, NA, NA }; /* successor n/a */

	for (dns_dnsseckey_t *dkey = ISC_LIST_HEAD(*keyring); dkey != NULL;
	     dkey = ISC_LIST_NEXT(dkey, link))
	{
		if (match_algorithms &&
		    (dst_key_alg(dkey->key) != dst_key_alg(key->key))) {
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
		rrsig_omnipresent[DST_KEY_DNSKEY] = NA;
		(void)dst_key_getstate(dkey->key, DST_KEY_DNSKEY,
				       &rrsig_omnipresent[DST_KEY_DNSKEY]);
		if (!keymgr_key_exists_with_state(keyring, key, type,
						  next_state, rrsig_omnipresent,
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
 * Check for existence of DS.
 *
 */
static bool
keymgr_have_ds(dns_dnsseckeylist_t *keyring, dns_dnsseckey_t *key, int type,
	       dst_key_state_t next_state) {
	dst_key_state_t states[2][4] = {
		/* DNSKEY, ZRRSIG, KRRSIG, DS */
		{ NA, NA, NA, OMNIPRESENT }, /* DS present */
		{ NA, NA, NA, RUMOURED }     /* DS introducing */
	};
	dst_key_state_t na[4] = { NA, NA, NA, NA }; /* successor n/a */

	/*
	 * Equation (3a):
	 * There is a key with the DS in either RUMOURD or OMNIPRESENT state.
	 */
	return (keymgr_key_exists_with_state(keyring, key, type, next_state,
					     states[0], na, false, false) ||
		keymgr_key_exists_with_state(keyring, key, type, next_state,
					     states[1], na, false, false));
}

/*
 * Check for existence of DNSKEY, or at least a good DNSKEY state.
 * See equations what are good DNSKEY states.
 *
 */
static bool
keymgr_have_dnskey(dns_dnsseckeylist_t *keyring, dns_dnsseckey_t *key, int type,
		   dst_key_state_t next_state) {
	dst_key_state_t states[9][4] = {
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
	dst_key_state_t na[4] = { NA, NA, NA, NA }; /* successor n/a */

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
	dst_key_state_t states[11][4] = {
		/* DNSKEY,     ZRRSIG,      KRRSIG, DS */
		{ OMNIPRESENT, OMNIPRESENT, NA, NA }, /* (3f) */
		{ UNRETENTIVE, OMNIPRESENT, NA, NA }, /* (3g)p */
		{ RUMOURED, OMNIPRESENT, NA, NA },    /* (3g)s */
		{ OMNIPRESENT, UNRETENTIVE, NA, NA }, /* (3h)p */
		{ OMNIPRESENT, RUMOURED, NA, NA },    /* (3h)s */
	};
	dst_key_state_t na[4] = { NA, NA, NA, NA }; /* successor n/a */

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
 *
 */
static bool
keymgr_policy_approval(dns_dnsseckeylist_t *keyring, dns_dnsseckey_t *key,
		       int type, dst_key_state_t next) {
	dst_key_state_t dnskeystate = HIDDEN;
	dst_key_state_t ksk_present[4] = { OMNIPRESENT, NA, OMNIPRESENT,
					   OMNIPRESENT };
	dst_key_state_t ds_rumoured[4] = { OMNIPRESENT, NA, OMNIPRESENT,
					   RUMOURED };
	dst_key_state_t ds_retired[4] = { OMNIPRESENT, NA, OMNIPRESENT,
					  UNRETENTIVE };
	dst_key_state_t ksk_rumoured[4] = { RUMOURED, NA, NA, OMNIPRESENT };
	dst_key_state_t ksk_retired[4] = { UNRETENTIVE, NA, NA, OMNIPRESENT };
	dst_key_state_t na[4] = { NA, NA, NA, NA }; /* successor n/a */

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
			  int type, dst_key_state_t next_state) {
	/* Debug logging. */
	if (isc_log_wouldlog(dns_lctx, ISC_LOG_DEBUG(1))) {
		bool rule1a, rule1b, rule2a, rule2b, rule3a, rule3b;
		char keystr[DST_KEY_FORMATSIZE];
		dst_key_format(key->key, keystr, sizeof(keystr));
		rule1a = keymgr_have_ds(keyring, key, type, NA);
		rule1b = keymgr_have_ds(keyring, key, type, next_state);
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
		(!keymgr_have_ds(keyring, key, type, NA) ||
		 keymgr_have_ds(keyring, key, type, next_state)) &&
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
	isc_stdtime_t lastchange, nexttime = now;

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
		case OMNIPRESENT:
		case HIDDEN:
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
			 * So we need to wait Dreg + Iret before the DS becomes
			 * OMNIPRESENT. This translates to:
			 *
			 *      parent-registration-delay +
			 *      parent-propagation-delay + parent-ds-ttl.
			 *
			 * We will also add the retire-safety interval.
			 */
			nexttime = lastchange + dns_kasp_dsttl(kasp) +
				   dns_kasp_parentregistrationdelay(kasp) +
				   dns_kasp_parentpropagationdelay(kasp) +
				   dns_kasp_retiresafety(kasp);
			break;
		default:
			nexttime = now;
			break;
		}
		break;
	default:
		INSIST(0);
		ISC_UNREACHABLE();
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
	      isc_stdtime_t *nexttime) {
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
						    next_state)) {
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
						       next_state)) {
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
keymgr_key_init(dns_dnsseckey_t *key, dns_kasp_t *kasp, isc_stdtime_t now) {
	bool ksk, zsk;
	isc_result_t ret;
	isc_stdtime_t active = 0, pub = 0, syncpub = 0;
	dst_key_state_t dnskey_state = HIDDEN;
	dst_key_state_t ds_state = HIDDEN;
	dst_key_state_t zrrsig_state = HIDDEN;

	REQUIRE(key != NULL);
	REQUIRE(key->key != NULL);

	/* Initialize role. */
	ret = dst_key_getbool(key->key, DST_BOOL_KSK, &ksk);
	if (ret != ISC_R_SUCCESS) {
		ksk = ((dst_key_flags(key->key) & DNS_KEYFLAG_KSK) != 0);
		dst_key_setbool(key->key, DST_BOOL_KSK, ksk);
	}
	ret = dst_key_getbool(key->key, DST_BOOL_ZSK, &zsk);
	if (ret != ISC_R_SUCCESS) {
		zsk = ((dst_key_flags(key->key) & DNS_KEYFLAG_KSK) == 0);
		dst_key_setbool(key->key, DST_BOOL_ZSK, zsk);
	}

	/* Get time metadata. */
	ret = dst_key_gettime(key->key, DST_TIME_ACTIVATE, &active);
	if (active <= now && ret == ISC_R_SUCCESS) {
		dns_ttl_t key_ttl = dst_key_getttl(key->key);
		key_ttl += dns_kasp_zonepropagationdelay(kasp);
		if ((active + key_ttl) <= now) {
			dnskey_state = OMNIPRESENT;
		} else {
			dnskey_state = RUMOURED;
		}
	}
	ret = dst_key_gettime(key->key, DST_TIME_PUBLISH, &pub);
	if (pub <= now && ret == ISC_R_SUCCESS) {
		dns_ttl_t zone_ttl = dns_kasp_zonemaxttl(kasp);
		zone_ttl += dns_kasp_zonepropagationdelay(kasp);
		if ((pub + zone_ttl) <= now) {
			zrrsig_state = OMNIPRESENT;
		} else {
			zrrsig_state = RUMOURED;
		}
	}
	ret = dst_key_gettime(key->key, DST_TIME_SYNCPUBLISH, &syncpub);
	if (syncpub <= now && ret == ISC_R_SUCCESS) {
		dns_ttl_t ds_ttl = dns_kasp_dsttl(kasp);
		ds_ttl += dns_kasp_parentregistrationdelay(kasp);
		ds_ttl += dns_kasp_parentpropagationdelay(kasp);
		if ((syncpub + ds_ttl) <= now) {
			ds_state = OMNIPRESENT;
		} else {
			ds_state = RUMOURED;
		}
	}

	/* Set key states for all keys that do not have them. */
	INITIALIZE_STATE(key->key, DST_KEY_DNSKEY, DST_TIME_DNSKEY,
			 dnskey_state, now);
	if (ksk) {
		INITIALIZE_STATE(key->key, DST_KEY_KRRSIG, DST_TIME_KRRSIG,
				 dnskey_state, now);
		INITIALIZE_STATE(key->key, DST_KEY_DS, DST_TIME_DS, ds_state,
				 now);
	}
	if (zsk) {
		INITIALIZE_STATE(key->key, DST_KEY_ZRRSIG, DST_TIME_ZRRSIG,
				 zrrsig_state, now);
	}
}

static isc_result_t
keymgr_key_rollover(dns_kasp_key_t *kaspkey, dns_dnsseckey_t *active_key,
		    dns_dnsseckeylist_t *keyring, dns_dnsseckeylist_t *newkeys,
		    const dns_name_t *origin, dns_rdataclass_t rdclass,
		    dns_kasp_t *kasp, uint32_t lifetime, isc_stdtime_t now,
		    isc_stdtime_t *nexttime, isc_mem_t *mctx) {
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
		isc_result_t result = keymgr_createkey(kaspkey, origin, rdclass,
						       mctx, keyring, &dst_key);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
		dst_key_setttl(dst_key, dns_kasp_dnskeyttl(kasp));
		dst_key_settime(dst_key, DST_TIME_CREATED, now);
		result = dns_dnsseckey_create(mctx, &dst_key, &new_key);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
		keymgr_key_init(new_key, kasp, now);
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

/*
 * Examine 'keys' and match 'kasp' policy.
 *
 */
isc_result_t
dns_keymgr_run(const dns_name_t *origin, dns_rdataclass_t rdclass,
	       const char *directory, isc_mem_t *mctx,
	       dns_dnsseckeylist_t *keyring, dns_kasp_t *kasp,
	       isc_stdtime_t now, isc_stdtime_t *nexttime) {
	isc_result_t result = ISC_R_SUCCESS;
	dns_dnsseckeylist_t newkeys;
	dns_kasp_key_t *kkey;
	dns_dnsseckey_t *newkey = NULL;
	isc_dir_t dir;
	bool dir_open = false;
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
				      "keymgr: keyring: dnskey %s (policy %s)",
				      keystr, dns_kasp_getname(kasp));
		}
	}

	/* Do we need to remove keys? */
	for (dns_dnsseckey_t *dkey = ISC_LIST_HEAD(*keyring); dkey != NULL;
	     dkey = ISC_LIST_NEXT(dkey, link))
	{
		bool found_match = false;

		keymgr_key_init(dkey, kasp, now);

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
	}

	/* Create keys according to the policy, if come in short. */
	for (kkey = ISC_LIST_HEAD(dns_kasp_keys(kasp)); kkey != NULL;
	     kkey = ISC_LIST_NEXT(kkey, link))
	{
		uint32_t lifetime = dns_kasp_key_lifetime(kkey);
		dns_dnsseckey_t *active_key = NULL;

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
						   &l) != ISC_R_SUCCESS) {
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
					    !keymgr_key_is_successor(
						    dkey->key,
						    active_key->key) &&
					    !keymgr_key_is_successor(
						    active_key->key, dkey->key))
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
				 * This is possibly an active key created
				 * outside dnssec-policy.  Initialize goal,
				 * if not set.
				 */
				dst_key_state_t goal;
				if (dst_key_getstate(dkey->key, DST_KEY_GOAL,
						     &goal) != ISC_R_SUCCESS) {
					dst_key_setstate(dkey->key,
							 DST_KEY_GOAL,
							 OMNIPRESENT);
				}

				/*
				 * Save the matched key only if it is active
				 * or desires to be active.
				 */
				if (dst_key_goal(dkey->key) == OMNIPRESENT ||
				    dst_key_is_active(dkey->key, now)) {
					active_key = dkey;
				}
			}
		}

		/* See if this key requires a rollover. */
		RETERR(keymgr_key_rollover(kkey, active_key, keyring, &newkeys,
					   origin, rdclass, kasp, lifetime, now,
					   nexttime, mctx));
	}

	/* Walked all kasp key configurations.  Append new keys. */
	if (!ISC_LIST_EMPTY(newkeys)) {
		ISC_LIST_APPENDLIST(*keyring, newkeys, link);
	}

	/* Read to update key states. */
	keymgr_update(keyring, kasp, now, nexttime);

	/* Store key states and update hints. */
	for (dns_dnsseckey_t *dkey = ISC_LIST_HEAD(*keyring); dkey != NULL;
	     dkey = ISC_LIST_NEXT(dkey, link))
	{
		dns_dnssec_get_hints(dkey, now);
		RETERR(dst_key_tofile(dkey->key, options, directory));
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

	return (result);
}

static void
keytime_status(dst_key_t *key, isc_stdtime_t now, isc_buffer_t *buf,
	       const char *pre, int ks, int kt) {
	char timestr[26]; /* Minimal buf as per ctime_r() spec. */
	isc_result_t ret;
	isc_stdtime_t when = 0;
	dst_key_state_t state;

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

		if (dst_key_is_unused(dkey->key)) {
			continue;
		}

		// key data
		dst_key_getbool(dkey->key, DST_BOOL_KSK, &ksk);
		dst_key_getbool(dkey->key, DST_BOOL_ZSK, &zsk);
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
		if (ksk) {
			keytime_status(dkey->key, now, &buf,
				       "  key signing:    ", DST_KEY_KRRSIG,
				       DST_TIME_PUBLISH);
		}
		if (zsk) {
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
