/*	$NetBSD: keyboard.c,v 1.2 2018/08/12 13:02:40 christos Exp $	*/

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


#include <config.h>

#include <sys/types.h>

#include <windows.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <io.h>

#include <isc/keyboard.h>
#include <isc/util.h>

isc_result_t
isc_keyboard_open(isc_keyboard_t *keyboard) {
	int fd;

	REQUIRE(keyboard != NULL);

	fd = _fileno(stdin);
	if (fd < 0)
		return (ISC_R_IOERROR);

	keyboard->fd = fd;

	keyboard->result = ISC_R_SUCCESS;

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_keyboard_close(isc_keyboard_t *keyboard, unsigned int sleeptime) {
	REQUIRE(keyboard != NULL);

	if (sleeptime > 0 && keyboard->result != ISC_R_CANCELED)
		(void)Sleep(sleeptime*1000);

	keyboard->fd = -1;

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_keyboard_getchar(isc_keyboard_t *keyboard, unsigned char *cp) {
	ssize_t cc;
	unsigned char c;

	REQUIRE(keyboard != NULL);
	REQUIRE(cp != NULL);

	cc = read(keyboard->fd, &c, 1);
	if (cc < 0) {
		keyboard->result = ISC_R_IOERROR;
		return (keyboard->result);
	}

	*cp = c;

	return (ISC_R_SUCCESS);
}

isc_boolean_t
isc_keyboard_canceled(isc_keyboard_t *keyboard) {
	return (ISC_TF(keyboard->result == ISC_R_CANCELED));
}

