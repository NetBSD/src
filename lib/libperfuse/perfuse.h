/*  $NetBSD: perfuse.h,v 1.2 2016/01/22 22:25:50 dholland Exp $ */

/*-
 *  Copyright (c) 2010 Emmanuel Dreyfus. All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 
 *  THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */ 

#ifndef _REFUSE_PERFUSE_WRAPPER_H
#define _REFUSE_PERFUSE_WRAPPER_H

#include <sys/cdefs.h>
#include <sys/types.h>

__BEGIN_DECLS

int perfuse_open(const char *, int, mode_t);
int perfuse_mount(const char *, const char *, const char *, long, const void *);

__END_DECLS

#ifndef LIBPERFUSE
#define mount(source, mnt, type, flags, optbuf) \
        perfuse_mount(source, mnt, type, flags, optbuf)
#define open(path, flags) \
        perfuse_open(path, flags, 0)
#endif /* LIBPERFUSE */

#endif /* _REFUSE_PERFUSE_WRAPPER_H */
