/*
 * Copyright (c) 2020 Yubico AB. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 */

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../openbsd-compat/openbsd-compat.h"
#include "mutator_aux.h"

extern int fido_hid_get_usage(const uint8_t *, size_t, uint32_t *);
extern int fido_hid_get_report_len(const uint8_t *, size_t, size_t *, size_t *);

struct param {
	int seed;
	struct blob report_descriptor;
};

/*
 * Sample HID report descriptor from the FIDO HID interface of a YubiKey 5.
 */
static const uint8_t dummy_report_descriptor[] = {
	0x06, 0xd0, 0xf1, 0x09, 0x01, 0xa1, 0x01, 0x09,
	0x20, 0x15, 0x00, 0x26, 0xff, 0x00, 0x75, 0x08,
	0x95, 0x40, 0x81, 0x02, 0x09, 0x21, 0x15, 0x00,
	0x26, 0xff, 0x00, 0x75, 0x08, 0x95, 0x40, 0x91,
	0x02, 0xc0
};

struct param *
unpack(const uint8_t *ptr, size_t len)
{
	cbor_item_t *item = NULL, **v;
	struct cbor_load_result cbor;
	struct param *p;
	int ok = -1;

	if ((p = calloc(1, sizeof(*p))) == NULL ||
	    (item = cbor_load(ptr, len, &cbor)) == NULL ||
	    cbor.read != len ||
	    cbor_isa_array(item) == false ||
	    cbor_array_is_definite(item) == false ||
	    cbor_array_size(item) != 2 ||
	    (v = cbor_array_handle(item)) == NULL)
		goto fail;

	if (unpack_int(v[0], &p->seed) < 0 ||
	    unpack_blob(v[1], &p->report_descriptor) < 0)
		goto fail;

	ok = 0;
fail:
	if (ok < 0) {
		free(p);
		p = NULL;
	}

	if (item)
		cbor_decref(&item);

	return p;
}

size_t
pack(uint8_t *ptr, size_t len, const struct param *p)
{
	cbor_item_t *argv[2], *array = NULL;
	size_t cbor_alloc_len, cbor_len = 0;
	unsigned char *cbor = NULL;

	memset(argv, 0, sizeof(argv));

	if ((array = cbor_new_definite_array(2)) == NULL ||
	    (argv[0] = pack_int(p->seed)) == NULL ||
	    (argv[1] = pack_blob(&p->report_descriptor)) == NULL)
		goto fail;

	for (size_t i = 0; i < 2; i++)
		if (cbor_array_push(array, argv[i]) == false)
			goto fail;

	if ((cbor_len = cbor_serialize_alloc(array, &cbor,
	    &cbor_alloc_len)) > len) {
		cbor_len = 0;
		goto fail;
	}

	memcpy(ptr, cbor, cbor_len);
fail:
	for (size_t i = 0; i < 2; i++)
		if (argv[i])
			cbor_decref(&argv[i]);

	if (array)
		cbor_decref(&array);

	free(cbor);

	return cbor_len;
}

size_t
pack_dummy(uint8_t *ptr, size_t len)
{
	struct param dummy;
	uint8_t	blob[4096];
	size_t blob_len;

	memset(&dummy, 0, sizeof(dummy));

	dummy.report_descriptor.len = sizeof(dummy_report_descriptor);
	memcpy(&dummy.report_descriptor.body, &dummy_report_descriptor,
	    dummy.report_descriptor.len);

	assert((blob_len = pack(blob, sizeof(blob), &dummy)) != 0);

	if (blob_len > len) {
		memcpy(ptr, blob, len);
		return len;
	}

	memcpy(ptr, blob, blob_len);

	return blob_len;
}

static void
get_usage(const struct param *p)
{
	uint32_t usage_page = 0;

	fido_hid_get_usage(p->report_descriptor.body, p->report_descriptor.len,
	    &usage_page);
	consume(&usage_page, sizeof(usage_page));
}

static void
get_report_len(const struct param *p)
{
	size_t report_in_len = 0;
	size_t report_out_len = 0;

	fido_hid_get_report_len(p->report_descriptor.body,
	    p->report_descriptor.len, &report_in_len, &report_out_len);
	consume(&report_in_len, sizeof(report_in_len));
	consume(&report_out_len, sizeof(report_out_len));
}

void
test(const struct param *p)
{
	prng_init((unsigned int)p->seed);
	fido_init(FIDO_DEBUG);
	fido_set_log_handler(consume_str);

	get_usage(p);
	get_report_len(p);
}

void
mutate(struct param *p, unsigned int seed, unsigned int flags) NO_MSAN
{
	if (flags & MUTATE_SEED)
		p->seed = (int)seed;

	if (flags & MUTATE_PARAM)
		mutate_blob(&p->report_descriptor);
}
