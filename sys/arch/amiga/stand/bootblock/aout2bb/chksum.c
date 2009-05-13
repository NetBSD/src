/* $NetBSD: chksum.c,v 1.6.14.1 2009/05/13 17:16:11 jym Exp $ */

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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include "chksum.h"

u_int32_t
chksum(u_int32_t *block, int size)
{
	u_int32_t sum, lastsum;
	int i;

	sum = 0;

	for (i=0; i<size; i++) {
		lastsum = sum;
		sum += block[i];
		if (sum < lastsum)
			++sum;
	}

	return sum;
}

#ifdef TESTSUM
u_int32_t myblock[8192];

int
main(int argc, char *argb[]) {
	int bbsize;
	u_int32_t cks, cks1;

	bbsize=atol(argb[1]);
	bbsize *= (512 / sizeof (u_int32_t));

	if (4*bbsize != read(0, myblock, sizeof(u_int32_t)*bbsize)) {
		fprintf(stderr, "short read\n");
		exit(1);
	}
	fprintf(stderr, "Cksum field = 0x%x, ", myblock[1]);
	cks = chksum(myblock, bbsize);
	fprintf(stderr, "cksum = 0x%x\n", cks);
	myblock[1] += 0xFFFFFFFF - cks;
	fprintf(stderr, "New cksum field = 0x%x, ", myblock[1]);
	cks1 = chksum(myblock, bbsize);
	fprintf(stderr, "cksum = 0x%x\n", cks1);
}
#endif
