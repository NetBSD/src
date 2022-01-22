/* $NetBSD: v21.c,v 1.1 2022/01/22 08:09:40 pho Exp $ */

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
__RCSID("$NetBSD: v21.c,v 1.1 2022/01/22 08:09:40 pho Exp $");
#endif /* !lint */

#include <err.h>
#include <fuse_internal.h>

/* Like FUSE 1.x, fuse_mount() is supposed to be called before
 * fuse_new(). The argument "opts" is a comma-separated string of
 * mount options. */
int
fuse_mount_v21(const char *mountpoint, const char *opts) {
    struct fuse_args args = FUSE_ARGS_INIT(0, NULL);
    int nominal_fd = -1;

    if (opts) {
        if (fuse_opt_add_arg(&args, "-o") != 0)
            goto free_args;

        if (fuse_opt_add_arg(&args, opts) != 0)
            goto free_args;
    }

    nominal_fd = fuse_mount_v25(mountpoint, &args);

  free_args:
    fuse_opt_free_args(&args);
    return nominal_fd;
}

struct fuse *
fuse_new_v21(int fd, const char *opts, const void *op,
             int op_version, void *user_data) {
    struct fuse_chan* chan;
    struct fuse* fuse;
    int rv;

    chan = fuse_chan_peek(fd);
    if (!chan) {
        warnx("%s: invalid channel: %d", __func__, fd);
        return NULL;
    }

    /* "opts" is another set of options to be interpreted by the FUSE
     * library, as opposed to the ones passed to fuse_mount() which is
     * handled by Linux kernel. We don't have such a distinction, so
     * insert them at argv[1] (not at the end of argv, because there
     * may be non-options there). */
    if (opts) {
        if (fuse_opt_insert_arg(fuse_chan_args(chan), 1, "-o") != 0) {
            fuse_chan_destroy(chan);
            return NULL;
        }
        if (fuse_opt_insert_arg(fuse_chan_args(chan), 2, opts) != 0) {
            fuse_chan_destroy(chan);
            return NULL;
        }
    }

    fuse = __fuse_new(fuse_chan_args(chan), op, op_version, user_data);
    if (!fuse) {
        fuse_chan_destroy(chan);
        return NULL;
    }
    fuse_chan_set_fuse(chan, fuse);

    /* Now we can finally mount it. Hope it's not too late. */
    rv = fuse_mount_v30(fuse, fuse_chan_mountpoint(chan));
    if (rv != 0) {
        fuse_destroy_v30(fuse);
        fuse_chan_destroy(chan);
        return NULL;
    }

    return fuse;
}
