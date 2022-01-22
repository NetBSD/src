/* $NetBSD: fuse_log.h,v 1.1 2022/01/22 07:39:22 pho Exp $ */

/*
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
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
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#if !defined(_FUSE_LOG_H_)
#define _FUSE_LOG_H_

/* FUSE logging API, appeared on FUSE 3.7. */

#include <stdarg.h>
#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

enum fuse_log_level {
	FUSE_LOG_EMERG,
	FUSE_LOG_ALERT,
	FUSE_LOG_CRIT,
	FUSE_LOG_ERR,
	FUSE_LOG_WARNING,
	FUSE_LOG_NOTICE,
	FUSE_LOG_INFO,
	FUSE_LOG_DEBUG
};

typedef void (*fuse_log_func_t)(enum fuse_log_level level, const char *fmt, va_list ap);

void fuse_set_log_func(fuse_log_func_t func);
void fuse_log(enum fuse_log_level level, const char *fmt, ...) __printflike(2, 3);

#ifdef __cplusplus
}
#endif

#endif
