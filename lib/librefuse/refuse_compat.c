/* $NetBSD: refuse_compat.c,v 1.3 2022/01/23 21:07:28 rillig Exp $ */

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
__RCSID("$NetBSD: refuse_compat.c,v 1.3 2022/01/23 21:07:28 rillig Exp $");
#endif /* !lint */

#include <fuse_internal.h>
#include <string.h>

/*
 * Compatibility symbols that had existed in old versions of
 * librefuse.
 */

struct fuse_cmdline_opts_rev0 {
	int singlethread;
	int foreground;
	int debug;
	int nodefault_fsname;
	char *mountpoint;
	int show_version;
	int show_help;
};

int fuse_daemonize_rev0(struct fuse* fuse) __RENAME(fuse_daemonize);
int fuse_mount(struct fuse *fuse, const char *mountpoint);
int fuse_main_real(int argc, char **argv, const struct fuse_operations_v26 *op,
                   size_t size, void *user_data);
struct fuse *fuse_new(struct fuse_args *args, const void *op, size_t op_size,
                      void *user_data);
void fuse_destroy(struct fuse* fuse);
int fuse_parse_cmdline(struct fuse_args *args, struct fuse_cmdline_opts_rev0 *opts);
void fuse_unmount(struct fuse* fuse);
void fuse_unmount_compat22(const char *mountpoint);

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

/* librefuse once had a function fuse_main_real() which was specific
 * to FUSE 2.6 API. */
__warn_references(
    fuse_main_real,
    "warning: reference to compatibility fuse_main_real();"
    " include <fuse.h> for correct reference")
int
fuse_main_real(int argc, char **argv, const struct fuse_operations_v26 *op,
               size_t size __attribute__((__unused__)), void *user_data) {
    return __fuse_main(argc, argv, op, 26, user_data);
}

/* librefuse once had a function fuse_mount() for FUSE 3.0 but without
 * a version postfix. */
__warn_references(
    fuse_mount,
    "warning: reference to compatibility fuse_mount();"
    " include <fuse.h> for correct reference")
int
fuse_mount(struct fuse *fuse, const char *mountpoint) {
    return fuse_mount_v30(fuse, mountpoint);
}

/* librefuse once had a function fuse_new() for FUSE 3.0 but without a
 * version postfix. */
__warn_references(
    fuse_new,
    "warning: reference to compatibility fuse_new();"
    " include <fuse.h> for correct reference")
struct fuse *
fuse_new(struct fuse_args *args, const void *op,
         size_t op_size __attribute__((__unused__)),
         void *user_data) {
    return fuse_new_v30(args, op, 30, user_data);
}

/* librefuse once had a function fuse_destroy() for FUSE 3.0 but
 * without a version postfix. */
__warn_references(
    fuse_new,
    "warning: reference to compatibility fuse_destroy();"
    " include <fuse.h> for correct reference")
void
fuse_destroy(struct fuse *fuse) {
    fuse_destroy_v30(fuse);
}

/* librefuse once had a function fuse_parse_cmdline() for FUSE 3.0 but
 * without a version postfix. It expected an old definition of struct
 * fuse_cmdline_opts too. */
__warn_references(
    fuse_parse_cmdline,
    "warning: reference to compatibility fuse_parse_cmdline();"
    " include <fuse.h> for correct reference")
int
fuse_parse_cmdline(struct fuse_args *args, struct fuse_cmdline_opts_rev0 *opts) {
    struct fuse_cmdline_opts tmp;

    if (fuse_parse_cmdline_v30(args, &tmp) != 0)
        return -1;

    memcpy(opts, &tmp, sizeof(*opts));
    return 0;
}

/* librefuse once had a function fuse_unmount() for FUSE 3.0 but
 * without a version postfix. */
__warn_references(
    fuse_unmount,
    "warning: reference to compatibility fuse_unmount();"
    " include <fuse.h> for correct reference")
void
fuse_unmount(struct fuse* fuse) {
    fuse_unmount_v30(fuse);
}

/* librefuse once had a function fuse_unmount_compat22() which was an
 * implementation of fuse_unmount() to be used when FUSE_USE_VERSION
 * was set to 22 or below. The function was actually a no-op. */
void
fuse_unmount_compat22(const char *mountpoint) {
    fuse_unmount_v11(mountpoint);
}
