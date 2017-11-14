/*	$NetBSD: console.c,v 1.2 2017/11/14 07:06:34 maxv Exp $	*/

/*
 * Copyright (c) 2017 The NetBSD Foundation, Inc. All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard.
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

#include "prekern.h"

extern vaddr_t atdevbase;
#define CONS_WID 80
#define CONS_HEI 25

static char *cons_start;
static size_t cons_x, cons_y;
static char cons_buffer[CONS_WID * 2 * CONS_HEI];

void init_cons(void)
{
	cons_start = (char *)atdevbase + (0xB8000 - IOM_BEGIN);
	cons_x = 0;
	cons_y = 0;
}

static void check_scroll(void)
{
	char *src, *dst;
	size_t i;

	if (cons_y != CONS_HEI)
		return;

	for (i = 0; i < CONS_HEI-1; i++) {
		dst = &cons_buffer[0] + i * (CONS_WID * 2);
		src = &cons_buffer[0] + (i + 1) * (CONS_WID * 2);
		memcpy(dst, src, (CONS_WID * 2));
	}
	memset(&cons_buffer[0] + (CONS_WID * 2) * (CONS_HEI-1), 0,
	    (CONS_WID * 2));
	cons_y--;
	memcpy(cons_start, &cons_buffer[0], (CONS_WID * 2) * (CONS_HEI-1));
}

void print_ext(int color, char *buf)
{
	char *ptr, *scr;
	size_t i;

	for (i = 0; buf[i] != '\0'; i++) {
		if (buf[i] == '\n') {
			cons_x = 0;
			cons_y++;
			check_scroll();
		} else {
			if (cons_x + 1 == CONS_WID) {
				cons_x = 0;
				cons_y++;
				check_scroll();
			}
			ptr = (cons_start + 2 * cons_x + 160 * cons_y);
			scr = (cons_buffer + 2 * cons_x + 160 * cons_y);
			ptr[0] = scr[0] = buf[i];
			ptr[1] = scr[1] = color;
			cons_x++;
		}
	}
}

void print(char *buf)
{
	print_ext(WHITE_ON_BLACK, buf);
}

void print_state(bool ok, char *buf)
{
	print("[");
	if (ok)
		print_ext(GREEN_ON_BLACK, "+");
	else
		print_ext(RED_ON_BLACK, "!");
	print("] ");
	print(buf);
	print("\n");
}

void print_banner(void)
{
	char *banner = 
		"           __________                 __                        \n"
		"           \\______   \\_______   ____ |  | __ ___________  ____  \n"
		"            |     ___/\\_  __ \\_/ __ \\|  |/ // __ \\_  __ \\/    \\ \n"
		"            |    |     |  | \\/\\  ___/|    <\\  ___/|  | \\/   |  \\\n"
		"            |____|     |__|    \\___  >__|_ \\\\___  >__|  |___|  /\n"
		"                                   \\/     \\/    \\/           \\/    Version 1.0\n"
	;
	print(banner);
}
