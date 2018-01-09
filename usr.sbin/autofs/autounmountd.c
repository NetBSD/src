/*	$NetBSD: autounmountd.c,v 1.1 2018/01/09 03:31:15 christos Exp $	*/

/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * Copyright (c) 2016 The DragonFly Project
 * Copyright (c) 2014 The FreeBSD Foundation
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tomohiro Kusumi <kusumi.tomohiro@gmail.com>.
 *
 * This software was developed by Edward Tomasz Napierala under sponsorship
 * from the FreeBSD Foundation.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#include <sys/cdefs.h>
__RCSID("$NetBSD: autounmountd.c,v 1.1 2018/01/09 03:31:15 christos Exp $");

#include <sys/types.h>
#include <sys/mount.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/fstypes.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include "common.h"

struct automounted_fs {
	TAILQ_ENTRY(automounted_fs)	af_next;
	time_t				af_mount_time;
	bool				af_mark;
	fsid_t				af_fsid;
	char				af_mountpoint[MNAMELEN];
};

static TAILQ_HEAD(, automounted_fs)	automounted;

static struct automounted_fs *
automounted_find(fsid_t fsid)
{
	struct automounted_fs *af;

	TAILQ_FOREACH(af, &automounted, af_next) {
		if (af->af_fsid.__fsid_val[0] == fsid.__fsid_val[0] &&
		    af->af_fsid.__fsid_val[1] == fsid.__fsid_val[1])
			return af;
	}

	return NULL;
}

static struct automounted_fs *
automounted_add(fsid_t fsid, const char *mountpoint)
{
	struct automounted_fs *af;

	af = calloc(1, sizeof(*af));
	if (af == NULL)
		log_err(1, "calloc");
	af->af_mount_time = time(NULL);
	af->af_fsid = fsid;
	strlcpy(af->af_mountpoint, mountpoint, sizeof(af->af_mountpoint));

	TAILQ_INSERT_TAIL(&automounted, af, af_next);

	return af;
}

static void
automounted_remove(struct automounted_fs *af)
{

	TAILQ_REMOVE(&automounted, af, af_next);
	free(af);
}

static void
refresh_automounted(void)
{
	struct automounted_fs *af, *tmpaf;
	struct statvfs *mntbuf;
	int i, nitems;

	nitems = getmntinfo(&mntbuf, MNT_WAIT);
	if (nitems <= 0)
		log_err(1, "getmntinfo");

	log_debugx("refreshing list of automounted filesystems");

	TAILQ_FOREACH(af, &automounted, af_next)
		af->af_mark = false;

	for (i = 0; i < nitems; i++) {
		if (strcmp(mntbuf[i].f_fstypename, "autofs") == 0) {
			log_debugx("skipping %s, filesystem type is autofs",
			    mntbuf[i].f_mntonname);
			continue;
		}

		if ((mntbuf[i].f_flag & MNT_AUTOMOUNTED) == 0) {
			log_debugx("skipping %s, not automounted",
			    mntbuf[i].f_mntonname);
			continue;
		}

		af = automounted_find(mntbuf[i].f_fsidx);
		if (af == NULL) {
			log_debugx("new automounted filesystem found on %s "
			    "(FSID:%d:%d)", mntbuf[i].f_mntonname,
			    mntbuf[i].f_fsidx.__fsid_val[0],
			    mntbuf[i].f_fsidx.__fsid_val[1]);
			af = automounted_add(mntbuf[i].f_fsidx,
			    mntbuf[i].f_mntonname);
		} else {
			log_debugx("already known automounted filesystem "
			    "found on %s (FSID:%d:%d)", mntbuf[i].f_mntonname,
			    mntbuf[i].f_fsidx.__fsid_val[0],
			    mntbuf[i].f_fsidx.__fsid_val[1]);
		}
		af->af_mark = true;
	}

	TAILQ_FOREACH_SAFE(af, &automounted, af_next, tmpaf) {
		if (af->af_mark)
			continue;
		log_debugx("lost filesystem mounted on %s (FSID:%d:%d)",
		    af->af_mountpoint, af->af_fsid.__fsid_val[0],
		    af->af_fsid.__fsid_val[1]);
		automounted_remove(af);
	}
}

static int
do_unmount(const fsid_t fsid __unused, const char *mountpoint)
{
	int error;

	error = unmount(mountpoint, 0);
	if (error != 0) {
		if (errno == EBUSY) {
			log_debugx("cannot unmount %s: %s",
			    mountpoint, strerror(errno));
		} else {
			log_warn("cannot unmount %s", mountpoint);
		}
	}

	return error;
}

static double
expire_automounted(double expiration_time)
{
	struct automounted_fs *af, *tmpaf;
	time_t now;
	double mounted_for, mounted_max = -1.0;
	int error;

	now = time(NULL);

	log_debugx("expiring automounted filesystems");

	TAILQ_FOREACH_SAFE(af, &automounted, af_next, tmpaf) {
		mounted_for = difftime(now, af->af_mount_time);

		if (mounted_for < expiration_time) {
			log_debugx("skipping %s (FSID:%d:%d), mounted "
			    "for %.0f seconds", af->af_mountpoint,
			    af->af_fsid.__fsid_val[0],
			    af->af_fsid.__fsid_val[1],
			    mounted_for);

			if (mounted_for > mounted_max)
				mounted_max = mounted_for;

			continue;
		}

		log_debugx("filesystem mounted on %s (FSID:%d:%d), "
		    "was mounted for %.0f seconds; unmounting",
		    af->af_mountpoint, af->af_fsid.__fsid_val[0],
		    af->af_fsid.__fsid_val[1], mounted_for);
		error = do_unmount(af->af_fsid, af->af_mountpoint);
		if (error != 0) {
			if (mounted_for > mounted_max)
				mounted_max = mounted_for;
		}
	}

	return mounted_max;
}

__dead static void
usage_autounmountd(void)
{

	fprintf(stderr, "Usage: %s [-r time][-t time][-dv]\n",
	    getprogname());
	exit(1);
}

static void
do_wait(int kq, double sleep_time)
{
	struct timespec timeout;
	struct kevent unused;
	int nevents;

	if (sleep_time != -1.0) {
		assert(sleep_time > 0.0);
		timeout.tv_sec = (int)sleep_time;
		timeout.tv_nsec = 0;

		log_debugx("waiting for filesystem event for %.0f seconds",
		    sleep_time);
		nevents = kevent(kq, NULL, 0, &unused, 1, &timeout);
	} else {
		log_debugx("waiting for filesystem event");
		nevents = kevent(kq, NULL, 0, &unused, 1, NULL);
	}
	if (nevents < 0) {
		if (errno == EINTR)
			return;
		log_err(1, "kevent");
	}

	if (nevents == 0) {
		log_debugx("timeout reached");
		assert(sleep_time > 0.0);
	} else {
		log_debugx("got filesystem event");
	}
}

int
main_autounmountd(int argc, char **argv)
{
	struct kevent event;
	int ch, debug = 0, error, kq;
	double expiration_time = 600, retry_time = 600, mounted_max, sleep_time;
	bool dont_daemonize = false;

	while ((ch = getopt(argc, argv, "dr:t:v")) != -1) {
		switch (ch) {
		case 'd':
			dont_daemonize = true;
			debug++;
			break;
		case 'r':
			retry_time = atoi(optarg);
			break;
		case 't':
			expiration_time = atoi(optarg);
			break;
		case 'v':
			debug++;
			break;
		case '?':
		default:
			usage_autounmountd();
		}
	}
	argc -= optind;
	if (argc != 0)
		usage_autounmountd();

	if (retry_time <= 0)
		log_errx(1, "retry time must be greater than zero");
	if (expiration_time <= 0)
		log_errx(1, "expiration time must be greater than zero");

	log_init(debug);

	if (dont_daemonize == false) {
		if (daemon(0, 0) == -1) {
			log_warn("cannot daemonize");
			pidfile_clean();
			exit(1);
		}
	}

	/*
	 * Call pidfile(3) after daemon(3).
	 */
	if (pidfile(NULL) == -1) {
		if (errno == EEXIST)
			log_errx(1, "daemon already running");
		else if (errno == ENAMETOOLONG)
			log_errx(1, "pidfile name too long");
		log_err(1, "cannot create pidfile");
	}
	if (pidfile_lock(NULL) == -1)
		log_err(1, "cannot lock pidfile");

	TAILQ_INIT(&automounted);

	kq = kqueue();
	if (kq < 0)
		log_err(1, "kqueue");

	EV_SET(&event, 0, EVFILT_FS, EV_ADD | EV_CLEAR, 0, 0, (intptr_t)NULL);
	error = kevent(kq, &event, 1, NULL, 0, NULL);
	if (error < 0)
		log_err(1, "kevent");

	for (;;) {
		refresh_automounted();
		mounted_max = expire_automounted(expiration_time);
		if (mounted_max == -1.0) {
			sleep_time = mounted_max;
			log_debugx("no filesystems to expire");
		} else if (mounted_max < expiration_time) {
			sleep_time = 
			    (double)difftime((time_t)expiration_time,
			    (time_t)mounted_max);
			log_debugx("some filesystems expire in %.0f seconds",
			    sleep_time);
		} else {
			sleep_time = retry_time;
			log_debugx("some expired filesystems remain mounted, "
			    "will retry in %.0f seconds", sleep_time);
		}

		do_wait(kq, sleep_time);
	}

	return 0;
}
