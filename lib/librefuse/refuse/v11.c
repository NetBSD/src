/* $NetBSD: v11.c,v 1.1 2022/01/22 08:09:40 pho Exp $ */

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
__RCSID("$NetBSD: v11.c,v 1.1 2022/01/22 08:09:40 pho Exp $");
#endif /* !lint */

#include <err.h>
#include <fuse_internal.h>
#include <stdlib.h>
#include <string.h>

/* FUSE < 3.0 had a very strange interface. Filesystems were supposed
 * to be mounted first, before creating an instance of struct
 * fuse. They revised the interface SO MANY TIMES but the fundamental
 * weirdness stayed the same. */
int
fuse_mount_v11(const char *mountpoint, const char *argv[]) {
    struct fuse_args args = FUSE_ARGS_INIT(0, NULL);
    int nominal_fd = -1;

    /* The argv is supposed to be a NULL-terminated array of
     * additional arguments to fusermount(8), and should not have a
     * program name at argv[0]. Our __fuse_new() expects one. So
     * prepend a dummy name. */
    if (fuse_opt_add_arg(&args, "dummy") != 0)
        goto free_args;

    if (argv) {
        for (size_t i = 0; argv[i] != NULL; i++) {
            if (fuse_opt_add_arg(&args, argv[i]) != 0)
                goto free_args;
        }
    }

    nominal_fd = fuse_mount_v25(mountpoint, &args);

free_args:
    fuse_opt_free_args(&args);
    return nominal_fd;
}

static bool
is_same_mountpoint(struct fuse_chan* chan, void* priv) {
    const char* mountpoint = priv;

    return strcmp(fuse_chan_mountpoint(chan), mountpoint) == 0;
}

static bool
is_same_fuse(struct fuse_chan* chan, void* priv) {
    struct fuse* fuse = priv;

    return fuse_chan_fuse(chan) == fuse;
}

/* FUSE < 3.0 didn't require filesystems to call fuse_unmount()
 * before fuse_destroy(). That is, it was completely legal to call
 * fuse_unmount() *after* fuse_destroy(), and it was even legal to
 * call fuse_mount() and then fuse_unmount() without calling
 * fuse_new() in the first place. On the other hand, our libpuffs
 * (like FUSE 3.0) wants a context in order to unmount a
 * filesystem. So, we have to do a workaround as follows:
 *
 * 1. fuse_mount() creates a struct fuse_chan and stashes it in a
 *    global channel list, but without actually mounting a filesystem.
 *
 * 2. fuse_new() fetches the stashed fuse_chan and creates a fuse
 *    object out of it, then mounts a filesystem. The fuse object is
 *    also stored in fuse_chan.
 *
 * 3. When fuse_destroy() is called without first unmounting the
 *    filesystem, it doesn't actually destroy the fuse object but it
 *    merely schedules it for destruction.
 *
 * 4. fuse_unmount() searches for the corresponding fuse_chan in the
 *    global list. If it's scheduled for destruction, destroy the fuse
 *    object after unmounting the filesystem. It then removes and
 *    deallocates the fuse_chan from the list.
 *
 * Note that there will be a space leak if a user calls fuse_destroy()
 * but never calls fuse_unmount(). The fuse_chan will forever be in
 * the global list in this case. There's nothing we can do about it,
 * and users aren't supposed to do it after all.
 */
void
fuse_unmount_v11(const char *mountpoint) {
    int idx;
    struct fuse_chan* chan;
    struct fuse* fuse;

    /* Search for the fuse_chan having the given mountpoint. It must
     * be in the global list. */
    chan = fuse_chan_find(is_same_mountpoint, &idx, __UNCONST(mountpoint));
    if (!chan)
        errx(EXIT_FAILURE, "%s: cannot find a channel for the mountpoint: %s",
             __func__, mountpoint);

    fuse = fuse_chan_fuse(chan);
    if (fuse) {
        /* The user did call fuse_new() after fuse_mount(). */
        fuse_unmount_v30(fuse);
    }

    if (fuse_chan_is_to_be_destroyed(chan)) {
        /* The user called fuse_destroy() before
         * fuse_unmount(). Destroy it now. */
        fuse_destroy_v30(fuse);
    }

    /* Remove the channel from the global list so that fuse_destroy(),
     * if it's called after this, can know that it's already been
     * unmounted. */
    fuse_chan_take(idx);
    fuse_chan_destroy(chan);
}

struct fuse *
fuse_new_v11(int fd, int flags, const void *op, int op_version) {
    const char *opts = NULL;

    /* FUSE_DEBUG was the only option allowed in this era. */
    if (flags & FUSE_DEBUG)
        opts = "debug";

    return fuse_new_v21(fd, opts, op, op_version, NULL);
}

void
fuse_destroy_v11(struct fuse *fuse) {
    struct fuse_chan* chan;

    /* Search for the fuse_chan that was used while creating this
     * struct fuse*. If it's not there it means the filesystem was
     * first unmounted before destruction. */
    chan = fuse_chan_find(is_same_fuse, NULL, fuse);
    if (chan) {
        /* The filesystem is still mounted and the user may later call
         * fuse_unmount() on it. Can't destroy the fuse object atm. */
        fuse_chan_set_to_be_destroyed(chan, true);
    }
    else {
        /* It's already been unmounted. Safe to destroy the fuse
         * object right now. */
        fuse_destroy_v30(fuse);
    }
}

int
fuse_loop_mt_v11(struct fuse *fuse) {
    return __fuse_loop_mt(fuse, 0);
}
