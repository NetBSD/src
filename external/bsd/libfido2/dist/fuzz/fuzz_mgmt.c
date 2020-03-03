/*
 * Copyright (c) 2019 Yubico AB. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mutator_aux.h"
#include "wiredata_fido2.h"
#include "dummy.h"
#include "fido.h"

#include "../openbsd-compat/openbsd-compat.h"

#define TAG_PIN1			0x01
#define TAG_PIN2			0x02
#define TAG_RESET_WIRE_DATA		0x03
#define TAG_INFO_WIRE_DATA		0x04
#define TAG_SET_PIN_WIRE_DATA		0x05
#define TAG_CHANGE_PIN_WIRE_DATA	0x06
#define TAG_RETRY_WIRE_DATA		0x07
#define TAG_SEED			0x08

struct param {
	char		pin1[MAXSTR];
	char		pin2[MAXSTR];
	struct blob	reset_wire_data;
	struct blob	info_wire_data;
	struct blob	set_pin_wire_data;
	struct blob	change_pin_wire_data;
	struct blob	retry_wire_data;
	int		seed;
};

static const uint8_t dummy_reset_wire_data[] = {
	WIREDATA_CTAP_INIT,
	WIREDATA_CTAP_CBOR_INFO,
	WIREDATA_CTAP_KEEPALIVE,
	WIREDATA_CTAP_KEEPALIVE,
	WIREDATA_CTAP_KEEPALIVE,
	WIREDATA_CTAP_CBOR_RESET,
};

static const uint8_t dummy_info_wire_data[] = {
	WIREDATA_CTAP_INIT,
	WIREDATA_CTAP_CBOR_INFO,
	WIREDATA_CTAP_CBOR_INFO,
};

static const uint8_t dummy_set_pin_wire_data[] = {
	WIREDATA_CTAP_INIT,
	WIREDATA_CTAP_CBOR_INFO,
	WIREDATA_CTAP_CBOR_AUTHKEY,
	WIREDATA_CTAP_CBOR_STATUS,
};

static const uint8_t dummy_change_pin_wire_data[] = {
	WIREDATA_CTAP_INIT,
	WIREDATA_CTAP_CBOR_INFO,
	WIREDATA_CTAP_CBOR_AUTHKEY,
	WIREDATA_CTAP_CBOR_STATUS,
};

static const uint8_t dummy_retry_wire_data[] = {
	WIREDATA_CTAP_INIT,
	WIREDATA_CTAP_CBOR_INFO,
	WIREDATA_CTAP_CBOR_RETRIES,
};

int    LLVMFuzzerTestOneInput(const uint8_t *, size_t);
size_t LLVMFuzzerCustomMutator(uint8_t *, size_t, size_t, unsigned int);

static int
unpack(const uint8_t *ptr, size_t len, struct param *p) NO_MSAN
{
	uint8_t **pp = (void *)&ptr;

	if (unpack_string(TAG_PIN1, pp, &len, p->pin1) < 0 ||
	    unpack_string(TAG_PIN2, pp, &len, p->pin2) < 0 ||
	    unpack_blob(TAG_RESET_WIRE_DATA, pp, &len, &p->reset_wire_data) < 0 ||
	    unpack_blob(TAG_INFO_WIRE_DATA, pp, &len, &p->info_wire_data) < 0 ||
	    unpack_blob(TAG_SET_PIN_WIRE_DATA, pp, &len, &p->set_pin_wire_data) < 0 ||
	    unpack_blob(TAG_CHANGE_PIN_WIRE_DATA, pp, &len, &p->change_pin_wire_data) < 0 ||
	    unpack_blob(TAG_RETRY_WIRE_DATA, pp, &len, &p->retry_wire_data) < 0 ||
	    unpack_int(TAG_SEED, pp, &len, &p->seed) < 0)
		return (-1);

	return (0);
}

static size_t
pack(uint8_t *ptr, size_t len, const struct param *p)
{
	const size_t max = len;

	if (pack_string(TAG_PIN1, &ptr, &len, p->pin1) < 0 ||
	    pack_string(TAG_PIN2, &ptr, &len, p->pin2) < 0 ||
	    pack_blob(TAG_RESET_WIRE_DATA, &ptr, &len, &p->reset_wire_data) < 0 ||
	    pack_blob(TAG_INFO_WIRE_DATA, &ptr, &len, &p->info_wire_data) < 0 ||
	    pack_blob(TAG_SET_PIN_WIRE_DATA, &ptr, &len, &p->set_pin_wire_data) < 0 ||
	    pack_blob(TAG_CHANGE_PIN_WIRE_DATA, &ptr, &len, &p->change_pin_wire_data) < 0 ||
	    pack_blob(TAG_RETRY_WIRE_DATA, &ptr, &len, &p->retry_wire_data) < 0 ||
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
dev_reset(struct param *p)
{
	fido_dev_t *dev;

	set_wire_data(p->reset_wire_data.body, p->reset_wire_data.len);

	if ((dev = prepare_dev()) == NULL)
		return;

	fido_dev_reset(dev);
	fido_dev_close(dev);
	fido_dev_free(&dev);
}

static void
dev_get_cbor_info(struct param *p)
{
	fido_dev_t *dev;
	fido_cbor_info_t *ci;
	uint64_t n;
	uint8_t proto;
	uint8_t major;
	uint8_t minor;
	uint8_t build;
	uint8_t flags;

	set_wire_data(p->info_wire_data.body, p->info_wire_data.len);

	if ((dev = prepare_dev()) == NULL)
		return;

	proto = fido_dev_protocol(dev);
	major = fido_dev_major(dev);
	minor = fido_dev_minor(dev);
	build = fido_dev_build(dev);
	flags = fido_dev_flags(dev);

	consume(&proto, sizeof(proto));
	consume(&major, sizeof(major));
	consume(&minor, sizeof(minor));
	consume(&build, sizeof(build));
	consume(&flags, sizeof(flags));

	if ((ci = fido_cbor_info_new()) == NULL)
		goto out;

	fido_dev_get_cbor_info(dev, ci);

	for (size_t i = 0; i < fido_cbor_info_versions_len(ci); i++) {
		char * const *sa = fido_cbor_info_versions_ptr(ci);
		consume(sa[i], strlen(sa[i]));
	}
	for (size_t i = 0; i < fido_cbor_info_extensions_len(ci); i++) {
		char * const *sa = fido_cbor_info_extensions_ptr(ci);
		consume(sa[i], strlen(sa[i]));
	}

	for (size_t i = 0; i < fido_cbor_info_options_len(ci); i++) {
		char * const *sa = fido_cbor_info_options_name_ptr(ci);
		const bool *va = fido_cbor_info_options_value_ptr(ci);
		consume(sa[i], strlen(sa[i]));
		consume(&va[i], sizeof(va[i]));
	}

	n = fido_cbor_info_maxmsgsiz(ci);
	consume(&n, sizeof(n));

	consume(fido_cbor_info_aaguid_ptr(ci), fido_cbor_info_aaguid_len(ci));
	consume(fido_cbor_info_protocols_ptr(ci),
	    fido_cbor_info_protocols_len(ci));

out:
	fido_dev_close(dev);
	fido_dev_free(&dev);

	fido_cbor_info_free(&ci);
}

static void
dev_set_pin(struct param *p)
{
	fido_dev_t *dev;

	set_wire_data(p->set_pin_wire_data.body, p->set_pin_wire_data.len);

	if ((dev = prepare_dev()) == NULL)
		return;

	fido_dev_set_pin(dev, p->pin1, NULL);
	fido_dev_close(dev);
	fido_dev_free(&dev);
}

static void
dev_change_pin(struct param *p)
{
	fido_dev_t *dev;

	set_wire_data(p->change_pin_wire_data.body, p->change_pin_wire_data.len);

	if ((dev = prepare_dev()) == NULL)
		return;

	fido_dev_set_pin(dev, p->pin2, p->pin1);
	fido_dev_close(dev);
	fido_dev_free(&dev);
}

static void
dev_get_retry_count(struct param *p)
{
	fido_dev_t *dev;
	int n;

	set_wire_data(p->retry_wire_data.body, p->retry_wire_data.len);

	if ((dev = prepare_dev()) == NULL)
		return;

	fido_dev_get_retry_count(dev, &n);
	consume(&n, sizeof(n));
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

	dev_reset(&p);
	dev_get_cbor_info(&p);
	dev_set_pin(&p);
	dev_change_pin(&p);
	dev_get_retry_count(&p);

	return (0);
}

static size_t
pack_dummy(uint8_t *ptr, size_t len)
{
	struct param	dummy;
	uint8_t		blob[16384];
	size_t		blob_len;

	memset(&dummy, 0, sizeof(dummy));

	strlcpy(dummy.pin1, dummy_pin1, sizeof(dummy.pin1));
	strlcpy(dummy.pin2, dummy_pin2, sizeof(dummy.pin2));

	dummy.reset_wire_data.len = sizeof(dummy_reset_wire_data);
	dummy.info_wire_data.len = sizeof(dummy_info_wire_data);
	dummy.set_pin_wire_data.len = sizeof(dummy_set_pin_wire_data);
	dummy.change_pin_wire_data.len = sizeof(dummy_change_pin_wire_data);
	dummy.retry_wire_data.len = sizeof(dummy_retry_wire_data);

	memcpy(&dummy.reset_wire_data.body, &dummy_reset_wire_data,
	    dummy.reset_wire_data.len);
	memcpy(&dummy.info_wire_data.body, &dummy_info_wire_data,
	    dummy.info_wire_data.len);
	memcpy(&dummy.set_pin_wire_data.body, &dummy_set_pin_wire_data,
	    dummy.set_pin_wire_data.len);
	memcpy(&dummy.change_pin_wire_data.body, &dummy_change_pin_wire_data,
	    dummy.change_pin_wire_data.len);
	memcpy(&dummy.retry_wire_data.body, &dummy_retry_wire_data,
	    dummy.retry_wire_data.len);

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
    unsigned int seed)
{
	struct param	p;
	uint8_t		blob[16384];
	size_t		blob_len;

	memset(&p, 0, sizeof(p));

	if (unpack(data, size, &p) < 0)
		return (pack_dummy(data, maxsize));

	p.seed = (int)seed;

	mutate_string(p.pin1);
	mutate_string(p.pin2);

	mutate_blob(&p.reset_wire_data);
	mutate_blob(&p.info_wire_data);
	mutate_blob(&p.set_pin_wire_data);
	mutate_blob(&p.change_pin_wire_data);
	mutate_blob(&p.retry_wire_data);

	blob_len = pack(blob, sizeof(blob), &p);

	if (blob_len == 0 || blob_len > maxsize)
		return (0);

	memcpy(data, blob, blob_len);

	return (blob_len);
}
