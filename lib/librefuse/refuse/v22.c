/* $NetBSD: v22.c,v 1.1 2022/01/22 08:09:40 pho Exp $ */

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
__RCSID("$NetBSD: v22.c,v 1.1 2022/01/22 08:09:40 pho Exp $");
#endif /* !lint */

#include <fuse_internal.h>

struct fuse *
fuse_setup_v22(int argc, char *argv[], const void *op, int op_version,
               char **mountpoint, int *multithreaded, int *fd) {
    /*
     * This is conceptually the part of fuse_main() before the event
     * loop. However, FUSE 2.2 fuse_setup() takes a pointer to store a
     * channel fd, which is supposed to be obtained by calling FUSE
     * 2.1 fuse_mount(). The problem is that we don't really have such
     * a thing as channel fd. Luckily for us, the only valid use of
     * the channel fd is to pass to fuse_new(), which is a part of
     * fuse_setup() itself. So it should be okay to just put a dummy
     * value there.
     */
    struct fuse* fuse;

    fuse = fuse_setup_v26(argc, argv, op, op_version, mountpoint, multithreaded, NULL);
    if (fuse == NULL)
        return NULL;

    *fd = -1;
    return fuse;
}

void
fuse_teardown_v22(struct fuse *fuse,
                  int fd __attribute__((__unused__)),
                  char *mountpoint __attribute__((__unused__))) {
    __fuse_teardown(fuse);
}
