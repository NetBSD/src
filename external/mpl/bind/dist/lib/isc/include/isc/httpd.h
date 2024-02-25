/*	$NetBSD: httpd.h,v 1.6.2.1 2024/02/25 15:47:21 martin Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#pragma once

/*! \file */

#include <stdbool.h>

#include <isc/event.h>
#include <isc/eventclass.h>
#include <isc/mutex.h>
#include <isc/task.h>
#include <isc/time.h>
#include <isc/types.h>
#include <isc/url.h>

#define HTTPD_EVENTCLASS ISC_EVENTCLASS(4300)
#define HTTPD_SHUTDOWN	 (HTTPD_EVENTCLASS + 0x0001)

#define ISC_HTTPDMGR_SHUTTINGDOWN 0x00000001

typedef isc_result_t(isc_httpdaction_t)(
	const isc_httpd_t *httpd, const isc_httpdurl_t *urlinfo, void *arg,
	unsigned int *retcode, const char **retmsg, const char **mimetype,
	isc_buffer_t *body, isc_httpdfree_t **freecb, void **freecb_args);

typedef bool(isc_httpdclientok_t)(const isc_sockaddr_t *, void *);

isc_result_t
isc_httpdmgr_create(isc_nm_t *nm, isc_mem_t *mctx, isc_sockaddr_t *addr,
		    isc_httpdclientok_t	 *client_ok,
		    isc_httpdondestroy_t *ondestroy, void *cb_arg,
		    isc_httpdmgr_t **httpdmgrp);

void
isc_httpdmgr_shutdown(isc_httpdmgr_t **httpdp);

isc_result_t
isc_httpdmgr_addurl(isc_httpdmgr_t *httpdmgr, const char *url, bool isstatic,
		    isc_httpdaction_t *func, void *arg);

void
isc_httpd_setfinishhook(void (*fn)(void));

bool
isc_httpdurl_isstatic(const isc_httpdurl_t *url);

const isc_time_t *
isc_httpdurl_loadtime(const isc_httpdurl_t *url);

const isc_time_t *
isc_httpd_if_modified_since(const isc_httpd_t *httpd);
