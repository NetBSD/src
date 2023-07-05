/*	$NetBSD: t_posix_memalign.c,v 1.8 2023/07/05 12:09:39 riastradh Exp $ */

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
__COPYRIGHT("@(#) Copyright (c) 2008\
 The NetBSD Foundation, inc. All rights reserved.");
__RCSID("$NetBSD: t_posix_memalign.c,v 1.8 2023/07/05 12:09:39 riastradh Exp $");

#include <atf-c.h>

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define	rounddown(x, n)	(((x) / (n)) * (n))

ATF_TC(posix_memalign_basic);
ATF_TC_HEAD(posix_memalign_basic, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks posix_memalign(3)");
}
ATF_TC_BODY(posix_memalign_basic, tc)
{
	enum { maxaligntest = 16384 };
	static const size_t align[] = {
		0, 1, 2, 3, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096,
		8192, maxaligntest,
	};
	static const size_t size[] = {
		0, 1, 2, 3, 4, 10, 100, 10000, 16384, 32768, 65536,
		rounddown(SIZE_MAX, maxaligntest),
	};
	size_t i, j;

	for (i = 0; i < __arraycount(align); i++) {
		for (j = 0; j < __arraycount(size); j++) {
			void *p = (void *)0x1;
			const int ret = posix_memalign(&p, align[i], size[j]);

			if (align[i] == 0 ||
			    (align[i] & (align[i] - 1)) != 0 ||
			    align[i] < sizeof(void *)) {
				ATF_CHECK_EQ_MSG(ret, EINVAL,
				    "posix_memalign(&p, %zu, %zu): %s",
				    align[i], size[j], strerror(ret));
				continue;
			}
			if (size[j] == rounddown(SIZE_MAX, maxaligntest) &&
			    ret != EINVAL) {
				/*
				 * If obscenely large alignment isn't
				 * rejected as EINVAL, we can't
				 * allocate that much memory anyway.
				 */
				ATF_CHECK_EQ_MSG(ret, ENOMEM,
				    "posix_memalign(&p, %zu, %zu): %s",
				    align[i], size[j], strerror(ret));
				continue;
			}

			/*
			 * Allocation should fail only if the alignment
			 * isn't supported, in which case it will fail
			 * with EINVAL.  No standard criterion for what
			 * alignments are supported, so just stop here
			 * on EINVAL.
			 */
			if (ret == EINVAL)
				continue;

			ATF_CHECK_EQ_MSG(ret, 0,
			    "posix_memalign(&p, %zu, %zu): %s",
			    align[i], size[j], strerror(ret));
			ATF_CHECK_EQ_MSG((intptr_t)p & (align[i] - 1), 0,
			    "posix_memalign(&p, %zu, %zu): %p",
			    align[i], size[j], p);

			if (size[j] != 0) {
				if (p == NULL) {
					atf_tc_fail_nonfatal(
					    "%s:%d:"
					    "posix_memalign(&p, %zu, %zu):"
					    " %p",
					    __FILE__, __LINE__,
					    align[i], size[j], p);
				}
			} else {
				/*
				 * No guarantees about whether
				 * zero-size allocation yields null
				 * pointer or something else.
				 */
			}

			free(p);
		}
	}
}


ATF_TC(aligned_alloc_basic);
ATF_TC_HEAD(aligned_alloc_basic, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks aligned_alloc(3)");
}
ATF_TC_BODY(aligned_alloc_basic, tc)
{
	enum { maxaligntest = 16384 };
	static const size_t align[] = {
		0, 1, 2, 3, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096,
		8192, maxaligntest,
	};
	static const size_t size[] = {
		0, 1, 2, 3, 4, 10, 100, 10000, 16384, 32768, 65536,
		rounddown(SIZE_MAX, maxaligntest),
	};
	size_t i, j;

	for (i = 0; i < __arraycount(align); i++) {
		for (j = 0; j < __arraycount(size); j++) {
			void *const p = aligned_alloc(align[i], size[j]);

			/*
			 * C17, 6.2.8 Alignment of objects, paragraph
			 * 4, p. 37:
			 *
			 *	Every valid alignment value shall be a
			 *	nonnegative integral power of two.
			 *
			 * C17, 7.22.3.1 The aligned_alloc function,
			 * paragraph 2, p. 348:
			 *
			 *	If the value of alignment is not a
			 *	valid alignment supported by the
			 *	implementation the function shall fail
			 *	by returning a null pointer.
			 *
			 * Setting errno to EINVAL is a NetBSD
			 * extension.  The last clause appears to rule
			 * out aligned_alloc(n, 0) for any n, but it's
			 * not clear.
			 */
			if (align[i] == 0 ||
			    (align[i] & (align[i] - 1)) != 0) {
				if (p != NULL) {
					ATF_CHECK_EQ_MSG(p, NULL,
					    "aligned_alloc(%zu, %zu): %p",
					    align[i], size[j], p);
					continue;
				}
				ATF_CHECK_EQ_MSG(errno, EINVAL,
				    "aligned_alloc(%zu, %zu): %s",
				    align[i], size[j], strerror(errno));
				continue;
			}

			if (size[j] == rounddown(SIZE_MAX, maxaligntest)) {
				ATF_CHECK_EQ_MSG(p, NULL,
				    "aligned_alloc(%zu, %zu): %p, %s",
				    align[i], size[j], p, strerror(errno));
				ATF_CHECK_MSG((errno == EINVAL ||
					errno == ENOMEM),
				    "aligned_alloc(%zu, %zu): %s",
				    align[i], size[j],
				    strerror(errno));
				continue;
			}

			/*
			 * Allocation should fail only if the alignment
			 * isn't supported, in which case it will fail
			 * with EINVAL.  No standard criterion for what
			 * alignments are supported, so just stop here
			 * on EINVAL.
			 */
			if (p == NULL && errno == EINVAL)
				continue;

			ATF_CHECK_EQ_MSG((intptr_t)p & (align[i] - 1), 0,
			    "aligned_alloc(%zu, %zu): %p",
			    align[i], size[j], p);
			if (size[j] != 0) {
				ATF_CHECK_MSG(p != NULL,
				    "aligned_alloc(&p, %zu, %zu): %p, %s",
				    align[i], size[j], p,
				    strerror(errno));
			} else {
				/*
				 * No guarantees about whether
				 * zero-size allocation yields null
				 * pointer or something else.
				 */
			}

			free(p);
		}
	}
}


ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, posix_memalign_basic);
	ATF_TP_ADD_TC(tp, aligned_alloc_basic);

	return atf_no_error();
}
