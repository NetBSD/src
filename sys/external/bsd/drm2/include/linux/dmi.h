/*	$NetBSD: dmi.h,v 1.3 2014/04/25 23:54:59 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef _LINUX_DMI_H_
#define _LINUX_DMI_H_

enum dmi_field {
	DMI_NONE,
	DMI_BIOS_VENDOR,
	DMI_BIOS_VERSION,
	DMI_BIOS_DATE,
	DMI_SYS_VENDOR,
	DMI_PRODUCT_NAME,
	DMI_PRODUCT_VERSION,
	DMI_PRODUCT_SERIAL,
	DMI_PRODUCT_UUID,
	DMI_BOARD_VENDOR,
	DMI_BOARD_NAME,
	DMI_BOARD_VERSION,
	DMI_BOARD_SERIAL,
	DMI_BOARD_ASSET_TAG,
	DMI_CHASSIS_VENDOR,
	DMI_CHASSIS_TYPE,
	DMI_CHASSIS_VERSION,
	DMI_CHASSIS_SERIAL,
	DMI_CHASSIS_ASSET_TAG,
	DMI_STRING_MAX
};

struct dmi_strmatch {
	unsigned char slot;
	char substr[79];
};

struct dmi_system_id {
	int (*callback)(const struct dmi_system_id *);
	const char *ident;
	struct dmi_strmatch matches[4];
};
#define DMI_MATCH(a, b)		{(a), (b)}
#define DMI_EXACT_MATCH(a, b)	{(a), (b)}

int dmi_check_system(const struct dmi_system_id *list);

#endif  /* _LINUX_DMI_H_ */
