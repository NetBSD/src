/*	$NetBSD: mouse.c,v 1.7.22.1 2008/05/18 12:30:55 yamt Exp $ */

/*-
 * Copyright (c) 1998, 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Juergen Hannken-Illjes and Julio M. Merino Vidal.
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
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wsconsctl.h"

static int mstype;
static int resolution;
static int samplerate;
static struct wsmouse_repeat repeat;

static void mouse_get_repeat(int);
static void mouse_put_repeat(int);

struct field mouse_field_tab[] = {
    { "resolution",		&resolution,	FMT_UINT,	FLG_WRONLY },
    { "samplerate",		&samplerate,	FMT_UINT,	FLG_WRONLY },
    { "type",			&mstype,	FMT_MSTYPE,	FLG_RDONLY },
    { "repeat.buttons",		&repeat.wr_buttons,
    						FMT_BITFIELD, FLG_MODIFY },
    { "repeat.delay.first",	&repeat.wr_delay_first,
    						FMT_UINT, FLG_MODIFY },
    { "repeat.delay.decrement",	&repeat.wr_delay_decrement,
    						FMT_UINT, FLG_MODIFY },
    { "repeat.delay.minimum",	&repeat.wr_delay_minimum,
 		   				FMT_UINT, FLG_MODIFY },
};

int mouse_field_tab_len = sizeof(mouse_field_tab)/
			   sizeof(mouse_field_tab[0]);

void
mouse_get_values(int fd)
{

	if (field_by_value(&mstype)->flags & FLG_GET)
		if (ioctl(fd, WSMOUSEIO_GTYPE, &mstype) < 0)
			err(EXIT_FAILURE, "WSMOUSEIO_GTYPE");

	if (field_by_value(&repeat.wr_buttons)->flags & FLG_GET ||
	    field_by_value(&repeat.wr_delay_first)->flags & FLG_GET ||
	    field_by_value(&repeat.wr_delay_decrement)->flags & FLG_GET ||
	    field_by_value(&repeat.wr_delay_minimum)->flags & FLG_GET)
		mouse_get_repeat(fd);
}

static void
mouse_get_repeat(int fd)
{
	struct wsmouse_repeat tmp;

	if (ioctl(fd, WSMOUSEIO_GETREPEAT, &tmp) == -1)
		err(EXIT_FAILURE, "WSMOUSEIO_GETREPEAT");

	if (field_by_value(&repeat.wr_buttons)->flags & FLG_GET)
		repeat.wr_buttons = tmp.wr_buttons;
	if (field_by_value(&repeat.wr_delay_first)->flags & FLG_GET)
		repeat.wr_delay_first = tmp.wr_delay_first;
	if (field_by_value(&repeat.wr_delay_decrement)->flags & FLG_GET)
		repeat.wr_delay_decrement = tmp.wr_delay_decrement;
	if (field_by_value(&repeat.wr_delay_minimum)->flags & FLG_GET)
		repeat.wr_delay_minimum = tmp.wr_delay_minimum;
}

void
mouse_put_values(int fd)
{
	int tmp;

	if (field_by_value(&resolution)->flags & FLG_SET) {
		tmp = resolution;
		if (ioctl(fd, WSMOUSEIO_SRES, &tmp) < 0)
			err(EXIT_FAILURE, "WSMOUSEIO_SRES");
		pr_field(field_by_value(&resolution), " -> ");
	}

	if (field_by_value(&samplerate)->flags & FLG_SET) {
		tmp = samplerate;
		if (ioctl(fd, WSMOUSEIO_SRATE, &tmp) < 0)
			err(EXIT_FAILURE, "WSMOUSEIO_SRATE");
		pr_field(field_by_value(&samplerate), " -> ");
	}

	if (field_by_value(&repeat.wr_buttons)->flags & FLG_SET ||
	    field_by_value(&repeat.wr_delay_first)->flags & FLG_SET ||
	    field_by_value(&repeat.wr_delay_decrement)->flags & FLG_SET ||
	    field_by_value(&repeat.wr_delay_minimum)->flags & FLG_SET)
		mouse_put_repeat(fd);
}

static void
mouse_put_repeat(int fd)
{
	struct wsmouse_repeat tmp;

	/* Fetch current values into the temporary structure. */
	if (ioctl(fd, WSMOUSEIO_GETREPEAT, &tmp) == -1)
		err(EXIT_FAILURE, "WSMOUSEIO_GETREPEAT");

	/* Overwrite the desired values in the temporary structure. */
	if (field_by_value(&repeat.wr_buttons)->flags & FLG_SET)
		tmp.wr_buttons = repeat.wr_buttons;
	if (field_by_value(&repeat.wr_delay_first)->flags & FLG_SET)
		tmp.wr_delay_first = repeat.wr_delay_first;
	if (field_by_value(&repeat.wr_delay_decrement)->flags & FLG_SET)
		tmp.wr_delay_decrement = repeat.wr_delay_decrement;
	if (field_by_value(&repeat.wr_delay_minimum)->flags & FLG_SET)
		tmp.wr_delay_minimum = repeat.wr_delay_minimum;

	/* Set new values for repeating events. */
	if (ioctl(fd, WSMOUSEIO_SETREPEAT, &tmp) == -1)
		err(EXIT_FAILURE, "WSMOUSEIO_SETREPEAT");

	/* Now print what changed. */
	if (field_by_value(&repeat.wr_buttons)->flags & FLG_SET)
		pr_field(field_by_value(&repeat.wr_buttons), " -> ");
	if (field_by_value(&repeat.wr_delay_first)->flags & FLG_SET)
		pr_field(field_by_value(&repeat.wr_delay_first), " -> ");
	if (field_by_value(&repeat.wr_delay_decrement)->flags & FLG_SET)
		pr_field(field_by_value(&repeat.wr_delay_decrement), " -> ");
	if (field_by_value(&repeat.wr_delay_minimum)->flags & FLG_SET)
		pr_field(field_by_value(&repeat.wr_delay_minimum), " -> ");
}
