/* $NetBSD: v25.c,v 1.1 2022/01/22 08:09:40 pho Exp $ */

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
__RCSID("$NetBSD: v25.c,v 1.1 2022/01/22 08:09:40 pho Exp $");
#endif /* !lint */

#include <err.h>
#include <fuse_internal.h>
#include <stdbool.h>
#include <string.h>

int
fuse_mount_v25(const char *mountpoint, struct fuse_args *args) {
    struct fuse_chan* chan;

    /* Of course we cannot accurately emulate the semantics, so we
     * save arguments and use them later in fuse_new(). */
    chan = fuse_chan_new(mountpoint, args);
    if (!chan)
        return -1;

    /* But, the return type of this function is just an int which is
     * supposed to be a file descriptor. It's too narrow to store a
     * pointer ofc, so... */
    return fuse_chan_stash(chan);
}

int
fuse_parse_cmdline_v25(struct fuse_args *args, char **mountpoint,
                       int *multithreaded, int *foreground) {
    struct fuse_cmdline_opts opts;

    if (fuse_parse_cmdline_v30(args, &opts) != 0)
        return -1;

    *mountpoint    = opts.mountpoint; /* Transfer the ownership of the string. */
    *multithreaded = !opts.singlethread;
    *foreground    = opts.foreground;
    return 0;
}

/* The interface of fuse_mount() and fuse_new() became even stranger
 * in FUSE 2.5. Now they both take "struct fuse_args" which are
 * expected to be identical between those two calls (which isn't
 * explained clearly in the documentation). Our implementation just
 * assume they are identical, because we can't recover from situations
 * where they differ, as there is no obvious way to merge them
 * together. */
struct fuse *
fuse_new_v25(int fd, struct fuse_args *args, const void *op,
             int op_version, void *user_data) {
    struct fuse_chan* chan;
    struct fuse_args* mount_args;
    bool args_differ = false;

    /* But at least we can emit a warning when they differ... */
    chan = fuse_chan_peek(fd);
    if (!chan) {
        warnx("%s: invalid channel: %d", __func__, fd);
        return NULL;
    }

    mount_args = fuse_chan_args(chan);

    if (mount_args->argc != args->argc) {
        args_differ = true;
    }
    else {
        int i;

        for (i = 0; i < args->argc; i++) {
            if (strcmp(mount_args->argv[i], args->argv[i]) != 0) {
                args_differ = true;
                break;
            }
        }
    }

    if (args_differ) {
        warnx("%s: the argument vector differs from "
              "that were passed to fuse_mount()", __func__);
    }

    return fuse_new_v21(fd, NULL, op, op_version, user_data);
}
