/*	$NetBSD: display.c,v 1.4 2004/06/03 19:18:41 christos Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <err.h>

#include <dev/wscons/wsconsio.h>

#include "wsconsctl.h"

static int dpytype;
static struct wsdisplay_usefontdata font;
static struct wsdisplay_scroll_data scroll_l;
static int havescroll = 1;

struct field display_field_tab[] = {
    { "type",			&dpytype,	FMT_DPYTYPE,	FLG_RDONLY },
    { "font",			&font.name,	FMT_STRING,	FLG_WRONLY },
    { "scroll.fastlines",	&scroll_l.fastlines, FMT_UINT, FLG_MODIFY },
    { "scroll.slowlines",	&scroll_l.slowlines, FMT_UINT, FLG_MODIFY },
};

int display_field_tab_len = sizeof(display_field_tab)/
			     sizeof(display_field_tab[0]);

static int
init_values(void)
{
	scroll_l.which = 0;

	if (field_by_value(&scroll_l.fastlines)->flags & FLG_GET)
		scroll_l.which |= WSDISPLAY_SCROLL_DOFASTLINES;
	if (field_by_value(&scroll_l.slowlines)->flags & FLG_GET)
		scroll_l.which |= WSDISPLAY_SCROLL_DOSLOWLINES;

	return scroll_l.which;

}
void
display_get_values(fd)
	int fd;
{
	if (field_by_value(&dpytype)->flags & FLG_GET)
		if (ioctl(fd, WSDISPLAYIO_GTYPE, &dpytype) < 0)
			err(1, "WSDISPLAYIO_GTYPE");
	
	if (init_values() == 0 || havescroll == 0)
		return;

	if (ioctl(fd, WSDISPLAYIO_DGSCROLL, &scroll_l) < 0) {
		if (errno != ENODEV)
			err(1, "WSDISPLAYIO_GSCROLL");
		else
			havescroll = 0;
	}
}

void
display_put_values(fd)
	int fd;
{
	if (field_by_value(&font.name)->flags & FLG_SET) {
		if (ioctl(fd, WSDISPLAYIO_SFONT, &font) < 0)
			err(1, "WSDISPLAYIO_SFONT");
		pr_field(field_by_value(&font.name), " -> ");
	}
	
	if (init_values() == 0 || havescroll == 0)
		return;

	if (scroll_l.which & WSDISPLAY_SCROLL_DOFASTLINES)
		pr_field(field_by_value(&scroll_l.fastlines), " -> ");
	if (scroll_l.which & WSDISPLAY_SCROLL_DOSLOWLINES)
		pr_field(field_by_value(&scroll_l.slowlines), " -> ");

	if (ioctl(fd, WSDISPLAYIO_DSSCROLL, &scroll_l) < 0) {
		if (errno != ENODEV)
			err (1, "WSDISPLAYIO_DSSCROLL");
		else {
			warnx("scrolling is not supported by this kernel");
			havescroll = 0;
		}
	}
}
