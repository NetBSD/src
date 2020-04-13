/*
 * Copyright (c) 2019 Yubico AB. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 */

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "mutator_aux.h"
#include "wiredata_fido2.h"
#include "wiredata_u2f.h"
#include "dummy.h"
#include "fido.h"

#include "../openbsd-compat/openbsd-compat.h"

#define TAG_U2F		0x01
#define TAG_TYPE	0x02
#define TAG_CDH		0x03
#define TAG_RP_ID	0x04
#define TAG_RP_NAME	0x05
#define TAG_USER_ID	0x06
#define TAG_USER_NAME	0x07
#define TAG_USER_NICK	0x08
#define TAG_USER_ICON	0x09
#define TAG_EXT		0x0a
#define TAG_SEED	0x0b
#define TAG_RK		0x0c
#define TAG_UV		0x0d
#define TAG_PIN		0x0e
#define TAG_WIRE_DATA	0x0f
#define TAG_EXCL_COUNT	0x10
#define TAG_EXCL_CRED	0x11

/* Parameter set defining a FIDO2 make credential operation. */
struct param {
	char		pin[MAXSTR];
	char		rp_id[MAXSTR];
	char		rp_name[MAXSTR];
	char		user_icon[MAXSTR];
	char		user_name[MAXSTR];
	char		user_nick[MAXSTR];
	int		ext;
	int		seed;
	struct blob	cdh;
	struct blob	excl_cred;
	struct blob	user_id;
	struct blob	wire_data;
	uint8_t		excl_count;
	uint8_t		rk;
	uint8_t		type;
	uint8_t		u2f;
	uint8_t		uv;
};

/*
 * Collection of HID reports from an authenticator issued with a FIDO2
 * make credential using the example parameters above.
 */
static const uint8_t dummy_wire_data_fido[] = {
	WIREDATA_CTAP_INIT,
	WIREDATA_CTAP_CBOR_INFO,
	WIREDATA_CTAP_CBOR_AUTHKEY,
	WIREDATA_CTAP_CBOR_PINTOKEN,
	WIREDATA_CTAP_KEEPALIVE,
	WIREDATA_CTAP_KEEPALIVE,
	WIREDATA_CTAP_KEEPALIVE,
	WIREDATA_CTAP_CBOR_CRED,
};

/*
 * Collection of HID reports from an authenticator issued with a U2F
 * registration using the example parameters above.
 */
static const uint8_t dummy_wire_data_u2f[] = {
	WIREDATA_CTAP_INIT,
	WIREDATA_CTAP_U2F_6985,
	WIREDATA_CTAP_U2F_6985,
	WIREDATA_CTAP_U2F_6985,
	WIREDATA_CTAP_U2F_6985,
	WIREDATA_CTAP_U2F_6985,
	WIREDATA_CTAP_U2F_REGISTER,
};

int    LLVMFuzzerTestOneInput(const uint8_t *, size_t);
size_t LLVMFuzzerCustomMutator(uint8_t *, size_t, size_t, unsigned int);

static int
unpack(const uint8_t *ptr, size_t len, struct param *p) NO_MSAN
{
	uint8_t **pp = (void *)&ptr;

	if (unpack_byte(TAG_RK, pp, &len, &p->rk) < 0 ||
	    unpack_byte(TAG_TYPE, pp, &len, &p->type) < 0 ||
	    unpack_byte(TAG_U2F, pp, &len, &p->u2f) < 0 ||
	    unpack_byte(TAG_UV, pp, &len, &p->uv) < 0 ||
	    unpack_byte(TAG_EXCL_COUNT, pp, &len, &p->excl_count) < 0 ||
	    unpack_string(TAG_PIN, pp, &len, p->pin) < 0 ||
	    unpack_string(TAG_RP_ID, pp, &len, p->rp_id) < 0 ||
	    unpack_string(TAG_RP_NAME, pp, &len, p->rp_name) < 0 ||
	    unpack_string(TAG_USER_ICON, pp, &len, p->user_icon) < 0 ||
	    unpack_string(TAG_USER_NAME, pp, &len, p->user_name) < 0 ||
	    unpack_string(TAG_USER_NICK, pp, &len, p->user_nick) < 0 ||
	    unpack_int(TAG_EXT, pp, &len, &p->ext) < 0 ||
	    unpack_int(TAG_SEED, pp, &len, &p->seed) < 0 ||
	    unpack_blob(TAG_CDH, pp, &len, &p->cdh) < 0 ||
	    unpack_blob(TAG_USER_ID, pp, &len, &p->user_id) < 0 ||
	    unpack_blob(TAG_WIRE_DATA, pp, &len, &p->wire_data) < 0 ||
	    unpack_blob(TAG_EXCL_CRED, pp, &len, &p->excl_cred) < 0)
		return (-1);

	return (0);
}

static size_t
pack(uint8_t *ptr, size_t len, const struct param *p)
{
	const size_t max = len;

	if (pack_byte(TAG_RK, &ptr, &len, p->rk) < 0 ||
	    pack_byte(TAG_TYPE, &ptr, &len, p->type) < 0 ||
	    pack_byte(TAG_U2F, &ptr, &len, p->u2f) < 0 ||
	    pack_byte(TAG_UV, &ptr, &len, p->uv) < 0 ||
	    pack_byte(TAG_EXCL_COUNT, &ptr, &len, p->excl_count) < 0 ||
	    pack_string(TAG_PIN, &ptr, &len, p->pin) < 0 ||
	    pack_string(TAG_RP_ID, &ptr, &len, p->rp_id) < 0 ||
	    pack_string(TAG_RP_NAME, &ptr, &len, p->rp_name) < 0 ||
	    pack_string(TAG_USER_ICON, &ptr, &len, p->user_icon) < 0 ||
	    pack_string(TAG_USER_NAME, &ptr, &len, p->user_name) < 0 ||
	    pack_string(TAG_USER_NICK, &ptr, &len, p->user_nick) < 0 ||
	    pack_int(TAG_EXT, &ptr, &len, p->ext) < 0 ||
	    pack_int(TAG_SEED, &ptr, &len, p->seed) < 0 ||
	    pack_blob(TAG_CDH, &ptr, &len, &p->cdh) < 0 ||
	    pack_blob(TAG_USER_ID, &ptr, &len, &p->user_id) < 0 ||
	    pack_blob(TAG_WIRE_DATA, &ptr, &len, &p->wire_data) < 0 ||
	    pack_blob(TAG_EXCL_CRED, &ptr, &len, &p->excl_cred) < 0)
		return (0);

	return (max - len);
}

static size_t
input_len(int max)
{
	return (5 * len_byte() + 6 * len_string(max) + 2 * len_int() +
	    4 * len_blob(max));
}

static void
make_cred(fido_cred_t *cred, uint8_t u2f, int type, const struct blob *cdh,
    const char *rp_id, const char *rp_name, struct blob *user_id,
    const char *user_name, const char *user_nick, const char *user_icon,
    int ext, uint8_t rk, uint8_t uv, const char *pin, uint8_t excl_count,
    struct blob *excl_cred)
{
	fido_dev_t	*dev;
	fido_dev_io_t	 io;

	memset(&io, 0, sizeof(io));

	io.open = dev_open;
	io.close = dev_close;
	io.read = dev_read;
	io.write = dev_write;

	if ((dev = fido_dev_new()) == NULL || fido_dev_set_io_functions(dev,
	    &io) != FIDO_OK || fido_dev_open(dev, "nodev") != FIDO_OK) {
		fido_dev_free(&dev);
		return;
	}

	if (u2f & 1)
		fido_dev_force_u2f(dev);

	for (uint8_t i = 0; i < excl_count; i++)
		fido_cred_exclude(cred, excl_cred->body, excl_cred->len);

	fido_cred_set_type(cred, type);
	fido_cred_set_clientdata_hash(cred, cdh->body, cdh->len);
	fido_cred_set_rp(cred, rp_id, rp_name);
	fido_cred_set_user(cred, user_id->body, user_id->len, user_name,
	    user_nick, user_icon);
	fido_cred_set_extensions(cred, ext);
	if (rk & 1)
		fido_cred_set_rk(cred, FIDO_OPT_TRUE);
	if (uv & 1)
		fido_cred_set_uv(cred, FIDO_OPT_TRUE);
	if (user_id->len)
		fido_cred_set_prot(cred, user_id->body[0] & 0x03);

	fido_dev_make_cred(dev, cred, u2f & 1 ? NULL : pin);

	fido_dev_cancel(dev);
	fido_dev_close(dev);
	fido_dev_free(&dev);
}

static void
verify_cred(int type, const unsigned char *cdh_ptr, size_t cdh_len,
    const char *rp_id, const char *rp_name, const unsigned char *authdata_ptr,
    size_t authdata_len, int ext, uint8_t rk, uint8_t uv,
    const unsigned char *x5c_ptr, size_t x5c_len, const unsigned char *sig_ptr,
    size_t sig_len, const char *fmt, int prot)
{
	fido_cred_t	*cred;
	uint8_t		 flags;

	if ((cred = fido_cred_new()) == NULL)
		return;

	fido_cred_set_type(cred, type);
	fido_cred_set_clientdata_hash(cred, cdh_ptr, cdh_len);
	fido_cred_set_rp(cred, rp_id, rp_name);
	if (fido_cred_set_authdata(cred, authdata_ptr, authdata_len) != FIDO_OK)
		fido_cred_set_authdata_raw(cred, authdata_ptr, authdata_len);
	fido_cred_set_extensions(cred, ext);
	fido_cred_set_x509(cred, x5c_ptr, x5c_len);
	fido_cred_set_sig(cred, sig_ptr, sig_len);
	fido_cred_set_prot(cred, prot);

	if (rk & 1)
		fido_cred_set_rk(cred, FIDO_OPT_TRUE);
	if (uv & 1)
		fido_cred_set_uv(cred, FIDO_OPT_TRUE);
	if (fmt)
		fido_cred_set_fmt(cred, fmt);

	fido_cred_verify(cred);
	fido_cred_verify_self(cred);

	consume(fido_cred_pubkey_ptr(cred), fido_cred_pubkey_len(cred));
	consume(fido_cred_id_ptr(cred), fido_cred_id_len(cred));
	consume(fido_cred_user_id_ptr(cred), fido_cred_user_id_len(cred));
	consume(fido_cred_user_name(cred), xstrlen(fido_cred_user_name(cred)));
	consume(fido_cred_display_name(cred),
	    xstrlen(fido_cred_display_name(cred)));

	flags = fido_cred_flags(cred);
	consume(&flags, sizeof(flags));
	type = fido_cred_type(cred);
	consume(&type, sizeof(type));

	fido_cred_free(&cred);
}

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	struct param	 p;
	fido_cred_t	*cred = NULL;
	int		 cose_alg = 0;

	memset(&p, 0, sizeof(p));

	if (size < input_len(GETLEN_MIN) || size > input_len(GETLEN_MAX) ||
	    unpack(data, size, &p) < 0)
		return (0);

	prng_init((unsigned int)p.seed);

	fido_init(FIDO_DEBUG);
	fido_set_log_handler(consume_str);

	if ((cred = fido_cred_new()) == NULL)
		return (0);

	set_wire_data(p.wire_data.body, p.wire_data.len);

	switch (p.type & 3) {
	case 0:
		cose_alg = COSE_ES256;
		break;
	case 1:
		cose_alg = COSE_RS256;
		break;
	default:
		cose_alg = COSE_EDDSA;
		break;
	}

	make_cred(cred, p.u2f, cose_alg, &p.cdh, p.rp_id, p.rp_name,
	    &p.user_id, p.user_name, p.user_nick, p.user_icon, p.ext, p.rk,
	    p.uv, p.pin, p.excl_count, &p.excl_cred);

	verify_cred(cose_alg,
	    fido_cred_clientdata_hash_ptr(cred),
	    fido_cred_clientdata_hash_len(cred), fido_cred_rp_id(cred),
	    fido_cred_rp_name(cred), fido_cred_authdata_ptr(cred),
	    fido_cred_authdata_len(cred), p.ext, p.rk, p.uv,
	    fido_cred_x5c_ptr(cred), fido_cred_x5c_len(cred),
	    fido_cred_sig_ptr(cred), fido_cred_sig_len(cred),
	    fido_cred_fmt(cred), fido_cred_prot(cred));

	fido_cred_free(&cred);

	return (0);
}

static size_t
pack_dummy(uint8_t *ptr, size_t len)
{
	struct param	dummy;
	uint8_t		blob[16384];
	size_t		blob_len;

	memset(&dummy, 0, sizeof(dummy));

	dummy.type = 1;
	dummy.ext = FIDO_EXT_HMAC_SECRET;

	strlcpy(dummy.pin, dummy_pin, sizeof(dummy.pin));
	strlcpy(dummy.rp_id, dummy_rp_id, sizeof(dummy.rp_id));
	strlcpy(dummy.rp_name, dummy_rp_name, sizeof(dummy.rp_name));
	strlcpy(dummy.user_icon, dummy_user_icon, sizeof(dummy.user_icon));
	strlcpy(dummy.user_name, dummy_user_name, sizeof(dummy.user_name));
	strlcpy(dummy.user_nick, dummy_user_nick, sizeof(dummy.user_nick));

	dummy.cdh.len = sizeof(dummy_cdh);
	dummy.user_id.len = sizeof(dummy_user_id);
	dummy.wire_data.len = sizeof(dummy_wire_data_fido);

	memcpy(&dummy.cdh.body, &dummy_cdh, dummy.cdh.len);
	memcpy(&dummy.user_id.body, &dummy_user_id, dummy.user_id.len);
	memcpy(&dummy.wire_data.body, &dummy_wire_data_fido,
	    dummy.wire_data.len);

	blob_len = pack(blob, sizeof(blob), &dummy);
	assert(blob_len != 0);

	if (blob_len > len) {
		memcpy(ptr, blob, len);
		return (len);
	}

	memcpy(ptr, blob, blob_len);

	return (blob_len);
}

size_t
LLVMFuzzerCustomMutator(uint8_t *data, size_t size, size_t maxsize,
    unsigned int seed) NO_MSAN
{
	struct param	p;
	uint8_t		blob[16384];
	size_t		blob_len;

	memset(&p, 0, sizeof(p));

	if (unpack(data, size, &p) < 0)
		return (pack_dummy(data, maxsize));

	mutate_byte(&p.rk);
	mutate_byte(&p.type);
	mutate_byte(&p.u2f);
	mutate_byte(&p.uv);
	mutate_byte(&p.excl_count);

	mutate_int(&p.ext);
	p.seed = (int)seed;

	mutate_blob(&p.cdh);
	mutate_blob(&p.user_id);

	if (p.u2f & 1) {
		p.wire_data.len = sizeof(dummy_wire_data_u2f);
		memcpy(&p.wire_data.body, &dummy_wire_data_u2f,
		    p.wire_data.len);
	} else {
		p.wire_data.len = sizeof(dummy_wire_data_fido);
		memcpy(&p.wire_data.body, &dummy_wire_data_fido,
		    p.wire_data.len);
	}

	mutate_blob(&p.wire_data);
	mutate_blob(&p.excl_cred);

	mutate_string(p.pin);
	mutate_string(p.user_icon);
	mutate_string(p.user_name);
	mutate_string(p.user_nick);
	mutate_string(p.rp_id);
	mutate_string(p.rp_name);

	blob_len = pack(blob, sizeof(blob), &p);

	if (blob_len == 0 || blob_len > maxsize)
		return (0);

	memcpy(data, blob, blob_len);

	return (blob_len);
}
