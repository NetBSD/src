/*      $NetBSD: hijackdlsym.c,v 1.3.2.1 2014/08/20 00:02:21 tls Exp $	*/

/*-
 * Copyright (c) 2011 Antti Kantee.  All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <rump/rumpuser_port.h>

#if !defined(lint)
__RCSID("$NetBSD: hijackdlsym.c,v 1.3.2.1 2014/08/20 00:02:21 tls Exp $");
#endif

#include <dlfcn.h>

#include "hijack.h"

/*
 * This is called from librumpclient in case of LD_PRELOAD.
 * It ensures correct RTLD_NEXT.
 *
 * (note, this module is compiled with -O0 to make sure this
 * function is not treated as a tailcall or other optimizations
 * applied)
 */

/* why is this indirection required for powerpc ???? */
static void * __noinline
bouncer(void *handle, const char *symbol)
{

	return dlsym(handle, symbol);
}

void *
rumphijack_dlsym(void *handle, const char *symbol)
{

	return bouncer(handle, symbol);
}
