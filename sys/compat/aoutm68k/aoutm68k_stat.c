/*	$NetBSD: aoutm68k_stat.c,v 1.20.16.1 2007/12/26 19:48:41 ad Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: aoutm68k_stat.c,v 1.20.16.1 2007/12/26 19:48:41 ad Exp $");

#if defined(_KERNEL_OPT)
#include "opt_compat_netbsd.h"
#include "opt_compat_43.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/vfs_syscalls.h>

#include <sys/syscall.h>
#include <sys/syscallargs.h>

#include <compat/sys/stat.h>

#include <compat/aoutm68k/aoutm68k_util.h>
#include <compat/aoutm68k/aoutm68k_stat.h>
#include <compat/aoutm68k/aoutm68k_syscall.h>
#include <compat/aoutm68k/aoutm68k_syscallargs.h>

#ifdef COMPAT_43
static void aoutm68k_stat43_convert(struct stat *, struct aoutm68k_stat43 *);
#endif
#ifdef COMPAT_12
static void aoutm68k_stat12_convert(struct stat *, struct aoutm68k_stat12 *);
#endif
static void aoutm68k_stat13_convert(struct stat *, struct aoutm68k_stat *);


#ifdef COMPAT_43
int
aoutm68k_compat_43_sys_stat(struct lwp *l, const struct aoutm68k_compat_43_sys_stat_args *uap, register_t *retval)
{
	struct aoutm68k_stat43 ast;
	struct stat sb;
	int error;

	error = do_sys_stat(l, SCARG(uap, path), FOLLOW, &sb);
	if (error)
		return error;

	aoutm68k_stat43_convert(&sb, &ast);

	return copyout(&ast, SCARG(uap, ub), sizeof(ast));
}

int
aoutm68k_compat_43_sys_fstat(struct lwp *l, const struct aoutm68k_compat_43_sys_fstat_args *uap, register_t *retval)
{
	struct aoutm68k_stat43 ast;
	struct stat sb;
	int error;

	error = do_sys_fstat(l, SCARG(uap, fd), &sb);
	if (error != 0)
		return error;

	aoutm68k_stat43_convert(&sb, &ast);

	return copyout(&ast, SCARG(uap, sb), sizeof(ast));
}

int
aoutm68k_compat_43_sys_lstat(struct lwp *l, const struct aoutm68k_compat_43_sys_lstat_args *uap, register_t *retval)
{
	struct aoutm68k_stat43 ast;
	struct stat sb;
	int error;

	error = do_sys_stat(l, SCARG(uap, path), NOFOLLOW, &sb);
	if (error)
		return error;

	aoutm68k_stat43_convert(&sb, &ast);

	return copyout(&ast, SCARG(uap, ub), sizeof(ast));
}
#endif /* COMPAT_43 */

#ifdef COMPAT_12
int
aoutm68k_compat_12_sys_stat(struct lwp *l, const struct aoutm68k_compat_12_sys_stat_args *uap, register_t *retval)
{
	struct aoutm68k_stat12 ast;
	struct stat sb;
	int error;

	error = do_sys_stat(l, SCARG(uap, path), FOLLOW, &sb);
	if (error)
		return error;

	aoutm68k_stat12_convert(&sb, &ast);

	return copyout(&ast, SCARG(uap, ub), sizeof(ast));
}

int
aoutm68k_compat_12_sys_fstat(struct lwp *l, const struct aoutm68k_compat_12_sys_fstat_args *uap, register_t *retval)
{
	struct aoutm68k_stat12 ast;
	struct stat sb;
	int error;

	error = do_sys_fstat(l, SCARG(uap, fd), &sb);
	if (error != 0)
		return error;

	aoutm68k_stat12_convert(&sb, &ast);

	return copyout(&ast, SCARG(uap, sb), sizeof(ast));
}

int
aoutm68k_compat_12_sys_lstat(struct lwp *l, const struct aoutm68k_compat_12_sys_lstat_args *uap, register_t *retval)
{
	struct aoutm68k_stat12 ast;
	struct stat sb;
	int error;

	error = do_sys_stat(l, SCARG(uap, path), NOFOLLOW, &sb);
	if (error)
		return error;

	aoutm68k_stat12_convert(&sb, &ast);

	return copyout(&ast, SCARG(uap, ub), sizeof(ast));
}
#endif /* COMPAT_12 */

int
aoutm68k_sys___stat13(struct lwp *l, const struct aoutm68k_sys___stat13_args *uap, register_t *retval)
{
	struct aoutm68k_stat ast;
	struct stat sb;
	int error;

	error = do_sys_stat(l, SCARG(uap, path), FOLLOW, &sb);
	if (error)
		return error;

	aoutm68k_stat13_convert(&sb, &ast);

	return copyout(&ast, SCARG(uap, ub), sizeof(ast));
}

int
aoutm68k_sys___fstat13(struct lwp *l, const struct aoutm68k_sys___fstat13_args *uap, register_t *retval)
{
	struct aoutm68k_stat ast;
	struct stat sb;
	int error;

	error = do_sys_fstat(l, SCARG(uap, fd), &sb);
	if (error != 0)
		return error;

	aoutm68k_stat13_convert(&sb, &ast);

	return copyout(&ast, SCARG(uap, sb), sizeof(ast));

}

int
aoutm68k_sys___lstat13(struct lwp *l, const struct aoutm68k_sys___lstat13_args *uap, register_t *retval)
{
	struct aoutm68k_stat ast;
	struct stat sb;
	int error;

	error = do_sys_stat(l, SCARG(uap, path), NOFOLLOW, &sb);
	if (error)
		return error;

	aoutm68k_stat13_convert(&sb, &ast);

	return copyout(&ast, SCARG(uap, ub), sizeof(ast));
}

int
aoutm68k_sys_fhstat(struct lwp *l, const struct aoutm68k_sys_fhstat_args *uap, register_t *retval)
{
	struct aoutm68k_stat ast;
	struct stat sb;
	int error;

	error = do_fhstat(l, SCARG(uap, fhp), FHANDLE_SIZE_COMPAT, &sb);
	if (error)
		return error;

	aoutm68k_stat13_convert(&sb, &ast);
	return copyout(&sb, SCARG(uap, sb), sizeof(sb));
}

#ifdef COMPAT_43
static void
aoutm68k_stat43_convert(struct stat *st, struct aoutm68k_stat43 *ast)
{

	memset(ast, 0, sizeof(*ast));
	ast->st_dev = st->st_dev;
	ast->st_ino = st->st_ino;
	ast->st_mode = st->st_mode;
	ast->st_nlink = st->st_nlink;
	ast->st_uid = st->st_uid;
	ast->st_gid = st->st_gid;
	ast->st_rdev = st->st_rdev;
	if (st->st_size < (off_t)1 << 32)
		ast->st_size = st->st_size;
	else
		ast->st_size = -2;
	ast->st_atimespec.tv_sec = st->st_atimespec.tv_sec;
	ast->st_atimespec.tv_nsec = st->st_atimespec.tv_nsec;
	ast->st_mtimespec.tv_sec = st->st_mtimespec.tv_sec;
	ast->st_mtimespec.tv_nsec = st->st_mtimespec.tv_nsec;
	ast->st_ctimespec.tv_sec = st->st_ctimespec.tv_sec;
	ast->st_ctimespec.tv_nsec = st->st_ctimespec.tv_nsec;
	ast->st_blksize = st->st_blksize;
	ast->st_blocks = st->st_blocks;
	ast->st_flags = st->st_flags;
	ast->st_gen = st->st_gen;
}
#endif /* COMPAT_43 */

#ifdef COMPAT_12
static void
aoutm68k_stat12_convert(struct stat *st, struct aoutm68k_stat12 *ast)
{

	memset(ast, 0, sizeof(*ast));
	ast->st_dev = st->st_dev;
	ast->st_ino = st->st_ino;
	ast->st_mode = st->st_mode;
	ast->st_nlink = st->st_nlink;
	ast->st_uid = st->st_uid;
	ast->st_gid = st->st_gid;
	ast->st_rdev = st->st_rdev;
	ast->st_atimespec.tv_sec = st->st_atimespec.tv_sec;
	ast->st_atimespec.tv_nsec = st->st_atimespec.tv_nsec;
	ast->st_mtimespec.tv_sec = st->st_mtimespec.tv_sec;
	ast->st_mtimespec.tv_nsec = st->st_mtimespec.tv_nsec;
	ast->st_ctimespec.tv_sec = st->st_ctimespec.tv_sec;
	ast->st_ctimespec.tv_nsec = st->st_ctimespec.tv_nsec;
	if (st->st_size < (off_t)1 << 32)
		ast->st_size = st->st_size;
	else
		ast->st_size = -2;
	ast->st_blocks = st->st_blocks;
	ast->st_blksize = st->st_blksize;
	ast->st_flags = st->st_flags;
	ast->st_gen = st->st_gen;
}
#endif /* COMPAT_12 */

static void
aoutm68k_stat13_convert(struct stat *st, struct aoutm68k_stat *ast)
{

	memset(ast, 0, sizeof(*ast));
	ast->st_dev = st->st_dev;
	ast->st_ino = st->st_ino;
	ast->st_mode = st->st_mode;
	ast->st_nlink = st->st_nlink;
	ast->st_uid = st->st_uid;
	ast->st_gid = st->st_gid;
	ast->st_rdev = st->st_rdev;
	ast->st_atimespec.tv_sec = st->st_atimespec.tv_sec;
	ast->st_atimespec.tv_nsec = st->st_atimespec.tv_nsec;
	ast->st_mtimespec.tv_sec = st->st_mtimespec.tv_sec;
	ast->st_mtimespec.tv_nsec = st->st_mtimespec.tv_nsec;
	ast->st_ctimespec.tv_sec = st->st_ctimespec.tv_sec;
	ast->st_ctimespec.tv_nsec = st->st_ctimespec.tv_nsec;
	if (st->st_size < (off_t)1 << 32)
		ast->st_size = st->st_size;
	else
		ast->st_size = -2;
	ast->st_blocks = st->st_blocks;
	ast->st_blksize = st->st_blksize;
	ast->st_flags = st->st_flags;
	ast->st_gen = st->st_gen;
	ast->st_qspare[0] = 0;
	ast->st_qspare[1] = 0;
}
