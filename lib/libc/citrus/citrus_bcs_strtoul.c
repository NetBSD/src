/* $NetBSD: citrus_bcs_strtoul.c,v 1.3.16.1 2013/06/23 06:21:04 tls Exp $ */

/*-
 * Copyright (c) 2005 The DragonFly Project.  All rights reserved.
 * Copyright (c) 2003, 2008 Citrus Project,
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
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if !defined(lint)
__RCSID("$NetBSD: citrus_bcs_strtoul.c,v 1.3.16.1 2013/06/23 06:21:04 tls Exp $");
#endif /* not lint */

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

#include "citrus_namespace.h"
#include "citrus_bcs.h"

#define	BCS_ONLY

#define	_FUNCNAME	_bcs_strtoul
#define	__UINT		unsigned long int
#define	__UINT_MAX	ULONG_MAX

#undef isspace
#define isspace(c)	_bcs_isspace(c)

#undef isdigit
#define isdigit(c)	_bcs_isdigit(c)

#undef isalpha
#define isalpha(c)	_bcs_isalpha(c)

#undef isupper
#define isupper(c)	_bcs_isupper(c)

#include "_strtoul.h"
