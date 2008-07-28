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

/** @file lock.c
 * Tests various potential failures of the DRM locking mechanisms
 */

#include <limits.h>
#include "drmtest.h"

enum auth_event {
	SERVER_READY,
	CLIENT_MAGIC,
	SERVER_LOCKED,
	CLIENT_LOCKED,
};

int commfd[2];
unsigned int lock1 = 0x00001111;
unsigned int lock2 = 0x00002222;

/* return time in milliseconds */
static unsigned int
get_millis()
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static void
wait_event(int pipe, enum auth_event expected_event)
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

static void
client_auth(int drmfd)
{
	struct drm_auth auth;
	int ret;

	/* Get a client magic number and pass it to the master for auth. */
	ret = ioctl(drmfd, DRM_IOCTL_GET_MAGIC, &auth);
	if (ret == -1)
		err(1, "Couldn't get client magic");
	send_event(0, CLIENT_MAGIC);
	ret = write(commfd[0], &auth.magic, sizeof(auth.magic));
	if (ret == -1)
		err(1, "Couldn't write auth data");
}

static void
server_auth(int drmfd)
{
	struct drm_auth auth;
	int ret;

	send_event(1, SERVER_READY);
	wait_event(1, CLIENT_MAGIC);
	ret = read(commfd[1], &auth.magic, sizeof(auth.magic));
	if (ret == -1)
		err(1, "Failure to read client magic");

	ret = ioctl(drmfd, DRM_IOCTL_AUTH_MAGIC, &auth);
	if (ret == -1)
		err(1, "Failure to authenticate client magic\n");
}

/** Tests that locking is successful in normal conditions */
static void
test_lock_unlock(int drmfd)
{
	int ret;

	ret = drmGetLock(drmfd, lock1, 0);
	if (ret != 0)
		err(1, "Locking failed");
	ret = drmUnlock(drmfd, lock1);
	if (ret != 0)
		err(1, "Unlocking failed");
}

/** Tests that unlocking the lock while it's not held works correctly */
static void
test_unlock_unlocked(int drmfd)
{
	int ret;

	ret = drmUnlock(drmfd, lock1);
	if (ret == 0)
		err(1, "Unlocking unlocked lock succeeded");
}

/** Tests that unlocking a lock held by another context fails appropriately */
static void
test_unlock_unowned(int drmfd)
{
	int ret;

	ret = drmGetLock(drmfd, lock1, 0);
	assert(ret == 0);
	ret = drmUnlock(drmfd, lock2);
	if (ret == 0)
		errx(1, "Unlocking other context's lock succeeded");
	ret = drmUnlock(drmfd, lock1);
	assert(ret == 0);
}

/**
 * Tests that an open/close by the same process doesn't result in the lock
 * being dropped.
 */
static void test_open_close_locked(drmfd)
{
	int ret, tempfd;

	ret = drmGetLock(drmfd, lock1, 0);
	assert(ret == 0);
	/* XXX: Need to make sure that this is the same device as drmfd */
	tempfd = drm_open_any();
	close(tempfd);
	ret = drmUnlock(drmfd, lock1);
	if (ret != 0)
		errx(1, "lock lost during open/close by same pid");
}

static void client()
{
	int drmfd, ret;
	unsigned int time;

	wait_event(0, SERVER_READY);

	/* XXX: Should make sure we open the same DRM as the master */
	drmfd = drm_open_any();

	client_auth(drmfd);

	/* Wait for the server to grab the lock, then grab it ourselves (to
	 * contest it).  Hopefully we hit it within the window of when the
	 * server locks.
	 */
	wait_event(0, SERVER_LOCKED);
	ret = drmGetLock(drmfd, lock2, 0);
	time = get_millis();
	if (ret != 0)
		err(1, "Failed to get lock on client\n");
	drmUnlock(drmfd, lock2);

	/* Tell the server that our locking completed, and when it did */
	send_event(0, CLIENT_LOCKED);
	ret = write(commfd[0], &time, sizeof(time));

	close(drmfd);
	exit(0);
}

static void server()
{
	int drmfd, tempfd, ret;
	unsigned int client_time, unlock_time;

	drmfd = drm_open_any_master();

	test_lock_unlock(drmfd);
	test_unlock_unlocked(drmfd);
	test_unlock_unowned(drmfd);
	test_open_close_locked(drmfd);

	/* Perform the authentication sequence with the client. */
	server_auth(drmfd);

	/* Now, test that the client attempting to lock while the server
	 * holds the lock works correctly.
	 */
	ret = drmGetLock(drmfd, lock1, 0);
	assert(ret == 0);
	send_event(1, SERVER_LOCKED);
	/* Wait a while for the client to do its thing */
	sleep(1);
	ret = drmUnlock(drmfd, lock1);
	assert(ret == 0);
	unlock_time = get_millis();

	wait_event(1, CLIENT_LOCKED);
	ret = read(commfd[1], &client_time, sizeof(client_time));
	if (ret == -1)
		err(1, "Failure to read client magic");

	if (client_time < unlock_time)
		errx(1, "Client took lock before server released it");

	close(drmfd);
}

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

