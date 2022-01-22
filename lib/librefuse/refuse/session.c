/* $NetBSD: session.c,v 1.1 2022/01/22 07:53:06 pho Exp $ */

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
__RCSID("$NetBSD: session.c,v 1.1 2022/01/22 07:53:06 pho Exp $");
#endif /* !lint */

#include <err.h>
#include <fuse_internal.h>
#include <puffs.h>

/* The documentation for FUSE is not clear as to what "struct fuse_session" is,
 * why it exists, or how it's different from "struct fuse". For now we leave it
 * undefined (i.e. an incomplete type) and treat "struct fuse_session *" as
 * being identical to "struct fuse *". */

struct fuse_session *
fuse_get_session(struct fuse *f) {
    return (struct fuse_session*)f;
}

int
fuse_session_fd(struct fuse_session *se) {
    struct fuse* fuse = (struct fuse*)se;

    /* We don't want to expose this to users, but filesystems in the wild often
     * wants to set FD_CLOEXEC on it. Hope they don't assume it's the real
     * /dev/fuse, because it's actually /dev/puffs in our implementation. */
    return puffs_getselectable(fuse->pu);
}

int
fuse_set_signal_handlers(struct fuse_session *se) {
    return __fuse_set_signal_handlers((struct fuse*)se);
}

void
fuse_remove_signal_handlers(struct fuse_session *se) {
    if (__fuse_remove_signal_handlers((struct fuse*)se) == -1)
        warn("%s: failed to remove signal handlers", __func__);
}
