/*	$NetBSD: usbhid.h,v 1.6 2016/01/22 23:51:23 dholland Exp $	*/

/*
 * Copyright (c) 1999 Lennart Augustsson <augustss@NetBSD.org>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _USBHID_H_
#define _USBHID_H_

#include <sys/cdefs.h>
#include <stdint.h>

typedef struct report_desc *report_desc_t;

typedef struct hid_data *hid_data_t;

typedef enum hid_kind {
	hid_input = 0,
	hid_output = 1,
	hid_feature = 2,
	hid_collection,
	hid_endcollection
} hid_kind_t;

typedef struct hid_item {
	/* Global */
	int _usage_page;
	int logical_minimum;
	int logical_maximum;
	int physical_minimum;
	int physical_maximum;
	int unit_exponent;
	int unit;
	int report_size;
	int report_ID;
#define NO_REPORT_ID 0
	int report_count;
	/* Local */
	unsigned int usage;
	int usage_minimum;
	int usage_maximum;
	int designator_index;
	int designator_minimum;
	int designator_maximum;
	int string_index;
	int string_minimum;
	int string_maximum;
	int set_delimiter;
	/* Misc */
	int collection;
	int collevel;
	enum hid_kind kind;
	unsigned int flags;
	/* Absolute data position (bits) */
	unsigned int pos;
	/* */
	struct hid_item *next;
} hid_item_t;

#define HID_PAGE(u) (((uint32_t)(u) >> 16) & 0xffff)
#define HID_USAGE(u) ((u) & 0xffff)

__BEGIN_DECLS

/* Obtaining a report descriptor, descr.c: */
report_desc_t hid_get_report_desc(int file);
report_desc_t hid_use_report_desc(const uint8_t *data, unsigned int size);
void hid_dispose_report_desc(report_desc_t);

/* Parsing of a HID report descriptor, parse.c: */
hid_data_t hid_start_parse(report_desc_t d, int kindset, int id);
void hid_end_parse(hid_data_t s);
int hid_get_item(hid_data_t s, hid_item_t *h);
int hid_report_size(report_desc_t d, enum hid_kind k, int id);
int hid_locate(report_desc_t d, unsigned int usage, enum hid_kind k, hid_item_t *h, int id);

/* Conversion to/from usage names, usage.c: */
const char *hid_usage_page(int i);
const char *hid_usage_in_page(unsigned int u);
void hid_init(const char *file);
int hid_parse_usage_in_page(const char *name);
int hid_parse_usage_page(const char *name);

/* Extracting/insertion of data, data.c: */
int hid_get_data(const void *p, const hid_item_t *h);
void hid_set_data(void *p, const hid_item_t *h, int data);

__END_DECLS

#endif /* _USBHID_H_ */
