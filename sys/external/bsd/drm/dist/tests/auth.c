/*
 * Copyright © 2007 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#include <limits.h>
#include "drmtest.h"

enum auth_event {
	SERVER_READY,
	CLIENT_MAGIC,
	CLIENT_DONE,
};

int commfd[2];

static void wait_event(int pipe, enum auth_event expected_event)
{
	int ret;
	enum auth_event event;
	unsigned char in;

	ret = read(commfd[pipe], &in, 1);
	if (ret == -1)
		err(1, "read error");
	event = in;

	if (event != expected_event)
		errx(1, "unexpected event: %d\n", event);
}

static void
send_event(int pipe, enum auth_event send_event)
{
	int ret;
	unsigned char event;

	event = send_event;
	ret = write(commfd[pipe], &event, 1);
	if (ret == -1)
		err(1, "failed to send event %d", event);
}

static void client()
{
	struct drm_auth auth;
	int drmfd, ret;

	/* XXX: Should make sure we open the same DRM as the master */
	wait_event(0, SERVER_READY);

	drmfd = drm_open_any();

	/* Get a client magic number and pass it to the master for auth. */
	auth.magic = 0; /* Quiet valgrind */
	ret = ioctl(drmfd, DRM_IOCTL_GET_MAGIC, &auth);
	if (ret == -1)
		err(1, "Couldn't get client magic");
	send_event(0, CLIENT_MAGIC);
	ret = write(commfd[0], &auth.magic, sizeof(auth.magic));
	if (ret == -1)
		err(1, "Couldn't write auth data");

	/* Signal that the client is completely done. */
	send_event(0, CLIENT_DONE);
}

static void server()
{
	int drmfd, ret;
	struct drm_auth auth;

	drmfd = drm_open_any_master();

	auth.magic = 0xd0d0d0d0;
	ret = ioctl(drmfd, DRM_IOCTL_AUTH_MAGIC, &auth);
	if (ret != -1 || errno != EINVAL)
		errx(1, "Authenticating bad magic succeeded\n");

	send_event(1, SERVER_READY);

	wait_event(1, CLIENT_MAGIC);
	ret = read(commfd[1], &auth.magic, sizeof(auth.magic));
	if (ret == -1)
		err(1, "Failure to read client magic");

	ret = ioctl(drmfd, DRM_IOCTL_AUTH_MAGIC, &auth);
	if (ret == -1)
		err(1, "Failure to authenticate client magic\n");

	wait_event(1, CLIENT_DONE);
}

/**
 * Checks DRM authentication mechanisms.
 */
int main(int argc, char **argv)
{
	int ret;

	ret = pipe(commfd);
	if (ret == -1)
		err(1, "Couldn't create pipe");

	ret = fork();
	if (ret == -1)
		err(1, "failure to fork client");
	if (ret == 0)
		client();
	else
		server();

	return 0;
}

