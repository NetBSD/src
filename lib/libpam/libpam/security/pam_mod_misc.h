/*	$NetBSD: pam_mod_misc.h,v 1.3.48.1 2012/04/17 00:05:29 yamt Exp $	*/

/*-
 * Copyright 1998 Juniper Networks, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	$FreeBSD: src/lib/libpam/libpam/security/pam_mod_misc.h,v 1.12 2003/05/31 16:56:35 des Exp $
 */

#ifndef PAM_MOD_MISC_H
#define PAM_MOD_MISC_H

#include <sys/cdefs.h>

/*
 * Common option names
 */
#define	PAM_OPT_NULLOK		"nullok"
#define PAM_OPT_AUTH_AS_SELF	"auth_as_self"
#define PAM_OPT_ECHO_PASS	"echo_pass"
#define PAM_OPT_DEBUG		"debug"

__BEGIN_DECLS
void	_pam_verbose_error(pam_handle_t *, int, const char *,
		const char *, const char *, ...) __printflike(5, 6);
__END_DECLS

#define	PAM_LOG(...) \
	openpam_log(PAM_LOG_DEBUG, __VA_ARGS__)

#define PAM_RETURN(arg) \
	return (arg)

#define PAM_VERBOSE_ERROR(...) \
	_pam_verbose_error(pamh, flags, __FILE__, __func__, __VA_ARGS__)

#endif
