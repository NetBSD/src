/*	$NetBSD: compat_file.h,v 1.2 2003/01/22 17:48:02 christos Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_COMPAT_FILE_H_
#define	_COMPAT_FILE_H_

int bsd_sys_open(struct lwp *, void *, register_t *);
int bsd_compat_43_sys_creat(struct lwp *, void *, register_t *);
int bsd_sys_link(struct lwp *, void *, register_t *);
int bsd_sys_unlink(struct lwp *, void *, register_t *);
int bsd_sys_chdir(struct lwp *, void *, register_t *);
int bsd_sys_mknod(struct lwp *, void *, register_t *);
int bsd_sys_chmod(struct lwp *, void *, register_t *);
int bsd_sys_chown(struct lwp *, void *, register_t *);
int bsd_sys_mount(struct lwp *, void *, register_t *);
int bsd_sys_unmount(struct lwp *, void *, register_t *);
int bsd_sys_access(struct lwp *, void *, register_t *);
int bsd_sys_chflags(struct lwp *, void *, register_t *);
int bsd_compat_43_sys_stat(struct lwp *, void *, register_t *);
int bsd_compat_43_sys_lstat(struct lwp *, void *, register_t *);
int bsd_sys_acct(struct lwp *, void *, register_t *);
int bsd_sys_revoke(struct lwp *, void *, register_t *);
int bsd_sys_symlink(struct lwp *, void *, register_t *);
int bsd_sys_readlink(struct lwp *, void *, register_t *);
int bsd_sys_execve(struct lwp *, void *, register_t *);
int bsd_sys_chroot(struct lwp *, void *, register_t *);
int bsd_compat_12_sys_swapon(struct lwp *, void *, register_t *);
int bsd_sys_rename(struct lwp *, void *, register_t *);
int bsd_compat_43_sys_truncate(struct lwp *, void *, register_t *);
int bsd_sys_mkfifo(struct lwp *, void *, register_t *);
int bsd_sys_mkdir(struct lwp *, void *, register_t *);
int bsd_sys_rmdir(struct lwp *, void *, register_t *);
int bsd_sys_utimes(struct lwp *, void *, register_t *);
int bsd_sys_quotactl(struct lwp *, void *, register_t *);
int bsd_sys_statfs(struct lwp *, void *, register_t *);
int bsd_sys_getfh(struct lwp *, void *, register_t *);
int bsd_compat_12_sys_stat(struct lwp *, void *, register_t *);
int bsd_compat_12_sys_lstat(struct lwp *, void *, register_t *);
int bsd_sys_pathconf(struct lwp *, void *, register_t *);
int bsd_sys_truncate(struct lwp *, void *, register_t *);
int bsd_sys_undelete(struct lwp *, void *, register_t *);
int bsd_sys_lchmod(struct lwp *, void *, register_t *);
int bsd_sys_lchown(struct lwp *, void *, register_t *);
int bsd_sys_lutimes(struct lwp *, void *, register_t *);
int bsd_sys___stat13(struct lwp *, void *, register_t *);
int bsd_sys___lstat13(struct lwp *, void *, register_t *);
int bsd_sys___posix_chown(struct lwp *, void *, register_t *);
int bsd_sys___posix_lchown(struct lwp *, void *, register_t *);
int bsd_sys_lchflags(struct lwp *, void *, register_t *);

#endif /* _COMPAT_FILE_H_ */
