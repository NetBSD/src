/*	$NetBSD: firmware.h,v 1.12 2021/12/19 12:01:04 riastradh Exp $	*/

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

#ifndef _LINUX_FIRMWARE_H_
#define _LINUX_FIRMWARE_H_

#include <sys/types.h>

#include <dev/firmload.h>

#include <linux/slab.h>

struct device;
struct module;

struct firmware {
	char			*data;
	size_t			size;
};

#define	firmware_request_nowarn		linux_firmware_request_nowarn
#define	release_firmware		linux_release_firmware
#define	request_firmware		linux_request_firmware
#define	request_firmware_direct		linux_request_firmware_direct
#define	request_firmware_nowait		linux_request_firmware_nowait

int	request_firmware(const struct firmware **, const char *,
	    struct device *);
int	request_firmware_direct(const struct firmware **, const char *,
	    struct device *);
int	firmware_request_nowarn(const struct firmware **, const char *,
	    struct device *);
int	request_firmware_nowait(struct module *, bool, const char *,
	    struct device *, gfp_t, void *,
	    void (*)(const struct firmware *, void *));
void	release_firmware(const struct firmware *);

#endif  /* _LINUX_FIRMWARE_H_ */
