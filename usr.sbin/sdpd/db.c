/*	$NetBSD: db.c,v 1.1.2.2 2009/05/13 19:20:39 jym Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Iain Hibbert.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: db.c,v 1.1.2.2 2009/05/13 19:20:39 jym Exp $");

#include <bluetooth.h>
#include <sdp.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <uuid.h>

#include "sdpd.h"

/*
 * Using a prebuilt service record means that providing ServerState
 * and a non-hardcoded ProviderName are difficult. Look into that later.
 */

/* ServiceDiscoveryServer service record */
static uint8_t sds_data[] = {
	0x09, 0x00, 0x00,	//  uint16	ServiceRecordHandle
	0x0a, 0x00, 0x00, 0x00,	//  uint32	0x00000000
	0x00,

	0x09, 0x00, 0x01,	//  uint16	ServiceClassIDList
	0x35, 0x03,		//  seq8(3)
	0x19, 0x10, 0x00,	//   uuid16	ServiceDiscoveryServer

	0x09, 0x00, 0x04,	//  uint16	ProtocolDescriptorList
	0x35, 0x0d,		//  seq8(13)
	0x35, 0x06,		//   seq8(6)
	0x19, 0x01, 0x00,	//    uuid16	L2CAP
	0x09, 0x00, 0x01,	//    uint16	L2CAP_PSM_SDP
	0x35, 0x03,		//   seq8(3)
	0x19, 0x00, 0x01,	//    uuid16	SDP

	0x09, 0x00, 0x05,	//  uint16	BrowseGroupList
	0x35, 0x03,		//  seq8(3)
	0x19, 0x10, 0x02,	//   uuid16	PublicBrowseGroup

	0x09, 0x00, 0x06,	//  uint16	LanguageBaseAttributeIDList
	0x35, 0x09,		//  seq8(9)
	0x09, 0x65, 0x6e,	//   uint16	0x656e	("en")
	0x09, 0x00, 0x6a,	//   uint16	106	(UTF-8)
	0x09, 0x01, 0x00,	//   uint16	PrimaryLanguageBaseID

	0x09, 0x01, 0x00,	//  uint16	PrimaryLanguageBaseID + ServiceNameOffset
	0x25, 0x1b, 0x42, 0x6c,	//  str8(27)	"Bluetooth service discovery"
	0x75, 0x65, 0x74, 0x6f,
	0x6f, 0x74, 0x68, 0x20,
	0x73, 0x65, 0x72, 0x76,
	0x69, 0x63, 0x65, 0x20,
	0x64, 0x69, 0x73, 0x63,
	0x6f, 0x76, 0x65, 0x72,
	0x79,

	0x09, 0x01, 0x02,	//  uint16	PrimaryLanguageBaseID + ProviderNameOffset
	0x25, 0x06, 0x4e, 0x65,	//  str8(6)	"NetBSD"
	0x74, 0x42, 0x53, 0x44,

	0x09, 0x02, 0x00,	//  uint16	VersionNumberList
	0x35, 0x03,		//  seq8(3)
	0x09, 0x01, 0x00,	//   uint16	v1.0
};

/* BrowseGroupDescriptor service record */
static uint8_t bgd_data[] = {
	0x09, 0x00, 0x00,	//  uint16	ServiceRecordHandle
	0x0a, 0x00, 0x00, 0x00,	//  uint32	0x00000001
	0x01,

	0x09, 0x00, 0x01,	//  uint16	ServiceClassIDList
	0x35, 0x03,		//  seq8(3)
	0x19, 0x10, 0x01,	//   uuid16	BrowseGroupDescriptor

	0x09, 0x00, 0x06,	//  uint16	LanguageBaseAttributeIDList
	0x35, 0x09,		//  seq8(9)
	0x09, 0x65, 0x6e,	//   uint16	0x656e	("en")
	0x09, 0x00, 0x6a,	//   uint16	106	(UTF-8)
	0x09, 0x01, 0x00,	//   uint16	PrimaryLanguageBaseID

	0x09, 0x01, 0x00,	//  uint16	PrimaryLanguageBaseID + ServiceNameOffset
	0x25, 0x12, 0x50, 0x75,	//  str8(18)	"Public Browse Root"
	0x62, 0x6c, 0x69, 0x63,
	0x20, 0x42, 0x72, 0x6f,
	0x77, 0x73, 0x65, 0x20,
	0x52, 0x6f, 0x6f, 0x74,

	0x09, 0x02, 0x00,	//  uint16	GroupID
	0x19, 0x10, 0x02,	//  uuid16	PublicBrowseRoot
};

/*
 * Initialise the record database with the ServiceDiscoveryServer
 * and BrowseGroupDescriptor records
 */
bool
db_init(server_t *srv)
{
	sdp_data_t d;

	LIST_INIT(&srv->rlist);
	srv->handle = 0;

	d.next = sds_data;
	d.end = sds_data + sizeof(sds_data);
	if (!db_create(srv, -1, BDADDR_ANY, srv->handle++, &d))
		return false;

	d.next = bgd_data;
	d.end = bgd_data + sizeof(bgd_data);
	if (!db_create(srv, -1, BDADDR_ANY, srv->handle++, &d))
		return false;

	return true;
}

/*
 * Iterate through records selected by fd.  rec should point to a NULL
 * value to start the iteration, and false will be returned when there
 * are no more records to return.
 */
bool
db_next(server_t *srv, int fd, record_t **rec)
{
	record_t *r;

	if (*rec == NULL)
		r = LIST_FIRST(&srv->rlist);
	else
		r = LIST_NEXT(*rec, next);

	while (r != NULL && !FD_ISSET(fd, &r->refset))
		r = LIST_NEXT(r, next);

	*rec = r;
	return (r == NULL) ? false : true;
}

/*
 * Match a ServiceRecord against a UUID. Note that because we already
 * know that the record data is valid, we don't need to recurse here
 * and can just skip over SEQ and ALT headers. Return true if equivalent
 * UUID is found.
 */
static bool
db_match_uuid(record_t *rec, uuid_t *uuid)
{
	uint8_t *p = rec->data.next;
	uuid_t u;

	while (p < rec->data.end) {
		switch(*p++) {
		case SDP_DATA_NIL:
			break;

		case SDP_DATA_BOOL:
		case SDP_DATA_INT8:
		case SDP_DATA_UINT8:
		case SDP_DATA_SEQ8:
		case SDP_DATA_ALT8:
			p += 1;
			break;

		case SDP_DATA_INT16:
		case SDP_DATA_UINT16:
		case SDP_DATA_SEQ16:
		case SDP_DATA_ALT16:
			p += 2;
			break;

		case SDP_DATA_INT32:
		case SDP_DATA_UINT32:
		case SDP_DATA_SEQ32:
		case SDP_DATA_ALT32:
			p += 4;
			break;

		case SDP_DATA_INT64:
		case SDP_DATA_UINT64:
			p += 8;
			break;

		case SDP_DATA_INT128:
		case SDP_DATA_UINT128:
			p += 16;
			break;

		case SDP_DATA_STR8:
		case SDP_DATA_URL8:
			p += 1 + *p;
			break;

		case SDP_DATA_STR16:
		case SDP_DATA_URL16:
			p += 2 + be16dec(p);
			break;

		case SDP_DATA_STR32:
		case SDP_DATA_URL32:
			p += 4 + be32dec(p);
			break;

		case SDP_DATA_UUID16:
			u = BLUETOOTH_BASE_UUID;
			u.time_low = be16dec(p);

			if (uuid_equal(&u, uuid, NULL))
				return true;

			p += 2;
			break;

		case SDP_DATA_UUID32:
			u = BLUETOOTH_BASE_UUID;
			u.time_low = be32dec(p);

			if (uuid_equal(&u, uuid, NULL))
				return true;

			p += 4;
			break;

		case SDP_DATA_UUID128:
			uuid_dec_be(p, &u);

			if (uuid_equal(&u, uuid, NULL))
				return true;

			p += 16;
			break;

		default:
			return false;
		}
	}

	return false;
}

/*
 * Select ServiceRecords matching ServiceSearchPattern
 *
 * A record is selected when it is visible to the client and
 * contains each and every UUID from the ServiceSearchPattern
 */
void
db_select_ssp(server_t *srv, int fd, sdp_data_t *ssp)
{
	record_t *r;
	sdp_data_t s;
	uuid_t u;

	LIST_FOREACH(r, &srv->rlist, next) {
		if (!r->valid)
			continue;

		if (!srv->fdidx[fd].control
		    && !bdaddr_any(&r->bdaddr)
		    && !bdaddr_same(&r->bdaddr, &srv->fdidx[fd].bdaddr))
			continue;

		s = *ssp;
		for (;;) {
			if (!sdp_get_uuid(&s, &u)) {
				/* matched all UUIDs */
				FD_SET(fd, &r->refset);
				r->refcnt++;
				break;
			}

			if (!db_match_uuid(r, &u)) {
				/* does not match UUID */
				break;
			}
		}
	}
}

/*
 * Select a ServiceRecord given the RecordHandle.
 */
void
db_select_handle(server_t *srv, int fd, uint32_t handle)
{
	record_t *r;

	LIST_FOREACH(r, &srv->rlist, next) {
		if (!r->valid)
			continue;

		if (!srv->fdidx[fd].control
		    && !bdaddr_any(&r->bdaddr)
		    && !bdaddr_same(&r->bdaddr, &srv->fdidx[fd].bdaddr))
			continue;

		if (handle == r->handle) {
			FD_SET(fd, &r->refset);
			r->refcnt++;
			break;
		}
	}
}

/*
 * Create a record and insert in server record list in ascending handle
 * order. Where a selectable record exists with the same handle number,
 * it will be expired.
 */
bool
db_create(server_t *srv, int fd, const bdaddr_t *bdaddr, uint32_t handle, sdp_data_t *data)
{
	record_t *n, *r, *rec;
	sdp_data_t d, v;
	uint16_t a;
	size_t len;

	d = *data;
	if (!sdp_get_attr(&d, &a, &v)
	    || a != SDP_ATTR_SERVICE_RECORD_HANDLE
	    || sdp_data_type(&v) != SDP_DATA_UINT32)
		return false;

	sdp_set_uint(&v, handle);

	len = data->end - data->next;
	rec = malloc(sizeof(record_t) + len);
	if (rec == NULL)
		return false;

	memset(rec, 0, sizeof(record_t));
	FD_ZERO(&rec->refset);
	rec->handle = handle;
	rec->valid = true;
	rec->fd = fd;
	bdaddr_copy(&rec->bdaddr, bdaddr);
	rec->data.next = rec->ext;
	rec->data.end = rec->ext + len;
	memcpy(rec->ext, data->next, len);

	/*
	 * Note, this does not handle the case where we expire
	 * the first record on the list, as that won't happen.
	 */
	n = LIST_FIRST(&srv->rlist);
	if (n != NULL) {
		do {
			r = n;
			n = LIST_NEXT(r, next);
		} while (n != NULL && n->handle < handle);

		if (n != NULL && n->valid && n->handle == handle) {
			if (n->refcnt-- == 0) {
				LIST_REMOVE(n, next);
				free(n);
			} else {
				n->valid = false;
				n->fd = -1;
			}
		}

		LIST_INSERT_AFTER(r, rec, next);
	} else {
		LIST_INSERT_HEAD(&srv->rlist, rec, next);
	}

	return true;
}

/*
 * Unselect any ServiceRecords selected by fd
 */
void
db_unselect(server_t *srv, int fd)
{
	record_t *n, *r;

	n = LIST_FIRST(&srv->rlist);
	while (n != NULL) {
		r = n;
		n = LIST_NEXT(r, next);

		if (FD_ISSET(fd, &r->refset)) {
			if (r->refcnt-- == 0) {
				LIST_REMOVE(r, next);
				free(r);
			} else {
				FD_CLR(fd, &r->refset);
			}
		}
	}
}

/*
 * Invalidate or release all records owned by fd
 */
void
db_release(server_t *srv, int fd)
{
	record_t *n, *r;

	n = LIST_FIRST(&srv->rlist);
	while (n != NULL) {
		r = n;
		n = LIST_NEXT(r, next);

		if (r->fd == fd) {
			if (r->refcnt-- == 0) {
				LIST_REMOVE(r, next);
				free(r);
			} else {
				r->valid = false;
				r->fd = -1;
			}
		}
	}
}
