/*	$NetBSD: posix_memalign.c,v 1.1 2008/02/03 22:56:14 christos Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
#include <sys/cdefs.h>
__RCSID("$NetBSD: posix_memalign.c,v 1.1 2008/02/03 22:56:14 christos Exp $");

#include <stdint.h>
#include <stdlib.h>
#include <err.h>

size_t size[] = {
	1, 2, 3, 4, 10, 100, 16384
};
size_t align[] = {
	512, 1024, 16, 32, 64, 4, 2048
};

int
main(int argc, char *argv[])
{
	size_t i;
	void *p;

	for (i = 0; i < __arraycount(size); i++) {
		if (posix_memalign(&p, align[i], size[i]) != 0)
			err(1, "a=%zu, s=%zu", align[i], size[i]);
		if (((intptr_t)p) & (align[i] - 1))
			errx(1, "p=%p a=%zu, s=%zu", p, align[i], size[i]);
	}

	/* This can fail */
	if (posix_memalign(&p, 32768, 16) == 0)
		if (((intptr_t)p) & (32768 - 1))
			errx(1, "p=%p a=%zu, s=%zu", p, align[i], size[i]);
	return 0;
}
