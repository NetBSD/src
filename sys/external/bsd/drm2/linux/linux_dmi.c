/*	$NetBSD: linux_dmi.c,v 1.1.10.2 2014/08/20 00:04:22 tls Exp $	*/

/*-
 * Copyright (C) 2014 NONAKA Kimihiro <nonaka@netbsd.org>
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_dmi.c,v 1.1.10.2 2014/08/20 00:04:22 tls Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/pmf.h>

#include <linux/dmi.h>

static bool
dmi_found(const struct dmi_system_id *dsi)
{
	const char *p;
	int i, slot;

	for (i = 0; i < __arraycount(dsi->matches); i++) {
		p = NULL;
		slot = dsi->matches[i].slot;
		switch (slot) {
		case DMI_NONE:
			continue;
		case DMI_BIOS_VENDOR:
			p = pmf_get_platform("bios-vendor");
			break;
		case DMI_BIOS_VERSION:
			p = pmf_get_platform("bios-version");
			break;
		case DMI_BIOS_DATE:
			p = pmf_get_platform("bios-date");
			break;
		case DMI_SYS_VENDOR:
			p = pmf_get_platform("system-vendor");
			break;
		case DMI_PRODUCT_NAME:
			p = pmf_get_platform("system-product");
			break;
		case DMI_PRODUCT_VERSION:
			p = pmf_get_platform("system-version");
			break;
		case DMI_PRODUCT_SERIAL:
			p = pmf_get_platform("system-serial");
			break;
		case DMI_PRODUCT_UUID:
			p = pmf_get_platform("system-uuid");
			break;
		case DMI_BOARD_VENDOR:
			p = pmf_get_platform("board-vendor");
			break;
		case DMI_BOARD_NAME:
			p = pmf_get_platform("board-product");
			break;
		case DMI_BOARD_VERSION:
			p = pmf_get_platform("board-version");
			break;
		case DMI_BOARD_SERIAL:
			p = pmf_get_platform("board-serial");
			break;
		case DMI_BOARD_ASSET_TAG:
			p = pmf_get_platform("board-asset-tag");
			break;
		case DMI_CHASSIS_VENDOR:
		case DMI_CHASSIS_TYPE:
		case DMI_CHASSIS_VERSION:
		case DMI_CHASSIS_SERIAL:
		case DMI_CHASSIS_ASSET_TAG:
			return false;
		case DMI_STRING_MAX:
		default:
			aprint_error("%s: unknown DMI field(%d)\n", __func__,
			    slot);
			return false;
		}
		if (p == NULL || strcmp(p, dsi->matches[i].substr))
			return false;
	}
	return true;
}

int
dmi_check_system(const struct dmi_system_id *sysid)
{
	const struct dmi_system_id *dsi;
	int num = 0;

	for (dsi = sysid; dsi->matches[0].slot != DMI_NONE; dsi++) {
		if (dmi_found(dsi)) {
			num++;
			if (dsi->callback && dsi->callback(dsi))
				break;
		}
	}
	return num;
}
