/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Alistair Crooks (agc@NetBSD.org)
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
/*
 * Copyright (c) 2005-2008 Nominet UK (www.nic.uk)
 * All rights reserved.
 * Contributors: Ben Laurie, Rachel Willmer. The Contributors have asserted
 * their moral rights under the UK Copyright Design and Patents Act 1988 to
 * be recorded as the authors of this copyright work.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.
 *
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/** \file
 */
#include "config.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/mman.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_OPENSSL_CAST_H
#include <openssl/cast.h>
#endif

#include "create.h"
#include "keyring.h"
#include "packet.h"
#include "signature.h"
#include "writer.h"
#include "readerwriter.h"
#include "keyring_local.h"
#include "loccreate.h"
#include "memory.h"
#include "netpgpdefs.h"

/**
 * \ingroup Core_Create
 * \param length
 * \param type
 * \param info
 * \return true if OK, otherwise false
 */

bool 
__ops_write_ss_header(unsigned length, __ops_content_tag_t type,
		    __ops_createinfo_t *info)
{
	return __ops_write_length(length, info) &&
		__ops_write_scalar((unsigned)(type -
			OPS_PTAG_SIGNATURE_SUBPACKET_BASE), 1, info);
}

/*
 * XXX: the general idea of _fast_ is that it doesn't copy stuff the safe
 * (i.e. non _fast_) version will, and so will also need to be freed.
 */

/**
 * \ingroup Core_Create
 *
 * __ops_fast_create_user_id() sets id->user_id to the given user_id.
 * This is fast because it is only copying a char*. However, if user_id
 * is changed or freed in the future, this could have injurious results.
 * \param id
 * \param user_id
 */

void 
__ops_fast_create_user_id(__ops_user_id_t * id, unsigned char *user_id)
{
	id->user_id = user_id;
}

/**
 * \ingroup Core_WritePackets
 * \brief Writes a User Id packet
 * \param id
 * \param info
 * \return true if OK, otherwise false
 */
bool 
__ops_write_struct_user_id(__ops_user_id_t *id,
			 __ops_createinfo_t *info)
{
	return __ops_write_ptag(OPS_PTAG_CT_USER_ID, info) &&
		__ops_write_length(strlen((char *) id->user_id), info) &&
		__ops_write(id->user_id, strlen((char *) id->user_id), info);
}

/**
 * \ingroup Core_WritePackets
 * \brief Write a User Id packet.
 * \param user_id
 * \param info
 *
 * \return return value from __ops_write_struct_user_id()
 */
bool 
__ops_write_user_id(const unsigned char *user_id, __ops_createinfo_t *info)
{
	__ops_user_id_t   id;

	id.user_id = __UNCONST(user_id);
	return __ops_write_struct_user_id(&id, info);
}

/**
\ingroup Core_MPI
*/
static unsigned 
mpi_length(const BIGNUM *bn)
{
	return 2 + (BN_num_bits(bn) + 7) / 8;
}

static unsigned 
pubkey_length(const __ops_pubkey_t *key)
{
	switch (key->alg) {
	case OPS_PKA_RSA:
		return mpi_length(key->key.rsa.n) + mpi_length(key->key.rsa.e);

	default:
		(void) fprintf(stderr,
			"pubkey_length: unknown key algorithm\n");
	}
	return 0;
}

static unsigned 
seckey_length(const __ops_seckey_t *key)
{
	int             len;

	len = 0;
	switch (key->pubkey.alg) {
	case OPS_PKA_RSA:
		len = mpi_length(key->key.rsa.d) + mpi_length(key->key.rsa.p) +
			mpi_length(key->key.rsa.q) + mpi_length(key->key.rsa.u);

		return len + pubkey_length(&key->pubkey);
	default:
		(void) fprintf(stderr,
			"pubkey_length: unknown key algorithm\n");
	}
	return 0;
}

/**
 * \ingroup Core_Create
 * \param key
 * \param t
 * \param n
 * \param e
*/
void 
__ops_fast_create_rsa_pubkey(__ops_pubkey_t *key, time_t t,
			       BIGNUM *n, BIGNUM *e)
{
	key->version = 4;
	key->birthtime = t;
	key->alg = OPS_PKA_RSA;
	key->key.rsa.n = n;
	key->key.rsa.e = e;
}

/*
 * Note that we support v3 keys here because they're needed for for
 * verification - the writer doesn't allow them, though
 */
static bool 
write_pubkey_body(const __ops_pubkey_t * key,
		      __ops_createinfo_t * info)
{
	if (!(__ops_write_scalar((unsigned)key->version, 1, info) &&
	      __ops_write_scalar((unsigned)key->birthtime, 4, info))) {
		return false;
	}

	if (key->version != 4 &&
	    !__ops_write_scalar(key->days_valid, 2, info)) {
		return false;
	}

	if (!__ops_write_scalar((unsigned)key->alg, 1, info)) {
		return false;
	}

	switch (key->alg) {
	case OPS_PKA_DSA:
		return __ops_write_mpi(key->key.dsa.p, info) &&
			__ops_write_mpi(key->key.dsa.q, info) &&
			__ops_write_mpi(key->key.dsa.g, info) &&
			__ops_write_mpi(key->key.dsa.y, info);

	case OPS_PKA_RSA:
	case OPS_PKA_RSA_ENCRYPT_ONLY:
	case OPS_PKA_RSA_SIGN_ONLY:
		return __ops_write_mpi(key->key.rsa.n, info) &&
			__ops_write_mpi(key->key.rsa.e, info);

	case OPS_PKA_ELGAMAL:
		return __ops_write_mpi(key->key.elgamal.p, info) &&
			__ops_write_mpi(key->key.elgamal.g, info) &&
			__ops_write_mpi(key->key.elgamal.y, info);

	default:
		(void) fprintf(stderr,
			"write_pubkey_body: bad algorithm\n");
		break;
	}
	return false;
}

/*
 * Note that we support v3 keys here because they're needed for for
 * verification - the writer doesn't allow them, though
 */
static bool 
write_seckey_body(const __ops_seckey_t * key,
		      const unsigned char *passphrase,
		      const size_t pplen,
		      __ops_createinfo_t * info)
{
	/* RFC4880 Section 5.5.3 Secret-Key Packet Formats */

	__ops_crypt_t     crypted;
	__ops_hash_t      hash;
	unsigned char   hashed[OPS_SHA1_HASH_SIZE];
	unsigned char   sesskey[CAST_KEY_LENGTH];
	unsigned int    done = 0;
	unsigned int    i = 0;

	if (!write_pubkey_body(&key->pubkey, info)) {
		return false;
	}
	if (key->s2k_usage != OPS_S2KU_ENCRYPTED_AND_HASHED) {
		(void) fprintf(stderr, "write_seckey_body: s2k usage\n");
		return false;
	}
	if (!__ops_write_scalar((unsigned)key->s2k_usage, 1, info)) {
		return false;
	}

	if (key->alg != OPS_SA_CAST5) {
		(void) fprintf(stderr, "write_seckey_body: algorithm\n");
		return false;
	}
	if (!__ops_write_scalar((unsigned)key->alg, 1, info)) {
		return false;
	}

	if (key->s2k_specifier != OPS_S2KS_SIMPLE &&
	    key->s2k_specifier != OPS_S2KS_SALTED) {
		/* = 1 \todo could also be iterated-and-salted */
		(void) fprintf(stderr, "write_seckey_body: s2k spec\n");
		return false;
	}
	if (!__ops_write_scalar((unsigned)key->s2k_specifier, 1, info)) {
		return false;
	}

	if (key->hash_alg != OPS_HASH_SHA1) {
		(void) fprintf(stderr, "write_seckey_body: hash alg\n");
		return false;
	}
	if (!__ops_write_scalar((unsigned)key->hash_alg, 1, info)) {
		return false;
	}

	switch (key->s2k_specifier) {
	case OPS_S2KS_SIMPLE:
		/* nothing more to do */
		break;

	case OPS_S2KS_SALTED:
		/* 8-octet salt value */
		__ops_random(__UNCONST(&key->salt[0]), OPS_SALT_SIZE);
		if (!__ops_write(key->salt, OPS_SALT_SIZE, info)) {
			return false;
		}
		break;

		/*
		 * \todo case OPS_S2KS_ITERATED_AND_SALTED: // 8-octet salt
		 * value // 1-octet count break;
		 */

	default:
		(void) fprintf(stderr,
			"invalid/unsupported s2k specifier %d\n",
			key->s2k_specifier);
		return false;
	}

	if (!__ops_write(&key->iv[0], __ops_block_size(key->alg), info)) {
		return false;
	}

	/*
	 * create the session key for encrypting the algorithm-specific
	 * fields
	 */

	switch (key->s2k_specifier) {
	case OPS_S2KS_SIMPLE:
	case OPS_S2KS_SALTED:
		/* RFC4880: section 3.7.1.1 and 3.7.1.2 */

		done = 0;
		for (i = 0; done < CAST_KEY_LENGTH; i++) {
			unsigned int    j = 0;
			unsigned char   zero = 0;
			int             needed;
			int             use;

			needed = CAST_KEY_LENGTH - done;
			use = MIN(needed, OPS_SHA1_HASH_SIZE);

			__ops_hash_any(&hash, key->hash_alg);
			hash.init(&hash);

			/* preload if iterating  */
			for (j = 0; j < i; j++) {
				/*
				 * Coverity shows a DEADCODE error on this
				 * line. This is expected since the hardcoded
				 * use of SHA1 and CAST5 means that it will
				 * not used. This will change however when
				 * other algorithms are supported.
				 */
				hash.add(&hash, &zero, 1);
			}

			if (key->s2k_specifier == OPS_S2KS_SALTED) {
				hash.add(&hash, key->salt, OPS_SALT_SIZE);
			}
			hash.add(&hash, passphrase, pplen);
			hash.finish(&hash, hashed);

			/*
			 * if more in hash than is needed by session key, use
			 * the leftmost octets
			 */
			(void) memcpy(sesskey + (i * OPS_SHA1_HASH_SIZE),
					hashed, (unsigned)use);
			done += use;
			if (done > CAST_KEY_LENGTH) {
				(void) fprintf(stderr,
					"write_seckey_body: short add\n");
				return false;
			}
		}

		break;

		/*
		 * \todo case OPS_S2KS_ITERATED_AND_SALTED: * 8-octet salt
		 * value * 1-octet count break;
		 */

	default:
		(void) fprintf(stderr,
			"invalid/unsupported s2k specifier %d\n",
			key->s2k_specifier);
		return false;
	}

	/* use this session key to encrypt */

	__ops_crypt_any(&crypted, key->alg);
	crypted.set_iv(&crypted, key->iv);
	crypted.set_key(&crypted, sesskey);
	__ops_encrypt_init(&crypted);

	if (__ops_get_debug_level(__FILE__)) {
		unsigned int    i2 = 0;

		(void) fprintf(stderr, "\nWRITING:\niv=");
		for (i2 = 0; i2 < __ops_block_size(key->alg); i2++) {
			(void) fprintf(stderr, "%02x ", key->iv[i2]);
		}
		(void) fprintf(stderr, "\n");

		(void) fprintf(stderr, "key=");
		for (i2 = 0; i2 < CAST_KEY_LENGTH; i2++) {
			(void) fprintf(stderr, "%02x ", sesskey[i2]);
		}
		(void) fprintf(stderr, "\n");

		(void) fprintf(stderr, "turning encryption on...\n");
	}
	__ops_writer_push_encrypt_crypt(info, &crypted);

	switch (key->pubkey.alg) {
		/* case OPS_PKA_DSA: */
		/* return __ops_write_mpi(key->key.dsa.x,info); */

	case OPS_PKA_RSA:
	case OPS_PKA_RSA_ENCRYPT_ONLY:
	case OPS_PKA_RSA_SIGN_ONLY:

		if (!__ops_write_mpi(key->key.rsa.d, info) ||
		    !__ops_write_mpi(key->key.rsa.p, info) ||
		    !__ops_write_mpi(key->key.rsa.q, info) ||
		    !__ops_write_mpi(key->key.rsa.u, info)) {
			if (__ops_get_debug_level(__FILE__)) {
				(void) fprintf(stderr,
					"4 x mpi not written - problem\n");
			}
			return false;
		}
		break;

		/* case OPS_PKA_ELGAMAL: */
		/* return __ops_write_mpi(key->key.elgamal.x,info); */

	default:
		return false;
	}

	if (!__ops_write(key->checkhash, OPS_CHECKHASH_SIZE, info)) {
		return false;
	}

	__ops_writer_pop(info);

	return true;
}


/**
   \ingroup HighLevel_KeyWrite

   \brief Writes a transferable PGP public key to the given output stream.

   \param keydata Key to be written
   \param armoured Flag is set for armoured output
   \param info Output stream

*/

bool 
__ops_write_transferable_pubkey(const __ops_keydata_t * keydata, bool armoured, __ops_createinfo_t * info)
{
	bool   rtn;
	unsigned int    i = 0, j = 0;

	if (armoured) {
		__ops_writer_push_armoured(info, OPS_PGP_PUBLIC_KEY_BLOCK);
	}
	/* public key */
	rtn = __ops_write_struct_pubkey(&keydata->key.seckey.pubkey, info);
	if (rtn != true) {
		return rtn;
	}

	/* TODO: revocation signatures go here */

	/* user ids and corresponding signatures */
	for (i = 0; i < keydata->nuids; i++) {
		__ops_user_id_t  *uid = &keydata->uids[i];

		rtn = __ops_write_struct_user_id(uid, info);
		if (!rtn) {
			return rtn;
		}

		/* find signature for this packet if it exists */
		for (j = 0; j < keydata->nsigs; j++) {
			sigpacket_t    *sig = &keydata->sigs[i];

			if (strcmp((char *) sig->userid->user_id,
					(char *) uid->user_id) == 0) {
				rtn = __ops_write(sig->packet->raw,
						sig->packet->length, info);
				if (!rtn) {
					return !rtn;
				}
			}
		}
	}

	/* TODO: user attributes and corresponding signatures */

	/*
	 * subkey packets and corresponding signatures and optional
	 * revocation
	 */

	if (armoured) {
		writer_info_finalise(&info->errors, &info->winfo);
		__ops_writer_pop(info);
	}
	return rtn;
}

/**
   \ingroup HighLevel_KeyWrite

   \brief Writes a transferable PGP secret key to the given output stream.

   \param keydata Key to be written
   \param passphrase
   \param pplen
   \param armoured Flag is set for armoured output
   \param info Output stream

*/

bool 
__ops_write_transferable_seckey(const __ops_keydata_t *keydata,
	const unsigned char *passphrase, const size_t pplen,
	bool armoured, __ops_createinfo_t * info)
{
	unsigned	i = 0, j = 0;
	bool		rtn;

	if (armoured) {
		__ops_writer_push_armoured(info, OPS_PGP_PRIVATE_KEY_BLOCK);
	}
	/* public key */
	rtn = __ops_write_struct_seckey(&keydata->key.seckey, passphrase,
			pplen, info);
	if (rtn != true) {
		return rtn;
	}

	/* TODO: revocation signatures go here */

	/* user ids and corresponding signatures */
	for (i = 0; i < keydata->nuids; i++) {
		__ops_user_id_t  *uid = &keydata->uids[i];

		rtn = __ops_write_struct_user_id(uid, info);
		if (!rtn) {
			return rtn;
		}

		/* find signature for this packet if it exists */
		for (j = 0; j < keydata->nsigs; j++) {
			sigpacket_t    *sig = &keydata->sigs[i];

			if (strcmp((char *) sig->userid->user_id,
					(char *) uid->user_id) == 0) {
				rtn = __ops_write(sig->packet->raw,
						sig->packet->length, info);
				if (!rtn) {
					return !rtn;
				}
			}
		}
	}

	/* TODO: user attributes and corresponding signatures */

	/*
	 * subkey packets and corresponding signatures and optional
	 * revocation
	 */

	if (armoured) {
		writer_info_finalise(&info->errors, &info->winfo);
		__ops_writer_pop(info);
	}
	return rtn;
}

/**
 * \ingroup Core_WritePackets
 * \brief Writes a Public Key packet
 * \param key
 * \param info
 * \return true if OK, otherwise false
 */
bool 
__ops_write_struct_pubkey(const __ops_pubkey_t * key,
			    __ops_createinfo_t * info)
{
	if (key->version != 4) {
		(void) fprintf(stderr,
			"__ops_write_struct_pubkey: wrong key version\n");
		return false;
	}
	return __ops_write_ptag(OPS_PTAG_CT_PUBLIC_KEY, info) &&
		__ops_write_length(1 + 4 + 1 + pubkey_length(key), info) &&
		write_pubkey_body(key, info);
}

/**
 * \ingroup Core_WritePackets
 * \brief Writes one RSA public key packet.
 * \param t Creation time
 * \param n RSA public modulus
 * \param e RSA public encryption exponent
 * \param info Writer settings
 *
 * \return true if OK, otherwise false
 */

bool 
__ops_write_rsa_pubkey(time_t t, const BIGNUM * n,
			 const BIGNUM * e,
			 __ops_createinfo_t * info)
{
	__ops_pubkey_t key;

	__ops_fast_create_rsa_pubkey(&key, t, __UNCONST(n), __UNCONST(e));
	return __ops_write_struct_pubkey(&key, info);
}

/**
 * \ingroup Core_Create
 * \param out
 * \param key
 * \param make_packet
 */

void 
__ops_build_pubkey(__ops_memory_t * out, const __ops_pubkey_t * key,
		     bool make_packet)
{
	__ops_createinfo_t *info;

	info = __ops_createinfo_new();
	__ops_memory_init(out, 128);
	__ops_writer_set_memory(info, out);
	write_pubkey_body(key, info);
	if (make_packet) {
		__ops_memory_make_packet(out, OPS_PTAG_CT_PUBLIC_KEY);
	}
	__ops_createinfo_delete(info);
}

/**
 * \ingroup Core_Create
 *
 * Create an RSA secret key structure. If a parameter is marked as
 * [OPTIONAL], then it can be omitted and will be calculated from
 * other parameters - or, in the case of e, will default to 0x10001.
 *
 * Parameters are _not_ copied, so will be freed if the structure is
 * freed.
 *
 * \param key The key structure to be initialised.
 * \param t
 * \param d The RSA parameter d (=e^-1 mod (p-1)(q-1)) [OPTIONAL]
 * \param p The RSA parameter p
 * \param q The RSA parameter q (q > p)
 * \param u The RSA parameter u (=p^-1 mod q) [OPTIONAL]
 * \param n The RSA public parameter n (=p*q) [OPTIONAL]
 * \param e The RSA public parameter e */

void 
__ops_fast_create_rsa_seckey(__ops_seckey_t * key, time_t t,
			     BIGNUM * d, BIGNUM * p, BIGNUM * q, BIGNUM * u,
			       BIGNUM * n, BIGNUM * e)
{
	__ops_fast_create_rsa_pubkey(&key->pubkey, t, n, e);

	/* XXX: calculate optionals */
	key->key.rsa.d = d;
	key->key.rsa.p = p;
	key->key.rsa.q = q;
	key->key.rsa.u = u;

	key->s2k_usage = OPS_S2KU_NONE;

	/* XXX: sanity check and add errors... */
}

/**
 * \ingroup Core_WritePackets
 * \brief Writes a Secret Key packet.
 * \param key The secret key
 * \param passphrase The passphrase
 * \param pplen Length of passphrase
 * \param info
 * \return true if OK; else false
 */
bool 
__ops_write_struct_seckey(const __ops_seckey_t * key,
			    const unsigned char *passphrase,
			    const size_t pplen,
			    __ops_createinfo_t * info)
{
	int             length = 0;

	if (key->pubkey.version != 4) {
		(void) fprintf(stderr,
			"__ops_write_struct_seckey: public key version\n");
		return false;
	}

	/* Ref: RFC4880 Section 5.5.3 */

	/* pubkey, excluding MPIs */
	length += 1 + 4 + 1 + 1;

	/* s2k usage */
	length += 1;

	switch (key->s2k_usage) {
	case OPS_S2KU_NONE:
		/* nothing to add */
		break;

	case OPS_S2KU_ENCRYPTED_AND_HASHED:	/* 254 */
	case OPS_S2KU_ENCRYPTED:	/* 255 */

		/* Ref: RFC4880 Section 3.7 */
		length += 1;	/* s2k_specifier */

		switch (key->s2k_specifier) {
		case OPS_S2KS_SIMPLE:
			length += 1;	/* hash algorithm */
			break;

		case OPS_S2KS_SALTED:
			length += 1 + 8;	/* hash algorithm + salt */
			break;

		case OPS_S2KS_ITERATED_AND_SALTED:
			length += 1 + 8 + 1;	/* hash algorithm, salt +
						 * count */
			break;

		default:
			(void) fprintf(stderr,
				"__ops_write_struct_seckey: s2k spec\n");
			return false;
		}
		break;

	default:
		(void) fprintf(stderr,
			"__ops_write_struct_seckey: s2k usage\n");
		return false;
	}

	/* IV */
	if (key->s2k_usage != 0) {
		length += __ops_block_size(key->alg);
	}
	/* checksum or hash */
	switch (key->s2k_usage) {
	case 0:
	case 255:
		length += 2;
		break;

	case 254:
		length += 20;
		break;

	default:
		(void) fprintf(stderr,
			"__ops_write_struct_seckey: s2k cksum usage\n");
		return false;
	}

	/* secret key and public key MPIs */
	length += seckey_length(key);

	return __ops_write_ptag(OPS_PTAG_CT_SECRET_KEY, info) &&
		/* __ops_write_length(1+4+1+1+seckey_length(key)+2,info) && */
		__ops_write_length((unsigned)length, info) &&
		write_seckey_body(key, passphrase, pplen, info);
}

/**
 * \ingroup Core_Create
 *
 * \brief Create a new __ops_createinfo_t structure.
 *
 * \return the new structure.
 * \note It is the responsiblity of the caller to call __ops_createinfo_delete().
 * \sa __ops_createinfo_delete()
 */
__ops_createinfo_t *
__ops_createinfo_new(void)
{
	return calloc(1, sizeof(__ops_createinfo_t));
}

/**
 * \ingroup Core_Create
 * \brief Delete an __ops_createinfo_t strucut and associated resources.
 *
 * Delete an __ops_createinfo_t structure. If a writer is active, then
 * that is also deleted.
 *
 * \param info the structure to be deleted.
 */
void 
__ops_createinfo_delete(__ops_createinfo_t * info)
{
	writer_info_delete(&info->winfo);
	(void) free(info);
}

/**
 \ingroup Core_Create
 \brief Calculate the checksum for a session key
 \param sesskey Session Key to use
 \param cs Checksum to be written
 \return true if OK; else false
*/
bool 
__ops_calc_sesskey_checksum(__ops_pk_sesskey_t * sesskey, unsigned char cs[2])
{
	unsigned int    i = 0;
	unsigned long   checksum = 0;

	if (!__ops_is_sa_supported(sesskey->symm_alg)) {
		return false;
	}

	for (i = 0; i < __ops_key_size(sesskey->symm_alg); i++) {
		checksum += sesskey->key[i];
	}
	checksum = checksum % 65536;

	cs[0] = (unsigned char)((checksum >> 8) & 0xff);
	cs[1] = (unsigned char)(checksum & 0xff);

	if (__ops_get_debug_level(__FILE__)) {
		(void) fprintf(stderr,"\nm buf checksum: ");
		(void) fprintf(stderr," %2x",cs[0]);
		(void) fprintf(stderr," %2x\n",cs[1]);
	}
	return true;
}

static bool 
create_unencoded_m_buf(__ops_pk_sesskey_t * sesskey, unsigned char *m_buf)
{
	int             i = 0;

	/* m_buf is the buffer which will be encoded in PKCS#1 block */
	/* encoding to form the "m" value used in the  */
	/* Public Key Encrypted Session Key Packet */
	/*
	 * as defined in RFC Section 5.1 "Public-Key Encrypted Session Key
	 * Packet"
	 */

	m_buf[0] = sesskey->symm_alg;

	if (sesskey->symm_alg != OPS_SA_CAST5) {
		(void) fprintf(stderr, "create_unencoded_m_buf: symm alg\n");
		return false;
	}
	for (i = 0; i < CAST_KEY_LENGTH; i++) {
		m_buf[1 + i] = sesskey->key[i];
	}

	return (__ops_calc_sesskey_checksum(sesskey,
				m_buf + 1 + CAST_KEY_LENGTH));
}

/**
\ingroup Core_Create
\brief implementation of EME-PKCS1-v1_5-ENCODE, as defined in OpenPGP RFC
\param M
\param mLen
\param pubkey
\param EM
\return true if OK; else false
*/
bool 
encode_m_buf(const unsigned char *M, size_t mLen,
	     const __ops_pubkey_t * pubkey,
	     unsigned char *EM)
{
	unsigned int    k;
	unsigned        i;

	/* implementation of EME-PKCS1-v1_5-ENCODE, as defined in OpenPGP RFC */

	if (pubkey->alg != OPS_PKA_RSA) {
		(void) fprintf(stderr, "encode_m_buf: pubkey algorithm\n");
		return false;
	}

	k = BN_num_bytes(pubkey->key.rsa.n);
	if (mLen > k - 11) {
		(void) fprintf(stderr, "encode_m_buf: message too long\n");
		return false;
	}
	/* these two bytes defined by RFC */
	EM[0] = 0x00;
	EM[1] = 0x02;

	/* add non-zero random bytes of length k - mLen -3 */
	for (i = 2; i < k - mLen - 1; ++i) {
		do {
			__ops_random(EM + i, 1);
		} while (EM[i] == 0);
	}

	if (i < 8 + 2) {
		(void) fprintf(stderr, "encode_m_buf: bad i len\n");
		return false;
	}

	EM[i++] = 0;

	(void) memcpy(EM + i, M, mLen);

	if (__ops_get_debug_level(__FILE__)) {
		unsigned int    i2 = 0;

		(void) fprintf(stderr, "Encoded Message: \n");
		for (i2 = 0; i2 < mLen; i2++) {
			(void) fprintf(stderr, "%2x ", EM[i2]);
		}
		(void) fprintf(stderr, "\n");
	}
	return true;
}

/**
 \ingroup Core_Create
\brief Creates an __ops_pk_sesskey_t struct from keydata
\param key Keydata to use
\return __ops_pk_sesskey_t struct
\note It is the caller's responsiblity to free the returned pointer
\note Currently hard-coded to use CAST5
\note Currently hard-coded to use RSA
*/
__ops_pk_sesskey_t *
__ops_create_pk_sesskey(const __ops_keydata_t * key)
{
	/*
         * Creates a random session key and encrypts it for the given key
         *
         * Session Key is for use with a SK algo,
         * can be any, we're hardcoding CAST5 for now
         *
         * Encryption used is PK,
         * can be any, we're hardcoding RSA for now
         */

	const __ops_pubkey_t *pub_key = __ops_get_pubkey(key);
#define SZ_UNENCODED_M_BUF CAST_KEY_LENGTH+1+2
	unsigned char   unencoded_m_buf[SZ_UNENCODED_M_BUF];
	const size_t    sz_encoded_m_buf = BN_num_bytes(pub_key->key.rsa.n);
	unsigned char  *encoded_m_buf = calloc(1, sz_encoded_m_buf);

	__ops_pk_sesskey_t *sesskey = calloc(1, sizeof(*sesskey));
	if (key->type != OPS_PTAG_CT_PUBLIC_KEY) {
		(void) fprintf(stderr,
			"__ops_create_pk_sesskey: bad type\n");
		return NULL;
	}
	sesskey->version = OPS_PKSK_V3;
	(void) memcpy(sesskey->key_id, key->key_id,
			sizeof(sesskey->key_id));

	if (__ops_get_debug_level(__FILE__)) {
		unsigned int    i = 0;

		(void) fprintf(stderr, "Encrypting for RSA key id : ");
		for (i = 0; i < sizeof(sesskey->key_id); i++) {
			(void) fprintf(stderr, "%2x ", key->key_id[i]);
		}
		(void) fprintf(stderr, "\n");
	}
	if (key->key.pubkey.alg != OPS_PKA_RSA) {
		(void) fprintf(stderr,
			"__ops_create_pk_sesskey: bad pubkey algorithm\n");
		return NULL;
	}
	sesskey->alg = key->key.pubkey.alg;

	/* \todo allow user to specify other algorithm */
	sesskey->symm_alg = OPS_SA_CAST5;
	__ops_random(sesskey->key, CAST_KEY_LENGTH);

	if (__ops_get_debug_level(__FILE__)) {
		unsigned int    i = 0;

		(void) fprintf(stderr,
			"CAST5 session key created (len=%d):\n ",
			CAST_KEY_LENGTH);
		for (i = 0; i < CAST_KEY_LENGTH; i++) {
			(void) fprintf(stderr, "%2x ", sesskey->key[i]);
		}
		(void) fprintf(stderr, "\n");
	}
	if (create_unencoded_m_buf(sesskey, &unencoded_m_buf[0]) == false) {
		(void) free(encoded_m_buf);
		return NULL;
	}
	if (__ops_get_debug_level(__FILE__)) {
		unsigned int    i = 0;

		printf("unencoded m buf:\n");
		for (i = 0; i < SZ_UNENCODED_M_BUF; i++) {
			printf("%2x ", unencoded_m_buf[i]);
		}
		printf("\n");
	}
	encode_m_buf(&unencoded_m_buf[0], SZ_UNENCODED_M_BUF, pub_key,
			&encoded_m_buf[0]);

	/* and encrypt it */
	if (!__ops_rsa_encrypt_mpi(encoded_m_buf, sz_encoded_m_buf, pub_key,
			&sesskey->parameters)) {
		(void) free(encoded_m_buf);
		return NULL;
	}
	(void) free(encoded_m_buf);
	return sesskey;
}

/**
\ingroup Core_WritePackets
\brief Writes Public Key Session Key packet
\param info Write settings
\param pksk Public Key Session Key to write out
\return true if OK; else false
*/
bool 
__ops_write_pk_sesskey(__ops_createinfo_t * info,
			 __ops_pk_sesskey_t * pksk)
{
	if (pksk == NULL) {
		(void) fprintf(stderr,
			"__ops_write_pk_sesskey: NULL pksk\n");
		return false;
	}
	if (pksk->alg != OPS_PKA_RSA) {
		(void) fprintf(stderr,
			"__ops_write_pk_sesskey: bad algorithm\n");
		return false;
	}

	return __ops_write_ptag(OPS_PTAG_CT_PK_SESSION_KEY, info) &&
		__ops_write_length((unsigned)(1 + 8 + 1 + BN_num_bytes(pksk->parameters.rsa.encrypted_m) + 2), info) &&
		__ops_write_scalar((unsigned)pksk->version, 1, info) &&
		__ops_write(pksk->key_id, 8, info) &&
		__ops_write_scalar((unsigned)pksk->alg, 1, info) &&
		__ops_write_mpi(pksk->parameters.rsa.encrypted_m, info)
	/* ??	&& __ops_write_scalar(0, 2, info); */
		;
}

/**
\ingroup Core_WritePackets
\brief Writes MDC packet
\param hashed Hash for MDC
\param info Write settings
\return true if OK; else false
*/

bool 
__ops_write_mdc(const unsigned char *hashed, __ops_createinfo_t *info)
{
	/* write it out */
	return __ops_write_ptag(OPS_PTAG_CT_MDC, info) &&
		__ops_write_length(OPS_SHA1_HASH_SIZE, info) &&
		__ops_write(hashed, OPS_SHA1_HASH_SIZE, info);
}

/**
\ingroup Core_WritePackets
\brief Writes Literal Data packet from buffer
\param data Buffer to write out
\param maxlen Max length of buffer
\param type Literal Data Type
\param info Write settings
\return true if OK; else false
*/
bool 
__ops_write_litdata(const unsigned char *data,
				const int maxlen,
				const __ops_litdata_type_t type,
				__ops_createinfo_t * info)
{
	/*
         * RFC4880 does not specify a meaning for filename or date.
         * It is implementation-dependent.
         * We will not implement them.
         */
	/* \todo do we need to check text data for <cr><lf> line endings ? */
	return __ops_write_ptag(OPS_PTAG_CT_LITERAL_DATA, info) &&
		__ops_write_length((unsigned)(1 + 1 + 4 + maxlen), info) &&
		__ops_write_scalar((unsigned)type, 1, info) &&
		__ops_write_scalar(0, 1, info) &&
		__ops_write_scalar(0, 4, info) &&
		__ops_write(data, (unsigned)maxlen, info);
}

/**
\ingroup Core_WritePackets
\brief Writes Literal Data packet from contents of file
\param filename Name of file to read from
\param type Literal Data Type
\param info Write settings
\return true if OK; else false
*/

bool 
__ops_fileread_litdata(const char *filename,
				 const __ops_litdata_type_t type,
				 __ops_createinfo_t * info)
{
	unsigned char    buf[1024];
	unsigned char	*mmapped;
	__ops_memory_t	*mem = NULL;
	struct stat	 st;
	size_t           len = 0;
	int              fd = 0;
	bool   		 rtn;

#ifdef O_BINARY
	fd = open(filename, O_RDONLY | O_BINARY);
#else
	fd = open(filename, O_RDONLY);
#endif
	if (fd < 0)
		return false;

	mem = __ops_memory_new();
	mmapped = MAP_FAILED;
#ifdef USE_MMAP_FOR_FILES
	if (fstat(fd, &st) == 0) {
		mem->length = (unsigned)st.st_size;
		mmapped = mem->buf = mmap(NULL, (size_t)st.st_size, PROT_READ,
					MAP_FILE | MAP_PRIVATE, fd, 0);
	}
#endif
	if (mmapped == MAP_FAILED) {
		__ops_memory_init(mem, (size_t)st.st_size);
		for (;;) {
			ssize_t         n = 0;

			if ((n = read(fd, buf, sizeof(buf))) == 0) {
				break;
			}
			__ops_memory_add(mem, buf, (unsigned)n);
		}
	}
	(void) close(fd);

	/* \todo do we need to check text data for <cr><lf> line endings ? */
	len = __ops_memory_get_length(mem);
	rtn = __ops_write_ptag(OPS_PTAG_CT_LITERAL_DATA, info) &&
		__ops_write_length(1 + 1 + 4 + len, info) &&
		__ops_write_scalar((unsigned)type, 1, info) &&
		__ops_write_scalar(0, 1, info)	/* filename */ &&
		__ops_write_scalar(0, 4, info)	/* date */ &&
		__ops_write(__ops_memory_get_data(mem), len, info);

#ifdef USE_MMAP_FOR_FILES
	if (mmapped != MAP_FAILED) {
		munmap(mmapped, mem->length);
		mmapped = buf;
	}
#endif
	if (mmapped == NULL) {
		__ops_memory_free(mem);
	}
	return rtn;
}

/**
   \ingroup HighLevel_General

   \brief Reads contents of file into new __ops_memory_t struct.

   \param filename Filename to read from
   \param errnum Pointer to error
   \return new __ops_memory_t pointer containing the contents of the file

   \note If there was an error opening the file or reading from it,
   	errnum is set to the cause
   \note It is the caller's responsibility to call __ops_memory_free(mem)
*/
__ops_memory_t   *
__ops_fileread(const char *filename, int *errnum)
{
	__ops_memory_t   *mem = NULL;
	unsigned char    buf[1024];
	struct stat	 st;
	int              fd = 0;

	*errnum = 0;
#ifdef O_BINARY
	fd = open(filename, O_RDONLY | O_BINARY);
#else
	fd = open(filename, O_RDONLY);
#endif
	if (fd < 0) {
		*errnum = errno;
		return false;
	}
	mem = __ops_memory_new();
	(void) fstat(fd, &st);
	__ops_memory_init(mem, (unsigned)st.st_size);
	for (;;) {
		ssize_t         n = 0;

		n = read(fd, buf, sizeof(buf));
		if (n < 0) {
			*errnum = errno;
			break;
		}
		if (!n) {
			break;
		}
		__ops_memory_add(mem, buf, (unsigned)n);
	}
	(void) close(fd);
	return mem;
}

/**
   \ingroup HighLevel_General

   \brief Writes contents of buffer into file

   \param filename Filename to write to
   \param buf Buffer to write to file
   \param len Size of buffer
   \param overwrite Flag to set whether to overwrite an existing file
   \return 1 if OK; 0 if error
*/

int 
__ops_filewrite(const char *filename, const char *buf,
			const size_t len, const bool overwrite)
{
	int		flags = 0;
	int		fd = 0;

	flags = O_WRONLY | O_CREAT;
	if (overwrite == true) {
		flags |= O_TRUNC;
	} else {
		flags |= O_EXCL;
	}
#ifdef O_BINARY
	flags |= O_BINARY;
#endif
	fd = open(filename, flags, 0600);
	if (fd < 0) {
		perror(NULL);
		return 0;
	}
	if (write(fd, buf, len) != (int)len) {
		return 0;
	}

	return (close(fd) == 0);
}

/**
\ingroup Core_WritePackets
\brief Write Symmetrically Encrypted packet
\param data Data to encrypt
\param len Length of data
\param info Write settings
\return true if OK; else false
\note Hard-coded to use AES256
*/
bool 
__ops_write_symm_enc_data(const unsigned char *data,
				       const int len,
				       __ops_createinfo_t * info)
{
			/* buffer to write encrypted data to */
	unsigned char  *encrypted = (unsigned char *) NULL;
	__ops_crypt_t	crypt_info;
	size_t		encrypted_sz = 0;	/* size of encrypted data */
	int             done = 0;

	/* \todo assume AES256 for now */
	__ops_crypt_any(&crypt_info, OPS_SA_AES_256);
	__ops_encrypt_init(&crypt_info);

	encrypted_sz = len + crypt_info.blocksize + 2;
	encrypted = calloc(1, encrypted_sz);

	done = __ops_encrypt_se(&crypt_info, encrypted, data, (unsigned)len);
	if (done != len) {
		(void) fprintf(stderr,
"__ops_write_symm_enc_data: done != len\n");
		return false;
	}

	return __ops_write_ptag(OPS_PTAG_CT_SE_DATA, info) &&
		__ops_write_length(1 + encrypted_sz, info) &&
		__ops_write(data, (unsigned)len, info);
}

/**
\ingroup Core_WritePackets
\brief Write a One Pass Signature packet
\param seckey Secret Key to use
\param hash_alg Hash Algorithm to use
\param sig_type Signature type
\param info Write settings
\return true if OK; else false
*/
bool 
__ops_write_one_pass_sig(const __ops_seckey_t * seckey,
		       const __ops_hash_alg_t hash_alg,
		       const __ops_sig_type_t sig_type,
		       __ops_createinfo_t * info)
{
	unsigned char   keyid[OPS_KEY_ID_SIZE];

	if (__ops_get_debug_level(__FILE__)) {
		fprintf(stderr, "calling __ops_keyid in write_one_pass_sig: this calls sha1_init\n");
	}
	__ops_keyid(keyid, OPS_KEY_ID_SIZE, OPS_KEY_ID_SIZE, &seckey->pubkey);

	return __ops_write_ptag(OPS_PTAG_CT_ONE_PASS_SIGNATURE, info) &&
		__ops_write_length(1 + 1 + 1 + 1 + 8 + 1, info) &&
		__ops_write_scalar(3, 1, info)	/* version */ &&
		__ops_write_scalar((unsigned)sig_type, 1, info) &&
		__ops_write_scalar((unsigned)hash_alg, 1, info) &&
		__ops_write_scalar((unsigned)seckey->pubkey.alg, 1, info) &&
		__ops_write(keyid, 8, info) &&
		__ops_write_scalar(1, 1, info);
}
