/*	$NetBSD: display.c,v 1.14.18.1 2008/06/02 13:21:24 mjf Exp $ */

/*-
 * Copyright (c) 1998, 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Juergen Hannken-Illjes.
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

#include <sys/ioctl.h>
#include <sys/time.h>

#include <dev/wscons/wsconsio.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wsconsctl.h"

static int border;
static int dpytype;
static struct wsdisplay_usefontdata font;
static struct wsdisplay_param backlight;
static struct wsdisplay_param brightness;
static struct wsdisplay_param contrast;
static struct wsdisplay_scroll_data scroll_l;
static int msg_default_attrs, msg_default_bg, msg_default_fg;
static int msg_kernel_attrs, msg_kernel_bg, msg_kernel_fg;
static int splash_enable, splash_progress;

struct field display_field_tab[] = {
    { "border",			&border,	FMT_COLOR,	0 },
    { "type",			&dpytype,	FMT_DPYTYPE,	FLG_RDONLY },
    { "font",			&font.name,	FMT_STRING,	FLG_WRONLY },
    { "backlight",		&backlight.curval,  FMT_UINT,	0 },
    { "brightness",		&brightness.curval, FMT_UINT,	FLG_MODIFY },
    { "contrast",		&contrast.curval,   FMT_UINT,	FLG_MODIFY },
    { "scroll.fastlines",	&scroll_l.fastlines, FMT_UINT,	FLG_MODIFY },
    { "scroll.slowlines",	&scroll_l.slowlines, FMT_UINT,	FLG_MODIFY },
    { "msg.default.attrs",	&msg_default_attrs, FMT_ATTRS,	0 },
    { "msg.default.bg",		&msg_default_bg, FMT_COLOR,	0 },
    { "msg.default.fg",		&msg_default_fg, FMT_COLOR,	0 },
    { "msg.kernel.attrs",	&msg_kernel_attrs, FMT_ATTRS,	0 },
    { "msg.kernel.bg",		&msg_kernel_bg, FMT_COLOR,	0 },
    { "msg.kernel.fg",		&msg_kernel_fg, FMT_COLOR,	0 },
    { "splash.enable",		&splash_enable, FMT_UINT,	FLG_WRONLY },
    { "splash.progress",	&splash_progress, FMT_UINT,	FLG_WRONLY },
};

int display_field_tab_len = sizeof(display_field_tab) /
	sizeof(display_field_tab[0]);

void
display_get_values(int fd)
{

	if (field_by_value(&dpytype)->flags & FLG_GET)
		if (ioctl(fd, WSDISPLAYIO_GTYPE, &dpytype) < 0)
			err(EXIT_FAILURE, "WSDISPLAYIO_GTYPE");
	
	if (field_by_value(&border)->flags & FLG_GET)
		if (ioctl(fd, WSDISPLAYIO_GBORDER, &border) < 0)
			field_disable_by_value(&border);
	
	if (field_by_value(&backlight.curval)->flags & FLG_GET) {
		backlight.param = WSDISPLAYIO_PARAM_BACKLIGHT;
		if (ioctl(fd, WSDISPLAYIO_GETPARAM, &backlight) < 0)
			field_disable_by_value(&backlight.curval);
	}

	if (field_by_value(&brightness.curval)->flags & FLG_GET) {
		brightness.param = WSDISPLAYIO_PARAM_BRIGHTNESS;
		if (ioctl(fd, WSDISPLAYIO_GETPARAM, &brightness))
			field_disable_by_value(&brightness.curval);
	}

	if (field_by_value(&contrast.curval)->flags & FLG_GET) {
		contrast.param = WSDISPLAYIO_PARAM_CONTRAST;
		if (ioctl(fd, WSDISPLAYIO_GETPARAM, &contrast))
			field_disable_by_value(&contrast.curval);
	}

	if (field_by_value(&msg_default_attrs)->flags & FLG_GET ||
	    field_by_value(&msg_default_bg)->flags & FLG_GET ||
	    field_by_value(&msg_default_fg)->flags & FLG_GET ||
	    field_by_value(&msg_kernel_attrs)->flags & FLG_GET ||
	    field_by_value(&msg_kernel_bg)->flags & FLG_GET ||
	    field_by_value(&msg_kernel_fg)->flags & FLG_GET) {
		struct wsdisplay_msgattrs ma;

		if (ioctl(fd, WSDISPLAYIO_GMSGATTRS, &ma) < 0) {
			field_disable_by_value(&msg_default_attrs);
			field_disable_by_value(&msg_default_bg);
			field_disable_by_value(&msg_default_fg);
			field_disable_by_value(&msg_kernel_attrs);
			field_disable_by_value(&msg_kernel_bg);
			field_disable_by_value(&msg_kernel_fg);
		} else {
			msg_default_attrs = ma.default_attrs;
			if (ma.default_attrs & WSATTR_WSCOLORS) {
				msg_default_bg = ma.default_bg;
				msg_default_fg = ma.default_fg;
			} else
				msg_default_bg = msg_default_fg = -1;

			msg_kernel_attrs = ma.kernel_attrs;
			if (ma.kernel_attrs & WSATTR_WSCOLORS) {
				msg_kernel_bg = ma.kernel_bg;
				msg_kernel_fg = ma.kernel_fg;
			} else
				msg_kernel_bg = msg_kernel_fg = -1;
		}
	}

	if (field_by_value(&scroll_l.fastlines)->flags & FLG_GET ||
	    field_by_value(&scroll_l.slowlines)->flags & FLG_GET) {
		if (ioctl(fd, WSDISPLAYIO_DGSCROLL, &scroll_l) < 0) {
			field_disable_by_value(&scroll_l.fastlines);
			field_disable_by_value(&scroll_l.slowlines);
		}
	}
}

void
display_put_values(fd)
	int fd;
{

	if (field_by_value(&font.name)->flags & FLG_SET) {
		if (ioctl(fd, WSDISPLAYIO_SFONT, &font) < 0)
			err(EXIT_FAILURE, "WSDISPLAYIO_SFONT");
		pr_field(field_by_value(&font.name), " -> ");
	}

	if (field_by_value(&border)->flags & FLG_SET) {
		if (ioctl(fd, WSDISPLAYIO_SBORDER, &border) < 0)
			err(EXIT_FAILURE, "WSDISPLAYIO_SBORDER");
		pr_field(field_by_value(&border), " -> ");
	}

	if (field_by_value(&backlight.curval)->flags & FLG_SET) {
		backlight.param = WSDISPLAYIO_PARAM_BACKLIGHT;
		if (ioctl(fd, WSDISPLAYIO_SETPARAM, &backlight) < 0)
			err(EXIT_FAILURE, "WSDISPLAYIO_PARAM_BACKLIGHT");
		pr_field(field_by_value(&backlight.curval), " -> ");
	}

	if (field_by_value(&brightness.curval)->flags & FLG_SET) {
		brightness.param = WSDISPLAYIO_PARAM_BRIGHTNESS;
		if (ioctl(fd, WSDISPLAYIO_SETPARAM, &brightness) < 0)
			err(EXIT_FAILURE, "WSDISPLAYIO_PARAM_BRIGHTNESS");
		pr_field(field_by_value(&brightness.curval), " -> ");
	}

	if (field_by_value(&contrast.curval)->flags & FLG_SET) {
		contrast.param = WSDISPLAYIO_PARAM_CONTRAST;
		if (ioctl(fd, WSDISPLAYIO_SETPARAM, &contrast) < 0)
			err(EXIT_FAILURE, "WSDISPLAYIO_PARAM_CONTRAST");
		pr_field(field_by_value(&contrast.curval), " -> ");
	}

	if (field_by_value(&splash_enable)->flags & FLG_SET) {
		if (ioctl(fd, WSDISPLAYIO_SSPLASH, &splash_enable) < 0)
			err(EXIT_FAILURE, "WSDISPLAYIO_SSPLASH");
		pr_field(field_by_value(&splash_enable), " -> ");
	}

	if (field_by_value(&splash_progress)->flags & FLG_SET) {
		if (ioctl(fd, WSDISPLAYIO_SPROGRESS, &splash_progress) < 0)
			err(EXIT_FAILURE, "WSDISPLAYIO_SPROGRESS");
		pr_field(field_by_value(&splash_progress), " -> ");
	}

	if (field_by_value(&msg_default_attrs)->flags & FLG_SET ||
	    field_by_value(&msg_default_bg)->flags & FLG_SET ||
	    field_by_value(&msg_default_fg)->flags & FLG_SET ||
	    field_by_value(&msg_kernel_attrs)->flags & FLG_SET ||
	    field_by_value(&msg_kernel_bg)->flags & FLG_SET ||
	    field_by_value(&msg_kernel_fg)->flags & FLG_SET) {
		struct wsdisplay_msgattrs ma;

		if (ioctl(fd, WSDISPLAYIO_GMSGATTRS, &ma) < 0)
			err(EXIT_FAILURE, "WSDISPLAYIO_GMSGATTRS");

		if (field_by_value(&msg_default_attrs)->flags & FLG_SET) {
			ma.default_attrs = msg_default_attrs;
			pr_field(field_by_value(&msg_default_attrs), " -> ");
		}
		if (ma.default_attrs & WSATTR_WSCOLORS) {
			if (field_by_value(&msg_default_bg)->flags & FLG_SET) {
				ma.default_bg = msg_default_bg;
				pr_field(field_by_value(&msg_default_bg),
				    " -> ");
			}
			if (field_by_value(&msg_default_fg)->flags & FLG_SET) {
				ma.default_fg = msg_default_fg;
				pr_field(field_by_value(&msg_default_fg),
				    " -> ");
			}
		}

		if (field_by_value(&msg_kernel_attrs)->flags & FLG_SET) {
			ma.kernel_attrs = msg_kernel_attrs;
			pr_field(field_by_value(&msg_kernel_attrs), " -> ");
		}
		if (ma.default_attrs & WSATTR_WSCOLORS) {
			if (field_by_value(&msg_kernel_bg)->flags & FLG_SET) {
				ma.kernel_bg = msg_kernel_bg;
				pr_field(field_by_value(&msg_kernel_bg),
				    " -> ");
			}
			if (field_by_value(&msg_kernel_fg)->flags & FLG_SET) {
				ma.kernel_fg = msg_kernel_fg;
				pr_field(field_by_value(&msg_kernel_fg),
				    " -> ");
			}
		}

		if (ioctl(fd, WSDISPLAYIO_SMSGATTRS, &ma) < 0)
			err(EXIT_FAILURE, "WSDISPLAYIO_SMSGATTRS");
	}

	scroll_l.which = 0;
	if (field_by_value(&scroll_l.fastlines)->flags & FLG_SET)
		scroll_l.which |= WSDISPLAY_SCROLL_DOFASTLINES;
	if (field_by_value(&scroll_l.slowlines)->flags & FLG_SET)
		scroll_l.which |= WSDISPLAY_SCROLL_DOSLOWLINES;
	if (scroll_l.which != 0 &&
	    ioctl(fd, WSDISPLAYIO_DSSCROLL, &scroll_l) < 0)
		err(EXIT_FAILURE, "WSDISPLAYIO_DSSCROLL");
	if (scroll_l.which & WSDISPLAY_SCROLL_DOFASTLINES)
		pr_field(field_by_value(&scroll_l.fastlines), " -> ");
	if (scroll_l.which & WSDISPLAY_SCROLL_DOSLOWLINES)
		pr_field(field_by_value(&scroll_l.slowlines), " -> ");
}
