/*	$NetBSD: device.h,v 1.6 2018/08/27 14:19:25 riastradh Exp $	*/

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

#ifndef _LINUX_DEVICE_H_
#define _LINUX_DEVICE_H_

#include <sys/types.h>
#include <sys/systm.h>

#define	dev_crit(DEV, FMT, ...)	do {					      \
	if (DEV)							      \
		aprint_error_dev((DEV), "critical: " FMT, ##__VA_ARGS__);     \
	else								      \
		aprint_error("critical: " FMT, ##__VA_ARGS__);		      \
} while (0)

#define	dev_err(DEV, FMT, ...)	do {					      \
	if (DEV)							      \
		aprint_error_dev((DEV), "error: " FMT, ##__VA_ARGS__);	      \
	else								      \
		aprint_error("error: " FMT, ##__VA_ARGS__);		      \
} while (0)

#define	dev_warn(DEV, FMT, ...)	do {					      \
	if (DEV)							      \
		aprint_error_dev((DEV), "warn: " FMT, ##__VA_ARGS__);	      \
	else								      \
		aprint_error("warn: " FMT, ##__VA_ARGS__);		      \
} while (0)

#define	dev_notice(DEV, FMT, ...)	do {				      \
	if (DEV)							      \
		aprint_normal_dev((DEV), "notice: " FMT, ##__VA_ARGS__);      \
	else								      \
		aprint_normal("notice: " FMT, ##__VA_ARGS__);		      \
} while (0)

#define	dev_info(DEV, FMT, ...)	do {					      \
	if (DEV)							      \
		aprint_normal_dev((DEV), "info: " FMT, ##__VA_ARGS__);	      \
	else								      \
		aprint_normal("info: " FMT, ##__VA_ARGS__);		      \
} while (0)

#define	dev_dbg(DEV, FMT, ...)	do {					      \
	if (DEV)							      \
		aprint_debug_dev((DEV), "debug: " FMT, ##__VA_ARGS__);	      \
	else								      \
		aprint_debug("debug: " FMT, ##__VA_ARGS__);		      \
} while (0)

#define	dev_name	device_xname

#endif  /* _LINUX_DEVICE_H_ */
