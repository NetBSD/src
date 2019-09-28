/* $NetBSD: audiodev.h,v 1.5.2.1 2019/09/28 07:41:18 martin Exp $ */

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

#ifndef _HAVE_AUDIODEV_H
#define _HAVE_AUDIODEV_H

#include <sys/audioio.h>
#include <sys/queue.h>
#include <sys/syslimits.h>

#include <stdbool.h>

struct audiofmt {
	struct audio_format fmt;
	TAILQ_ENTRY(audiofmt) next;
};

struct audiodev {
	char pxname[16];	/* hw (parent) device */
	char xname[16];		/* audio(4) device */
	uint16_t unit;
	char path[PATH_MAX+1];
	char ctlpath[PATH_MAX+1];
	int fd;
	int ctlfd;

	dev_t dev;
	bool defaultdev;

	audio_device_t audio_device;
	TAILQ_HEAD(, audiofmt) formats;
	audio_info_t hwinfo;

	TAILQ_ENTRY(audiodev) next;
};

int			audiodev_refresh(void);
unsigned int		audiodev_count(void);
struct audiodev *	audiodev_get(unsigned int);
int			audiodev_set_default(struct audiodev *);
int			audiodev_set_param(struct audiodev *, int,
				const char *, unsigned int, unsigned int,
				unsigned int);
int			audiodev_test(struct audiodev *);

extern const char *	encoding_names[];
extern u_int		encoding_max;

#endif /* !_HAVE_AUDIODEV_H */
