/* $NetBSD: v26.c,v 1.1 2022/01/22 08:09:40 pho Exp $ */

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
__RCSID("$NetBSD: v26.c,v 1.1 2022/01/22 08:09:40 pho Exp $");
#endif /* !lint */

#include <err.h>
#include <fuse_internal.h>
#include <stdlib.h>
#include <string.h>

struct fuse_chan *
fuse_mount_v26(const char *mountpoint, struct fuse_args *args) {
    int nominal_fd;

    /* fuse_mount() in FUSE 2.6 returns a fuse_chan instead of fd. We
     * still need to store the channel in the global list, because
     * users may call fuse_destroy() before fuse_unmount().
     */
    nominal_fd = fuse_mount_v25(mountpoint, args);
    if (nominal_fd == -1)
        return NULL;

    return fuse_chan_peek(nominal_fd);
}

static bool
is_same_channel(struct fuse_chan* chan, void* priv) {
    return chan == (struct fuse_chan*)priv;
}

void
fuse_unmount_v26(const char *mountpoint, struct fuse_chan *ch) {
    /* Although the API documentation doesn't say so, fuse_unmount()
     * from FUSE >= 2.6 < 3.0 in fact allows "ch" to be NULL. */
    if (ch)
        if (strcmp(mountpoint, fuse_chan_mountpoint(ch)) != 0)
            warnx("%s: mountpoint `%s' differs from that was passed to fuse_mount(): %s",
                  __func__, mountpoint, fuse_chan_mountpoint(ch));

    /* Ask fuse_unmount_v11() to find the channel object that is
     * already in our hand. We are going to need to know its index in
     * the global list anyway. */
    fuse_unmount_v11(mountpoint);
}

struct fuse *
fuse_new_v26(struct fuse_chan *ch, struct fuse_args *args,
             const void *op, int op_version,
             void *user_data) {
    int idx;

    /* Although the fuse_chan object is already in our hand, we need
     * to know its index in the global list because that's what
     * fuse_new_v25() wants. */
    if (fuse_chan_find(is_same_channel, &idx, ch) == NULL)
        errx(EXIT_FAILURE, "%s: cannot find the channel index", __func__);

    return fuse_new_v25(idx, args, op, op_version, user_data);
}

struct fuse *
fuse_setup_v26(int argc, char *argv[],
               const void *op, int op_version,
               char **mountpoint, int *multithreaded,
               void *user_data) {
    struct fuse* fuse;
    struct fuse_cmdline_opts opts;

    fuse = __fuse_setup(argc, argv, op, op_version, user_data, &opts);
    if (fuse == NULL)
        return NULL;

    *mountpoint = opts.mountpoint; /* Transfer the ownership of the string. */
    *multithreaded = !opts.singlethread;
    return fuse;
}

void
fuse_teardown_v26(struct fuse *fuse,
                  char *mountpoint __attribute__((__unused__))) {
    __fuse_teardown(fuse);
}
