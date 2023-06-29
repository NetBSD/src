/* $NetBSD: main.c,v 1.5 2023/06/29 19:06:54 nia Exp $ */
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
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <paths.h>
#include <curses.h>
#include <stdlib.h>
#include <err.h>
#include "app.h"
#include "draw.h"
#include "parse.h"

static void process_device_select(struct aiomixer *, unsigned int);
static void open_device(struct aiomixer *, const char *);
static void __dead usage(void);
static int adjust_level(int, int);
static int select_class(struct aiomixer *, unsigned int);
static int select_control(struct aiomixer *, unsigned int);
static void slide_control(struct aiomixer *, struct aiomixer_control *, bool);
static int toggle_set(struct aiomixer *);
static void step_up(struct aiomixer *);
static void step_down(struct aiomixer *);
static int read_key(struct aiomixer *, int);

static void __dead
usage(void)
{
	fputs("aiomixer [-u] [-d device]\n", stderr);
	exit(1);
}

static int
select_class(struct aiomixer *aio, unsigned int n)
{
	struct aiomixer_class *class;
	unsigned i;

	if (n >= aio->numclasses)
		return -1;

	class = &aio->classes[n];
	aio->widgets_resized = true;
	aio->class_scroll_y = 0;
	aio->curcontrol = 0;
	aio->curclass = n;
	for (i = 0; i < class->numcontrols; ++i) {
		class->controls[i].setindex = -1;
		draw_control(aio, &class->controls[i], false);
	}
	draw_classbar(aio);
	return 0;
}

static int
select_control(struct aiomixer *aio, unsigned int n)
{
	struct aiomixer_class *class;
	struct aiomixer_control *lastcontrol;
	struct aiomixer_control *control;

	class = &aio->classes[aio->curclass];

	if (n >= class->numcontrols)
		return -1;

	lastcontrol = &class->controls[aio->curcontrol];
	lastcontrol->setindex = -1;
	draw_control(aio, lastcontrol, false);

	control = &class->controls[n];
	aio->curcontrol = n;
	control->setindex = 0;
	draw_control(aio, control, true);

	if (aio->class_scroll_y > control->widget_y) {
		aio->class_scroll_y = control->widget_y;
		aio->widgets_resized = true;
	}

	if ((control->widget_y + control->height) >
	    ((getmaxy(stdscr) - 4) + aio->class_scroll_y)) {
		aio->class_scroll_y = control->widget_y;
		aio->widgets_resized = true;
	}
	return 0;
}

static int
adjust_level(int level, int delta)
{
	if (level > (AUDIO_MAX_GAIN - delta))
		return AUDIO_MAX_GAIN;

	if (delta < 0 && level < (AUDIO_MIN_GAIN + (-delta)))
		return AUDIO_MIN_GAIN;

	return level + delta;
}

static void
slide_control(struct aiomixer *aio,
    struct aiomixer_control *control, bool right)
{
	struct mixer_devinfo *info = &control->info;
	struct mixer_ctrl value;
	unsigned char *level;
	int i, delta;
	int cur_index = 0;

	if (info->type != AUDIO_MIXER_SET) {
		value.dev = info->index;
		value.type = info->type;
		if (info->type == AUDIO_MIXER_VALUE)
			value.un.value.num_channels = info->un.v.num_channels;

		if (ioctl(aio->fd, AUDIO_MIXER_READ, &value) < 0)
			err(EXIT_FAILURE, "failed to read mixer control");
	}

	switch (info->type) {
	case AUDIO_MIXER_VALUE:
		delta = right ? info->un.v.delta : -info->un.v.delta;
		/*
		 * work around strange problem where the level can be
		 * increased but not decreased, seen with uaudio(4)
		 */
		if (delta < 16)
			delta *= 2;
		if (aio->channels_unlocked) {
			level = &value.un.value.level[control->setindex];
			*level = (unsigned char)adjust_level(*level, delta);
		} else {
			for (i = 0; i < value.un.value.num_channels; ++i) {
				level = &value.un.value.level[i];
				*level = (unsigned char)adjust_level(*level, delta);
			}
		}
		break;
	case AUDIO_MIXER_ENUM:
		for (i = 0; i < info->un.e.num_mem; ++i) {
			if (info->un.e.member[i].ord == value.un.ord) {
				cur_index = i;
				break;
			}
		}
		if (right) {
			value.un.ord = cur_index < (info->un.e.num_mem - 1) ?
			    info->un.e.member[cur_index + 1].ord :
			    info->un.e.member[0].ord;
		} else {
			value.un.ord = cur_index > 0 ?
			    info->un.e.member[cur_index - 1].ord :
			    info->un.e.member[control->info.un.e.num_mem - 1].ord;
		}
		break;
	case AUDIO_MIXER_SET:
		if (right) {
			control->setindex =
			    control->setindex < (info->un.s.num_mem - 1) ?
				control->setindex + 1 : 0;
		} else {
			control->setindex = control->setindex > 0 ?
			    control->setindex - 1 :
				control->info.un.s.num_mem - 1;
		}
		break;
	}

	if (info->type != AUDIO_MIXER_SET) {
		if (ioctl(aio->fd, AUDIO_MIXER_WRITE, &value) < 0)
			err(EXIT_FAILURE, "failed to adjust mixer control");
	}

	draw_control(aio, control, true);
}

static int
toggle_set(struct aiomixer *aio)
{
	struct mixer_ctrl ctrl;
	struct aiomixer_class *class = &aio->classes[aio->curclass];
	struct aiomixer_control *control = &class->controls[aio->curcontrol];

	ctrl.dev = control->info.index;
	ctrl.type = control->info.type;

	if (control->info.type != AUDIO_MIXER_SET)
		return -1;

	if (ioctl(aio->fd, AUDIO_MIXER_READ, &ctrl) < 0)
		err(EXIT_FAILURE, "failed to read mixer control");

	ctrl.un.mask ^= control->info.un.s.member[control->setindex].mask;

	if (ioctl(aio->fd, AUDIO_MIXER_WRITE, &ctrl) < 0)
		err(EXIT_FAILURE, "failed to read mixer control");

	draw_control(aio, control, true);
	return 0;
}

static void
step_up(struct aiomixer *aio)
{
	struct aiomixer_class *class;
	struct aiomixer_control *control;

	class = &aio->classes[aio->curclass];
	control = &class->controls[aio->curcontrol];

	if (aio->channels_unlocked &&
	    control->info.type == AUDIO_MIXER_VALUE &&
	    control->setindex > 0) {
		control->setindex--;
		draw_control(aio, control, true);
		return;
	}
	select_control(aio, aio->curcontrol - 1);
}

static void
step_down(struct aiomixer *aio)
{
	struct aiomixer_class *class;
	struct aiomixer_control *control;

	class = &aio->classes[aio->curclass];
	control = &class->controls[aio->curcontrol];

	if (aio->channels_unlocked &&
	    control->info.type == AUDIO_MIXER_VALUE &&
	    control->setindex < (control->info.un.v.num_channels - 1)) {
		control->setindex++;
		draw_control(aio, control, true);
		return;
	}

	select_control(aio, (aio->curcontrol + 1) % class->numcontrols);
}

static int
read_key(struct aiomixer *aio, int ch)
{
	struct aiomixer_class *class;
	struct aiomixer_control *control;
	size_t i;

	switch (ch) {
	case KEY_RESIZE:
		class = &aio->classes[aio->curclass];
		resize_widgets(aio);
		draw_header(aio);
		draw_classbar(aio);
		for (i = 0; i < class->numcontrols; ++i) {
			draw_control(aio,
			    &class->controls[i],
			    aio->state == STATE_CONTROL_SELECT ?
				(aio->curcontrol == i) : false);
		}
		break;
	case KEY_LEFT:
	case 'h':
		if (aio->state == STATE_CLASS_SELECT) {
			select_class(aio, aio->curclass > 0 ?
			    aio->curclass - 1 : aio->numclasses - 1);
		} else if (aio->state == STATE_CONTROL_SELECT) {
			class = &aio->classes[aio->curclass];
			slide_control(aio,
			    &class->controls[aio->curcontrol], false);
		}
		break;
	case KEY_RIGHT:
	case 'l':
		if (aio->state == STATE_CLASS_SELECT) {
			select_class(aio,
			    (aio->curclass + 1) % aio->numclasses);
		} else if (aio->state == STATE_CONTROL_SELECT) {
			class = &aio->classes[aio->curclass];
			slide_control(aio,
			    &class->controls[aio->curcontrol], true);
		}
		break;
	case KEY_UP:
	case 'k':
		if (aio->state == STATE_CONTROL_SELECT) {
			if (aio->curcontrol == 0) {
				class = &aio->classes[aio->curclass];
				control = &class->controls[aio->curcontrol];
				control->setindex = -1;
				aio->state = STATE_CLASS_SELECT;
				draw_control(aio, control, false);
			} else {
				step_up(aio);
			}
		}
		break;
	case KEY_DOWN:
	case 'j':
		if (aio->state == STATE_CLASS_SELECT) {
			class = &aio->classes[aio->curclass];
			if (class->numcontrols > 0) {
				aio->state = STATE_CONTROL_SELECT;
				select_control(aio, 0);
			}
		} else if (aio->state == STATE_CONTROL_SELECT) {
			step_down(aio);
		}
		break;
	case '\n':
	case ' ':
		if (aio->state == STATE_CONTROL_SELECT)
			toggle_set(aio);
		break;
	case '1':
		select_class(aio, 0);
		break;
	case '2':
		select_class(aio, 1);
		break;
	case '3':
		select_class(aio, 2);
		break;
	case '4':
		select_class(aio, 3);
		break;
	case '5':
		select_class(aio, 4);
		break;
	case '6':
		select_class(aio, 5);
		break;
	case '7':
		select_class(aio, 6);
		break;
	case '8':
		select_class(aio, 7);
		break;
	case '9':
		select_class(aio, 8);
		break;
	case 'q':
	case '\e':
		if (aio->state == STATE_CONTROL_SELECT) {
			class = &aio->classes[aio->curclass];
			control = &class->controls[aio->curcontrol];
			aio->state = STATE_CLASS_SELECT;
			draw_control(aio, control, false);
			break;
		}
		return 1;
	case 'u':
		aio->channels_unlocked = !aio->channels_unlocked;
		if (aio->state == STATE_CONTROL_SELECT) {
			class = &aio->classes[aio->curclass];
			control = &class->controls[aio->curcontrol];
			if (control->info.type == AUDIO_MIXER_VALUE)
				draw_control(aio, control, true);
		}
		break;
	}

	draw_screen(aio);
	return 0;
}

static void
process_device_select(struct aiomixer *aio, unsigned int num_devices)
{
	unsigned int selected_device = 0;
	char device_path[16];
	int ch;

	draw_mixer_select(num_devices, selected_device);

	while ((ch = getch()) != ERR) {
		switch (ch) {
		case '\n':
			clear();
			(void)snprintf(device_path, sizeof(device_path),
			    "/dev/mixer%d", selected_device);
			open_device(aio, device_path);
			return;
		case KEY_UP:
		case 'k':
			if (selected_device > 0)
				selected_device--;
			else
				selected_device = (num_devices - 1);
			break;
		case KEY_DOWN:
		case 'j':
			if (selected_device < (num_devices - 1))
				selected_device++;
			else
				selected_device = 0;
			break;
		case '1':
			selected_device = 0;
			break;
		case '2':
			selected_device = 1;
			break;
		case '3':
			selected_device = 2;
			break;
		case '4':
			selected_device = 3;
			break;
		case '5':
			selected_device = 4;
			break;
		case '6':
			selected_device = 5;
			break;
		case '7':
			selected_device = 6;
			break;
		case '8':
			selected_device = 7;
			break;
		case '9':
			selected_device = 8;
			break;
		}
		draw_mixer_select(num_devices, selected_device);
	}
}

static void
open_device(struct aiomixer *aio, const char *device)
{
	int ch;

	if ((aio->fd = open(device, O_RDWR)) < 0)
		err(EXIT_FAILURE, "couldn't open mixer device");

	if (ioctl(aio->fd, AUDIO_GETDEV, &aio->mixerdev) < 0)
		err(EXIT_FAILURE, "AUDIO_GETDEV failed");

	aio->state = STATE_CLASS_SELECT;

	aiomixer_parse(aio);

	create_widgets(aio);

	draw_header(aio);
	select_class(aio, 0);
	draw_screen(aio);

	while ((ch = getch()) != ERR) {
		if (read_key(aio, ch) != 0)
			break;
	}
}

static __dead void
on_signal(int dummy)
{
	endwin();
	exit(0);
}

int
main(int argc, char **argv)
{
	const char *mixer_device = NULL;
	extern char *optarg;
	extern int optind;
	struct aiomixer *aio;
	char mixer_path[32];
	unsigned int mixer_count = 0;
	int i, fd;
	int ch;
	char *no_color = getenv("NO_COLOR");

	if ((aio = malloc(sizeof(struct aiomixer))) == NULL) {
		err(EXIT_FAILURE, "malloc failed");
	}

	while ((ch = getopt(argc, argv, "d:u")) != -1) {
		switch (ch) {
		case 'd':
			mixer_device = optarg;
			break;
		case 'u':
			aio->channels_unlocked = true;
			break;
		default:
			usage();
			break;
		}
	}

	argc -= optind;
	argv += optind;

	if (initscr() == NULL)
		err(EXIT_FAILURE, "can't initialize curses");

	(void)signal(SIGHUP, on_signal);
	(void)signal(SIGINT, on_signal);
	(void)signal(SIGTERM, on_signal);

	curs_set(0);
	keypad(stdscr, TRUE);
	cbreak();
	noecho();

	aio->use_colour = true;

	if (!has_colors())
		aio->use_colour = false;

	if (no_color != NULL && no_color[0] != '\0')
		aio->use_colour = false;

	if (aio->use_colour) {
		start_color();
		use_default_colors();
		init_pair(COLOR_CONTROL_SELECTED, COLOR_BLUE, COLOR_BLACK);
		init_pair(COLOR_LEVELS, COLOR_GREEN, COLOR_BLACK);
		init_pair(COLOR_SET_SELECTED, COLOR_BLACK, COLOR_GREEN);
		init_pair(COLOR_ENUM_ON, COLOR_WHITE, COLOR_RED);
		init_pair(COLOR_ENUM_OFF, COLOR_WHITE, COLOR_BLUE);
		init_pair(COLOR_ENUM_MISC, COLOR_BLACK, COLOR_YELLOW);
	}

	if (mixer_device != NULL) {
		open_device(aio, mixer_device);
	} else {
		for (i = 0; i < 16; ++i) {
			(void)snprintf(mixer_path, sizeof(mixer_path),
			    "/dev/mixer%d", i);
			fd = open(mixer_path, O_RDWR);
			if (fd == -1)
				break;
			close(fd);
			mixer_count++;
		}

		if (mixer_count > 1) {
			process_device_select(aio, mixer_count);
		} else {
			open_device(aio, _PATH_MIXER);
		} 
	}

	endwin();
	close(aio->fd);
	free(aio);

	return 0;
}
