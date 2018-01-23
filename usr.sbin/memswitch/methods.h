/*	$NetBSD: methods.h,v 1.5 2018/01/23 21:06:25 sevan Exp $	*/

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


int atoi_(const char**);

int fill_uchar(struct property*);
int fill_ushort(struct property*);
int fill_ulong(struct property*);

int flush_uchar(struct property*);
int flush_ushort(struct property*);
int flush_ulong(struct property*);
int flush_dummy(struct property*);

int parse_dummy(struct property*, const char*);
int parse_uchar(struct property*, const char*);
int parse_ushort(struct property*, const char*);
int parse_ulong(struct property*, const char*);
int parse_byte(struct property*, const char*);
int parse_time(struct property*, const char*);
int parse_bootdev(struct property*, const char*);
int parse_serial(struct property*, const char*);
int parse_srammode(struct property*, const char*);

int print_uchar(struct property*, char*);
int print_ucharh(struct property*, char*);
int print_ushorth(struct property*, char*);
int print_ulong(struct property*, char*);
int print_ulongh(struct property*, char*);
int print_magic(struct property*, char*);
int print_timesec(struct property*, char*);
int print_bootdev(struct property*, char*);
int print_serial(struct property*, char*);
int print_srammode(struct property*, char*);
