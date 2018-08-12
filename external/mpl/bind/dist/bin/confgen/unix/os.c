/*	$NetBSD: os.c,v 1.1.1.1 2018/08/12 12:07:17 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */


/*! \file */

#include <config.h>

#include <confgen/os.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>

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
