/*	$NetBSD: wsconsctl.h,v 1.12 2012/12/24 01:20:44 khorben Exp $ */

/*-
 * Copyright (c) 1998, 2004, 2012 The NetBSD Foundation, Inc.
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

#include <dev/wscons/wsksymvar.h>

/* fg / bg values. Made identical to ANSI terminal color codes. */
#define WSCOL_UNSUPPORTED	-1
#define WSCOL_BLACK		0
#define WSCOL_RED		1
#define WSCOL_GREEN		2
#define WSCOL_BROWN		3
#define WSCOL_BLUE		4
#define WSCOL_MAGENTA		5
#define WSCOL_CYAN		6
#define WSCOL_WHITE		7
/* flag values: */
#define WSATTR_NONE		0
#define WSATTR_REVERSE		1
#define WSATTR_HILIT		2
#define WSATTR_BLINK		4
#define WSATTR_UNDERLINE	8
#define WSATTR_WSCOLORS 	16

struct field {
	const char *name;
	void *valp;
#define FMT_UINT	1		/* unsigned integer */
#define FMT_STRING	2		/* zero terminated string */
#define FMT_BITFIELD	3		/* bit field */
#define FMT_INT		4		/* signed integer */
#define FMT_KBDTYPE	101		/* keyboard type */
#define FMT_MSTYPE	102		/* mouse type */
#define FMT_DPYTYPE	103		/* display type */
#define FMT_KBDENC	104		/* keyboard encoding */
#define FMT_KBMAP	105		/* keyboard map */
#define FMT_COLOR	201		/* display color */
#define FMT_ATTRS	202		/* display attributes */
	int format;
#define FLG_RDONLY	0x0001		/* variable cannot be modified */
#define FLG_WRONLY	0x0002		/* variable cannot be displayed */
#define FLG_NOAUTO	0x0004		/* skip variable on -a flag */
#define FLG_MODIFY	0x0008		/* variable may be modified with += */
#define FLG_DISABLED	0x0010		/* variable is not available */
#define FLG_GET		0x0100		/* read this variable from driver */
#define FLG_SET		0x0200		/* write this variable to driver */
	int flags;
};

void field_setup(struct field *, int);
struct field *field_by_name(char *);
struct field *field_by_value(void *);
void field_disable_by_value(void *);
void pr_field(struct field *, const char *);
void rd_field(struct field *, char *, int);
int name2ksym(char *);
const char *ksym2name(int);
keysym_t ksym_upcase(keysym_t);
void keyboard_get_values(int);
void keyboard_put_values(int);
void mouse_get_values(int);
void mouse_put_values(int);
void display_get_values(int);
void display_put_values(int);
#ifndef YYEMPTY
int yyparse(void);
#endif
int yylex(void);
void map_scan_setinput(char *);
