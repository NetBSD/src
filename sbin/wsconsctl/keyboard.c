/*	$NetBSD: keyboard.c,v 1.12 2021/09/22 14:15:29 mlelstv Exp $ */

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

#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsconsio.h>

#include <err.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "wsconsctl.h"

static int kbtype;
static int keyclick;
static struct wskbd_bell_data bell;
static struct wskbd_bell_data dfbell;
static struct wscons_keymap mapdata[KS_NUMKEYCODES];
struct wskbd_map_data kbmap = /* used in map_parse.y and in util.c */
    { KS_NUMKEYCODES, mapdata };
static struct wscons_keymap oldmapdata[KS_NUMKEYCODES];
static struct wskbd_map_data oldkbmap =
    { KS_NUMKEYCODES, oldmapdata };
static struct wskbd_keyrepeat_data repeat;
static struct wskbd_keyrepeat_data dfrepeat;
static struct wskbd_scroll_data scroll;
static int ledstate;
static kbd_t kbdencoding;
static int havescroll = 1;
static int kbmode;

struct field keyboard_field_tab[] = {
    { "type",			&kbtype,	FMT_KBDTYPE,	FLG_RDONLY },
    { "mode",			&kbmode,	FMT_UINT,	FLG_MODIFY },
    { "bell.pitch",		&bell.pitch,	FMT_UINT,	FLG_MODIFY },
    { "bell.period",		&bell.period,	FMT_UINT,	FLG_MODIFY },
    { "bell.volume",		&bell.volume,	FMT_UINT,	FLG_MODIFY },
    { "bell.pitch.default",	&dfbell.pitch,	FMT_UINT,	FLG_MODIFY },
    { "bell.period.default",	&dfbell.period,	FMT_UINT,	FLG_MODIFY },
    { "bell.volume.default",	&dfbell.volume,	FMT_UINT,	FLG_MODIFY },
    { "map",			&kbmap,		FMT_KBMAP,	FLG_MODIFY |
    								FLG_NOAUTO },
    { "repeat.del1",		&repeat.del1,	FMT_UINT,	FLG_MODIFY },
    { "repeat.deln",		&repeat.delN,	FMT_UINT,	FLG_MODIFY },
    { "repeat.del1.default",	&dfrepeat.del1,	FMT_UINT,	FLG_MODIFY },
    { "repeat.deln.default",	&dfrepeat.delN,	FMT_UINT,	FLG_MODIFY },
    { "ledstate",		&ledstate,	FMT_UINT,	0 },
    { "encoding",		&kbdencoding,	FMT_KBDENC,	FLG_MODIFY },
    { "keyclick",		&keyclick,	FMT_UINT,	FLG_MODIFY },
    { "scroll.mode",		&scroll.mode,	FMT_UINT,	FLG_MODIFY },
    { "scroll.modifier",	&scroll.modifier, FMT_UINT,	FLG_MODIFY },
};

int keyboard_field_tab_len = sizeof(keyboard_field_tab) /
	sizeof(keyboard_field_tab[0]);

static void
diff_kmap(struct wskbd_map_data *omap, struct wskbd_map_data *nmap)
{
	unsigned int u;
	struct wscons_keymap *op, *np;

	for (u = 0; u < nmap->maplen; u++) {
		op = omap->map + u;
		np = nmap->map + u;
		if (op->command == np->command &&
		    op->group1[0] == np->group1[0] &&
		    op->group1[1] == np->group1[1] &&
		    op->group2[0] == np->group2[0] &&
		    op->group2[1] == np->group2[1]) {
			np->command = KS_voidSymbol;
			np->group1[0] = KS_voidSymbol;
			np->group1[1] = KS_voidSymbol;
			np->group2[0] = KS_voidSymbol;
			np->group2[1] = KS_voidSymbol;
		}
	}
}

void
keyboard_get_values(int fd)
{

	if (field_by_value(&kbtype)->flags & FLG_GET)
		if (ioctl(fd, WSKBDIO_GTYPE, &kbtype) < 0)
			err(EXIT_FAILURE, "WSKBDIO_GTYPE");

	if (field_by_value(&kbmode)->flags & FLG_GET) {
		ioctl(fd, WSKBDIO_GETMODE, &kbmode);
		/* Optional; don't complain. */
	}

	bell.which = 0;
	if (field_by_value(&bell.pitch)->flags & FLG_GET)
		bell.which |= WSKBD_BELL_DOPITCH;
	if (field_by_value(&bell.period)->flags & FLG_GET)
		bell.which |= WSKBD_BELL_DOPERIOD;
	if (field_by_value(&bell.volume)->flags & FLG_GET)
		bell.which |= WSKBD_BELL_DOVOLUME;
	if (bell.which != 0 && ioctl(fd, WSKBDIO_GETBELL, &bell) < 0)
		err(EXIT_FAILURE, "WSKBDIO_GETBELL");

	dfbell.which = 0;
	if (field_by_value(&dfbell.pitch)->flags & FLG_GET)
		dfbell.which |= WSKBD_BELL_DOPITCH;
	if (field_by_value(&dfbell.period)->flags & FLG_GET)
		dfbell.which |= WSKBD_BELL_DOPERIOD;
	if (field_by_value(&dfbell.volume)->flags & FLG_GET)
		dfbell.which |= WSKBD_BELL_DOVOLUME;
	if (dfbell.which != 0 &&
	    ioctl(fd, WSKBDIO_GETDEFAULTBELL, &dfbell) < 0)
		err(EXIT_FAILURE, "WSKBDIO_GETDEFAULTBELL");

	if (field_by_value(&kbmap)->flags & FLG_GET) {
		kbmap.maplen = KS_NUMKEYCODES;
		if (ioctl(fd, WSKBDIO_GETMAP, &kbmap) < 0)
			err(EXIT_FAILURE, "WSKBDIO_GETMAP");
		memcpy(oldmapdata, mapdata, sizeof(oldmapdata));
	}

	repeat.which = 0;
	if (field_by_value(&repeat.del1)->flags & FLG_GET)
		repeat.which |= WSKBD_KEYREPEAT_DODEL1;
	if (field_by_value(&repeat.delN)->flags & FLG_GET)
		repeat.which |= WSKBD_KEYREPEAT_DODELN;
	if (repeat.which != 0 &&
	    ioctl(fd, WSKBDIO_GETKEYREPEAT, &repeat) < 0)
		err(EXIT_FAILURE, "WSKBDIO_GETKEYREPEAT");

	dfrepeat.which = 0;
	if (field_by_value(&dfrepeat.del1)->flags & FLG_GET)
		dfrepeat.which |= WSKBD_KEYREPEAT_DODEL1;
	if (field_by_value(&dfrepeat.delN)->flags & FLG_GET)
		dfrepeat.which |= WSKBD_KEYREPEAT_DODELN;
	if (dfrepeat.which != 0 &&
	    ioctl(fd, WSKBDIO_GETKEYREPEAT, &dfrepeat) < 0)
		err(EXIT_FAILURE, "WSKBDIO_GETKEYREPEAT");

	if (field_by_value(&ledstate)->flags & FLG_GET)
		if (ioctl(fd, WSKBDIO_GETLEDS, &ledstate) < 0)
			err(EXIT_FAILURE, "WSKBDIO_GETLEDS");

	if (field_by_value(&kbdencoding)->flags & FLG_GET)
		if (ioctl(fd, WSKBDIO_GETENCODING, &kbdencoding) < 0)
			err(EXIT_FAILURE, "WSKBDIO_GETENCODING");

	if (field_by_value(&keyclick)->flags & FLG_GET) {
		ioctl(fd, WSKBDIO_GETKEYCLICK, &keyclick);
		/* Optional; don't complain. */
	}
	
	scroll.which = 0;
	if (field_by_value(&scroll.mode)->flags & FLG_GET)
		scroll.which |= WSKBD_SCROLL_DOMODE;
	if (field_by_value(&scroll.modifier)->flags & FLG_GET)
		scroll.which |= WSKBD_SCROLL_DOMODIFIER;
	if (scroll.which != 0) {
		if (ioctl(fd, WSKBDIO_GETSCROLL, &scroll) == -1) {
			if (errno != ENODEV)
				err(EXIT_FAILURE, "WSKBDIO_GETSCROLL");
			else
				havescroll = 0;
		}
	}
}

void
keyboard_put_values(int fd)
{

	if (field_by_value(&kbmode)->flags & FLG_SET) {
		if (ioctl(fd, WSKBDIO_SETMODE, &kbmode) < 0)
			err(EXIT_FAILURE, "WSKBDIO_SETMODE");
		pr_field(field_by_value(&kbmode), " -> ");
	}

	bell.which = 0;
	if (field_by_value(&bell.pitch)->flags & FLG_SET)
		bell.which |= WSKBD_BELL_DOPITCH;
	if (field_by_value(&bell.period)->flags & FLG_SET)
		bell.which |= WSKBD_BELL_DOPERIOD;
	if (field_by_value(&bell.volume)->flags & FLG_SET)
		bell.which |= WSKBD_BELL_DOVOLUME;
	if (bell.which != 0 && ioctl(fd, WSKBDIO_SETBELL, &bell) < 0)
		err(EXIT_FAILURE, "WSKBDIO_SETBELL");
	if (bell.which & WSKBD_BELL_DOPITCH)
		pr_field(field_by_value(&bell.pitch), " -> ");
	if (bell.which & WSKBD_BELL_DOPERIOD)
		pr_field(field_by_value(&bell.period), " -> ");
	if (bell.which & WSKBD_BELL_DOVOLUME)
		pr_field(field_by_value(&bell.volume), " -> ");

	dfbell.which = 0;
	if (field_by_value(&dfbell.pitch)->flags & FLG_SET)
		dfbell.which |= WSKBD_BELL_DOPITCH;
	if (field_by_value(&dfbell.period)->flags & FLG_SET)
		dfbell.which |= WSKBD_BELL_DOPERIOD;
	if (field_by_value(&dfbell.volume)->flags & FLG_SET)
		dfbell.which |= WSKBD_BELL_DOVOLUME;
	if (dfbell.which != 0 &&
	    ioctl(fd, WSKBDIO_SETDEFAULTBELL, &dfbell) < 0)
		err(EXIT_FAILURE, "WSKBDIO_SETDEFAULTBELL");
	if (dfbell.which & WSKBD_BELL_DOPITCH)
		pr_field(field_by_value(&dfbell.pitch), " -> ");
	if (dfbell.which & WSKBD_BELL_DOPERIOD)
		pr_field(field_by_value(&dfbell.period), " -> ");
	if (dfbell.which & WSKBD_BELL_DOVOLUME)
		pr_field(field_by_value(&dfbell.volume), " -> ");

	if (field_by_value(&kbmap)->flags & FLG_SET) {
		if (ioctl(fd, WSKBDIO_SETMAP, &kbmap) < 0)
			err(EXIT_FAILURE, "WSKBDIO_SETMAP");
		if (field_by_value(&kbmap)->flags & FLG_MODIFIED) {
			diff_kmap(&oldkbmap, &kbmap);
			pr_field(field_by_value(&kbmap), " +> ");
		} else
			pr_field(field_by_value(&kbmap), " -> ");
	}

	repeat.which = 0;
	if (field_by_value(&repeat.del1)->flags & FLG_SET)
		repeat.which |= WSKBD_KEYREPEAT_DODEL1;
	if (field_by_value(&repeat.delN)->flags & FLG_SET)
		repeat.which |= WSKBD_KEYREPEAT_DODELN;
	if (repeat.which != 0 &&
	    ioctl(fd, WSKBDIO_SETKEYREPEAT, &repeat) < 0)
		err(EXIT_FAILURE, "WSKBDIO_SETKEYREPEAT");
	if (repeat.which & WSKBD_KEYREPEAT_DODEL1)
		pr_field(field_by_value(&repeat.del1), " -> ");
	if (repeat.which & WSKBD_KEYREPEAT_DODELN)
		pr_field(field_by_value(&repeat.delN), " -> ");

	dfrepeat.which = 0;
	if (field_by_value(&dfrepeat.del1)->flags & FLG_SET)
		dfrepeat.which |= WSKBD_KEYREPEAT_DODEL1;
	if (field_by_value(&dfrepeat.delN)->flags & FLG_SET)
		dfrepeat.which |= WSKBD_KEYREPEAT_DODELN;
	if (dfrepeat.which != 0 &&
	    ioctl(fd, WSKBDIO_SETDEFAULTKEYREPEAT, &dfrepeat) < 0)
		err(EXIT_FAILURE, "WSKBDIO_SETDEFAULTKEYREPEAT");
	if (dfrepeat.which &WSKBD_KEYREPEAT_DODEL1)
		pr_field(field_by_value(&dfrepeat.del1), " -> ");
	if (dfrepeat.which & WSKBD_KEYREPEAT_DODELN)
		pr_field(field_by_value(&dfrepeat.delN), " -> ");

	if (field_by_value(&ledstate)->flags & FLG_SET) {
		if (ioctl(fd, WSKBDIO_SETLEDS, &ledstate) < 0)
			err(EXIT_FAILURE, "WSKBDIO_SETLEDS");
		pr_field(field_by_value(&ledstate), " -> ");
	}

	if (field_by_value(&kbdencoding)->flags & FLG_SET) {
		if (ioctl(fd, WSKBDIO_SETENCODING, &kbdencoding) < 0)
			err(EXIT_FAILURE, "WSKBDIO_SETENCODING");
		pr_field(field_by_value(&kbdencoding), " -> ");
	}

	if (field_by_value(&keyclick)->flags & FLG_SET) {
		if (ioctl(fd, WSKBDIO_SETKEYCLICK, &keyclick) < 0)
			err(EXIT_FAILURE, "WSKBDIO_SETKEYCLICK");
		pr_field(field_by_value(&keyclick), " -> ");
	}


	if (havescroll == 0)
		return;

	scroll.which = 0;
	if (field_by_value(&scroll.mode)->flags & FLG_SET)
		scroll.which |= WSKBD_SCROLL_DOMODE;
	if (field_by_value(&scroll.modifier)->flags & FLG_SET)
		scroll.which |= WSKBD_SCROLL_DOMODIFIER;

	if (scroll.which & WSKBD_SCROLL_DOMODE)
		pr_field(field_by_value(&scroll.mode), " -> ");
	if (scroll.which & WSKBD_SCROLL_DOMODIFIER)
		pr_field(field_by_value(&scroll.modifier), " -> ");
	if (scroll.which != 0) {
		if (ioctl(fd, WSKBDIO_SETSCROLL, &scroll) == -1) {
			if (errno != ENODEV)
				err(EXIT_FAILURE, "WSKBDIO_SETSCROLL");
			else {
				warnx("scrolling is not supported by this "
				    "kernel");
				havescroll = 0;
			}
		}
	}
}

