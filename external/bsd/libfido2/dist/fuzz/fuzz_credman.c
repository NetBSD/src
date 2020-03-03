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
#include "dummy.h"

#include "fido.h"
#include "fido/credman.h"

#include "../openbsd-compat/openbsd-compat.h"

#define TAG_META_WIRE_DATA	0x01
#define TAG_RP_WIRE_DATA	0x02
#define TAG_RK_WIRE_DATA	0x03
#define TAG_DEL_WIRE_DATA	0x04
#define TAG_CRED_ID		0x05
#define TAG_PIN			0x06
#define TAG_RP_ID		0x07
#define TAG_SEED		0x08

/* Parameter set defining a FIDO2 credential management operation. */
struct param {
	char		pin[MAXSTR];
	char		rp_id[MAXSTR];
	int		seed;
	struct blob	cred_id;
	struct blob	del_wire_data;
	struct blob	meta_wire_data;
	struct blob	rk_wire_data;
	struct blob	rp_wire_data;
};

/*
 * Collection of HID reports from an authenticator issued with a FIDO2
 * 'getCredsMetadata' credential management command.
 */
static const uint8_t dummy_meta_wire_data[] = {
	WIREDATA_CTAP_INIT,
	WIREDATA_CTAP_CBOR_INFO,
	WIREDATA_CTAP_CBOR_AUTHKEY,
	WIREDATA_CTAP_CBOR_PINTOKEN,
	WIREDATA_CTAP_CBOR_CREDMAN_META,
};

/*
 * Collection of HID reports from an authenticator issued with a FIDO2
 * 'enumerateRPsBegin' credential management command.
 */
static const uint8_t dummy_rp_wire_data[] = {
	WIREDATA_CTAP_INIT,
	WIREDATA_CTAP_CBOR_INFO,
	WIREDATA_CTAP_CBOR_AUTHKEY,
	WIREDATA_CTAP_CBOR_PINTOKEN,
	WIREDATA_CTAP_CBOR_CREDMAN_RPLIST,
};

/*
 * Collection of HID reports from an authenticator issued with a FIDO2
 * 'enumerateCredentialsBegin' credential management command.
 */
static const uint8_t dummy_rk_wire_data[] = {
	WIREDATA_CTAP_INIT,
	WIREDATA_CTAP_CBOR_INFO,
	WIREDATA_CTAP_CBOR_AUTHKEY,
	WIREDATA_CTAP_CBOR_PINTOKEN,
	WIREDATA_CTAP_CBOR_CREDMAN_RKLIST,
};

/*
 * Collection of HID reports from an authenticator issued with a FIDO2
 * 'deleteCredential' credential management command.
 */
static const uint8_t dummy_del_wire_data[] = {
	WIREDATA_CTAP_INIT,
	WIREDATA_CTAP_CBOR_INFO,
	WIREDATA_CTAP_CBOR_AUTHKEY,
	WIREDATA_CTAP_CBOR_PINTOKEN,
	WIREDATA_CTAP_CBOR_STATUS,
};

int    LLVMFuzzerTestOneInput(const uint8_t *, size_t);
size_t LLVMFuzzerCustomMutator(uint8_t *, size_t, size_t, unsigned int);

static int
unpack(const uint8_t *ptr, size_t len, struct param *p) NO_MSAN
{
	uint8_t **pp = (void *)&ptr;

	if (unpack_string(TAG_PIN, pp, &len, p->pin) < 0 ||
	    unpack_string(TAG_RP_ID, pp, &len, p->rp_id) < 0 ||
	    unpack_blob(TAG_CRED_ID, pp, &len, &p->cred_id) < 0 ||
	    unpack_blob(TAG_META_WIRE_DATA, pp, &len, &p->meta_wire_data) < 0 ||
	    unpack_blob(TAG_RP_WIRE_DATA, pp, &len, &p->rp_wire_data) < 0 ||
	    unpack_blob(TAG_RK_WIRE_DATA, pp, &len, &p->rk_wire_data) < 0 ||
	    unpack_blob(TAG_DEL_WIRE_DATA, pp, &len, &p->del_wire_data) < 0 ||
	    unpack_int(TAG_SEED, pp, &len, &p->seed) < 0)
		return (-1);

	return (0);
}

static size_t
pack(uint8_t *ptr, size_t len, const struct param *p)
{
	const size_t max = len;

	if (pack_string(TAG_PIN, &ptr, &len, p->pin) < 0 ||
	    pack_string(TAG_RP_ID, &ptr, &len, p->rp_id) < 0 ||
	    pack_blob(TAG_CRED_ID, &ptr, &len, &p->cred_id) < 0 ||
	    pack_blob(TAG_META_WIRE_DATA, &ptr, &len, &p->meta_wire_data) < 0 ||
	    pack_blob(TAG_RP_WIRE_DATA, &ptr, &len, &p->rp_wire_data) < 0 ||
	    pack_blob(TAG_RK_WIRE_DATA, &ptr, &len, &p->rk_wire_data) < 0 ||
	    pack_blob(TAG_DEL_WIRE_DATA, &ptr, &len, &p->del_wire_data) < 0 ||
	    pack_int(TAG_SEED, &ptr, &len, p->seed) < 0)
		return (0);

	return (max - len);
}

static size_t
input_len(int max)
{
	return (2 * len_string(max) + 5 * len_blob(max) + len_int());
}

static fido_dev_t *
prepare_dev()
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
		return (NULL);
	}

	return (dev);
}

static void
get_metadata(struct param *p)
{
	fido_dev_t *dev;
	fido_credman_metadata_t *metadata;
	uint64_t existing;
	uint64_t remaining;

	set_wire_data(p->meta_wire_data.body, p->meta_wire_data.len);

	if ((dev = prepare_dev()) == NULL)
		return;

	if ((metadata = fido_credman_metadata_new()) == NULL) {
		fido_dev_close(dev);
		fido_dev_free(&dev);
		return;
	}

	fido_credman_get_dev_metadata(dev, metadata, p->pin);

	existing = fido_credman_rk_existing(metadata);
	remaining = fido_credman_rk_remaining(metadata);
	consume(&existing, sizeof(existing));
	consume(&remaining, sizeof(remaining));

	fido_credman_metadata_free(&metadata);
	fido_dev_close(dev);
	fido_dev_free(&dev);
}

static void
get_rp_list(struct param *p)
{
	fido_dev_t *dev;
	fido_credman_rp_t *rp;

	set_wire_data(p->rp_wire_data.body, p->rp_wire_data.len);

	if ((dev = prepare_dev()) == NULL)
		return;

	if ((rp = fido_credman_rp_new()) == NULL) {
		fido_dev_close(dev);
		fido_dev_free(&dev);
		return;
	}

	fido_credman_get_dev_rp(dev, rp, p->pin);

	/* +1 on purpose */
	for (size_t i = 0; i < fido_credman_rp_count(rp) + 1; i++) {
		consume(fido_credman_rp_id_hash_ptr(rp, i),
		    fido_credman_rp_id_hash_len(rp, i));
		consume(fido_credman_rp_id(rp, i),
		    xstrlen(fido_credman_rp_id(rp, i)));
		consume(fido_credman_rp_name(rp, i),
		    xstrlen(fido_credman_rp_name(rp, i)));
	}

	fido_credman_rp_free(&rp);
	fido_dev_close(dev);
	fido_dev_free(&dev);
}

static void
get_rk_list(struct param *p)
{
	fido_dev_t *dev;
	fido_credman_rk_t *rk;
	const fido_cred_t *cred;
	int type;

	set_wire_data(p->rk_wire_data.body, p->rk_wire_data.len);

	if ((dev = prepare_dev()) == NULL)
		return;

	if ((rk = fido_credman_rk_new()) == NULL) {
		fido_dev_close(dev);
		fido_dev_free(&dev);
		return;
	}

	fido_credman_get_dev_rk(dev, p->rp_id, rk, p->pin);

	/* +1 on purpose */
	for (size_t i = 0; i < fido_credman_rk_count(rk) + 1; i++) {
		if ((cred = fido_credman_rk(rk, i)) == NULL) {
			assert(i >= fido_credman_rk_count(rk));
			continue;
		}
		type = fido_cred_type(cred);
		consume(&type, sizeof(type));
		consume(fido_cred_id_ptr(cred), fido_cred_id_len(cred));
		consume(fido_cred_pubkey_ptr(cred), fido_cred_pubkey_len(cred));
		consume(fido_cred_user_id_ptr(cred),
		    fido_cred_user_id_len(cred));
		consume(fido_cred_user_name(cred),
		    xstrlen(fido_cred_user_name(cred)));
		consume(fido_cred_display_name(cred),
		    xstrlen(fido_cred_display_name(cred)));
	}

	fido_credman_rk_free(&rk);
	fido_dev_close(dev);
	fido_dev_free(&dev);
}

static void
del_rk(struct param *p)
{
	fido_dev_t *dev;

	set_wire_data(p->del_wire_data.body, p->del_wire_data.len);

	if ((dev = prepare_dev()) == NULL)
		return;

	fido_credman_del_dev_rk(dev, p->cred_id.body, p->cred_id.len, p->pin);
	fido_dev_close(dev);
	fido_dev_free(&dev);
}

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	struct param p;

	memset(&p, 0, sizeof(p));

	if (size < input_len(GETLEN_MIN) || size > input_len(GETLEN_MAX) ||
	    unpack(data, size, &p) < 0)
		return (0);

	prng_init((unsigned int)p.seed);

	fido_init(FIDO_DEBUG);
	fido_set_log_handler(consume_str);

	get_metadata(&p);
	get_rp_list(&p);
	get_rk_list(&p);
	del_rk(&p);

	return (0);
}

static size_t
pack_dummy(uint8_t *ptr, size_t len)
{
	struct param	dummy;
	uint8_t		blob[32768];
	size_t		blob_len;

	memset(&dummy, 0, sizeof(dummy));

	strlcpy(dummy.pin, dummy_pin, sizeof(dummy.pin));
	strlcpy(dummy.rp_id, dummy_rp_id, sizeof(dummy.rp_id));

	dummy.meta_wire_data.len = sizeof(dummy_meta_wire_data);
	dummy.rp_wire_data.len = sizeof(dummy_rp_wire_data);
	dummy.rk_wire_data.len = sizeof(dummy_rk_wire_data);
	dummy.del_wire_data.len = sizeof(dummy_del_wire_data);
	dummy.cred_id.len = sizeof(dummy_cred_id);

	memcpy(&dummy.meta_wire_data.body, &dummy_meta_wire_data,
	    dummy.meta_wire_data.len);
	memcpy(&dummy.rp_wire_data.body, &dummy_rp_wire_data,
	    dummy.rp_wire_data.len);
	memcpy(&dummy.rk_wire_data.body, &dummy_rk_wire_data,
	    dummy.rk_wire_data.len);
	memcpy(&dummy.del_wire_data.body, &dummy_del_wire_data,
	    dummy.del_wire_data.len);
	memcpy(&dummy.cred_id.body, &dummy_cred_id, dummy.cred_id.len);

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

	p.seed = (int)seed;

	mutate_blob(&p.cred_id);
	mutate_blob(&p.meta_wire_data);
	mutate_blob(&p.rp_wire_data);
	mutate_blob(&p.rk_wire_data);
	mutate_blob(&p.del_wire_data);

	mutate_string(p.pin);
	mutate_string(p.rp_id);

	blob_len = pack(blob, sizeof(blob), &p);

	if (blob_len == 0 || blob_len > maxsize)
		return (0);

	memcpy(data, blob, blob_len);

	return (blob_len);
}
