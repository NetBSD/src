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
#include "fido/bio.h"

#include "../openbsd-compat/openbsd-compat.h"

#define TAG_PIN			0x01
#define TAG_NAME		0x02
#define TAG_SEED		0x03
#define TAG_ID			0x04
#define TAG_INFO_WIRE_DATA	0x05
#define TAG_ENROLL_WIRE_DATA	0x06
#define TAG_LIST_WIRE_DATA	0x07
#define TAG_SET_NAME_WIRE_DATA	0x08
#define TAG_REMOVE_WIRE_DATA	0x09

/* Parameter set defining a FIDO2 credential management operation. */
struct param {
	char		pin[MAXSTR];
	char		name[MAXSTR];
	int		seed;
	struct blob	id;
	struct blob	info_wire_data;
	struct blob	enroll_wire_data;
	struct blob	list_wire_data;
	struct blob	set_name_wire_data;
	struct blob	remove_wire_data;
};

/*
 * Collection of HID reports from an authenticator issued with a FIDO2
 * 'getFingerprintSensorInfo' bio enrollment command.
 */
static const uint8_t dummy_info_wire_data[] = {
	WIREDATA_CTAP_INIT,
	WIREDATA_CTAP_CBOR_INFO,
	WIREDATA_CTAP_CBOR_BIO_INFO,
};

/*
 * Collection of HID reports from an authenticator issued with FIDO2
 * 'enrollBegin' + 'enrollCaptureNextSample' bio enrollment commands.
 */
static const uint8_t dummy_enroll_wire_data[] = {
	WIREDATA_CTAP_INIT,
	WIREDATA_CTAP_CBOR_INFO,
	WIREDATA_CTAP_CBOR_AUTHKEY,
	WIREDATA_CTAP_CBOR_PINTOKEN,
	WIREDATA_CTAP_CBOR_BIO_ENROLL,
};

/*
 * Collection of HID reports from an authenticator issued with a FIDO2
 * 'enumerateEnrollments' bio enrollment command.
 */
static const uint8_t dummy_list_wire_data[] = {
	WIREDATA_CTAP_INIT,
	WIREDATA_CTAP_CBOR_INFO,
	WIREDATA_CTAP_CBOR_AUTHKEY,
	WIREDATA_CTAP_CBOR_PINTOKEN,
	WIREDATA_CTAP_CBOR_BIO_ENUM,
};

/*
 * Collection of HID reports from an authenticator issued with a FIDO2
 * 'setFriendlyName' bio enrollment command.
 */
static const uint8_t dummy_set_name_wire_data[] = {
	WIREDATA_CTAP_INIT,
	WIREDATA_CTAP_CBOR_INFO,
	WIREDATA_CTAP_CBOR_AUTHKEY,
	WIREDATA_CTAP_CBOR_PINTOKEN,
	WIREDATA_CTAP_CBOR_STATUS,
};

/*
 * Collection of HID reports from an authenticator issued with a FIDO2
 * 'removeEnrollment' bio enrollment command.
 */
static const uint8_t dummy_remove_wire_data[] = {
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
	    unpack_string(TAG_NAME, pp, &len, p->name) < 0 ||
	    unpack_int(TAG_SEED, pp, &len, &p->seed) < 0 ||
	    unpack_blob(TAG_ID, pp, &len, &p->id) < 0 ||
	    unpack_blob(TAG_INFO_WIRE_DATA, pp, &len, &p->info_wire_data) < 0 ||
	    unpack_blob(TAG_ENROLL_WIRE_DATA, pp, &len, &p->enroll_wire_data) < 0 ||
	    unpack_blob(TAG_LIST_WIRE_DATA, pp, &len, &p->list_wire_data) < 0 ||
	    unpack_blob(TAG_SET_NAME_WIRE_DATA, pp, &len, &p->set_name_wire_data) < 0 ||
	    unpack_blob(TAG_REMOVE_WIRE_DATA, pp, &len, &p->remove_wire_data) < 0)
		return (-1);

	return (0);
}

static size_t
pack(uint8_t *ptr, size_t len, const struct param *p)
{
	const size_t max = len;

	if (pack_string(TAG_PIN, &ptr, &len, p->pin) < 0 ||
	    pack_string(TAG_NAME, &ptr, &len, p->name) < 0 ||
	    pack_int(TAG_SEED, &ptr, &len, p->seed) < 0 ||
	    pack_blob(TAG_ID, &ptr, &len, &p->id) < 0 ||
	    pack_blob(TAG_INFO_WIRE_DATA, &ptr, &len, &p->info_wire_data) < 0 ||
	    pack_blob(TAG_ENROLL_WIRE_DATA, &ptr, &len, &p->enroll_wire_data) < 0 ||
	    pack_blob(TAG_LIST_WIRE_DATA, &ptr, &len, &p->list_wire_data) < 0 ||
	    pack_blob(TAG_SET_NAME_WIRE_DATA, &ptr, &len, &p->set_name_wire_data) < 0 ||
	    pack_blob(TAG_REMOVE_WIRE_DATA, &ptr, &len, &p->remove_wire_data) < 0)
		return (0);

	return (max - len);
}

static size_t
input_len(int max)
{
	return (2 * len_string(max) + len_int() + 6 * len_blob(max));
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
get_info(struct param *p)
{
	fido_dev_t *dev = NULL;
	fido_bio_info_t *i = NULL;
	uint8_t type;
	uint8_t max_samples;

	set_wire_data(p->info_wire_data.body, p->info_wire_data.len);

	if ((dev = prepare_dev()) == NULL || (i = fido_bio_info_new()) == NULL)
		goto done;

	fido_bio_dev_get_info(dev, i);

	type = fido_bio_info_type(i);
	max_samples = fido_bio_info_max_samples(i);
	consume(&type, sizeof(type));
	consume(&max_samples, sizeof(max_samples));

done:
	if (dev)
		fido_dev_close(dev);

	fido_dev_free(&dev);
	fido_bio_info_free(&i);
}

static void
consume_template(const fido_bio_template_t *t)
{
	consume(fido_bio_template_name(t), xstrlen(fido_bio_template_name(t)));
	consume(fido_bio_template_id_ptr(t), fido_bio_template_id_len(t));
}

static void
consume_enroll(fido_bio_enroll_t *e)
{
	uint8_t last_status;
	uint8_t remaining_samples;

	last_status = fido_bio_enroll_last_status(e);
	remaining_samples = fido_bio_enroll_remaining_samples(e);
	consume(&last_status, sizeof(last_status));
	consume(&remaining_samples, sizeof(remaining_samples));
}

static void
enroll(struct param *p)
{
	fido_dev_t *dev = NULL;
	fido_bio_template_t *t = NULL;
	fido_bio_enroll_t *e = NULL;
	size_t cnt = 0;

	set_wire_data(p->enroll_wire_data.body, p->enroll_wire_data.len);

	if ((dev = prepare_dev()) == NULL ||
	    (t = fido_bio_template_new()) == NULL ||
	    (e = fido_bio_enroll_new()) == NULL)
		goto done;

	fido_bio_dev_enroll_begin(dev, t, e, p->seed, p->pin);

	consume_template(t);
	consume_enroll(e);

	while (fido_bio_enroll_remaining_samples(e) > 0 && cnt++ < 5) {
		fido_bio_dev_enroll_continue(dev, t, e, p->seed);
		consume_template(t);
		consume_enroll(e);
	}

done:
	if (dev)
		fido_dev_close(dev);

	fido_dev_free(&dev);
	fido_bio_template_free(&t);
	fido_bio_enroll_free(&e);
}

static void
list(struct param *p)
{
	fido_dev_t *dev = NULL;
	fido_bio_template_array_t *ta = NULL;
	const fido_bio_template_t *t = NULL;

	set_wire_data(p->list_wire_data.body, p->list_wire_data.len);

	if ((dev = prepare_dev()) == NULL ||
	    (ta = fido_bio_template_array_new()) == NULL)
		goto done;

	fido_bio_dev_get_template_array(dev, ta, p->pin);

	/* +1 on purpose */
	for (size_t i = 0; i < fido_bio_template_array_count(ta) + 1; i++)
		if ((t = fido_bio_template(ta, i)) != NULL)
			consume_template(t);

done:
	if (dev)
		fido_dev_close(dev);

	fido_dev_free(&dev);
	fido_bio_template_array_free(&ta);
}

static void
set_name(struct param *p)
{
	fido_dev_t *dev = NULL;
	fido_bio_template_t *t = NULL;

	set_wire_data(p->set_name_wire_data.body, p->set_name_wire_data.len);

	if ((dev = prepare_dev()) == NULL ||
	    (t = fido_bio_template_new()) == NULL)
		goto done;

	fido_bio_template_set_name(t, p->name);
	fido_bio_template_set_id(t, p->id.body, p->id.len);
	consume_template(t);

	fido_bio_dev_set_template_name(dev, t, p->pin);

done:
	if (dev)
		fido_dev_close(dev);

	fido_dev_free(&dev);
	fido_bio_template_free(&t);
}

static void
del(struct param *p)
{
	fido_dev_t *dev = NULL;
	fido_bio_template_t *t = NULL;

	set_wire_data(p->remove_wire_data.body, p->remove_wire_data.len);

	if ((dev = prepare_dev()) == NULL ||
	    (t = fido_bio_template_new()) == NULL)
		goto done;

	fido_bio_template_set_id(t, p->id.body, p->id.len);
	consume_template(t);

	fido_bio_dev_enroll_remove(dev, t, p->pin);

done:
	if (dev)
		fido_dev_close(dev);

	fido_dev_free(&dev);
	fido_bio_template_free(&t);
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

	get_info(&p);
	enroll(&p);
	list(&p);
	set_name(&p);
	del(&p);

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
	strlcpy(dummy.name, dummy_name, sizeof(dummy.name));

	dummy.info_wire_data.len = sizeof(dummy_info_wire_data);
	dummy.enroll_wire_data.len = sizeof(dummy_enroll_wire_data);
	dummy.list_wire_data.len = sizeof(dummy_list_wire_data);
	dummy.set_name_wire_data.len = sizeof(dummy_set_name_wire_data);
	dummy.remove_wire_data.len = sizeof(dummy_remove_wire_data);
	dummy.id.len = sizeof(dummy_id);

	memcpy(&dummy.info_wire_data.body, &dummy_info_wire_data,
	    dummy.info_wire_data.len);
	memcpy(&dummy.enroll_wire_data.body, &dummy_enroll_wire_data,
	    dummy.enroll_wire_data.len);
	memcpy(&dummy.list_wire_data.body, &dummy_list_wire_data,
	    dummy.list_wire_data.len);
	memcpy(&dummy.set_name_wire_data.body, &dummy_set_name_wire_data,
	    dummy.set_name_wire_data.len);
	memcpy(&dummy.remove_wire_data.body, &dummy_remove_wire_data,
	    dummy.remove_wire_data.len);
	memcpy(&dummy.id.body, &dummy_id, dummy.id.len);

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

	mutate_blob(&p.id);
	mutate_blob(&p.info_wire_data);
	mutate_blob(&p.enroll_wire_data);
	mutate_blob(&p.list_wire_data);
	mutate_blob(&p.set_name_wire_data);
	mutate_blob(&p.remove_wire_data);

	mutate_string(p.pin);
	mutate_string(p.name);

	blob_len = pack(blob, sizeof(blob), &p);

	if (blob_len == 0 || blob_len > maxsize)
		return (0);

	memcpy(data, blob, blob_len);

	return (blob_len);
}
