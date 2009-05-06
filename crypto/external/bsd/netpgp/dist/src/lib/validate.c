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
#include "config.h"

#include "packet-parse.h"
#include "packet-show.h"
#include "keyring.h"
#include "signature.h"
#include "netpgpsdk.h"

#include "readerwriter.h"
#include "netpgpdefs.h"
#include "memory.h"
#include "keyring_local.h"
#include "parse_local.h"
#include "validate.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>

#include <string.h>
#include <stdio.h>


/* Does the signed hash match the given hash? */
static          bool
check_binary_sig(const unsigned len,
		       const unsigned char *data,
		       const __ops_sig_t * sig,
		    const __ops_pubkey_t * signer __attribute__((unused)))
{
	unsigned char   hashout[OPS_MAX_HASH_SIZE];
	unsigned char   trailer[6];
	unsigned int    hashedlen;
	__ops_hash_t	hash;
	unsigned	n = 0;

	__ops_hash_any(&hash, sig->info.hash_algorithm);
	hash.init(&hash);
	hash.add(&hash, data, len);
	switch (sig->info.version) {
	case OPS_V3:
		trailer[0] = sig->info.type;
		trailer[1] = (unsigned)(sig->info.birthtime) >> 24;
		trailer[2] = (unsigned)(sig->info.birthtime) >> 16;
		trailer[3] = (unsigned)(sig->info.birthtime) >> 8;
		trailer[4] = (unsigned char)(sig->info.birthtime);
		hash.add(&hash, &trailer[0], 5);
		break;

	case OPS_V4:
		hash.add(&hash, sig->info.v4_hashed_data,
				sig->info.v4_hashed_data_length);
		trailer[0] = 0x04;	/* version */
		trailer[1] = 0xFF;
		hashedlen = sig->info.v4_hashed_data_length;
		trailer[2] = hashedlen >> 24;
		trailer[3] = hashedlen >> 16;
		trailer[4] = hashedlen >> 8;
		trailer[5] = hashedlen;
		hash.add(&hash, &trailer[0], 6);
		break;

	default:
		fprintf(stderr, "Invalid signature version %d\n",
				sig->info.version);
		return false;
	}

	n = hash.finish(&hash, hashout);
	if (__ops_get_debug_level(__FILE__)) {
		printf("check_binary_sig: hash length %" PRIsize "u\n",
			hash.size);
	}
	return __ops_check_sig(hashout, n, sig, signer);
}

static int 
keydata_reader(void *dest, size_t length, __ops_error_t ** errors,
	       __ops_reader_info_t * rinfo,
	       __ops_callback_data_t * cbinfo)
{
	validate_reader_t *reader = __ops_reader_get_arg(rinfo);

	OPS_USED(errors);
	OPS_USED(cbinfo);
	if (reader->offset == reader->key->packets[reader->packet].length) {
		reader->packet += 1;
		reader->offset = 0;
	}
	if (reader->packet == reader->key->npackets) {
		return 0;
	}

	/*
	 * we should never be asked to cross a packet boundary in a single
	 * read
	 */
	if (reader->key->packets[reader->packet].length <
			reader->offset + length) {
		(void) fprintf(stderr, "keydata_reader: weird length\n");
		return 0;
	}

	(void) memcpy(dest,
		&reader->key->packets[reader->packet].raw[reader->offset],
		length);
	reader->offset += length;

	return length;
}

static void 
free_sig_info(__ops_sig_info_t * sig)
{
	(void) free(sig->v4_hashed_data);
	(void) free(sig);
}

static void 
copy_sig_info(__ops_sig_info_t * dst,
			const __ops_sig_info_t * src)
{
	(void) memcpy(dst, src, sizeof(*src));
	dst->v4_hashed_data = calloc(1, src->v4_hashed_data_length);
	(void) memcpy(dst->v4_hashed_data, src->v4_hashed_data,
		src->v4_hashed_data_length);
}

static void 
add_sig_to_list(const __ops_sig_info_t *sig,
			__ops_sig_info_t **sigs,
			unsigned *count)
{
	if (*count == 0) {
		*sigs = calloc(*count + 1, sizeof(__ops_sig_info_t));
	} else {
		*sigs = realloc(*sigs,
				(*count + 1) * sizeof(__ops_sig_info_t));
	}
	copy_sig_info(&(*sigs)[*count], sig);
	*count += 1;
}


__ops_parse_cb_return_t
__ops_validate_key_cb(const __ops_packet_t * pkt,
			__ops_callback_data_t * cbinfo)
{
	const __ops_parser_content_union_t *content = &pkt->u;
	const __ops_keydata_t	*signer;
	validate_key_cb_t	*key = __ops_parse_cb_get_arg(cbinfo);
	__ops_error_t		**errors = __ops_parse_cb_get_errors(cbinfo);
	bool			 valid = false;

	if (__ops_get_debug_level(__FILE__)) {
		printf("%s\n", __ops_show_packet_tag(pkt->tag));
	}

	switch (pkt->tag) {
	case OPS_PTAG_CT_PUBLIC_KEY:
		if (key->pkey.version != 0) {
			(void) fprintf(stderr,
				"__ops_validate_key_cb: version bad\n");
			return OPS_FINISHED;
		}
		key->pkey = content->pubkey;
		return OPS_KEEP_MEMORY;

	case OPS_PTAG_CT_PUBLIC_SUBKEY:
		if (key->subkey.version)
			__ops_pubkey_free(&key->subkey);
		key->subkey = content->pubkey;
		return OPS_KEEP_MEMORY;

	case OPS_PTAG_CT_SECRET_KEY:
		key->skey = content->seckey;
		key->pkey = key->skey.pubkey;
		return OPS_KEEP_MEMORY;

	case OPS_PTAG_CT_USER_ID:
		if (key->user_id.user_id)
			__ops_user_id_free(&key->user_id);
		key->user_id = content->user_id;
		key->last_seen = ID;
		return OPS_KEEP_MEMORY;

	case OPS_PTAG_CT_USER_ATTRIBUTE:
		if (content->user_attribute.data.len == 0) {
			(void) fprintf(stderr,
"__ops_validate_key_cb: user attribute length 0");
			return OPS_FINISHED;
		}
		printf("user attribute, length=%d\n",
			(int) content->user_attribute.data.len);
		if (key->user_attribute.data.len)
			__ops_user_attribute_free(&key->user_attribute);
		key->user_attribute = content->user_attribute;
		key->last_seen = ATTRIBUTE;
		return OPS_KEEP_MEMORY;

	case OPS_PTAG_CT_SIGNATURE:	/* V3 sigs */
	case OPS_PTAG_CT_SIGNATURE_FOOTER:	/* V4 sigs */

		signer = __ops_keyring_find_key_by_id(key->keyring,
					 content->sig.info.signer_id);
		if (!signer) {
			add_sig_to_list(&content->sig.info,
					&key->result->unknown_sigs,
					&key->result->unknownc);
			break;
		}
		switch (content->sig.info.type) {
		case OPS_CERT_GENERIC:
		case OPS_CERT_PERSONA:
		case OPS_CERT_CASUAL:
		case OPS_CERT_POSITIVE:
		case OPS_SIG_REV_CERT:
			if (key->last_seen == ID)
				valid = __ops_check_useridcert_sig(&key->pkey,
							      &key->user_id,
							&content->sig,
				       __ops_get_pubkey(signer),
					key->rarg->key->packets[key->rarg->packet].raw);
			else
				valid = __ops_check_userattrcert_sig(&key->pkey,
						       &key->user_attribute,
							&content->sig,
				       __ops_get_pubkey(signer),
					key->rarg->key->packets[key->rarg->packet].raw);

			break;

		case OPS_SIG_SUBKEY:
			/*
			 * XXX: we should also check that the signer is the
			 * key we are validating, I think.
			 */
			valid = __ops_check_subkey_sig(&key->pkey, &key->subkey,
							&content->sig,
				       __ops_get_pubkey(signer),
			    key->rarg->key->packets[key->rarg->packet].raw);
			break;

		case OPS_SIG_DIRECT:
			valid = __ops_check_direct_sig(&key->pkey, &content->sig,
				       __ops_get_pubkey(signer),
			    key->rarg->key->packets[key->rarg->packet].raw);
			break;

		case OPS_SIG_STANDALONE:
		case OPS_SIG_PRIMARY:
		case OPS_SIG_REV_KEY:
		case OPS_SIG_REV_SUBKEY:
		case OPS_SIG_TIMESTAMP:
		case OPS_SIG_3RD_PARTY:
			OPS_ERROR_1(errors, OPS_E_UNIMPLEMENTED,
				"Sig Verification type 0x%02x not done yet\n",
				content->sig.info.type);
			break;

		default:
			OPS_ERROR_1(errors, OPS_E_UNIMPLEMENTED,
				    "Unexpected signature type 0x%02x\n",
				    	content->sig.info.type);
		}

		if (valid) {
			add_sig_to_list(&content->sig.info,
				&key->result->valid_sigs,
				&key->result->validc);
		} else {
			OPS_ERROR(errors, OPS_E_V_BAD_SIGNATURE, "Bad Sig");
			add_sig_to_list(&content->sig.info,
					&key->result->invalid_sigs,
					&key->result->invalidc);
		}
		break;

		/* ignore these */
	case OPS_PARSER_PTAG:
	case OPS_PTAG_CT_SIGNATURE_HEADER:
	case OPS_PARSER_PACKET_END:
		break;

	case OPS_PARSER_CMD_GET_SK_PASSPHRASE:
		if (key->cb_get_passphrase) {
			return key->cb_get_passphrase(pkt, cbinfo);
		}
		break;

	default:
		(void) fprintf(stderr, "unexpected tag=0x%x\n", pkt->tag);
		return OPS_FINISHED;
	}
	return OPS_RELEASE_MEMORY;
}

__ops_parse_cb_return_t
validate_data_cb(const __ops_packet_t * pkt,
			__ops_callback_data_t * cbinfo)
{
	const __ops_parser_content_union_t *content = &pkt->u;
	const __ops_keydata_t	*signer;
	validate_data_cb_t	*data = __ops_parse_cb_get_arg(cbinfo);
	__ops_error_t		**errors = __ops_parse_cb_get_errors(cbinfo);
	bool			 valid = false;

	if (__ops_get_debug_level(__FILE__)) {
		printf("validate_data_cb: %s\n",
			__ops_show_packet_tag(pkt->tag));
	}
	switch (pkt->tag) {
	case OPS_PTAG_CT_SIGNED_CLEARTEXT_HEADER:
		/*
		 * ignore - this gives us the "Armor Header" line "Hash:
		 * SHA1" or similar
		 */
		break;

	case OPS_PTAG_CT_LITERAL_DATA_HEADER:
		/* ignore */
		break;

	case OPS_PTAG_CT_LITERAL_DATA_BODY:
		data->data.litdata_body = content->litdata_body;
		data->use = LITERAL_DATA;
		__ops_memory_add(data->mem, data->data.litdata_body.data,
			       data->data.litdata_body.length);
		return OPS_KEEP_MEMORY;

	case OPS_PTAG_CT_SIGNED_CLEARTEXT_BODY:
		data->data.signed_cleartext_body = content->signed_cleartext_body;
		data->use = SIGNED_CLEARTEXT;
		__ops_memory_add(data->mem, data->data.litdata_body.data,
			       data->data.litdata_body.length);
		return OPS_KEEP_MEMORY;

	case OPS_PTAG_CT_SIGNED_CLEARTEXT_TRAILER:
		/* this gives us an __ops_hash_t struct */
		break;

	case OPS_PTAG_CT_SIGNATURE:	/* V3 sigs */
	case OPS_PTAG_CT_SIGNATURE_FOOTER:	/* V4 sigs */
		if (__ops_get_debug_level(__FILE__)) {
			unsigned i = 0;

			printf("\n*** hashed data:\n");
			for (i = 0;
			     i < content->sig.info.v4_hashed_data_length;
			     i++) {
				printf("0x%02x ",
				content->sig.info.v4_hashed_data[i]);
			}
			printf("\n");
			printf("  type=%02x signer_id=",
				content->sig.info.type);
			hexdump(content->sig.info.signer_id,
				sizeof(content->sig.info.signer_id), "");
			printf("\n");
		}
		signer = __ops_keyring_find_key_by_id(data->keyring,
					 content->sig.info.signer_id);
		if (!signer) {
			OPS_ERROR(errors, OPS_E_V_UNKNOWN_SIGNER,
					"Unknown Signer");
			add_sig_to_list(&content->sig.info,
					&data->result->unknown_sigs,
					&data->result->unknownc);
			break;
		}
		switch (content->sig.info.type) {
		case OPS_SIG_BINARY:
		case OPS_SIG_TEXT:
			valid = check_binary_sig(
					__ops_memory_get_length(data->mem),
					__ops_memory_get_data(data->mem),
					&content->sig,
					__ops_get_pubkey(signer));
			break;

		default:
			OPS_ERROR_1(errors, OPS_E_UNIMPLEMENTED,
				    "No Sig Verification type 0x%02x yet\n",
				    content->sig.info.type);
			break;

		}

		__ops_memory_free(data->mem);

		if (valid) {
			add_sig_to_list(&content->sig.info,
					&data->result->valid_sigs,
					&data->result->validc);
		} else {
			OPS_ERROR(errors, OPS_E_V_BAD_SIGNATURE,
					"Bad Signature");
			add_sig_to_list(&content->sig.info,
					&data->result->invalid_sigs,
					&data->result->invalidc);
		}
		break;

		/* ignore these */
	case OPS_PARSER_PTAG:
	case OPS_PTAG_CT_SIGNATURE_HEADER:
	case OPS_PTAG_CT_ARMOUR_HEADER:
	case OPS_PTAG_CT_ARMOUR_TRAILER:
	case OPS_PTAG_CT_ONE_PASS_SIGNATURE:
	case OPS_PARSER_PACKET_END:
		break;

	default:
		OPS_ERROR(errors, OPS_E_V_NO_SIGNATURE, "No signature");
		break;
	}
	return OPS_RELEASE_MEMORY;
}

static void 
keydata_destroyer(__ops_reader_info_t * rinfo)
{
	free(__ops_reader_get_arg(rinfo));
}

void 
__ops_keydata_reader_set(__ops_parse_info_t *pinfo,
			const __ops_keydata_t *key)
{
	validate_reader_t *data = calloc(1, sizeof(*data));

	data->key = key;
	data->packet = 0;
	data->offset = 0;

	__ops_reader_set(pinfo, keydata_reader, keydata_destroyer, data);
}

/**
 * \ingroup HighLevel_Verify
 * \brief Indicicates whether any errors were found
 * \param result Validation result to check
 * \return false if any invalid signatures or unknown signers
 	or no valid signatures; else true
 */
static bool 
validate_result_status(__ops_validation_t *val)
{
	return val->validc && !val->invalidc && !val->unknownc;
}

/**
 * \ingroup HighLevel_Verify
 * \brief Validate all signatures on a single key against the given keyring
 * \param result Where to put the result
 * \param key Key to validate
 * \param keyring Keyring to use for validation
 * \param cb_get_passphrase Callback to use to get passphrase
 * \return true if all signatures OK; else false
 * \note It is the caller's responsiblity to free result after use.
 * \sa __ops_validate_result_free()
 */
bool 
__ops_validate_key_sigs(__ops_validation_t * result,
				const __ops_keydata_t * key,
			    const __ops_keyring_t * keyring,
			    __ops_parse_cb_return_t cb_get_passphrase(const __ops_packet_t *, __ops_callback_data_t *)
)
{
	__ops_parse_info_t *pinfo;
	validate_key_cb_t carg;

	(void) memset(&carg, 0x0, sizeof(carg));
	carg.result = result;
	carg.cb_get_passphrase = cb_get_passphrase;

	pinfo = __ops_parse_info_new();
	/* __ops_parse_options(&opt,OPS_PTAG_CT_SIGNATURE,OPS_PARSE_PARSED); */

	carg.keyring = keyring;

	__ops_parse_cb_set(pinfo, __ops_validate_key_cb, &carg);
	pinfo->rinfo.accumulate = true;
	__ops_keydata_reader_set(pinfo, key);

	/* Note: Coverity incorrectly reports an error that carg.rarg */
	/* is never used. */
	carg.rarg = pinfo->rinfo.arg;

	__ops_parse(pinfo, 0);

	__ops_pubkey_free(&carg.pkey);
	if (carg.subkey.version)
		__ops_pubkey_free(&carg.subkey);
	__ops_user_id_free(&carg.user_id);
	__ops_user_attribute_free(&carg.user_attribute);

	__ops_parse_info_delete(pinfo);

	if (result->invalidc || result->unknownc || !result->validc)
		return false;
	else
		return true;
}

/**
   \ingroup HighLevel_Verify
   \param result Where to put the result
   \param ring Keyring to use
   \param cb_get_passphrase Callback to use to get passphrase
   \note It is the caller's responsibility to free result after use.
   \sa __ops_validate_result_free()
*/
bool 
__ops_validate_all_sigs(__ops_validation_t * result,
			    const __ops_keyring_t * ring,
			    __ops_parse_cb_return_t cb_get_passphrase(const __ops_packet_t *, __ops_callback_data_t *)
)
{
	int             n;

	(void) memset(result, 0x0, sizeof(*result));
	for (n = 0; n < ring->nkeys; ++n) {
		__ops_validate_key_sigs(result, &ring->keys[n], ring,
				cb_get_passphrase);
	}
	return validate_result_status(result);
}

/**
   \ingroup HighLevel_Verify
   \brief Frees validation result and associated memory
   \param result Struct to be freed
   \note Must be called after validation functions
*/
void 
__ops_validate_result_free(__ops_validation_t *result)
{
	if (result != NULL) {
		if (result->valid_sigs) {
			free_sig_info(result->valid_sigs);
		}
		if (result->invalid_sigs) {
			free_sig_info(result->invalid_sigs);
		}
		if (result->unknown_sigs) {
			free_sig_info(result->unknown_sigs);
		}
		(void) free(result);
		result = NULL;
	}
}

/**
   \ingroup HighLevel_Verify
   \brief Verifies the signatures in a signed file
   \param result Where to put the result
   \param filename Name of file to be validated
   \param armoured Treat file as armoured, if set
   \param keyring Keyring to use
   \return true if signatures validate successfully;
   	false if signatures fail or there are no signatures
   \note After verification, result holds the details of all keys which
   have passed, failed and not been recognised.
   \note It is the caller's responsiblity to call
   	__ops_validate_result_free(result) after use.
*/
bool 
__ops_validate_file(__ops_validation_t *result,
			const char *filename,
			const int armoured,
			const __ops_keyring_t *keyring)
{
	__ops_parse_info_t	*pinfo = NULL;
	validate_data_cb_t	 validation;
	struct stat		 st;
	int64_t		 	 origsize;
	char			 origfile[MAXPATHLEN];
	int			 fd;
	int			 cc;

#define SIG_OVERHEAD	284 /* depends on sig size */

	if (stat(filename, &st) < 0) {
		(void) fprintf(stderr, "can't validate \"%s\"\n", filename);
		return false;
	}
	origsize = st.st_size;
	cc = snprintf(origfile, sizeof(origfile), "%s", filename);
	if (strcmp(&origfile[cc - 4], ".sig") == 0) {
		origfile[cc - 4] = 0x0;
		if (stat(origfile, &st) == 0 &&
		    st.st_size - SIG_OVERHEAD < origsize) {
			pinfo->synthlit = strdup(origfile);
		}
	}

	fd = __ops_setup_file_read(&pinfo, filename, &validation,
				validate_data_cb, true);
	if (fd < 0) {
		return false;
	}

	/* Set verification reader and handling options */
	(void) memset(&validation, 0x0, sizeof(validation));
	validation.result = result;
	validation.keyring = keyring;
	validation.mem = __ops_memory_new();
	__ops_memory_init(validation.mem, 128);
	/* Note: Coverity incorrectly reports an error that carg.rarg */
	/* is never used. */
	validation.rarg = pinfo->rinfo.arg;

	if (armoured) {
		__ops_reader_push_dearmour(pinfo);
	}

	/* Do the verification */
	__ops_parse(pinfo, 0);

	if (__ops_get_debug_level(__FILE__)) {
		printf("valid=%d, invalid=%d, unknown=%d\n",
		       result->validc,
		       result->invalidc,
		       result->unknownc);
	}

	/* Tidy up */
	if (armoured) {
		__ops_reader_pop_dearmour(pinfo);
	}
	__ops_teardown_file_read(pinfo, fd);

	return validate_result_status(result);
}

/**
   \ingroup HighLevel_Verify
   \brief Verifies the signatures in a __ops_memory_t struct
   \param result Where to put the result
   \param mem Memory to be validated
   \param armoured Treat data as armoured, if set
   \param keyring Keyring to use
   \return true if signature validates successfully; false if not
   \note After verification, result holds the details of all keys which
   have passed, failed and not been recognised.
   \note It is the caller's responsiblity to call
   	__ops_validate_result_free(result) after use.
*/

bool 
__ops_validate_mem(__ops_validation_t *result, __ops_memory_t *mem,
			const int armoured, const __ops_keyring_t *keyring)
{
	__ops_parse_info_t *pinfo = NULL;
	validate_data_cb_t validation;

	__ops_setup_memory_read(&pinfo, mem, &validation, validate_data_cb,
			true);

	/* Set verification reader and handling options */
	(void) memset(&validation, 0x0, sizeof(validation));
	validation.result = result;
	validation.keyring = keyring;
	validation.mem = __ops_memory_new();
	__ops_memory_init(validation.mem, 128);
	/* Note: Coverity incorrectly reports an error that carg.rarg */
	/* is never used. */
	validation.rarg = pinfo->rinfo.arg;

	if (armoured) {
		__ops_reader_push_dearmour(pinfo);
	}

	/* Do the verification */
	__ops_parse(pinfo, 0);
	if (__ops_get_debug_level(__FILE__)) {
		printf("valid=%d, invalid=%d, unknown=%d\n",
		       result->validc,
		       result->invalidc,
		       result->unknownc);
	}

	/* Tidy up */
	if (armoured) {
		__ops_reader_pop_dearmour(pinfo);
	}
	__ops_teardown_memory_read(pinfo, mem);

	return validate_result_status(result);
}
