/*	$NetBSD: tp.c,v 1.5 2006/05/22 20:17:03 uwe Exp $	*/

/*-
 * Copyright (c) 2002, 2003 TAKEMRUA Shin
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
 * 3. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>

#include "tpctl.h"

#define MIN(a, b)	((a) < (b) ? (a) : (b))

#ifndef lint
#include <sys/cdefs.h>
__RCSID("$NetBSD: tp.c,v 1.5 2006/05/22 20:17:03 uwe Exp $");
#endif /* not lint */

int
tp_init(struct tp *tp, int fd)
{
	u_int flags;
	struct wsmouse_calibcoords calibcoords;
	struct wsmouse_id id;

	tp->fd = fd;

#if 0
	if (ioctl(tp->fd, WSMOUSEIO_GTYPE, &type) < 0)
		return (-1);
	if (type != WSMOUSE_TYPE_TPANEL) {
		errno = EINVAL;
		return (-1);
	}
#else
	if (ioctl(tp->fd, WSMOUSEIO_GCALIBCOORDS, &calibcoords) < 0)
		return (-1);
#endif
	flags = fcntl(tp->fd, F_GETFL);
	if (flags == -1)
		return (-1);
	flags |= O_NONBLOCK;
	if (fcntl(tp->fd, F_SETFL, flags) < 0)
		return (-1);

	id.type = WSMOUSE_ID_TYPE_UIDSTR;
	if (ioctl(tp->fd, WSMOUSEIO_GETID, &id) == 0) {
		(void)strlcpy(tp->id, (char *)id.data,
			      MIN(sizeof(tp->id), id.length));
	} else {
		tp->id[0] = '*';
		tp->id[1] = '\0';
	}

	return (0);
}

int
tp_setrawmode(struct tp *tp)
{
	struct wsmouse_calibcoords raw;

	memset(&raw, 0, sizeof(raw));
	raw.samplelen = WSMOUSE_CALIBCOORDS_RESET;

	return ioctl(tp->fd, WSMOUSEIO_SCALIBCOORDS, &raw);
}

int
tp_setcalibcoords(struct tp *tp, struct wsmouse_calibcoords *calibcoords)
{
	return ioctl(tp->fd, WSMOUSEIO_SCALIBCOORDS, calibcoords);
}

int
tp_flush(struct tp *tp)
{
	struct wscons_event ev;
	int count;

	count = 0;
	while (read(tp->fd, &ev, sizeof(ev)) == sizeof(ev)) {
		switch (ev.type) {
		case WSCONS_EVENT_MOUSE_UP:
		case WSCONS_EVENT_MOUSE_DOWN:
		case WSCONS_EVENT_MOUSE_DELTA_X:
		case WSCONS_EVENT_MOUSE_DELTA_Y:
		case WSCONS_EVENT_MOUSE_ABSOLUTE_X:
		case WSCONS_EVENT_MOUSE_ABSOLUTE_Y:
		case WSCONS_EVENT_MOUSE_DELTA_Z:
		case WSCONS_EVENT_MOUSE_ABSOLUTE_Z:
			count++;
			break;

		default:
			break;
		}
	}

	return (count);
}

int
tp_get(struct tp *tp, int *x, int *y, int (*cancel)(void *), void *data)
{
	struct wscons_event ev;
	int x_done, y_done, res;

	x_done = y_done = 0;
	while (1) {
		if (cancel != NULL && (res = (*cancel)(data)) != 0)
			return (res);
		if ((res = read(tp->fd, &ev, sizeof(ev))) < 0) {
			if (errno != EWOULDBLOCK)
				return (-1);
			continue;
		}
		if (res != sizeof(ev)) {
			errno = EINVAL;
			return (-1);
		}
		switch (ev.type) {
		case WSCONS_EVENT_MOUSE_ABSOLUTE_X:
			*x = ev.value;
			if (y_done)
				return (0);
			x_done = 1;
			break;

		case WSCONS_EVENT_MOUSE_ABSOLUTE_Y:
			*y = ev.value;
			if (x_done)
				return (0);
			y_done = 1;
			break;

		default:
			break;
		}
	}
}

int
tp_waitup(struct tp *tp, int msec, int (*cancel)(void*), void *data)
{
	int res;

	while (1) {
		if (cancel != NULL && (res = (*cancel)(data)) != 0)
			return (res);
		usleep(msec * 1000);
		if (tp_flush(tp) == 0)
			break;
	}

	return (0);
}
