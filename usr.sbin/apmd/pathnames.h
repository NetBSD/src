/*	$NetBSD: pathnames.h,v 1.5 2008/04/28 20:24:15 martin Exp $ */

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by John Kohl.
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

#define _PATH_APM_SOCKET	"/var/run/apmdev"
#define _PATH_APM_CTLDEV	"/dev/apmctl"
#define _PATH_APM_ETC_DIR	"/etc/apm"
#define _PATH_APM_ETC_SUSPEND	_PATH_APM_ETC_DIR"/suspend"
#define _PATH_APM_ETC_STANDBY	_PATH_APM_ETC_DIR"/standby"
#define _PATH_APM_ETC_RESUME	_PATH_APM_ETC_DIR"/resume"
#define _PATH_APM_ETC_LINE	_PATH_APM_ETC_DIR"/line"
#define _PATH_APM_ETC_BATTERY	_PATH_APM_ETC_DIR"/battery"
#define _PATH_APM_NORMAL	"/dev/apm"
#define _PATH_DEV_SPEAKER	"/dev/speaker"
