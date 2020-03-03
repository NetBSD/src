/*
 * Copyright (c) 2019 Yubico AB. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 */

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mutator_aux.h"

size_t LLVMFuzzerMutate(uint8_t *, size_t, size_t);

static uint8_t *wire_data_ptr = NULL;
static size_t   wire_data_len = 0;

size_t
xstrlen(const char *s)
{
	if (s == NULL)
		return (0);

	return (strlen(s));
}

void
consume(const void *body, size_t len)
{
	const volatile uint8_t *ptr = body;
	volatile uint8_t x = 0;

	while (len--)
		x ^= *ptr++;
}

void
consume_str(const char *str)
{
	consume(str, strlen(str));
}

int
unpack_int(uint8_t t, uint8_t **ptr, size_t *len, int *v) NO_MSAN
{
	size_t l;

	if (*len < sizeof(t) || **ptr != t)
		return (-1);

	*ptr += sizeof(t);
	*len -= sizeof(t);

	if (*len < sizeof(l))
		return (-1);

	memcpy(&l, *ptr, sizeof(l));
	*ptr += sizeof(l);
	*len -= sizeof(l);

	if (l != sizeof(*v) || *len < l)
		return (-1);

	memcpy(v, *ptr, sizeof(*v));
	*ptr += sizeof(*v);
	*len -= sizeof(*v);

	return (0);
}

int
unpack_string(uint8_t t, uint8_t **ptr, size_t *len, char *v) NO_MSAN
{
	size_t l;

	if (*len < sizeof(t) || **ptr != t)
		return (-1);

	*ptr += sizeof(t);
	*len -= sizeof(t);

	if (*len < sizeof(l))
		return (-1);

	memcpy(&l, *ptr, sizeof(l));
	*ptr += sizeof(l);
	*len -= sizeof(l);

	if (*len < l || l >= MAXSTR)
		return (-1);

	memcpy(v, *ptr, l);
	v[l] = '\0';

	*ptr += l;
	*len -= l;

	return (0);
}

int
unpack_byte(uint8_t t, uint8_t **ptr, size_t *len, uint8_t *v) NO_MSAN
{
	size_t l;

	if (*len < sizeof(t) || **ptr != t)
		return (-1);

	*ptr += sizeof(t);
	*len -= sizeof(t);

	if (*len < sizeof(l))
		return (-1);

	memcpy(&l, *ptr, sizeof(l));
	*ptr += sizeof(l);
	*len -= sizeof(l);

	if (l != sizeof(*v) || *len < l)
		return (-1);

	memcpy(v, *ptr, sizeof(*v));
	*ptr += sizeof(*v);
	*len -= sizeof(*v);

	return (0);
}

int
unpack_blob(uint8_t t, uint8_t **ptr, size_t *len, struct blob *v) NO_MSAN
{
	size_t l;

	v->len = 0;

	if (*len < sizeof(t) || **ptr != t)
		return (-1);

	*ptr += sizeof(t);
	*len -= sizeof(t);

	if (*len < sizeof(l))
		return (-1);

	memcpy(&l, *ptr, sizeof(l));
	*ptr += sizeof(l);
	*len -= sizeof(l);

	if (*len < l || l > sizeof(v->body))
		return (-1);

	memcpy(v->body, *ptr, l);
	*ptr += l;
	*len -= l;

	v->len = l;

	return (0);
}

int
pack_int(uint8_t t, uint8_t **ptr, size_t *len, int v) NO_MSAN
{
	const size_t l = sizeof(v);

	if (*len < sizeof(t) + sizeof(l) + l)
		return (-1);

	(*ptr)[0] = t;
	memcpy(&(*ptr)[sizeof(t)], &l, sizeof(l));
	memcpy(&(*ptr)[sizeof(t) + sizeof(l)], &v, l);

	*ptr += sizeof(t) + sizeof(l) + l;
	*len -= sizeof(t) + sizeof(l) + l;

	return (0);
}

int
pack_string(uint8_t t, uint8_t **ptr, size_t *len, const char *v) NO_MSAN
{
	const size_t l = strlen(v);

	if (*len < sizeof(t) + sizeof(l) + l)
		return (-1);

	(*ptr)[0] = t;
	memcpy(&(*ptr)[sizeof(t)], &l, sizeof(l));
	memcpy(&(*ptr)[sizeof(t) + sizeof(l)], v, l);

	*ptr += sizeof(t) + sizeof(l) + l;
	*len -= sizeof(t) + sizeof(l) + l;

	return (0);
}

int
pack_byte(uint8_t t, uint8_t **ptr, size_t *len, uint8_t v) NO_MSAN
{
	const size_t l = sizeof(v);

	if (*len < sizeof(t) + sizeof(l) + l)
		return (-1);

	(*ptr)[0] = t;
	memcpy(&(*ptr)[sizeof(t)], &l, sizeof(l));
	memcpy(&(*ptr)[sizeof(t) + sizeof(l)], &v, l);

	*ptr += sizeof(t) + sizeof(l) + l;
	*len -= sizeof(t) + sizeof(l) + l;

	return (0);
}

int
pack_blob(uint8_t t, uint8_t **ptr, size_t *len, const struct blob *v) NO_MSAN
{
	const size_t l = v->len;

	if (*len < sizeof(t) + sizeof(l) + l)
		return (-1);

	(*ptr)[0] = t;
	memcpy(&(*ptr)[sizeof(t)], &l, sizeof(l));
	memcpy(&(*ptr)[sizeof(t) + sizeof(l)], v->body, l);

	*ptr += sizeof(t) + sizeof(l) + l;
	*len -= sizeof(t) + sizeof(l) + l;

	return (0);
}

size_t
len_int(void)
{
	return (sizeof(uint8_t)	+ sizeof(size_t) + sizeof(int));
}

size_t
len_string(int max)
{
	return ((sizeof(uint8_t) + sizeof(size_t)) + (max ?  MAXSTR - 1 : 0));
}

size_t
len_byte(void)
{
	return (sizeof(uint8_t) + sizeof(size_t) + sizeof(uint8_t));
}

size_t
len_blob(int max)
{
	return (sizeof(uint8_t) + sizeof(size_t) + (max ? MAXBLOB : 0));
}

void
mutate_byte(uint8_t *b)
{
	LLVMFuzzerMutate(b, sizeof(*b), sizeof(*b));
}

void
mutate_int(int *i)
{
	LLVMFuzzerMutate((uint8_t *)i, sizeof(*i), sizeof(*i));
}

void
mutate_blob(struct blob *blob)
{
	blob->len = LLVMFuzzerMutate((uint8_t *)blob->body, blob->len,
	    sizeof(blob->body));
}

void
mutate_string(char *s)
{
	size_t n;

	n = LLVMFuzzerMutate((uint8_t *)s, strlen(s), MAXSTR - 1);
	s[n] = '\0';
}
 
void *
dev_open(const char *path)
{
	(void)path;

	return ((void *)0xdeadbeef);
}

void
dev_close(void *handle)
{
	assert(handle == (void *)0xdeadbeef);
}

int
dev_read(void *handle, unsigned char *ptr, size_t len, int ms)
{
	size_t n;

	(void)ms;

	assert(handle == (void *)0xdeadbeef);
	assert(len == 64);

	if (wire_data_len < len)
		n = wire_data_len;
	else
		n = len;

	memcpy(ptr, wire_data_ptr, n);

	wire_data_ptr += n;
	wire_data_len -= n;

	return ((int)n);
}

int
dev_write(void *handle, const unsigned char *ptr, size_t len)
{
	assert(handle == (void *)0xdeadbeef);
	assert(len == 64 + 1);

	consume(ptr, len);

	if (uniform_random(400) < 1)
		return (-1);

	return ((int)len);
}

void
set_wire_data(uint8_t *ptr, size_t len)
{
	wire_data_ptr = ptr;
	wire_data_len = len;
}
