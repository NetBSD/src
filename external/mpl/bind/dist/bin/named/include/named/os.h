/*	$NetBSD: os.h,v 1.2.2.2 2024/02/25 15:43:07 martin Exp $	*/

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

#include <pwd.h>
#include <stdbool.h>

#include <isc/types.h>

void
named_os_init(const char *progname);

void
named_os_daemonize(void);

void
named_os_opendevnull(void);

void
named_os_closedevnull(void);

void
named_os_chroot(const char *root);

void
named_os_inituserinfo(const char *username);

void
named_os_changeuser(void);

uid_t
ns_os_uid(void);

void
named_os_adjustnofile(void);

void
named_os_minprivs(void);

FILE *
named_os_openfile(const char *filename, mode_t mode, bool switch_user);

void
named_os_writepidfile(const char *filename, bool first_time);

bool
named_os_issingleton(const char *filename);

void
named_os_shutdown(void);

void
named_os_shutdownmsg(char *command, isc_buffer_t *text);

void
named_os_tzset(void);

void
named_os_started(void);

const char *
named_os_uname(void);
