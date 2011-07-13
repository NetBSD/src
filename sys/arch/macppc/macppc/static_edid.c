/*	$NetBSD: static_edid.c,v 1.1 2011/07/13 22:54:33 macallan Exp $ */

/*-
 * Copyright (c) 2011 Michael Lorenz
 * All rights reserved.
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
__KERNEL_RCSID(0, "$NetBSD: static_edid.c,v 1.1 2011/07/13 22:54:33 macallan Exp $");
#include <sys/param.h>

/* EDID blocks for some known hardware that doesn't provide its own */

/*
 * PowerBook Pismo, derived from iBook G4 which needs the same parameters
 * should work on Lombard, PDQ and TFT Wallstreets as well
 */
uint8_t edid_pismo[128] = {
/* 00 */	0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
/* 08 */	0x06, 0x10, 0x1f, 0x9c, 0x01, 0x01, 0x01, 0x01,
/* 10 */	0x00, 0x0c, 0x01, 0x03, 0x80, 0x1c, 0x15, 0x78,
/* 18 */	0x0a, 0xe7, 0xb5, 0x93, 0x56, 0x4f, 0x8d, 0x28,
/* 20 */	0x1f, 0x50, 0x54, 0x00, 0x08, 0x00, 0x01, 0x01,
/* 28 */	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
/* 30 */	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x64, 0x19,
/* 38 */	0x00, 0x40, 0x41, 0x00, 0x26, 0x30, 0x18, 0x88,
/* 40 */	0x36, 0x00, 0xf6, 0xb8, 0x00, 0x00, 0x00, 0x18,
/* 48 */	0x00, 0x00, 0x00, 0xfe, 0x00, 0x49, 0x42, 0x4d,
/* 50 */	0x2d, 0x49, 0x41, 0x58, 0x47, 0x30, 0x31, 0x41,
/* 58 */	0x0a, 0x20, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x49,
/* 60 */	0x42, 0x4d, 0x2d, 0x49, 0x41, 0x58, 0x47, 0x30,
/* 68 */	0x31, 0x41, 0x0a, 0x20, 0x00, 0x00, 0x00, 0xfc,
/* 70 */	0x00, 0x43, 0x6f, 0x6c, 0x6f, 0x72, 0x20, 0x4c,
/* 78 */	0x43, 0x44, 0x0a, 0x20, 0x20, 0x20, 0x00, 0x52
};

