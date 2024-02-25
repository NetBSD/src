/*	$NetBSD: os.c,v 1.2.2.2 2024/02/25 15:43:00 martin Exp $	*/

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

/*! \file */

#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <confgen/os.h>

int
set_user(FILE *fd, const char *user) {
	struct passwd *pw;

	pw = getpwnam(user);
	if (pw == NULL) {
		errno = EINVAL;
		return (-1);
	}
	return (fchown(fileno(fd), pw->pw_uid, -1));
}
