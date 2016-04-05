/*	$NetBSD: reallocarray.c,v 1.6 2016/04/05 15:01:26 roy Exp $	*/
/*	$OpenBSD: reallocarray.c,v 1.1 2014/05/08 21:43:49 deraadt Exp $	*/

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: reallocarray.c,v 1.6 2016/04/05 15:01:26 roy Exp $");

#define _OPENBSD_SOURCE
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

#define SQRT_SIZE_MAX (((size_t)1) << (sizeof(size_t) * CHAR_BIT / 2))
void *
reallocarray(void *optr, size_t nmemb, size_t size)
{

	/*
	 * Try to avoid division here.
	 *
	 * It isn't possible to overflow during multiplication if neither
	 * operand uses any of the most significant half of the bits.
	 */
	if (__predict_false((nmemb | size) >= SQRT_SIZE_MAX &&
			    nmemb > SIZE_MAX / size))
	{
		errno = EOVERFLOW;
		return NULL;
	}
	return realloc(optr, nmemb * size);
}
