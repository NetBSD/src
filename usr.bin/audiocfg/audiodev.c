/* $NetBSD: audiodev.c,v 1.15 2019/08/24 07:39:42 isaki Exp $ */

/*
 * Copyright (c) 2010 Jared D. McNeill <jmcneill@invisible.ca>
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

#include <sys/queue.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/drvctlio.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "audiodev.h"
#include "drvctl.h"
#include "dtmf.h"

static int audiodev_test_chmask(struct audiodev *, unsigned int,
	audio_info_t *);

static TAILQ_HEAD(audiodevhead, audiodev) audiodevlist =
    TAILQ_HEAD_INITIALIZER(audiodevlist);

static int
audiodev_getinfo(struct audiodev *adev)
{
	struct stat st;
	struct audiofmt *f;
	audio_format_query_t query;
	int i;

	if (stat(adev->ctlpath, &st) == -1)
		return -1;
	adev->dev = st.st_rdev;

	if (stat(_PATH_AUDIOCTL, &st) != -1 && st.st_rdev == adev->dev)
		adev->defaultdev = true;

	adev->ctlfd = open(adev->ctlpath, O_RDONLY);
	if (adev->ctlfd == -1) {
			return -1;
	}
	if (ioctl(adev->ctlfd, AUDIO_GETDEV, &adev->audio_device) == -1) {
		close(adev->ctlfd);
		return -1;
	}

	for (i = 0; ;i++) {
		memset(&query, 0, sizeof(query));
		query.index = i;
		if (ioctl(adev->ctlfd, AUDIO_QUERYFORMAT, &query) == -1) {
			if (errno == ENODEV) {
				/* QUERYFORMAT not supported. */
				break;
			}
			if (errno == EINVAL)
				break;
			close(adev->ctlfd);
			return -1;
		}

		f = calloc(1, sizeof(*f));
		f->fmt = query.fmt;
		TAILQ_INSERT_TAIL(&adev->formats, f, next);
	}

	if (ioctl(adev->ctlfd, AUDIO_GETFORMAT, &adev->hwinfo) == -1) {
		close(adev->ctlfd);
		return -1;
	}

	return 0;
}

static int
audiodev_add(const char *pdev, const char *dev, unsigned int unit)
{
	struct audiodev *adev;

	adev = calloc(1, sizeof(*adev));
	if (adev == NULL) 
		return -1;

	strlcpy(adev->pxname, pdev, sizeof(adev->pxname));
	strlcpy(adev->xname, dev, sizeof(adev->xname));
	snprintf(adev->path, sizeof(adev->path), "/dev/%s", dev);
	snprintf(adev->ctlpath, sizeof(adev->ctlpath), "/dev/audioctl%d", unit);
	adev->unit = unit;
	TAILQ_INIT(&adev->formats);

	if (audiodev_getinfo(adev) == -1) {
		free(adev);
		return -1;
	}

#ifdef DEBUG
	printf("DEBUG: [%c] %s(%s): %s\n", adev->defaultdev ? '*' : ' ',
	    adev->path, adev->ctlpath, adev->audio_device.name);
	struct audiofmt *f;
	TAILQ_FOREACH(f, &adev->formats, next) {
		printf("DEBUG: enc%d, %d/%d, %dch\n",
		    f->fmt.encoding,
		    f->fmt.validbits,
		    f->fmt.precision,
		    f->fmt.channels);
	}
#endif

	TAILQ_INSERT_TAIL(&audiodevlist, adev, next);

	return 0;
}

static void
audiodev_cb(void *args, const char *pdev, const char *dev, unsigned int unit)
{
	audiodev_add(pdev, dev, unit);
}

int
audiodev_refresh(void)
{
	struct audiodev *adev;
	int fd, error;

	fd = open(DRVCTLDEV, O_RDONLY);
	if (fd == -1) {
		warn("open %s", DRVCTLDEV);
		return -1;
	}

	while (!TAILQ_EMPTY(&audiodevlist)) {
		adev = TAILQ_FIRST(&audiodevlist);
		if (adev->ctlfd != -1)
			close(adev->ctlfd);
		TAILQ_REMOVE(&audiodevlist, adev, next);
		free(adev);
	}

	error = drvctl_foreach(fd, "audio", audiodev_cb, NULL);
	if (error == -1) {
		warnx("drvctl failed");
		return -1;
	}

	close(fd);

	return 0;
}

unsigned int
audiodev_count(void)
{
	struct audiodev *adev;
	unsigned int n;

	n = 0;
	TAILQ_FOREACH(adev, &audiodevlist, next)
		++n;

	return n;
}

struct audiodev *
audiodev_get(unsigned int i)
{
	struct audiodev *adev;
	unsigned int n;

	n = 0;
	TAILQ_FOREACH(adev, &audiodevlist, next) {
		if (n == i)
			return adev;
		++n;
	}

	return NULL;
}

int
audiodev_set_default(struct audiodev *adev)
{
	char audiopath[PATH_MAX+1];
	char soundpath[PATH_MAX+1];
	char audioctlpath[PATH_MAX+1];
	char mixerpath[PATH_MAX+1];

	snprintf(audiopath, sizeof(audiopath),
	    _PATH_AUDIO "%u", adev->unit);
	snprintf(soundpath, sizeof(soundpath),
	    _PATH_SOUND "%u", adev->unit);
	snprintf(audioctlpath, sizeof(audioctlpath),
	    _PATH_AUDIOCTL "%u", adev->unit);
	snprintf(mixerpath, sizeof(mixerpath),
	    _PATH_MIXER "%u", adev->unit);

	unlink(_PATH_AUDIO);
	unlink(_PATH_SOUND);
	unlink(_PATH_AUDIOCTL);
	unlink(_PATH_MIXER);

	if (symlink(audiopath, _PATH_AUDIO) == -1) {
		warn("symlink %s", _PATH_AUDIO);
		return -1;
	}
	if (symlink(soundpath, _PATH_SOUND) == -1) {
		warn("symlink %s", _PATH_SOUND);
		return -1;
	}
	if (symlink(audioctlpath, _PATH_AUDIOCTL) == -1) {
		warn("symlink %s", _PATH_AUDIOCTL);
		return -1;
	}
	if (symlink(mixerpath, _PATH_MIXER) == -1) {
		warn("symlink %s", _PATH_MIXER);
		return -1;
	}

	return 0;
}

int
audiodev_set_param(struct audiodev *adev, int mode,
	const char *encname, unsigned int prec, unsigned int ch, unsigned int freq)
{
	audio_info_t ai;
	int setmode;
	u_int enc;

	setmode = 0;
	ai = adev->hwinfo;

	for (enc = 0; enc < encoding_max; enc++) {
		if (strcmp(encname, encoding_names[enc]) == 0)
			break;
	}
	if (enc >= encoding_max) {
		warnx("unknown encoding name: %s", encname);
		return -1;
	}

	if ((ai.mode & mode & AUMODE_PLAY)) {
		setmode |= AUMODE_PLAY;
		ai.play.encoding = enc;
		ai.play.precision = prec;
		ai.play.channels = ch;
		ai.play.sample_rate = freq;
	}
	if ((ai.mode & mode & AUMODE_RECORD)) {
		setmode |= AUMODE_RECORD;
		ai.record.encoding = enc;
		ai.record.precision = prec;
		ai.record.channels = ch;
		ai.record.sample_rate = freq;
	}

	ai.mode = setmode;
	printf("setting %s to %s:%u, %uch, %uHz\n",
	    adev->xname, encname, prec, ch, freq);
	if (ioctl(adev->ctlfd, AUDIO_SETFORMAT, &ai) == -1) {
		warn("ioctl AUDIO_SETFORMAT");
		return -1;
	}
	return 0;
}

int
audiodev_test(struct audiodev *adev)
{
	audio_info_t info;
	unsigned int i;
	int rv;

	rv = -1;

	adev->fd = open(adev->path, O_WRONLY);
	if (adev->fd == -1) {
		warn("open %s", adev->path);
		return -1;
	}

	AUDIO_INITINFO(&info);
	info.play.sample_rate = adev->hwinfo.play.sample_rate;
	info.play.channels = adev->hwinfo.play.channels;
	info.play.precision = 16;
	info.play.encoding = AUDIO_ENCODING_SLINEAR_LE;
	info.mode = AUMODE_PLAY;
	if (ioctl(adev->fd, AUDIO_SETINFO, &info) == -1) {
		warn("ioctl AUDIO_SETINFO");
		goto done;
	}
	if (ioctl(adev->fd, AUDIO_GETBUFINFO, &info) == -1) {
		warn("ioctl AUDIO_GETBUFINFO");
		goto done;
	}

	for (i = 0; i < adev->hwinfo.play.channels; i++) {
		printf("  testing channel %u...", i);
		fflush(stdout);
		if (audiodev_test_chmask(adev, 1 << i, &info) == -1)
			goto done;
		printf(" done\n");
	}

	rv = 0;
done:
	close(adev->fd);
	return rv;
}

static int
audiodev_test_chmask(struct audiodev *adev, unsigned int chanmask,
	audio_info_t *info)
{
	int16_t *buf;
	size_t buflen;
	off_t off;
	int rv;

	rv = -1;

	dtmf_new(&buf, &buflen, adev->hwinfo.play.sample_rate, 2,
	    adev->hwinfo.play.channels, chanmask, 350.0, 440.0);
	if (buf == NULL) {
		return -1;
	}

	off = 0;
	while (buflen > 0) {
		size_t wlen; 
		ssize_t ret;

		wlen = info->play.buffer_size;
		if (wlen > buflen)
			wlen = buflen;
		ret = write(adev->fd, (char *)buf + off, wlen);
		if (ret == -1) {
			warn("write");
			goto done;
		}
		wlen = ret;
		off += wlen;
		buflen -= wlen;
	}

	if (ioctl(adev->fd, AUDIO_DRAIN) == -1) {
		warn("ioctl AUDIO_DRAIN");
		goto done;
	}

	rv = 0;
done:
	free(buf);
	return rv;
}
