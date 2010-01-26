/*	$NetBSD: posix_memalign.c,v 1.3 2010/01/26 22:14:01 mrg Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
__RCSID("$NetBSD: posix_memalign.c,v 1.3 2010/01/26 22:14:01 mrg Exp $");

#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <err.h>
#include <stdbool.h>

#ifdef _LP64
#define FOURFAIL	true
#else
#define FOURFAIL	false
#endif

struct {
	size_t	size;
	size_t	align;
	bool	fail;
} testlist[] = {
	{ 1, 512, false, },
	{ 2, 1024, false, },
	{ 3, 16, false, },
	{ 4, 32, false, },
	{ 10, 64, false, },
	{ 100, 4, FOURFAIL, },
	{ 10, 2, true, },
	{ 16384, 2048, false, },
};

int
main(int argc, char *argv[])
{
	size_t i;
	void *p;
	int error;

	for (i = 0; i < __arraycount(testlist); i++) {
		error = posix_memalign(&p, testlist[i].align, testlist[i].size);
		if (testlist[i].fail == false) {
			if (error != 0) {
				errno = error;
				err(1, "a=%zu, s=%zu", testlist[i].align, testlist[i].size);
			}
			if (((intptr_t)p) & (testlist[i].align - 1))
				errx(1, "p=%p a=%zu, s=%zu", p, testlist[i].align, testlist[i].size);
		} else if (error == 0)
			errx(1, "didn't fail: a=%zu, s=%zu", testlist[i].align, testlist[i].size);
	}

	/* This can fail */
	if (posix_memalign(&p, 32768, 16) == 0)
		if (((intptr_t)p) & (32768 - 1))
			errx(1, "p=%p a=%zu, s=%zu", p, 32768, 16);
	return 0;
}
