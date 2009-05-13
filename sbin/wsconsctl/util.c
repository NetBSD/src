/*	$NetBSD: util.c,v 1.27.6.1 2009/05/13 19:19:07 jym Exp $ */

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

#include <sys/time.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsksymdef.h>

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "wsconsctl.h"

#define TABLEN(t)		(sizeof(t)/sizeof(t[0]))

extern struct wskbd_map_data kbmap;	/* from keyboard.c */
extern struct wskbd_map_data newkbmap;	/* from map_parse.y */

struct nameint {
	int value;
	const char *name;
};

static struct nameint kbtype_tab[] = {
	{ WSKBD_TYPE_LK201,		"lk201" },
	{ WSKBD_TYPE_LK401,		"lk401" },
	{ WSKBD_TYPE_PC_XT,		"pc-xt" },
	{ WSKBD_TYPE_PC_AT,		"pc-at" },
	{ WSKBD_TYPE_USB,		"usb" },
	{ WSKBD_TYPE_HPC_KBD,		"hpc-kbd" },
	{ WSKBD_TYPE_HPC_BTN,		"hpc-btn" },
	{ WSKBD_TYPE_ARCHIMEDES,	"archimedes" },
	{ WSKBD_TYPE_RISCPC,		"riscpc" },
	{ WSKBD_TYPE_ADB,		"adb" },
	{ WSKBD_TYPE_HIL,		"hil" },
	{ WSKBD_TYPE_AMIGA,		"amiga" },
	{ WSKBD_TYPE_MAPLE,		"maple" },
	{ WSKBD_TYPE_ATARI,		"atari" },
	{ WSKBD_TYPE_SUN,		"sun" },
	{ WSKBD_TYPE_SUN5,		"sun-type5" },
	{ WSKBD_TYPE_SGI,		"sgi" },
	{ WSKBD_TYPE_MATRIXKP,		"matrix-keypad" },
	{ WSKBD_TYPE_BLUETOOTH,		"bluetooth" },
};

static struct nameint mstype_tab[] = {
	{ WSMOUSE_TYPE_VSXXX,		"dec-tc" },
	{ WSMOUSE_TYPE_PS2,		"ps2" },
	{ WSMOUSE_TYPE_USB,		"usb" },
	{ WSMOUSE_TYPE_LMS,		"logitech-bus" },
	{ WSMOUSE_TYPE_MMS,		"ms-inport" },
	{ WSMOUSE_TYPE_TPANEL,		"touch-panel" },
	{ WSMOUSE_TYPE_NEXT,		"next" },
	{ WSMOUSE_TYPE_ARCHIMEDES,	"archimedes" },
	{ WSMOUSE_TYPE_HIL,		"hil" },
	{ WSMOUSE_TYPE_AMIGA,		"amiga" },
	{ WSMOUSE_TYPE_MAXINE,		"dec-maxine" },
	{ WSMOUSE_TYPE_MAPLE,		"maple" },
	{ WSMOUSE_TYPE_BLUETOOTH,	"bluetooth" },
};

static struct nameint dpytype_tab[] = {
	{ WSDISPLAY_TYPE_UNKNOWN,	"unknown" },
	{ WSDISPLAY_TYPE_PM_MONO,	"dec-pm-mono" },
	{ WSDISPLAY_TYPE_PM_COLOR,	"dec-pm-color" },
	{ WSDISPLAY_TYPE_CFB,		"dec-cfb" },
	{ WSDISPLAY_TYPE_XCFB,		"dec-xcfb" },
	{ WSDISPLAY_TYPE_MFB,		"dec-mfb" },
	{ WSDISPLAY_TYPE_SFB,		"dec-sfb" },
	{ WSDISPLAY_TYPE_ISAVGA,	"vga-isa" },
	{ WSDISPLAY_TYPE_PCIVGA,	"vga-pci" },
	{ WSDISPLAY_TYPE_TGA,		"dec-tga-pci" },
	{ WSDISPLAY_TYPE_SFBP,		"dec-sfb+" },
	{ WSDISPLAY_TYPE_PCIMISC,	"generic-pci" },
	{ WSDISPLAY_TYPE_NEXTMONO,	"next-mono" },
	{ WSDISPLAY_TYPE_PX,		"dec-px" },
	{ WSDISPLAY_TYPE_PXG,		"dec-pxg" },
	{ WSDISPLAY_TYPE_TX,		"dec-tx" },
	{ WSDISPLAY_TYPE_HPCFB,		"generic-hpc" },
	{ WSDISPLAY_TYPE_VIDC,		"arm-vidc" },
	{ WSDISPLAY_TYPE_SPX,		"dec-spx" },
	{ WSDISPLAY_TYPE_GPX,		"dec-gpx" },
	{ WSDISPLAY_TYPE_LCG,		"dec-lcg" },
	{ WSDISPLAY_TYPE_VAX_MONO,	"dec-vax-mono" },
	{ WSDISPLAY_TYPE_SB_P9100,	"sparcbook-p9100" },
	{ WSDISPLAY_TYPE_EGA,		"ega" },
	{ WSDISPLAY_TYPE_DCPVR,		"dreamcast-pvr" },
	{ WSDISPLAY_TYPE_GATOR,		"hp-gator" },
	{ WSDISPLAY_TYPE_TOPCAT,	"hp-topcat" },
	{ WSDISPLAY_TYPE_RENAISSANCE,	"hp-renaissance" },
	{ WSDISPLAY_TYPE_CATSEYE,	"hp-catseye" },
	{ WSDISPLAY_TYPE_DAVINCI,	"hp-davinci" },
	{ WSDISPLAY_TYPE_TIGER,		"hp-tiger" },
	{ WSDISPLAY_TYPE_HYPERION,	"hp-hyperion" },
	{ WSDISPLAY_TYPE_AMIGACC,	"amiga-cc" },
	{ WSDISPLAY_TYPE_SUN24,		"sun24" },
	{ WSDISPLAY_TYPE_NEWPORT,	"sgi-newport" },
	{ WSDISPLAY_TYPE_GR2,		"sgi-gr2" },
	{ WSDISPLAY_TYPE_SUNCG12,	"suncg12" },
	{ WSDISPLAY_TYPE_SUNCG14,	"suncg14" },
	{ WSDISPLAY_TYPE_SUNTCX,	"suntcx" },
	{ WSDISPLAY_TYPE_SUNFFB,	"sunffb" },
};

static struct nameint kbdenc_tab[] = {
	KB_ENCTAB
};

static struct nameint kbdvar_tab[] = {
	KB_VARTAB
};

static struct nameint color_tab[] = {
	{ WSCOL_UNSUPPORTED,		"unsupported" },
	{ WSCOL_BLACK,			"black" },
	{ WSCOL_RED,			"red" },
	{ WSCOL_GREEN,			"green" },
	{ WSCOL_BROWN,			"brown" },
	{ WSCOL_BLUE,			"blue" },
	{ WSCOL_MAGENTA,		"magenta" },
	{ WSCOL_CYAN,			"cyan" },
	{ WSCOL_WHITE,			"white" },
};

static struct nameint attr_tab[] = {
	{ WSATTR_NONE,			"none" },
	{ WSATTR_REVERSE,		"reverse" },
	{ WSATTR_HILIT,			"hilit" },
	{ WSATTR_BLINK,			"blink" },
	{ WSATTR_UNDERLINE,		"underline" },
	{ WSATTR_WSCOLORS,		"color" },
};

static struct field *field_tab;
static int field_tab_len;

static const char *int2name(int, int, struct nameint *, int);
static int name2int(char *, struct nameint *, int);
static void print_kmap(struct wskbd_map_data *);
static unsigned int rd_bitfield(const char *);
static void pr_bitfield(unsigned int);

void
field_setup(struct field *ftab, int len)
{

	field_tab = ftab;
	field_tab_len = len;
}

struct field *
field_by_name(char *name)
{
	int i;

	for (i = 0; i < field_tab_len; i++)
		if (strcmp(field_tab[i].name, name) == 0)
			return field_tab + i;

	errx(EXIT_FAILURE, "%s: not found", name);
}

struct field *
field_by_value(void *addr)
{
	int i;

	for (i = 0; i < field_tab_len; i++)
		if (field_tab[i].valp == addr)
			return field_tab + i;

	errx(EXIT_FAILURE, "internal error: field_by_value: not found");
}

void
field_disable_by_value(void *addr)
{
	struct field *f;

	f = field_by_value(addr);
	f->flags |= FLG_DISABLED;
}

static const char *
int2name(int val, int uflag, struct nameint *tab, int len)
{
	static char tmp[20];
	int i;

	for (i = 0; i < len; i++)
		if (tab[i].value == val)
			return tab[i].name;

	if (uflag) {
		(void)snprintf(tmp, sizeof(tmp), "unknown_%d", val);
		return tmp;
	} else
		return NULL;
}

static int
name2int(char *val, struct nameint *tab, int len)
{
	int i;

	for (i = 0; i < len; i++)
		if (strcmp(tab[i].name, val) == 0)
			return tab[i].value;
	return -1;
}

void
pr_field(struct field *f, const char *sep)
{
	const char *p;
	unsigned int flags;
	int first, i, mask;

	if (sep)
		(void)printf("%s%s", f->name, sep);

	switch (f->format) {
	case FMT_UINT:
		(void)printf("%u", *((unsigned int *) f->valp));
		break;
	case FMT_STRING:
		(void)printf("\"%s\"", *((char **) f->valp));
		break;
	case FMT_BITFIELD:
		pr_bitfield(*((unsigned int *) f->valp));
		break;
	case FMT_KBDTYPE:
		p = int2name(*((unsigned int *) f->valp), 1,
		    kbtype_tab, TABLEN(kbtype_tab));
		(void)printf("%s", p);
		break;
	case FMT_MSTYPE:
		p = int2name(*((unsigned int *) f->valp), 1,
		    mstype_tab, TABLEN(mstype_tab));
		(void)printf("%s", p);
		break;
	case FMT_DPYTYPE:
		p = int2name(*((unsigned int *) f->valp), 1,
		    dpytype_tab, TABLEN(dpytype_tab));
		(void)printf("%s", p);
		break;
	case FMT_KBDENC:
		p = int2name(KB_ENCODING(*((unsigned int *) f->valp)), 1,
		    kbdenc_tab, TABLEN(kbdenc_tab));
		(void)printf("%s", p);

		flags = KB_VARIANT(*((unsigned int *) f->valp));
		for (i = 0; i < 32; i++) {
			if (!(flags & (1 << i)))
				continue;
			p = int2name(flags & (1 << i), 1,
			    kbdvar_tab, TABLEN(kbdvar_tab));
			(void)printf(".%s", p);
		}
		break;
	case FMT_KBMAP:
		print_kmap((struct wskbd_map_data *) f->valp);
		break;
	case FMT_COLOR:
		p = int2name(*((unsigned int *) f->valp), 1,
		    color_tab, TABLEN(color_tab));
		(void)printf("%s", p);
		break;
	case FMT_ATTRS:
		mask = 0x10;
		first = 1;
		while (mask > 0) {
			if (*((unsigned int *) f->valp) & mask) {
				p = int2name(*((unsigned int *) f->valp) & mask,
				    1, attr_tab, TABLEN(attr_tab));
				(void)printf("%s%s", first ? "" : ",", p);
				first = 0;
			}
			mask >>= 1;
		}
		if (first)
			(void)printf("none");
		break;
	default:
		errx(EXIT_FAILURE, "internal error: pr_field: no format %d",
		    f->format);
		break;
	}

	(void)printf("\n");
}

static void
pr_bitfield(unsigned int f)
{

	if (f == 0)
		(void)printf("none");
	else {
		unsigned int i;
		int first, mask;

		for (i = 0, first = 1, mask = 1; i < sizeof(f) * 8; i++) {
			if (f & mask) {
				(void)printf("%s%u", first ? "" : " ", i);
				first = 0;
			}
			mask = mask << 1;
		}
	}
}

void
rd_field(struct field *f, char *val, int merge)
{
	int i;
	unsigned int u;
	char *p;
	struct wscons_keymap *mp;

	switch (f->format) {
	case FMT_UINT:
		if (sscanf(val, "%u", &u) != 1)
			errx(EXIT_FAILURE, "%s: not a number", val);
		if (merge)
			*((unsigned int *) f->valp) += u;
		else
			*((unsigned int *) f->valp) = u;
		break;
	case FMT_STRING:
		if ((*((char **) f->valp) = strdup(val)) == NULL)
			err(EXIT_FAILURE, "strdup");
		break;
	case FMT_BITFIELD:
		*((unsigned int *) f->valp) = rd_bitfield(val);
		break;
	case FMT_KBDENC:
		p = strchr(val, '.');
		if (p != NULL)
			*p++ = '\0';

		i = name2int(val, kbdenc_tab, TABLEN(kbdenc_tab));
		if (i == -1)
			errx(EXIT_FAILURE, "%s: not a valid encoding", val);
		*((unsigned int *) f->valp) = i;

		while (p) {
			val = p;
			p = strchr(p, '.');
			if (p != NULL)
				*p++ = '\0';
			i = name2int(val, kbdvar_tab, TABLEN(kbdvar_tab));
			if (i == -1)
				errx(EXIT_FAILURE, "%s: not a valid variant",
				    val);
			*((unsigned int *) f->valp) |= i;
		}
		break;
	case FMT_KBMAP:
		if (! merge)
			kbmap.maplen = 0;
		map_scan_setinput(val);
		yyparse();
		if (merge) {
			if (newkbmap.maplen < kbmap.maplen)
				newkbmap.maplen = kbmap.maplen;
			for (u = 0; u < kbmap.maplen; u++) {
				mp = newkbmap.map + u;
				if (mp->command == KS_voidSymbol &&
				    mp->group1[0] == KS_voidSymbol &&
				    mp->group1[1] == KS_voidSymbol &&
				    mp->group2[0] == KS_voidSymbol &&
				    mp->group2[1] == KS_voidSymbol)
					*mp = kbmap.map[u];
			}
		}
		kbmap.maplen = newkbmap.maplen;
		bcopy(newkbmap.map, kbmap.map,
		    kbmap.maplen * sizeof(struct wscons_keymap));
		break;
	case FMT_COLOR:
		i = name2int(val, color_tab, TABLEN(color_tab));
		if (i == -1)
			errx(EXIT_FAILURE, "%s: not a valid color", val);
		*((unsigned int *) f->valp) = i;
		break;
	case FMT_ATTRS:
		p = val;
		while (p) {
			val = p;
			p = strchr(p, ',');
			if (p != NULL)
				*p++ = '\0';
			i = name2int(val, attr_tab, TABLEN(attr_tab));
			if (i == -1)
				errx(EXIT_FAILURE, "%s: not a valid attribute",
				    val);
			*((unsigned int *) f->valp) |= i;
		}
		break;
	default:
		errx(EXIT_FAILURE, "internal error: rd_field: no format %d",
		    f->format);
		break;
	}
}

static unsigned int
rd_bitfield(const char *str)
{
	const char *ptr;
	char *ep;
	long lval;
	unsigned int result;

	ep = NULL;
	ptr = str;
	result = 0;
	while (*ptr != '\0') {
		errno = 0;
		lval = strtol(ptr, &ep, 10);
		if (*ep != '\0' && *ep != ' ')
			errx(EXIT_FAILURE, "%s: not a valid number list", str);
		if (errno == ERANGE && (lval == LONG_MAX || lval == LONG_MIN))
			errx(EXIT_FAILURE, "%s: not a valid number list", str);
		if (lval >= (long)sizeof(result) * 8)
			errx(EXIT_FAILURE, "%ld: number out of range", lval);
		result |= (1 << lval);

		ptr = ep;
		while (*ptr == ' ')
			ptr++;
	}

	return result;
}

static void
print_kmap(struct wskbd_map_data *map)
{
	unsigned int i;
	struct wscons_keymap *mp;

	for (i = 0; i < map->maplen; i++) {
		mp = map->map + i;

		if (mp->command == KS_voidSymbol &&
		    mp->group1[0] == KS_voidSymbol &&
		    mp->group1[1] == KS_voidSymbol &&
		    mp->group2[0] == KS_voidSymbol &&
		    mp->group2[1] == KS_voidSymbol)
			continue;
		(void)printf("\n");
		(void)printf("keycode %u =", i);
		if (mp->command != KS_voidSymbol)
			(void)printf(" %s", ksym2name(mp->command));
		(void)printf(" %s", ksym2name(mp->group1[0]));
		if (mp->group1[0] != mp->group1[1] ||
		    mp->group1[0] != mp->group2[0] ||
		    mp->group1[0] != mp->group2[1]) {
			(void)printf(" %s", ksym2name(mp->group1[1]));
			if (mp->group1[0] != mp->group2[0] ||
			    mp->group1[1] != mp->group2[1]) {
				(void)printf(" %s", ksym2name(mp->group2[0]));
				(void)printf(" %s", ksym2name(mp->group2[1]));
			}
		}
	}
}
