/*
 * Copyright (c) 2011 Rustam Kovhaev. All rights reserved.
 * Copyright (c) 2021 Eivind Næss. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The name(s) of the authors of this software must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission.
 *
 * THE AUTHORS OF THIS SOFTWARE DISCLAIM ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * NOTES:
 *
 * PEAP has 2 phases,
 * 1 - Outer EAP, where TLS session gets established
 * 2 - Inner EAP, where inside TLS session with EAP MSCHAPV2 auth, or any other auth
 *
 * And so protocols encapsulation looks like this:
 * Outer EAP -> TLS -> Inner EAP -> MSCHAPV2
 * PEAP can compress an inner EAP packet prior to encapsulating it within
 * the Data field of a PEAP packet by removing its Code, Identifier,
 * and Length fields, and Microsoft PEAP server/client always does that
 *
 * Current implementation does not support:
 * a) Fast reconnect
 * b) Inner EAP fragmentation
 * c) Any other auth other than MSCHAPV2
 *
 * For details on the PEAP protocol, look to Microsoft:
 *    https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-peap
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <openssl/opensslv.h>
#include <openssl/ssl.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/err.h>

#include "pppd-private.h"
#include "eap.h"
#include "tls.h"
#include "chap.h"
#include "chap_ms.h"
#include "mppe.h"
#include "peap.h"

#ifdef UNIT_TEST
#define novm(x)
#endif

struct peap_state {
	SSL_CTX *ctx;
	SSL *ssl;
	BIO *in_bio;
	BIO *out_bio;

	int phase;
	int written, read;
	u_char *in_buf;
	u_char *out_buf;

	u_char ipmk[PEAP_TLV_IPMK_LEN];
	u_char tk[PEAP_TLV_TK_LEN];
	u_char nonce[PEAP_TLV_NONCE_LEN];
	struct tls_info *info;
#ifdef PPP_WITH_CHAPMS
	struct chap_digest_type *chap;
#endif
};

/*
 * K = Key, S = Seed, LEN = output length
 * PRF+(K, S, LEN) = T1 | T2 | ... |Tn
 * Where:
 * T1 = HMAC-SHA1 (K, S | 0x01 | 0x00 | 0x00)
 * T2 = HMAC-SHA1 (K, T1 | S | 0x02 | 0x00 | 0x00)
 * ...
 * Tn = HMAC-SHA1 (K, Tn-1 | S | n | 0x00 | 0x00)
 * As shown, PRF+ is computed in iterations. The number of iterations (n)
 * depends on the output length (LEN).
 */
static void peap_prfplus(u_char *seed, size_t seed_len, u_char *key, size_t key_len, u_char *out_buf, size_t pfr_len)
{
	int pos;
	u_char *buf, *hash;
	size_t max_iter, i, j, k;
	u_int len;

	max_iter = (pfr_len + SHA_DIGEST_LENGTH - 1) / SHA_DIGEST_LENGTH;
	buf = malloc(seed_len + max_iter * SHA_DIGEST_LENGTH);
	if (!buf)
		novm("pfr buffer");
	hash = malloc(pfr_len + SHA_DIGEST_LENGTH);
	if (!hash)
		novm("hash buffer");

	for (i = 0; i < max_iter; i++) {
		j = 0;
		k = 0;

		if (i > 0)
			j = SHA_DIGEST_LENGTH;
		for (k = 0; k < seed_len; k++)
			buf[j + k] = seed[k];
		pos = j + k;
		buf[pos] = i + 1;
		pos++;
		buf[pos] = 0x00;
		pos++;
		buf[pos] = 0x00;
		pos++;
		if (!HMAC(EVP_sha1(), key, key_len, buf, pos, (hash + i * SHA_DIGEST_LENGTH), &len))
			fatal("HMAC() failed");
		for (j = 0; j < SHA_DIGEST_LENGTH; j++)
			buf[j] = hash[i * SHA_DIGEST_LENGTH + j];
	}
	BCOPY(hash, out_buf, pfr_len);
	free(hash);
	free(buf);
}

static void generate_cmk(u_char *ipmk, u_char *tempkey, u_char *nonce, u_char *tlv_response_out, int client)
{
	const char *label = PEAP_TLV_IPMK_SEED_LABEL;
	u_char data_tlv[PEAP_TLV_DATA_LEN] = {0};
	u_char isk[PEAP_TLV_ISK_LEN] = {0};
	u_char ipmkseed[PEAP_TLV_IPMKSEED_LEN] = {0};
	u_char cmk[PEAP_TLV_CMK_LEN] = {0};
	u_char buf[PEAP_TLV_CMK_LEN + PEAP_TLV_IPMK_LEN] = {0};
	u_char compound_mac[PEAP_TLV_COMP_MAC_LEN] = {0};
	u_int len;

	/* format outgoing CB TLV response packet */
	data_tlv[1] = PEAP_TLV_TYPE;
	data_tlv[3] = PEAP_TLV_LENGTH_FIELD;
	if (client)
		data_tlv[7] = PEAP_TLV_SUBTYPE_RESPONSE;
	else
		data_tlv[7] = PEAP_TLV_SUBTYPE_REQUEST;
	BCOPY(nonce, (data_tlv + PEAP_TLV_HEADERLEN), PEAP_TLV_NONCE_LEN);
	data_tlv[60] = EAPT_PEAP;

#ifdef PPP_WITH_MPPE
	mppe_get_send_key(isk, MPPE_MAX_KEY_LEN);
	mppe_get_recv_key(isk + MPPE_MAX_KEY_LEN, MPPE_MAX_KEY_LEN);
#endif

	BCOPY(label, ipmkseed, strlen(label));
	BCOPY(isk, ipmkseed + strlen(label), PEAP_TLV_ISK_LEN);
	peap_prfplus(ipmkseed, PEAP_TLV_IPMKSEED_LEN,
			tempkey, PEAP_TLV_TEMPKEY_LEN, buf, PEAP_TLV_CMK_LEN + PEAP_TLV_IPMK_LEN);

	BCOPY(buf, ipmk, PEAP_TLV_IPMK_LEN);
	BCOPY(buf + PEAP_TLV_IPMK_LEN, cmk, PEAP_TLV_CMK_LEN);
	if (!HMAC(EVP_sha1(), cmk, PEAP_TLV_CMK_LEN, data_tlv, PEAP_TLV_DATA_LEN, compound_mac, &len))
		fatal("HMAC() failed");
	BCOPY(compound_mac, data_tlv + PEAP_TLV_HEADERLEN + PEAP_TLV_NONCE_LEN, PEAP_TLV_COMP_MAC_LEN);
	/* do not copy last byte to response packet */
	BCOPY(data_tlv, tlv_response_out, PEAP_TLV_DATA_LEN - 1);
}

static void verify_compound_mac(struct peap_state *psm, u_char *in_buf)
{
	u_char nonce[PEAP_TLV_NONCE_LEN] = {0};
	u_char out_buf[PEAP_TLV_LEN] = {0};

	BCOPY(in_buf, nonce, PEAP_TLV_NONCE_LEN);
	generate_cmk(psm->ipmk, psm->tk, nonce, out_buf, 0);
	if (memcmp((in_buf + PEAP_TLV_NONCE_LEN), (out_buf + PEAP_TLV_HEADERLEN + PEAP_TLV_NONCE_LEN), PEAP_TLV_CMK_LEN))
			fatal("server's CMK does not match client's CMK, potential MiTM");
}

#ifdef PPP_WITH_MPPE
#define PEAP_MPPE_KEY_LEN 32

static void generate_mppe_keys(u_char *ipmk, int client)
{
	const char *label = PEAP_TLV_CSK_SEED_LABEL;
	u_char csk[PEAP_TLV_CSK_LEN] = {0};
	size_t len;

	dbglog("PEAP CB: generate mppe keys");
	len = strlen(label);
	len++; /* CSK requires NULL byte in seed */
	peap_prfplus((u_char *)label, len, ipmk, PEAP_TLV_IPMK_LEN, csk, PEAP_TLV_CSK_LEN);

	/*
	 * The first 64 bytes of the CSK are split into two MPPE keys, as follows.
	 *
	 * +-----------------------+------------------------+
	 * | First 32 bytes of CSK | Second 32 bytes of CSK |
	 * +-----------------------+------------------------+
	 * | MS-MPPE-Send-Key      | MS-MPPE-Recv-Key       |
	 * +-----------------------+------------------------+
	 */
	if (client) {
		mppe_set_keys(csk, csk + PEAP_MPPE_KEY_LEN, PEAP_MPPE_KEY_LEN);
	} else {
		mppe_set_keys(csk + PEAP_MPPE_KEY_LEN, csk, PEAP_MPPE_KEY_LEN);
	}
}

#endif

#ifndef UNIT_TEST

static void peap_ack(eap_state *esp, u_char id)
{
	u_char *outp;

	outp = outpacket_buf;
	MAKEHEADER(outp, PPP_EAP);
	PUTCHAR(EAP_RESPONSE, outp);
	PUTCHAR(id, outp);
	esp->es_client.ea_id = id;
	PUTSHORT(PEAP_HEADERLEN, outp);
	PUTCHAR(EAPT_PEAP, outp);
	PUTCHAR(PEAP_FLAGS_ACK, outp);
	output(esp->es_unit, outpacket_buf, PPP_HDRLEN + PEAP_HEADERLEN);
}

static void peap_response(eap_state *esp, u_char id, u_char *buf, int len)
{
	struct peap_state *psm = esp->ea_peap;
	u_char *outp;
	int peap_len;

	outp = outpacket_buf;
	MAKEHEADER(outp, PPP_EAP);
	PUTCHAR(EAP_RESPONSE, outp);
	PUTCHAR(id, outp);
	esp->es_client.ea_id = id;

	if (psm->phase == PEAP_PHASE_1)
		peap_len = PEAP_HEADERLEN + PEAP_FRAGMENT_LENGTH_FIELD + len;
	else
		peap_len = PEAP_HEADERLEN + len;

	PUTSHORT(peap_len, outp);
	PUTCHAR(EAPT_PEAP, outp);

	if (psm->phase == PEAP_PHASE_1) {
		PUTCHAR(PEAP_L_FLAG_SET, outp);
		PUTLONG(len, outp);
	} else
		PUTCHAR(PEAP_NO_FLAGS, outp);

	BCOPY(buf, outp, len);
	output(esp->es_unit, outpacket_buf, PPP_HDRLEN + peap_len);
}

void peap_do_inner_eap(u_char *in_buf, int in_len, eap_state *esp, int id,
		u_char *out_buf, int *out_len)
{
	struct peap_state *psm = esp->ea_peap;
	int used = 0;
	int typenum;
	int secret_len;
	char secret[MAXSECRETLEN + 1];
	char rhostname[MAXWORDLEN];
	u_char *outp = out_buf;

	dbglog("PEAP: EAP (in): %.*B", in_len, in_buf);

	if (*(in_buf + EAP_HEADERLEN) == PEAP_CAPABILITIES_TYPE &&
			in_len  == (EAP_HEADERLEN + PEAP_CAPABILITIES_LEN)) {
		/* use original packet as template for response */
		BCOPY(in_buf, outp, EAP_HEADERLEN + PEAP_CAPABILITIES_LEN);
		PUTCHAR(EAP_RESPONSE, outp);
		PUTCHAR(id, outp);
		/* change last byte to 0 to disable fragmentation */
		*(outp + PEAP_CAPABILITIES_LEN + 1) = 0x00;
		used = EAP_HEADERLEN + PEAP_CAPABILITIES_LEN;
		goto done;
	}
	if (*(in_buf + EAP_HEADERLEN + PEAP_TLV_HEADERLEN) == PEAP_TLV_TYPE &&
			in_len == PEAP_TLV_LEN) {
		/* PEAP TLV message, do cryptobinding */
		SSL_export_keying_material(psm->ssl, psm->tk, PEAP_TLV_TK_LEN,
				PEAP_TLV_TK_SEED_LABEL, strlen(PEAP_TLV_TK_SEED_LABEL), NULL, 0, 0);
		/* verify server's CMK */
		verify_compound_mac(psm, in_buf + EAP_HEADERLEN + PEAP_TLV_RESULT_LEN + PEAP_TLV_HEADERLEN);
		/* generate client's CMK with new nonce */
		PUTCHAR(EAP_RESPONSE, outp);
		PUTCHAR(id, outp);
		PUTSHORT(PEAP_TLV_LEN, outp);
		BCOPY(in_buf + EAP_HEADERLEN, outp, PEAP_TLV_RESULT_LEN);
		outp = outp + PEAP_TLV_RESULT_LEN;
		RAND_bytes(psm->nonce, PEAP_TLV_NONCE_LEN);
		generate_cmk(psm->ipmk, psm->tk, psm->nonce, outp, 1);
#ifdef PPP_WITH_MPPE
		/* set mppe keys */
		generate_mppe_keys(psm->ipmk, 1);
#endif
		used = PEAP_TLV_LEN;
		goto done;
	}

	GETCHAR(typenum, in_buf);
	in_len--;

	switch (typenum) {
	case EAPT_IDENTITY:
		/* Respond with our identity to the peer */
		PUTCHAR(EAPT_IDENTITY, outp);
		BCOPY(esp->es_client.ea_name, outp,
				esp->es_client.ea_namelen);
		used += (esp->es_client.ea_namelen + 1);
		break;

	case EAPT_TLS:
		/* Send NAK to EAP_TLS request */
		PUTCHAR(EAPT_NAK, outp);
		PUTCHAR(EAPT_MSCHAPV2, outp);
		used += 2;
		break;

#if PPP_WITH_CHAPMS
	case EAPT_MSCHAPV2: {

		// Must have at least 4 more bytes to process CHAP header
		if (in_len < 4) {
			error("PEAP: received invalid MSCHAPv2 packet, too short");
			break;
		}

		u_char opcode;
		GETCHAR(opcode, in_buf);

		u_char chap_id;
		GETCHAR(chap_id, in_buf);

		short mssize;
		GETSHORT(mssize, in_buf);

		// Validate the CHAP packet (including header)
		if (in_len != mssize) {
			error("PEAP: received invalid MSCHAPv2 packet, invalid length");
			break;
		}
		in_len -= 4;

		switch (opcode) {
		case CHAP_CHALLENGE: {

			u_char *challenge = in_buf;	// VLEN + VALUE
			u_char vsize;

			GETCHAR(vsize, in_buf);
			in_len -= 1;

			if (vsize != MS_CHAP2_PEER_CHAL_LEN || in_len < MS_CHAP2_PEER_CHAL_LEN) {
				error("PEAP: received invalid MSCHAPv2 packet, invalid value-length: %d", vsize);
				goto done;
			}

			INCPTR(MS_CHAP2_PEER_CHAL_LEN, in_buf);
			in_len -= MS_CHAP2_PEER_CHAL_LEN;

			// Copy the provided remote host name
			rhostname[0] = '\0';
			if (in_len > 0) {
				if (in_len >= sizeof(rhostname)) {
					dbglog("PEAP: trimming really long peer name down");
					in_len = sizeof(rhostname) - 1;
				}
				BCOPY(in_buf, rhostname, in_len);
				rhostname[in_len] = '\0';
			}

			// In case the remote doesn't give us his name, or user explictly specified remotename is config
			if (explicit_remote || (remote_name[0] != '\0' && in_len == 0))
				strlcpy(rhostname, remote_name, sizeof(rhostname));

			// Get the scrert for authenticating ourselves with the specified host
			if (get_secret(esp->es_unit, esp->es_client.ea_name,
						rhostname, secret, &secret_len, 0)) {

				u_char response[MS_CHAP2_RESPONSE_LEN+1];
				u_char user_len = esp->es_client.ea_namelen;
				char *user = esp->es_client.ea_name;

				psm->chap->make_response(response, chap_id, user,
						challenge, secret, secret_len, NULL);

				PUTCHAR(EAPT_MSCHAPV2, outp);
				PUTCHAR(CHAP_RESPONSE, outp);
				PUTCHAR(chap_id, outp);
				PUTCHAR(0, outp);
				PUTCHAR(5 + user_len + MS_CHAP2_RESPONSE_LEN, outp);
				BCOPY(response, outp, MS_CHAP2_RESPONSE_LEN+1);	// VLEN + VALUE
				INCPTR(MS_CHAP2_RESPONSE_LEN+1, outp);
				BCOPY(user, outp, user_len);
				used = 5 + user_len + MS_CHAP2_RESPONSE_LEN + 1;

			} else {
				dbglog("PEAP: no CHAP secret for auth to %q", rhostname);
				PUTCHAR(EAPT_NAK, outp);
				++used;
			}
			break;
		}
		case CHAP_SUCCESS: {

			u_char status = CHAP_FAILURE;
			if (psm->chap->check_success(chap_id, in_buf, in_len)) {
				info("Chap authentication succeeded! %.*v", in_len, in_buf);
				status = CHAP_SUCCESS;
			}

			PUTCHAR(EAPT_MSCHAPV2, outp);
			PUTCHAR(status, outp);
			used += 2;
			break;
		}
		case CHAP_FAILURE: {

			u_char status = CHAP_FAILURE;
			psm->chap->handle_failure(in_buf, in_len);
			PUTCHAR(EAPT_MSCHAPV2, outp);
			PUTCHAR(status, outp);
			used += 2;
			break;
		}
		default:
			break;
		}
		break;
	}	// EAPT_MSCHAPv2
#endif
	default:

		/* send compressed EAP NAK for any unknown packet */
		PUTCHAR(EAPT_NAK, outp);
		++used;
	}

done:

	dbglog("PEAP: EAP (out): %.*B", used, psm->out_buf);
	*out_len = used;
}

int peap_init(struct peap_state **ctx, const char *rhostname)
{
	const SSL_METHOD *method;

	if (!ctx)
		return -1;

	tls_init();

	struct peap_state *psm = malloc(sizeof(*psm));
	if (!psm)
		novm("peap psm struct");
	psm->in_buf = malloc(TLS_RECORD_MAX_SIZE);
	if (!psm->in_buf)
		novm("peap tls buffer");
	psm->out_buf = malloc(TLS_RECORD_MAX_SIZE);
	if (!psm->out_buf)
		novm("peap tls buffer");
	method = tls_method();
	if (!method)
		novm("TLS_method() failed");
	psm->ctx = SSL_CTX_new(method);
	if (!psm->ctx)
		novm("SSL_CTX_new() failed");

	/* Configure the default options */
	tls_set_opts(psm->ctx);

	/* Configure the max TLS version */
	tls_set_version(psm->ctx, max_tls_version);

	/* Configure the peer certificate callback */
	tls_set_verify(psm->ctx, 5);

	/* Configure CA locations */
	if (tls_set_ca(psm->ctx, ca_path, cacert_file)) {
		fatal("Could not set CA verify locations");
	}

	/* Configure CRL check (if any) */
	if (tls_set_crl(psm->ctx, crl_dir, crl_file)) {
		fatal("Could not set CRL verify locations");
	}

	psm->out_bio = BIO_new(BIO_s_mem());
	psm->in_bio = BIO_new(BIO_s_mem());
	BIO_set_mem_eof_return(psm->out_bio, -1);
	BIO_set_mem_eof_return(psm->in_bio, -1);
	psm->ssl = SSL_new(psm->ctx);
	SSL_set_bio(psm->ssl, psm->in_bio, psm->out_bio);
	SSL_set_connect_state(psm->ssl);
	psm->phase = PEAP_PHASE_1;
	tls_set_verify_info(psm->ssl, explicit_remote ? rhostname : NULL, NULL, 1, &psm->info);
	psm->chap = chap_find_digest(CHAP_MICROSOFT_V2);
	*ctx = psm;
	return 0;
}

void peap_finish(struct peap_state **psm) {

	if (psm && *psm) {
		struct peap_state *tmp = *psm;

		if (tmp->ssl)
			SSL_free(tmp->ssl);

		if (tmp->ctx)
			SSL_CTX_free(tmp->ctx);

		if (tmp->info)
			tls_free_verify_info(&tmp->info);

		// NOTE: BIO and memory is freed as a part of SSL_free()

		free(*psm);
		*psm = NULL;
	}
}

int peap_process(eap_state *esp, u_char id, u_char *inp, int len)
{
	int ret;
	int out_len;

	struct peap_state *psm = esp->ea_peap;

	if (esp->es_client.ea_id == id) {
		info("PEAP: retransmits are not supported..");
		return -1;
	}

	switch (*inp) {
	case PEAP_S_FLAG_SET:
		dbglog("PEAP: S bit is set, starting PEAP phase 1");
		ret = SSL_do_handshake(psm->ssl);
		if (ret != 1) {
			ret = SSL_get_error(psm->ssl, ret);
			if (ret != SSL_ERROR_WANT_READ && ret != SSL_ERROR_WANT_WRITE)
				fatal("SSL_do_handshake(): %s", ERR_error_string(ret, NULL));

		}
		psm->read = BIO_read(psm->out_bio, psm->out_buf, TLS_RECORD_MAX_SIZE);
		peap_response(esp, id, psm->out_buf, psm->read);
		break;

	case PEAP_LM_FLAG_SET:
		dbglog("PEAP TLS: LM bits are set, need to get more TLS fragments");
		inp = inp + PEAP_FRAGMENT_LENGTH_FIELD + PEAP_FLAGS_FIELD;
		psm->written = BIO_write(psm->in_bio, inp, len - PEAP_FRAGMENT_LENGTH_FIELD - PEAP_FLAGS_FIELD);
		peap_ack(esp, id);
		break;

	case PEAP_M_FLAG_SET:
		dbglog("PEAP TLS: M bit is set, need to get more TLS fragments");
		inp = inp + PEAP_FLAGS_FIELD;
		psm->written = BIO_write(psm->in_bio, inp, len - PEAP_FLAGS_FIELD);
		peap_ack(esp, id);
		break;

	case PEAP_L_FLAG_SET:
	case PEAP_NO_FLAGS:
		if (*inp == PEAP_L_FLAG_SET) {
			dbglog("PEAP TLS: L bit is set");
			inp = inp + PEAP_FRAGMENT_LENGTH_FIELD + PEAP_FLAGS_FIELD;
			psm->written = BIO_write(psm->in_bio, inp, len - PEAP_FRAGMENT_LENGTH_FIELD - PEAP_FLAGS_FIELD);
		} else {
			dbglog("PEAP TLS: all bits are off");
			inp = inp + PEAP_FLAGS_FIELD;
			psm->written = BIO_write(psm->in_bio, inp, len - PEAP_FLAGS_FIELD);
		}

		if (psm->phase == PEAP_PHASE_1) {
			dbglog("PEAP TLS: continue handshake");
			ret = SSL_do_handshake(psm->ssl);
			if (ret != 1) {
				ret = SSL_get_error(psm->ssl, ret);
				if (ret != SSL_ERROR_WANT_READ && ret != SSL_ERROR_WANT_WRITE)
					fatal("SSL_do_handshake(): %s", ERR_error_string(ret, NULL));
			}
			if (SSL_is_init_finished(psm->ssl))
				psm->phase = PEAP_PHASE_2;
			if (BIO_ctrl_pending(psm->out_bio) == 0) {
				peap_ack(esp, id);
				break;
			}
			psm->read = 0;
			psm->read = BIO_read(psm->out_bio, psm->out_buf,
					TLS_RECORD_MAX_SIZE);
			peap_response(esp, id, psm->out_buf, psm->read);
			break;
		}
		psm->read = SSL_read(psm->ssl, psm->in_buf,
				TLS_RECORD_MAX_SIZE);
		out_len = TLS_RECORD_MAX_SIZE;
		peap_do_inner_eap(psm->in_buf, psm->read, esp, id,
				psm->out_buf, &out_len);
		if (out_len > 0) {
			psm->written = SSL_write(psm->ssl, psm->out_buf, out_len);
			psm->read = BIO_read(psm->out_bio, psm->out_buf,
				TLS_RECORD_MAX_SIZE);
			peap_response(esp, id, psm->out_buf, psm->read);
		}
		break;
	}
	return 0;
}

#else

u_char outpacket_buf[255];
int debug = 1;
int error_count = 0;
int unsuccess = 0;

/**
 * Using the example in MS-PEAP, section 4.4.1.
 *	see https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-peap/5308642b-90c9-4cc4-beec-fb367325c0f9
 */
int test_cmk(u_char *ipmk) {
	u_char nonce[PEAP_TLV_NONCE_LEN] = {
		0x6C, 0x6B, 0xA3, 0x87, 0x84, 0x23, 0x74, 0x57,
		0xCC, 0xC9, 0x0B, 0x1A, 0x90, 0x8C, 0xBD, 0xF4,
		0x71, 0x1B, 0x69, 0x99, 0x4D, 0x0C, 0xFE, 0x8D,
		0x3D, 0xB4, 0x4E, 0xCB, 0xCD, 0xAD, 0x37, 0xE9
	};

	u_char tmpkey[PEAP_TLV_TEMPKEY_LEN] = {
		0x73, 0x8B, 0xB5, 0xF4, 0x62, 0xD5, 0x8E, 0x7E,
		0xD8, 0x44, 0xE1, 0xF0, 0x0D, 0x0E, 0xBE, 0x50,
		0xC5, 0x0A, 0x20, 0x50, 0xDE, 0x11, 0x99, 0x77,
		0x10, 0xD6, 0x5F, 0x45, 0xFB, 0x5F, 0xBA, 0xB7,
		0xE3, 0x18, 0x1E, 0x92, 0x4F, 0x42, 0x97, 0x38,
		// 0xDE, 0x40, 0xC8, 0x46, 0xCD, 0xF5, 0x0B, 0xCB,
		// 0xF9, 0xCE, 0xDB, 0x1E, 0x85, 0x1D, 0x22, 0x52,
		// 0x45, 0x3B, 0xDF, 0x63
	};

	u_char expected[60] = {
		0x00, 0x0C, 0x00, 0x38, 0x00, 0x00, 0x00, 0x01,
		0x6C, 0x6B, 0xA3, 0x87, 0x84, 0x23, 0x74, 0x57,
		0xCC, 0xC9, 0x0B, 0x1A, 0x90, 0x8C, 0xBD, 0xF4,
		0x71, 0x1B, 0x69, 0x99, 0x4D, 0x0C, 0xFE, 0x8D,
		0x3D, 0xB4, 0x4E, 0xCB, 0xCD, 0xAD, 0x37, 0xE9,
		0x42, 0xE0, 0x86, 0x07, 0x1D, 0x1C, 0x8B, 0x8C,
		0x8E, 0x45, 0x8F, 0x70, 0x21, 0xF0, 0x6A, 0x6E,
		0xAB, 0x16, 0xB6, 0x46
	};

	u_char inner_mppe_keys[32] = {
		0x67, 0x3E, 0x96, 0x14, 0x01, 0xBE, 0xFB, 0xA5,
		0x60, 0x71, 0x7B, 0x3B, 0x5D, 0xDD, 0x40, 0x38,
		0x65, 0x67, 0xF9, 0xF4, 0x16, 0xFD, 0x3E, 0x9D,
		0xFC, 0x71, 0x16, 0x3B, 0xDF, 0xF2, 0xFA, 0x95
	};

	u_char response[60] = {};

	// Set the inner MPPE keys (e.g. from CHAPv2)
	mppe_set_keys(inner_mppe_keys, inner_mppe_keys + 16, 16);

	// Generate and compare the response
	generate_cmk(ipmk, tmpkey, nonce, response, 1);
	if (memcmp(expected, response, sizeof(response)) != 0) {
		dbglog("Failed CMK key generation\n");
		dbglog("%.*B", sizeof(response), response);
		dbglog("%.*B", sizeof(expected), expected);
		return -1;
	}

	return 0;
}

int test_mppe(u_char *ipmk) {
	u_char outer_mppe_send_key[MPPE_MAX_KEY_SIZE] = {
		0x6A, 0x02, 0xD7, 0x82, 0x20, 0x1B, 0xC7, 0x13,
		0x8B, 0xF8, 0xEF, 0xF7, 0x33, 0xB4, 0x96, 0x97,
		0x0D, 0x7C, 0xAB, 0x30, 0x0A, 0xC9, 0x57, 0x72,
		0x78, 0xE1, 0xDD, 0xD5, 0xAE, 0xF7, 0x66, 0x97
	};

	u_char outer_mppe_recv_key[MPPE_MAX_KEY_SIZE] = {
		0x17, 0x52, 0xD4, 0xE5, 0x84, 0xA1, 0xC8, 0x95,
		0x03, 0x9B, 0x4D, 0x05, 0xE3, 0xBC, 0x9A, 0x84,
		0x84, 0xDD, 0xC2, 0xAA, 0x6E, 0x2C, 0xE1, 0x62,
		0x76, 0x5C, 0x40, 0x68, 0xBF, 0xF6, 0x5A, 0x45
	};

	u_char result[MPPE_MAX_KEY_SIZE];
	int len;

	mppe_clear_keys();

	generate_mppe_keys(ipmk, 1);

	len = mppe_get_recv_key(result, sizeof(result));
	if (len != sizeof(result)) {
		dbglog("Invalid length of resulting MPPE recv key");
		return -1;
	}

	if (memcmp(result, outer_mppe_recv_key, len) != 0) {
		dbglog("Invalid result for outer mppe recv key");
		return -1;
	}

	len = mppe_get_send_key(result, sizeof(result));
	if (len != sizeof(result)) {
		dbglog("Invalid length of resulting MPPE send key");
		return -1;
	}

	if (memcmp(result, outer_mppe_send_key, len) != 0) {
		dbglog("Invalid result for outer mppe send key");
		return -1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	 u_char ipmk[PEAP_TLV_IPMK_LEN] = {
		0x3A, 0x91, 0x1C, 0x25, 0x54, 0x73, 0xE8, 0x3E,
		0x9A, 0x0C, 0xC3, 0x33, 0xAE, 0x1F, 0x8A, 0x35,
		0xCD, 0xC7, 0x41, 0x63, 0xE7, 0xF6, 0x0F, 0x6C,
		0x65, 0xEF, 0x71, 0xC2, 0x64, 0x42, 0xAA, 0xAC,
		0xA2, 0xB6, 0xF1, 0xEB, 0x4F, 0x25, 0xEC, 0xA3,
	};
	int ret = -1;

	ret = test_cmk(ipmk);
	if (ret != 0) {
		return -1;
	}

	ret = test_mppe(ipmk);
	if (ret != 0) {
		return -1;
	}

	return 0;
}

#endif
