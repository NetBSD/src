/* $NetBSD: app.h,v 1.2 2023/06/29 19:06:54 nia Exp $ */
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
#ifndef APP_H
#define APP_H
#include <sys/audioio.h>
#include <stdbool.h>

enum aiomixer_state {
	STATE_DEVICE_SELECT,
	STATE_CLASS_SELECT,
	STATE_CONTROL_SELECT
};

struct aiomixer_control {
	struct mixer_devinfo info;
	/* currently selected index for sets, channel for sliders */
	int setindex;
	int widget_y;
	int height;
	WINDOW *widgetpad;
};

struct aiomixer_class {
	char name[MAX_AUDIO_DEV_LEN];
	struct aiomixer_control controls[128];
	unsigned int numcontrols;
	WINDOW *widgetpad;
	int index;
	int height;
};

struct aiomixer {
	int fd;
	enum aiomixer_state state;
	struct audio_device mixerdev;
	struct aiomixer_class classes[128];
	unsigned int numclasses;
	unsigned int curclass;
	unsigned int curcontrol;
	bool channels_unlocked;
	int class_scroll_y;
	int last_max_x;
	bool widgets_resized;
	WINDOW *header;
	WINDOW *classbar;
	bool use_colour;
};

#define COLOR_CONTROL_SELECTED	1
#define COLOR_LEVELS		2
#define COLOR_SET_SELECTED	3
#define COLOR_ENUM_OFF		4
#define COLOR_ENUM_ON		5
#define COLOR_ENUM_MISC		6

#endif
