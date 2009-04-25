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

#include "signature.h"
#include "crypto.h"
#include "create.h"
#include "netpgpsdk.h"

#include "readerwriter.h"
#include "loccreate.h"
#include "validate.h"
#include "netpgpdefs.h"

#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif

#include <string.h>
#include <fcntl.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_OPENSSL_DSA_H
#include <openssl/dsa.h>
#endif

#define MAXBUF 1024		/* <! Standard buffer size to use */

/** \ingroup Core_Create
 * needed for signature creation
 */
struct __ops_create_signature {
	__ops_hash_t      hash;
	__ops_signature_t sig;
	__ops_memory_t   *mem;
	__ops_create_info_t *info;/* !< how to do the writing */
	unsigned        hashed_count_offset;
	unsigned        hashed_data_length;
	unsigned        unhashed_count_offset;
};

/**
   \ingroup Core_Signature
   Creates new __ops_create_signature_t
   \return new __ops_create_signature_t
   \note It is the caller's responsibility to call __ops_create_signature_delete()
   \sa __ops_create_signature_delete()
*/
__ops_create_signature_t *
__ops_create_signature_new()
{
	return calloc(1, sizeof(__ops_create_signature_t));
}

/**
   \ingroup Core_Signature
   Free signature and memory associated with it
   \param sig struct to free
   \sa __ops_create_signature_new()
*/
void 
__ops_create_signature_delete(__ops_create_signature_t * sig)
{
	__ops_create_info_delete(sig->info);
	sig->info = NULL;
	free(sig);
}

static unsigned char prefix_md5[] = {0x30, 0x20, 0x30, 0x0C, 0x06, 0x08, 0x2A, 0x86,
	0x48, 0x86, 0xF7, 0x0D, 0x02, 0x05, 0x05, 0x00,
0x04, 0x10};

static unsigned char prefix_sha1[] = {0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0E,
0x03, 0x02, 0x1A, 0x05, 0x00, 0x04, 0x14};

static unsigned char prefix_sha256[] = {0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86,
	0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05,
0x00, 0x04, 0x20};

#if 0
/**
   \ingroup Core_Create
   implementation of EMSA-PKCS1-v1_5, as defined in OpenPGP RFC
   \param M
   \param mLen
   \param hash_alg Hash algorithm to use
   \param EM
   \return true if OK; else false
*/
static bool 
encode_hash_buf(const unsigned char *M, size_t mLen,
		const __ops_hash_algorithm_t hash_alg,
		unsigned char *EM
)
{
	/* implementation of EMSA-PKCS1-v1_5, as defined in OpenPGP RFC */

	unsigned        i;

	int             n = 0;
	__ops_hash_t      hash;
	int             hash_sz = 0;
	int             encoded_hash_sz = 0;
	int             prefix_sz = 0;
	unsigned        padding_sz = 0;
	unsigned        encoded_msg_sz = 0;
	unsigned char  *prefix = NULL;

	assert(hash_alg == OPS_HASH_SHA1);

	/* 1. Apply hash function to M */

	__ops_hash_any(&hash, hash_alg);
	hash.init(&hash);
	hash.add(&hash, M, mLen);

	/* \todo combine with rsa_sign */

	/* 2. Get hash prefix */

	switch (hash_alg) {
	case OPS_HASH_SHA1:
		prefix = prefix_sha1;
		prefix_sz = sizeof(prefix_sha1);
		hash_sz = OPS_SHA1_HASH_SIZE;
		encoded_hash_sz = hash_sz + prefix_sz;
		/* \todo why is Ben using a PS size of 90 in rsa_sign? */
		/* (keysize-hashsize-1-2) */
		padding_sz = 90;
		break;

	default:
		assert(0);
	}

	/* \todo 3. Test for len being too short */

	/* 4 and 5. Generate PS and EM */

	EM[0] = 0x00;
	EM[1] = 0x01;

	for (i = 0; i < padding_sz; i++)
		EM[2 + i] = 0xFF;

	i += 2;

	EM[i++] = 0x00;

	(void) memcpy(&EM[i], prefix, prefix_sz);
	i += prefix_sz;

	/* finally, write out hashed result */

	n = hash.finish(&hash, &EM[i]);

	encoded_msg_sz = i + hash_sz - 1;

	/* \todo test n for OK response? */

	if (__ops_get_debug_level(__FILE__)) {
		fprintf(stderr, "Encoded Message: \n");
		for (i = 0; i < encoded_msg_sz; i++)
			fprintf(stderr, "%2x ", EM[i]);
		fprintf(stderr, "\n");
	}
	return true;
}
#endif

/* XXX: both this and verify would be clearer if the signature were */
/* treated as an MPI. */
static void 
rsa_sign(__ops_hash_t * hash, const __ops_rsa_public_key_t * rsa,
	 const __ops_rsa_secret_key_t * srsa,
	 __ops_create_info_t * opt)
{
	unsigned char   hashbuf[NETPGP_BUFSIZ];
	unsigned char   sigbuf[NETPGP_BUFSIZ];
	unsigned        keysize;
	unsigned        hashsize;
	unsigned        n;
	unsigned        t;
	BIGNUM         *bn;

	/* XXX: we assume hash is sha-1 for now */
	hashsize = 20 + sizeof(prefix_sha1);

	keysize = (BN_num_bits(rsa->n) + 7) / 8;
	assert(keysize <= sizeof(hashbuf));
	assert(10 + hashsize <= keysize);

	hashbuf[0] = 0;
	hashbuf[1] = 1;
	if (__ops_get_debug_level(__FILE__)) {
		printf("rsa_sign: PS is %d\n", keysize - hashsize - 1 - 2);
	}
	for (n = 2; n < keysize - hashsize - 1; ++n)
		hashbuf[n] = 0xff;
	hashbuf[n++] = 0;

	(void) memcpy(&hashbuf[n], prefix_sha1, sizeof(prefix_sha1));
	n += sizeof(prefix_sha1);

	t = hash->finish(hash, &hashbuf[n]);
	assert(t == 20);

	__ops_write(&hashbuf[n], 2, opt);

	n += t;
	assert(n == keysize);

	t = __ops_rsa_private_encrypt(sigbuf, hashbuf, keysize, srsa, rsa);
	bn = BN_bin2bn(sigbuf, (int)t, NULL);
	__ops_write_mpi(bn, opt);
	BN_free(bn);
}

static void 
dsa_sign(__ops_hash_t * hash,
	 const __ops_dsa_public_key_t * dsa,
	 const __ops_dsa_secret_key_t * sdsa,
	 __ops_create_info_t * cinfo)
{
	unsigned char   hashbuf[NETPGP_BUFSIZ];
	unsigned        hashsize;
	unsigned        t;
	DSA_SIG        *dsasig;

	/* hashsize must be "equal in size to the number of bits of q,  */
	/* the group generated by the DSA key's generator value */
	/* 160/8 = 20 */

	hashsize = 20;

	/* finalise hash */
	t = hash->finish(hash, &hashbuf[0]);
	assert(t == 20);

	__ops_write(&hashbuf[0], 2, cinfo);

	/* write signature to buf */
	dsasig = __ops_dsa_sign(hashbuf, hashsize, sdsa, dsa);

	/* convert and write the sig out to memory */
	__ops_write_mpi(dsasig->r, cinfo);
	__ops_write_mpi(dsasig->s, cinfo);
	DSA_SIG_free(dsasig);
}

static bool 
rsa_verify(__ops_hash_algorithm_t type,
	   const unsigned char *hash,
	   size_t hash_length,
	   const __ops_rsa_signature_t * sig,
	   const __ops_rsa_public_key_t * rsa)
{
	unsigned char   sigbuf[NETPGP_BUFSIZ];
	unsigned char   hashbuf_from_sig[NETPGP_BUFSIZ];
	unsigned        n;
	unsigned        keysize;
	const unsigned char *prefix;
	unsigned	plen;
	int             debug_len_decrypted;

	plen = 0;
	prefix = (const unsigned char *) "";
	keysize = BN_num_bytes(rsa->n);
	/* RSA key can't be bigger than 65535 bits, so... */
	assert(keysize <= sizeof(hashbuf_from_sig));
	assert((unsigned) BN_num_bits(sig->sig) <= 8 * sizeof(sigbuf));
	BN_bn2bin(sig->sig, sigbuf);

	n = __ops_rsa_public_decrypt(hashbuf_from_sig, sigbuf,
		(unsigned)(BN_num_bits(sig->sig) + 7) / 8, rsa);
	debug_len_decrypted = n;

	if (n != keysize)	/* obviously, this includes error returns */
		return false;

	/* XXX: why is there a leading 0? The first byte should be 1... */
	/* XXX: because the decrypt should use keysize and not sigsize? */
	if (hashbuf_from_sig[0] != 0 || hashbuf_from_sig[1] != 1)
		return false;

	switch (type) {
	case OPS_HASH_MD5:
		prefix = prefix_md5;
		plen = sizeof(prefix_md5);
		break;
	case OPS_HASH_SHA1:
		prefix = prefix_sha1;
		plen = sizeof(prefix_sha1);
		break;
	case OPS_HASH_SHA256:
		prefix = prefix_sha256;
		plen = sizeof(prefix_sha256);
		break;
	default:
		(void) fprintf(stderr, "Unknown hash algorithm: %d\n", type);
		return false;
	}

	if (keysize - plen - hash_length < 10)
		return false;

	for (n = 2; n < keysize - plen - hash_length - 1; ++n)
		if (hashbuf_from_sig[n] != 0xff)
			return false;

	if (hashbuf_from_sig[n++] != 0)
		return false;

	if (__ops_get_debug_level(__FILE__)) {
		int             zz;
		unsigned        uu;

		printf("\n");
		printf("hashbuf_from_sig\n");
		for (zz = 0; zz < debug_len_decrypted; zz++) {
			printf("%02x ", hashbuf_from_sig[n + zz]);
		}
		printf("\n");
		printf("prefix\n");
		for (zz = 0; zz < plen; zz++) {
			printf("%02x ", prefix[zz]);
		}
		printf("\n");

		printf("\n");
		printf("hash from sig\n");
		for (uu = 0; uu < hash_length; uu++) {
			printf("%02x ", hashbuf_from_sig[n + plen + uu]);
		}
		printf("\n");
		printf("hash passed in (should match hash from sig)\n");
		for (uu = 0; uu < hash_length; uu++) {
			printf("%02x ", hash[uu]);
		}
		printf("\n");
	}
	if (memcmp(&hashbuf_from_sig[n], prefix, plen) != 0
	    || memcmp(&hashbuf_from_sig[n + plen], hash, hash_length) != 0)
		return false;

	return true;
}

static void 
hash_add_key(__ops_hash_t * hash, const __ops_public_key_t * key)
{
	__ops_memory_t   *mem = __ops_memory_new();
	size_t          l;

	__ops_build_public_key(mem, key, false);

	l = __ops_memory_get_length(mem);
	__ops_hash_add_int(hash, 0x99, 1);
	__ops_hash_add_int(hash, l, 2);
	hash->add(hash, __ops_memory_get_data(mem), l);

	__ops_memory_free(mem);
}

static void 
initialise_hash(__ops_hash_t * hash, const __ops_signature_t * sig)
{
	__ops_hash_any(hash, sig->info.hash_algorithm);
	hash->init(hash);
}

static void 
init_key_signature(__ops_hash_t * hash, const __ops_signature_t * sig,
		   const __ops_public_key_t * key)
{
	initialise_hash(hash, sig);
	hash_add_key(hash, key);
}

static void 
hash_add_trailer(__ops_hash_t * hash, const __ops_signature_t * sig,
		 const unsigned char *raw_packet)
{
	if (sig->info.version == OPS_V4) {
		if (raw_packet)
			hash->add(hash, raw_packet + sig->v4_hashed_data_start,
				  sig->info.v4_hashed_data_length);
		__ops_hash_add_int(hash, (unsigned)sig->info.version, 1);
		__ops_hash_add_int(hash, 0xff, 1);
		__ops_hash_add_int(hash, sig->info.v4_hashed_data_length, 4);
	} else {
		__ops_hash_add_int(hash, (unsigned)sig->info.type, 1);
		__ops_hash_add_int(hash, (unsigned)sig->info.creation_time, 4);
	}
}

/**
   \ingroup Core_Signature
   \brief Checks a signature
   \param hash Signature Hash to be checked
   \param length Signature Length
   \param sig The Signature to be checked
   \param signer The signer's public key
   \return true if good; else false
*/
bool 
__ops_check_signature(const unsigned char *hash, unsigned length,
		    const __ops_signature_t * sig,
		    const __ops_public_key_t * signer)
{
	bool   ret;

	if (__ops_get_debug_level(__FILE__)) {
		printf("__ops_check_signature: (length %d) hash=", length);
		/* hashout[0]=0; */
		hexdump(hash, length, "");
	}
	ret = 0;
	switch (sig->info.key_algorithm) {
	case OPS_PKA_DSA:
		ret = __ops_dsa_verify(hash, length, &sig->info.signature.dsa, &signer->key.dsa);
		break;

	case OPS_PKA_RSA:
		ret = rsa_verify(sig->info.hash_algorithm, hash, length, &sig->info.signature.rsa,
				 &signer->key.rsa);
		break;

	default:
		assert(/*CONSTCOND*/0);
	}

	return ret;
}

static bool 
hash_and_check_signature(__ops_hash_t * hash,
			 const __ops_signature_t * sig,
			 const __ops_public_key_t * signer)
{
	unsigned char   hashout[OPS_MAX_HASH_SIZE];
	unsigned	n;

	n = hash->finish(hash, hashout);

	return __ops_check_signature(hashout, n, sig, signer);
}

static bool 
finalise_signature(__ops_hash_t * hash,
		   const __ops_signature_t * sig,
		   const __ops_public_key_t * signer,
		   const unsigned char *raw_packet)
{
	hash_add_trailer(hash, sig, raw_packet);
	return hash_and_check_signature(hash, sig, signer);
}

/**
 * \ingroup Core_Signature
 *
 * \brief Verify a certification signature.
 *
 * \param key The public key that was signed.
 * \param id The user ID that was signed
 * \param sig The signature.
 * \param signer The public key of the signer.
 * \param raw_packet The raw signature packet.
 * \return true if OK; else false
 */
bool
__ops_check_user_id_certification_signature(const __ops_public_key_t * key,
					  const __ops_user_id_t * id,
					  const __ops_signature_t * sig,
					  const __ops_public_key_t * signer,
					  const unsigned char *raw_packet)
{
	__ops_hash_t      hash;
	size_t          user_id_len = strlen((char *) id->user_id);

	init_key_signature(&hash, sig, key);

	if (sig->info.version == OPS_V4) {
		__ops_hash_add_int(&hash, 0xb4, 1);
		__ops_hash_add_int(&hash, user_id_len, 4);
	}
	hash.add(&hash, id->user_id, user_id_len);

	return finalise_signature(&hash, sig, signer, raw_packet);
}

/**
 * \ingroup Core_Signature
 *
 * Verify a certification signature.
 *
 * \param key The public key that was signed.
 * \param attribute The user attribute that was signed
 * \param sig The signature.
 * \param signer The public key of the signer.
 * \param raw_packet The raw signature packet.
 * \return true if OK; else false
 */
bool
__ops_check_user_attribute_certification_signature(const __ops_public_key_t * key,
				     const __ops_user_attribute_t * attribute,
						 const __ops_signature_t * sig,
					    const __ops_public_key_t * signer,
					    const unsigned char *raw_packet)
{
	__ops_hash_t      hash;

	init_key_signature(&hash, sig, key);

	if (sig->info.version == OPS_V4) {
		__ops_hash_add_int(&hash, 0xd1, 1);
		__ops_hash_add_int(&hash, attribute->data.len, 4);
	}
	hash.add(&hash, attribute->data.contents, attribute->data.len);

	return finalise_signature(&hash, sig, signer, raw_packet);
}

/**
 * \ingroup Core_Signature
 *
 * Verify a subkey signature.
 *
 * \param key The public key whose subkey was signed.
 * \param subkey The subkey of the public key that was signed.
 * \param sig The signature.
 * \param signer The public key of the signer.
 * \param raw_packet The raw signature packet.
 * \return true if OK; else false
 */
bool
__ops_check_subkey_signature(const __ops_public_key_t * key,
			   const __ops_public_key_t * subkey,
			   const __ops_signature_t * sig,
			   const __ops_public_key_t * signer,
			   const unsigned char *raw_packet)
{
	__ops_hash_t      hash;

	init_key_signature(&hash, sig, key);
	hash_add_key(&hash, subkey);

	return finalise_signature(&hash, sig, signer, raw_packet);
}

/**
 * \ingroup Core_Signature
 *
 * Verify a direct signature.
 *
 * \param key The public key which was signed.
 * \param sig The signature.
 * \param signer The public key of the signer.
 * \param raw_packet The raw signature packet.
 * \return true if OK; else false
 */
bool
__ops_check_direct_signature(const __ops_public_key_t * key,
			   const __ops_signature_t * sig,
			   const __ops_public_key_t * signer,
			   const unsigned char *raw_packet)
{
	__ops_hash_t      hash;

	init_key_signature(&hash, sig, key);
	return finalise_signature(&hash, sig, signer, raw_packet);
}

/**
 * \ingroup Core_Signature
 *
 * Verify a signature on a hash (the hash will have already been fed
 * the material that was being signed, for example signed cleartext).
 *
 * \param hash A hash structure of appropriate type that has been fed
 * the material to be signed. This MUST NOT have been finalised.
 * \param sig The signature to be verified.
 * \param signer The public key of the signer.
 * \return true if OK; else false
 */
bool
__ops_check_hash_signature(__ops_hash_t * hash,
			 const __ops_signature_t * sig,
			 const __ops_public_key_t * signer)
{
	if (sig->info.hash_algorithm != hash->algorithm)
		return false;

	return finalise_signature(hash, sig, signer, NULL);
}

static void 
start_signature_in_mem(__ops_create_signature_t * sig)
{
	/* since this has subpackets and stuff, we have to buffer the whole */
	/* thing to get counts before writing. */
	sig->mem = __ops_memory_new();
	__ops_memory_init(sig->mem, 100);
	__ops_writer_set_memory(sig->info, sig->mem);

	/* write nearly up to the first subpacket */
	__ops_write_scalar((unsigned)sig->sig.info.version, 1, sig->info);
	__ops_write_scalar((unsigned)sig->sig.info.type, 1, sig->info);
	__ops_write_scalar((unsigned)sig->sig.info.key_algorithm, 1, sig->info);
	__ops_write_scalar((unsigned)sig->sig.info.hash_algorithm, 1, sig->info);

	/* dummy hashed subpacket count */
	sig->hashed_count_offset = __ops_memory_get_length(sig->mem);
	__ops_write_scalar(0, 2, sig->info);
}

/**
 * \ingroup Core_Signature
 *
 * __ops_signature_start() creates a V4 public key signature with a SHA1 hash.
 *
 * \param sig The signature structure to initialise
 * \param key The public key to be signed
 * \param id The user ID being bound to the key
 * \param type Signature type
 */
void 
__ops_signature_start_key_signature(__ops_create_signature_t * sig,
				  const __ops_public_key_t * key,
				  const __ops_user_id_t * id,
				  __ops_sig_type_t type)
{
	sig->info = __ops_create_info_new();

	/* XXX:  refactor with check (in several ways - check should
	 * probably use the buffered writer to construct packets
	 * (done), and also should share code for hash calculation) */
	sig->sig.info.version = OPS_V4;
	sig->sig.info.hash_algorithm = OPS_HASH_SHA1;
	sig->sig.info.key_algorithm = key->algorithm;
	sig->sig.info.type = type;

	sig->hashed_data_length = (unsigned)-1;

	init_key_signature(&sig->hash, &sig->sig, key);

	__ops_hash_add_int(&sig->hash, 0xb4, 1);
	__ops_hash_add_int(&sig->hash, strlen((char *) id->user_id), 4);
	sig->hash.add(&sig->hash, id->user_id, strlen((char *) id->user_id));

	start_signature_in_mem(sig);
}

/**
 * \ingroup Core_Signature
 *
 * Create a V4 public key signature over some cleartext.
 *
 * \param sig The signature structure to initialise
 * \param id
 * \param type
 * \todo Expand description. Allow other hashes.
 */

static void 
__ops_signature_start_signature(__ops_create_signature_t * sig,
			      const __ops_secret_key_t * key,
			      const __ops_hash_algorithm_t hash,
			      const __ops_sig_type_t type)
{
	sig->info = __ops_create_info_new();

	/* XXX: refactor with check (in several ways - check should probably */
	/*
	 * use the buffered writer to construct packets (done), and also
	 * should
	 */
	/* share code for hash calculation) */
	sig->sig.info.version = OPS_V4;
	sig->sig.info.key_algorithm = key->public_key.algorithm;
	sig->sig.info.hash_algorithm = hash;
	sig->sig.info.type = type;

	sig->hashed_data_length = (unsigned)-1;

	if (__ops_get_debug_level(__FILE__)) {
		fprintf(stderr, "initialising hash for sig in mem\n");
	}
	initialise_hash(&sig->hash, &sig->sig);
	start_signature_in_mem(sig);
}

/**
 * \ingroup Core_Signature
 * \brief Setup to start a cleartext's signature
 */
void 
__ops_signature_start_cleartext_signature(__ops_create_signature_t * sig,
					const __ops_secret_key_t * key,
					const __ops_hash_algorithm_t hash,
					const __ops_sig_type_t type)
{
	__ops_signature_start_signature(sig, key, hash, type);
}

/**
 * \ingroup Core_Signature
 * \brief Setup to start a message's signature
 */
void 
__ops_signature_start_message_signature(__ops_create_signature_t * sig,
				      const __ops_secret_key_t * key,
				      const __ops_hash_algorithm_t hash,
				      const __ops_sig_type_t type)
{
	__ops_signature_start_signature(sig, key, hash, type);
}

/**
 * \ingroup Core_Signature
 *
 * Add plaintext data to a signature-to-be.
 *
 * \param sig The signature-to-be.
 * \param buf The plaintext data.
 * \param length The amount of plaintext data.
 */
void 
__ops_signature_add_data(__ops_create_signature_t * sig, const void *buf,
		       size_t length)
{
	if (__ops_get_debug_level(__FILE__)) {
		fprintf(stderr, "__ops_signature_add_data adds to hash\n");
	}
	sig->hash.add(&sig->hash, buf, length);
}

/**
 * \ingroup Core_Signature
 *
 * Mark the end of the hashed subpackets in the signature
 *
 * \param sig
 */

bool 
__ops_signature_hashed_subpackets_end(__ops_create_signature_t * sig)
{
	sig->hashed_data_length = __ops_memory_get_length(sig->mem)
	- sig->hashed_count_offset - 2;
	__ops_memory_place_int(sig->mem, sig->hashed_count_offset,
			     sig->hashed_data_length, 2);
	/* dummy unhashed subpacket count */
	sig->unhashed_count_offset = __ops_memory_get_length(sig->mem);
	return __ops_write_scalar(0, 2, sig->info);
}

/**
 * \ingroup Core_Signature
 *
 * Write out a signature
 *
 * \param sig
 * \param key
 * \param skey
 * \param info
 *
 */

bool 
__ops_write_signature(__ops_create_signature_t * sig, const __ops_public_key_t * key,
		    const __ops_secret_key_t * skey, __ops_create_info_t * info)
{
	bool   rtn = false;
	size_t          l = __ops_memory_get_length(sig->mem);

	/* check key not decrypted */
	switch (skey->public_key.algorithm) {
	case OPS_PKA_RSA:
	case OPS_PKA_RSA_ENCRYPT_ONLY:
	case OPS_PKA_RSA_SIGN_ONLY:
		assert(skey->key.rsa.d);
		break;

	case OPS_PKA_DSA:
		assert(skey->key.dsa.x);
		break;

	default:
		fprintf(stderr, "Unsupported algorithm %d\n", skey->public_key.algorithm);
		assert(/* CONSTCOND */0);
	}

	assert(sig->hashed_data_length != (unsigned) -1);

	__ops_memory_place_int(sig->mem, sig->unhashed_count_offset,
			     l - sig->unhashed_count_offset - 2, 2);

	/* add the packet from version number to end of hashed subpackets */

	if (__ops_get_debug_level(__FILE__)) {
		fprintf(stderr, "--- Adding packet to hash from version number to hashed subpkts\n");
	}
	sig->hash.add(&sig->hash, __ops_memory_get_data(sig->mem),
		      sig->unhashed_count_offset);

	/* add final trailer */
	__ops_hash_add_int(&sig->hash, (unsigned)sig->sig.info.version, 1);
	__ops_hash_add_int(&sig->hash, 0xff, 1);
	/* +6 for version, type, pk alg, hash alg, hashed subpacket length */
	__ops_hash_add_int(&sig->hash, sig->hashed_data_length + 6, 4);

	if (__ops_get_debug_level(__FILE__)) {
		fprintf(stderr, "--- Finished adding packet to hash from version number to hashed subpkts\n");
	}
	/* XXX: technically, we could figure out how big the signature is */
	/* and write it directly to the output instead of via memory. */
	switch (skey->public_key.algorithm) {
	case OPS_PKA_RSA:
	case OPS_PKA_RSA_ENCRYPT_ONLY:
	case OPS_PKA_RSA_SIGN_ONLY:
		rsa_sign(&sig->hash, &key->key.rsa, &skey->key.rsa, sig->info);
		break;

	case OPS_PKA_DSA:
		dsa_sign(&sig->hash, &key->key.dsa, &skey->key.dsa, sig->info);
		break;

	default:
		fprintf(stderr, "Unsupported algorithm %d\n", skey->public_key.algorithm);
		assert(/*CONSTCOND*/0);
	}



	rtn = __ops_write_ptag(OPS_PTAG_CT_SIGNATURE, info);
	if (rtn != false) {
		l = __ops_memory_get_length(sig->mem);
		rtn = __ops_write_length(l, info)
			&& __ops_write(__ops_memory_get_data(sig->mem), l, info);
	}
	__ops_memory_free(sig->mem);

	if (rtn == false) {
		OPS_ERROR(&info->errors, OPS_E_W, "Cannot write signature");
	}
	return rtn;
}

/**
 * \ingroup Core_Signature
 *
 * __ops_signature_add_creation_time() adds a creation time to the signature.
 *
 * \param sig
 * \param when
 */
bool 
__ops_signature_add_creation_time(__ops_create_signature_t * sig, time_t when)
{
	return __ops_write_ss_header(5, OPS_PTAG_SS_CREATION_TIME, sig->info)
	&& __ops_write_scalar((unsigned)when, 4, sig->info);
}

/**
 * \ingroup Core_Signature
 *
 * Adds issuer's key ID to the signature
 *
 * \param sig
 * \param keyid
 */

bool 
__ops_signature_add_issuer_key_id(__ops_create_signature_t * sig,
				const unsigned char keyid[OPS_KEY_ID_SIZE])
{
	return __ops_write_ss_header(OPS_KEY_ID_SIZE + 1, OPS_PTAG_SS_ISSUER_KEY_ID, sig->info)
	&& __ops_write(keyid, OPS_KEY_ID_SIZE, sig->info);
}

/**
 * \ingroup Core_Signature
 *
 * Adds primary user ID to the signature
 *
 * \param sig
 * \param primary
 */
void 
__ops_signature_add_primary_user_id(__ops_create_signature_t * sig,
				  bool primary)
{
	__ops_write_ss_header(2, OPS_PTAG_SS_PRIMARY_USER_ID, sig->info);
	__ops_write_scalar(primary, 1, sig->info);
}

/**
 * \ingroup Core_Signature
 *
 * Get the hash structure in use for the signature.
 *
 * \param sig The signature structure.
 * \return The hash structure.
 */
__ops_hash_t     *
__ops_signature_get_hash(__ops_create_signature_t * sig)
{
	return &sig->hash;
}

static int 
open_output_file(__ops_create_info_t ** cinfo, const char *input_filename, const char *output_filename, const bool use_armour, const bool overwrite)
{
	int             fd_out;

	/* setup output file */

	if (output_filename) {
		fd_out = __ops_setup_file_write(cinfo, output_filename, overwrite);
	} else {
		char           *myfilename = NULL;
		unsigned        filenamelen = strlen(input_filename) + 4 + 1;
		myfilename = calloc(1, filenamelen);
		if (use_armour)
			snprintf(myfilename, filenamelen, "%s.asc", input_filename);
		else
			snprintf(myfilename, filenamelen, "%s.gpg", input_filename);
		fd_out = __ops_setup_file_write(cinfo, myfilename, overwrite);
		free(myfilename);
	}

	return fd_out;
}

/**
   \ingroup HighLevel_Sign
   \brief Sign a file with a Cleartext Signature
   \param input_filename Name of file to be signed
   \param output_filename Filename to be created. If NULL, filename will be constructed from the input_filename.
   \param skey Secret Key to sign with
   \param overwrite Allow output file to be overwritten, if set
   \return true if OK, else false

   Example code:
   \code
   void example(const __ops_secret_key_t *skey, bool overwrite)
   {
   if (__ops_sign_file_as_cleartext("mytestfile.txt",NULL,skey,overwrite)==true)
       printf("OK");
   else
       printf("ERR");
   }
   \endcode
*/
bool 
__ops_sign_file_as_cleartext(const char *input_filename, const char *output_filename, const __ops_secret_key_t * skey, const bool overwrite)
{
	/* \todo allow choice of hash algorithams */
	/* enforce use of SHA1 for now */

	unsigned char   keyid[OPS_KEY_ID_SIZE];
	__ops_create_signature_t *sig = NULL;

	int             fd_in = 0;
	int             fd_out = 0;
	__ops_create_info_t *cinfo = NULL;
	unsigned char   buf[MAXBUF];
	/* int flags=0; */
	bool   rtn = false;
	bool   use_armour = true;

	/* open file to sign */
#ifdef O_BINARY
	fd_in = open(input_filename, O_RDONLY | O_BINARY);
#else
	fd_in = open(input_filename, O_RDONLY);
#endif
	if (fd_in < 0) {
		return false;
	}
	/* set up output file */

	fd_out = open_output_file(&cinfo, input_filename, output_filename, use_armour, overwrite);

	if (fd_out < 0) {
		close(fd_in);
		return false;
	}
	/* set up signature */
	sig = __ops_create_signature_new();
	if (!sig) {
		close(fd_in);
		__ops_teardown_file_write(cinfo, fd_out);
		return false;
	}
	/* \todo could add more error detection here */
	__ops_signature_start_cleartext_signature(sig, skey, OPS_HASH_SHA1, OPS_SIG_BINARY);
	if (__ops_writer_push_clearsigned(cinfo, sig) != true) {
		return false;
	}
	/* Do the signing */

	for (;;) {
		int             n = 0;

		n = read(fd_in, buf, sizeof(buf));
		if (!n)
			break;
		assert(n >= 0);
		__ops_write(buf, (unsigned)n, cinfo);
	}
	close(fd_in);

	/* add signature with subpackets: */
	/* - creation time */
	/* - key id */
	rtn = __ops_writer_switch_to_armoured_signature(cinfo)
		&& __ops_signature_add_creation_time(sig, time(NULL));
	if (rtn == false) {
		__ops_teardown_file_write(cinfo, fd_out);
		return false;
	}
	__ops_keyid(keyid, OPS_KEY_ID_SIZE, OPS_KEY_ID_SIZE, &skey->public_key);

	rtn = __ops_signature_add_issuer_key_id(sig, keyid)
		&& __ops_signature_hashed_subpackets_end(sig)
		&& __ops_write_signature(sig, &skey->public_key, skey, cinfo);

	__ops_teardown_file_write(cinfo, fd_out);

	if (rtn == false) {
		OPS_ERROR(&cinfo->errors, OPS_E_W, "Cannot sign file as cleartext");
	}
	return rtn;
}

#if 0
/* XXX commented out until I work out what header file to put this one in */
/**
 * \ingroup HighLevel_Sign
 * \brief Sign a buffer with a Cleartext signature
 * \param cleartext Text to be signed
 * \param len Length of text
 * \param signed_cleartext __ops_memory_t struct in which to write the signed cleartext
 * \param skey Secret key with which to sign the cleartext
 * \return true if OK; else false

 * \note It is the calling function's responsibility to free signed_cleartext
 * \note signed_cleartext should be a NULL pointer when passed in

 Example code:
 \code
 void example(const __ops_secret_key_t *skey)
 {
   __ops_memory_t* mem=NULL;
   const char* buf="Some example text";
   size_t len=strlen(buf);
   if (__ops_sign_buf_as_cleartext(buf,len, &mem, skey)==true)
     printf("OK");
   else
     printf("ERR");
   // free signed cleartext after use
   __ops_memory_free(mem);
 }
 \endcode
 */
bool 
__ops_sign_buf_as_cleartext(const char *cleartext, const size_t len, __ops_memory_t ** signed_cleartext, const __ops_secret_key_t * skey)
{
	bool   rtn = false;

	/* \todo allow choice of hash algorithams */
	/* enforce use of SHA1 for now */

	unsigned char   keyid[OPS_KEY_ID_SIZE];
	__ops_create_signature_t *sig = NULL;

	__ops_create_info_t *cinfo = NULL;

	assert(*signed_cleartext == NULL);

	/* set up signature */
	sig = __ops_create_signature_new();
	if (!sig) {
		return false;
	}
	/* \todo could add more error detection here */
	__ops_signature_start_cleartext_signature(sig, skey, OPS_HASH_SHA1, OPS_SIG_BINARY);

	/* set up output file */
	__ops_setup_memory_write(&cinfo, signed_cleartext, len);

	/* Do the signing */
	/* add signature with subpackets: */
	/* - creation time */
	/* - key id */
	rtn = __ops_writer_push_clearsigned(cinfo, sig)
		&& __ops_write(cleartext, len, cinfo)
		&& __ops_writer_switch_to_armoured_signature(cinfo)
		&& __ops_signature_add_creation_time(sig, time(NULL));

	if (rtn == false) {
		return false;
	}
	__ops_keyid(keyid, OPS_KEY_ID_SIZE, OPS_KEY_ID_SIZE, &skey->public_key);

	rtn = __ops_signature_add_issuer_key_id(sig, keyid)
		&& __ops_signature_hashed_subpackets_end(sig)
		&& __ops_write_signature(sig, &skey->public_key, skey, cinfo)
		&& __ops_writer_close(cinfo);

	/* Note: the calling function must free signed_cleartext */
	__ops_create_info_delete(cinfo);

	return rtn;
}
#endif

/**
\ingroup HighLevel_Sign
\brief Sign a file
\param input_filename Input filename
\param output_filename Output filename. If NULL, a name is constructed from the input filename.
\param skey Secret Key to use for signing
\param use_armour Write armoured text, if set.
\param overwrite May overwrite existing file, if set.
\return true if OK; else false;

Example code:
\code
void example(const __ops_secret_key_t *skey)
{
  const char* filename="mytestfile";
  const bool use_armour=false;
  const bool overwrite=false;
  if (__ops_sign_file(filename, NULL, skey, use_armour, overwrite)==true)
    printf("OK");
  else
    printf("ERR");
}
\endcode
*/
bool 
__ops_sign_file(const char *input_filename, const char *output_filename, const __ops_secret_key_t * skey, const bool use_armour, const bool overwrite)
{
	/* \todo allow choice of hash algorithams */
	/* enforce use of SHA1 for now */

	unsigned char   keyid[OPS_KEY_ID_SIZE];
	__ops_create_signature_t *sig = NULL;

	int             fd_out = 0;
	__ops_create_info_t *cinfo = NULL;

	__ops_hash_algorithm_t hash_alg = OPS_HASH_SHA1;
	__ops_sig_type_t  sig_type = OPS_SIG_BINARY;

	__ops_memory_t   *mem_buf = NULL;
	__ops_hash_t     *hash = NULL;

	/* read input file into buf */

	int             errnum;
	mem_buf = __ops_write_mem_from_file(input_filename, &errnum);
	if (errnum)
		return false;

	/* setup output file */

	fd_out = open_output_file(&cinfo, input_filename, output_filename, use_armour, overwrite);

	if (fd_out < 0) {
		__ops_memory_free(mem_buf);
		return false;
	}
	/* set up signature */
	sig = __ops_create_signature_new();
	__ops_signature_start_message_signature(sig, skey, hash_alg, sig_type);

	/* set armoured/not armoured here */
	if (use_armour)
		__ops_writer_push_armoured_message(cinfo);

	if (__ops_get_debug_level(__FILE__)) {
		fprintf(stderr, "** Writing out one pass sig\n");
	}
	/* write one_pass_sig */
	__ops_write_one_pass_sig(skey, hash_alg, sig_type, cinfo);

	/* hash file contents */
	hash = __ops_signature_get_hash(sig);
	hash->add(hash, __ops_memory_get_data(mem_buf), __ops_memory_get_length(mem_buf));

	/* output file contents as Literal Data packet */

	if (__ops_get_debug_level(__FILE__)) {
		fprintf(stderr, "** Writing out data now\n");
	}
	__ops_write_literal_data_from_buf(__ops_memory_get_data(mem_buf),
		(const int)__ops_memory_get_length(mem_buf),
		OPS_LDT_BINARY, cinfo);

	if (__ops_get_debug_level(__FILE__)) {
		fprintf(stderr, "** After Writing out data now\n");
	}
	/* add subpackets to signature */
	/* - creation time */
	/* - key id */

	__ops_signature_add_creation_time(sig, time(NULL));

	__ops_keyid(keyid, OPS_KEY_ID_SIZE, OPS_KEY_ID_SIZE, &skey->public_key);
	__ops_signature_add_issuer_key_id(sig, keyid);

	__ops_signature_hashed_subpackets_end(sig);

	/* write out sig */
	__ops_write_signature(sig, &skey->public_key, skey, cinfo);

	__ops_teardown_file_write(cinfo, fd_out);

	/* tidy up */
	__ops_create_signature_delete(sig);
	__ops_memory_free(mem_buf);

	return true;
}

#if 0
/* XXX commented out until I work out what header file to put this one in */
/**
\ingroup HighLevel_Sign
\brief Signs a buffer
\param input Input text to be signed
\param input_len Length of input text
\param sig_type Signature type
\param skey Secret Key
\param use_armour Write armoured text, if set
\return New __ops_memory_t struct containing signed text
\note It is the caller's responsibility to call __ops_memory_free(me)

Example Code:
\code
void example(const __ops_secret_key_t *skey)
{
  const char* buf="Some example text";
  const size_t len=strlen(buf);
  const bool use_armour=true;

  __ops_memory_t* mem=NULL;

  mem=__ops_sign_buf(buf,len,OPS_SIG_BINARY,skey,use_armour);
  if (mem)
  {
    printf ("OK");
    __ops_memory_free(mem);
  }
  else
  {
    printf("ERR");
  }
}
\endcode
*/
__ops_memory_t   *
__ops_sign_buf(const void *input, const size_t input_len, const __ops_sig_type_t sig_type, const __ops_secret_key_t * skey, const bool use_armour)
{
	/* \todo allow choice of hash algorithams */
	/* enforce use of SHA1 for now */

	unsigned char   keyid[OPS_KEY_ID_SIZE];
	__ops_create_signature_t *sig = NULL;

	__ops_create_info_t *cinfo = NULL;
	__ops_memory_t   *mem = __ops_memory_new();

	__ops_hash_algorithm_t hash_alg = OPS_HASH_SHA1;
	__ops_literal_data_type_t ld_type;
	__ops_hash_t     *hash = NULL;

	/* setup literal data packet type */
	if (sig_type == OPS_SIG_BINARY)
		ld_type = OPS_LDT_BINARY;
	else
		ld_type = OPS_LDT_TEXT;

	/* set up signature */
	sig = __ops_create_signature_new();
	__ops_signature_start_message_signature(sig, skey, hash_alg, sig_type);

	/* setup writer */
	__ops_setup_memory_write(&cinfo, &mem, input_len);

	/* set armoured/not armoured here */
	if (use_armour)
		__ops_writer_push_armoured_message(cinfo);

	if (__ops_get_debug_level(__FILE__)) {
		fprintf(stderr, "** Writing out one pass sig\n");
	}
	/* write one_pass_sig */
	__ops_write_one_pass_sig(skey, hash_alg, sig_type, cinfo);

	/* hash file contents */
	hash = __ops_signature_get_hash(sig);
	hash->add(hash, input, input_len);

	/* output file contents as Literal Data packet */

	if (__ops_get_debug_level(__FILE__)) {
		fprintf(stderr, "** Writing out data now\n");
	}
	__ops_write_literal_data_from_buf(input, input_len, ld_type, cinfo);

	if (__ops_get_debug_level(__FILE__)) {
		fprintf(stderr, "** After Writing out data now\n");
	}
	/* add subpackets to signature */
	/* - creation time */
	/* - key id */

	__ops_signature_add_creation_time(sig, time(NULL));

	__ops_keyid(keyid, OPS_KEY_ID_SIZE, OPS_KEY_ID_SIZE, &skey->public_key);
	__ops_signature_add_issuer_key_id(sig, keyid);

	__ops_signature_hashed_subpackets_end(sig);

	/* write out sig */
	__ops_write_signature(sig, &skey->public_key, skey, cinfo);

	/* tidy up */
	__ops_writer_close(cinfo);
	__ops_create_signature_delete(sig);

	return mem;
}
#endif
