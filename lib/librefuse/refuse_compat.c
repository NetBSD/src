/* $NetBSD: refuse_compat.c,v 1.1 2022/01/22 08:02:49 pho Exp $ */

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

#include <sys/cdefs.h>
#if !defined(lint)
__RCSID("$NetBSD: refuse_compat.c,v 1.1 2022/01/22 08:02:49 pho Exp $");
#endif /* !lint */

#include <fuse_internal.h>

/*
 * Compatibility symbols that had existed in old versions of
 * librefuse.
 */

int fuse_daemonize_rev0(struct fuse* fuse) __RENAME(fuse_daemonize);

/* librefuse once had a function fuse_daemonize() with an incompatible
 * prototype with that of FUSE. We keep ABI compatibility with
 * executables compiled against old librefuse by having this shim
 * function as a symbol "fuse_daemonize". However, we can NOT keep API
 * compatibility with old code expecting the wrong prototype. The only
 * way to work around the problem is to put "#if FUSE_H_ < 20211204"
 * into old user code. pho@ really regrets this mistake... */
__warn_references(
    fuse_daemonize,
    "warning: reference to compatibility fuse_daemonize();"
    " include <fuse.h> for correct reference")
int
fuse_daemonize_rev0(struct fuse* fuse __attribute__((__unused__))) {
    return fuse_daemonize(1);
}
