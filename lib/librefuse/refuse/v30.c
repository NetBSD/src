/* $NetBSD: v30.c,v 1.1 2022/01/22 08:09:40 pho Exp $ */

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
__RCSID("$NetBSD: v30.c,v 1.1 2022/01/22 08:09:40 pho Exp $");
#endif /* !lint */

#include <fuse_internal.h>

int
fuse_mount_v30(struct fuse *fuse, const char *mountpoint) {
    return __fuse_mount(fuse, mountpoint);
}

void
fuse_unmount_v30(struct fuse *fuse) {
    __fuse_unmount(fuse);
}

struct fuse *
fuse_new_v30(struct fuse_args *args,
             const void *op, int op_version, void *user_data) {
    return __fuse_new(args, op, op_version, user_data);
}

void
fuse_destroy_v30(struct fuse *fuse) {
    return __fuse_destroy(fuse);
}

int
fuse_loop_mt_v30(struct fuse *fuse, int clone_fd) {
    struct fuse_loop_config config = {
        .clone_fd         = clone_fd,
        .max_idle_threads = 10 /* The default value when "config" is NULL. */
    };
    return __fuse_loop_mt(fuse, &config);
}

int
fuse_parse_cmdline_v30(struct fuse_args *args, struct fuse_cmdline_opts *opts) {
    return __fuse_parse_cmdline(args, opts);
}
