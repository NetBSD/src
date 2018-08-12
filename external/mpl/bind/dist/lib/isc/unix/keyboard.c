/*	$NetBSD: keyboard.c,v 1.1.1.1 2018/08/12 12:08:25 christos Exp $	*/

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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/uio.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#include <isc/keyboard.h>
#include <isc/util.h>

isc_result_t
isc_keyboard_open(isc_keyboard_t *keyboard) {
	int fd;
	isc_result_t ret;
	struct termios current_mode;

	REQUIRE(keyboard != NULL);

	fd = open("/dev/tty", O_RDONLY, 0);
	if (fd < 0)
		return (ISC_R_IOERROR);

	keyboard->fd = fd;

	if (tcgetattr(fd, &keyboard->saved_mode) < 0) {
		ret = ISC_R_IOERROR;
		goto errout;
	}

	current_mode = keyboard->saved_mode;

	current_mode.c_iflag &=
			~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
	current_mode.c_oflag &= ~OPOST;
	current_mode.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
	current_mode.c_cflag &= ~(CSIZE|PARENB);
	current_mode.c_cflag |= CS8;

	current_mode.c_cc[VMIN] = 1;
	current_mode.c_cc[VTIME] = 0;
	if (tcsetattr(fd, TCSAFLUSH, &current_mode) < 0) {
		ret = ISC_R_IOERROR;
		goto errout;
	}

	keyboard->result = ISC_R_SUCCESS;

	return (ISC_R_SUCCESS);

 errout:
	close (fd);

	return (ret);
}

isc_result_t
isc_keyboard_close(isc_keyboard_t *keyboard, unsigned int sleeptime) {
	REQUIRE(keyboard != NULL);

	if (sleeptime > 0 && keyboard->result != ISC_R_CANCELED)
		(void)sleep(sleeptime);

	(void)tcsetattr(keyboard->fd, TCSAFLUSH, &keyboard->saved_mode);
	(void)close(keyboard->fd);

	keyboard->fd = -1;

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_keyboard_getchar(isc_keyboard_t *keyboard, unsigned char *cp) {
	ssize_t cc;
	unsigned char c;
	cc_t *controlchars;

	REQUIRE(keyboard != NULL);
	REQUIRE(cp != NULL);

	cc = read(keyboard->fd, &c, 1);
	if (cc < 0) {
		keyboard->result = ISC_R_IOERROR;
		return (keyboard->result);
	}

	controlchars = keyboard->saved_mode.c_cc;
	if (c == controlchars[VINTR] || c == controlchars[VQUIT]) {
		keyboard->result = ISC_R_CANCELED;
		return (keyboard->result);
	}

	*cp = c;

	return (ISC_R_SUCCESS);
}

isc_boolean_t
isc_keyboard_canceled(isc_keyboard_t *keyboard) {
	return (ISC_TF(keyboard->result == ISC_R_CANCELED));
}
