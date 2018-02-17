/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
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

#ifdef TEST
# define FAIL(msg, ...)	err(EXIT_FAILURE, msg, ## __VA_ARGS__)
# define CHECK_EQUAL(a, b, msg) \
    do { \
	    if ((a) != (b)) \
		errx(EXIT_FAILURE, "%s: " # a "(%ju) != " # b "(%ju)", \
		    msg, (uintmax_t)(a), (uintmax_t)(b)); \
    } while (/*CONSTCOND*/0)
#else

#include <atf-c.h>
# define FAIL(msg, ...)	\
	do { \
		ATF_CHECK_MSG(0, msg, ## __VA_ARGS__); \
		goto fail; \
	} while (/*CONSTCOND*/0)
# define CHECK_EQUAL(a, b, msg) ATF_CHECK_EQ_MSG(a, b, "%s: " \
    # a "(%ju) != " # b "(%ju) ", msg, (uintmax_t)(a), (uintmax_t)(b));
#endif
