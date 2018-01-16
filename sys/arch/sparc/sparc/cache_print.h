/*	$NetBSD: cache_print.h,v 1.1 2018/01/16 08:23:17 mrg Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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

/*
 * This works for kernel and cpuctl(8).  It only relies upon having
 * a working printf() in the environment.
 */

static void cache_printf_backend(struct cacheinfo *ci, const char *cpuname);

static void
cache_printf_backend(struct cacheinfo *ci, const char *cpuname)
{

	if (ci->c_flags & CACHE_TRAPPAGEBUG)
		printf("%s: cache chip bug; trap page uncached\n", cpuname);

	printf("%s: ", cpuname);

	if (ci->c_totalsize == 0) {
		printf("no cache\n");
		return;
	}

	if (ci->c_split) {
		const char *sep = "";

		printf("%s", (ci->c_physical ? "physical " : ""));
		if (ci->ic_totalsize > 0) {
			printf("%s%dK instruction (%d b/l)", sep,
			    ci->ic_totalsize/1024, ci->ic_linesize);
			sep = ", ";
		}
		if (ci->dc_totalsize > 0) {
			printf("%s%dK data (%d b/l)", sep,
			    ci->dc_totalsize/1024, ci->dc_linesize);
		}
	} else if (ci->c_physical) {
		/* combined, physical */
		printf("physical %dK combined cache (%d bytes/line)",
		    ci->c_totalsize/1024, ci->c_linesize);
	} else {
		/* combined, virtual */
		printf("%dK byte write-%s, %d bytes/line, %cw flush",
		    ci->c_totalsize/1024,
		    (ci->c_vactype == VAC_WRITETHROUGH) ? "through" : "back",
		    ci->c_linesize,
		    ci->c_hwflush ? 'h' : 's');
	}

	if (ci->ec_totalsize > 0) {
		printf(", %dK external (%d b/l)",
		    ci->ec_totalsize/1024, ci->ec_linesize);
	}
	printf(": ");
	if (ci->c_enabled)
		printf("cache enabled");
	printf("\n");
}
