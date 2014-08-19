/*	$NetBSD: openpam_dlfunc.h,v 1.2.8.2 2014/08/19 23:52:07 tls Exp $	*/

/*-
 * Copyright (c) 2013 Dag-Erling Smørgrav
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
 * Id: openpam_dlfunc.h 660 2013-03-11 15:08:52Z des 
 */

#ifndef OPENPAM_DLFCN_H_INCLUDED
#define OPENPAM_DLFCN_H_INCLUDED

#ifndef HAVE_DLFUNC
/*-
 * The actual type declared by this typedef is immaterial, provided that
 * it is a function pointer.  Its purpose is to provide a return type for
 * dlfunc() which can be cast to a function pointer type without depending
 * on behavior undefined by the C standard, which might trigger a compiler
 * diagnostic.  We intentionally declare a unique type signature to force
 * a diagnostic should the application not cast the return value of dlfunc()
 * appropriately.       
 */
struct __dlfunc_arg {   
	int	__dlfunc_dummy; 
};

typedef void (*dlfunc_t)(struct __dlfunc_arg);

static inline dlfunc_t
dlfunc(void *handle, const char *symbol)
{

	return ((dlfunc_t)dlsym(handle, symbol));
}
#endif

#endif
