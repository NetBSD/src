/* $NetBSD: session.h,v 1.1 2022/01/22 07:53:06 pho Exp $ */

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
#if !defined(_FUSE_SESSION_H_)
#define _FUSE_SESSION_H_

/* FUSE session API
 */

#if !defined(FUSE_H_)
#  error Do not include this header directly. Include <fuse.h> instead.
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
struct fuse;

/* A private structure appeared on FUSE 2.4. */
struct fuse_session;

/* Get a session from a fuse object. Appeared on FUSE 2.6. */
struct fuse_session *fuse_get_session(struct fuse *f);

/* Get the file descriptor for communicaiton with kernel. Appeared on
 * FUSE 3.0. */
int fuse_session_fd(struct fuse_session *se);

/* Exit a session on SIGHUP, SIGTERM, and SIGINT and ignore
 * SIGPIPE. Appeared on FUSE 2.5. */
int fuse_set_signal_handlers(struct fuse_session *se);

/* Restore default signal handlers. Appeared on FUSE 2.5. */
void fuse_remove_signal_handlers(struct fuse_session *se);

#ifdef __cplusplus
}
#endif

#endif
