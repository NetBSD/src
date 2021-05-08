/* $NetBSD: parse.c,v 1.2 2021/05/08 12:53:15 nia Exp $ */
/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nia Alarie.
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
#include <sys/audioio.h>
#include <sys/ioctl.h>
#include <curses.h>
#include <stdlib.h>
#include "app.h"
#include "parse.h"

static struct aiomixer_class *get_class(struct aiomixer *, int);
static int compare_control(const void *, const void *);

static struct aiomixer_class *
get_class(struct aiomixer *aio, int class)
{
	size_t i;

	for (i = 0; i < aio->numclasses; ++i) {
		if (aio->classes[i].index == class) {
			return &aio->classes[i];
		}
	}
	return NULL;
}

static int
compare_control(const void *pa, const void *pb)
{
	const struct aiomixer_control *a = (const struct aiomixer_control *)pa;
	const struct aiomixer_control *b = (const struct aiomixer_control *)pb;

	if (a->info.prev != AUDIO_MIXER_LAST ||
	    b->info.prev != AUDIO_MIXER_LAST) {
		if (b->info.prev == a->info.index)
			return -1;
		if (a->info.prev == b->info.index)
			return 1;
	} else {
		return strcmp(a->info.label.name, b->info.label.name);
	}
	return 0;
}

int
aiomixer_parse(struct aiomixer *aio)
{
	size_t i;
	struct mixer_devinfo info;
	struct aiomixer_class *class;
	struct aiomixer_control *control;

	for (info.index = 0;
	    ioctl(aio->fd, AUDIO_MIXER_DEVINFO, &info) != -1; ++info.index) {
		if (info.type != AUDIO_MIXER_CLASS)
			continue;
		if (aio->numclasses >= __arraycount(aio->classes))
			break;
		class = &aio->classes[aio->numclasses++];
		memcpy(class->name, info.label.name, MAX_AUDIO_DEV_LEN);
		class->index = info.index;
		class->numcontrols = 0;
	}
	for (info.index = 0;
	    ioctl(aio->fd, AUDIO_MIXER_DEVINFO, &info) != -1; ++info.index) {
		if (info.type == AUDIO_MIXER_CLASS)
			continue;
		if (info.type == AUDIO_MIXER_VALUE) {
			/* XXX workaround for hdaudio(4) bugs */
			if (info.un.v.delta > AUDIO_MAX_GAIN)
				continue;
			if (info.un.v.num_channels == 0)
				continue;
		}
		class = get_class(aio, info.mixer_class);
		if (class == NULL)
			break;
		if (class->numcontrols >= __arraycount(class->controls))
			break;
		control = &class->controls[class->numcontrols++];
		control->info = info;
	}
	for (i = 0; i < aio->numclasses; ++i) {
		class = &aio->classes[i];
		qsort(class->controls, class->numcontrols,
		    sizeof(struct aiomixer_control),
		    compare_control);
	}
	return 0;
}

