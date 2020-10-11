/*	$NetBSD: static_edid.c,v 1.1 2020/10/11 19:39:22 jdc Exp $ */

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman.
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
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: static_edid.c,v 1.1 2020/10/11 19:39:22 jdc Exp $");
#include <sys/param.h>

/* EDID blocks for some known hardware that doesn't provide its own */

/*
 * Naturetech Mesostation 999
 * Based on VESA DMT 1E, but with slight adjustment to the horizontal timing
 */
uint8_t edid_meso999[128] = {
/* 00 */	0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
/* 08 */	0x38, 0x34, 0x99, 0x09, 0x00, 0x00, 0x00, 0x00,
/* 10 */	0xff, 0x0f, 0x01, 0x03, 0x68, 0x25, 0x17, 0x78,
/* 18 */	0x2a, 0x3c, 0x40, 0x9c, 0x56, 0x4d, 0x98, 0x26,
/* 20 */	0x16, 0x50, 0x54, 0xa5, 0x4a, 0x80, 0x81, 0x40,
/* 28 */	0x81, 0x80, 0x81, 0x8f, 0x95, 0x00, 0x01, 0x01,
/* 30 */	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xab, 0x22,
/* 38 */	0xa0, 0xa0, 0x50, 0x84, 0x1a, 0x30, 0x38, 0x20,
/* 40 */	0x36, 0x00, 0x9a, 0x01, 0x11, 0x00, 0x00, 0x1a,
/* 48 */	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 50 */	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 58 */	0x00, 0x01, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x4e,
/* 60 */	0x41, 0x74, 0x75, 0x72, 0x65, 0x74, 0x65, 0x63,
/* 68 */	0x68, 0x0a, 0x20, 0x20, 0x00, 0x00, 0x00, 0xfe,
/* 70 */	0x00, 0x4d, 0x65, 0x73, 0x6f, 0x20, 0x39, 0x39,
/* 78 */	0x39, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x00, 0xed
};
