/*	$NetBSD: methods.c,v 1.1.1.1 1999/06/21 15:56:03 minoura Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Minoura Makoto.
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

#include <stdio.h>
#include <string.h>
#include <err.h>
#include <sys/types.h>

#include "memswitch.h"


int
atoi_ (p)
	char **p;
{
	char *p1 = *p;
	int v = 0;
	int first = 1;

	while (*p1 == ' ' || *p1 == '\t')
		p1++;

	if (*p1 == 0) {
		*p = 0;
		return 0;
	}
	if (strlen (p1) >= 2 && strncasecmp ("0x", p1, 2) == 0) {
		p1 += 2;
		while (1) {
			if (*p1 >= '0' && *p1 <= '9') {
				v *= 16;
				v += *p1 - '0';
				first = 0;
			} else if (*p1 >= 'A' && *p1 <= 'F') {
				v *= 16;
				v += *p1 - 'A' + 10;
				first = 0;
			} else if (*p1 >= 'a' && *p1 <= 'f') {
				v *= 16;
				v += *p1 - 'a' + 10;
				first = 0;
			} else {
				break;
			}
			p1++;
		}
	} else {
		while (1) {
			if (*p1 >= '0' && *p1 <= '9') {
				v *= 10;
				v += *p1 - '0';
				first = 0;
			} else {
				break;
			}
			p1++;
		}
	}

	if (first) {
		*p = 0;
		return 0;
	}

	while (*p1 == ' ' || *p1 == '\t');
	*p = p1;
	return v;
}

int
fill_uchar (prop)
	struct property *prop;
{
	if (current_values == 0)
		alloc_current_values ();

	prop->current_value.byte[0] = current_values[prop->offset];
	prop->current_value.byte[1] = 0;
	prop->current_value.byte[2] = 0;
	prop->current_value.byte[3] = 0;
	prop->value_valid = 1;

	return 0;
}

int
fill_ushort (prop)
	struct property *prop;
{
	if (current_values == 0)
		alloc_current_values ();

	prop->current_value.byte[0] = current_values[prop->offset];
	prop->current_value.byte[1] = current_values[prop->offset+1];
	prop->current_value.byte[2] = 0;
	prop->current_value.byte[3] = 0;
	prop->value_valid = 1;

	return 0;
}

int
fill_ulong (prop)
	struct property *prop;
{
	if (current_values == 0)
		alloc_current_values ();

	prop->current_value.byte[0] = current_values[prop->offset];
	prop->current_value.byte[1] = current_values[prop->offset+1];
	prop->current_value.byte[2] = current_values[prop->offset+2];
	prop->current_value.byte[3] = current_values[prop->offset+3];
	prop->value_valid = 1;

	return 0;
}

int
flush_uchar (prop)
	struct property *prop;
{
	if (!prop->modified)
		return 0;

	if (modified_values == 0)
		alloc_modified_values ();

	modified_values[prop->offset] = prop->modified_value.byte[0];

	return 0;
}

int
flush_ushort (prop)
	struct property *prop;
{
	if (!prop->modified)
		return 0;

	if (modified_values == 0)
		alloc_modified_values ();

	modified_values[prop->offset] = prop->modified_value.byte[0];
	modified_values[prop->offset+1] = prop->modified_value.byte[1];

	return 0;
}

int
flush_ulong (prop)
	struct property *prop;
{
	if (!prop->modified)
		return 0;

	if (modified_values == 0)
		alloc_modified_values ();

	modified_values[prop->offset] = prop->modified_value.byte[0];
	modified_values[prop->offset+1] = prop->modified_value.byte[1];
	modified_values[prop->offset+2] = prop->modified_value.byte[2];
	modified_values[prop->offset+3] = prop->modified_value.byte[3];

	return 0;
}

int
flush_dummy (prop)
	struct property *prop;
{
	return 0;
}

int
parse_dummy (prop, value)
	struct property *prop;
	const char *value;
{
	warnx ("Cannot modify %s.%s", prop->class, prop->node);

	return -1;
}

int
parse_byte (prop, value)
	struct property *prop;
	const char *value;
{
	const char *p = value;
	int v;

	v = atoi_ (&p);
	if (p == 0) {
		warnx ("%s: Invalid value", value);
		return -1;
	}

	if (strcasecmp ("MB", p) == 0)
		v *= 1024 * 1024;
	else if (strcasecmp ("KB", p) == 0)
		v *= 1024;
	else if (*p != 0 &&
		 strcasecmp ("B", p) != 0) {
		warnx ("%s: Invalid value", value);
		return -1;
	}

	if (v < prop->min) {
		warnx ("%s: Too small", value);
		return -1;
	} else if (v > prop->max) {
		warnx ("%s: Too large", value);
		return -1;
	}

	prop->modified = 1;
	prop->modified_value.longword = v;

	return 0;
}

int
parse_uchar (prop, value)
	struct property *prop;
	const char *value;
{
	const char *p = value;
	int v;

	v = atoi_ (&p);
	if (p == 0) {
		warnx ("%s: Invalid value", value);
		return -1;
	}

	if (v < prop->min) {
		warnx ("%s: Too small", value);
		return -1;
	} else if (v > prop->max) {
		warnx ("%s: Too large", value);
		return -1;
	}

	prop->modified = 1;
	prop->modified_value.byte[0] = v;

	return 0;
}

int
parse_ulong (prop, value)
	struct property *prop;
	const char *value;
{
	const char *p = value;
	int v;

	v = atoi_ (&p);
	if (p == 0) {
		warnx ("%s: Invalid value", value);
		return -1;
	}

	if (v < prop->min) {
		warnx ("%s: Too small", value);
		return -1;
	} else if (v > prop->max) {
		warnx ("%s: Too large", value);
		return -1;
	}

	prop->modified = 1;
	prop->modified_value.longword = v;

	return 0;
}

int
parse_ushort (prop, value)
	struct property *prop;
	const char *value;
{
	const char *p = value;
	int v;

	v = atoi_ (&p);
	if (p == 0) {
		warnx ("%s: Invalid value", value);
		return -1;
	}

	if (v < prop->min) {
		warnx ("%s: Too small", value);
		return -1;
	} else if (v > prop->max) {
		warnx ("%s: Too large", value);
		return -1;
	}

	prop->modified = 1;
	prop->modified_value.word[0] = v;

	return 0;
}

int
parse_time (prop, value)
	struct property *prop;
	const char *value;
{
	const char *p = value;
	int v;

	while (*p == ' ' || *p == '\t');
	if (*p == '-') {
		p++;
		v = -atoi_ (&p);
	} else
		v = atoi_ (&p);
	if (p == 0) {
		warnx ("%s: Invalid value", value);
		return -1;
	}

	if (strcasecmp ("hours", p) == 0 || strcasecmp ("hour", p) == 0)
		v *= 60 * 60;
	else if (strcasecmp ("minutes", p) == 0 ||
		 strcasecmp ("minute", p) == 0)
		v *= 60;
	else if (*p != 0 &&
		 strcasecmp ("second", p) != 0 &&
		 strcasecmp ("seconds", p) != 0) {
		warnx ("%s: Invalid value", value);
		return -1;
	}

	if (v < prop->min) {
		warnx ("%s: Too small", value);
		return -1;
	} else if (v > prop->max) {
		warnx ("%s: Too large", value);
		return -1;
	}

	prop->modified = 1;
	prop->modified_value.longword = v;

	return 0;
}

int
parse_bootdev (prop, value)
	struct property *prop;
	const char *value;
{
	const char *p = value;
	int v;

	while (*p == ' ' || *p == '\t');

	if (strcasecmp ("STD", p) == 0)
		v = 0;
	else if (strcasecmp ("ROM", p) == 0)
		v = 0xa000;
	else if (strcasecmp ("RAM", p) == 0)
		v = 0xb000;
	else if (strncasecmp ("HD", p, 2) == 0) {
		p += 2;
		v = atoi_ (&p);
		if (p == 0 || v < 0 || v > 15) {
			warnx ("%s: Invalid value", value);
			return -1;
		}
		v *= 0x0100;
		v += 0x8000;
	} else if (strncasecmp ("FD", p, 2) == 0) {
		p += 2;
		v = atoi_ (&p);
		if (p == 0 || v < 0 || v > 3) {
			warnx ("%s: Invalid value", value);
			return -1;
		}
		v *= 0x0100;
		v += 0x9070;
	} else {
		warnx ("%s: Invalid value", value);
		return -1;
	}

	prop->modified = 1;
	prop->modified_value.word[0] = v;

	return 0;
}

int
print_uchar (prop, str)
	struct property *prop;
	char *str;
{
	if (prop->modified)
		snprintf (str, MAXVALUELEN,
			  "%d", prop->modified_value.byte[0]);
	else {
		if (!prop->value_valid)
			prop->fill (prop);
		snprintf (str, MAXVALUELEN, "%d",
			  prop->current_value.byte[0]);
	}

	return 0;
}

int
print_ucharh (prop, str)
	struct property *prop;
	char *str;
{
	if (prop->modified)
		snprintf (str, MAXVALUELEN,
			  "0x%4.4x", prop->modified_value.byte[0]);
	else {
		if (!prop->value_valid)
			prop->fill (prop);
		snprintf (str, MAXVALUELEN,
			  "0x%4.4x", prop->current_value.byte[0]);
	}

	return 0;
}

int
print_ushorth (prop, str)
	struct property *prop;
	char *str;
{
	if (prop->modified)
		snprintf (str, MAXVALUELEN,
			  "0x%4.4x", prop->modified_value.word[0]);
	else {
		if (!prop->value_valid)
			prop->fill (prop);
		snprintf (str, MAXVALUELEN,
			  "0x%4.4x", prop->current_value.word[0]);
	}

	return 0;
}

int
print_ulong (prop, str)
	struct property *prop;
	char *str;
{
	if (prop->modified)
		snprintf (str, MAXVALUELEN,
			  "%ld", prop->modified_value.longword);
	else {
		if (!prop->value_valid)
			prop->fill (prop);
		snprintf (str, MAXVALUELEN,
			  "%ld", prop->current_value.longword);
	}

	return 0;
}

int
print_ulongh (prop, str)
	struct property *prop;
	char *str;
{
	if (prop->modified)
		snprintf (str, MAXVALUELEN,
			  "0x%8.8lx", prop->modified_value.longword);
	else {
		if (!prop->value_valid)
			prop->fill (prop);
		snprintf (str, MAXVALUELEN,
			  "0x%8.8lx", prop->current_value.longword);
	}

	return 0;
}

int
print_magic (prop, str)
	struct property *prop;
	char *str;
{
	if (!prop->value_valid)
		prop->fill (prop);
	snprintf (str, MAXVALUELEN, "%c%c%c%c",
		  prop->current_value.byte[0],
		  prop->current_value.byte[1],
		  prop->current_value.byte[2],
		  prop->current_value.byte[3]);

	return 0;
}

int
print_timesec (prop, str)
	struct property *prop;
	char *str;
{
	if (prop->modified)
		snprintf (str, MAXVALUELEN,
			  "%ld second", prop->modified_value.longword);
	else {
		if (!prop->value_valid)
			prop->fill (prop);
		snprintf (str, MAXVALUELEN,
			  "%ld second", prop->current_value.longword);
	}

	return 0;
}

int
print_bootdev (prop, str)
	struct property *prop;
	char *str;
{
	unsigned int v;

	if (prop->modified)
		v = prop->modified_value.word[0];
	else {
		if (!prop->value_valid)
			prop->fill (prop);
		v = prop->current_value.word[0];
	}

	if (v == 0)
		strcpy (str, "STD");
	else if (v == 0xa000)
		strcpy (str, "ROM");
	else if (v == 0xb000)
		strcpy (str, "RAM");
	else if (v >= 0x8000 && v < 0x9000)
		snprintf (str, MAXVALUELEN, "HD%d", (v & 0x0f00) >> 8);
	else if (v >= 0x9000 && v < 0xa000)
		snprintf (str, MAXVALUELEN, "FD%d", (v & 0x0f00) >> 8);
	else
		snprintf (str, MAXVALUELEN, "%8.8x", v);

	return 0;
}
