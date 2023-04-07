/* $NetBSD: fdt_platform.c,v 1.1 2023/04/07 08:55:31 skrll Exp $ */

/*-
 * Copyright (c) 2017 Jared D. McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fdt_platform.c,v 1.1 2023/04/07 08:55:31 skrll Exp $");

#include <sys/param.h>

#include <dev/ofw/openfirm.h>

#include <dev/fdt/fdtvar.h>

const struct fdt_platform *
fdt_platform_find(void)
{
	static const struct fdt_platform_info *booted_platform = NULL;
	__link_set_decl(fdt_platforms, struct fdt_platform_info);
	struct fdt_platform_info * const *info;

	if (booted_platform == NULL) {
		const struct fdt_platform_info *best_info = NULL;
		const int phandle = OF_peer(0);
		int match, best_match = 0;

		__link_set_foreach(info, fdt_platforms) {
			const struct device_compatible_entry compat_data[] = {
				{ .compat = (*info)->fpi_compat },
				DEVICE_COMPAT_EOL
			};

			match = of_compatible_match(phandle, compat_data);
			if (match > best_match) {
				best_match = match;
				best_info = *info;
			}
		}

		booted_platform = best_info;
	}

	/*
	 * No SoC specific platform was found. Try to find a generic
	 * platform definition and use that if available.
	 */
	if (booted_platform == NULL) {
		__link_set_foreach(info, fdt_platforms) {
			if (strcmp((*info)->fpi_compat, FDT_PLATFORM_DEFAULT) == 0) {
				booted_platform = *info;
				break;
			}
		}
	}

	return booted_platform == NULL ? NULL : booted_platform->fpi_ops;
}

// XXXNH remove this and rely on a default
FDT_PLATFORM(dummy, NULL, NULL);
