/*	$NetBSD: h_r_rel.c,v 1.1 2025/05/02 03:26:26 riastradh Exp $	*/

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: h_r_rel.c,v 1.1 2025/05/02 03:26:26 riastradh Exp $");

#include <stdio.h>

/*
 * When built as position-independent executable, the value of foop and
 * foopp should be computed either via R_*_RELATIVE or R_*_REL32 or
 * similar, which -- ports that support it -- may be compressed into a
 * SHT_RELR section.
 *
 * One pointer indirection is enough to produce this effect, but we use
 * two pointer indirections to increase the probability of a crash in
 * case the relocations are done wrong.
 */
static int foo = 0x5f4d7635;
static int *volatile foop = &foo;
static int *volatile *volatile foopp = &foop;

/*
 * The RELR section compresses relocations for adjacent addresses into
 * bitmaps of 31 or 63 bits apiece.  Create a bunch of consecutive
 * addresses to relocate, punctuated by the occasional non-relocated
 * address (null), to check for fencepost errors in the bitmap
 * iteration.
 */
static int bar[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
	0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
	0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
	0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
	0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
};

static int *volatile barp[] = {
	&bar[0x00], &bar[0x01], &bar[0x02], &bar[0x03],
	&bar[0x04], &bar[0x05], &bar[0x06], &bar[0x07],
	&bar[0x08], &bar[0x09], &bar[0x0a], &bar[0x0b],
	&bar[0x0c], &bar[0x0d], &bar[0x0e], &bar[0x0f],
	&bar[0x10], &bar[0x11], &bar[0x12], &bar[0x13],
	&bar[0x14], &bar[0x15], &bar[0x16], &bar[0x17],
	&bar[0x18], &bar[0x19], &bar[0x1a], &bar[0x1b],
	&bar[0x1c], &bar[0x1d], &bar[0x1e], &bar[0x1f],
	&bar[0x20], &bar[0x21], &bar[0x22], &bar[0x23],
	&bar[0x24], &bar[0x25], &bar[0x26], &bar[0x27],
	&bar[0x28], &bar[0x29], &bar[0x2a], &bar[0x2b],
	&bar[0x2c], &bar[0x2d], &bar[0x2e], &bar[0x2f],
	&bar[0x30], &bar[0x31], &bar[0x32], &bar[0x33],
	&bar[0x34], &bar[0x35], &bar[0x36], &bar[0x37],
	&bar[0x38], &bar[0x39], &bar[0x3a], &bar[0x3b],
	&bar[0x3c], &bar[0x3d], &bar[0x3e], &bar[0x3f],
	NULL,       &bar[0x41], &bar[0x42], &bar[0x43], /* test a clear bit */
	&bar[0x44], &bar[0x45], &bar[0x46], &bar[0x47],
	&bar[0x48], &bar[0x49], &bar[0x4a], &bar[0x4b],
	&bar[0x4c], &bar[0x4d], &bar[0x4e], &bar[0x4f],
	&bar[0x50], &bar[0x51], &bar[0x52], &bar[0x53],
	&bar[0x54], &bar[0x55], &bar[0x56], &bar[0x57],
	&bar[0x58], &bar[0x59], &bar[0x5a], &bar[0x5b],
	&bar[0x5c], &bar[0x5d], &bar[0x5e], &bar[0x5f],
	&bar[0x60], &bar[0x61], &bar[0x62], &bar[0x63],
	&bar[0x64], &bar[0x65], &bar[0x66], &bar[0x67],
	&bar[0x68], &bar[0x69], &bar[0x6a], &bar[0x6b],
	&bar[0x6c], &bar[0x6d], &bar[0x6e], &bar[0x6f],
	&bar[0x70], &bar[0x71], &bar[0x72], &bar[0x73],
	&bar[0x74], &bar[0x75], &bar[0x76], &bar[0x77],
	&bar[0x78], &bar[0x79], &bar[0x7a], &bar[0x7b],
	&bar[0x7c], &bar[0x7d], &bar[0x7e], &bar[0x7f],
	NULL,		/* confirm we stop at the end */
};

static int baz = -0x1adbd477;
static int *volatile bazp = &baz;

int
main(void)
{
	int i, result = 0;

	if (**foopp != 0x5f4d7635) {
		fprintf(stderr, "foo @ %p, foop = %p, *foop = %p,"
		    " **foopp = 0x%x\n",
		    &foo, foop, *foopp, **foopp);
		result |= 1;
	}
	for (i = 0; i < (int)__arraycount(barp); i++) {
		if (i == 0x40 || i == 0x80) {
			if (barp[i] != NULL) {
				fprintf(stderr, "barp[%u] = %p\n",
				    i, barp[i]);
			}
		} else {
			if (*barp[i] != i) {
				fprintf(stderr, "bar[%u] @ %p, barp[%u] = %p,"
				    " *barp[%u] = %u\n",
				    i, &bar[i], i, barp[i], i, *barp[i]);
				result |= 1;
			}
		}
	}
	if (*bazp != -0x1adbd477) {
		fprintf(stderr, "baz @ %p, bazp = %p, *bazp = 0x%x\n",
		    &baz, bazp, *bazp);
		result |= 1;
	}

	return result;
}
