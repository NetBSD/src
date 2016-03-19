/* $NetBSD: amigatypes.h,v 1.7.64.1 2016/03/19 11:29:55 skrll Exp $ */

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ignatios Souvatzis.
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

#ifndef _AMIGA_TYPES_H_
#define _AMIGA_TYPES_H_

/* Dummy structs, used only as abstract pointers */

struct Library;
struct TextAttr;
struct Gadget;
struct BitMap;
struct NewScreen;
struct MemNode;

/* real structs */

struct TagItem {u_int32_t item; void *data;};

struct Library {
	u_int8_t Dmy1[20];
	u_int16_t Version, Revision;
	u_int8_t Dmy2[34-24];
};

struct MemHead {
	struct MemHead *next;
	u_int8_t Dmy1[  9-  4];
	int8_t Pri;
	u_int8_t Dmy2[ 14- 10];
	u_int16_t Attribs;
	u_int32_t First, Lower, Upper, Free;
};

struct ExecBase {
	struct Library LibNode;
	u_int8_t Dmy1[296-34];
	u_int16_t AttnFlags;	/* 296 */
	u_int8_t Dmy2[300-298];	/* 298 */
	void *ResModules;	/* 300 */
	u_int8_t Dmy3[322-304];	/* 304 */
	struct MemHead *MemLst;	/* 322 */
	/*
	 * XXX: actually, its a longer List base, but we only need to
	 * search it once.
	 */
	u_int8_t Dmy4[568-326];	/* 326 */
	u_int32_t EClockFreq;	/* 330 */
	u_int8_t Dmy5[632-334];
} __attribute__((packed));

#endif /* _AMIGA_TYPES_H */
